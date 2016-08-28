	;INCLUDE semaphore.h
	#include "config.h"
	#include "share.h"

	IMPORT	scheduler
	IMPORT	__next_task
	IMPORT	__mutex_wakeup
	IMPORT	__mutex_waiting
	IMPORT	__semaphore_wakeup
	IMPORT	__semaphore_waiting
	
	
	
	AREA	STACK, DATA, READWRITE
nest	DCD	0

	PRESERVE8
	AREA 	PLATFORM,CODE,READONLY
	ALIGN
;##########################################################################
;			MACROS BEGIN				
;##########################################################################	
	MACRO
$label	SPIN_LOCK
	MOV	R2, #__LOCKED		;
$label.retry
	LDREX	R1, [R0]		; load the lock value
	CMP	R1, #__UNLOCKED	; is the lock free?
	ITT	EQ			; IT instruction for STREXEQ and CMPEQ
	STREXEQ	R1, R2, [r0]		; try and claim the lock
	CMPEQ	R1, #0			; did this succeed?
	BNE	$label.retry		; no? try again
	; yes? we have the lock	
	DMB
	MEND
	
	MACRO
	SPIN_UNLOCK
	MOV	R2, #__UNLOCKED
	STR	R2, [R0]
	DMB
	MEND
;##########################################################################
;			MACRO END				
;##########################################################################

;##########################################################################
;			context BEGIN				
;##########################################################################
SCB_ICSR
	DCD	0XE000ED04
SCB_ICSR_PENDSVSET
	DCD	(1<<28)
MSP_BASE	
	DCD	ISR_STACK_BASE
;context END
;##########################################################################
;		Scheduler Functions BEGIN
;##########################################################################
scheduler_pause	FUNCTION
	export	scheduler_pause
	;pause scheduler befor change variable "nest".
	MOV	R0, #CONFIG_BASE_PRIORITY
	MOV	R0, R0, LSL#4	;STM32 uses 4 Bits for the Priority Levels
	MSR	BASEPRI, R0
	DSB
	ISB
	;nest+1
	LDR	R0, =nest
SP_RETYR
	LDREX	R1, [R0]
	ADD	R1, R1, #1
	STREX	R2, R1, [R0]
	CMP	R2, #0
	BNE	SP_RETYR
	;return BASEPRI
	;MRS	R0, BASEPRI
	BX	LR
	;NOP
	ENDFUNC
;##########################################################################
scheduler_continue	FUNCTION
	export	scheduler_continue
	;nest++
	LDR	R0, =nest
SC_RETRY
	LDREX	R1, [R0]
   IF :DEF: CONFIG_OS_DEBUG
	CMP	R1, #0
	BEQ	.
   ENDIF
	SUB	R1, R1, #1
	STREX	R2, R1, [R0]
	CMP	R2, #0
	BNE	SC_RETRY
	;if nest==0, start scheduler
	CMP	R1, #0
	BNE	SC_END
	MOV	R0, #0
	MSR	BASEPRI, R0
	DSB
	ISB
SC_END
	;return value of BASEPRI
	;MRS	R0, BASEPRI
	BX	LR
	;NOP
	ENDFUNC
;##########################################################################
__scheduler	FUNCTION
	export	__scheduler
	LDR	R0, =SCB_ICSR
	LDR	R1, [R0]
	LDR	R0, =SCB_ICSR_PENDSVSET
	LDR	R2, [R0]
	STR	R2, [R1]
	DSB
	BX	LR	
	ENDFUNC
;##########################################################################
;this function should be called by main() only
;int os_start(void *sp)
os_start	FUNCTION
	EXPORT	os_start
	PUSH	{R0, R4-R5, LR}		;push R0 for Eight-byte stack alignment
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
	;it's not neccessary to use unprivilege mode for simple application
	MOV	R2, #2
	MSR	CONTROL, R2
	ISB
	DSB
	;raise PendSV
	BL	__scheduler
	;enable scheduler
	BL	scheduler_continue
	POP	{R0, R4-R5, LR}
	MOV	R0, #0
	BX	LR
	;NOP
	ENDFUNC
;##########################################################################
;		Scheduler Functions END
;##########################################################################
;##########################################################################
;		Synchronization mechanism BEGIN
;##########################################################################
    IF  :DEF: CONFIG_HAVE_SPINLOCK
;void spin_lock(spinlock_t *lock)
spin_lock	FUNCTION
	EXPORT spin_lock
