#include             "global.h"

#define PCB_STATE_LIVE 0L
#define PCB_STATE_DEAD 1L

struct Process_Control_Block  {
	long         ContextID;
	char*        ProcessName;
	int          ProcessID;
	long         ProcessState;
	long         WakeUpTime;
	int          Priority;
};

//structs for PCB Table
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
	struct Process_Control_Block *PCB;
	struct Timer_Queue_Element *Prev_Element;
	struct Timer_Queue_Element *Next_Element;
} ;

struct Timer_Queue{
	int Element_Number;
	struct Timer_Queue_Element *First_Element;
};

struct Ready_Queue_Element{
	struct Process_Control_Block *PCB;
	struct Ready_Queue_Element *Prev_Element;
	struct Ready_Queue_Element *Next_Element;
};

struct Ready_Queue{
	int Element_Number;
	struct Ready_Queue_Element *First_Element;
};

struct Timer_Queue *timerQueue;
struct Process_Control_Block *currentPCB;
struct PCB_Table *pcbTable;
struct Ready_Queue *readyQueue;

//timer queue functions
void initTimerQueue();
void enTimerQueue(struct Process_Control_Block *PCB);
void deTimerQueue();

//PCB table functions
void initPCBTable();
void enPCBTable(struct Process_Control_Block *PCB);
struct Process_Control_Block *findPCBbyProcessID(long ProcessID);
int findProcessNameInTable(char* ProcessName);
struct Process_Control_Block *findPCBbyProcessName(char* ProcessName);
int findPCBIDbyName(char* ProcessName);

//ready queue function
void initReadyQueue();
void enReadyQueue(struct Process_Control_Block *PCB);
void deReadyQueue();