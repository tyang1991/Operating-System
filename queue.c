#include "queue.h"
#include "stdio.h"
#include "string.h"
#include "Control.h"
#include "syscalls.h"
#include "global.h"
#include "Utility.h"
#include "stdlib.h"
#include "protos.h"

/*********************PCB Table************************/
//initialize PCB Table
void initPCBTable(){
	pcbTable = (struct PCB_Table*)malloc(sizeof(struct PCB_Table));
	pcbTable->Element_Number = 0;
	pcbTable->Suspended_Number = 0;
	pcbTable->Terminated_Number = 0;
	pcbTable->Msg_Suspended_Number = 0;
	pcbTable->Cur_Running_Number = 0;
}

//put a PCB into PCB table
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

//calculate PCBs that are runnable at present
int PCBLiveNumber() {
	return pcbTable->Element_Number - pcbTable->Suspended_Number
		- pcbTable->Terminated_Number - pcbTable->Msg_Suspended_Number;
}

//These two functions are used to lock and unlock PCB table
void lockPCBTable() {
	READ_MODIFY(MEMORY_INTERLOCK_PCB_Table, DO_LOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
}

void unlockPCBTable() {
	READ_MODIFY(MEMORY_INTERLOCK_PCB_Table, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
}
/*******************************************************/

/*********************Timer Queue**********************/
//initialize timer queue
void initTimerQueue(){
	timerQueue = (struct Timer_Queue*)malloc(sizeof(struct Timer_Queue));
	timerQueue->Element_Number = 0;
}

//put a PCB into timer queue
void enTimerQueue(struct Process_Control_Block *PCB){
	//lock timer queue
	lockTimerQueue();
	//Set Location
	PCB->ProcessLocation = PCB_LOCATION_TIMER_QUEUE;
	//create a new timer queue element
	struct Timer_Queue_Element *newElement = (struct Timer_Queue_Element*)malloc(sizeof(struct Timer_Queue_Element));
	newElement->PCB = PCB;
	int insertPosition = -1;
	
	//if timer queue is empty
	if (timerQueue->Element_Number == 0){
		//build timer queue element
		newElement->Prev_Element = newElement;
		newElement->Next_Element = newElement;
		//set inser position
		insertPosition = 0;
	}
	//if timer queue not empty
	else{
		//create a pointer for iteration
		struct Timer_Queue_Element *checkingElement = timerQueue->First_Element;
		//check wake up time of each PCB to insert 
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

//take and return the first PCB of timer queue
struct Process_Control_Block *deTimerQueue(){
	//if timer queue empty, return NULL
	if (timerQueue->Element_Number == 0){
		printf("There is no element in timer queue\n");
		return NULL;
	}
	//if not empty
	else{
		//lock timer queue
		lockTimerQueue();
		struct Process_Control_Block *PCB = timerQueue->First_Element->PCB;
		//change PCB state
		PCB->ProcessLocation = PCB_LOCATION_FLOATING;

		//if only one PCB lift in timer queue
		if (timerQueue->Element_Number == 1){
			timerQueue->First_Element = NULL;
		}
		//more than one PCB in timer queue
		else{
			//change queue link
			timerQueue->First_Element->Prev_Element->Next_Element = timerQueue->First_Element->Next_Element;
			timerQueue->First_Element->Next_Element->Prev_Element = timerQueue->First_Element->Prev_Element;
			timerQueue->First_Element = timerQueue->First_Element->Next_Element;
		}
		timerQueue->Element_Number -= 1;
		//unlock timer queue
		unlockTimerQueue();

		//return timerout PCB
		return PCB;
	}
}

//for lock and unlock timer queue
void lockTimerQueue() {
	READ_MODIFY(MEMORY_INTERLOCK_TIMER_QUEUE, DO_LOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
}

void unlockTimerQueue() {
	READ_MODIFY(MEMORY_INTERLOCK_TIMER_QUEUE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
}
/*******************************************************/

/*********************Ready Queue**********************/
//initialize ready queue
void initReadyQueue(){
	readyQueue = (struct Ready_Queue*)malloc(sizeof(struct Ready_Queue));
	readyQueue->Element_Number = 0;
}

//put a PCB in ready queue in the order of priority
void enReadyQueue(struct Process_Control_Block *PCB){
	//lock ready queue
	lockReadyQueue();

	struct Ready_Queue_Element *newElement = (struct Ready_Queue_Element*)malloc(sizeof(struct Ready_Queue_Element));
	newElement->PCB = PCB;
	int insertPosition = -1;
	//set PCB position
	PCB->ProcessLocation = PCB_LOCATION_READY_QUEUE;

	//if ready queue empty
	if (readyQueue->Element_Number == 0){
		//build ready queue element
		newElement->Prev_Element = newElement;
		newElement->Next_Element = newElement;
		//set inser position
		insertPosition = 0;
	}
	//if ready queue not empty
	else{
		struct Ready_Queue_Element *checkingElement = readyQueue->First_Element;
		//check every element based on priority
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
	//unlock ready queue
	unlockReadyQueue();
}

//take and return the first PCB in ready queue
struct Process_Control_Block *deReadyQueue(){
	//if ready queue empty, return NULL
	if (readyQueue->Element_Number == 0){
		printf("There is no element in ready queue\n");
		return NULL;
	}
	//if ready queue not empty
	else{
		//lock ready queue
		lockReadyQueue();

		struct Process_Control_Block *PCB = readyQueue->First_Element->PCB;
		//set location
		PCB->ProcessLocation = PCB_LOCATION_FLOATING;

		//if only one PCB in ready queue
		if (readyQueue->Element_Number == 1){
			readyQueue->First_Element = NULL;
		}
		//if more than one PCB in ready queue
		else{
			readyQueue->First_Element->Prev_Element->Next_Element = readyQueue->First_Element->Next_Element;
			readyQueue->First_Element->Next_Element->Prev_Element = readyQueue->First_Element->Prev_Element;
			readyQueue->First_Element = readyQueue->First_Element->Next_Element;
		}
		readyQueue->Element_Number -= 1;
		//unlock ready queue
		unlockReadyQueue();
		//return timerout PCB
		return PCB;
	}
}

//take a certain PCB out of ready queue, for CHANGE_PRIORITY
struct Process_Control_Block *deCertainPCBFromReadyQueue(int PID) {
	//if ready queue empty
	if (readyQueue->Element_Number == 0) {
		printf("There is no element in ready queue\n");
		return NULL;
	}
	//if ready queue not empty
	else {
		//lock ready queue
		lockReadyQueue();

		struct Ready_Queue_Element *checkingElement = readyQueue->First_Element;
		//find the PCB regarding PID
		for (int i = 0; i < readyQueue->Element_Number; i++) {
			//if PCB found
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
		//unlock ready queue
		unlockReadyQueue();

		return checkingElement->PCB;
	}
}

//lock and unlock ready queue
void lockReadyQueue() {
	READ_MODIFY(MEMORY_INTERLOCK_READY_QUEUE, DO_LOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
}

void unlockReadyQueue() {
	READ_MODIFY(MEMORY_INTERLOCK_READY_QUEUE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
}
/*******************************************************/

/************************Message************************/
//initialize message table
void initMessageTable() {
	messageTable = (struct Message_Table*)malloc(sizeof(struct Message_Table));
	messageTable->Element_Number = 0;
}

#define         MAX_MESSAGE_LENGTH           (INT16)64
#define         MAX_MASSAGE_NUMBER           (INT16)10

//create and return a message
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

//put a message into message table in the order of coming order
void enMessageTable(struct Message *Message) {
	lockMessageTable();

	struct Message_Table_Element *newElement = (struct Message_Table_Element*)malloc(sizeof(struct Message_Table_Element));
	newElement->Message = Message;

	//if message table empty
	if (messageTable->Element_Number == 0) {
		newElement->Prev_Element = newElement;
		newElement->Next_Element = newElement;
		messageTable->First_Element = newElement;
	}
	//if not empty
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

//remove a message from message table
void RemoveMessage(struct Message_Table_Element *MessageToRemove) {
	lockMessageTable();

	if (MessageToRemove == NULL || messageTable->Element_Number == 0) {
		printf("Unable to remove Message\n");
	}
	//reconnect linking message
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

//find out if a message in message table
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

	//check every message until found
	for (int i = 0; i < messageTable->Element_Number; i++) {
		//when receive a targeted message
		if (Source_PID != -1) {
			//find a message from a certain sender
			if ((checkingElement->Message->Target_PID == receiverPID || checkingElement->Message->Target_PID == -1)
				&& checkingElement->Message->Sender_PID == Source_PID){
					//if message can be received
				if (ReceiveLength >= checkingElement->Message->Message_Length) {
					*ErrorReturned_ReceiveMessage = ERR_SUCCESS;
					//if a broadcast message, return actual sender PID
					if (checkingElement->Message->Target_PID == -1) {
						*ActualSourcePID = checkingElement->Message->Sender_PID;
					}
					//return send length
					*ActualSendLength = checkingElement->Message->Message_Length;
					//copy message
					strcpy(ReceiveBuffer, checkingElement->Message->Message_Buffer);
					//remove message from message table
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
		//when receive from a targeted or broadcast message
		else {
			//if found
			if ( (checkingElement->Message->Target_PID == receiverPID || checkingElement->Message->Target_PID == -1) 
				&& checkingElement->Message->Sender_PID != receiverPID ) {
				//if message length shorter than receive length, copy message information back
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

//lock and unlock message table
void lockMessageTable() {
	READ_MODIFY(MEMORY_INTERLOCK_MESSAGE_TABLE, DO_LOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
}

void unlockMessageTable() {
	READ_MODIFY(MEMORY_INTERLOCK_MESSAGE_TABLE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
}

/*******************************************************/

/************************Disk***************************/
void initDiskTable() {
	DiskTable = (struct Disk_Table*)malloc(sizeof(struct Disk_Table));
	for (int i = 0; i < MAX_NUMBER_OF_DISKS + 1; i++) {
		DiskTable->Disk_Number[i] = NULL;
	}
}

struct DISK_OP *CreateDiskOp(int DiskOp, long DiskID, long Sector, char *Data, int PID) {
	struct DISK_OP *newDiskOp = (struct DISK_OP*)malloc(sizeof(struct DISK_OP));
	newDiskOp->Disk_Operation = DiskOp;
	newDiskOp->DiskID = DiskID;
	newDiskOp->Sector = Sector;
	newDiskOp->Data = Data;
	newDiskOp->PID = PID;

	return newDiskOp;
}

void enDiskQueue(struct DISK_OP *DiskOp) {
	lockDiskQueue();

	struct Disk_Table_Element *newElement = (struct Disk_Table_Element*)malloc(sizeof(struct Disk_Table_Element));
	newElement->Disk_Op = DiskOp;

	if (DiskTable->Disk_Number[DiskOp->DiskID] == NULL) {
		newElement->Prev_Element = newElement;
		newElement->Next_Element = newElement;
		DiskTable->Disk_Number[DiskOp->DiskID] = newElement;
	}
	else {
		newElement->Prev_Element = DiskTable->Disk_Number[DiskOp->DiskID]->Prev_Element;
		newElement->Next_Element = DiskTable->Disk_Number[DiskOp->DiskID];
		DiskTable->Disk_Number[DiskOp->DiskID]->Prev_Element->Next_Element = newElement;
		DiskTable->Disk_Number[DiskOp->DiskID]->Prev_Element = newElement;
	}

	unlockDiskQueue();
}

void lockDiskQueue() {
	READ_MODIFY(MEMORY_INTERLOCK_DISK_QUEUE, DO_LOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
}

void unlockDiskQueue() {
	READ_MODIFY(MEMORY_INTERLOCK_DISK_QUEUE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED,
		&LockResult);
}
/*******************************************************/