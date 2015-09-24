#include "global.h"
#include "stdio.h"
#include "syscalls.h"

void StartTimer(){
	MEMORY_MAPPED_IO mmio;    //for hardware interface
	//get current context ID
	mmio.Mode = Z502GetCurrentContext;
	mmio.Field1 = mmio.Field2 = mmio.Field3 = mmio.Field4 = 0;
	MEM_READ(Z502Context, &mmio);
	printf("returned Context ID-4: %d\n", mmio.Field1);
}