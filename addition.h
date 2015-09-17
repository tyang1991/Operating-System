#include             "global.h"

typedef struct  {
	INT32        Name;
	INT32        ProcessID;
	long*        Context;
	INT32        WakeUpTime;
} Process_Control_Block;