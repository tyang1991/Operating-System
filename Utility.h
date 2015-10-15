/******************************  Find  *************************************/
struct Process_Control_Block *findPCBbyProcessName(char* ProcessName);
struct Process_Control_Block *findPCBbyProcessID(int ProcessID);
struct Process_Control_Block *findPCBbyContextID(long ContextID);
/***************************************************************************/

/****************************Scheduler Printer******************************/
void lockSP();
void unlockSP();
void SchedularPrinter(char *TargetAction, int TargetPID);
/***************************************************************************/

/********************************Test Parser********************************/
long *TestParser(char *TestInput);
/***************************************************************************/