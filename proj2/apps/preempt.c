#include "VirtualMachine.h" 	 	    		
#include <stdio.h>
#ifndef NULL
#define NULL    ((void *)0)
#endif

void VMThread(void *param){
    volatile int *Val = (int *)param;
    int RunTimeTicks;
    TVMTick CurrentTick, EndTick;
    TVMThreadID curThread;
    VMTickMS(&RunTimeTicks);
    RunTimeTicks = (5000 + RunTimeTicks - 1)/RunTimeTicks;
    VMTickCount(&CurrentTick);
    EndTick = CurrentTick + RunTimeTicks;
    VMThreadID(&curThread);
    VMPrint("VMThread %d Alive\nVMThread Starting\n", curThread);
    TVMTick lastTick = CurrentTick;
    while(EndTick > CurrentTick){
        
        (*Val)++;
        VMTickCount(&CurrentTick);
        if(CurrentTick - lastTick >= 1){
            VMThreadID(&curThread);
            printf("Thread %d, Val: %d, CurrentTick: %d \n", curThread, *Val, CurrentTick);
            lastTick = CurrentTick;
        }
    }
    VMPrint("VMThread Done\n");
}

void VMMain(int argc, char *argv[]){
    TVMThreadID VMThreadID1, VMThreadID2;
    TVMThreadState VMState1, VMState2;
    volatile int Val1 = 0, Val2 = 0;
    volatile int LocalVal1, LocalVal2;
    VMPrint("VMMain creating threads.\n");
    VMThreadCreate(VMThread, (void *)&Val1, 0x100000, VM_THREAD_PRIORITY_LOW, &VMThreadID1);
    VMThreadCreate(VMThread, (void *)&Val2, 0x100000, VM_THREAD_PRIORITY_LOW, &VMThreadID2);
    VMPrint("VMMain activating threads.\n");
    VMThreadActivate(VMThreadID1);
    VMThreadActivate(VMThreadID2);
    VMPrint("VMMain Waiting\n");
    do{
        TVMTick CurrentTick;
        VMTickCount(&CurrentTick);
        TVMThreadID curThread;
        VMThreadID(&curThread);
        printf("Thread %d, CurrentTick %d \n", curThread, CurrentTick);
        LocalVal1 = Val1; 
        LocalVal2 = Val2;
        VMThreadState(VMThreadID1, &VMState1);
        VMThreadState(VMThreadID2, &VMState2);
        VMTickCount(&CurrentTick);
        VMPrint("CurrentTick: %d, %d %d\n", CurrentTick, LocalVal1, LocalVal2);
        VMThreadSleep(2);
    }while((VM_THREAD_STATE_DEAD != VMState1)||(VM_THREAD_STATE_DEAD != VMState2));
    VMPrint("VMMain Done\n");
    VMPrint("Goodbye\n");
}

