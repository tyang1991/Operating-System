/*******************************Current State*********************************/
long CurrentTime();
long CurrentContext();
struct Process_Control_Block *CurrentPCB();
int CurrentPID();
/*****************************************************************************/

/******************************Process Controle*******************************/
#define Uniprocessor 1L
#define Multiprocessor 2L
//this value is used to set processor mode
long ProcessorMode;

void Dispatcher();
void Dispatcher_Uniprocessor();
void Dispatcher_Multiprocessor();
void OSStartProcess_Only(struct Process_Control_Block* PCB);
void OSSuspendCurrentProcess();
void OSTerminateCurrentProcess_Only();
struct Process_Control_Block *OSCreateProcess(long *ProcessName, long *Test_To_Run, long *priority, long *ProcessID, long *ErrorReturned);
void OSStartProcess(struct Process_Control_Block* PCB);
void SuspendProcess();
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

/******************************Memory Control*********************************/
int NewPTNumber;
int victimCheckingPosition;

void initMemory();
int NewFrameNumber();
int GetFreeFrameNumber();
int FrameIsFull();
void StartDiskOp(struct DISK_OP *DiskOp);
void DiskWrite(INT16 DiskID, INT16 Sector, char *DataWritten);
void DiskRead(INT16 DiskID, INT16 Sector, char *DataRead);
int DiskStatus(INT16 DiskID);
void ClearInterruptStatus(long DeviceID);
INT16 GetFreeDiskAddress(int PID, INT16 pageNumber);
struct Frame_Map *GetVictimFrame();
/*****************************************************************************/