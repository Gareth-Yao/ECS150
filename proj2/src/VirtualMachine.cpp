// #include "VirtualMachineUtils.c"
#include "VirtualMachine.h"
#include "Machine.h"
#include <iostream>
extern "C"{
	volatile TVMTick currentTick = 0;
	static int msPerTick = 100;
	TVMMainEntry VMLoadModule(const char *module);
	void VMUnloadModule(void);
	TVMStatus VMFilePrint(int filedescriptor, const char *format, ...);
		void machineCallBack(void *calldata, int result){

		if(result < 0){
			VMFilePrint(2, (const char *)calldata);
		}
		return;
	}

	void alarmCallBack(void* calldata){
		currentTick++;
	}

	void callCreateThread(){

	}
	TVMStatus VMStart(int tickms, int argc, char *argv[]) {
		msPerTick = tickms;
		TVMMainEntry mainEntry = VMLoadModule(argv[0]);
		if(NULL == mainEntry){
			return VM_STATUS_FAILURE;
		}
		MachineInitialize();
		MachineEnableSignals();
		MachineRequestAlarm(tickms * 1000,alarmCallBack,NULL);

		mainEntry(argc, argv);
		MachineTerminate();
		VMUnloadModule();
		return VM_STATUS_SUCCESS;
	}

	TVMStatus VMFileWrite(int filedescriptor, void *data, int *length){
		char callback[] = "Machine File Write";
		MachineFileWrite(filedescriptor, data, *length, machineCallBack, callback);
		return VM_STATUS_SUCCESS;
	}

	void runThreadEntry(TVMThreadEntry entry, void *param){
		entry(param);
	}
	TVMStatus VMThreadCreate(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio, TVMThreadIDRef tid){
		
		runThreadEntry(entry, param);
	}

	TVMStatus VMThreadActivate(TVMThreadID thread){

	}

	TVMStatus VMThreadSleep(TVMTick tick){
		while(currentTick < tick){
			// cout << currentTick << "\n";
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