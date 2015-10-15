#include "global.h"
#include "stdio.h"
#include "syscalls.h"
#include "queue.h"
#include "Control.h"
#include "Utility.h"

/*******************************Current State*********************************/
//These functions returns current information

//This function returns current time of system
long CurrentTime(){
	MEMORY_MAPPED_IO mmio;    //for hardware interface

	mmio.Mode = Z502ReturnValue;
	mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
	MEM_READ(Z502Clock, &mmio);
	return mmio.Field1;
}

//This function returns current running Context
long CurrentContext() {
	MEMORY_MAPPED_IO mmio;    //for hardware interface

	mmio.Mode = Z502GetCurrentContext;
	mmio.Field1 = mmio.Field2 = mmio.Field3 = mmio.Field4 = 0;
	MEM_READ(Z502Context, &mmio);
	return mmio.Field1;
}

//This function returns current running PCB
struct Process_Control_Block *CurrentPCB() {
	long currentContext = CurrentContext();
	struct Process_Control_Block *PCB = findPCBbyContextID(currentContext);
	return PCB;
}

//This function returns current running PID
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
//This function is called whenever another PCB is needed to start
void Dispatcher(){
	switch (ProcessorMode) {
		//in Uniprocessor mode, let Dispatcher_Uniprocessor() take charge.
		case Uniprocessor:
			Dispatcher_Uniprocessor();
			break;
		//in multiprocessor mode, let Dispatcher_Multiprocessor() take charge.
		case Multiprocessor:
			Dispatcher_Multiprocessor();
			break;
	}
}

//This function is in charge of dispatching in uniprocessor mode
void Dispatcher_Uniprocessor() {
	struct Process_Control_Block *PCB;//for temp use

	while (1) {
		while (readyQueue->Element_Number == 0) {
			CALL(1);
		}

		//take the first PCB in ready queue to check status
		PCB = readyQueue->First_Element->PCB;

		//only take first PCB and break if PCB if not terminated nor suspended
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

	//start the PCB
	OSStartProcess(PCB);
}

//This function is in charge of dispatching in multiprocessor mode
void Dispatcher_Multiprocessor() {
	struct Process_Control_Block *PCB;//for temp use

	//check the ready queue forever
	while (1) {
		//wait until ready queue is not empty
		while (readyQueue->Element_Number == 0) {
			CALL(1);
		}

		PCB = NULL;
		//take PCB only if it's not terminated nor suspended
		if (readyQueue->First_Element->PCB->ProcessState == PCB_STATE_LIVE) {
			PCB = deReadyQueue();
		}
		else if (readyQueue->First_Element->PCB->ProcessState == PCB_STATE_TERMINATE) {
			deReadyQueue();
		}
		else if (readyQueue->First_Element->PCB->ProcessState == PCB_STATE_SUSPEND) {
			deReadyQueue();
		}
		else if (readyQueue->First_Element->PCB->ProcessState == PCB_STATE_MSG_SUSPEND) {
			PCB = deReadyQueue();
		}
		//if a PCB is taken, start the PCB
		if (PCB != NULL) {
			OSStartProcess_Only(PCB);
		}
	}
}

//This function is used to start a new PCB without suspending others
//in multiprocessor mode
void OSStartProcess_Only(struct Process_Control_Block* PCB) {
	if (PCB->ProcessState != PCB_STATE_RUNNING) {
		MEMORY_MAPPED_IO mmio;
		pcbTable->Cur_Running_Number += 1;
//		printf("+++++++++++++++++++++++++++++++1, PID: %d\n", PCB->ProcessID);
		PCB->ProcessState = PCB_STATE_RUNNING;

		mmio.Mode = Z502StartContext;
		mmio.Field1 = PCB->ContextID;
		mmio.Field2 = START_NEW_CONTEXT_ONLY;
		MEM_WRITE(Z502Context, &mmio);     // Start up the context
	}
}

//This function is used to suspend current running PCB, in multiprocessor mode
void OSSuspendCurrentProcess() {
	MEMORY_MAPPED_IO mmio;
	pcbTable->Cur_Running_Number -= 1;
	struct Process_Control_Block* PCB = CurrentPCB();
//	printf("-----------------------------------1, PID: %d\n", PCB->ProcessID);
	PCB->ProcessState = PCB_STATE_LIVE;

	mmio.Mode = Z502StartContext;
	mmio.Field1 = 0;
	mmio.Field2 = SUSPEND_CURRENT_CONTEXT_ONLY;
	MEM_WRITE(Z502Context, &mmio);     // Start up the context
}

#define MAX_PCB_NUMBER 10 //limit number of PCB created

//This function creates and returns a PCB
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

void HaltProcess(){
	MEMORY_MAPPED_IO mmio;    //for hardware interface
	mmio.Mode = Z502Action;
	mmio.Field1 = mmio.Field2 = mmio.Field3 = mmio.Field4 = 0;
	MEM_WRITE(Z502Halt, &mmio);
}

void TerminateProcess(struct Process_Control_Block *PCB) {
	if (PCB != NULL) {
		PCB->ProcessState = PCB_STATE_TERMINATE;
		pcbTable->Terminated_Number += 1;

		if (PCBLiveNumber() > 1) {
			if (PCB == CurrentPCB()) {
				if (ProcessorMode == Uniprocessor) {
					//first PCB in Ready Queue starts
					Dispatcher();
				}
				else {
					OSSuspendCurrentProcess();
				}
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