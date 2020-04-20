// #include "VirtualMachineUtils.c"
#include "VirtualMachine.h"
#include "Machine.h"
#include <iostream>
extern "C"{
	TVMMainEntry VMLoadModule(const char *module);
	void VMUnloadModule(void);
	TVMStatus VMFilePrint(int filedescriptor, const char *format, ...);
	TVMStatus VMStart(int tickms, int argc, char *argv[]) {
		TVMMainEntry mainEntry = VMLoadModule(argv[0]);
		if(NULL == mainEntry){
			return VM_STATUS_FAILURE;
		}
		MachineInitialize();
		MachineEnableSignals();
		mainEntry(argc, argv);
		MachineTerminate();
		VMUnloadModule();
		return VM_STATUS_SUCCESS;
	}
	void machineCallBack(void *calldata, int result){

		if(result < 0){
			VMFilePrint(2, (const char *)calldata);
		}
		return;
	}
	TVMStatus VMFileWrite(int filedescriptor, void *data, int *length){
		char callback[] = "Machine File Write";
		MachineFileWrite(filedescriptor, data, *length, machineCallBack, callback);
		return VM_STATUS_SUCCESS;
	}

	TVMStatus VMThreadCreate(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio, TVMThreadIDRef tid){

	}

	TVMStatus VMThreadActivate(TVMThreadID thread){

	}

	TVMStatus VMThreadSleep(TVMTick tick){
		
	}
}