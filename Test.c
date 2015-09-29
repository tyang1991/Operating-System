/************************************************************************

 test.c

 These programs are designed to test the OS502 functionality

 Read Appendix B about test programs and Appendix C concerning
 system calls when attempting to understand these programs.

 Revision History:
 1.0 August 1990
 1.1 December 1990: Tests cleaned up; 1b, 1e - 1k added
 Portability attempted.
 1.2 December 1991: Still working on portabililty.
 1.3 July     1992: tests1i/1j made re-entrant.
 1.4 December 1992: Minor changes - portability
 tests2c/2d added.  2f/2g rewritten
 1.5 August   1993: Produced new test2g to replace old
 2g that did a swapping test.
 1.6 June     1995: Test Cleanup.
 1.7 June     1999: Add test0, minor fixes.
 2.0 January  2000: Lots of small changes & bug fixes
 2.1 March    2001: Minor Bugfixes.
 Rewrote GetSkewedRandomNumber
 2.2 July     2002: Make appropriate for undergrads
 3.0 August   2004: Modified to support memory mapped IO
 3.1 August   2004: hardware interrupt runs on separate thread
 3.11 August  2004: Support for OS level locking
 3.13 November 2004: Minor fix defining USER
 3.41 August  2009: Additional work for multiprocessor + 64 bit
 3.53 November 2011: Changed test2c so data structure used
 ints (4 bytes) rather than longs.
 3.61 November 2012: Fixed a number of bugs in test2g and test2gx
 (There are probably many more)
 3.70 December 2012: Rename test2g to test2h - it still does
 shared memory.  Define a new test2g that runs
 multiple copies of test2f.
 4.03 December 2013: Changes to test 2e and 2f.
 4.10 December 2013: Changes to test 2g regarding a clean ending.
 July     2014: Numerous small changes to tests.
 Major rwrite of Test2h
 4.11 November 2014: Bugfix to test2c and test2e
 4.12 November 2014: Bugfix to test2e
 4.20 January 2015: Start work to make thread safe and therefore support
 multiprocessors.
 ************************************************************************/

#define          USER
#include         "global.h"
#include         "protos.h"
#include         "syscalls.h"

#include         "stdio.h"
#include         "string.h"
#include         "stdlib.h"
#include         "math.h"

INT16 Z502_PROGRAM_COUNTER;

// extern INT16 Z502_MODE;
/*      Prototypes for internally called routines.                  */

void test1x(void);
void test1j_echo(void);
void test2hx(void);
void ErrorExpected(INT32, char[]);
void SuccessExpected(INT32, char[]);
void GetRandomNumber(long *, long);
void GetSkewedRandomNumber(long *, long);
void Test2f_Statistics(int Pid);

/**************************************************************************

 Test0

 Exercises the system calls for GET_TIME_OF_DAY and TERMINATE_PROCESS

 **************************************************************************/

void test0(void) {
	long ReturnedTime;
	long ErrorReturned;
	printf("This is Release %s:  Test 0\n", CURRENT_REL);
	GET_TIME_OF_DAY(&ReturnedTime);

	printf("Time of day is %ld\n", ReturnedTime);
	TERMINATE_PROCESS(-1, &ErrorReturned);

	// We should never get to this line since the TERMINATE_PROCESS call
	// should cause the program to end.
	printf("ERROR: Test should be terminated but isn't.\n");
}                                               // End of test0

/**************************************************************************
 Test1a

 Exercises GET_TIME_OF_DAY and SLEEP and TERMINATE_PROCESS
 What should happen here is that the difference between the time1 and time2
 will be GREATER than SleepTime.  This is because a timer interrupt takes
 AT LEAST the time specified.

 **************************************************************************/

void test1a(void) {
	long ErrorReturned;
	long SleepTime = 100;
	INT32 Time1, Time2;

	printf("This is Release %s:  Test 1a\n", CURRENT_REL);
	GET_TIME_OF_DAY(&Time1);

	SLEEP(SleepTime);

	GET_TIME_OF_DAY(&Time2);

	printf("Sleep Time = %ld, elapsed time= %d\n", SleepTime, Time2 - Time1);
	TERMINATE_PROCESS(-1, &ErrorReturned);

	printf("ERROR: Test should be terminated but isn't.\n");

} /* End of test1a    */

/**************************************************************************
 Test1b

 Exercises the CREATE_PROCESS and GET_PROCESS_ID  commands.

 This test tries lots of different inputs for create_process.
 In particular, there are tests for each of the following:

 1. use of illegal priorities
 2. use of a process name of an already existing process.
 3. creation of a LARGE number of processes, showing that
 there is a limit somewhere ( you run out of some
 resource ) in which case you take appropriate action.

 Test the following items for get_process_id:

 1. Various legal process id inputs.
 2. An illegal/non-existant name.

 **************************************************************************/

#define         ILLEGAL_PRIORITY                -3
#define         LEGAL_PRIORITY                  10

void test1b(void) {
	long ProcessID1;            // Used as return of process id's.
	long ProcessID2;            // Used as return of process id's.
	long NumberOfProcesses = 0; // Cntr of # of processes created.
	long ErrorReturned;         // Used as return of error code.
	long TimeNow;               // Holds ending time of test
	char ProcessName[16];       // Holds generated process name

	// Try to create a process with an illegal priority.
	printf("This is Release %s:  Test 1b\n", CURRENT_REL);
	CREATE_PROCESS("test1b_a", test1x, ILLEGAL_PRIORITY, &ProcessID1,
			&ErrorReturned);
	ErrorExpected(ErrorReturned, "CREATE_PROCESS");

	// Create two processes with same name - 1st succeeds, 2nd fails
	// Then terminate the process that has been created
	CREATE_PROCESS("two_the_same", test1x, LEGAL_PRIORITY, &ProcessID2,
			&ErrorReturned);
	SuccessExpected(ErrorReturned, "CREATE_PROCESS");
	CREATE_PROCESS("two_the_same", test1x, LEGAL_PRIORITY, &ProcessID1,
			&ErrorReturned);
	ErrorExpected(ErrorReturned, "CREATE_PROCESS");
	TERMINATE_PROCESS(ProcessID2, &ErrorReturned);
	SuccessExpected(ErrorReturned, "TERMINATE_PROCESS");

	// Loop until an error is found on the CREATE_PROCESS system call.
	// Since the call itself is legal, we must get an error
	// because we exceed some limit.
	ErrorReturned = ERR_SUCCESS;
	while (ErrorReturned == ERR_SUCCESS) {
		NumberOfProcesses++; /* Generate next unique program name*/
		sprintf(ProcessName, "Test1b_%ld", NumberOfProcesses);
		printf("Creating process \"%s\"\n", ProcessName);
		CREATE_PROCESS(ProcessName, test1x, LEGAL_PRIORITY, &ProcessID1,
				&ErrorReturned);
	}

	//  When we get here, we've created all the processes we can.
	//  So the OS should have given us an error
	ErrorExpected(ErrorReturned, "CREATE_PROCESS");
	printf("%ld processes were created in all.\n", NumberOfProcesses);

	//      Now test the call GET_PROCESS_ID for ourselves
	GET_PROCESS_ID("", &ProcessID2, &ErrorReturned);     // Legal
	SuccessExpected(ErrorReturned, "GET_PROCESS_ID");
	printf("The PID of this process is %ld\n", ProcessID2);

	// Try GET_PROCESS_ID on another existing process
	strcpy(ProcessName, "Test1b_1");
	GET_PROCESS_ID(ProcessName, &ProcessID1, &ErrorReturned); /* Legal */
	SuccessExpected(ErrorReturned, "GET_PROCESS_ID");
	printf("The PID of target process is %ld\n", ProcessID1);

	// Try GET_PROCESS_ID on a non-existing process
	GET_PROCESS_ID("bogus_name", &ProcessID1, &ErrorReturned); // Illegal
	ErrorExpected(ErrorReturned, "GET_PROCESS_ID");

	GET_TIME_OF_DAY(&TimeNow);
	printf("Test1b, PID %ld, Ends at Time %ld\n", ProcessID2, TimeNow);
	TERMINATE_PROCESS(-2, &ErrorReturned)

}                                                  // End of test1b

/**************************************************************************
 Test1c

 Tests multiple copies of test1x running simultaneously.
 Test1c runs these with the same priority in order to show FCFS scheduling
 behavior; Test1d uses different priorities in order to show priority
 scheduling.

 WARNING:  This test assumes tests 1a - 1b run successfully

 **************************************************************************/
#define         PRIORITY1C              10

void test1c(void) {
	long ProcessID1;            // Contains a Process ID
	long ProcessID2;            // Contains a Process ID
	long ProcessID3;            // Contains a Process ID
	long ProcessID4;            // Contains a Process ID
	long ProcessID5;            // Contains a Process ID
	long ReturnedPID;           // Value of PID returned by System Call
	long ErrorReturned;
	long SleepTime = 1000;

	printf("This is Release %s:  Test 1c\n", CURRENT_REL);
	CREATE_PROCESS("test1c_a", test1x, PRIORITY1C, &ProcessID1, &ErrorReturned);
	SuccessExpected(ErrorReturned, "CREATE_PROCESS");

	CREATE_PROCESS("test1c_b", test1x, PRIORITY1C, &ProcessID2, &ErrorReturned);

	CREATE_PROCESS("test1c_c", test1x, PRIORITY1C, &ProcessID3, &ErrorReturned);

	CREATE_PROCESS("test1c_d", test1x, PRIORITY1C, &ProcessID4, &ErrorReturned);

	CREATE_PROCESS("test1c_e", test1x, PRIORITY1C, &ProcessID5, &ErrorReturned);

	// Now we sleep, see if one of the five processes has terminated, and
	// continue the cycle until one of them is gone.  This allows the test1x
	// processes to exhibit scheduling.
	// We know that the process terminated when we do a GET_PROCESS_ID and
	// receive an error on the system call.

	ErrorReturned = ERR_SUCCESS;
	while (ErrorReturned == ERR_SUCCESS) {
		SLEEP(SleepTime);
		GET_PROCESS_ID("test1c_e", &ReturnedPID, &ErrorReturned);
	}

	TERMINATE_PROCESS(-2, &ErrorReturned); /* Terminate all */

}                                                     // End test1c

/**************************************************************************
 Test 1d

 Tests multiple copies of test1x running simultaneously.
 Test1c runs these with the same priority in order to show
 FCFS scheduling behavior; Test1d uses different priorities
 in order to show priority scheduling.

 WARNING:  This test assumes tests 1a - 1b run successfully

 **************************************************************************/

