/*
 * common.h
 *
 *  Created on: Jan 5, 2024
 *      Author: nexususer
 *
 *      NOTE: If you feel that there are common
 *      C functions corresponding to this
 *      header, then any C functions you write must go into a corresponding c file that you create in the Core->Src folder
 */

#ifndef INC_COMMON_H_
#define INC_COMMON_H_

#include <stdint.h>

#define TID_NULL 0 //predefined Task ID for the NULL task
#define MAX_TASKS 16 //maximum number of tasks in the system
#define STACK_SIZE 0x200 //min. size of each task’s stack
#define DORMANT 0 //state of terminated task
#define READY 1 //state of task that can be scheduled but is not running
#define RUNNING 2 //state of running task
#define SLEEPING 3 //state of sleeping task
#define DEFAULT_DEADLINE 5
#define RTX_OK 0 // bing chilling
#define RTX_ERR 1 // error
#define _ICSR *(uint32_t*)0xE000ED04

typedef uint8_t task_t;

typedef struct task_control_block{
	void (*ptask)(void* args); //entry address
	uint32_t stack_high; //stack starting address (high address)
	uint32_t sp; //stack pointer
	task_t tid; //task ID
	uint8_t state; //task's state
	uint16_t stack_size; //stack size. Must be a multiple of 8
	uint32_t deadline;
	uint32_t original_deadline;
	uint32_t sleep_time;
}TCB;

#endif /* INC_COMMON_H_ */
