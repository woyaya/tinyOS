#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#ifdef __cplusplus
 extern "C" {
#endif 

typedef struct{
	uint32_t R4;
	uint32_t R5;
	uint32_t R6;
	uint32_t R7;
	uint32_t R8;
	uint32_t R9;
	uint32_t R10;
	uint32_t R11;

//Following registers auto-saved @ exception
//In subroutine calls, 'BL' just put $PC to $LR, 
//all other registers should be saved by manual
	uint32_t R0;
	uint32_t R1;
	uint32_t R2;
	uint32_t R3;
	uint32_t R12;
	uint32_t LR;
	uint32_t PC;
	uint32_t xPSR;
	#define DEFAULT_PSR	0x21000000
}stack_frame_t;

#ifdef __cplusplus
 }
#endif 


#endif