#define         PRIORITY1               10
#define         PRIORITY2               11
#define         PRIORITY3               11
#define         PRIORITY4               90
#define         PRIORITY5               40

void test1d(void) {
	long ProcessID1;            // Contains a Process ID
	long ProcessID2;            // Contains a Process ID
	long ProcessID3;            // Contains a Process ID
	long ProcessID4;            // Contains a Process ID
	long ProcessID5;            // Contains a Process ID
	long ReturnedPID;           // Value of PID returned by System Call
	long ErrorReturned;
	long SleepTime = 1000;

	printf("This is Release %s:  Test 1d\n", CURRENT_REL);
	CREATE_PROCESS("test1d_1", test1x, PRIORITY1, &ProcessID1, &ErrorReturned);
	SuccessExpected(ErrorReturned, "CREATE_PROCESS");

	CREATE_PROCESS("test1d_2", test1x, PRIORITY2, &ProcessID2, &ErrorReturned);

	CREATE_PROCESS("test1d_3", test1x, PRIORITY3, &ProcessID3, &ErrorReturned);

	CREATE_PROCESS("test1d_4", test1x, PRIORITY4, &ProcessID4, &ErrorReturned);

	CREATE_PROCESS("test1d_5", test1x, PRIORITY5, &ProcessID5, &ErrorReturned);

	// Now we sleep, see if one of the five processes has terminated, and
	// continue the cycle until one of them is gone.  This allows the test1x
	// processes to exhibit scheduling.
	// We know that the process terminated when we do a GET_PROCESS_ID and
	// receive an error on the system call.

	ErrorReturned = ERR_SUCCESS;
	while (ErrorReturned == ERR_SUCCESS) {
		SLEEP(SleepTime);
		GET_PROCESS_ID("test1d_4", &ReturnedPID, &ErrorReturned);
	}

	TERMINATE_PROCESS(-2, &ErrorReturned);

}                                                 // End test1d

/**************************************************************************
 Test 1e

 Exercises the SUSPEND_PROCESS and RESUME_PROCESS commands

 This test should try lots of different inputs for suspend and resume.
 In particular, there should be tests for each of the following:

 1. use of illegal process id.
 2. what happens when you suspend yourself - is it legal?  The answer
 to this depends on the OS architecture and is up to the developer.
 3. suspending an already suspended process.
 4. resuming a process that's not suspended.

 there are probably lots of other conditions possible.

 **************************************************************************/
#define         LEGAL_PRIORITY_1E               10

void test1e(void) {
	long TargetProcessID;
	long OurProcessID;
	long TimeNow;               // Holds ending time of test
	long ErrorReturned;

	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
	printf("Release %s:Test 1e: Pid %ld\n", CURRENT_REL, OurProcessID);

	// Make a legal target process
	CREATE_PROCESS("test1e_a", test1x, LEGAL_PRIORITY_1E, &TargetProcessID,
			&ErrorReturned);
	SuccessExpected(ErrorReturned, "CREATE_PROCESS");

	// Try to Suspend an Illegal PID
	SUSPEND_PROCESS((INT32 )9999, &ErrorReturned);
	ErrorExpected(ErrorReturned, "SUSPEND_PROCESS");

	// Try to Resume an Illegal PID
	RESUME_PROCESS((INT32 )9999, &ErrorReturned);
	ErrorExpected(ErrorReturned, "RESUME_PROCESS");

	// Suspend alegal PID
	SUSPEND_PROCESS(TargetProcessID, &ErrorReturned);
	SuccessExpected(ErrorReturned, "SUSPEND_PROCESS");

	// Suspend already suspended PID
	SUSPEND_PROCESS(TargetProcessID, &ErrorReturned);
	ErrorExpected(ErrorReturned, "SUSPEND_PROCESS");

	// Do a legal resume of the process we have suspended
	RESUME_PROCESS(TargetProcessID, &ErrorReturned);
	SuccessExpected(ErrorReturned, "RESUME_PROCESS");

	// Resume an already resumed process
	RESUME_PROCESS(TargetProcessID, &ErrorReturned);
	ErrorExpected(ErrorReturned, "RESUME_PROCESS");

	// Try to resume ourselves
	RESUME_PROCESS(OurProcessID, &ErrorReturned);
	ErrorExpected(ErrorReturned, "RESUME_PROCESS");

	// It may or may not be legal to suspend ourselves;
	// architectural decision.   It can be a useful technique
	// as a way to pass off control to another process.
	SUSPEND_PROCESS(-1, &ErrorReturned);

	/* If we returned "SUCCESS" here, then there is an inconsistency;
	 * success implies that the process was suspended.  But if we
	 * get here, then we obviously weren't suspended.  Therefore
	 * this must be an error.                                    */
	ErrorExpected(ErrorReturned, "SUSPEND_PROCESS");

	GET_TIME_OF_DAY(&TimeNow);
	printf("Test1e, PID %ld, Ends at Time %ld\n", OurProcessID, TimeNow);

	TERMINATE_PROCESS(-2, &ErrorReturned);
}                                                // End of test1e

/**************************************************************************
 Test1f

 Successfully suspend and resume processes. This assumes that Test1e
 runs successfully.

 In particular, show what happens to scheduling when processes
 are temporarily suspended.

 This test works by starting up a number of processes at different
 priorities.  Then some of them are suspended.  Then some are resumed.

 **************************************************************************/
#define         PRIORITY_1F1             5
#define         PRIORITY_1F2            10
#define         PRIORITY_1F3            15
#define         PRIORITY_1F4            20
#define         PRIORITY_1F5            25

void test1f(void) {
	long OurProcessID;         // PID of this process
	long ProcessID1;           // Created processes
	long ProcessID2;
	long ProcessID3;
	long ProcessID4;
	long ProcessID5;
	long SleepTime = 300;
	long ErrorReturned;
	int Iterations;

	// Get OUR PID
	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
	printf("Release %s:Test 1f: Pid %ld\n", CURRENT_REL, OurProcessID);

	// Make legal targets
	CREATE_PROCESS("test1f_a", test1x, PRIORITY_1F1, &ProcessID1,
			&ErrorReturned);
	CREATE_PROCESS("test1f_b", test1x, PRIORITY_1F2, &ProcessID2,
			&ErrorReturned);
	CREATE_PROCESS("test1f_c", test1x, PRIORITY_1F3, &ProcessID3,
			&ErrorReturned);
	CREATE_PROCESS("test1f_d", test1x, PRIORITY_1F4, &ProcessID4,
			&ErrorReturned);
	CREATE_PROCESS("test1f_e", test1x, PRIORITY_1F5, &ProcessID5,
			&ErrorReturned);

	// Let the 5 processes go for a while
	SLEEP(SleepTime);

	// Do a set of suspends/resumes four times
	for (Iterations = 0; Iterations < 4; Iterations++) {
		// Suspend 3 of the pids and see what happens - we should see
		// scheduling behavior where the processes are yanked out of the
		// ready and the waiting states, and placed into the suspended state.

		SUSPEND_PROCESS(ProcessID1, &ErrorReturned);
		SUSPEND_PROCESS(ProcessID3, &ErrorReturned);
		SUSPEND_PROCESS(ProcessID5, &ErrorReturned);

		// Sleep so we can watch the scheduling action
		SLEEP(SleepTime);

		RESUME_PROCESS(ProcessID1, &ErrorReturned);
		RESUME_PROCESS(ProcessID3, &ErrorReturned);
		RESUME_PROCESS(ProcessID5, &ErrorReturned);
	}

	//   Wait for children to finish, then quit
	SLEEP((INT32 )10000);
	TERMINATE_PROCESS(-2, &ErrorReturned);

}                        // End of test1f

/**************************************************************************
 Test1g

 Generate lots of errors for CHANGE_PRIORITY

 Try lots of different inputs: In particular, some of the possible
 inputs include:

 1. use of illegal priorities
 2. use of an illegal process id.

 **************************************************************************/
#define         LEGAL_PRIORITY_1G               10
#define         ILLEGAL_PRIORITY_1G            999

void test1g(void) {
	long OurProcessID;         // PID of this process
	long TargetProcessID;      // Created processes
	long ErrorReturned;

	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
	printf("Release %s:Test 1g: Pid %ld\n", CURRENT_REL, OurProcessID);

	// Make a legal target
	CREATE_PROCESS("test1g_a", test1x, LEGAL_PRIORITY_1G, &TargetProcessID,
			&ErrorReturned);
	SuccessExpected(ErrorReturned, "CREATE_PROCESS");

	// Target Illegal PID
	CHANGE_PRIORITY((INT32 )9999, LEGAL_PRIORITY_1G, &ErrorReturned);
	ErrorExpected(ErrorReturned, "CHANGE_PRIORITY");

	// Use illegal priority
	CHANGE_PRIORITY(TargetProcessID, ILLEGAL_PRIORITY_1G, &ErrorReturned);
	ErrorExpected(ErrorReturned, "CHANGE_PRIORITY");

	// Use legal priority on legal process
	CHANGE_PRIORITY(TargetProcessID, LEGAL_PRIORITY_1G, &ErrorReturned);
	SuccessExpected(ErrorReturned, "CHANGE_PRIORITY");
	// Terminate all existing processes
	TERMINATE_PROCESS(-2, &ErrorReturned);

}                                          // End of test1g

/**************************************************************************

 Test1h  Successfully change the priority of a process

 There are TWO ways we can see that the priorities have changed:
 1. When you change the priority, it should be possible to see
    the scheduling behaviour of the system change; processes
    that used to be scheduled first are no longer first.  This will be
    visible in the ready Q as shown by the scheduler printer.
 2. The processes with more favorable priority should schedule first so
    they should finish first.

 **************************************************************************/

#define         MOST_FAVORABLE_PRIORITY         1
#define         FAVORABLE_PRIORITY             10
#define         NORMAL_PRIORITY                20
#define         LEAST_FAVORABLE_PRIORITY       30

