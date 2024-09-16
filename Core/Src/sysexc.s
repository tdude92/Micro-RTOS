.syntax unified
.cpu cortex-m4
.thumb

.global SVC_Handler
//.p2align 2
//.type SVC_Handler, %function
.thumb_func
SVC_Handler:
	//.fnstart
	// https://developer.arm.com/documentation/ka004005/latest/
	//.global SVC_Handler_Main
    TST lr, #4
    ITE EQ
    MRSEQ r0, MSP
    MRSNE r0, PSP
    B SVC_Handler_Main

	//.fnend


.global PendSV_Handler
//.p2align 2
//.type PendSV_Handler, %function
.thumb_func
PendSV_Handler:
	//.fnstart
	//.global PendSV_Handler

	bl disableIRQ

	// move old task sp into r0
	MRS r0, PSP
	// move old task r4 - r11 into memory
	STMDB r0!, {r4-r11}
	// c function to call for setting PSP, load psp of the new task ur switching into

	bl addTask

	// move new task sp into r0
	MRS r0, PSP
	// move new task r4 - r11 into registers
	LDMIA r0!, {r4-r11}

	// Set new psp
	MSR PSP, r0

	bl enableIRQ

	// magic number
	MOV LR, #0xFFFFFFFD
	BX LR

	//.fnend
