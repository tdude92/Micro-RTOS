#include "k_task.h"
#include "k_mem.h"
#include "common.h"
#include "stm32f4xx_hal.h"
#include "cmsis_gcc.h"

volatile uint32_t* MSP_INIT_VAL;
volatile TCB tcbArray[MAX_TASKS + 1];
int osState = DORMANT;
int RUNNING_TID = TID_NULL;

// code that TID_NULL runs
void _nullTask() {
	while (1) {
		osYield();
	}
}

void _initDormantTask(uint32_t tid) {
	volatile TCB* tcb = &tcbArray[tid];

	__disable_irq();

	tcb->ptask = NULL;
	tcb->tid = tid;
	tcb->state = DORMANT;
	tcb->stack_size = THREAD_STACK_SIZE;
	tcb->deadline = DEFAULT_DEADLINE;
	tcb->original_deadline = DEFAULT_DEADLINE;

	__enable_irq();
}

/**
 * @param currentTID The TID of the current task.
 * @returns int The TID of the next task to run.
 */
uint32_t _schedule(uint32_t currentTID) {
	uint32_t lowestTID = currentTID;
	for (uint32_t tid = 1; tid < MAX_TASKS + 1; tid++) {
		if (tcbArray[tid].state == READY){
			if(tcbArray[lowestTID].deadline > tcbArray[tid].deadline){
				lowestTID = tid;
			}
			else if (tcbArray[lowestTID].deadline == tcbArray[tid].deadline){
				if (lowestTID > tid){
					lowestTID = tid;
				}
			}
		}
	}
	return lowestTID;
}

// warning! set currTID state before calling this
task_t PREV_TID = TID_NULL;
void _dispatch(uint32_t currTID, uint32_t newTID) {
	__disable_irq();
	// Switch tasks
	tcbArray[newTID].state = RUNNING;
	PREV_TID = currTID;
	RUNNING_TID = newTID;
	// Context switch
	_ICSR |= 1<<28; // control register bit for a PendSV interrupt
	__asm("isb");
	__enable_irq();
}

extern void addTask(){
	tcbArray[PREV_TID].sp = __get_PSP() - 32;
	uint32_t *top_ptr = tcbArray[RUNNING_TID].sp;
	__set_PSP((uint32_t)top_ptr);
}

extern void enableIRQ(){
	__enable_irq();
}

extern void disableIRQ(){
	__disable_irq();
}

// Tick all deadlines
void k_helper_tick_all(void) {
	if (osState != RUNNING) {
		return;
	}

	int yielded = 0;
	for (int i = 1; i < MAX_TASKS + 1; i++) {
		if (tcbArray[i].state != DORMANT){
			tcbArray[i].deadline -= 1;
			if (tcbArray[i].deadline == 0) {
				tcbArray[i].deadline = tcbArray[i].original_deadline;
				if (!yielded && tcbArray[i].state == RUNNING){
					osYield();
					yielded = 1;
				}

			}

			if(tcbArray[i].state == SLEEPING){
				tcbArray[i].sleep_time -= 1;
				if (tcbArray[i].sleep_time == 0){
					tcbArray[i].deadline = tcbArray[i].original_deadline;
					tcbArray[i].state = READY;
				}
			}
		}
	}
}

void osKernelInit(void) {
	__disable_irq();

	MSP_INIT_VAL = *(uint32_t**)0x0;

	for (uint32_t tid = 1; tid < MAX_TASKS + 1; tid++) {
		_initDormantTask(tid);
	}

	osState = READY;
	k_mem_init();

	const uint32_t nullTaskStackSize = 0x100;
	const uint32_t nullTaskDeadline = 4294967295; // uint32 max

	tcbArray[TID_NULL].tid = TID_NULL;
	tcbArray[TID_NULL].ptask = _nullTask;
	tcbArray[TID_NULL].stack_size = nullTaskStackSize;
	tcbArray[TID_NULL].stack_high = (uint32_t*)((k_mem_alloc(nullTaskStackSize) + nullTaskStackSize));
	tcbArray[TID_NULL].sp = tcbArray[TID_NULL].stack_high;
	tcbArray[TID_NULL].deadline = nullTaskDeadline;
	tcbArray[TID_NULL].original_deadline = nullTaskDeadline;

	uint32_t* stackptr = (uint32_t*)tcbArray[TID_NULL].sp;
	*(--stackptr) = 1 << 24;
	*(--stackptr) = (uint32_t)tcbArray[TID_NULL].ptask;
	for (int i = 0; i < 14; i++) {
		*(--stackptr) = 0xA;
	}
	tcbArray[TID_NULL].sp =(uint32_t)stackptr;

	__enable_irq();
}