void test1h(void) {
	long OurProcessID;         // PID of this process
	long ProcessID1;           // Created processes
	long ProcessID2;
	long ProcessID3;
	long ProcessID4;
	long ProcessID5;
	long ProcessID6;
	long SleepTime = 600;
	long ErrorReturned;
	long Ourself = -1;

	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);

	// Make our priority high
	printf("Release %s:Test 1h: Pid %ld\n", CURRENT_REL, OurProcessID);
	CHANGE_PRIORITY(Ourself, MOST_FAVORABLE_PRIORITY, &ErrorReturned);

	// Make legal targets
        printf( "TEST1h: Processes started with priority %d\n", NORMAL_PRIORITY);
	CREATE_PROCESS("test1h_a", test1x, NORMAL_PRIORITY, &ProcessID1,
			&ErrorReturned);
	CREATE_PROCESS("test1h_b", test1x, NORMAL_PRIORITY, &ProcessID2,
			&ErrorReturned);
	CREATE_PROCESS("test1h_c", test1x, NORMAL_PRIORITY, &ProcessID3,
			&ErrorReturned);
	CREATE_PROCESS("test1h_d", test1x, NORMAL_PRIORITY, &ProcessID4,
			&ErrorReturned);
	CREATE_PROCESS("test1h_e", test1x, NORMAL_PRIORITY, &ProcessID5,
			&ErrorReturned);
	CREATE_PROCESS("test1h_f", test1x, NORMAL_PRIORITY, &ProcessID6,
			&ErrorReturned);

	//      Sleep awhile to watch the scheduling
	SLEEP(SleepTime);

	//  Now change the priority - it should be possible to see
	//  that the priorities have been changed for processes that
	//  are ready and for processes that are sleeping.

        printf( "TEST1h: Process %ld now has priority %d\n", ProcessID1, 
                  FAVORABLE_PRIORITY);
        CHANGE_PRIORITY(ProcessID1, FAVORABLE_PRIORITY, &ErrorReturned);

        printf( "TEST1h: Process %ld now has priority %d\n", ProcessID3, 
                  LEAST_FAVORABLE_PRIORITY);
        CHANGE_PRIORITY(ProcessID3, LEAST_FAVORABLE_PRIORITY, &ErrorReturned);

        //      Sleep awhile to watch the scheduling
        SLEEP(SleepTime);

        //  Now change the priority - it should be possible to see
        //  that the priorities have been changed for processes that
        //  are ready and for processes that are sleeping.

        printf( "TEST1h: Process %ld now has priority %d\n", ProcessID1,
                  LEAST_FAVORABLE_PRIORITY);
        CHANGE_PRIORITY(ProcessID1, LEAST_FAVORABLE_PRIORITY, &ErrorReturned);

        printf( "TEST1h: Process %ld now has priority %d\n", ProcessID2,
                  FAVORABLE_PRIORITY);
        CHANGE_PRIORITY(ProcessID2, FAVORABLE_PRIORITY, &ErrorReturned);

        //     Sleep awhile to watch the scheduling
        SLEEP(SleepTime);

        // Terminate everyone
	TERMINATE_PROCESS(-2, &ErrorReturned);

}                                               // End of test1h  

/**************************************************************************

 Test1i   SEND_MESSAGE and RECEIVE_MESSAGE with errors.

 This has the same kind of error conditions that previous tests did;
 bad PIDs, bad message lengths, illegal buffer addresses, etc.
 Your imagination can go WILD on this one.

 This is a good time to mention an important aspect of the OS and
 scheduling.
 As you know, after doing a switch_context, the hardware passes
 control to the code after SwitchContext.  In other words, a process
 that is being rescheduled "disappears" into SwitchContext.  But
 it "reappears" after some other process causes that "disappeared"
 process to be scheduled.
 So at that "reappearing" location is a good place to put your message 
 code so as to do the rendevous work that's necessary to match up sends
 and receives.

 Why do this:        Suppose process A has sent a message to
 process B.  It so happens that you may well want
 to do some preparation in process B once it's
 registers are in memory, but BEFORE it executes
 the test.  In other words, it allows you to
 complete the work for the send to process B.

 We use test1x as our target process; but since it doesn't have any
 messaging code, it never actually sends or receives messages so it
 will never actually be scheduled.

 **************************************************************************/

#define         LEGAL_MESSAGE_LENGTH           (INT16)64
#define         ILLEGAL_MESSAGE_LENGTH         (INT16)1000

#define         MOST_FAVORABLE_PRIORITY         1
#define         NORMAL_PRIORITY                20

typedef struct {
	long TargetPid;
	long SourcePid;
	long ActualSourcePid;
	long SendLength;
	long ReceiveLength;
	long ActualSendLength;
	long loop_count;
	char MessageBuffer[LEGAL_MESSAGE_LENGTH ];
} TEST1I_DATA;

void test1i(void) {
	long OurProcessID;         // PID of this process
	long ProcessID1;           // Created processes
	long ErrorReturned;
	TEST1I_DATA *td; // Use as ptr to data */

	// Here we maintain the data to be used by this process when running
	// on this routine.  This code should be re-entrant.

	td = (TEST1I_DATA *) calloc(1, sizeof(TEST1I_DATA));
	if (td == 0) {
		printf("Something screwed up allocating space in test1i\n");
	}

	td->loop_count = 0;

	// Get OUR PID
	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
	printf("Release %s:Test 1i: Pid %ld\n", CURRENT_REL, OurProcessID);

	// Make our priority high
	CHANGE_PRIORITY(-1, MOST_FAVORABLE_PRIORITY, &ErrorReturned);

	// Make a legal target
	CREATE_PROCESS("test1i_a", test1x, NORMAL_PRIORITY, &ProcessID1,
			&ErrorReturned);

	// Send a message to illegal process
	td->TargetPid = 9999;
	td->SendLength = 8;
	SEND_MESSAGE(td->TargetPid, "message", td->SendLength, &ErrorReturned);
	ErrorExpected(ErrorReturned, "SEND_MESSAGE");

	// Try an illegal message length
	td->TargetPid = ProcessID1;
	td->SendLength = ILLEGAL_MESSAGE_LENGTH;
	SEND_MESSAGE(td->TargetPid, "message", td->SendLength, &ErrorReturned);
	ErrorExpected(ErrorReturned, "SEND_MESSAGE");

	//      Receive from illegal process
	td->SourcePid = 9999;
	td->ReceiveLength = LEGAL_MESSAGE_LENGTH;
	RECEIVE_MESSAGE(td->SourcePid, td->MessageBuffer, td->ReceiveLength,
			&(td->ActualSendLength), &(td->ActualSourcePid), &ErrorReturned);
	ErrorExpected(ErrorReturned, "RECEIVE_MESSAGE");

	//      Receive with illegal buffer size
	td->SourcePid = ProcessID1;
	td->ReceiveLength = ILLEGAL_MESSAGE_LENGTH;
	RECEIVE_MESSAGE(td->SourcePid, td->MessageBuffer, td->ReceiveLength,
			&(td->ActualSendLength), &(td->ActualSourcePid), &ErrorReturned);
	ErrorExpected(ErrorReturned, "RECEIVE_MESSAGE");

	//      Send a legal ( but long ) message to self
	td->TargetPid = OurProcessID;
	td->SendLength = LEGAL_MESSAGE_LENGTH;
	SEND_MESSAGE(td->TargetPid, "a long but legal message", td->SendLength,
			&ErrorReturned);
	SuccessExpected(ErrorReturned, "SEND_MESSAGE");
	td->loop_count++;      // Count the number of legal messages sent

	//   Receive this long message, which should error because the receive buffer is too small
	td->SourcePid = OurProcessID;
	td->ReceiveLength = 10;
	RECEIVE_MESSAGE(td->SourcePid, td->MessageBuffer, td->ReceiveLength,
			&(td->ActualSendLength), &(td->ActualSourcePid), &ErrorReturned);
	ErrorExpected(ErrorReturned, "RECEIVE_MESSAGE");

	// Keep sending legal messages until the architectural
	// limit for buffer space is exhausted.  In order to pass
	// the  test1j, this number should be at least EIGHT

	ErrorReturned = ERR_SUCCESS;
	while (ErrorReturned == ERR_SUCCESS) {
		td->TargetPid = ProcessID1;
		td->SendLength = LEGAL_MESSAGE_LENGTH;
		SEND_MESSAGE(td->TargetPid, "Legal Test1i Message", td->SendLength,
				&ErrorReturned);
		td->loop_count++;
	}  // End of while

	printf("A total of %ld messages were enqueued.\n", td->loop_count - 1);
	TERMINATE_PROCESS(-2, &ErrorReturned);
}                                              // End of test1i     

/**************************************************************************

 Test1j   SEND_MESSAGE and RECEIVE_MESSAGE Successfully.

 Creates three other processes, each running their own code.
 RECEIVE and SEND messages are winged back and forth at them.

 The SEND and RECEIVE system calls as implemented by this test
 imply the following behavior:

 SENDER = PID A          RECEIVER = PID B,

 Designates SourcePid =
 TargetPid =          A            C             -1
 ----------------+------------+------------+--------------+
 |            |            |              |               |
 | B          |  Message   |     X        |   Message     |
 |Transmitted |            | Transmitted  |               |
 ----------------+------------+------------+--------------+
 |            |            |              |               |
 | C          |     X      |     X        |       X       |
 |            |            |              |               |
 ----------------+------------+------------+--------------+
 |            |            |              |               |
 | -1         |   Message  |     X        |   Message     |
 | Transmitted|            | Transmitted  |               |
 ----------------+------------+------------+--------------+
 A broadcast ( TargetPid = -1 ) means send to everyone BUT yourself.
 ANY of the receiving processes can handle a broadcast message.
 A receive ( SourcePid = -1 ) means receive from anyone.
 **************************************************************************/

#define         LEGAL_MESSAGE_LENGTH            (INT16)64
#define         ILLEGAL_MESSAGE_LENGTH          (INT16)1000
#define         MOST_FAVORABLE_PRIORITY         1
#define         NORMAL_PRIORITY                20

typedef struct {
	long TargetPid;
	long SourcePid;
	long ActualSourcePid;
	long SendLength;
	long ReceiveLength;
	long ActualSendLength;
	long send_loop_count;
	long receive_loop_count;
	char MessageBuffer[LEGAL_MESSAGE_LENGTH ];
	char MessageSent[LEGAL_MESSAGE_LENGTH ];
} TEST1J_DATA;

