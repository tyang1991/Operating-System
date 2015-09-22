#include "addition.h"

struct Timer_Queue_Header timerQueue;

void timerQueueInit(){
	timerQueue.Element_Number = 0;
}

void insertTimerQueue(struct Process_Control_Block *PCB, long wakeUpTime){
	struct Timer_Queue_Element newElement;
	
	if (timerQueue.Element_Number == 0){
		//build timer queue element
		newElement.WakeUpTime = wakeUpTime;
		newElement.PCB = PCB;
		newElement.Prev_Element = &newElement;
		newElement.Next_Element = &newElement;
		//make change in timer queue
		timerQueue.Element_Number = 1;
		timerQueue.First_Element = &newElement;
	}
	else{
		newElement.WakeUpTime = wakeUpTime;
		newElement.PCB = PCB;

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
		//make change to Element_Number in timerQueue
		timerQueue.Element_Number += 1;
	}
}