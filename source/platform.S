	;INCLUDE semaphore.h
	#include "config.h"
	#include "spinlock.h"

	PRESERVE8
	AREA 	PLATFORM,CODE,READONLY
	ALIGN


;MACROS BEGIN
	MACRO
	WAIT
	WFE	;FIXME: call scheduler
	MEND
	
	MACRO
	WAKEUP
	SEV	;FIXME: call scheduler
	MEND

	MACRO
$label	SPIN_LOCK
	MOV	R2, #__SPIN_LOCKED		;
$label.retry
	LDREX	R1, [R0]		; load the lock value
	CMP	R1, #__SPIN_UNLOCKED	; is the lock free?
	ITT	EQ			; IT instruction for STREXEQ and CMPEQ
	STREXEQ	R1, R2, [r0]		; try and claim the lock
	CMPEQ	R1, #0			; did this succeed?
	BNE	$label.retry		; no? try again
	; yes? we have the lock	
	DMB
	MEND
	
	MACRO
	SPIN_UNLOCK
	MOV	R2, #__SPIN_UNLOCKED
	STR	R2, [R0]
	DMB
	MEND
;MACRO END


;FUNCTIONS BEGIN
spin_lock	FUNCTION
	EXPORT spin_lock
sl	SPIN_LOCK
	BX	LR
	ENDFUNC

spin_lock_irqsave	FUNCTION
	EXPORT	spin_lock_irqsave
	MRS	R4, PRIMASK
	CPSID	I
sli	SPIN_LOCK
	MOV	R0, R4
	BX	LR
	ENDFUNC

spin_unlock	FUNCTION
	EXPORT spin_unlock
	SPIN_UNLOCK
	BX	LR
	ENDFUNC

spin_unlock_irqrestore	FUNCTION
	EXPORT	spin_unlock_irqrestore
	SPIN_UNLOCK
	MSR	PRIMASK, R1
	DSB
	BX	LR
	ENDFUNC

mutex_lock	FUNCTION
	export mutex_lock
	MOV	R2, #__SPIN_LOCKED
ml_retry
	LDREX	R1, [R0]		; load the lock value
	CMP	R1, #__SPIN_UNLOCKED	; is the lock free?
	BNE	ml_sch		; wait resource ready by call scheduler
	ITT	EQ			; IT instruction for STREXEQ and CMPEQ
	STREXEQ	R1, R2, [r0]		; try and claim the lock
	CMPEQ	R1, #0			; did this succeed?
	BNE	ml_retry		; no? try again
	; yes? we have the lock	
	DMB
	BX	LR
ml_sch
	;wait resource ready
	WAIT
	B	ml_retry
	ENDFUNC
	
mutex_unlock	FUNCTION
	export mutex_unlock
	MOV	R2, #__SPIN_UNLOCKED
	STR	R2, [R0]
	DSB
	;notify resource ready
	WAKEUP	
	BX	LR
	ENDFUNC

semaphore_inc	FUNCTION
	export semaphore_inc
si_retry
	LDREX	R1, [R0]
	ADD	R1, R1, #1
	STREX	R2, R1, [R0]
	CMP	R2, #0
	BNE	si_retry
	DMB
	BX	LR
	ENDFUNC
	
semaphore_dec	FUNCTION
	export semaphore_dec
sd_retry
	LDREX	R1, [R0]
	CMP	R1, #0
	BEQ	sd_sch
	SUB	R1, #1
	STREX	R2, R1, [R0]
	CMP	R2, #0
	BNE	sd_retry
	DMB
	BX	LR
sd_sch
	WAIT	;wait resource ready
	B	sd_retry
	ENDFUNC

	IMPORT	next_task
PendSV_Handler	FUNCTION
	export	PendSV_Handler
	;check EXC_RETURN if return stack is PSP?
	TST	LR, #4
BUG
	BEQ	BUG	;main stack mode
	;process stack mode
	;save user space registers(context)
	MRS	R0, PSP
	STMDB	R0!, {R4-R11}
	MSR	PSP, R0
	;flush cache
	DSB
	;save LR
	STMFD	SP!, {LR}
	;call task select & task switch
	BL	next_task	;function next_task() return SP of new task
	;restore LR
	LDMFD	SP!, {LR}
	;restore context
	;MRS	R0, PSP
	LDMIA	R0!, {R4-R11}
	MSR	PSP, R0
	;clear PENDSV, it may set by preemption interrupt
	BX	LR
	ENDFUNC

MSP_BASE	
	DCD	ISR_STACK_BASE
;this function should be called by main() only
scheduler_start	FUNCTION
	EXPORT	scheduler_start
	STMFD	SP!, {R4-R5}
	;copy stack from MSP to PSP
	MRS	R1, MSP
	LDR	R2, =MSP_BASE
	LDR	R2, [R2]
	MOV	R4, R2
	SUB	R3, R2, R1
	AND	R3, R3, #-3
	SUB	R0, R0, R3
	MOV	R5, R0
COPY_LOOP
	SUBS	R3, R3, #4
	BMI	COPY_END
	LDR	R2, [R1, R3]
	STR	R2, [R0, R3]
	B	COPY_LOOP
COPY_END
	;update MSP & PSP
	MSR	MSP, R4
	MSR	PSP, R5
	;change to thread & privilege mode
	MOV	R2, #2
	MSR	CONTROL, R2
	ISB
	DSB
	LDMFD	SP!, {r4, r5}
	;raise PendSV
	
	
	

	BX	LR
	;NOP
	ENDFUNC

	END	;end of file