void test1j(void) {
	long OurProcessID;         // PID of this process
	long ProcessID1;           // Created processes
	long ProcessID2;           // Created processes
	long ProcessID3;           // Created processes
	long ErrorReturned;
	int Iteration;
	TEST1J_DATA *td;           // Use as pointer to data

	// Here we maintain the data to be used by this process when running
	// on this routine.  This code should be re-entrant.                */

	td = (TEST1J_DATA *) calloc(1, sizeof(TEST1J_DATA));
	if (td == 0) {
		printf("Something screwed up allocating space in test1j\n");
	}
	td->send_loop_count = 0;
	td->receive_loop_count = 0;

	// Get OUR PID
	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);

	// Make our prior high
	printf("Release %s:Test 1j: Pid %ld\n", CURRENT_REL, OurProcessID);
	CHANGE_PRIORITY(-1, MOST_FAVORABLE_PRIORITY, &ErrorReturned);
	SuccessExpected(ErrorReturned, "CHANGE_PRIORITY");

	// Make 3 legal targets
	CREATE_PROCESS("test1j_1", test1j_echo, NORMAL_PRIORITY, &ProcessID1,
			&ErrorReturned);
	SuccessExpected(ErrorReturned, "CREATE_PROCESS");
	CREATE_PROCESS("test1j_2", test1j_echo, NORMAL_PRIORITY, &ProcessID2,
			&ErrorReturned);
	SuccessExpected(ErrorReturned, "CREATE_PROCESS");
	CREATE_PROCESS("test1j_3", test1j_echo, NORMAL_PRIORITY, &ProcessID3,
			&ErrorReturned);
	SuccessExpected(ErrorReturned, "CREATE_PROCESS");

	//      Send/receive a legal message to each child
	for (Iteration = 1; Iteration <= 3; Iteration++) {
		if (Iteration == 1) {
			td->TargetPid = ProcessID1;
			strcpy(td->MessageSent, "message to #1");
		}
		if (Iteration == 2) {
			td->TargetPid = ProcessID2;
			strcpy(td->MessageSent, "message to #2");
		}
		if (Iteration == 3) {
			td->TargetPid = ProcessID3;
			strcpy(td->MessageSent, "message to #3");
		}
		td->SendLength = 20;
		SEND_MESSAGE(td->TargetPid, td->MessageSent, td->SendLength,
				&ErrorReturned);
		SuccessExpected(ErrorReturned, "SEND_MESSAGE");

		td->SourcePid = -1;
		td->ReceiveLength = LEGAL_MESSAGE_LENGTH;
		RECEIVE_MESSAGE(td->SourcePid, td->MessageBuffer, td->ReceiveLength,
				&(td->ActualSendLength), &(td->ActualSourcePid),
				&ErrorReturned);
		SuccessExpected(ErrorReturned, "RECEIVE_MESSAGE");

		if (strcmp(td->MessageBuffer, td->MessageSent) != 0)
			printf("ERROR - msg sent != msg received.\n");

		if (td->ActualSourcePid != td->TargetPid)
			printf("ERROR - source PID not correct.\n");

		if (td->ActualSendLength != td->SendLength)
			printf("ERROR - send length not sent correctly.\n");
	}    // End of for loop

	//      Keep sending legal messages until the architectural (OS)
	//      limit for buffer space is exhausted.

	ErrorReturned = ERR_SUCCESS;
	while (ErrorReturned == ERR_SUCCESS) {
		td->TargetPid = -1;
		sprintf(td->MessageSent, "This is message %ld          ", 
                       td->send_loop_count);
		td->SendLength = 20 + (td->send_loop_count % 8);
		SEND_MESSAGE(td->TargetPid, td->MessageSent, td->SendLength,
				&ErrorReturned);

		td->send_loop_count++;
	}
	td->send_loop_count--;
	printf("A total of %ld messages were enqueued.\n", td->send_loop_count);

	//  Now receive back from the other processes the same number of messages we've just sent.
	SLEEP(100);
	while (td->receive_loop_count < td->send_loop_count) {
		td->SourcePid = -1;
		td->ReceiveLength = LEGAL_MESSAGE_LENGTH;
		RECEIVE_MESSAGE(td->SourcePid, td->MessageBuffer, td->ReceiveLength,
				&(td->ActualSendLength), &(td->ActualSourcePid),
				&ErrorReturned);
		SuccessExpected(ErrorReturned, "RECEIVE_MESSAGE");
                if ( strlen(td->MessageBuffer) != td->ActualSendLength ) {
                   printf( "Test1j:  ERROR - String received doesn't match specified length\n");
                }
		printf("Test1j: Receive from PID = %ld: length = %ld: msg = %s:\n",
				td->ActualSourcePid, 
                                td->ActualSendLength, 
                                td->MessageBuffer);
		td->receive_loop_count++;
	}

	printf("A total of %ld messages were received.\n", td->receive_loop_count);
	CALL(TERMINATE_PROCESS(-2, &ErrorReturned ));

}                                                 // End of test1j     

/**************************************************************************

 Test1k  Test other oddities in your system.


 There are many other strange effects, not taken into account
 by the previous tests.  One of these is:

 1. Executing a privileged instruction from a user program
 This should cause the program to be terminated.  The hardware will
 cause a fault to occur.  Your fault handler in the OS will catch this
 and terminate the program.

 **************************************************************************/

void test1k(void) {
	long OurProcessID;         // PID of this process
	long ErrorReturned;
	//   INT32        MemoryReadResult;
	MEMORY_MAPPED_IO mmio;

	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);

	printf("Release %s:Test 1k: Pid %ld\n", CURRENT_REL, OurProcessID);

	//  Do an illegal hardware instruction - we will
	//  not return from this.    The hardware will take a fault
	mmio.Mode = Z502Status;
	MEM_READ(Z502Timer, (void * )&mmio);

}                       // End of test1k

/**************************************************************************
 Test 1l

 Write an interesting test of your own to exhibit some feature of
 your Operating System.

 **************************************************************************/
void test1l(void) {
}                                               // End test1l

/**************************************************************************
 Test1x

 is used as a target by the process creation programs.
 It has the virtue of causing lots of rescheduling activity in
 a relatively random way.

 **************************************************************************/

#define         NUMBER_OF_TEST1X_ITERATIONS     10

void test1x(void) {
	long OurProcessID;
	long RandomSeed;
	long EndingTime;
	long RandomSleep = 17;
	long ErrorReturned;
	int Iterations;

	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
	printf("Release %s:Test 1x: Pid %ld\n", CURRENT_REL, OurProcessID);

	for (Iterations = 0; Iterations < NUMBER_OF_TEST1X_ITERATIONS;
			Iterations++) {
		GET_TIME_OF_DAY(&RandomSeed);
		RandomSleep = (RandomSleep * RandomSeed) % 143;
		SLEEP(RandomSleep);
		GET_TIME_OF_DAY(&EndingTime);
		printf("Test1X: Pid = %d, Sleep Time = %ld, Latency Time = %d\n",
				(int) OurProcessID, RandomSleep,
				(int) (EndingTime - RandomSeed));
	}
	printf("Test1x, PID %ld, Ends at Time %ld\n", OurProcessID, EndingTime);

	TERMINATE_PROCESS(-1, &ErrorReturned);
	printf("ERROR: Test1x should be terminated but isn't.\n");

}                                                  // End of test1x 

/**************************************************************************

 Test1j_echo

 is used as a target by the message send/receive programs.
 All it does is send back the same message it received to the
 same sender.

 **************************************************************************/

typedef struct {
	long TargetPid;
	long SourcePid;
	long ActualSourcePid;
	long SendLength;
	long ReceiveLength;
	long ActualSendersLength;
	char MessageBuffer[LEGAL_MESSAGE_LENGTH ];
	char MessageSent[LEGAL_MESSAGE_LENGTH ];
} TEST1J_ECHO_DATA;

void test1j_echo(void) {
	long OurProcessID;
	long ErrorReturned;
	TEST1J_ECHO_DATA *td;                          // Use as ptr to data

	// Here we maintain the data to be used by this process when running
	// on this routine.  This code should be re-entrant.

	td = (TEST1J_ECHO_DATA *) calloc(1, sizeof(TEST1J_ECHO_DATA));
	if (td == 0) {
		printf("Something screwed up allocating space in test1j_echo\n");
	}

	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
	SuccessExpected(ErrorReturned, "GET_PROCESS_ID");
	printf("Release %s:Test 1j_echo: Pid %ld\n", CURRENT_REL, OurProcessID);

	while (1) {       // Loop forever.
		td->SourcePid = -1;
		td->ReceiveLength = LEGAL_MESSAGE_LENGTH;
		RECEIVE_MESSAGE(td->SourcePid, td->MessageBuffer, td->ReceiveLength,
				&(td->ActualSendersLength), &(td->ActualSourcePid),
				&ErrorReturned);
		SuccessExpected(ErrorReturned, "RECEIVE_MESSAGE");
                if ( strlen(td->MessageBuffer) != td->ActualSendersLength ) {
                   printf( "Pid %ld:  ERROR - String received doesn't ", OurProcessID);
                   printf( "match specified length\n");

                }
		printf("PID %ld: Receive from PID = %ld: length = %ld: msg = %s:\n",
				OurProcessID, td->ActualSourcePid, 
                                td->ActualSendersLength, td->MessageBuffer);

		td->TargetPid = td->ActualSourcePid;
		strcpy(td->MessageSent, td->MessageBuffer);
		td->SendLength = td->ActualSendersLength;
		SEND_MESSAGE(td->TargetPid, td->MessageSent, td->SendLength,
				&ErrorReturned);
		SuccessExpected(ErrorReturned, "SEND_MESSAGE");
	}     // End of while

}                                               // End of test1j_echo

/**************************************************************************
 ErrorExpected    and    SuccessExpected

 These routines simply handle the display of success/error data.

 **************************************************************************/

void ErrorExpected(INT32 ErrorCode, char sys_call[]) {
	if (ErrorCode == ERR_SUCCESS) {
		printf("An Error SHOULD have occurred.\n");
		printf("????: Error( %d ) occurred in case %d (%s)\n", ErrorCode,
				Z502_PROGRAM_COUNTER - 2, sys_call);
	} else
		printf("Program correctly returned an error: %d\n", ErrorCode);

}                      // End of ErrorExpected

void SuccessExpected(INT32 ErrorCode, char sys_call[]) {
	if (ErrorCode != ERR_SUCCESS) {
		printf("An Error should NOT have occurred.\n");
		printf("????: Error( %d ) occurred in case %d (%s)\n", ErrorCode,
				Z502_PROGRAM_COUNTER - 2, sys_call);
	} else
		printf("Program correctly returned success.\n");

}                      // End of SuccessExpected

/**************************************************************************
 Test2a exercises a simple memory write and read

 In global.h, there's a variable  DO_MEMORY_DEBUG.   Switching it to
 TRUE will allow you to see what the memory system thinks is happening.
 WARNING - it's verbose -- and I don't want to see such output - it's
 strictly for your debugging pleasure.
 **************************************************************************/
void test2a(void) {
	long OurProcessID;
	long MemoryAddress;
	long DataWritten;
	long DataRead;
	long ErrorReturned;

	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);

	printf("Release %s:Test 2a: Pid %ld\n", CURRENT_REL, OurProcessID);
	MemoryAddress = 412;
	DataWritten = MemoryAddress + OurProcessID;
	MEM_WRITE(MemoryAddress, &DataWritten);

	MEM_READ(MemoryAddress, &DataRead);

	printf("PID= %ld  address= %ld   written= %ld   read= %ld\n", OurProcessID,
			MemoryAddress, DataWritten, DataRead);
	if (DataRead != DataWritten)
		printf("AN ERROR HAS OCCURRED.\n");
	TERMINATE_PROCESS(-1, &ErrorReturned);

}                   // End of test2a   

