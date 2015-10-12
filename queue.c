#include "queue.h"
#include "stdio.h"
#include "string.h"
#include "Control.h"
#include "syscalls.h"
#include "global.h"
#include "Utility.h"

#define                  DO_LOCK                     1
#define                  DO_UNLOCK                   0
#define                  SUSPEND_UNTIL_LOCKED        TRUE
#define                  DO_NOT_SUSPEND              FALSE
INT32 LockResult;
char mySuccess[] = "      Action Failed\0        Action Succeeded";
#define          mySPART          22
#define      MEMORY_INTERLOCK_PCB_Table       MEMORY_INTERLOCK_BASE+1
#define      MEMORY_INTERLOCK_READY_QUEUE     MEMORY_INTERLOCK_PCB_Table+1
#define      MEMORY_INTERLOCK_TIMER_QUEUE     MEMORY_INTERLOCK_READY_QUEUE+1
#define      MEMORY_INTERLOCK_MESSAGE_TABLE   MEMORY_INTERLOCK_TIMER_QUEUE+1
/*********************PCB Table************************/
void initPCBTable(){
	pcbTable = (struct PCB_Table*)malloc(sizeof(struct PCB_Table));
	pcbTable->Element_Number = 0;
	pcbTable->Suspended_Number = 0;
	pcbTable->Terminated_Number = 0;
}

void enPCBTable(struct Process_Control_Block *PCB){
	lockPCBTable();

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

	unlockPCBTable();
}

int PCBLiveNumber() {
	return pcbTable->Element_Number - pcbTable->Suspended_Number - pcbTable->Terminated_Number;
}

