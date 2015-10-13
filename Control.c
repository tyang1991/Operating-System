#include "global.h"
#include "stdio.h"
#include "syscalls.h"
#include "queue.h"
#include "Control.h"
#include "Utility.h"

/*******************************Current State*********************************/
long CurrentTime(){
	MEMORY_MAPPED_IO mmio;    //for hardware interface

	mmio.Mode = Z502ReturnValue;
	mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
	MEM_READ(Z502Clock, &mmio);
	return mmio.Field1;
}

long CurrentContext() {
	MEMORY_MAPPED_IO mmio;    //for hardware interface

	mmio.Mode = Z502GetCurrentContext;
	mmio.Field1 = mmio.Field2 = mmio.Field3 = mmio.Field4 = 0;
	MEM_READ(Z502Context, &mmio);
	return mmio.Field1;
}

struct Process_Control_Block *CurrentPCB() {
	long currentContext = CurrentContext();
	struct Process_Control_Block *PCB = findPCBbyContextID(currentContext);
	return PCB;
}

int CurrentPID() {
	long currentContext = CurrentContext();
	struct Process_Control_Block *PCB = findPCBbyContextID(currentContext);
	if (PCB != NULL) {
		return PCB->ProcessID;
	}
	else {
		return -1;
	}
}
/*****************************************************************************/


/******************************Process Controle*******************************/
#define Uniprocessor 1L
#define Multiprocessor 2L
#define ProcessorMode Multiprocessor

void Dispatcher(){
	struct Process_Control_Block *PCB;//for temp use
	
	switch (ProcessorMode) {
		case Uniprocessor:
			while (1) {
				while (readyQueue->Element_Number == 0) {
					CALL(1);
				}

				PCB = readyQueue->First_Element->PCB;

				if (PCB->ProcessState == PCB_STATE_LIVE) {
					PCB = deReadyQueue();
					break;
				}
				else if (PCB->ProcessState == PCB_STATE_TERMINATE) {
					deReadyQueue();
				}
				else if (PCB->ProcessState == PCB_STATE_SUSPEND) {
					deReadyQueue();
				}
				else if (PCB->ProcessState == PCB_STATE_MSG_SUSPEND) {
					PCB = deReadyQueue();
					break;
				}
			}
			
			OSStartProcess(PCB);
			break;
		case Multiprocessor:
			//if multiple processes running at the same time, we only start new process
			//when ready queue is not empty
			if (pcbTable->Cur_Running_Number > 1) {
				if (readyQueue->Element_Number >= 1) {
					do {
						PCB = readyQueue->First_Element->PCB;

						if (PCB->ProcessState == PCB_STATE_LIVE) {
							PCB = deReadyQueue();
						}
						else if (PCB->ProcessState == PCB_STATE_TERMINATE) {
							deReadyQueue();
						}
						else if (PCB->ProcessState == PCB_STATE_SUSPEND) {
							deReadyQueue();
						}
						else if (PCB->ProcessState == PCB_STATE_MSG_SUSPEND) {
							PCB = deReadyQueue();
						}

						if (PCB != NULL) {
							OSStartProcess_Only(PCB);
						}

					} while (readyQueue->Element_Number >= 1);
				}

				//suspend itself
				PCB = CurrentPCB();
				OSSuspendCurrentProcess();
			}
			//if only one process running, it's responsible to start another process
			else {
				while (1) {
					while (readyQueue->Element_Number == 0) {
						CALL(1);
					}

					PCB = readyQueue->First_Element->PCB;

					do {
						if (PCB->ProcessState == PCB_STATE_LIVE) {
							PCB = deReadyQueue();
						}
						else if (PCB->ProcessState == PCB_STATE_TERMINATE) {
							deReadyQueue();
						}
						else if (PCB->ProcessState == PCB_STATE_SUSPEND) {
							deReadyQueue();
						}
						else if (PCB->ProcessState == PCB_STATE_MSG_SUSPEND) {
							PCB = deReadyQueue();
						}

						if (PCB != NULL) {
							OSStartProcess_Only(PCB);
						}

					} while (readyQueue->Element_Number >= 1);
					//if other processes are started, break and suspend itself
					if (pcbTable->Cur_Running_Number > 1) {
						break;
					}
				}

				//suspend itself
				PCB = CurrentPCB();
				if (PCB != NULL) {
					OSSuspendCurrentProcess();
				}
			}
			break;
	}
}

void OSStartProcess_Only(struct Process_Control_Block* PCB) {
	if (PCB->ProcessState != PCB_STATE_RUNNING) {
		MEMORY_MAPPED_IO mmio;
		pcbTable->Cur_Running_Number += 1;
		printf("+++++++++++++++++++++++++++++++1, PID: %d\n", PCB->ProcessID);
		PCB->ProcessState = PCB_STATE_RUNNING;

		mmio.Mode = Z502StartContext;
		mmio.Field1 = PCB->ContextID;
		mmio.Field2 = START_NEW_CONTEXT_ONLY;
		MEM_WRITE(Z502Context, &mmio);     // Start up the context
	}
}

void OSSuspendCurrentProcess() {
	MEMORY_MAPPED_IO mmio;
	pcbTable->Cur_Running_Number -= 1;
	struct Process_Control_Block* PCB = CurrentPCB();
	printf("-----------------------------------1, PID: %d\n", PCB->ProcessID);
	PCB->ProcessState = PCB_STATE_LIVE;

	mmio.Mode = Z502StartContext;
	mmio.Field1 = 0;
	mmio.Field2 = SUSPEND_CURRENT_CONTEXT_ONLY;
	MEM_WRITE(Z502Context, &mmio);     // Start up the context
}