/**************************************************************************
 Test2b

 Exercises simple memory writes and reads.  Watch out, the addresses 
 used are diabolical and are designed to show unusual features of your 
 memory management system.

 We do sanity checks - after each read/write pair, we will 
 read back the first set of data to make sure it's still there.

 **************************************************************************/

#define         TEST_DATA_SIZE          (INT16)7

void test2b(void) {
	long OurProcessID;
	long FirstMemoryAddress;
	long FirstDataWritten;
	long FirstDataRead;
	long MemoryAddress;
	long DataWritten;
	long DataRead;
	long ErrorReturned;
	INT16 TestDataIndex = 0;

	// This is an array containing the memory address we are accessing
	INT32 test_data[TEST_DATA_SIZE ] = { 0, 4, PGSIZE - 2, PGSIZE, 3 * PGSIZE
			- 2, (VIRTUAL_MEM_PAGES - 1) * PGSIZE, VIRTUAL_MEM_PAGES * PGSIZE
			- 2 };

	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
	printf("\n\nRelease %s:Test 2b: Pid %ld\n", CURRENT_REL, OurProcessID);

	// Try a simple memory write
	FirstMemoryAddress = 5 * PGSIZE;
	FirstDataWritten = FirstMemoryAddress + OurProcessID + 7;
	MEM_WRITE(FirstMemoryAddress, &FirstDataWritten);

	// Loop through all the memory addresses defined in the array
	while (TRUE ) {
		MemoryAddress = test_data[TestDataIndex];
		DataWritten = MemoryAddress + OurProcessID + 27;
		MEM_WRITE(MemoryAddress, &DataWritten);

		MEM_READ(MemoryAddress, &DataRead);

		printf("PID= %ld  address= %ld  written= %ld   read= %ld\n",
				OurProcessID, MemoryAddress, DataWritten, DataRead);
		if (DataRead != DataWritten)
			printf("AN ERROR HAS OCCURRED.\n");

		//      Go back and check earlier write
		MEM_READ(FirstMemoryAddress, &FirstDataRead);

		printf("PID= %ld  address= %ld   written= %ld   read= %ld\n",
				OurProcessID, FirstMemoryAddress, FirstDataWritten,
				FirstDataRead);
		if (FirstDataRead != FirstDataWritten)
			printf("AN ERROR HAS OCCURRED.\n");
		TestDataIndex++;
	}
}                            // End of test2b    

/**************************************************************************

 Test2c causes usage of disks.  The test is designed to give
 you a chance to develop a mechanism for handling disk requests.

 You will need a way to get the data read back from the disk into the
 buffer defined by the user process.  This can most easily be done after
 the process is rescheduled and about to return to user code.
 **************************************************************************/

#define         DISPLAY_GRANULARITY2c           10
#define         TEST2C_LOOPS                    50

typedef union {
	char char_data[PGSIZE ];
	UINT32 int_data[PGSIZE / sizeof(int)];
} DISK_DATA;

void test2c(void) {
	long OurProcessID;
	long CurrentTime;
	long ErrorReturned;

	DISK_DATA *DataWritten;     // A pointer to structures holding disk info
	DISK_DATA *DataRead;
	long DiskID;
	INT32 CheckValue = 1234;
	long Sector;
	int Iterations;

	DataWritten = (DISK_DATA *) calloc(1, sizeof(DISK_DATA));
	DataRead = (DISK_DATA *) calloc(1, sizeof(DISK_DATA));
	if (DataRead == 0)
		printf("Something screwed up allocating space in test2c\n");

	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);

	Sector = OurProcessID;
	printf("\n\nRelease %s:Test 2c: Pid %ld\n", CURRENT_REL, OurProcessID);

	for (Iterations = 0; Iterations < TEST2C_LOOPS; Iterations++) {
		// Pick some location on the disk to write to
		DiskID = (OurProcessID / 2) % MAX_NUMBER_OF_DISKS + 1;
		//Sector = ( Iterations + (Sector * 177)) % NUM_LOGICAL_SECTORS;
		Sector = OurProcessID + (Iterations * 17) % NUM_LOGICAL_SECTORS; // Bugfix 4.11
		DataWritten->int_data[0] = DiskID;
		DataWritten->int_data[1] = CheckValue;
		DataWritten->int_data[2] = Sector;
		DataWritten->int_data[3] = (int) OurProcessID;
		//printf( "Test2c - Pid = %d, Writing from Addr = %X\n", (int)OurProcessID, (unsigned int )(DataWritten->char_data));
		DISK_WRITE(DiskID, Sector, (char* )(DataWritten->char_data));

		// Now read back the same data.  Note that we assume the
		// DiskID and Sector have not been modified by the previous
		// call.
		DISK_READ(DiskID, Sector, (char* )(DataRead->char_data));

		if ((DataRead->int_data[0] != DataWritten->int_data[0])
				|| (DataRead->int_data[1] != DataWritten->int_data[1])
				|| (DataRead->int_data[2] != DataWritten->int_data[2])
				|| (DataRead->int_data[3] != DataWritten->int_data[3])) {
			printf("ERROR in Test 2c, Pid = %ld. Disk = %ld\n", OurProcessID,
					DiskID);
			printf("Written:  %d,  %d,  %d,  %d\n", DataWritten->int_data[0],
					DataWritten->int_data[1], DataWritten->int_data[2],
					DataWritten->int_data[3]);
			printf("Read:     %d,  %d,  %d,  %d\n", DataRead->int_data[0],
					DataRead->int_data[1], DataRead->int_data[2],
					DataRead->int_data[3]);
		} else if ((Iterations % DISPLAY_GRANULARITY2c) == 0) {
			printf(
					"TEST2C: PID = %ld: SUCCESS READING  DiskID =%ld, Sector = %ld\n",
					OurProcessID, DiskID, Sector);
		}
	}   // End of for loop

	// Now read back the data we've previously written and read

	printf("TEST2C: Reading back data: PID %ld.\n", OurProcessID);
	Sector = OurProcessID;

	for (Iterations = 0; Iterations < TEST2C_LOOPS; Iterations++) {
		DiskID = (OurProcessID / 2) % MAX_NUMBER_OF_DISKS + 1;
		//Sector = ( Iterations + (Sector * 177)) % NUM_LOGICAL_SECTORS;
		Sector = OurProcessID + (Iterations * 17) % NUM_LOGICAL_SECTORS; // Bugfix 4.11
		DataWritten->int_data[0] = DiskID;
		DataWritten->int_data[1] = CheckValue;
		DataWritten->int_data[2] = Sector;
		DataWritten->int_data[3] = OurProcessID;

		DISK_READ(DiskID, Sector, (char* )(DataRead->char_data));

		if ((DataRead->int_data[0] != DataWritten->int_data[0])
				|| (DataRead->int_data[1] != DataWritten->int_data[1])
				|| (DataRead->int_data[2] != DataWritten->int_data[2])
				|| (DataRead->int_data[3] != DataWritten->int_data[3])) {
			printf("AN ERROR HAS OCCURRED in Test 2c, Pid = %ld.\n",
					OurProcessID);
			printf("Written:  %d,  %d,  %d,  %d\n", DataWritten->int_data[0],
					DataWritten->int_data[1], DataWritten->int_data[2],
					DataWritten->int_data[3]);
			printf("Read:     %d,  %d,  %d,  %d\n", DataRead->int_data[0],
					DataRead->int_data[1], DataRead->int_data[2],
					DataRead->int_data[3]);
		} else if ((Iterations % DISPLAY_GRANULARITY2c) == 0) {
			printf(
					"TEST2C: PID = %ld: SUCCESS READING  DiskID =%ld, Sector = %ld\n",
					OurProcessID, DiskID, Sector);
		}

	}   // End of for loop

	GET_TIME_OF_DAY(&CurrentTime);
	printf("TEST2C:    PID %ld, Ends at Time %ld\n", OurProcessID, CurrentTime);
	TERMINATE_PROCESS(-1, &ErrorReturned);

}                                       // End of test2c    

/**************************************************************************

 Test2d runs several disk programs at a time.  The purpose here
 is to watch the scheduling that goes on for these
 various disk processes.  The behavior that should be seen
 is that the processes alternately run and do disk
 activity - there should always be someone running unless
 ALL processes happen to be waiting on the disk at some
 point.
 This program will terminate when all the test2c routines
 have finished.

 **************************************************************************/
#define           MOST_FAVORABLE_PRIORITY                       1

void test2d(void) {
	long OurProcessID;
	long ProcessID1;           // Created processes
	long CurrentTime;
	long ErrorReturned;
	long SleepTime = 1000;

	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
	printf("\n\nRelease %s:Test 2d: Pid %ld\n", CURRENT_REL, OurProcessID);
	CHANGE_PRIORITY(-1, MOST_FAVORABLE_PRIORITY, &ErrorReturned);

	CREATE_PROCESS("first", test2c, 5, &ProcessID1, &ErrorReturned);
	CREATE_PROCESS("second", test2c, 5, &ProcessID1, &ErrorReturned);
	CREATE_PROCESS("third", test2c, 5, &ProcessID1, &ErrorReturned);
	CREATE_PROCESS("fourth", test2c, 5, &ProcessID1, &ErrorReturned);
	CREATE_PROCESS("fifth", test2c, 5, &ProcessID1, &ErrorReturned);

	// In these next cases, we will loop until EACH of the child processes
	// has terminated.  We know it terminated because for a while we get
	// success on the call GET_PROCESS_ID, and then we get failure when the
	// process no longer exists.
	// We do this for each process so we can get decent statistics on completion

	ErrorReturned = ERR_SUCCESS;      // Modified 07/2014 Rev 4.10
	while (ErrorReturned == ERR_SUCCESS) {
		SLEEP(SleepTime);
		GET_PROCESS_ID("first", &ProcessID1, &ErrorReturned);
	}
	GET_TIME_OF_DAY(&CurrentTime);
	printf("Test2d, PID %ld, Found Process -first- completed at %ld\n\n",
			OurProcessID, CurrentTime);
	ErrorReturned = ERR_SUCCESS;
	while (ErrorReturned == ERR_SUCCESS) {
		SLEEP(SleepTime);
		GET_PROCESS_ID("second", &ProcessID1, &ErrorReturned);
	}
	GET_TIME_OF_DAY(&CurrentTime);
	printf("Test2d, PID %ld, Found Process -second- completed at %ld\n\n",
			OurProcessID, CurrentTime);
	ErrorReturned = ERR_SUCCESS;
	while (ErrorReturned == ERR_SUCCESS) {
		SLEEP(SleepTime);
		GET_PROCESS_ID("third", &ProcessID1, &ErrorReturned);
	}
	GET_TIME_OF_DAY(&CurrentTime);
	printf("Test2d, PID %ld, Found Process -third- completed at %ld\n\n",
			OurProcessID, CurrentTime);
	ErrorReturned = ERR_SUCCESS;
	while (ErrorReturned == ERR_SUCCESS) {
		SLEEP(SleepTime);
		GET_PROCESS_ID("fourth", &ProcessID1, &ErrorReturned);
	}
	GET_TIME_OF_DAY(&CurrentTime);
	printf("Test2d, PID %ld, Found Process -fourth- completed at %ld\n\n",
			OurProcessID, CurrentTime);
	ErrorReturned = ERR_SUCCESS;
	while (ErrorReturned == ERR_SUCCESS) {
		SLEEP(SleepTime);
		GET_PROCESS_ID("fifth", &ProcessID1, &ErrorReturned);
	}
	GET_TIME_OF_DAY(&CurrentTime);
	printf("Test2d, PID %ld, Found Process -fifth- completed at %ld\n\n",
			OurProcessID, CurrentTime);
	GET_TIME_OF_DAY(&CurrentTime);
	printf("Test2d, PID %ld, Ends at Time %ld\n\n", OurProcessID, CurrentTime);
	TERMINATE_PROCESS(-2, &ErrorReturned);

}                                   // End of test2d 