// matthews note: maran ma said to make sure provided tcb is updated to match _tid(tcb)
int osCreateTask(TCB* task){
	int return_value = RTX_ERR;
    for (uint8_t tid = 1; tid < MAX_TASKS + 1; tid++) {
    	if (tcbArray[tid].state == DORMANT) {
    		// We found a task/slot to create this one in. This task is already
    		// created with stack ptr, tid, etc. Check osKernelInit().

    		__disable_irq();

            task_t currRunning = RUNNING_TID;
            if(currRunning != TID_NULL){
            	tcbArray[currRunning].state = READY;
            	tcbArray[tid].state = RUNNING;
				RUNNING_TID = tid;
            }

            uint32_t* stack = (uint32_t*)k_mem_alloc(task->stack_size);
            if (stack == 0) {
            	return RTX_ERR;
            }

        	tcbArray[tid].ptask = task->ptask;
			tcbArray[tid].stack_size = task->stack_size;
			tcbArray[tid].stack_high = stack + task->stack_size;
			tcbArray[tid].sp = tcbArray[tid].stack_high;
			tcbArray[tid].original_deadline = task->deadline;
			task->tid = tcbArray[tid].tid;

			if (task->deadline != 0) {
				tcbArray[tid].deadline = task->deadline;
			}

			if (currRunning != TID_NULL) {
				tcbArray[currRunning].state = RUNNING;
				RUNNING_TID = currRunning;
			}
			tcbArray[tid].state = READY;

			uint32_t* stackptr = (uint32_t*)tcbArray[tid].sp;
			*(--stackptr) = 1 << 24;
			*(--stackptr) = (uint32_t)tcbArray[tid].ptask;
			for (int i = 0; i < 14; i++) {
				*(--stackptr) = 0xA;
			}
			tcbArray[tid].sp =(uint32_t)stackptr;
            return_value = RTX_OK;

            __enable_irq();

            break;
    	} // its DORMANT
    }

    // TODO investigate if need to disable interrupts here
    //      yes disable: test to ensure that ctx switch actually happens
    //      no disable: what if systick preemption happens here but
    //                  the child task is about to be dispatched?
    if (
		return_value == RTX_OK &&
		RUNNING_TID != TID_NULL &&
		tcbArray[RUNNING_TID].deadline > task->deadline
	) {
    	// preempt
		tcbArray[RUNNING_TID].state = READY;
		_dispatch(RUNNING_TID, task->tid);
    }

    return return_value;
}

int osKernelStart(void) {
	// Only start the kernel if it has been initialized and is not running.
	if (osState != READY) {
		return RTX_ERR;
	}

	// Start the first task we find that's not dormant.
	int tid = _schedule(0);
	osState = RUNNING;

	__disable_irq();
	tcbArray[tid].state = RUNNING;
	RUNNING_TID = tid;
	__set_PSP(tcbArray[tid].sp);
	__enable_irq();

	__asm("SVC #7");

	return RTX_OK;
}

void osYield(void) {
	__disable_irq(); // irq re-enabled in _dispatch

	// Set current task to ready
	tcbArray[RUNNING_TID].state = READY;
	uint32_t newTID = _schedule(RUNNING_TID);
	_dispatch(RUNNING_TID, newTID);
}

int osTaskInfo(task_t tid, TCB* taskCopy){
	if (tid == TID_NULL) return RTX_ERR;

	taskCopy->state = tcbArray[tid].state;
	taskCopy->stack_size = tcbArray[tid].stack_size;
	taskCopy->sp = tcbArray[tid].sp;
	taskCopy->tid = tcbArray[tid].tid;
	taskCopy->stack_high = tcbArray[tid].stack_high;
	taskCopy->ptask = tcbArray[tid].ptask;
	taskCopy->deadline = tcbArray[tid].deadline;

	return RTX_OK;
}

int osTaskExit(void){
	if (osState != RUNNING) {
		return RTX_ERR;
	}

	// Schedule
	uint32_t newTID = _schedule(RUNNING_TID);

	// Reset tcb
	_initDormantTask(RUNNING_TID);

	// Context switch
	_dispatch(RUNNING_TID, newTID);

	return RTX_OK;
}

void osSleep(int timeInMs){
	if (RUNNING_TID != TID_NULL){
		__disable_irq(); // irq re-enabled in _dispatch
		tcbArray[RUNNING_TID].state = SLEEPING;
		tcbArray[RUNNING_TID].sleep_time = timeInMs;

		uint32_t newTID = _schedule(0);
		_dispatch(RUNNING_TID, newTID);
	}

}

void osPeriodYield() {
	if (RUNNING_TID != TID_NULL) {
		__disable_irq(); // irq re-enabled in osSleep
		uint32_t currDeadline = tcbArray[RUNNING_TID].deadline;
		osSleep(currDeadline);
	}
}

int osSetDeadline(int deadline, task_t tid) {
	if (
		deadline <= 0 ||
		tid < 0 ||
		tid > MAX_TASKS ||
		tcbArray[tid].state != READY
	) {
		return RTX_ERR;
	}

	__disable_irq();

	tcbArray[tid].deadline = deadline;
	tcbArray[tid].original_deadline = deadline;

	if (
		RUNNING_TID != TID_NULL &&
		tcbArray[RUNNING_TID].deadline > tcbArray[tid].deadline
	){
		// preempt
		tcbArray[RUNNING_TID].state = READY;
		_dispatch(RUNNING_TID, tid); // irq re-enabled in _dispatch
	} else {
		// don't preempt
		__enable_irq();
	}

	return RTX_OK;
}

int osCreateDeadlineTask(int deadline, int s_size, TCB* task) {
	if (deadline <= 0) {
		return RTX_ERR;
	}

	if (s_size < STACK_SIZE) {
		return RTX_ERR;
	}

	task->deadline = deadline;
	task->stack_size = s_size;
	return osCreateTask(task);
}
