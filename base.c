/************************************************************************

 This code forms the base of the operating system you will
 build.  It has only the barest rudiments of what you will
 eventually construct; yet it contains the interfaces that
 allow test.c and z502.c to be successfully built together.

 Revision History:
 1.0 August 1990
 1.1 December 1990: Portability attempted.
 1.3 July     1992: More Portability enhancements.
 Add call to SampleCode.
 1.4 December 1992: Limit (temporarily) printout in
 interrupt handler.  More portability.
 2.0 January  2000: A number of small changes.
 2.1 May      2001: Bug fixes and clear STAT_VECTOR
 2.2 July     2002: Make code appropriate for undergrads.
 Default program start is in test0.
 3.0 August   2004: Modified to support memory mapped IO
 3.1 August   2004: hardware interrupt runs on separate thread
 3.11 August  2004: Support for OS level locking
 4.0  July    2013: Major portions rewritten to support multiple threads
 4.20 Jan     2015: Thread safe code - prepare for multiprocessors
 ************************************************************************/

#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"
#include             <stdlib.h>
#include             "queue.h"
#include             "Control.h"

//  Allows the OS and the hardware to agree on where faults occur
extern void *TO_VECTOR[];

char *call_names[] = { "mem_read ", "mem_write", "read_mod ", "get_time ",
		"sleep    ", "get_pid  ", "create   ", "term_proc", "suspend  ",
		"resume   ", "ch_prior ", "send     ", "receive  ", "disk_read",
		"disk_wrt ", "def_sh_ar" };

/************************************************************************
 INTERRUPT_HANDLER
 When the Z502 gets a hardware interrupt, it transfers control to
 this routine in the OS.
 ************************************************************************/
void InterruptHandler(void) {
	INT32 DeviceID;
//	INT32 Status;

	MEMORY_MAPPED_IO mmio;       // Enables communication with hardware

	static BOOL remove_this_in_your_code = TRUE; /** TEMP **/
	static INT32 how_many_interrupt_entries = 0; /** TEMP **/

	// Get cause of interrupt
	mmio.Mode = Z502GetInterruptInfo;
	mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
	MEM_READ(Z502InterruptDevice, &mmio);
	DeviceID = mmio.Field1;
	//Status = mmio.Field2;

	printf("here is in the interrupt handler\n");
	deTimerQueue();
	printf("Element in timer Q: %d\n", timerQueue->Element_Number);
	
	// Clear out this device - we're done with it
	mmio.Mode = Z502ClearInterruptStatus;
	mmio.Field1 = DeviceID;
	mmio.Field2 = mmio.Field3 = 0;
	MEM_WRITE(Z502InterruptDevice, &mmio);
}           // End of InterruptHandler

/************************************************************************
 FAULT_HANDLER
 The beginning of the OS502.  Used to receive hardware faults.
 ************************************************************************/

void FaultHandler(void) {
	INT32 DeviceID;
	INT32 Status;

	MEMORY_MAPPED_IO mmio;       // Enables communication with hardware

	// Get cause of interrupt
	mmio.Mode = Z502GetInterruptInfo;
	MEM_READ(Z502InterruptDevice, &mmio);
	DeviceID = mmio.Field1;
	Status = mmio.Field2;

	printf("Fault_handler: Found vector type %d with value %d\n", DeviceID,
			Status);

	// Clear out this device - we're done with it
	mmio.Mode = Z502ClearInterruptStatus;
	mmio.Field1 = DeviceID;
	MEM_WRITE(Z502InterruptDevice, &mmio);
} // End of FaultHandler

/************************************************************************
 SVC
 The beginning of the OS502.  Used to receive software interrupts.
 All system calls come to this point in the code and are to be
 handled by the student written code here.
 The variable do_print is designed to print out the data for the
 incoming calls, but does so only for the first ten calls.  This
 allows the user to see what's happening, but doesn't overwhelm
 with the amount of data.
 ************************************************************************/

