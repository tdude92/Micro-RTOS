/*
 * k_mem.h
 *
 *  Created on: Jan 5, 2024
 *      Author: nexususer
 *
 *      NOTE: any C functions you write must go into a corresponding c file that you create in the Core->Src folder
 */

#include <common.h>

#ifndef INC_K_MEM_H_
#define INC_K_MEM_H_

#define size_type long unsigned int
#define true 1
#define false 0

extern uint32_t _img_end;
extern uint32_t _estack;
extern uint32_t _Min_Stack_Size;
extern uint8_t* SHEAP;

typedef struct freelist_node {
	struct freelist_node* prev;
	struct freelist_node* next;
	uint32_t size;
} freelist_node;

typedef struct allocated_block_metadata {
	uint32_t magic_number;
	uint8_t __pad[3];
	uint8_t tid;
	uint32_t size;
} allocated_block_metadata;


int k_mem_init();
void* k_mem_alloc(size_type size);
int k_mem_dealloc(void* ptr);
int k_mem_count_extfrag(size_type size);

#endif /* INC_K_MEM_H_ */