sl	SPIN_LOCK
	BX	LR
	ENDFUNC
;##########################################################################
;uint32_t spin_lock_irqsave(spinlock_t *lock)
spin_lock_irqsave	FUNCTION
	EXPORT	spin_lock_irqsave
	MRS	R3, PRIMASK
	CPSID	I
sli	SPIN_LOCK
	MOV	R0, R3
	BX	LR
	ENDFUNC
;##########################################################################
;void spin_unlock(spinlock_t *lock)
spin_unlock	FUNCTION
	EXPORT spin_unlock
	SPIN_UNLOCK
	BX	LR
	ENDFUNC
;##########################################################################
;void spin_unlock_irqrestore(spinlock_t *lock, uint32_t flag)
spin_unlock_irqrestore	FUNCTION
	EXPORT	spin_unlock_irqrestore
	SPIN_UNLOCK
	MSR	PRIMASK, R1
	DSB
	BX	LR
	ENDFUNC
    ENDIF
;##########################################################################
    IF :DEF: CONFIG_HAVE_MUTEX
;void mutex_lock(mutex_t *mutex)
mutex_lock	FUNCTION
	export mutex_lock
ml_retry
	LDREX	R1, [R0]		; load the lock value
	CMP	R1, #__UNLOCKED		; is the lock free?
	BNE	ml_sch			; wait resource ready by call scheduler
	MOV	R2, #__LOCKED
	ITT	EQ			; IT instruction for STREXEQ and CMPEQ
	STREXEQ	R1, R2, [R0]		; try and claim the lock
	CMPEQ	R1, #0			; did this succeed?
	BNE	ml_retry		; no? try again
	; yes? we have the lock	
	DMB
	BX	LR
ml_sch
	;wait resource ready
	PUSH	{R0, LR}
	MOV	R1, #0			;delay = 0 means wait forever
	BL	__mutex_waiting
	POP	{R0, LR}
	B	ml_retry
	ENDFUNC
;##########################################################################
;void mutex_unlock(mutex_t *mutex)
mutex_unlock	FUNCTION
	export mutex_unlock
	MOV	R2, #__UNLOCKED
	STR	R2, [R0]
	DSB
	;notify resource ready
	PUSH	{LR, R0}	;push R0 for Eight-byte stack alignment
	BL	__mutex_wakeup
	POP	{LR, R0}
	BX	LR
	ENDFUNC
    ENDIF
;##########################################################################
    IF :DEF: CONFIG_HAVE_SEMAPHORE
;void semaphore_inc(semaphore_t *sem)
semaphore_inc	FUNCTION
	export semaphore_inc
si_retry
	LDREX	R1, [R0]
  IF :DEF: CONFIG_OS_DEBUG
	LDR	R2, [R0, #4]	;semaphore_t::range
	CMP	R1, R2
	BGE	.
  ENDIF
	ADD	R1, R1, #1
	STREX	R2, R1, [R0]
	CMP	R2, #0
	BNE	si_retry
	DMB
	PUSH	{R0, LR}
	BL	__semaphore_wakeup
	POP	{R0, LR}
	BX	LR
	ENDFUNC
;##########################################################################
;void semaphore_dec(semaphore_t *sem)
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
	;wait resource ready
	PUSH	{R0, LR}
	MOV	R1, #1			;delay = 0 means wait forever
	BL	__semaphore_waiting
	POP	{R0, LR}
	B	sd_retry
	ENDFUNC
    ENDIF
;##########################################################################
;		Synchronization mechanism END
;##########################################################################
;##########################################################################
;		ISR BEGIN
;##########################################################################
PendSV_Handler	FUNCTION
	export	PendSV_Handler
  IF :DEF: CONFIG_OS_DEBUG
	;check EXC_RETURN if return stack is PSP?
	TST	LR, #4
	BEQ	.	;main stack mode
  ENDIF
	;process stack mode
	;save user space registers(context)
	MRS	R0, PSP
	STMDB	R0!, {R4-R11}
	MSR	PSP, R0
	;flush cache
	DSB
	;save LR
	PUSH	{R0, LR}		;push R0 for Eight-byte stack alignment
	;call task select & task switch
	BL	__next_task		;function __next_task() return SP of new task
	;restore context
	;MRS	R0, PSP
	LDMIA	R0!, {R4-R11}
	MSR	PSP, R0
	;restore LR
	POP	{R0, LR}
	BX	LR
	ENDFUNC

	END	;end of file
