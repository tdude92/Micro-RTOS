/*
 * k_task.h
 *
 *  Created on: Jan 5, 2024
 *      Author: nexususer
 *
 *      NOTE: any C functions you write must go into a corresponding c file that you create in the Core->Src folder
 */

#ifndef INC_K_TASK_H_
#define INC_K_TASK_H_

#include "common.h"

#define MAIN_STACK_SIZE 0x400
#define THREAD_STACK_SIZE 0x400

extern volatile uint32_t* MSP_INIT_VAL;
extern volatile TCB tcbArray[MAX_TASKS + 1];
extern int osState;

extern int RUNNING_TID;

void osKernelInit(void);
int osCreateTask(TCB* task);
int osKernelStart(void);
void osYield(void);
int osTaskInfo(task_t TID, TCB* task_copy);
int osTaskExit(void);
void osSleep(int timeInMs);
void osPeriodYield();
int osSetDeadline(int deadline, task_t tid);
int osCreateDeadlineTask(int deadline, int s_size, TCB* task);

void k_helper_tick_all(void);

#endif /* INC_K_TASK_H_ */
