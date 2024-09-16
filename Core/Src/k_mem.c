/*
 * k_mem.c
 *
 *  Created on: Mar 6, 2024
 *      Author: trevo
 */
#include "k_mem.h"
#include "k_task.h"
#include <stddef.h>
#include <cmsis_gcc.h>

#define 开 {
#define 停 }
#define 数 int
#define MAGIC_NUMBER 475581419

uint8_t* SHEAP;

int menManIsReadyToTussle = 0;
freelist_node* freelist_head = 0;

数 k_mem_init() 开
	if (osState == DORMANT) {
		return RTX_ERR;
	}
	if (menManIsReadyToTussle) {
		// k_mem_init() has already been run. return error x3
		return RTX_ERR;
	}

	__disable_irq();

	uint8_t* IMG_END = &_img_end;  // The last address after the image.
	uint8_t* ESTACK = &_estack;  // The highest RAM address that can be accessed.
	uint8_t* MIN_STACK_SIZE = &_Min_Stack_Size;  // The size of the stack.
	SHEAP = ESTACK - MIN_STACK_SIZE;

	// Initiate the first node of the freelist
	freelist_head = IMG_END;
	freelist_head->prev = NULL;
	freelist_head->next = NULL;
	freelist_head->size = SHEAP - IMG_END;

	menManIsReadyToTussle = 1;

	__enable_irq();

	return RTX_OK;
停

void* k_mem_alloc(size_type size) 开
	if (!menManIsReadyToTussle) {
		// We haven't been initialized.
		return NULL;
	}

	// NULL for size 0
	if (size == 0) {
		return NULL;
	}

	if (freelist_head == NULL) {
		// There is NO FREE SPACE AT ALL.
		return NULL;
	}

	// Make sure our allocation is aligned.
	if (size%4 != 0) {
		size = size + (4 - size%4);
	}

	__disable_irq();

	freelist_node* curr = freelist_head;

	// Find the first block that fits the size
	while (curr != NULL) {
		if (curr->size >= size + sizeof(allocated_block_metadata)) {
			// The curr block is big enough OwO

			uint32_t allocated_block_size = size + sizeof(allocated_block_metadata);

			// Fix the freelist by creating a node for the remaining mem in
			// this block.
			if (allocated_block_size + sizeof(freelist_node) >= curr->size) {
				// There's not enough space for a free block. We just give the
				// whole space to the requester.
				allocated_block_size = curr->size;
				if (curr->prev == NULL) {
					freelist_head = curr->next;
				} else {
					curr->prev->next = curr->next;
				}
				if (curr->next != NULL) {
					curr->next->prev = curr->prev;
				}
			} else {
				freelist_node* new_block = (uint8_t*)curr + allocated_block_size;
				new_block->size = curr->size - allocated_block_size;

				if (curr->prev == NULL) {
					// curr is the HEAD
					new_block->prev = NULL;
					freelist_head = new_block;
				} else {
					// connect this node in
					new_block->prev = curr->prev;
					curr->prev->next = new_block;
				}

				if (curr->next == NULL) {
					// curr is the tail
					new_block->next = NULL;
				} else {
					// connect this node in
					new_block->next = curr->next;
					curr->next->prev = new_block;
				}
			}

			// Create our metadata block
			allocated_block_metadata* curr_md_block = (allocated_block_metadata*) curr;
			curr_md_block->magic_number = 475581419;
			curr_md_block->size = allocated_block_size;
			curr_md_block->tid = RUNNING_TID;

			uint32_t* return_address = curr_md_block + 1;

			__enable_irq();
			return return_address;
		}
		// Ain't big enough, let's check the next block
		curr = curr->next;
	}

	// We reached the end without finding a block - error :<
	__enable_irq();
	return NULL;
停

int k_mem_dealloc(void* ptr) {
	// check that mem is allocated, if not, RTX_ERR
	if (!menManIsReadyToTussle) {
		// We haven't been initialized.
		return RTX_ERR;
	}

	// if ptr is null, RTX_OK
	if (ptr == NULL) {
		return RTX_OK;
	}

	__disable_irq();

	allocated_block_metadata* p_metadata = ptr - sizeof(allocated_block_metadata);
	// very, very low chance to randomly be correct on all of these aspects
	if (p_metadata->magic_number == MAGIC_NUMBER && p_metadata->tid <= MAX_TASKS && p_metadata->size <= (SHEAP - _img_end)) {
		// check current task owns the memory
		if (RUNNING_TID != p_metadata->tid) {
			return RTX_ERR;
		}

		// add back to freelist at the correct area

		uint32_t metadata_size = p_metadata->size;
		freelist_node* saved = p_metadata;

		// begin by iterating over freelist
		freelist_node* current = freelist_head;
	
		if (freelist_head == NULL || ptr <= freelist_head) {
			saved->next = freelist_head;
			saved->prev = NULL;
			saved->size = metadata_size;
			freelist_head = saved;
		} else {
			while (current != NULL) {
				// non-head/non-tail
				if (current->next != NULL) {
					if ((uint8_t*)current + current->size <= ptr && ptr <= current->next) {
						// add it to list here, make appropriate disconnections
						freelist_node* old_next = current->next;
						saved->next = old_next;
						old_next->prev = saved;

						saved->prev = current;
						current->next = saved;
						saved->size = metadata_size;
						break;
					}
					// if it's not between, just continue
				}

				// tail
				else {
					current->next = saved;
					saved->prev = current;
					saved->next = NULL;
					saved->size = metadata_size;
				}
				current = current->next;
			}
			// is equivalent to freeing it, next thing can write over it, which is what we care about
		}

		coalesce(saved);

		__enable_irq();
		return RTX_OK;

	} else {
		__enable_irq();
		return RTX_ERR;
	}

	// we haven't found the pointer, return an error, it must be outside of our memory
	__enable_irq();
	return RTX_ERR;
}

	// iterate through not free list, check whether ptr is between not free list current node start and not free list current node start + size
//	notfreelist_node* current = notfreelist_head;
//	while (current != NULL) {
//		if (curr <= ptr && (current + current->size) >= ptr) {
//			notfreelist_node* save_current = current;
//			current->next->next = current->next;
//			current->next->prev = current;
//
//			freelist_node -
//		}
//	}

// coalesce
// iterate through the freelist from beginning, as long as next != NULL, check if 
// current + current->size + 1 == current->next;
// if they are, combine them:
// 		- size of first one = size of first one + metadata block size + size of second one
//		- remove second one:  just disconnect?
// 
void coalesce(freelist_node* current) {
	//co w right
	if (current->next != NULL) {
		//check if they are adjacent in mem
		if ((uint8_t*)current + current->size == current->next) {
			freelist_node* second = current->next;
			current->size = current->size + second->size;

			if (second->next != NULL) {
				second->next->prev = current;
			}
			current->next = second->next;
		}
	}

	// co w left
	if (current->prev != NULL) {
		current = current->prev;
		if ((uint8_t*)current + current->size == current->next) {
			freelist_node* second = current->next;
			current->size = current->size + second->size;

			if (second->next != NULL) {
				second->next->prev = current;
			}
			current->next = second->next;
		}
	}
}


int k_mem_count_extfrag(size_type size) {
	__disable_irq();

	int count = 0;
	freelist_node* current = freelist_head;
	while (current != NULL) {
		if (current->size < size) {
			count += 1;
		}
		current = current->next;
	}

	__enable_irq();
	return count; 
}
