#include "global.h"
#include "stdio.h"
#include "syscalls.h"
#include "queue.h"

long CurrentTime(){
	MEMORY_MAPPED_IO mmio;    //for hardware interface

	mmio.Mode = Z502ReturnValue;
	mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
	MEM_READ(Z502Clock, &mmio);
	return mmio.Field1;
}

void ResetTimer(){
	MEMORY_MAPPED_IO mmio;    //for hardware interface

	if (timerQueue->Element_Number != 0){
		// Start the timer - here's the sequence to use
		mmio.Mode = Z502Start;
		mmio.Field1 = timerQueue->First_Element->PCB->WakeUpTime - CurrentTime();   // You pick the time units
		mmio.Field2 = mmio.Field3 = 0;
		MEM_WRITE(Z502Timer, &mmio);
	}
}

void OSCreateProcess(long *ProcessName, long *Test_To_Run, long *Priority, long *ProcessID, long *ErrorReturned){
	//check input
	if ((int)Priority < 0){
		*ErrorReturned = ERR_BAD_PARAM;
		return;
	}
	else if (findPCBbyProcessName((char*)ProcessName) != NULL){
		*ErrorReturned = ERR_BAD_PARAM;
		printf("1. Failed to create a process: %s\n", (char*)ProcessName);
		struct Process_Control_Block *tmpPCB = findPCBbyProcessName((char*)ProcessName);
		printf("2. PID: %d\n", tmpPCB->ProcessID);
		printf("3. PName: %s\n", tmpPCB->ProcessName);
		return;
	}
	else{
		*ErrorReturned = ERR_SUCCESS;
	}

	void *PageTable = (void *)calloc(2, VIRTUAL_MEM_PAGES);
	MEMORY_MAPPED_IO mmio;

	struct Process_Control_Block *newPCB = (struct Process_Control_Block*)malloc(sizeof(struct Process_Control_Block));

	mmio.Mode = Z502InitializeContext;
	mmio.Field1 = 0;
	mmio.Field2 = (long)Test_To_Run;//test 1a
	mmio.Field3 = (long)PageTable;
	MEM_WRITE(Z502Context, &mmio);   // Initialize Context

	newPCB->ContextID = mmio.Field1;
	newPCB->Priority = (int)Priority;
	newPCB->ProcessID = pcbTable->Element_Number + 1;
	char* newProcessName = (char*)calloc(sizeof(char),16);
	strcpy(newProcessName, (char*)ProcessName);//
	newPCB->ProcessName = newProcessName;
	newPCB->ProcessState = PCB_STATE_LIVE;
	*ProcessID = newPCB->ProcessID;

	printf("4. Successfully create a process: %s\n", (char*)ProcessName);
	printf("5. Successfully create a PID: %d\n", newPCB->ProcessID);

	//put new PCB into PCB Table and Ready Queue
	enPCBTable(newPCB);
	enReadyQueue(newPCB);
}

void OSStartProcess(struct Process_Control_Block* PCB){
	MEMORY_MAPPED_IO mmio;

	currentPCB = PCB;//set current PCB

	mmio.Mode = Z502StartContext;
	mmio.Field1 = PCB->ContextID;
	mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
	MEM_WRITE(Z502Context, &mmio);     // Start up the context
}

void Dispatcher(){
	OSStartProcess(readyQueue->First_Element->PCB);
	deReadyQueue();
}

/*
void StartTimer(SleepTime){
MEMORY_MAPPED_IO mmio;    //for hardware interface
struct Process_Control_Block * currentPCB = pcbTable->First_Element->PCB;//get current PCB

//get current time
long currentTime;
mmio.Mode = Z502ReturnValue;
mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
MEM_READ(Z502Clock, &mmio);
currentTime = mmio.Field1;

//put current PCB into timer Queue
long wakeUpTime = SleepTime + currentTime;
enTimerQueue(currentPCB, wakeUpTime);

// Start the timer - here's the sequence to use
mmio.Mode = Z502Start;
mmio.Field1 = SleepTime;   // You pick the time units
mmio.Field2 = mmio.Field3 = 0;
MEM_WRITE(Z502Timer, &mmio);

// Go idle until the interrupt occurs
mmio.Mode = Z502Action;
mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
MEM_WRITE(Z502Idle, &mmio);       //  Let the interrupt for this timer occur
DoSleep(10);                       // Give it a little more time
}
*/