/**************************************************************************

 Test2e touches a number of logical pages - but the number of pages touched
 will fit into the physical memory available.
 The addresses accessed are pseudo-random distributed in a non-uniform manner.

 **************************************************************************/

#define         STEP_SIZE               VIRTUAL_MEM_PAGES/(4 * PHYS_MEM_PGS )
#define         DISPLAY_GRANULARITY2e   16 * STEP_SIZE
#define         TOTAL_ITERATIONS        256

void test2e(void) {
	long OurProcessID;
	long MemoryAddress;
	//long             DataWritten;
	//long             DataRead;
	int DataWritten;
	int DataRead;
	long CurrentTime;
	long ErrorReturned;

	int Iteration, MixItUp;
	int AddressesWritten[VIRTUAL_MEM_PAGES];

	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
	printf("\n\nRelease %s:Test 2e: Pid %ld\n", CURRENT_REL, OurProcessID);

	for (Iteration = 0; Iteration < TOTAL_ITERATIONS; Iteration++)
		AddressesWritten[Iteration] = 0;
	for (Iteration = 0; Iteration < TOTAL_ITERATIONS; Iteration++) {
		GetSkewedRandomNumber(&MemoryAddress, 128); // Generate Address    Bugfix 4.12
		MemoryAddress = 4 * (MemoryAddress / 4);     // Put address on mod 4 boundary
		AddressesWritten[Iteration] = MemoryAddress; // Keep record of location written
		DataWritten = PGSIZE * MemoryAddress;        // Generate Data    Bugfix 4.12
		MEM_WRITE(MemoryAddress, &DataWritten);      // Write the data

		MEM_READ(MemoryAddress, &DataRead); // Read back data

		if (Iteration % DISPLAY_GRANULARITY2e == 0)
			printf("PID= %ld  address= %6ld   written= %6d   read= %6d\n",
					OurProcessID, MemoryAddress, DataWritten, DataRead);
		if (DataRead != DataWritten) {  // Written = Read?
			printf("AN ERROR HAS OCCURRED ON 1ST READBACK.\n");
			printf("PID= %ld  address= %ld   written= %d   read= %d\n",
					OurProcessID, MemoryAddress, DataWritten, DataRead);
		}

		// It makes life more fun!! to write the data again
		MEM_WRITE(MemoryAddress, &DataWritten); // Write the data

	}    // End of for loop

	// Now read back the data we've written and paged
	// We try to jump around a bit in choosing addresses we read back
	printf("Reading back data: test 2e, PID %ld.\n", OurProcessID);

	for (Iteration = 0; Iteration < TOTAL_ITERATIONS; Iteration++) {
		if (!(Iteration % 2))      // Bugfix 4.11
			MixItUp = Iteration;
		else
			MixItUp = TOTAL_ITERATIONS - Iteration;
		MemoryAddress = AddressesWritten[MixItUp];     // Get location we wrote
		DataWritten = PGSIZE * MemoryAddress;    // Generate Data    Bugfix 4.12
		MEM_READ(MemoryAddress, &DataRead);      // Read back data

		if (Iteration % DISPLAY_GRANULARITY2e == 0)
			printf("PID= %ld  address= %6ld   written= %6d   read= %6d\n",
					OurProcessID, MemoryAddress, DataWritten, DataRead);
		if (DataRead != DataWritten) {  // Written = Read?
			printf("AN ERROR HAS OCCURRED ON 2ND READBACK.\n");
			printf("PID= %ld  address= %ld   written= %d   read= %d\n",
					OurProcessID, MemoryAddress, DataWritten, DataRead);
		}
	}    // End of for loop
	GET_TIME_OF_DAY(&CurrentTime);
	printf("TEST2e:    PID %ld, Ends at Time %ld\n", OurProcessID, CurrentTime);
	TERMINATE_PROCESS(-2, &ErrorReturned);      // Added 12/1/2013
}                                           // End of test2e    

/**************************************************************************

 Test2f causes extensive page replacement, but reuses pages.
 This program will terminate, but it might take a while.

 **************************************************************************/

#define                 LOOP_COUNT                    4000
#define                 DISPLAY_GRANULARITY2          100
#define                 LOGICAL_PAGES_TO_TOUCH       2 * PHYS_MEM_PGS

// This structure keeps track of which addresses have been touched.
// We're accessing random sparse locations, so not all memory is accessed.
// Indeterminate results occur if we read memory that hasn't been written.
typedef struct {
	INT16 page_touched[LOOP_COUNT];   // Bugfix Rel 4.03  12/1/2013
} MEMORY_TOUCHED_RECORD;

void test2f(void) {
	long OurProcessID;
	long PageNumber;
	long MemoryAddress;
	long DataWritten;
	long DataRead;
	long ErrorReturned;
        long CurrentTime;
	MEMORY_TOUCHED_RECORD *mtr;
	long Index, Loops;

	mtr = (MEMORY_TOUCHED_RECORD *) calloc(1, sizeof(MEMORY_TOUCHED_RECORD));

	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
	printf("\n\nRelease %s:Test 2f: Pid %ld\n", CURRENT_REL, OurProcessID);

	for (Index = 0; Index < LOOP_COUNT; Index++) // Bugfix Rel 4.03  12/1/2013
		mtr->page_touched[Index] = 0;
	for (Loops = 0; Loops < LOOP_COUNT; Loops++) {
		// Get a random page number
		GetSkewedRandomNumber(&PageNumber, LOGICAL_PAGES_TO_TOUCH);
		MemoryAddress = PGSIZE * PageNumber; // Convert page to addr.
		DataWritten = MemoryAddress + OurProcessID; // Generate data for page
		MEM_WRITE(MemoryAddress, &DataWritten);
		// Write it again, just as a test
		MEM_WRITE(MemoryAddress, &DataWritten);

		// Read it back and make sure it's the same
		MEM_READ(MemoryAddress, &DataRead);
		if (Loops % DISPLAY_GRANULARITY2 == 0)
			printf("PID= %ld  address= %ld   written= %ld   read= %ld\n",
				OurProcessID, MemoryAddress, DataWritten, DataRead);
		if (DataRead != DataWritten)
			printf("AN ERROR HAS OCCURRED: READ NOT EQUAL WRITE.\n");


		// Record in our data-base that we've accessed this page
		mtr->page_touched[(short) Loops] = PageNumber;
		Test2f_Statistics(OurProcessID);

	}   // End of for Loops

	for (Loops = 0; Loops < LOOP_COUNT; Loops++) {

		// We can only read back from pages we've previously
		// written to, so find out which pages those are.
		PageNumber = mtr->page_touched[(short) Loops];
		MemoryAddress = PGSIZE * PageNumber;        // Convert page to addr.
		DataWritten = MemoryAddress + OurProcessID; // Expected read
		MEM_READ(MemoryAddress, &DataRead);
		Test2f_Statistics(OurProcessID);

		if (Loops % DISPLAY_GRANULARITY2 == 0)
			printf("PID= %ld  address= %ld   written= %ld   read= %ld\n",
						OurProcessID, MemoryAddress, DataWritten, DataRead);
		if (DataRead != DataWritten)
			printf("ERROR HAS OCCURRED: READ NOT SAME AS WRITE.\n");

	}   // End of for Loops

	// We've completed reading back everything
	printf("TEST 2f, PID %ld, HAS COMPLETED %ld Loops\n", OurProcessID,
				Loops);

	GET_TIME_OF_DAY(&CurrentTime);
	printf("TEST2f:    PID %ld, Ends at Time %ld\n", OurProcessID, CurrentTime);
	TERMINATE_PROCESS(-1, &ErrorReturned);

}                                 // End of test2f

/**************************************************************************
 Test2g

 Tests multiple copies of test2f running simultaneously.
 Test2f runs these with the same priority in order to show
 equal preference for each child process.  This means all the
 child processes will be stealing memory from each other.

 WARNING:  This test assumes tests 2e - 2f run successfully

 **************************************************************************/

#define         PRIORITY2G              10

