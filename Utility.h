/******************************  Find  *************************************/
int findProcessNameInTable(char* ProcessName);
int findPCBIDbyName(char* ProcessName);
struct Process_Control_Block *findPCBbyProcessName(char* ProcessName);
struct Process_Control_Block *findPCBbyProcessID(int ProcessID);
/***************************************************************************/


/**************************  Print  State  *********************************/
void PrintPIDinReadyQueue();
void PrintPIDinTimerQueue();
void PrintCurrentPID();
void PrintPIDinPCBTable();
void PrintCurrentState();
/***************************************************************************/