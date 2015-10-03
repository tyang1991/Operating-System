void ResetTimer();
long CurrentTime();
void OSCreateProcess(long *ProcessName, long *Test_To_Run, long *priority, long *ProcessID, long *ErrorReturned);
void OSStartProcess(struct Process_Control_Block* PCB);
void Dispatcher();
void SuspendProcess();
void IdleProcess();
void HaltProcess();
void TerminateCurrentProcess();
void PrintPIDinReadyQueue();
void PrintPIDinTimerQueue();
void PrintCurrentPID();
void PrintPIDinPCBTable();