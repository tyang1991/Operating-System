#include "queue.h"
#include "stdio.h"
#include "string.h"
#include "Control.h"
#include "syscalls.h"
#include "global.h"

#define                  DO_LOCK                     1
#define                  DO_UNLOCK                   0
#define                  SUSPEND_UNTIL_LOCKED        TRUE
#define                  DO_NOT_SUSPEND              FALSE
INT32 LockResult;
char mySuccess[] = "      Action Failed\0        Action Succeeded";
#define          mySPART          22
#define      MEMORY_INTERLOCK_READY_QUEUE     MEMORY_INTERLOCK_BASE+1
#define      MEMORY_INTERLOCK_TIMER_QUEUE     MEMORY_INTERLOCK_READY_QUEUE+1

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

//Timer Queue
void initTimerQueue(){
	timerQueue = (struct Timer_Queue*)malloc(sizeof(struct Timer_Queue));
	timerQueue->Element_Number = 0;
}

void enTimerQueue(struct Process_Control_Block *PCB){
	//lock timer queue
	lockTimerQueue();
	PCB->ProcessLocation = PCB_LOCATION_TIMER_QUEUE;

	struct Timer_Queue_Element *newElement = (struct Timer_Queue_Element*)malloc(sizeof(struct Timer_Queue_Element));
	newElement->PCB = PCB;
	int insertPosition = -1;
	
	if (timerQueue->Element_Number == 0){
		//build timer queue element
		newElement->Prev_Element = newElement;
		newElement->Next_Element = newElement;
		//set inser position
		insertPosition = 0;
	}
	else{
		struct Timer_Queue_Element *checkingElement = timerQueue->First_Element;

		for (insertPosition = 0; insertPosition < timerQueue->Element_Number; insertPosition++){
			if (newElement->PCB->WakeUpTime < checkingElement->PCB->WakeUpTime){
				//change list links
				newElement->Prev_Element = checkingElement->Prev_Element;
				newElement->Next_Element = checkingElement;
				checkingElement->Prev_Element->Next_Element = newElement;
				checkingElement->Prev_Element = newElement;
				break;
			}
			else{
				checkingElement = checkingElement->Next_Element;
			}
			//change Last_Element in timerQueue if newElement is last
			if (insertPosition == timerQueue->Element_Number - 1){
				newElement->Prev_Element = checkingElement->Prev_Element;
				newElement->Next_Element = checkingElement;
				checkingElement->Prev_Element->Next_Element = newElement;
				checkingElement->Prev_Element = newElement;
			}
		}
	}

	//make change to timerQueue Header
	if (insertPosition == 0){
		timerQueue->First_Element = newElement;
	}
	timerQueue->Element_Number += 1;

	//unlock timer queue
	unlockTimerQueue();
}

struct Process_Control_Block *deTimerQueue(){
	if (timerQueue->Element_Number == 0){
		printf("There is no element in timer queue\n");
	}
	else{
		lockTimerQueue();
		struct Process_Control_Block *PCB = timerQueue->First_Element->PCB;
		PCB->ProcessLocation = PCB_LOCATION_FLOATING;

		if (timerQueue->Element_Number == 1){
			timerQueue->First_Element = NULL;
		}
		else{
			//we don't care about the discarded element
			timerQueue->First_Element->Prev_Element->Next_Element = timerQueue->First_Element->Next_Element;
			timerQueue->First_Element->Next_Element->Prev_Element = timerQueue->First_Element->Prev_Element;
			timerQueue->First_Element = timerQueue->First_Element->Next_Element;
		}
		timerQueue->Element_Number -= 1;

		unlockTimerQueue();

		//return timerout PCB
		return PCB;
	}
}

