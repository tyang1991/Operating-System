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

//In multiprocessor mode
//This function is used to start a new PCB without suspending others
void OSStartProcess_Only(struct Process_Control_Block* PCB) {
	if (PCB->ProcessState != PCB_STATE_RUNNING) {
		MEMORY_MAPPED_IO mmio;
		pcbTable->Cur_Running_Number += 1;
		//set PCB state
		PCB->ProcessState = PCB_STATE_RUNNING;

		mmio.Mode = Z502StartContext;
		mmio.Field1 = PCB->ContextID;
		mmio.Field2 = START_NEW_CONTEXT_ONLY;
		MEM_WRITE(Z502Context, &mmio);     // Start up the context
	}
}

//In multiprocessor mode
//This function is used to suspend current running PCB
void OSSuspendCurrentProcess() {
	MEMORY_MAPPED_IO mmio;
	pcbTable->Cur_Running_Number -= 1;
	struct Process_Control_Block* PCB = CurrentPCB();
	//set PCB state
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
	//if bad inputs checked, return NULL and error
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
	//create a new PCB and allocate memory
	struct Process_Control_Block *newPCB = (struct Process_Control_Block*)malloc(sizeof(struct Process_Control_Block));

	//make context
	mmio.Mode = Z502InitializeContext;
	mmio.Field1 = 0;
	mmio.Field2 = (long)Test_To_Run;//test 1a
	mmio.Field3 = (long)PageTable;
	MEM_WRITE(Z502Context, &mmio);   // Initialize Context

	//pass PCB information into PCB created
	newPCB->ContextID = mmio.Field1;
	newPCB->Priority = (int)Priority;
	newPCB->ProcessID = pcbTable->Element_Number ;
	char* newProcessName = (char*)calloc(sizeof(char),16);
	strcpy(newProcessName, (char*)ProcessName);
	newPCB->ProcessName = newProcessName;
	newPCB->ProcessState = PCB_STATE_LIVE;
	newPCB->ProcessLocation = PCB_LOCATION_FLOATING;
	*ProcessID = newPCB->ProcessID;

	//return the PCB created
	return newPCB;
}

//In uniprocessor mode
//This function starts a PCB and suspend others
void OSStartProcess(struct Process_Control_Block* PCB){
	MEMORY_MAPPED_IO mmio;

	mmio.Mode = Z502StartContext;
	mmio.Field1 = PCB->ContextID;
	mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
	MEM_WRITE(Z502Context, &mmio);     // Start up the context
}

//This function halts the whole system
void HaltProcess(){
	MEMORY_MAPPED_IO mmio;    //for hardware interface
	mmio.Mode = Z502Action;
	mmio.Field1 = mmio.Field2 = mmio.Field3 = mmio.Field4 = 0;
	MEM_WRITE(Z502Halt, &mmio);
}

//This function terminates a PCB
void TerminateProcess(struct Process_Control_Block *PCB) {
	//check if PCB passed into, and terminate only if not NULL
	if (PCB != NULL) {
		//set PCB state
		PCB->ProcessState = PCB_STATE_TERMINATE;
		pcbTable->Terminated_Number += 1;
		//terminate if more than one runnable PCBs, otherwise halt
		if (PCBLiveNumber() > 1) {
			//do more actions when terminating current PCB
			if (PCB == CurrentPCB()) {
				//if uniprocessor, start a new PCB
				if (ProcessorMode == Uniprocessor) {
					Dispatcher();
				}
				//if multiprocessor, suspend itself
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

//This function suspends a PCB
void SuspendProcess(struct Process_Control_Block *PCB) {
	//check input PCB, suspend only if it's not NULL
	if (PCB != NULL) {
		//suspend only if there're other PCBs alive
		if (PCBLiveNumber() > 1) {
			//set PCB state
			PCB->ProcessState = PCB_STATE_SUSPEND;
			pcbTable->Suspended_Number += 1;
			//if suspend current PCB, start a new one
			if (PCB == CurrentPCB()) {
				Dispatcher();
			}
		}
	}
}

//This function resumes a PCB
void ResumeProcess(struct Process_Control_Block *PCB) {
	//check input PCB, do action only if PCB is not NULL
	if (PCB != NULL) {
		//resume a process only if it has been suspended
		if (PCB->ProcessState == PCB_STATE_SUSPEND) {
			//set state
			PCB->ProcessState = PCB_STATE_LIVE;
			pcbTable->Suspended_Number -= 1;
			//if the PCB is not on ready queue nor timer queue, put the
			//PCB on ready queue
			if (PCB->ProcessLocation != PCB_LOCATION_READY_QUEUE
				&& PCB->ProcessLocation != PCB_LOCATION_TIMER_QUEUE) {
				enReadyQueue(PCB);
			}
		}
	}
}
/*****************************************************************************/


/******************************Timer Control**********************************/
//This function set the timer to specified value
void SetTimer(long SleepTime){
	MEMORY_MAPPED_IO mmio;    //for hardware interface
	//only set timer when sleep time is positive
	if (SleepTime > 0){
		mmio.Mode = Z502Start;
		mmio.Field1 = SleepTime;   // You pick the time units
		mmio.Field2 = mmio.Field3 = 0;
		MEM_WRITE(Z502Timer, &mmio);
	}
}

//This function reset timer according to first PCB in timer queue
int ResetTimer(){
	MEMORY_MAPPED_IO mmio;    //for hardware interface
	int SleepTime;

	//when timer queue is not empty
	if (timerQueue->Element_Number > 0){
		//calculate sleep time
		SleepTime = timerQueue->First_Element->PCB->WakeUpTime - CurrentTime();
		//set timer only if it's positive
		if (SleepTime > 0){
			mmio.Mode = Z502Start;
			mmio.Field1 = SleepTime;   // You pick the time units
			mmio.Field2 = mmio.Field3 = 0;
			MEM_WRITE(Z502Timer, &mmio);
			//if success, return 1
			return 1;
		}
		else{
			//if sleep time is negative, return 0
			return 0;
		}
	}
	else{
		//if timer queue empty, return -1
		return -1;
	}
}

//These two functions are in charge of locking and unlocking timer
void lockTimer() {
	READ_MODIFY(MEMORY_INTERLOCK_TIMER, DO_LOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
}

void unlockTimer() {
	READ_MODIFY(MEMORY_INTERLOCK_TIMER, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
}
/*****************************************************************************/