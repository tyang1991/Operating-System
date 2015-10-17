#include "global.h"
#include "syscalls.h"

#define         PRIORITY1C              10
void test1xx(void);

void test1myTest(void){
	long ProcessID1;            // Contains a Process ID
	long ProcessID2;            // Contains a Process ID
	long SleepTime = 1000;
	long ReturnedPID;           // Value of PID returned by System Call
	long ErrorReturned;

	printf("This is Release %s:  MyTest\n", CURRENT_REL);

	CREATE_PROCESS("test1xx", test1xx, PRIORITY1C, &ProcessID1, &ErrorReturned);

	for (int i = 0; i < 3; i++){
		SLEEP(SleepTime);
	}

//	RESTART(ProcessID1, &ProcessID2, &ErrorReturned);

	while (ErrorReturned == ERR_SUCCESS) {
		SLEEP(SleepTime);
		GET_PROCESS_ID("test1xx", &ReturnedPID, &ErrorReturned);
	}
	TERMINATE_PROCESS(-2, &ErrorReturned); /* Terminate all */

}

#define         NUMBER_OF_TEST_ITERATIONS     20

void test1xx(void) {
	long OurProcessID;
	long RandomSeed;
	long EndingTime;
	long RandomSleep = 17;
	long ErrorReturned;
	int Iterations;

	GET_PROCESS_ID("", &OurProcessID, &ErrorReturned);
	printf("Release %s:Test 1x: Pid %ld\n", CURRENT_REL, OurProcessID);
	long t;
	for (Iterations = 0; Iterations < NUMBER_OF_TEST_ITERATIONS;
		Iterations++) {
		GET_TIME_OF_DAY(&RandomSeed);
		RandomSleep = (RandomSleep * RandomSeed) % 143;
		SLEEP(RandomSleep);
		GET_TIME_OF_DAY(&EndingTime);

		printf("Test1XX: Pid = %d, Wake Up for: %d times\n",
			(int)OurProcessID, RandomSleep,
			(int)(Iterations+1));
	}
	//	printf("Test1x, PID %ld, Ends at Time %ld\n", OurProcessID, EndingTime);
	GET_TIME_OF_DAY(&t);

	printf("%d. Test1x, PID %ld, Ends at Time %ld\n", t, OurProcessID, EndingTime);

	TERMINATE_PROCESS(-1, &ErrorReturned);
	printf("ERROR: Test1x should be terminated but isn't.\n");
}                                                  // End of test1x 
