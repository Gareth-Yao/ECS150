// #include "VirtualMachineUtils.c"
#include "VirtualMachine.h"
#include "Machine.h"
#include <iostream>
#include <vector>
#include <stack>
#include <queue>
#include <string.h>
#include <unordered_map>



extern "C"{

	volatile TVMTick currentTick = 0;
	static int msPerTick = 100;
	volatile static TVMThreadID currentThread = 1;
	void* sharedmemory;
	class TCB{
		public:
		TVMThreadID _tid;
		TVMMemoryPoolID _pid;
		TVMMutexID _mid;
		TVMThreadPriority _tPrio;
		TVMThreadState _tState;
		TVMThreadEntry _tEntry;
		void *_param;
		TVMMemorySize _tMemsize;
		void* _stackaddr;
		SMachineContext _machineContext;
		int _fileOPerationResult;
		TCB(TVMThreadID tid, TVMThreadPriority tPrio, TVMThreadEntry tEntry,
			void *param, TVMMemorySize tMemsize){
			_tid = tid;
			_pid = 512 * _tid;
			_mid = VM_MUTEX_ID_INVALID;
			_tPrio = tPrio;
			_tState = VM_THREAD_STATE_DEAD;
			_tEntry = tEntry;
			_param = param;
			_tMemsize = tMemsize;
			_stackaddr = malloc(tMemsize);
			_fileOPerationResult = -1;

		}

		void setStatus(TVMThreadState state){
			_tState = state;
		}

		~TCB(){
			if(_param != NULL){
				delete _param;
			}

			delete _stackaddr;
		}
	};

	class TCBcompare{
	public:
		bool operator() (TCB* left, TCB* right){
			if(left->_tPrio == right->_tPrio){
				if(left->_tid == 0){
					return true;
				}else if(right->_tid == 0){
					return false;
				}
			}
			return left->_tPrio < right->_tPrio;
		}
	};

	class Mutex{
	public:
		TVMThreadID _ownerID;
		TVMMutexID _mutexID;
		std::priority_queue<TCB*, std::vector<TCB*>, TCBcompare> _waitQueue;
		Mutex(TVMMutexID mutexID){
			_ownerID = VM_THREAD_ID_INVALID;
			_mutexID = mutexID;
		};
	};
	static std::vector<TCB*> threadVector;
	static std::vector<Mutex*> mutexVector;
	static std::priority_queue<TCB*, std::vector<TCB*>, TCBcompare> threadQueue;
	static TVMThreadID idleTid;
	static TVMThreadID mainTid;
	static std::unordered_map<TVMTick, std::vector<TVMThreadID>> sleepThreads;
	TVMMainEntry VMLoadModule(const char *module);
	void VMUnloadModule(void);
	TVMStatus VMFilePrint(int filedescriptor, const char *format, ...);

	void scheduler(){
		TCB* running = threadVector[currentThread];
		TCB *nextThread;
		if(threadQueue.size() != 0){
			nextThread = threadQueue.top();
			threadQueue.pop();
		}else{
			nextThread = threadVector[idleTid];
		}

		if(nextThread->_tState == VM_THREAD_STATE_DEAD){
				//MachineResumeSignals(&currentSignalState);
				VMThreadActivate(idleTid);
		}else{
				currentThread = nextThread->_tid;
				nextThread->setStatus(VM_THREAD_STATE_RUNNING);
				//MachineResumeSignals(&currentSignalState);
				std::cout << "SWitching to " << nextThread->_tid << "\n";
				MachineContextSwitch(&(running->_machineContext), &(nextThread->_machineContext));
		}
	}


	TVMStatus VMMutexCreate(TVMMutexIDRef mutexref){
		TMachineSignalState currentSignalState;
		MachineSuspendSignals(&currentSignalState);
		if(mutexref == NULL){
			MachineResumeSignals(&currentSignalState);
			return VM_STATUS_ERROR_INVALID_PARAMETER;
		}
		*mutexref = mutexVector.size();
		Mutex *newMutex = new Mutex(*mutexref);
		mutexVector.push_back(newMutex);
		MachineResumeSignals(&currentSignalState);
		return VM_STATUS_SUCCESS;
	}
	TVMStatus VMMutexDelete(TVMMutexID mutex){
		TMachineSignalState currentSignalState;
		MachineSuspendSignals(&currentSignalState);
		if(mutex >= mutexVector.size() || mutexVector[mutex] == NULL){
			MachineResumeSignals(&currentSignalState);
			return VM_STATUS_ERROR_INVALID_ID;
		}
		if(mutexVector[mutex] != NULL && mutexVector[mutex]->_ownerID == VM_THREAD_ID_INVALID){
			delete mutexVector[mutex];
			mutexVector[mutex] = NULL;
			MachineResumeSignals(&currentSignalState);
			return VM_STATUS_SUCCESS;
		}else{
			MachineResumeSignals(&currentSignalState);
			return VM_STATUS_ERROR_INVALID_STATE;
		}
	}

	TVMStatus VMMutexQuery(TVMMutexID mutex, TVMThreadIDRef ownerref){
		TMachineSignalState currentSignalState;
		MachineSuspendSignals(&currentSignalState);
		if(ownerref == NULL){
			MachineResumeSignals(&currentSignalState);
			return VM_STATUS_ERROR_INVALID_PARAMETER;
		}
		std::cout << "Mutex: " << mutex;
		std::cout << "Size: " << mutexVector.size();
		if(mutex >= mutexVector.size() || mutexVector[mutex] == NULL){
			MachineResumeSignals(&currentSignalState);
			return VM_STATUS_ERROR_INVALID_ID;
		}

		*ownerref = mutexVector[mutex]->_ownerID;
		MachineResumeSignals(&currentSignalState);
		return VM_STATUS_SUCCESS;
	}

	TVMStatus VMMutexAcquire(TVMMutexID mutex, TVMTick timeout){
		TMachineSignalState currentSignalState;
		MachineSuspendSignals(&currentSignalState);
		if(mutex >= mutexVector.size() || mutexVector[mutex] == NULL){
			MachineResumeSignals(&currentSignalState);
			return VM_STATUS_ERROR_INVALID_ID;
		}
		Mutex* curMutex = mutexVector[mutex];
		TCB* running = threadVector[currentThread];
		
		if(curMutex->_ownerID != VM_THREAD_ID_INVALID){
			if(timeout != VM_TIMEOUT_IMMEDIATE){
				curMutex->_waitQueue.push(running);
				running->setStatus(VM_THREAD_STATE_WAITING);
				if(timeout != VM_TIMEOUT_INFINITE){
					VMThreadSleep(timeout);
				}else{
					scheduler();
				}
			}

			if(curMutex->_ownerID != VM_THREAD_ID_INVALID){
				MachineResumeSignals(&currentSignalState);
				return VM_STATUS_FAILURE;
			}
		}

		curMutex->_ownerID = currentThread;
		running->_mid = mutex;
		MachineResumeSignals(&currentSignalState);
		return VM_STATUS_SUCCESS;
	} 
	TVMStatus VMMutexRelease(TVMMutexID mutex){
		TMachineSignalState currentSignalState;
		MachineSuspendSignals(&currentSignalState);
		
		if(mutex >= mutexVector.size() || mutexVector[mutex] == NULL){
			MachineResumeSignals(&currentSignalState);
			return VM_STATUS_ERROR_INVALID_ID;
		}

		if(mutexVector[mutex]->_ownerID != currentThread){
			std::cout << "Current Thread: " << currentThread << "\n";
			std::cout << "Owner: " << mutexVector[mutex]->_ownerID << "\n";
			MachineResumeSignals(&currentSignalState);
			return VM_STATUS_ERROR_INVALID_STATE;
		}

		Mutex* curMutex = mutexVector[mutex];
		TCB* running = threadVector[curMutex->_ownerID];
		std::cout << "Owner: " << curMutex->_ownerID << " Releasing Mutex: " << mutex << "\n";
		curMutex->_ownerID = VM_THREAD_ID_INVALID;
		if(curMutex->_waitQueue.size() != 0){
			TCB* newThread = curMutex->_waitQueue.top();
			std::cout << "Next Thread Is: " << newThread->_tid;
			curMutex->_waitQueue.pop();
			threadQueue.push(newThread);
			if(newThread->_tPrio > running->_tPrio){
				std::cout << "Scheduling Thread: " << newThread->_tid << "\n";
				threadQueue.push(running);
				scheduler();
			}
		}
		MachineResumeSignals(&currentSignalState);
		return VM_STATUS_SUCCESS;
	}














	void alarmCallBack(void* calldata){
		TMachineSignalState currentSignalState;
		MachineSuspendSignals(&currentSignalState);
		//std::cout << "QueueSize: " << threadQueue.size();
		currentTick++;
		auto got = sleepThreads.find((TVMTick)currentTick);
		if(got != sleepThreads.end()){
			std::vector<TVMThreadID> idList = got->second;
			for(auto itr = idList.begin(); itr != idList.end(); itr++){
					TCB *awakenedThread = threadVector[*itr];
					awakenedThread->setStatus(VM_THREAD_STATE_READY);
				 	threadQueue.push(awakenedThread);
			}
			sleepThreads.erase(got);
		}
		// std::vector<TCB*> tempqueue;
		// while(threadQueue.size() != 0){
		// 	TCB* temp = threadQueue.top();
		// 	threadQueue.pop();
		// 	std::cout << temp->_tid;
		// 	tempqueue.push_back(temp);
		// }
		// for(int i = 0; i < tempqueue.size();i++){
		// 	threadQueue.push(tempqueue[i]);
		// }
		// std::cout << "\n";
		//std::cout << "Queue size: " << threadQueue.size() << "\n";
		TCB *running = threadVector[currentThread];
		if(threadQueue.size() != 0 && threadQueue.top()->_tPrio < running->_tPrio){
			MachineResumeSignals(&currentSignalState);
			return;
		}
		running->setStatus(VM_THREAD_STATE_READY);
		threadQueue.push(running);
		TCB *nextThread = threadQueue.top();
		threadQueue.pop();
		nextThread->setStatus(VM_THREAD_STATE_RUNNING);
		currentThread = nextThread->_tid;
		//MachineResumeSignals(&currentSignalState);
		//std::cout << "Alarm SWitching to " << nextThread->_tid << "\n";
		MachineContextSwitch(&(running->_machineContext), &(nextThread->_machineContext));
		MachineResumeSignals(&currentSignalState);
	}

	void idleEntry(void* nothing){
		std::cout << "In Idle\n";
		MachineEnableSignals();
		while(1){
			
		}
	}
	void emptyEntry(void* nothing){}

	TVMStatus VMStart(int tickms, TVMMemorySize memsize, int argc, char *argv[]) {
		msPerTick = tickms;
		TVMMainEntry mainEntry = VMLoadModule(argv[0]);
		if(NULL == mainEntry){
			return VM_STATUS_FAILURE;
		}
		sharedmemory = (char*)MachineInitialize(memsize);
		MachineEnableSignals();
		VMThreadCreate(idleEntry, NULL, 0x100000, VM_THREAD_PRIORITY_LOW, &idleTid);
		VMThreadCreate(emptyEntry, NULL, 0x01, VM_THREAD_PRIORITY_NORMAL,&mainTid);
		MachineRequestAlarm(tickms * 1000,alarmCallBack,NULL);
		mainEntry(argc, argv);
		MachineTerminate();
		VMUnloadModule();
		return VM_STATUS_SUCCESS;
	}

	void fileCallBack(void *calldata, int result){
		TMachineSignalState currentSignalState;
		MachineSuspendSignals(&currentSignalState);
		TCB* blocked = (TCB*) calldata;
		//std::cout << "In "<< blocked->_tid << " File Callback\n";
		if(result < 0){
			blocked->_fileOPerationResult = -1;
			VMPrintError("Failed to operate\n");
		}else{
			blocked->_fileOPerationResult = result;
 		}
		blocked->setStatus(VM_THREAD_STATE_READY);

		TCB *running = threadVector[currentThread];
		if(blocked->_tPrio > running->_tPrio){
			running->setStatus(VM_THREAD_STATE_READY);
			threadQueue.push(running);
			currentThread = blocked->_tid;
			blocked->setStatus(VM_THREAD_STATE_RUNNING);
			//std::cout << "PRint SWitching to " << blocked->_tid << "\n";
			MachineContextSwitch(&(running->_machineContext), &(blocked->_machineContext));
		}else{
			threadQueue.push(blocked);
		}
		// threadQueue.push(blocked);
		// std::vector<TCB*> tempqueue;
		// while(threadQueue.size() != 0){
		// 	TCB* temp = threadQueue.top();
		// 	threadQueue.pop();
		// 	std::cout << temp->_tid;
		// 	tempqueue.push_back(temp);
		// }
		// for(int i = 0; i < tempqueue.size();i++){
		// 	threadQueue.push(tempqueue[i]);
		// }
		// std::cout << "\n";

		// if(threadQueue.size() != 0 && threadQueue.top()->_tPrio > running->_tPrio){
		// 	TCB * nextThread = threadQueue.top();
		// 	threadQueue.pop();
		// 	running->setStatus(VM_THREAD_STATE_READY);
		// 	threadQueue.push(running);
		// 	currentThread = nextThread->_tid;
		// 	nextThread->setStatus(VM_THREAD_STATE_RUNNING);		
		// 	MachineResumeSignals(&currentSignalState);
		// 	std::cout << "PRint Callback SWitching to " << blocked->_tid << "\n";
		// 	MachineContextSwitch(&(running->_machineContext), &(nextThread->_machineContext));
		// 	return;
		// }
		MachineResumeSignals(&currentSignalState);
	}


	TVMStatus VMFileWrite(int filedescriptor, void *data, int *length){
		TMachineSignalState currentSignalState;
		MachineSuspendSignals(&currentSignalState);
		int checkLength = *length;
		int dataIndex = 0;
		TCB *running = threadVector[currentThread];

		while(checkLength > 0){
			int dataLength = checkLength > 512 ? 512 : checkLength;
			memcpy((char*)sharedmemory + running->_pid, (char*)data + dataIndex, dataLength);
			MachineFileWrite(filedescriptor, (char*)sharedmemory + running->_pid, dataLength, fileCallBack, running);
			checkLength -= dataLength;
			dataIndex += dataLength;
			running->setStatus(VM_THREAD_STATE_WAITING);
			scheduler();
		}
		MachineResumeSignals(&currentSignalState);
		return VM_STATUS_SUCCESS;
	}

	TVMStatus VMFileOpen(const char *filename, int flags, int mode, int *filedescriptor){
		TMachineSignalState currentSignalState;
		MachineSuspendSignals(&currentSignalState);
		TCB *running = threadVector[currentThread];
		MachineFileOpen(filename, flags, mode, fileCallBack, running);
		running->setStatus(VM_THREAD_STATE_WAITING);
		scheduler();
		if(running->_fileOPerationResult != -1){
			*filedescriptor = running->_fileOPerationResult;
			running->_fileOPerationResult = -1;
			MachineResumeSignals(&currentSignalState);
			return VM_STATUS_SUCCESS;
		}else{
			MachineResumeSignals(&currentSignalState);
			return VM_STATUS_FAILURE;
		}
	}

	TVMStatus VMFileClose(int filedescriptor){
		TMachineSignalState currentSignalState;
		MachineSuspendSignals(&currentSignalState);
		TCB *running = threadVector[currentThread];
		MachineFileClose(filedescriptor, fileCallBack, running);
		running->setStatus(VM_THREAD_STATE_WAITING);
		scheduler();
		MachineResumeSignals(&currentSignalState);
		return VM_STATUS_FAILURE;
	}

	TVMStatus VMFileRead(int filedescriptor, void *data, int *length){
		TMachineSignalState currentSignalState;
		MachineSuspendSignals(&currentSignalState);
		TCB *running = threadVector[currentThread];
		int checkLength = *length;
		int dataIndex = 0;

		while(checkLength > 0){
			int dataLength = checkLength > 512 ? 512 : checkLength;		
			MachineFileRead(filedescriptor, (char*)sharedmemory + running->_pid, dataLength, fileCallBack, running);
			checkLength -= dataLength;
			dataIndex += dataLength;
			running->setStatus(VM_THREAD_STATE_WAITING);
			scheduler();
			memcpy(data, ((char*)sharedmemory + running->_pid), dataLength);
		}
		if(running->_fileOPerationResult != -1){
			*length = running->_fileOPerationResult;
			running->_fileOPerationResult = -1;
			MachineResumeSignals(&currentSignalState);
			return VM_STATUS_SUCCESS;
		}else{
			MachineResumeSignals(&currentSignalState);
			return VM_STATUS_FAILURE;
		}

	}
	TVMStatus VMFileSeek(int filedescriptor, int offset, int whence, int *newoffset){
		TMachineSignalState currentSignalState;
		MachineSuspendSignals(&currentSignalState);
		TCB *running = threadVector[currentThread];
		MachineFileSeek(filedescriptor, offset, whence, fileCallBack, running);
		running->setStatus(VM_THREAD_STATE_WAITING);
		scheduler();
		if(running->_fileOPerationResult != -1){
			*newoffset = running->_fileOPerationResult;
			running->_fileOPerationResult = -1;
			MachineResumeSignals(&currentSignalState);
			return VM_STATUS_SUCCESS;
		}else{
			MachineResumeSignals(&currentSignalState);
			return VM_STATUS_FAILURE;
		}	
	}

	void runThreadEntry(void *param){
		MachineEnableSignals();
		TCB *thread = threadVector[currentThread];
		thread->_tEntry(thread->_param);
		VMThreadTerminate(currentThread);
	}

	TVMStatus VMThreadCreate(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio, TVMThreadIDRef tid){
		TMachineSignalState currentSignalState;
		MachineSuspendSignals(&currentSignalState);
		if(NULL == entry || NULL == tid){
			MachineResumeSignals(&currentSignalState);
			return VM_STATUS_ERROR_INVALID_PARAMETER;
		}
		*tid = threadVector.size();
		TCB *newThread = new TCB(*tid, prio, entry, param, memsize);
		threadVector.push_back(newThread);
		MachineResumeSignals(&currentSignalState);
		return VM_STATUS_SUCCESS;
	}

	TVMStatus VMThreadDelete(TVMThreadID thread){
		TMachineSignalState currentSignalState;
		MachineSuspendSignals(&currentSignalState);
		if(thread >= threadVector.size() || threadVector[thread] == NULL){
			MachineResumeSignals(&currentSignalState);
			return VM_STATUS_ERROR_INVALID_ID;
		}

		if(threadVector[thread]->_tState != VM_THREAD_STATE_DEAD){
			MachineResumeSignals(&currentSignalState);
			return VM_STATUS_ERROR_INVALID_STATE;
		}

		delete threadVector[thread];
		threadVector[thread] = NULL;
		MachineResumeSignals(&currentSignalState);
		return VM_STATUS_SUCCESS;
	}

	TVMStatus VMThreadActivate(TVMThreadID thread){
		TMachineSignalState currentSignalState;
		MachineSuspendSignals(&currentSignalState);

		std::cout << "In Activate\n";
		if(thread >= threadVector.size() || threadVector[thread] == NULL){
			MachineResumeSignals(&currentSignalState);
			return VM_STATUS_ERROR_INVALID_ID;
		}

		if(threadVector[thread]->_tState != VM_THREAD_STATE_DEAD){
			MachineResumeSignals(&currentSignalState);
			return VM_STATUS_ERROR_INVALID_STATE;
		}
		TCB *target = threadVector[thread];
		TCB *running = threadVector[currentThread];
		MachineContextCreate(&(target->_machineContext), runThreadEntry, NULL, (void*)target->_stackaddr, target->_tMemsize);
		
		if(target->_tPrio > running->_tPrio){
			target->setStatus(VM_THREAD_STATE_RUNNING);
			running->setStatus(VM_THREAD_STATE_READY);
			threadQueue.push(running);		
			currentThread = thread;
			MachineContextSwitch(&(running->_machineContext), &(target->_machineContext));
		}else if(thread == idleTid){
			target->setStatus(VM_THREAD_STATE_RUNNING);
			currentThread = thread;
			MachineContextSwitch(&(running->_machineContext), &(target->_machineContext));
		}else{
			target->setStatus(VM_THREAD_STATE_READY);
			threadQueue.push(target);		
		}
		MachineResumeSignals(&currentSignalState);
		return VM_STATUS_SUCCESS;
	}

	TVMStatus VMThreadTerminate(TVMThreadID thread){
		TMachineSignalState currentSignalState;
		MachineSuspendSignals(&currentSignalState);
		if(thread >= threadVector.size() || threadVector[thread] == NULL){
			MachineResumeSignals(&currentSignalState);
			return VM_STATUS_ERROR_INVALID_ID;
		}

		if(threadVector[thread]->_tState == VM_THREAD_STATE_DEAD){
			MachineResumeSignals(&currentSignalState);
			return VM_STATUS_ERROR_INVALID_STATE;
		}
		std::cout << "Terminating Thread: " << thread << "\n";
		TCB* running = threadVector[thread];
		running->setStatus(VM_THREAD_STATE_DEAD);
		mutexVector[running->_mid]->_ownerID = currentThread;
		VMMutexRelease(running->_mid);
		running->_mid = VM_MUTEX_ID_INVALID;
		
		scheduler();
		MachineResumeSignals(&currentSignalState);
		return VM_STATUS_SUCCESS;
	}

	TVMStatus VMThreadSleep(TVMTick tick){
		TMachineSignalState currentSignalState;
		MachineSuspendSignals(&currentSignalState);
		std::cout << "IN Sleep\n";

		if(tick == VM_TIMEOUT_INFINITE){
			std::cout << "Infinite\n";
			MachineResumeSignals(&currentSignalState);
			return VM_STATUS_ERROR_INVALID_PARAMETER;
		}

		TVMTick targetTick = currentTick + tick;
		TCB *running = threadVector[currentThread];
		TCB* nextThread = threadQueue.top();
		std::cout << "Current Thread: " << running->_tid << "\n";
		if(tick != VM_TIMEOUT_IMMEDIATE){
			std::cout << "Put in map\n";
			std::vector<TVMThreadID> idList;
			auto got = sleepThreads.find(targetTick);
			if(got != sleepThreads.end()){
				idList = got->second;
			}
			idList.push_back((TVMThreadID)currentThread);
			sleepThreads.emplace(targetTick, idList);
		}else if(nextThread->_tPrio < running->_tPrio){
			std::cout << "Returning\n";
			MachineResumeSignals(&currentSignalState);
			return VM_STATUS_SUCCESS;
		}
		
		running->setStatus(VM_THREAD_STATE_WAITING);
		scheduler();
		MachineResumeSignals(&currentSignalState);
		return VM_STATUS_SUCCESS;
	}

	TVMStatus VMThreadID(TVMThreadIDRef threadref){
		TMachineSignalState currentSignalState;
		MachineSuspendSignals(&currentSignalState);
		if(threadref == NULL){
			MachineResumeSignals(&currentSignalState);
			return VM_STATUS_ERROR_INVALID_PARAMETER;
		}
		*threadref = currentThread;
		MachineResumeSignals(&currentSignalState);
		return VM_STATUS_SUCCESS;
		
	}
	TVMStatus VMThreadState(TVMThreadID thread, TVMThreadStateRef stateref){
		TMachineSignalState currentSignalState;
		MachineSuspendSignals(&currentSignalState);
		if(thread >= threadVector.size() || threadVector[thread] == NULL){
			MachineResumeSignals(&currentSignalState);
			return VM_STATUS_ERROR_INVALID_ID;
		}

		if(stateref == NULL){
			MachineResumeSignals(&currentSignalState);
			return VM_STATUS_ERROR_INVALID_PARAMETER;
		}
		TCB *target = threadVector[thread];
		*stateref = target->_tState;
		MachineResumeSignals(&currentSignalState);
		return VM_STATUS_SUCCESS;

	}

	TVMStatus VMTickMS(int *tickmsref){
		TMachineSignalState currentSignalState;
		MachineSuspendSignals(&currentSignalState);
		if(tickmsref == NULL){
			MachineResumeSignals(&currentSignalState);
			return VM_STATUS_ERROR_INVALID_PARAMETER;
		}
		*tickmsref = msPerTick;
		MachineResumeSignals(&currentSignalState);
		return VM_STATUS_SUCCESS;

	}

	TVMStatus VMTickCount(TVMTickRef tickref){
		TMachineSignalState currentSignalState;
		MachineSuspendSignals(&currentSignalState);
		if(tickref == NULL){
			MachineResumeSignals(&currentSignalState);
			return VM_STATUS_ERROR_INVALID_PARAMETER;
		}
		*tickref = currentTick;
		MachineResumeSignals(&currentSignalState);
		return VM_STATUS_SUCCESS;
	}
}