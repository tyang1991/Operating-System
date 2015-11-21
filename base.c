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
#include             "DataStructure.h"
#include             "Control.h"
#include             "Utility.h"
#include             "MyTest.h"

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
	struct Process_Control_Block *tmpPCB;
	long DiskID;

	MEMORY_MAPPED_IO mmio;       // Enables communication with hardware

	static BOOL remove_this_in_your_code = TRUE; /** TEMP **/
	static INT32 how_many_interrupt_entries = 0; /** TEMP **/

	// Get cause of interrupt
	mmio.Mode = Z502GetInterruptInfo;
	mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
	MEM_READ(Z502InterruptDevice, &mmio);
	DeviceID = mmio.Field1;
	//Status = mmio.Field2;

/////////////////////////Code go here////////////////////////////////////
	if (DeviceID == TIMER_INTERRUPT) {
		//take all timeout PCB from timer queue and put into ready queue
		do {
			tmpPCB = deTimerQueue();
			enReadyQueue(tmpPCB);
		} while (ResetTimer() == 0);
	}
	else if (DeviceID >= DISK_INTERRUPT && DeviceID <= DISK_INTERRUPT+7){
		//Disk ID from 1 to 8, disk interrupt from 5+0 to 5+7
		DiskID = DeviceID - DISK_INTERRUPT + 1;
//		printf("DISK_INTERRUPT: DiskID %d\n", DiskID);
		tmpPCB = deDiskQueue(DiskID);
		enReadyQueue(tmpPCB);
		if (diskQueue->Disk_Number[DiskID] != NULL) {
			StartDiskOp(diskQueue->Disk_Number[DiskID]->Disk_Op);
		}

	}
	else {
		printf("Error: INTERRUPT not recognized\n");
	}

/////////////////////////Code end here///////////////////////////////////
	
	ClearInterruptStatus(DeviceID);
}           // End of InterruptHandler

/************************************************************************
 FAULT_HANDLER
 The beginning of the OS502.  Used to receive hardware faults.
 ************************************************************************/

