/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdio.h> //You are permitted to use this library, but currently only printf is implemented. Anything else is up to you!
#include <stddef.h>
#include "k_task.h"
#include "k_mem.h"

int bomboclatctr = 0;
int wagwanctr = 0;
int ahliectr = 0;

uint32_t* donkeykong = NULL;

void bomboclat(void) {
	while(1) {
		donkeykong = (uint32_t*)k_mem_alloc(sizeof(uint32_t)*10);
		for (int i = 0; i < 10; ++i) {
			donkeykong[i] = i;
		}
		osPeriodYield();
		k_mem_dealloc(donkeykong);
//		__disable_irq();
//		printf("bomboCLAt! %d\r\n", bomboclatctr);
//		__enable_irq();
	}
}

void wagwan(void) {
	while(1) {
		osPeriodYield();
		for (int i = 0; i < 10; ++i) {
			printf("wagwan addr: %d, i: %d,  val: %d\r\n", donkeykong, i, donkeykong[i]);
		}
//		__disable_irq();
//		printf("wagwan %d\r\n", wagwanctr);
//		__enable_irq();
	}
}

void ahlie(void) {
	while(1) {
		//printf("ahlie\r\n");
		ahliectr--;
		osPeriodYield();
	}
}

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* MCU Configuration: Don't change this or the whole chip won't work!*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();
  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  /* MCU Configuration is now complete. Start writing your code below this line */

  printf("\r\n\r\nNEW RUN STARTED ----------------- \r\n\r\n\r\n");

  // Initialize kernel structures.
  osKernelInit();

	// Create a test task.
	TCB newTask;
	newTask.ptask = (void(*)(void*))bomboclat;
	osCreateDeadlineTask(1000, 0x400, &newTask);

	TCB newTask2;
	newTask2.ptask = (void(*)(void*))wagwan;
	osCreateDeadlineTask(2000, 0x400, &newTask2);

	printf("%d %d\r\n", newTask.tid, newTask2.tid);

//	TCB newTask3;
//	newTask3.ptask = (void(*)(void*))ahlie;
//	newTask3.stack_size = 0x400;
//	newTask3.deadline = 1000;
//	osCreateTask(&newTask3);


  //TCB task_copy;
  //osTaskInfo(3, &task_copy);


//  TCB newTask4;
//  newTask4.ptask = (void(*)(void*))print_continuously_test;
//  osCreateTask(&newTask4);


  // Start the kernel :>
  osKernelStart();
}
