#include "queue.h"
#include "stdio.h"
#include "string.h"

//PCB Table
void initPCBTable(){
	pcbTable = (struct PCB_Table*)malloc(sizeof(struct PCB_Table));
	pcbTable->Element_Number = 0;
}

void enPCBTable(struct Process_Control_Block *PCB){
	struct PCB_Table_Element *newElement = (struct PCB_Table_Element*)malloc(sizeof(struct PCB_Table_Element));
	newElement->PCB = PCB;

	//insert element as the first element
	if (pcbTable->Element_Number == 0){
		pcbTable->First_Element = newElement;
	}
	else{
		newElement->Next_Element = pcbTable->First_Element;
		pcbTable->First_Element = newElement;
	}
	pcbTable->Element_Number += 1;
}

int findProcessNameInTable(char* ProcessName){
	if (pcbTable->Element_Number == 0){
		return 0;// if no PCB, return 0
	}
	else{
		struct PCB_Table_Element *checkingElement = pcbTable->First_Element;
		for (int i = 0; i < pcbTable; i++){
			if (strcmp(checkingElement->PCB->ProcessName, ProcessName) == 0){
				return 1;//if find a same ProcessName, return 1
			}
		}
	}
	return 0;//if ProcessName not found, return 0
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
}

char * findPCBIDbyName(char* ProcessName){
	if (findPCBbyProcessName(ProcessName) != NULL){
		return findPCBbyProcessName(ProcessName)->ProcessID;
	}
	else{
		return NULL;
	}
}

//Timer Queue
void initTimerQueue(){
	timerQueue = (struct Timer_Queue*)malloc(sizeof(struct Timer_Queue));
	timerQueue->Element_Number = 0;
}

void enTimerQueue(struct Process_Control_Block *PCB){
	struct Timer_Queue_Element *newElement = (struct Timer_Queue_Element*)malloc(sizeof(struct Timer_Queue_Element));
	newElement->PCB = PCB;
	
	if (timerQueue->Element_Number == 0){
		//build timer queue element
		newElement->Prev_Element = newElement;
		newElement->Next_Element = newElement;
		//make change in timer queue
		timerQueue->Element_Number = 1;
		timerQueue->First_Element = newElement;
	}
	else{
		struct Timer_Queue_Element *checkingElement = timerQueue->First_Element;

		for (int i = 0; i < timerQueue->Element_Number; i++){
			if (newElement->PCB->WakeUpTime < checkingElement->PCB->WakeUpTime){
				//change list links
				newElement->Prev_Element = checkingElement->Prev_Element;
				newElement->Next_Element = checkingElement;
				checkingElement->Prev_Element->Next_Element = newElement;
				checkingElement->Prev_Element = newElement;
				//change First_Element in timerQueue if newElement is first
				if (i == 0){
					timerQueue->First_Element = newElement;
				}
				break;
			}
			else{
				checkingElement = checkingElement->Next_Element;
			}
			//change Last_Element in timerQueue if newElement is last
			if (i == timerQueue->Element_Number - 1){
				newElement->Prev_Element = checkingElement;
				newElement->Next_Element = checkingElement->Next_Element;
				checkingElement->Next_Element->Prev_Element = newElement;
				checkingElement->Next_Element = newElement;
			}
		}
		//make change to Element_Number in timerQueue
		timerQueue->Element_Number += 1;
	}
}

void deTimerQueue(){
	if (timerQueue->Element_Number == 0){
		printf("There is no element in timer queue\n");
	}
	else if (timerQueue->Element_Number == 1){
		timerQueue->First_Element = NULL;
		timerQueue->Element_Number -= 1;
	}
	else{
		timerQueue->First_Element = timerQueue->First_Element->Next_Element;
		timerQueue->Element_Number -= 1;
	}
}

//Ready Queue
void initReadyQueue(){
	readyQueue = (struct Ready_Queue*)malloc(sizeof(struct Ready_Queue));
	readyQueue->Element_Number = 0;
}

void enReadyQueue(struct Process_Control_Block *PCB){
	struct Ready_Queue_Element *newElement = (struct Ready_Queue_Element*)malloc(sizeof(struct Ready_Queue_Element));
	newElement->PCB = PCB;

	if (readyQueue->Element_Number == 0){
		//build ready queue element
		newElement->Prev_Element = newElement;
		newElement->Next_Element = newElement;
		//make change in timer queue
		readyQueue->Element_Number = 1;
		readyQueue->First_Element = newElement;
	}
	else{
		struct Ready_Queue_Element *checkingElement = readyQueue->First_Element;

		for (int i = 0; i < readyQueue->Element_Number; i++){
			if (newElement->PCB->Priority < checkingElement->PCB->Priority){
				//change list links
				newElement->Prev_Element = checkingElement->Prev_Element;
				newElement->Next_Element = checkingElement;
				checkingElement->Prev_Element->Next_Element = newElement;
				checkingElement->Prev_Element = newElement;
				//change First_Element in timerQueue if newElement is first
				if (i == 0){
					readyQueue->First_Element = newElement;
				}
				break;
			}
			else{
				checkingElement = checkingElement->Next_Element;
			}
			//change Last_Element in timerQueue if newElement is last
			if (i == readyQueue->Element_Number - 1){
				newElement->Prev_Element = checkingElement;
				newElement->Next_Element = checkingElement->Next_Element;
				checkingElement->Next_Element->Prev_Element = newElement;
				checkingElement->Next_Element = newElement;
			}
		}
		//make change to Element_Number in timerQueue
		readyQueue->Element_Number += 1;
	}
}

void deReadyQueue(){
	if (readyQueue->Element_Number == 0){
		printf("There is no element in ready queue\n");
	}
	else if (readyQueue->Element_Number == 1){
		readyQueue->First_Element = NULL;
		readyQueue->Element_Number -= 1;
	}
	else{
		readyQueue->First_Element = readyQueue->First_Element->Next_Element;
		readyQueue->Element_Number -= 1;
	}
}
