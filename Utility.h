/******************************  Find  *************************************/
int findProcessNameInTable(char* ProcessName);
int findPCBIDbyName(char* ProcessName);
struct Process_Control_Block *findPCBbyProcessName(char* ProcessName);
struct Process_Control_Block *findPCBbyProcessID(int ProcessID);
struct Process_Control_Block *findPCBbyContextID(long ContextID);
/***************************************************************************/


/**************************  Print  State  *********************************/
void PrintPIDinReadyQueue();
void PrintPIDinTimerQueue();
void PrintCurrentPID();
void PrintPIDinPCBTable();
void PrintCurrentState();
/***************************************************************************/

void SchedularPrinter(char *TargetAction, int TargetPID);