void svc(SYSTEM_CALL_DATA *SystemCallData) {
	short call_type;
	static short do_print = 10;
	short i;

	MEMORY_MAPPED_IO mmio;    //for hardware interface
	INT32 Temp_Clock;         //for SYSNUM_GET_TIME_OF_DAY
	long Sleep_Time;          //for SYSNUM_SLEEP
	long returnedContextID;   //for SYSNUM_SLEEP
	struct Process_Control_Block *termPCB;//for SYSNUM_TERMINATE_PROCESS
	struct Process_Control_Block *returnedPCB;

	call_type = (short) SystemCallData->SystemCallNumber;
	if (do_print > 0) {
		printf("SVC handler: %s\n", call_names[call_type]);
		for (i = 0; i < SystemCallData->NumberOfArguments - 1; i++) {
			//Value = (long)*SystemCallData->Argument[i];
			printf("Arg %d: Contents = (Decimal) %8ld,  (Hex) %8lX\n", i,
					(unsigned long) SystemCallData->Argument[i],
					(unsigned long) SystemCallData->Argument[i]);
		}
		do_print--;
	}
	switch (call_type) {
			// Get time service call
		case SYSNUM_GET_TIME_OF_DAY:   // This value is found in syscalls.h
			mmio.Mode = Z502ReturnValue;
			mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
			MEM_READ(Z502Clock, &mmio);
			Temp_Clock = mmio.Field1;
			*SystemCallData->Argument[0] = Temp_Clock;
			break;
		case SYSNUM_SLEEP:
			//Calculate WakeUpTime for PCB
			Sleep_Time = SystemCallData->Argument[0];
			currentPCB->WakeUpTime = CurrentTime() + Sleep_Time;

			enTimerQueue(currentPCB);
			ResetTimer();

			// Go idle until the interrupt occurs
			mmio.Mode = Z502Action;
			mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
			MEM_WRITE(Z502Idle, &mmio);       //  Let the interrupt for this timer occur
			DoSleep(10);                       // Give it a little more time
			break;
		case SYSNUM_CREATE_PROCESS:
			OSCreateProcess(SystemCallData->Argument[0], SystemCallData->Argument[1],
							SystemCallData->Argument[2], SystemCallData->Argument[3], 
							SystemCallData->Argument[4]);
			break;
		case SYSNUM_TERMINATE_PROCESS:
			termPCB = findPCBbyProcessID((long)SystemCallData->Argument[0]);
			//if PCB found, return SUCCESS; otherwise, return BAD
			if (termPCB != NULL){
				termPCB->ProcessState = PCB_STATE_DEAD;
				*SystemCallData->Argument[1] = ERR_SUCCESS;
			}
			else{
				*SystemCallData->Argument[1] = ERR_BAD_PARAM;
			}
			//if PCB running, call Dispatcher
			if (currentPCB == termPCB){
				Dispatcher();
			}

			break;
		case SYSNUM_GET_PROCESS_ID:
			break;


		default:
			printf("ERROR!  call_type not recognized!\n");
			printf("Call_type is - %i\n", call_type);
	}
}                                               // End of svc

/************************************************************************
 osInit
 This is the first routine called after the simulation begins.  This
 is equivalent to boot code.  All the initial OS components can be
 defined and initialized here.
 ************************************************************************/

void osInit(int argc, char *argv[]) {
	void *PageTable = (void *) calloc(2, VIRTUAL_MEM_PAGES);
	INT32 i;
	MEMORY_MAPPED_IO mmio;

	//init Queues
	currentPCB = (struct Process_Control_Block*)malloc(sizeof(struct Process_Control_Block));
	initPCBTable();
	initTimerQueue();
	initReadyQueue();

  // Demonstrates how calling arguments are passed thru to here       

    printf( "Program called with %d arguments:", argc );
    for ( i = 0; i < argc; i++ )
        printf( " %s", argv[i] );
    printf( "\n" );
    printf( "Calling with argument 'sample' executes the sample program.\n" );
    // Here we check if a second argument is present on the command line.
    // If so, run in multiprocessor mode
    if ( argc > 2 ){
    	printf("Simulation is running as a MultProcessor\n\n");
		mmio.Mode = Z502SetProcessorNumber;
		mmio.Field1 = MAX_NUMBER_OF_PROCESSORS;
		mmio.Field2 = (long) 0;
		mmio.Field3 = (long) 0;
		mmio.Field4 = (long) 0;

		MEM_WRITE(Z502Processor, &mmio);   // Set the number of processors
    }
    else {
    	printf("Simulation is running as a UniProcessor\n");
    	printf("Add an 'M' to the command line to invoke multiprocessor operation.\n\n");
    }

	//          Setup so handlers will come to code in base.c   

	TO_VECTOR[TO_VECTOR_INT_HANDLER_ADDR ] = (void *) InterruptHandler;
	TO_VECTOR[TO_VECTOR_FAULT_HANDLER_ADDR ] = (void *) FaultHandler;
	TO_VECTOR[TO_VECTOR_TRAP_HANDLER_ADDR ] = (void *) svc;

	//  Determine if the switch was set, and if so go to demo routine. 

	PageTable = (void *) calloc(2, VIRTUAL_MEM_PAGES);
  	if ((argc > 1) && (strcmp(argv[1], "sample") == 0)) {
		mmio.Mode = Z502InitializeContext;
		mmio.Field1 = 0;
		mmio.Field2 = (long) SampleCode;
		mmio.Field3 = (long) PageTable;

		MEM_WRITE(Z502Context, &mmio);   // Start of Make Context Sequence
		mmio.Mode = Z502StartContext;
		// Field1 contains the value of the context returned in the last call
		mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
		MEM_WRITE(Z502Context, &mmio);     // Start up the context

 	} // End of handler for sample code - This routine should never return here

//	long* sss = {(long*) "test1a_a" };
//	printf("1.%s\n", (char*)sss);

	long ErrorReturned;
	long newPID;
	OSCreateProcess((long*)"test1bb", (long*)test1b, (long*)3, (long*)&newPID, (long*)&ErrorReturned);
	OSStartProcess(pcbTable->First_Element->PCB);
}                                               // End of osInit