#include             "global.h"

struct Process_Control_Block  {
	long         ContextID;
	long         ProcessState;
	INT32        ProcessID;
	long         ProgramCounter;
	//registers, memory limits, list of open files
};

//structs for Queue Hub
struct PCB_Hub_Element {
	struct Process_Control_Block *PCB;
	struct PCB_Hub_Element *Next_Element;
};

struct PCB_Hub {
	int Element_Number;
	struct PCB_Hub_Element *First_Element;
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

void initTimerQueue();
void enTimerQueue(struct Process_Control_Block *PCB, long wakeUpTime);
void deTimerQueue();