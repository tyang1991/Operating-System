#include             "global.h"

typedef struct  {
	long         Context;
	long         ProcessState;
	INT32        ProcessID;
	long         ProgramCounter;
	//registers, memory limits, list of open files

} Process_Control_Block;

//structs for Queue Hub
typedef struct {
	struct Queue_Hub_Element* Next_Element;
} Queue_Hub_Element;

typedef struct {
	Queue_Hub_Element* First_Element;
} Queue_Hub_Header;

//structs for Timer Queue
typedef struct {
	long WakeUpTime;
	struct Timer_Queue_Element* Next_Element;
} Timer_Queue_Element;

typedef struct {
	Timer_Queue_Element* First_Element;
} Timer_Queue_Header;

//structs for Ready Queue
typedef struct {
	long Priority;
	struct Ready_Queue_Element* Next_Element;
} Ready_Queue_Element;

typedef struct {
	Ready_Queue_Element* First_Element;
} Ready_Queue_Header;
