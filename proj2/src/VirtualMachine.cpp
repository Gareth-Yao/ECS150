// #include "VirtualMachineUtils.c"
#include "VirtualMachine.h"
#include "Machine.h"
#include <iostream>
#include <vector>
#include <stack>
#include <queue>
#include <unordered_map>



extern "C"{

	volatile TVMTick currentTick = 0;
	static int msPerTick = 100;
	volatile static TVMThreadID currentThread = 1;
	class TCB{
		public:
		TVMThreadID _tid;
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

	static std::vector<TCB*> threadVector;
	static std::priority_queue<TCB*, std::vector<TCB*>, TCBcompare> threadQueue;
	static TVMThreadID idleTid;
	static TVMThreadID mainTid;
	static std::unordered_map<TVMTick, std::vector<TVMThreadID>> sleepThreads;
	TVMMainEntry VMLoadModule(const char *module);
	void VMUnloadModule(void);
	TVMStatus VMFilePrint(int filedescriptor, const char *format, ...);

	void scheduler(){

	}

	void alarmCallBack(void* calldata){
		// TMachineSignalState currentSignalState;
		// MachineSuspendSignals(&currentSignalState);
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
		TCB *running = threadVector[currentThread];
		if(threadQueue.size() != 0 && threadQueue.top()->_tPrio < running->_tPrio){
			return;
		}
		running->setStatus(VM_THREAD_STATE_READY);
		threadQueue.push(running);
		TCB *nextThread = threadQueue.top();
		threadQueue.pop();
		nextThread->setStatus(VM_THREAD_STATE_RUNNING);
		currentThread = nextThread->_tid;
		// MachineResumeSignals(&currentSignalState);
		MachineContextSwitch(&(running->_machineContext), &(nextThread->_machineContext));
	}

	void idleEntry(void* nothing){
		MachineEnableSignals();
		while(1){
			
		}
	}
	void emptyEntry(void* nothing){}

	TVMStatus VMStart(int tickms, int argc, char *argv[]) {
		msPerTick = tickms;
		TVMMainEntry mainEntry = VMLoadModule(argv[0]);
		if(NULL == mainEntry){
			return VM_STATUS_FAILURE;
		}
		MachineInitialize();
		MachineEnableSignals();
		VMThreadCreate(idleEntry, NULL, 0x100000, VM_THREAD_PRIORITY_LOW, &idleTid);
		VMThreadCreate(emptyEntry, NULL, 0x01, VM_THREAD_PRIORITY_NORMAL,&mainTid);
		// VMThreadActivate(idleTid);
		MachineRequestAlarm(tickms * 1000,alarmCallBack,NULL);
		mainEntry(argc, argv);
		MachineTerminate();
		VMUnloadModule();
		return VM_STATUS_SUCCESS;
	}

	void fileCallBack(void *calldata, int result){
		TCB* blocked = (TCB*) calldata;
		if(result < 0){
			blocked->_fileOPerationResult = -1;
			VMPrintError("Failed to operate\n");
		}else{
			blocked->_fileOPerationResult = result;

		}
		blocked->setStatus(VM_THREAD_STATE_READY);
		threadQueue.push(blocked);
	}


	TVMStatus VMFileWrite(int filedescriptor, void *data, int *length){
		TMachineSignalState currentSignalState;
		MachineSuspendSignals(&currentSignalState);
		TCB *running = threadVector[currentThread];
		running->setStatus(VM_THREAD_STATE_WAITING);
		TCB *nextThread;
		if(threadQueue.size() != 0){
			nextThread = threadQueue.top();
			threadQueue.pop();
		}else{
			nextThread = threadVector[idleTid];
		}
		MachineFileWrite(filedescriptor, data, *length, fileCallBack, running);

		if(nextThread->_tid == idleTid){
			MachineResumeSignals(&currentSignalState);
			VMThreadActivate(idleTid);
		}else{
			currentThread = nextThread->_tid;
			nextThread->setStatus(VM_THREAD_STATE_RUNNING);
			MachineResumeSignals(&currentSignalState);
			MachineContextSwitch(&(running->_machineContext), &(nextThread->_machineContext));
		}	
		return VM_STATUS_SUCCESS;
	}

	TVMStatus VMFileOpen(const char *filename, int flags, int mode, int *filedescriptor){
		TMachineSignalState currentSignalState;
		MachineSuspendSignals(&currentSignalState);
		TCB *running = threadVector[currentThread];
		running->setStatus(VM_THREAD_STATE_WAITING);
		TCB *nextThread;
		if(threadQueue.size() != 0){
			nextThread = threadQueue.top();
			threadQueue.pop();
		}else{
			nextThread = threadVector[idleTid];
		}
		MachineFileOpen(filename, flags, mode, fileCallBack, running);
		if(nextThread->_tid == idleTid){
			MachineResumeSignals(&currentSignalState);
			VMThreadActivate(idleTid);
		}else{
			currentThread = nextThread->_tid;
			nextThread->setStatus(VM_THREAD_STATE_RUNNING);
			MachineResumeSignals(&currentSignalState);
			MachineContextSwitch(&(running->_machineContext), &(nextThread->_machineContext));
		}
		if(running->_fileOPerationResult != -1){
			*filedescriptor = running->_fileOPerationResult;
			running->_fileOPerationResult = -1;
			return VM_STATUS_SUCCESS;
		}else{
			return VM_STATUS_FAILURE;
		}	
	}

	TVMStatus VMFileClose(int filedescriptor){
		TMachineSignalState currentSignalState;
		MachineSuspendSignals(&currentSignalState);
		TCB *running = threadVector[currentThread];
		running->setStatus(VM_THREAD_STATE_WAITING);
		TCB *nextThread;
		if(threadQueue.size() != 0){
			nextThread = threadQueue.top();
			threadQueue.pop();
		}else{
			nextThread = threadVector[idleTid];
		}
		MachineFileClose(filedescriptor, fileCallBack, running);
		
		if(nextThread->_tid == idleTid){
			MachineResumeSignals(&currentSignalState);
			VMThreadActivate(idleTid);
		}else{
			currentThread = nextThread->_tid;
			nextThread->setStatus(VM_THREAD_STATE_RUNNING);
			MachineResumeSignals(&currentSignalState);
			MachineContextSwitch(&(running->_machineContext), &(nextThread->_machineContext));
		}
		return VM_STATUS_FAILURE;
	}
	TVMStatus VMFileRead(int filedescriptor, void *data, int *length){
		TMachineSignalState currentSignalState;
		MachineSuspendSignals(&currentSignalState);
		TCB *running = threadVector[currentThread];
		running->setStatus(VM_THREAD_STATE_WAITING);
		TCB *nextThread;
		if(threadQueue.size() != 0){
			nextThread = threadQueue.top();
			threadQueue.pop();
		}else{
			nextThread = threadVector[idleTid];
		}
		MachineFileRead(filedescriptor, data, *length, fileCallBack, running);
		if(nextThread->_tid == idleTid){
			MachineResumeSignals(&currentSignalState);
			VMThreadActivate(idleTid);
		}else{
			currentThread = nextThread->_tid;
			nextThread->setStatus(VM_THREAD_STATE_RUNNING);
			MachineResumeSignals(&currentSignalState);
			MachineContextSwitch(&(running->_machineContext), &(nextThread->_machineContext));
		}
		if(running->_fileOPerationResult != -1){
			*length = running->_fileOPerationResult;
			running->_fileOPerationResult = -1;
			return VM_STATUS_SUCCESS;
		}else{
			return VM_STATUS_FAILURE;
		}

	}
	TVMStatus VMFileSeek(int filedescriptor, int offset, int whence, int *newoffset){
		TMachineSignalState currentSignalState;
		MachineSuspendSignals(&currentSignalState);
		TCB *running = threadVector[currentThread];
		running->setStatus(VM_THREAD_STATE_WAITING);
		TCB *nextThread;
		if(threadQueue.size() != 0){
			nextThread = threadQueue.top();
			threadQueue.pop();
		}else{
			nextThread = threadVector[idleTid];
		}
		MachineFileSeek(filedescriptor, offset, whence, fileCallBack, running);
		
		if(nextThread->_tid == idleTid){
			MachineResumeSignals(&currentSignalState);
			VMThreadActivate(idleTid);
		}else{
			currentThread = nextThread->_tid;
			nextThread->setStatus(VM_THREAD_STATE_RUNNING);
			MachineResumeSignals(&currentSignalState);
			MachineContextSwitch(&(running->_machineContext), &(nextThread->_machineContext));
		}
		if(running->_fileOPerationResult != -1){
			*newoffset = running->_fileOPerationResult;
			running->_fileOPerationResult = -1;
			return VM_STATUS_SUCCESS;
		}else{
			return VM_STATUS_FAILURE;
		}	
	}

	void runThreadEntry(void *param){
		TCB *thread = threadVector[currentThread];
		thread->_tEntry(thread->_param);
		VMThreadTerminate(currentThread);
		TCB *nextThread;
		if(threadQueue.size() != 0){
			nextThread = threadQueue.top();
			threadQueue.pop();
		}else{
			nextThread = threadVector[idleTid];
		}
		nextThread->setStatus(VM_THREAD_STATE_RUNNING);
		currentThread = nextThread->_tid;
		MachineContextSwitch(&(thread->_machineContext), &(nextThread->_machineContext));
	}

	TVMStatus VMThreadCreate(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio, TVMThreadIDRef tid){
		if(NULL == entry || NULL == tid){
			return VM_STATUS_ERROR_INVALID_PARAMETER;
		}
		*tid = threadVector.size();
		TCB *newThread = new TCB(*tid, prio, entry, param, memsize);
		threadVector.push_back(newThread);
		return VM_STATUS_SUCCESS;
	}

	TVMStatus VMThreadActivate(TVMThreadID thread){
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

		return VM_STATUS_SUCCESS;
	}

	TVMStatus VMThreadTerminate(TVMThreadID thread){
		TCB* running = threadVector[thread];
		running->setStatus(VM_THREAD_STATE_DEAD);

		return VM_STATUS_SUCCESS;
	}

	TVMStatus VMThreadSleep(TVMTick tick){
		TVMTick targetTick = currentTick + tick;
		TCB *nextThread;
		if(threadQueue.size() != 0){
			nextThread = threadQueue.top();
			threadQueue.pop();
		}else{
			nextThread = threadVector[idleTid];
		}
		std::vector<TVMThreadID> idList;
		auto got = sleepThreads.find(targetTick);
		if(got != sleepThreads.end()){
			idList = got->second;
		}

		idList.push_back((TVMThreadID)currentThread);
		sleepThreads.emplace(targetTick, idList);

		TCB *running = threadVector[currentThread];
		running->setStatus(VM_THREAD_STATE_WAITING);
		
		if(nextThread->_tid == idleTid){
			VMThreadActivate(idleTid);
		}else{
			currentThread = nextThread->_tid;
			nextThread->setStatus(VM_THREAD_STATE_RUNNING);
			MachineContextSwitch(&(running->_machineContext), &(nextThread->_machineContext));
		}	
	
		return VM_STATUS_SUCCESS;
	}

	TVMStatus VMThreadID(TVMThreadIDRef threadref){
		if(threadref == NULL){
			return VM_STATUS_ERROR_INVALID_PARAMETER;
		}
		if(*threadref = currentThread){
			return VM_STATUS_SUCCESS;
		}else{
			return VM_STATUS_FAILURE;
		}
		
	}
	TVMStatus VMThreadState(TVMThreadID thread, TVMThreadStateRef stateref){
		if(stateref == NULL){
			return VM_STATUS_ERROR_INVALID_PARAMETER;
		}
		TCB *target = threadVector[thread];
		if(*stateref = target->_tState){
			return VM_STATUS_SUCCESS;
		}else{
			return VM_STATUS_FAILURE;
		}

	}

	TVMStatus VMTickMS(int *tickmsref){
		if(tickmsref == NULL){
			return VM_STATUS_ERROR_INVALID_PARAMETER;
		}
		if(*tickmsref = msPerTick){
			return VM_STATUS_SUCCESS;
		}else{
			return VM_STATUS_FAILURE;
		}

	}

	TVMStatus VMTickCount(TVMTickRef tickref){
		if(tickref == NULL){
			return VM_STATUS_ERROR_INVALID_PARAMETER;
		}
		if(*tickref = currentTick){
			return VM_STATUS_SUCCESS;
		}else{
			return VM_STATUS_FAILURE;
		}
	}
}