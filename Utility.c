#include "queue.h"
#include "stdio.h"
#include "string.h"
#include "Utility.h"
#include "syscalls.h"
#include "protos.h"
#include "MyTest.h"
#include "Control.h"

/********************  Find  ***********************************************/
//These functions find and return PCB in PCB Table by various properties

struct Process_Control_Block *findPCBbyProcessName(char* ProcessName){
	//if PCB table is empty, return null PCB
	if (pcbTable->Element_Number == 0){
		return NULL;
	}

	//create a temp pointer to check elements
	struct PCB_Table_Element *checkingElement = pcbTable->First_Element;

	//check every PCB to see if any PCB is what we need
	for (int i = 0; i < pcbTable->Element_Number; i++){
		//if PCB found and not terminated, then return PCB
		if (strcmp(checkingElement->PCB->ProcessName, ProcessName) == 0
			&& checkingElement->PCB->ProcessState != PCB_STATE_TERMINATE){
			return checkingElement->PCB;
		}
		else{
			//if not found, return null
			if (i == pcbTable->Element_Number - 1){
				return NULL;
			}
			//check next PCB
			else{
				checkingElement = checkingElement->Next_Element;
			}
		}
	}

	//in case of error, return NULL in the end
	return NULL;
}

struct Process_Control_Block *findPCBbyProcessID(int ProcessID){
	//if PCB table is empty, return null PCB
	if (pcbTable->Element_Number == 0){
		return NULL;
	}

	//if PID = -1, return current running PID
	if (ProcessID == -1) {
		ProcessID = CurrentPID();
	}

	//create a temp pointer to check elements
	struct PCB_Table_Element *checkingElement = pcbTable->First_Element;

	//check every PCB to see if any PCB is what we need
	for (int i = 0; i < pcbTable->Element_Number; i++){
		if (checkingElement->PCB->ProcessID == ProcessID){
			//if PCB found and not terminated, then return PCB
			if (checkingElement->PCB->ProcessState == PCB_STATE_TERMINATE){
				return NULL;
			}
			else{
				return checkingElement->PCB;
			}
		}
		else{
			//if not found, return null PCB
			if (i == pcbTable->Element_Number - 1){
				return NULL;
			}
			else{
				checkingElement = checkingElement->Next_Element;
			}
		}
	}
	//in case of error, return NULL in the end
	return NULL;
}

struct Process_Control_Block *findPCBbyContextID(long ContextID) {
	//if PCB table is empty, return null PCB
	if (pcbTable->Element_Number == 0) {
		return NULL;
	}

	//create a temp pointer to check elements
	struct PCB_Table_Element *checkingElement = pcbTable->First_Element;

	//check every PCB to see if any PCB is what we need
	for (int i = 0; i < pcbTable->Element_Number; i++) {
		if (checkingElement->PCB->ContextID == ContextID) {
			return checkingElement->PCB;
		}
		else {
			//if not found, return null PCB
			if (i == pcbTable->Element_Number - 1) {
				return NULL;
			}
			else {
				checkingElement = checkingElement->Next_Element;
			}
		}
	}
	return NULL;
}
/***************************************************************************/

