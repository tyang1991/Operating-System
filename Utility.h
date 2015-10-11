/******************************  Find  *************************************/
int findPCBIDbyName(char* ProcessName);
struct Process_Control_Block *findPCBbyProcessName(char* ProcessName);
struct Process_Control_Block *findPCBbyProcessID(int ProcessID);
struct Process_Control_Block *findPCBbyContextID(long ContextID);
/***************************************************************************/

void SchedularPrinter(char *TargetAction, int TargetPID);