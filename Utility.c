#include "queue.h"
#include "stdio.h"
#include "string.h"
#include "Utility.h"
#include "syscalls.h"

/********************  Find  ***********************************************/
struct Process_Control_Block *findPCBbyProcessName(char* ProcessName){
	//if PCB table is empty, return null PCB
	if (pcbTable->Element_Number == 0){
		//		printf("empty pcbTable, NULL returned\n");
		return NULL;
	}

	//create a temp pointer to check elements
	struct PCB_Table_Element *checkingElement = pcbTable->First_Element;

	//check Element_Number-1 times because last element doesn't have a pointer to next
	for (int i = 0; i < pcbTable->Element_Number; i++){
		if (strcmp(checkingElement->PCB->ProcessName, ProcessName) == 0){
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
				//				printf("PCB not found, NULL returned\n");
				return NULL;
			}
			else{
				checkingElement = checkingElement->Next_Element;
			}
		}
	}
	return NULL;
}

struct Process_Control_Block *findPCBbyProcessID(int ProcessID){
	//if PCB table is empty, return null PCB
	if (pcbTable->Element_Number == 0){
		//		printf("empty pcbTable, NULL returned\n");
		return NULL;
	}

	if (ProcessID == -1) {
		ProcessID = CurrentPID();
	}

	//create a temp pointer to check elements
	struct PCB_Table_Element *checkingElement = pcbTable->First_Element;

	//check Element_Number-1 times because last element doesn't have a pointer to next
	for (int i = 0; i < pcbTable->Element_Number; i++){
		if (checkingElement->PCB->ProcessID == ProcessID){
			return checkingElement->PCB;
		}
		else{
			//if not found, return null PCB
			if (i == pcbTable->Element_Number - 1){
				//				printf("PCB not found, NULL returned\n");
				return NULL;
			}
			else{
				checkingElement = checkingElement->Next_Element;
			}
		}
	}
	return NULL;
}

struct Process_Control_Block *findPCBbyContextID(long ContextID) {
	//if PCB table is empty, return null PCB
	if (pcbTable->Element_Number == 0) {
		//		printf("empty pcbTable, NULL returned\n");
		return NULL;
	}

	//create a temp pointer to check elements
	struct PCB_Table_Element *checkingElement = pcbTable->First_Element;

	//check Element_Number-1 times because last element doesn't have a pointer to next
	for (int i = 0; i < pcbTable->Element_Number; i++) {
		if (checkingElement->PCB->ContextID == ContextID) {
			return checkingElement->PCB;
		}
		else {
			//if not found, return null PCB
			if (i == pcbTable->Element_Number - 1) {
				//				printf("PCB not found, NULL returned\n");
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

/************************** Scheduler Printer*******************************/

#define PRINTSTATES 1  //1 to print states; 0 to hide states

void SchedularPrinter(char *TargetAction, int TargetPID) {
	if (PRINTSTATES) {
		SP_INPUT_DATA SPData;    // Used to feed SchedulerPrinter
		memset(&SPData, 0, sizeof(SP_INPUT_DATA));
		strcpy(SPData.TargetAction, TargetAction);
		SPData.CurrentlyRunningPID = CurrentPID();
		SPData.TargetPID = TargetPID;
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

		CALL(SPPrintLine(&SPData));
	}
}
/************************** Scheduler Printer*******************************/