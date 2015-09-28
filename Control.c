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

void OSCreateProcess(long *Test_To_Run){

	void *PageTable = (void *)calloc(2, VIRTUAL_MEM_PAGES);
	MEMORY_MAPPED_IO mmio;

	struct Process_Control_Block *newPCB = (struct Process_Control_Block*)malloc(sizeof(struct Process_Control_Block));

	mmio.Mode = Z502InitializeContext;
	mmio.Field1 = 0;
	mmio.Field2 = (long)Test_To_Run;//test 1a
	mmio.Field3 = (long)PageTable;
	MEM_WRITE(Z502Context, &mmio);   // Initialize Context

	newPCB->ContextID = mmio.Field1;
	enPCBTable(newPCB);
}

void OSStartProcess(struct Process_Control_Block* PCB){
	MEMORY_MAPPED_IO mmio;

	currentPCB = PCB;//set current PCB

	mmio.Mode = Z502StartContext;
	mmio.Field1 = PCB->ContextID;
	mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
	MEM_WRITE(Z502Context, &mmio);     // Start up the context
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