void test2g(void) {
        long OurProcessID;
	long ProcessID1;           // Created processes
	long ProcessID2;
	long ProcessID3;
	long ProcessID4;
	long ProcessID5;
	long ErrorReturned;
	long SleepTime = 10000;
        long CurrentTime;

	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
	printf("\n\nRelease %s:Test 2g: Pid %ld\n", CURRENT_REL, OurProcessID);
	CREATE_PROCESS("test2g_a", test2f, PRIORITY2G, &ProcessID1, &ErrorReturned);
	CREATE_PROCESS("test2g_b", test2f, PRIORITY2G, &ProcessID2, &ErrorReturned);
	CREATE_PROCESS("test2g_c", test2f, PRIORITY2G, &ProcessID3, &ErrorReturned);
	CREATE_PROCESS("test2g_d", test2f, PRIORITY2G, &ProcessID4, &ErrorReturned);
	CREATE_PROCESS("test2g_e", test2f, PRIORITY2G, &ProcessID5, &ErrorReturned);
	SuccessExpected(ErrorReturned, "CREATE_PROCESS");

	// In these next cases, we will loop until EACH of the child processes
	// has terminated.  We know it terminated because for a while we get
	// success on the call GET_PROCESS_ID, and then we get failure when the
	// process no longer exists.
	// We do this for each process so we can get decent statistics on completion

	ErrorReturned = ERR_SUCCESS;      // Modified 12/2013 Rev 4.10
	while (ErrorReturned == ERR_SUCCESS) {
		SLEEP(SleepTime);
		GET_PROCESS_ID("test2g_a", &ProcessID1, &ErrorReturned);
	}
	ErrorReturned = ERR_SUCCESS;
	while (ErrorReturned == ERR_SUCCESS) {
		SLEEP(SleepTime);
		GET_PROCESS_ID("test2g_b", &ProcessID2, &ErrorReturned);
	}
	ErrorReturned = ERR_SUCCESS;
	while (ErrorReturned == ERR_SUCCESS) {
		SLEEP(SleepTime);
		GET_PROCESS_ID("test2g_c", &ProcessID3, &ErrorReturned);
	}
	ErrorReturned = ERR_SUCCESS;
	while (ErrorReturned == ERR_SUCCESS) {
		SLEEP(SleepTime);
		GET_PROCESS_ID("test2g_d", &ProcessID4, &ErrorReturned);
	}
	ErrorReturned = ERR_SUCCESS;
	while (ErrorReturned == ERR_SUCCESS) {
		SLEEP(SleepTime);
		GET_PROCESS_ID("test2g_e", &ProcessID5, &ErrorReturned);
	}
	GET_TIME_OF_DAY(&CurrentTime);
	printf("TEST2g:    PID %ld, Ends at Time %ld\n", OurProcessID, CurrentTime);
	TERMINATE_PROCESS(-2, &ErrorReturned); // Terminate all

}            // End test2g

/**************************************************************************

 Test2h starts up a number of processes who do tests of shared area.

 This process doesn't do much - the real action is in test2hx
 **************************************************************************/

#define           MOST_FAVORABLE_PRIORITY                       1
#define           NUMBER_2HX_PROCESSES                          5
#define           SLEEP_TIME_2H                             10000

void test2h(void) {
	long OurProcessID;
	long ProcessID1;           // Created processes
	long ErrorReturned;
        long CurrentTime;

	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
	printf("\n\nRelease %s:Test 2h: Pid %ld\n", CURRENT_REL, OurProcessID);
	CHANGE_PRIORITY(-1, MOST_FAVORABLE_PRIORITY, &ErrorReturned);

	CREATE_PROCESS("first", test2hx, 5, &ProcessID1, &ErrorReturned);
	CREATE_PROCESS("second", test2hx, 6, &ProcessID1, &ErrorReturned);
	CREATE_PROCESS("third", test2hx, 7, &ProcessID1, &ErrorReturned);
	CREATE_PROCESS("fourth", test2hx, 8, &ProcessID1, &ErrorReturned);
	CREATE_PROCESS("fifth", test2hx, 9, &ProcessID1, &ErrorReturned);

	// Loop here until the "2hx" final process terminate.

	ErrorReturned = ERR_SUCCESS;
	while (ErrorReturned == ERR_SUCCESS) {
		SLEEP(SLEEP_TIME_2H);
		GET_PROCESS_ID("first", &ProcessID1, &ErrorReturned);
	}
	GET_TIME_OF_DAY(&CurrentTime);
	printf("TEST2h:    PID %ld, Ends at Time %ld\n", OurProcessID, CurrentTime);
	TERMINATE_PROCESS(-2, &ErrorReturned);

}                                  // End of test2h

/**************************************************************************

 Test2hx - test shared memory usage.

 This test runs as multiple instances of processes; there are several
 processes who in turn manipulate shared memory.

 The algorithm used here flows as follows:

 o Get our PID and print it out.
 o Use our PID to determine the address at which to start shared
 area - every process will have a different starting address.
 o Define the shared area.
 o Fill in initial portions of the shared area by:
 + No locking required because we're modifying only our portion of
 the shared area.
 + Determine which location within shared area is ours by using the
 Shared ID returned by the DEFINE_SHARED_AREA.  The OS will return
 a unique ID for each caller.
 + Fill in portions of the shared area.
 o Sleep to let all 2hx PIDs start up.

 o If (shared_index == 0)   This is the MASTER Process

 o LOOP many times doing the following steps:
 + We DON'T need to lock the shared area since the SEND/RECEIVE
 act as synchronization.
 + Select a target process
 + Communicate back and forth with the target process.
 + SEND_MESSAGE( "next", ..... );
 END OF LOOP
 o Loop through each Slave process telling them to terminate

 o If (shared_index > 0)   This is a SLAVE Process
 o LOOP Many times until we get terminate message
 + RECEIVE_MESSAGE( "-1", ..... )
 + Read my mailbox and communicate with master
 + Print out lots of stuff
 + Do lots of sanity checks
 + If MASTER tells us to terminate, do so.
 END OF LOOP

 o If (shared_index == 0)   This is the MASTER Process
 o sleep                      Until everyone is done
 o print the whole shared structure

 o Terminate the process.

 **************************************************************************/

#define           NUMBER_MASTER_ITERATIONS    28
#define           PROC_INFO_STRUCT_TAG        1234
#define           SHARED_MEM_NAME             "almost_done!!\0"

// This is the per-process area for each of the processes participating
// in the shared memory.
typedef struct {
	INT32 structure_tag;      // Unique Tag so we know everything is right
	INT32 Pid;                // The PID of the slave owning this area
	INT32 TerminationRequest; // If non-zero, process should terminate.
	INT32 MailboxToMaster;    // Data sent from Slave to Master
	INT32 MailboxToSlave;     // Data sent from Master To Slave
	INT32 WriterOfMailbox;    // PID of process who last wrote in this area
} PROCESS_INFO;

// The following structure will be laid on shared memory by using
// the MEM_ADJUST   macro                                          

typedef struct {
	PROCESS_INFO proc_info[NUMBER_2HX_PROCESSES + 1];
} SHARED_DATA;

// We use this local structure to gather together the information we
// have for this process.
typedef struct {
	INT32 StartingAddressOfSharedArea;     // Local Virtual Address
	INT32 PagesInSharedArea;               // Size of shared area
	// How OS knows to put all in same shared area
	char AreaTag[32];
	// Unique number supplied by OS. First must be 0 and the
	// return values must be monotonically increasing for each
	// process that's doing the Shared Memory request.
	INT32 OurSharedID;
	INT32 TargetShared;                   // Shared ID of slave we're sending to
	long TargetPid;                      // Pid of slave we're sending to
	INT32 ErrorReturned;

	long SourcePid;
	char ReceiveBuffer[20];
	long MessageReceiveLength;
	INT32 MessageSendLength;
	INT32 MessageSenderPid;
} LOCAL_DATA;

// This MEM_ADJUST macro allows us to overlay the SHARED_DATA structure
// onto the shared memory we've defined.  It generates an address
// appropriate for use by READ and MEM_WRITE.

#define         MEM_ADJUST( arg )                                   \
  (long)&(shared_ptr->arg) - (long)(shared_ptr)                     \
                      + (long)ld->StartingAddressOfSharedArea

#define         MEM_ADJUST2( shared, local, arg )                   \
  (long)&(shared->arg) - (long)(shared)                             \
                      + (long)local->StartingAddressOfSharedArea

// This allows us to print out the shared memory for debugging purposes
void PrintTest2hMemory(SHARED_DATA *sp, LOCAL_DATA *ld) {
	int Index;
	INT32 Data1, Data2, Data3, Data4, Data5;

	printf("\nNumber of Masters + Slaves = %d\n", (int) NUMBER_2HX_PROCESSES);

	for (Index = 0; Index < NUMBER_2HX_PROCESSES; Index++) {
		MEM_READ(MEM_ADJUST2( sp, ld, proc_info[ Index ].structure_tag),
				&Data1);
		MEM_READ(MEM_ADJUST2( sp, ld, proc_info[ Index ].Pid ), &Data2);
		MEM_READ(MEM_ADJUST2( sp, ld, proc_info[ Index ].MailboxToMaster ),
				&Data3);
		MEM_READ(MEM_ADJUST2( sp, ld, proc_info[ Index ].MailboxToSlave ),
				&Data4);
		MEM_READ(MEM_ADJUST2( sp, ld, proc_info[ Index ].WriterOfMailbox),
				&Data5);

		printf("Mailbox info for index %d:\n", Index);
		printf("\tIndex = %d, Struct Tag = %d,  ", Index, Data1);
		printf(" Pid =  %d,  Mail To Master = %d, ", Data2, Data3);
		printf(" Mail To Slave =  %d,  Writer Of Mailbox = %d\n", Data4, Data5);
	}          // END of for Index
}      // End of PrintTest2hMemory

