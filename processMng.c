#include "global.h"
#include "stdio.h"
#include "syscalls.h"
#include "queue.h"

void StartTimer(SleepTime){
	MEMORY_MAPPED_IO mmio;    //for hardware interface
	struct Process_Control_Block * currentPCB = pcbTable->First_Element->PCB;//get current PCB
	
	//get current time
	long currentTime;
	mmio.Mode = Z502ReturnValue;
	mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
	MEM_READ(Z502Clock, &mmio);
	currentTime = mmio.Field1;

//	GET_TIME_OF_DAY(&currentTime);

	//put current PCB into timer Queue
	printf("currentTime:%d\n", currentTime);
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