/****************************Scheduler Printer******************************/
//These two functions are used to lock and unlock SP
void lockSP() {
	READ_MODIFY(MEMORY_INTERLOCK_PRINTER, DO_LOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
}
void unlockSP() {
	READ_MODIFY(MEMORY_INTERLOCK_PRINTER, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
}

//This function prints current States of:
//1. Ready Queue. 2. Timer Queue. 3. PCB suspended 4. PCB message suspended
//5. PCBs currently running in Multiprocessor mode
//Action and PID are passed into function so that it's easy to used by syscalls
void SchedularPrinter(char *TargetAction, int TargetPID) {
	if (PRINTSTATES) {
		//lock SP and other data structure
		lockSP();
		lockPCBTable();
		lockTimerQueue();
		lockReadyQueue();

		// Used to feed SchedulerPrinter
		SP_INPUT_DATA SPData;
		memset(&SPData, 0, sizeof(SP_INPUT_DATA));
		
		//pass args into SPData
		strcpy(SPData.TargetAction, TargetAction);
		SPData.TargetPID = TargetPID;

		//pass current running PID in uniprocessor mode
		if (ProcessorMode == Uniprocessor){
			SPData.CurrentlyRunningPID = currentPCB->ProcessID;
		}
		SPData.NumberOfRunningProcesses = 1;

		//PCB on Ready Queue
		SPData.NumberOfReadyProcesses = readyQueue->Element_Number;
		if (readyQueue->Element_Number > 0) {
			struct Ready_Queue_Element *checkingElement_ReadyQueue = readyQueue->First_Element;
			for (int i = 0; i < SPData.NumberOfReadyProcesses; i++) {
				SPData.ReadyProcessPIDs[i] = checkingElement_ReadyQueue->PCB->ProcessID;
				checkingElement_ReadyQueue = checkingElement_ReadyQueue->Next_Element;
			}
		}

		//PCB on Timer Queue
		SPData.NumberOfTimerSuspendedProcesses = timerQueue->Element_Number;
		if (timerQueue->Element_Number > 0) {
			struct Timer_Queue_Element *checkingElement_TimerQueue = timerQueue->First_Element;
			for (int i = 0; i < timerQueue->Element_Number; i++) {
				SPData.TimerSuspendedProcessPIDs[i] = checkingElement_TimerQueue->PCB->ProcessID;
				checkingElement_TimerQueue = checkingElement_TimerQueue->Next_Element;
			}
		}

		//PCB Suspended
		SPData.NumberOfProcSuspendedProcesses = pcbTable->Suspended_Number;
		if (pcbTable->Suspended_Number > 0) {
			struct PCB_Table_Element *checkingElement_Suspend = pcbTable->First_Element;
			int j = 0;
			for (int i = 0; i < pcbTable->Element_Number; i++) {
				if (checkingElement_Suspend->PCB->ProcessState == PCB_STATE_SUSPEND) {
					SPData.ProcSuspendedProcessPIDs[j] = checkingElement_Suspend->PCB->ProcessID;
					j++;
				}
				checkingElement_Suspend = checkingElement_Suspend->Next_Element;
			}
		}

		//PCB Message Suspended
		SPData.NumberOfMessageSuspendedProcesses = pcbTable->Msg_Suspended_Number;
		if (pcbTable->Msg_Suspended_Number > 0) {
			struct PCB_Table_Element *checkingElement_Mess_Suspend = pcbTable->First_Element;
			int m = 0;
			for (int i = 0; i < pcbTable->Msg_Suspended_Number; i++) {
				if (checkingElement_Mess_Suspend->PCB->ProcessState == PCB_STATE_MSG_SUSPEND) {
					SPData.MessageSuspendedProcessPIDs[m] = checkingElement_Mess_Suspend->PCB->ProcessID;
					m++;
				}
				checkingElement_Mess_Suspend = checkingElement_Mess_Suspend->Next_Element;
			}
		}

		//PCB Running
		SPData.NumberOfRunningProcesses = pcbTable->Cur_Running_Number;
		if (pcbTable->Cur_Running_Number > 0) {
			struct PCB_Table_Element *checkingElement_RunningPCB = pcbTable->First_Element;
			int n = 0;
			for (int i = 0; i < pcbTable->Cur_Running_Number; i++) {
				if (checkingElement_RunningPCB->PCB->ProcessState == PCB_STATE_RUNNING) {
					SPData.RunningProcessPIDs[n] = checkingElement_RunningPCB->PCB->ProcessID;
					n++;
				}
				checkingElement_RunningPCB = checkingElement_RunningPCB->Next_Element;
			}
		}

		//print states using SPData with passed in information
		CALL(SPPrintLine(&SPData));
//		free(SPData);
		//unlock SP and other data structure
		unlockSP();
		unlockPCBTable();
		unlockTimerQueue();
		unlockReadyQueue();
	}
}
/***************************************************************************/

/********************************Test Parser********************************/
//This function returns relavent test pointer regarding to command line
//input information
long *TestParser(char *TestInput){
	if (strcmp(TestInput, "test0") == 0){
		PRINTSTATES = 0;
		return (long *)test0;
	}
	if (strcmp(TestInput, "test1a") == 0){
		PRINTSTATES = 0;
		return (long *)test1a;
	}
	else if (strcmp(TestInput, "test1b") == 0){
		PRINTSTATES = 0;
		return (long *)test1b;
	}
	else if (strcmp(TestInput, "test1c") == 0){
		PRINTSTATES = 1;
		return (long *)test1c;
	}
	else if (strcmp(TestInput, "test1d") == 0){
		PRINTSTATES = 1;
		return (long *)test1d;
	}
	else if (strcmp(TestInput, "test1e") == 0){
		PRINTSTATES = 0;
		return (long *)test1e;
	}
	else if (strcmp(TestInput, "test1f") == 0){
		PRINTSTATES = 1;
		return (long *)test1f;
	}
	else if (strcmp(TestInput, "test1g") == 0){
		PRINTSTATES = 0;
		return (long *)test1g;
	}
	else if (strcmp(TestInput, "test1h") == 0){
		PRINTSTATES = 1;
		return (long *)test1h;
	}
	else if (strcmp(TestInput, "test1i") == 0){
		PRINTSTATES = 0;
		return (long *)test1i;
	}
	else if (strcmp(TestInput, "test1j") == 0){
		PRINTSTATES = 1;
		return (long *)test1j;
	}
	else if (strcmp(TestInput, "test1k") == 0){
		PRINTSTATES = 0;
		return (long *)test1k;
	}
	else if (strcmp(TestInput, "test1myTest") == 0) {
		PRINTSTATES = 1;
		return (long *)test1myTest;
	}
	else if (strcmp(TestInput, "test2a") == 0) {
		PRINTSTATES = 0;
		return (long *)test2a;
	}
	else if (strcmp(TestInput, "test2b") == 0) {
		PRINTSTATES = 0;
		return (long *)test2b;
	}
	else{
		PRINTSTATES = 1;
		return (long *)test1c;
	}
}
/***************************************************************************/
