#include             "global.h"

struct Process_Control_Block  {
	long         ContextID;
	long         ProcessState;
	INT32        ProcessID;
	long         ProgramCounter;
	//registers, memory limits, list of open files
};


//structs for Timer Queue
struct Timer_Queue_Element{
	long WakeUpTime;
	struct Process_Control_Block *PCB;
	struct Timer_Queue_Element *Next_Element;
	struct Timer_Queue_Element *Prev_Element;
} ;

struct Timer_Queue_Header{
	struct Timer_Queue_Element* First_Element;
	int Element_Number;
};

void initTimerQueue();
void enTimerQueue(struct Process_Control_Block *PCB, long wakeUpTime);
void deTimerQueue();