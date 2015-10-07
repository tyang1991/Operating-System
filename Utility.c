#include "queue.h"
#include "stdio.h"
#include "string.h"
#include "Utility.h"

/********************  Find  ***********************************************/
int findProcessNameInTable(char* ProcessName){
	if (pcbTable->Element_Number == 0){
		return 0;// if no PCB, return 0
	}
	else{
		struct PCB_Table_Element *checkingElement = pcbTable->First_Element;
		for (int i = 0; i < pcbTable->Element_Number; i++){
			if (strcmp(checkingElement->PCB->ProcessName, ProcessName) == 0){
				return 1;//if find a same ProcessName, return 1
			}
		}
	}
	return 0;//if ProcessName not found, return 0
}

int findPCBIDbyName(char* ProcessName){
	if (findPCBbyProcessName(ProcessName) != NULL){
		struct Process_Control_Block *PCB = findPCBbyProcessName(ProcessName);
		return PCB->ProcessID;
	}
	else{
		return -1;
	}
}

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
			if (checkingElement->PCB->ProcessState == PCB_STATE_DEAD){
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

struct Process_Control_Block *findPCBbyProcessID(long ProcessID){
	//if PCB table is empty, return null PCB
	if (pcbTable->Element_Number == 0){
		//		printf("empty pcbTable, NULL returned\n");
		return NULL;
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
/***************************************************************************/

/**************************  Print  State  *********************************/
void PrintPIDinReadyQueue(){
	if (readyQueue->Element_Number == 0){
		return;
	}
	else{
		struct Ready_Queue_Element* checkingElement = readyQueue->First_Element;
		printf("  ReadyQueue PID list: ");
		for (int i = 0; i < readyQueue->Element_Number; i++){
			printf("%d ", checkingElement->PCB->ProcessID);
			if (i != readyQueue->Element_Number - 1){
				checkingElement = checkingElement->Next_Element;
			}
		}
		printf("\n");
	}
}

void PrintPIDinTimerQueue() {
	if (timerQueue->Element_Number == 0) {
		return;
	}
	else {
		struct Timer_Queue_Element* checkingElement = timerQueue->First_Element;
		printf("  TimerQueue PID list: ");
		for (int i = 0; i < timerQueue->Element_Number; i++) {
			printf("%d ", checkingElement->PCB->ProcessID);
			if (i != timerQueue->Element_Number - 1) {
				checkingElement = checkingElement->Next_Element;
			}
		}
		printf("\n");
	}
}

void PrintCurrentPID(){
	if (currentPCB == NULL){
		printf("  No currentPCB\n");
	}
	else{
		printf("  Current PID: %d\n", currentPCB->ProcessID);
	}
}

void PrintPIDinPCBTable() {
	if (pcbTable->Element_Number == 0) {
		return;
	}
	else {
		struct PCB_Table_Element* checkingElement = pcbTable->First_Element;
		printf("PCBTable PID list: ");
		for (int i = 0; i < pcbTable->Element_Number; i++) {
			printf("%d ", checkingElement->PCB->ProcessID);
			if (i != pcbTable->Element_Number - 1) {
				checkingElement = checkingElement->Next_Element;
			}
		}
		printf("\n");
	}
}

void PrintCurrentState(){
	PrintCurrentPID();
	PrintPIDinReadyQueue();
	PrintPIDinTimerQueue();
}

/***************************************************************************/