void FaultHandler(void) {
	INT32 DeviceID;
	INT32 Status;
	char DataReadFromMemory[PGSIZE];
	INT32 freeDisk;
	INT32 freeDiskID;
	INT32 freeDiskSector;
	INT16 *victimShadowPageTable;
	INT16 victimShadowPage;


	MEMORY_MAPPED_IO mmio;       // Enables communication with hardware

	// Get cause of interrupt
	mmio.Mode = Z502GetInterruptInfo;
	MEM_READ(Z502InterruptDevice, &mmio);
	DeviceID = mmio.Field1;
	Status = mmio.Field2;

	printf("Fault_handler: Found vector type %d with value %d\n", DeviceID,
			Status);
	/*****************************My Code******************************/
	if (Status < 0 || Status >= 1024) {
		HaltProcess();
	}

	//if Sbit = 1
	//    get victim frame when (Vbit == 1, Rbit == 0)
	//    if Sbit == 0
	//        Z502ReadPhysicalMemory
	//        get free DiskID & Sector
	//        DISK_WRITE
	//        write victim's ShadowPageTable
	//        Sbit = 1
	//    else
	//        Z502ReadPhysicalMemory
	//        get DiskID & Sector from ShadowPageTable
	//        DISK_WRITE
	//    Vbit = 0
	//    change frame table map to NULL
	//    change victim's pageTable (Vbit = 0, Rbit = 0, pageNumber = 0)

	//    get DiskID & Sector from ShadowPageTable
	//    DISK_READ
	//    Z502WritePhysicalMemory
	//    Vbit = 1, Rbit = 1

	//write frameTable(PID, pageNumber)
	//write pageTable(frame number, Vbit = 1, Rbit = 1)

	/*********************************************************************/
	struct Process_Control_Block *currentPCB = CurrentPCB();
	INT16 *currentPageTable = (INT16 *)currentPCB->PageTable;
	
	INT16 Sbit_Current = (currentPageTable[Status] | 0x1000) / 4096; //get Sbit
	if (Sbit_Current == 1) {
		//get victim frame
		struct Frame_Map *victimFrame = GetVictimFrame();
		//get victim page
		INT16 *victimPageTable = (INT16 *)victimFrame->PCB->PageTable;
		INT16 victimPage = victimPageTable[victimFrame->pageNumber];
		//make a copy into disk
		int Sbit_Victim = (victimPage | 0x1000) / 4096;
		if (Sbit_Victim == 0) {
			//read from physical memory
			Z502WritePhysicalMemory(victimFrame->frameNumber, (char *)DataReadFromMemory);
			//get free DiskID & Sector
			freeDisk = GetFreeDiskAddress(CurrentPID(), Status);
			freeDiskID = freeDisk / 4096;
			freeDiskSector = freeDisk | 0x0FFF;
			//DISK_WRITE
			DISK_WRITE(freeDiskID, freeDiskSector, (char *)DataReadFromMemory);
			//write victim's ShadowPageTable
			victimShadowPageTable = (INT16*)victimFrame->PCB->ShadowPageTable;
			victimShadowPage = victimShadowPageTable[victimFrame->pageNumber];

		}
	}


	/*********************************************************************/
/*
	struct Process_Control_Block *currentPCB = CurrentPCB();
	INT16 *currentPageTable = (INT16 *)currentPCB->PageTable;
	currentPageTable[Status] = currentPageTable[Status] + NewFrameNumber();
	//write frame table ***
	currentPageTable[Status] = currentPageTable[Status] | 0x8000;
*/
	/******************************************************************/

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

	//for hardware interface
	MEMORY_MAPPED_IO mmio;
	//for GET_TIME_OF_DAY
	INT32 Temp_Clock;
	//for SLEEP
	long Sleep_Time;
	struct Process_Control_Block *sleepPCB;
	//for RESTART_PROCESS
	long PID_restart;
	struct Process_Control_Block *restartPCB;
	struct Process_Control_Block *recreatedPCB;
	//for CREATE_PROCESS
	struct Process_Control_Block *newPCB;
	//for TERMINATE_PROCESS
	long termPID;
	struct Process_Control_Block *termPCB;
	//for GET_PROCESS_ID
	int ReturnedPID;
	char* ProcessName;
	struct Process_Control_Block *PCBbyProcessName;
	//for SUSPEND_PROCESS
	int suspendPID;
	struct Process_Control_Block *suspendPCB;
	//for RESUME_PROCESS
	int resumePID;
	struct Process_Control_Block *resumePCB;
	//for CHANGE_PRIORITY
	int changePrioPID;
	struct Process_Control_Block *changePrioPCB;
	int newPriority;
	//for SEND_MESSAGE
	long TargetPID;
	char *MessageBuffer;
	long SendLength;
	struct Message *MessageCreated;
	long *ErrorReturned_SendMessage;
	//for RECEIVE_MESSAGE
	long SourcePID;
	char *ReceiveBuffer;
	long ReceiveLength;
	long *ActualSendLength;
	long *ActualSourcePID;
	long *ErrorReturned_ReceiveMessage;
	struct Process_Control_Block *Mess_PCB;
	//for DISK_WRITTEN & DISK_READ
	long DiskID;
	long Sector;
	char *DataWritten;
	char *DataRead;
	struct DISK_OP *newDiskOp;

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
		//get and return current system time
		case SYSNUM_GET_TIME_OF_DAY:
			mmio.Mode = Z502ReturnValue;
			mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
			MEM_READ(Z502Clock, &mmio);
			Temp_Clock = mmio.Field1;
			*SystemCallData->Argument[0] = Temp_Clock;
			break;
		//create a new PCB and put it into pcb table and ready queue
		case SYSNUM_CREATE_PROCESS:
			//create a new PCB
			newPCB = OSCreateProcess(SystemCallData->Argument[0], SystemCallData->Argument[1],
							SystemCallData->Argument[2], SystemCallData->Argument[3], 
							SystemCallData->Argument[4]);
			//if create successfully, put it into PCB table and ready queue
			if (newPCB != NULL) {
				SchedularPrinter("Create", newPCB->ProcessID);//print states
				enPCBTable(newPCB);
				enReadyQueue(newPCB);
			}
			break;
		//return PID regarding process name
		case SYSNUM_GET_PROCESS_ID:
			ProcessName = (char*)SystemCallData->Argument[0];
			//if no input process name, return the current running PID
			if (strcmp(ProcessName, "") == 0) {
				PCBbyProcessName = CurrentPCB();
				*SystemCallData->Argument[1] = PCBbyProcessName->ProcessID;
				*SystemCallData->Argument[2] = ERR_SUCCESS;
			}
			//find the PCB in PCB table and return PID if found
			else {
				PCBbyProcessName = findPCBbyProcessName(ProcessName);
				//if found, return PID
				if (PCBbyProcessName != NULL) {
					ReturnedPID = PCBbyProcessName->ProcessID;
					*SystemCallData->Argument[1] = ReturnedPID;
					*SystemCallData->Argument[2] = ERR_SUCCESS;
				}
				//if not found, return error
				else {
					*SystemCallData->Argument[2] = ERR_BAD_PARAM;
				}
			}
			break;
		//if a PCB wanna sleep, put itself into timer queue and start
		//a new PCB
		case SYSNUM_SLEEP:
			//print states
			SchedularPrinter("Sleep", CurrentPID());
			//Calculate WakeUpTime for PCB
			Sleep_Time = (long)SystemCallData->Argument[0];
			sleepPCB = CurrentPCB();
			sleepPCB->WakeUpTime = CurrentTime() + Sleep_Time;
			//Put current running PCB into timer queue and reset time 
			lockTimer();
			enTimerQueue(sleepPCB);
			if (sleepPCB == timerQueue->First_Element->PCB){
				SetTimer(Sleep_Time);
			}
			unlockTimer();
			//in uniprocessor, start a new PCB
			//in multiprocessor, only suspend itself
			if (ProcessorMode == Uniprocessor) {
				//first PCB in Ready Queue starts
				Dispatcher();
			}
			else {
				OSSuspendCurrentProcess();
			}
			break;
			//restart a PCB by terminate itself and created a new PCB with everything the same except PID
		case SYSNUM_RESTART_PROCESS:
			//initial return error
			*SystemCallData->Argument[2] = ERR_BAD_PARAM;
			PID_restart = (long)SystemCallData->Argument[0];
			//if not restart itself
			if (PID_restart != CurrentPID()){
				//find restarted PCB
				restartPCB = findPCBbyProcessID(PID_restart);
				//if PCB found, terminate itself and create a new one
				if (restartPCB != NULL){
					TerminateProcess(restartPCB);
					recreatedPCB = OSCreateProcess((long*)restartPCB->ProcessName, restartPCB->TestToRun,
						(long*)restartPCB->Priority, SystemCallData->Argument[1], SystemCallData->Argument[2]);
					//if create successfully, put it into PCB table and ready queue
					if (recreatedPCB != NULL) {
						*SystemCallData->Argument[2] = ERR_SUCCESS;
						SchedularPrinter("Create", recreatedPCB->ProcessID);//print states
						enPCBTable(recreatedPCB);
						enReadyQueue(recreatedPCB);
						SchedularPrinter("Restart", restartPCB->ProcessID);
					}
				}
				else{
					*SystemCallData->Argument[2] = ERR_BAD_PARAM;
				}
			}
			else{
				*SystemCallData->Argument[2] = ERR_BAD_PARAM;
			}
			break;
			//terminate a process
		case SYSNUM_TERMINATE_PROCESS:
			termPID = (long)SystemCallData->Argument[0];
			//if PID = -1, terminate current running PCB
			if (termPID == -1) {
				if (PCBLiveNumber() > 1) {
					*SystemCallData->Argument[1] = ERR_SUCCESS;
					//print states
					SchedularPrinter("Terminate", termPID);
					//terminate current PCB
					TerminateProcess(CurrentPCB());
				}
				else {
					*SystemCallData->Argument[1] = ERR_SUCCESS;
					HaltProcess();
				}
			}
			//if PID = -2, terminate OS
			if (termPID == -2) {
				*SystemCallData->Argument[1] = ERR_SUCCESS;
				HaltProcess();
			}
			//if PID positive, terminate specified PID
			else {
				termPCB = findPCBbyProcessID((long)SystemCallData->Argument[0]);
				//if PCB found, terminate it
				if (termPCB != NULL) {
					*SystemCallData->Argument[1] = ERR_SUCCESS;
					//if more than one PCB alive, simply terminate it
					if (PCBLiveNumber() > 1) {
						//print states
						SchedularPrinter("Terminate", termPID);
						//terminate specified PCB
						TerminateProcess(termPCB);
					}
					//if last alive PCB, terminate OS
					else {
						HaltProcess();
					}
				}
				else {
					*SystemCallData->Argument[1] = ERR_BAD_PARAM;
				}
			}
			break;
		//suspend a PCB, which can be resumed
		case SYSNUM_SUSPEND_PROCESS:
			suspendPID = (int)SystemCallData->Argument[0];
			//if PID = -1, suspend current running PCB
			if (suspendPID == -1) {
				//if more than one PCB alive, suspend it
				if (PCBLiveNumber() > 1) {
					*SystemCallData->Argument[1] = ERR_SUCCESS;
					//print states
					SchedularPrinter("Suspend", suspendPID);
					//Suspend Current Process
					SuspendProcess(CurrentPCB());
				}
				//if last one PCB alive, return error
				else {
					*SystemCallData->Argument[1] = ERR_BAD_PARAM;
				}
			}
			//if PID positive, suspend specified PCB
			else {
				suspendPCB = findPCBbyProcessID((int)suspendPID);
				//if PCB found
				if (suspendPCB != NULL) {
					//if more than one PCB alive, suspend it
					if (suspendPCB->ProcessState == PCB_STATE_LIVE && PCBLiveNumber() > 1) {
						*SystemCallData->Argument[1] = ERR_SUCCESS;
						//print states
						SchedularPrinter("Suspend", suspendPID);
						//Suspend specified process
						SuspendProcess(suspendPCB);
					}
					//if last one PCB alive, return error
					else {
						*SystemCallData->Argument[1] = ERR_BAD_PARAM;
					}
				}
				else {
					*SystemCallData->Argument[1] = ERR_BAD_PARAM;
				}
			}
			break;
		//resumes a previously suspended PCB
		case SYSNUM_RESUME_PROCESS:
			resumePID = (int)SystemCallData->Argument[0];
			resumePCB = findPCBbyProcessID(resumePID);
			//if PCB found
			if (resumePCB != NULL) {
				//if PCB is previously suspended
				if (resumePCB->ProcessState == PCB_STATE_SUSPEND) {
					*SystemCallData->Argument[1] = ERR_SUCCESS;
					//print states
					SchedularPrinter("Resume", resumePID);
					//Resume specified process
					ResumeProcess(resumePCB);
				}
				else {
					*SystemCallData->Argument[1] = ERR_BAD_PARAM;
				}
			}
			else {
				*SystemCallData->Argument[1] = ERR_BAD_PARAM;
			}
			break;
		//change the priority of a PCB
		case SYSNUM_CHANGE_PRIORITY:
			changePrioPID = (int)SystemCallData->Argument[0];
			changePrioPCB = findPCBbyProcessID((int)changePrioPID);
			newPriority = (int)SystemCallData->Argument[1];
			//if legal priority
			if (newPriority<=40 && newPriority>=0) {
				if (changePrioPCB != NULL) {
					*SystemCallData->Argument[2] = ERR_SUCCESS;
					//print states
					printf("Before changing Priority\n");
					SchedularPrinter("ChangePrio", changePrioPID);
					//if PCB in ready queue, change order in ready queue
					if (changePrioPCB->ProcessLocation == PCB_LOCATION_READY_QUEUE
										&& newPriority != changePrioPCB->Priority) {
						changePrioPCB = deCertainPCBFromReadyQueue(changePrioPID);
						changePrioPCB->Priority = newPriority;
						enReadyQueue(changePrioPCB);
					}
					else {
						changePrioPCB->Priority = newPriority;
					}
					//print states
					printf("After changing Priority\n");
					SchedularPrinter("ChangePrio", changePrioPID);
				}
				else {
					*SystemCallData->Argument[2] = ERR_BAD_PARAM;
				}
			}
			else {
				*SystemCallData->Argument[2] = ERR_BAD_PARAM;
			}
			break;
		//PCB stores a message in message table
		case SYSNUM_SEND_MESSAGE:
			TargetPID = (long)SystemCallData->Argument[0];
			MessageBuffer = (char*)SystemCallData->Argument[1];
			SendLength = (long)SystemCallData->Argument[2];
			ErrorReturned_SendMessage = SystemCallData->Argument[3];
			//create a message
			MessageCreated = CreateMessage(TargetPID, MessageBuffer, 
				                        SendLength, ErrorReturned_SendMessage);
			//if successfully create a message, put it into message table
			if (MessageCreated != NULL) {
				SchedularPrinter("SendMsg", CurrentPID());
				enMessageTable(MessageCreated);
			}
			break;
		//retrive a message in message table
		case SYSNUM_RECEIVE_MESSAGE:
			SourcePID = (long)SystemCallData->Argument[0];
			ReceiveBuffer = (char*)SystemCallData->Argument[1];
			ReceiveLength = (long)SystemCallData->Argument[2];
			ActualSendLength = SystemCallData->Argument[3];
			ActualSourcePID = SystemCallData->Argument[4];
			ErrorReturned_ReceiveMessage = SystemCallData->Argument[5];
			Mess_PCB = CurrentPCB();
			Mess_PCB->ProcessState = PCB_STATE_MSG_SUSPEND;
			pcbTable->Msg_Suspended_Number += 1;
			//PCB kept suspended by sleep until find a message
			while (findMessage(SourcePID, ReceiveBuffer, ReceiveLength,
				ActualSendLength, ActualSourcePID, ErrorReturned_ReceiveMessage) == 0) {
				Mess_PCB->WakeUpTime = CurrentTime() + 10;
				//Put current running PCB into timer queue and reset time 
				enTimerQueue(Mess_PCB);
				if (Mess_PCB == timerQueue->First_Element->PCB) {
					SetTimer(10);
				}
				//first PCB in Ready Queue starts
				Dispatcher();
			}

			Mess_PCB->ProcessState = PCB_STATE_LIVE;
			pcbTable->Msg_Suspended_Number -= 1;
			SchedularPrinter("ReceiveMsg", CurrentPID());
			break;
		case SYSNUM_DISK_WRITE:
			DiskID = (long)SystemCallData->Argument[0];
			Sector = (long)SystemCallData->Argument[1];
			DataWritten = (char *)SystemCallData->Argument[2];

			newDiskOp = CreateDiskOp(DISK_OPERATION_WRITE, DiskID, Sector,
				DataWritten, CurrentPCB());
			enDiskQueue(newDiskOp);

			if (DiskStatus(DiskID) != DEVICE_IN_USE) {
				StartDiskOp(newDiskOp);
			}

			Dispatcher();
			break;
		case SYSNUM_DISK_READ:
			DiskID = (long)SystemCallData->Argument[0];
			Sector = (long)SystemCallData->Argument[1];
			DataRead = (char *)SystemCallData->Argument[2];

			newDiskOp = CreateDiskOp(DISK_OPERATION_READ, DiskID, Sector,
				DataRead, CurrentPCB());
			enDiskQueue(newDiskOp);

			if (DiskStatus(DiskID) != DEVICE_IN_USE) {
				StartDiskOp(newDiskOp);
			}

			Dispatcher();
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
	initPCBTable();
	initTimerQueue();
	initReadyQueue();
	initMessageTable();
	initDiskQueue();
	currentPCB = (struct Process_Control_Block*)malloc(sizeof(struct Process_Control_Block));
	//init memory
	initMemory();
	initFrameMapTable();

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

	/****************************Parse Input******************************/
	long *TestToRun;
	switch (argc){
		case 2:
			TestToRun = TestParser(argv[1]);
			ProcessorMode = Uniprocessor;
			break;
		case 3:
			TestToRun = TestParser(argv[1]);
			ProcessorMode = Multiprocessor;
			break;
		default:
			TestToRun = (long*)test1c;
			ProcessorMode = Uniprocessor;
			break;
	}
	/********************************************************************/

	long ErrorReturned;
	long newPID;
	struct Process_Control_Block *newPCB = OSCreateProcess((long*)"test1", TestToRun, (long*)3, (long*)&newPID, (long*)&ErrorReturned);
	if (newPCB != NULL) {
		enPCBTable(newPCB);
		enReadyQueue(newPCB);
	}
	Dispatcher();
}                                               // End of osInit