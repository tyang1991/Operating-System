#include "queue.h"
#include "stdio.h"

struct Timer_Queue timerQueue;
struct PCB_Hub pcbHub;

void initTimerQueue(){
	timerQueue.Element_Number = 0;
}

void enTimerQueue(struct Process_Control_Block *PCB, long wakeUpTime){
	struct Timer_Queue_Element newElement;
	newElement.WakeUpTime = wakeUpTime;
	newElement.PCB = PCB;
	
	if (timerQueue.Element_Number == 0){
		//build timer queue element
		newElement.Prev_Element = &newElement;
		newElement.Next_Element = &newElement;
		//make change in timer queue
		timerQueue.Element_Number = 1;
		timerQueue.First_Element = &newElement;
	}
	else{
		struct Timer_Queue_Element *checkingElement = timerQueue.First_Element;

		for (int i = 0; i < timerQueue.Element_Number; i++){
			if (newElement.WakeUpTime < checkingElement->WakeUpTime){
				//change list links
				checkingElement->Prev_Element->Next_Element = &newElement;
				checkingElement->Prev_Element = &newElement;
				//change First_Element in timerQueue if newElement is first
				if (i == 0){
					timerQueue.First_Element = &newElement;
				}
				break;
			}
			else{
				checkingElement = checkingElement->Next_Element;
			}
			//change Last_Element in timerQueue if newElement is last
			if (i == timerQueue.Element_Number){
				checkingElement->Next_Element->Prev_Element = &newElement;
				checkingElement->Next_Element = &newElement;
			}
		}
		//free temp pointer
		free(checkingElement);
		//make change to Element_Number in timerQueue
		timerQueue.Element_Number += 1;
	}
}

void deTimerQueue(){
	if (timerQueue.Element_Number == 0){
		printf("There is no element in timer queue\n");
	}
	else if (timerQueue.Element_Number == 1){
		timerQueue.First_Element = NULL;
		timerQueue.Element_Number -= 1;
	}
	else{
		timerQueue.First_Element = timerQueue.First_Element->Next_Element;
		timerQueue.Element_Number -= 1;
	}
}

void initPCBHub(){
	pcbHub.Element_Number = 0;
}

void enPCBHub(struct Process_Control_Block *PCB){
	struct PCB_Hub_Element newElement;
	newElement.PCB = PCB;

	//insert element as the first element
	if (pcbHub.Element_Number == 0){
		pcbHub.First_Element = &newElement;
	}
	else{
		newElement.Next_Element = pcbHub.First_Element;
		pcbHub.First_Element = &newElement;
	}
}
