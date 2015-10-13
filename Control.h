/*******************************Current State*********************************/
long CurrentTime();
long CurrentContext();
struct Process_Control_Block *CurrentPCB();
int CurrentPID();
/*****************************************************************************/

/******************************Process Controle*******************************/
void Dispatcher();
void OSStartProcess_Only(struct Process_Control_Block* PCB);
void OSSuspendCurrentProcess();
struct Process_Control_Block *OSCreateProcess(long *ProcessName, long *Test_To_Run, long *priority, long *ProcessID, long *ErrorReturned);
void OSStartProcess(struct Process_Control_Block* PCB);
void SuspendProcess();
void IdleProcess();
void HaltProcess();
void TerminateProcess(struct Process_Control_Block *PCB);
void SuspendProcess(struct Process_Control_Block *PCB);
void ResumeProcess(struct Process_Control_Block *PCB);
/*****************************************************************************/

/******************************Timer Control**********************************/
void SetTimer(long SleepTime);
int ResetTimer();
void lockTimer();
void unlockTimer();
/*****************************************************************************/