void lockPCBTable() {
	READ_MODIFY(MEMORY_INTERLOCK_PCB_Table, DO_LOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	//	printf("%s\n", &(mySuccess[mySPART * LockResult]));
}

void unlockPCBTable() {
	READ_MODIFY(MEMORY_INTERLOCK_PCB_Table, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	//	printf("%s\n", &(mySuccess[mySPART * LockResult]));
}

/*******************************************************/

/*********************Timer Queue**********************/
void initTimerQueue(){
	timerQueue = (struct Timer_Queue*)malloc(sizeof(struct Timer_Queue));
	timerQueue->Element_Number = 0;
}

void enTimerQueue(struct Process_Control_Block *PCB){
	//lock timer queue
	lockTimerQueue();
	//Set Location
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
		return NULL;
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
/*******************************************************/

/*********************Ready Queue**********************/
void initReadyQueue(){
	readyQueue = (struct Ready_Queue*)malloc(sizeof(struct Ready_Queue));
	readyQueue->Element_Number = 0;
}

void enReadyQueue(struct Process_Control_Block *PCB){
	//lock ready queue
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
		return NULL;
	}
	else{
		//lock ready queue
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
		return NULL;
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
/*******************************************************/

/************************Message************************/
void initMessageTable() {
	messageTable = (struct Message_Table*)malloc(sizeof(struct Message_Table));
	messageTable->Element_Number = 0;
}

#define         MAX_MESSAGE_LENGTH           (INT16)64
#define         MAX_MASSAGE_NUMBER           (INT16)10

struct Message *CreateMessage(long Target_PID, char *Message_Buffer, long SendLength, long *ErrorReturned) {
	//check input
	if (Target_PID < -1 || Target_PID > pcbTable->Element_Number - 1) {
		*ErrorReturned = ERR_BAD_PARAM;
		return NULL;
	}
	else if (SendLength < 0 || SendLength > MAX_MESSAGE_LENGTH) {
		*ErrorReturned = ERR_BAD_PARAM;
		return NULL;
	}
	else if (messageTable->Element_Number >= MAX_MASSAGE_NUMBER) {
		*ErrorReturned = ERR_BAD_PARAM;
		return NULL;
	}
	else {
		*ErrorReturned = ERR_SUCCESS;

		struct Process_Control_Block *SenderPCB = CurrentPCB();
		long SenderPID = SenderPCB->ProcessID;

		struct Message *newMessage = (struct Message*)malloc(sizeof(struct Message));
		newMessage->Target_PID = Target_PID;
		newMessage->Sender_PID = SenderPID;
		char *newMessageBuffer = (char*)calloc(sizeof(char), MAX_MESSAGE_LENGTH);
		strcpy(newMessageBuffer, Message_Buffer);
		newMessage->Message_Buffer = newMessageBuffer;
		newMessage->Message_Length = SendLength;

		return newMessage;
	}
}

void enMessageTable(struct Message *Message) {
	lockMessageTable();

	struct Message_Table_Element *newElement = (struct Message_Table_Element*)malloc(sizeof(struct Message_Table_Element));
	newElement->Message = Message;

	if (messageTable->Element_Number == 0) {
		newElement->Prev_Element = newElement;
		newElement->Next_Element = newElement;
		messageTable->First_Element = newElement;
	}
	else {
		//always put new message at the end of message table
		newElement->Prev_Element = messageTable->First_Element->Prev_Element;
		newElement->Next_Element = messageTable->First_Element;
		messageTable->First_Element->Prev_Element->Next_Element = newElement;
		messageTable->First_Element->Prev_Element = newElement;
	}
	messageTable->Element_Number += 1;

	unlockMessageTable();
}

void RemoveMessage(struct Message_Table_Element *MessageToRemove) {
	lockMessageTable();

	if (MessageToRemove == NULL || messageTable->Element_Number == 0) {
		printf("Unable to remove Message\n");
	}
	else {
		MessageToRemove->Prev_Element->Next_Element = MessageToRemove->Next_Element;
		MessageToRemove->Next_Element->Prev_Element = MessageToRemove->Prev_Element;
		if (MessageToRemove == messageTable->First_Element) {
			if (messageTable->Element_Number == 1) {
				messageTable->First_Element = NULL;
			}
			else {
				messageTable->First_Element = MessageToRemove->Next_Element;
			}
		}
		messageTable->Element_Number -= 1;
	}

	unlockMessageTable();
}

#define GO_ON 1
#define STAY  0
int findMessage(long Source_PID, char *ReceiveBuffer, long ReceiveLength, 
	long *ActualSendLength, long *ActualSourcePID, long *ErrorReturned_ReceiveMessage) {

	lockMessageTable();
	//check input
	if (messageTable->Element_Number == 0) {
		*ErrorReturned_ReceiveMessage = ERR_BAD_PARAM;
		return GO_ON;
	}
	else if (Source_PID < -1 || Source_PID > pcbTable->Element_Number - 1) {
		*ErrorReturned_ReceiveMessage = ERR_BAD_PARAM;
		return GO_ON;
	}
	else if (ReceiveLength > MAX_MESSAGE_LENGTH) {
		*ErrorReturned_ReceiveMessage = ERR_BAD_PARAM;
		return GO_ON;
	}

	struct Process_Control_Block *receiverPCB = CurrentPCB();
	long receiverPID = receiverPCB->ProcessID;
	int returnState = STAY;

	struct Message_Table_Element *checkingElement = messageTable->First_Element;

	for (int i = 0; i < messageTable->Element_Number; i++) {
		if (Source_PID != -1) {
			if ( (checkingElement->Message->Target_PID == receiverPID || checkingElement->Message->Target_PID == -1)
				&& checkingElement->Message->Sender_PID == Source_PID){
				if (ReceiveLength >= checkingElement->Message->Message_Length) {

					*ErrorReturned_ReceiveMessage = ERR_SUCCESS;
					if (checkingElement->Message->Target_PID == -1) {//if broadcast
						*ActualSourcePID = checkingElement->Message->Sender_PID;
					}
					*ActualSendLength = checkingElement->Message->Message_Length;
					strcpy(ReceiveBuffer, checkingElement->Message->Message_Buffer);
					RemoveMessage(checkingElement);
				}
				else {
					*ErrorReturned_ReceiveMessage = ERR_BAD_PARAM;
					*ActualSendLength = checkingElement->Message->Message_Length;
				}
				returnState = GO_ON;
				break;
			}
			else {
				if (i != messageTable->Element_Number - 1) {
					checkingElement = checkingElement->Next_Element;
				}
				else {
					*ErrorReturned_ReceiveMessage = ERR_BAD_PARAM;
				}
			}
		}
		else {
			if ( (checkingElement->Message->Target_PID == receiverPID || checkingElement->Message->Target_PID == -1) 
				&& checkingElement->Message->Sender_PID != receiverPID ) {
				if (ReceiveLength >= checkingElement->Message->Message_Length) {
					*ErrorReturned_ReceiveMessage = ERR_SUCCESS;
					*ActualSourcePID = checkingElement->Message->Sender_PID;
					*ActualSendLength = checkingElement->Message->Message_Length;
					strcpy(ReceiveBuffer, checkingElement->Message->Message_Buffer);
					RemoveMessage(checkingElement);
				}
				else {
					*ErrorReturned_ReceiveMessage = ERR_BAD_PARAM;
					*ActualSourcePID = checkingElement->Message->Sender_PID;
					*ActualSendLength = checkingElement->Message->Message_Length;
				}
				returnState = GO_ON;
				break;
			}
			else {
				if (i != messageTable->Element_Number - 1) {
					checkingElement = checkingElement->Next_Element;
				}
				else {
					*ErrorReturned_ReceiveMessage = ERR_BAD_PARAM;
				}
			}
		}
	}

	unlockMessageTable();
	return returnState;
}

void lockMessageTable() {
	READ_MODIFY(MEMORY_INTERLOCK_MESSAGE_TABLE, DO_LOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	//	printf("%s\n", &(mySuccess[mySPART * LockResult]));
}

void unlockMessageTable() {
	READ_MODIFY(MEMORY_INTERLOCK_MESSAGE_TABLE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
	//	printf("%s\n", &(mySuccess[mySPART * LockResult]));
}
