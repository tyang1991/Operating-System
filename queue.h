#include             "global.h"

//definations for lock
#define                  DO_LOCK                     1
#define                  DO_UNLOCK                   0
#define                  SUSPEND_UNTIL_LOCKED        TRUE
#define                  DO_NOT_SUSPEND              FALSE
INT32 LockResult;
//char mySuccess[] = "      Action Failed\0        Action Succeeded";

//lock address
#define          mySPART          22
#define      MEMORY_INTERLOCK_PCB_Table       MEMORY_INTERLOCK_BASE+1             //PCB Table
#define      MEMORY_INTERLOCK_READY_QUEUE     MEMORY_INTERLOCK_PCB_Table+1        //Ready Queue
#define      MEMORY_INTERLOCK_TIMER_QUEUE     MEMORY_INTERLOCK_READY_QUEUE+1      //Timer Queue
#define      MEMORY_INTERLOCK_MESSAGE_TABLE   MEMORY_INTERLOCK_TIMER_QUEUE+1      //Message Table
#define      MEMORY_INTERLOCK_TIMER           MEMORY_INTERLOCK_MESSAGE_TABLE+1    //Timer
#define      MEMORY_INTERLOCK_PRINTER         MEMORY_INTERLOCK_TIMER+1            //Scheduler Printer

//PCB states
#define PCB_STATE_LIVE 0L               //initial PCB state when created
#define PCB_STATE_SUSPEND 1L            //suspended
#define PCB_STATE_MSG_SUSPEND 2L        //message suspended
#define PCB_STATE_TERMINATE 3L          //terminated
#define PCB_STATE_RUNNING 4L            //currently running, only in multiprocessor mode

//PCB location
#define PCB_LOCATION_FLOATING 0L        //initial PCB location, not in ready queue nor timer queue
#define PCB_LOCATION_READY_QUEUE 1L     //PCB in ready queue
#define PCB_LOCATION_TIMER_QUEUE 2L     //PCB in timer queue

//global data structures
struct PCB_Table *pcbTable;               //PCB table
struct Timer_Queue *timerQueue;           //timer queue
struct Ready_Queue *readyQueue;           //ready queue
struct Message_Table *messageTable;       //message table
struct Process_Control_Block *currentPCB; //current running PCB

/*********************PCB Table************************/
//PCB Table is a linded list, PCB is only allowed to
//enter PCB when created, no deletion is allowed

//PCB information
struct Process_Control_Block  {
	long         ContextID;
	char*        ProcessName;
	int          ProcessID;
	long         ProcessState;
	long         ProcessLocation;
	long         WakeUpTime;
	int          Priority;
	long*        TestToRun;
};
//PCB table element
struct PCB_Table_Element {
	struct Process_Control_Block *PCB;
	struct PCB_Table_Element *Next_Element;
};
//PCB table header
struct PCB_Table {
	int Element_Number;
	int Terminated_Number;
	int Suspended_Number;
	int Msg_Suspended_Number;
	int Cur_Running_Number;
	struct PCB_Table_Element *First_Element;
};

void initPCBTable();
void enPCBTable(struct Process_Control_Block *PCB);
int PCBLiveNumber();
void lockPCBTable();
void unlockPCBTable();
/*******************************************************/

/*********************Timer Queue**********************/
//timer queue is a double linked list. when a PCB enters,
//it's automaticly arranged in the order of WakeUpTime

//timer queue element
struct Timer_Queue_Element{
	struct Process_Control_Block *PCB;
	struct Timer_Queue_Element *Prev_Element;
	struct Timer_Queue_Element *Next_Element;
} ;
//timer queue header
struct Timer_Queue{
	int Element_Number;
	struct Timer_Queue_Element *First_Element;
};

void initTimerQueue();
void enTimerQueue(struct Process_Control_Block *PCB);
struct Process_Control_Block *deTimerQueue();
void lockTimerQueue();
void unlockTimerQueue();
/*******************************************************/

/*********************Ready Queue**********************/
//ready queue is a double linked list. when a PCB enters,
//it's automaticly arranged in the order of priority

//ready queue element
struct Ready_Queue_Element{
	struct Process_Control_Block *PCB;
	struct Ready_Queue_Element *Prev_Element;
	struct Ready_Queue_Element *Next_Element;
};
//ready queue header
struct Ready_Queue{
	int Element_Number;
	struct Ready_Queue_Element *First_Element;
};

void initReadyQueue();
void enReadyQueue(struct Process_Control_Block *PCB);
struct Process_Control_Block *deReadyQueue();
struct Process_Control_Block *deCertainPCBFromReadyQueue(int PID);
void lockReadyQueue();
void unlockReadyQueue();
/*******************************************************/

/************************Message************************/
//message queue is a double linked list. it's used to
//store message data. the order of message is arranged
//in the order of entering order. whenever a message is
//received, it's deleted.

//message information
struct Message {
	long Target_PID;
	long Sender_PID;
	char *Message_Buffer;
	long Message_Length;
};
//message table element
struct Message_Table_Element {
	struct Message *Message;
	struct Message_Table_Element *Prev_Element;
	struct Message_Table_Element *Next_Element;
};
//message table header
struct Message_Table {
	long Element_Number;
	struct Message_Table_Element *First_Element;
};

void initMessageTable();
struct Message *CreateMessage(long Target_PID, char *Message_Buffer, long SendLength, long *ErrorReturned);
void enMessageTable(struct Message *Message);
void RemoveMessage(struct Message_Table_Element *MessageToRemove);
int findMessage(long Source_PID, char *ReceiveBuffer, long ReceiveLength,
	long *ActualSendLength, long *ActualSourcePID, long *ErrorReturned_ReceiveMessage);
void lockMessageTable();
void unlockMessageTable();
/*******************************************************/