void lockTimerQueue() {
	READ_MODIFY(MEMORY_INTERLOCK_TIMER_QUEUE, DO_LOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
//	printf("%s\n", &(mySuccess[mySPART * LockResult]));
}

void unlockTimerQueue() {
	READ_MODIFY(MEMORY_INTERLOCK_TIMER_QUEUE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
//	printf("%s\n", &(mySuccess[mySPART * LockResult]));
}

//Ready Queue
void initReadyQueue(){
	readyQueue = (struct Ready_Queue*)malloc(sizeof(struct Ready_Queue));
	readyQueue->Element_Number = 0;
}

void enReadyQueue(struct Process_Control_Block *PCB){
	lockReadyQueue();

	struct Ready_Queue_Element *newElement = (struct Ready_Queue_Element*)malloc(sizeof(struct Ready_Queue_Element));
	newElement->PCB = PCB;
	int insertPosition = -1;
	PCB->ProcessLocation = PCB_LOCATION_READY_QUEUE;

	if (readyQueue->Element_Number == 0){
		//build ready queue element
		newElement->Prev_Element = newElement;
		newElement->Next_Element = newElement;
		//set inser position
		insertPosition = 0;
	}
	else{
		struct Ready_Queue_Element *checkingElement = readyQueue->First_Element;

		for (insertPosition = 0; insertPosition < readyQueue->Element_Number; insertPosition++){
			if (newElement->PCB->Priority < checkingElement->PCB->Priority){
				//change list links
				newElement->Prev_Element = checkingElement->Prev_Element;
				newElement->Next_Element = checkingElement;
				checkingElement->Prev_Element->Next_Element = newElement;
				checkingElement->Prev_Element = newElement;
				//change First_Element in timerQueue if newElement is first
				break;
			}
			else{
				checkingElement = checkingElement->Next_Element;
			}
			//when checked a loop, place it at end
			if (insertPosition == readyQueue->Element_Number - 1){
				newElement->Prev_Element = checkingElement->Prev_Element;
				newElement->Next_Element = checkingElement;
				checkingElement->Prev_Element->Next_Element = newElement;
				checkingElement->Prev_Element = newElement;
			}
		}
	}

	//make change to timerQueue Header
	if (insertPosition == 0){
		readyQueue->First_Element = newElement;
	}
	readyQueue->Element_Number += 1;

	unlockReadyQueue();
}

struct Process_Control_Block *deReadyQueue(){
	if (readyQueue->Element_Number == 0){
		printf("There is no element in ready queue\n");
	}
	else{
		lockReadyQueue();
		struct Process_Control_Block *PCB = readyQueue->First_Element->PCB;
		PCB->ProcessLocation = PCB_LOCATION_FLOATING;

		if (readyQueue->Element_Number == 1){
			readyQueue->First_Element = NULL;
		}
		else{
			readyQueue->First_Element->Prev_Element->Next_Element = readyQueue->First_Element->Next_Element;
			readyQueue->First_Element->Next_Element->Prev_Element = readyQueue->First_Element->Prev_Element;
			readyQueue->First_Element = readyQueue->First_Element->Next_Element;
		}
		readyQueue->Element_Number -= 1;

		unlockReadyQueue();

		//return timerout PCB
		return PCB;
	}
}

struct Process_Control_Block *deCertainPCBFromReadyQueue(int PID) {
	if (readyQueue->Element_Number == 0) {
		printf("There is no element in ready queue\n");
	}
	else {
		lockReadyQueue();

		struct Ready_Queue_Element *checkingElement = readyQueue->First_Element;
		
		for (int i = 0; i < readyQueue->Element_Number; i++) {
			if (checkingElement->PCB->ProcessID == PID) {
				checkingElement->Next_Element->Prev_Element = checkingElement->Prev_Element;
				checkingElement->Prev_Element->Next_Element = checkingElement->Next_Element;
				
				if (i == 0) {
					if (readyQueue->Element_Number == 1) {
						readyQueue->First_Element = NULL;
					}
					else {
						readyQueue->First_Element = readyQueue->First_Element->Next_Element;
					}
				}
			}
			else {
				checkingElement = checkingElement->Next_Element;
			}
		}

		readyQueue->Element_Number -= 1;
		unlockReadyQueue();

		return checkingElement->PCB;
	}
}

void lockReadyQueue() {
	READ_MODIFY(MEMORY_INTERLOCK_READY_QUEUE, DO_LOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
//	printf("%s\n", &(mySuccess[mySPART * LockResult]));
}

void unlockReadyQueue() {
	READ_MODIFY(MEMORY_INTERLOCK_READY_QUEUE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
//	printf("%s\n", &(mySuccess[mySPART * LockResult]));
}