#define MAX_PCB_NUMBER 10

struct Process_Control_Block *OSCreateProcess(long *ProcessName, long *Test_To_Run, 
							long *Priority, long *ProcessID, long *ErrorReturned){
	//check input
	if ((int)Priority < 0){
		*ErrorReturned = ERR_BAD_PARAM;
		return NULL;
	}
	else if (findPCBbyProcessName((char*)ProcessName) != NULL){
		*ErrorReturned = ERR_BAD_PARAM;
		return NULL;
	}
	else if (pcbTable->Element_Number >= MAX_PCB_NUMBER){
		*ErrorReturned = ERR_BAD_PARAM;
		return NULL;
	}
	else{
		*ErrorReturned = ERR_SUCCESS;
	}

	void *PageTable = (void *)calloc(2, VIRTUAL_MEM_PAGES);
	MEMORY_MAPPED_IO mmio;

	struct Process_Control_Block *newPCB = (struct Process_Control_Block*)malloc(sizeof(struct Process_Control_Block));

	mmio.Mode = Z502InitializeContext;
	mmio.Field1 = 0;
	mmio.Field2 = (long)Test_To_Run;//test 1a
	mmio.Field3 = (long)PageTable;
	MEM_WRITE(Z502Context, &mmio);   // Initialize Context

	newPCB->ContextID = mmio.Field1;
	newPCB->Priority = (int)Priority;
	newPCB->ProcessID = pcbTable->Element_Number ;
	char* newProcessName = (char*)calloc(sizeof(char),16);
	strcpy(newProcessName, (char*)ProcessName);
	newPCB->ProcessName = newProcessName;
	newPCB->ProcessState = PCB_STATE_LIVE;
	newPCB->ProcessLocation = PCB_LOCATION_FLOATING;
	*ProcessID = newPCB->ProcessID;

	printf("Create PCB: ProcessName: %s; PID: %d\n", (char*)ProcessName, newPCB->ProcessID);

	return newPCB;
}

void OSStartProcess(struct Process_Control_Block* PCB){
	MEMORY_MAPPED_IO mmio;

	mmio.Mode = Z502StartContext;
	mmio.Field1 = PCB->ContextID;
	mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
	MEM_WRITE(Z502Context, &mmio);     // Start up the context
}

void IdleProcess(){
	MEMORY_MAPPED_IO mmio;    //for hardware interface
	// Go idle until the interrupt occurs
	mmio.Mode = Z502Action;
	mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
	MEM_WRITE(Z502Idle, &mmio);       //  Let the interrupt for this timer occur
	DoSleep(10);                       // Give it a little more time
}

void HaltProcess(){
	MEMORY_MAPPED_IO mmio;    //for hardware interface
	mmio.Mode = Z502Action;
	mmio.Field1 = mmio.Field2 = mmio.Field3 = mmio.Field4 = 0;
	MEM_WRITE(Z502Halt, &mmio);
}

void TerminateProcess(struct Process_Control_Block *PCB) {
	if (PCB != NULL) {
		if (PCBLiveNumber() > 1) {
			PCB->ProcessState = PCB_STATE_TERMINATE;
			pcbTable->Terminated_Number += 1;

			if (PCB == CurrentPCB()) {
				Dispatcher();
			}
		}
		else {
			HaltProcess();
		}
	}
}

void SuspendProcess(struct Process_Control_Block *PCB) {
	if (PCB != NULL) {
		if (PCBLiveNumber() > 1) {
			PCB->ProcessState = PCB_STATE_SUSPEND;
			pcbTable->Suspended_Number += 1;

			if (PCB == CurrentPCB()) {
				Dispatcher();
			}
		}
	}
}

void ResumeProcess(struct Process_Control_Block *PCB) {
	if (PCB != NULL) {
		if (PCB->ProcessState == PCB_STATE_SUSPEND) {
			PCB->ProcessState = PCB_STATE_LIVE;
			pcbTable->Suspended_Number -= 1;

			if (PCB->ProcessLocation != PCB_LOCATION_READY_QUEUE
				&& PCB->ProcessLocation != PCB_LOCATION_TIMER_QUEUE) {
				enReadyQueue(PCB);
			}
		}
	}
}
/*****************************************************************************/


/******************************Timer Control**********************************/
void SetTimer(long SleepTime){
	MEMORY_MAPPED_IO mmio;    //for hardware interface

	if (SleepTime > 0){
		mmio.Mode = Z502Start;
		mmio.Field1 = SleepTime;   // You pick the time units
		mmio.Field2 = mmio.Field3 = 0;
		MEM_WRITE(Z502Timer, &mmio);
	}
	else{
//		printf("ERROR: Time reset NEGATIVE!!!");
	}
}

int ResetTimer(){
	MEMORY_MAPPED_IO mmio;    //for hardware interface
	int SleepTime;

	if (timerQueue->Element_Number > 0){
		SleepTime = timerQueue->First_Element->PCB->WakeUpTime - CurrentTime();

		if (SleepTime > 0){
			mmio.Mode = Z502Start;
			mmio.Field1 = SleepTime;   // You pick the time units
			mmio.Field2 = mmio.Field3 = 0;
			MEM_WRITE(Z502Timer, &mmio);

			return 1;
		}
		else{
			return 0;
		}
	}
	else{
		return -1;
	}
}

void lockTimer() {
	READ_MODIFY(MEMORY_INTERLOCK_TIMER, DO_LOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
}

void unlockTimer() {
	READ_MODIFY(MEMORY_INTERLOCK_TIMER, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
}
/*****************************************************************************/