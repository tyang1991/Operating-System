#include             "global.h"

#define PCB_STATE_LIVE 0L
#define PCB_STATE_SUSPEND 1L
#define PCB_STATE_TERMINATE 2L
#define PCB_LOCATION_FLOATING 0L
#define PCB_LOCATION_READY_QUEUE 1L
#define PCB_LOCATION_TIMER_QUEUE 2L

struct Timer_Queue *timerQueue;
struct Process_Control_Block *currentPCB;
struct PCB_Table *pcbTable;
struct Ready_Queue *readyQueue;
struct Message_Table *messageTable;

/*********************PCB Table************************/
struct Process_Control_Block  {
	long         ContextID;
	char*        ProcessName;
	int          ProcessID;
	long         ProcessState;
	long         ProcessLocation;
	long         WakeUpTime;
	int          Priority;
};
struct PCB_Table_Element {
	struct Process_Control_Block *PCB;
	struct PCB_Table_Element *Next_Element;
};
struct PCB_Table {
	int Element_Number;
	int Terminated_Number;
	int Suspended_Number;
	struct PCB_Table_Element *First_Element;
};

void initPCBTable();
void enPCBTable(struct Process_Control_Block *PCB);
int PCBLiveNumber();
void lockPCBTable();
void unlockPCBTable();
/*******************************************************/

/*********************Timer Queue**********************/
struct Timer_Queue_Element{
	struct Process_Control_Block *PCB;
	struct Timer_Queue_Element *Prev_Element;
	struct Timer_Queue_Element *Next_Element;
} ;
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
struct Ready_Queue_Element{
	struct Process_Control_Block *PCB;
	struct Ready_Queue_Element *Prev_Element;
	struct Ready_Queue_Element *Next_Element;
};
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
struct Message {
	long Target_PID;
	long Sender_PID;
	char *Message_Buffer;
	long Message_Length;
};

struct Message_Table_Element {
	struct Message *Message;
	struct Message_Table_Element *Next_Element;
};

struct Message_Table {
	long Element_Number;
	struct Message_Table_Element *First_Element;
};

void initMessageTable();
struct Message *CreateMessage(long Target_PID, long Sender_PID, char *Message_Buffer, long SendLength, long *ErrorReturned);
void enMessageTable(struct Message *Message);
void lockMessageTable();
void unlockMessageTable();
/*******************************************************/