void test2hx(void) {
	// The declaration of shared_ptr is only for use by MEM_ADJUST macro.
	// It points to a bogus location - but that's ok because we never
	// actually use the result of the pointer, only its offset.

	long OurProcessID;
	INT32 ErrorReturned;
	LOCAL_DATA *ld;
	SHARED_DATA *shared_ptr = 0;
	int Index;
	INT32 ReadWriteData;    // Used to move to and from shared memory

	ld = (LOCAL_DATA *) calloc(1, sizeof(LOCAL_DATA));
	if (ld == 0) {
		printf("Unable to allocate memory in test2hx\n");
	}
	strcpy(ld->AreaTag, SHARED_MEM_NAME);

	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
	printf("\n\nRelease %s:Test 2hx: Pid %ld\n", CURRENT_REL, OurProcessID);

	// As an interesting wrinkle, each process should start
	// its shared region at a somewhat different virtual address;
	// determine that here.
	ld->StartingAddressOfSharedArea = (OurProcessID % 17) * PGSIZE;

	// This is the number of pages required in the shared area.
	ld->PagesInSharedArea = sizeof(SHARED_DATA) / PGSIZE + 1;

	// Now ask the OS to map us into the shared area
	DEFINE_SHARED_AREA((long )ld->StartingAddressOfSharedArea, // Input - our virtual address
			(long)ld->PagesInSharedArea,// Input - pages to map
			(long)ld->AreaTag,// Input - ID for this shared area
			&ld->OurSharedID,// Output - Unique shared ID
			&ld->ErrorReturned);              // Output - any error
	SuccessExpected(ld->ErrorReturned, "DEFINE_SHARED_AREA");

	ReadWriteData = PROC_INFO_STRUCT_TAG; // Sanity data
	MEM_WRITE(MEM_ADJUST(proc_info[ld->OurSharedID].structure_tag),
			&ReadWriteData);

	ReadWriteData = OurProcessID; // Store PID in our slot
	MEM_WRITE(MEM_ADJUST(proc_info[ld->OurSharedID].Pid), &ReadWriteData);
	ReadWriteData = 0;         // Initialize this counter
	MEM_WRITE(MEM_ADJUST(proc_info[ld->OurSharedID].MailboxToMaster),
			&ReadWriteData);
	ReadWriteData = 0;         // Initialize this counter
	MEM_WRITE(MEM_ADJUST(proc_info[ld->OurSharedID].MailboxToSlave),
			&ReadWriteData);

	//  This is the code used ONLY by the MASTER Process
	if (ld->OurSharedID == 0) {  //   We are the MASTER Process
		// Loop here the required number of times
		for (Index = 0; Index < NUMBER_MASTER_ITERATIONS; Index++) {

			// Wait for all slaves to start up - we assume after the sleep
			// that the slaves are no longer modifying their shared areas.
			SLEEP(1000); // Wait for slaves to start

			// Get slave ID we're going to work with - be careful here - the
			// code further on depends on THIS algorithm
			ld->TargetShared = (Index % (NUMBER_2HX_PROCESSES - 1)) + 1;

			// Read the memory of that slave to make sure it's OK
			MEM_READ(MEM_ADJUST(proc_info[ld->TargetShared].structure_tag),
					&ReadWriteData);
			if (ReadWriteData != PROC_INFO_STRUCT_TAG) {
				printf("We should see a structure tag, but did not\n");
				printf("This means that this memory is not mapped \n");
				printf("consistent with the memory used by the writer\n");
				printf("of this structure.  It's a page table problem.\n");
			}
			// Get the pid of the process we're working with
			MEM_READ(MEM_ADJUST(proc_info[ld->TargetShared].Pid),
					&ld->TargetPid);

			// We're sending data to the Slave
			MEM_WRITE(MEM_ADJUST(proc_info[ld->TargetShared].MailboxToSlave),
					&Index);
			MEM_WRITE(MEM_ADJUST(proc_info[ld->TargetShared].WriterOfMailbox),
					&OurProcessID);
			ReadWriteData = 0;   // Do NOT terminate
			MEM_WRITE(
					MEM_ADJUST(proc_info[ld->TargetShared].TerminationRequest),
					&ReadWriteData);
			printf("Sender %ld to Receiver %d passing data %d\n", OurProcessID,
					(int) ld->TargetPid, Index);

			// Check the iteration count of the slave.  If it tells us it has done a
			// certain number of iterations, then tell it to terminate itself.
			MEM_READ(MEM_ADJUST(proc_info[ld->TargetShared].MailboxToMaster),
					&ReadWriteData);
			if (ReadWriteData
					>= (NUMBER_MASTER_ITERATIONS / (NUMBER_2HX_PROCESSES - 1))
							- 1) {
				ReadWriteData = 1;   // Do terminate
				MEM_WRITE(
						MEM_ADJUST(proc_info[ld->TargetShared].TerminationRequest),
						&ReadWriteData);
				printf("Master is sending termination message to PID %d\n",
						(int) ld->TargetPid);
			}

			// Now we are done with this slave - send it a message which will start it working.
			// The iterations may not be quite right - we may be sending a message to a
			// process that's already terminated, but that's OK
			SEND_MESSAGE(ld->TargetPid, " ", 1, &ld->ErrorReturned);   // Bugfix, July 2015
		}     // End of For Index
	}     // End of MASTER PROCESS

	// This is the start of the slave process work
	if (ld->OurSharedID != 0) {  //   We are a SLAVE Process
		// The slaves keep going forever until the master tells them to quit
		while (TRUE ) {

			ld->SourcePid = -1; // From anyone
			ld->MessageReceiveLength = 20;
			RECEIVE_MESSAGE(ld->SourcePid, ld->ReceiveBuffer,
					ld->MessageReceiveLength, &ld->MessageSendLength,
					&ld->MessageSenderPid, &ld->ErrorReturned);
			SuccessExpected(ld->ErrorReturned, "RECEIVE_MESSAGE");

			// Make sure we have our memory mapped correctly
			MEM_READ(MEM_ADJUST(proc_info[ld->OurSharedID].structure_tag),
					&ReadWriteData);
			if (ReadWriteData != PROC_INFO_STRUCT_TAG) {
				printf("We should see a structure tag, but did not.\n");
				printf("This means that this memory is not mapped \n");
				printf("consistent with the memory used when WE wrote\n");
				printf("this structure.  It's a page table problem.\n");
			}

			// Get the value placed in shared memory and compare it with the PID provided
			// by the messaging system.
			MEM_READ(MEM_ADJUST(proc_info[ld->OurSharedID].WriterOfMailbox),
					&ReadWriteData);
			if (ReadWriteData != ld->MessageSenderPid) {
				printf("ERROR: ERROR: The sender PID, given by the \n");
				printf("RECEIVE_MESSAGE and by the mailbox, don't match\n");
			}

			// We're receiving data from the Master
			MEM_READ(MEM_ADJUST(proc_info[ld->OurSharedID].MailboxToSlave),
					&ReadWriteData);
			MEM_READ(MEM_ADJUST(proc_info[ld->OurSharedID].WriterOfMailbox),
					&ld->MessageSenderPid);
			printf("Receiver %ld got message from %d passing data %d\n",
					OurProcessID, ld->MessageSenderPid, ReadWriteData);

			// See if we've been asked to terminate
			MEM_READ(MEM_ADJUST(proc_info[ld->OurSharedID].TerminationRequest),
					&ReadWriteData);
			if (ReadWriteData > 0) {
				printf("Process %ld received termination message\n",
						OurProcessID);
				TERMINATE_PROCESS(-1, &ErrorReturned);
			}

			// Increment the number of iterations we've done.  This will ultimately lead
			// to the master telling us to terminate.
			MEM_READ(MEM_ADJUST(proc_info[ld->OurSharedID].MailboxToMaster),
					&ReadWriteData);
			ReadWriteData++;
			MEM_WRITE(MEM_ADJUST(proc_info[ld->OurSharedID].MailboxToMaster),
					&ReadWriteData);

		}  //End of while TRUE
	}      // End of SLAVE

	// The Master comes here and prints out the entire shared area

	if (ld->OurSharedID == 0) {      // The slaves should terminate before this.
		SLEEP(5000);                        // Wait for msgs to finish
		printf("Overview of shared area at completion of Test2h\n");
		PrintTest2hMemory(shared_ptr, ld);
		TERMINATE_PROCESS(-1, &ErrorReturned);
	}              // END of if
	TERMINATE_PROCESS(-2, &ErrorReturned);

}                                // End of test2hx   
/**************************************************************************

 test2f_Statistics   This is designed to give an overview of how the
 paging algorithms are working.  It is especially useful when running
 test2g because it will enable us to better understand the global policy
 being used for allocating pages.

 **************************************************************************/

#define  MAX2F_PID    20
#define  REPORT_GRANULARITY_2F     100
void Test2f_Statistics(int Pid) {
	short NotInitialized = TRUE;
	int PagesTouched[MAX2F_PID];
	int i;
	int LowestReportingPid = -1;

	if (NotInitialized) {
		for (i = 0; i < MAX2F_PID; i++)
			PagesTouched[i] = 0;
		NotInitialized = FALSE;
	}
	if (Pid >= MAX2F_PID) {
		printf("In Test2f_Statistics - the pid entered, ");
		printf("%d, is larger than the maximum allowed", Pid);
		exit(0);
	}
	PagesTouched[Pid]++;
	// It's time to print out a report
	if (PagesTouched[Pid] % REPORT_GRANULARITY_2F == 0) {
		i = 0;
		while (PagesTouched[i] == 0)
			i++;
		LowestReportingPid = i;
		if (Pid == LowestReportingPid) {
			printf("----- Report by test2f - Pid %d -----\n", Pid);
			for (i = 0; i < MAX2F_PID; i++) {
				if (PagesTouched[i] > 0)
					printf("Pid = %d, Pages Touched = %d\n", i,
							PagesTouched[i]);
			}
		}
	}
}                 // End of Test2f_Statistics
/**************************************************************************

 GetSkewedRandomNumber   Is a homegrown deterministic random
 number generator.  It produces  numbers that are NOT uniform across
 the allowed range.
 This is useful in picking page locations so that pages
 get reused and makes a LRU algorithm meaningful.
 This algorithm is VERY good for developing page replacement tests.
 July 2015 - optimized values so as to differentiate between random 
             replacement and LRU replacement.  I saw a 10% improvement
             in fault rates when using LRU as compared to random replacement.
 **************************************************************************/

#define                 SKEWING_FACTOR          0.30
void GetSkewedRandomNumber(long *random_number, long range) {
	double temp, temp2, temp3;
	long extended_range = (long) pow(range, (double) (1 / SKEWING_FACTOR));
	double multiplier = (double) extended_range / (double) RAND_MAX;

	temp = multiplier * (double) rand();
	if (temp < 0)
		temp = -temp;
	temp2 = (double) ((long) temp % extended_range);
	temp3 = pow(temp2, (double) SKEWING_FACTOR);
	*random_number = (long) temp3;
	//printf( "GSRN RAND_MAX Extended, temp, temp2, temp3 %6d %6ld  %6d  %6d  %6d\n",
	//	   RAND_MAX, extended_range, (int)temp, (int)temp2, (int)temp3 );
} // End GetSkewedRandomNumber 

void GetRandomNumber(long *random_number, long range) {

	*random_number = (long) rand() % range;
} // End GetRandomNumber 

/*****************************************************************
 testStartCode()
 A new thread (other than the initial thread) comes here the
 first time it's scheduled.
 *****************************************************************/
void testStartCode() {
	void (*routine)(void);
	routine = (void (*)(void)) Z502PrepareProcessForExecution();
	(*routine)();
	// If we ever get here, it's because the thread ran to the end
	// of a test program and wasn't terminated properly.
	printf("ERROR:  Simulation did not end correctly\n");
	exit(0);
}

/*****************************************************************
 main()
 This is the routine that will start running when the
 simulator is invoked.
 *****************************************************************/
int main(int argc, char *argv[]) {
	int i;
	for (i = 0; i < MAX_NUMBER_OF_USER_THREADS; i++) {
		Z502CreateUserThread(testStartCode);
	}

	osInit(argc, argv);
	// We should NEVER return from this routine.  The result of
	// osInit is to select a program to run, to start a process
	// to execute that program, and NEVER RETURN!
	return (-1);
}    // End of main

