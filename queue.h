#include             "global.h"

struct Process_Control_Block  {
	long         ContextID;
	INT32        ProcessID;
	long         ProcessState;
};

//structs for Queue Hub
struct PCB_Table_Element {
	struct Process_Control_Block *PCB;
	struct PCB_Table_Element *Next_Element;
};

struct PCB_Table {
	int Element_Number;
	struct PCB_Table_Element *First_Element;
};

//structs for Timer Queue
struct Timer_Queue_Element{
	long WakeUpTime;
	struct Process_Control_Block *PCB;
	struct Timer_Queue_Element *Prev_Element;
	struct Timer_Queue_Element *Next_Element;
} ;

struct Timer_Queue{
	int Element_Number;
	struct Timer_Queue_Element *First_Element;
};

//timer queue functions
void initTimerQueue();
void enTimerQueue(struct Process_Control_Block *PCB, long wakeUpTime);
void deTimerQueue();

//page table functions
void initPCBTable();
void enPCBTableb(struct Process_Control_Block *PCB);