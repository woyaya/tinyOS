#ifndef __CONFIG_H__
#define __CONFIG_H__
#ifndef IRAM_BASE
  #define IRAM_BASE	0x20000000
#endif
#ifndef IRAM_SIZE
  #define IRAM_SIZE	0x00005000
#endif

#define CONFIG_PENDSV_PRIORITY		0x0F
#define CONFIG_SYSTICK_PRIORITY		0x0E
/*The processor does not process any exception 
 *with a priority value greater than or equal to BASEPRI.
 *Any exception handler which may access task list, 
 *should set greater or equal priority then CONFIG_BASE_PRIORITY */
#define CONFIG_BASE_PRIORITY		0x08

#define CONFIG_SYSTICK_PERIOD		10	//mS

#define TASK_STACK_SIZE_MIN	(0x80 * sizeof(uint32_t))

#define ISR_STACK_BASE		(IRAM_BASE+IRAM_SIZE)
#define ISR_STACK_SIZE		TASK_STACK_SIZE_MIN

#define MAIN_STACK_BASE		(ISR_STACK_BASE-ISR_STACK_SIZE)
#define MAIN_STACK_SIZE		TASK_STACK_SIZE_MIN

#define IDLE_STACK_BASE		(MAIN_STACK_BASE - MAIN_STACK_SIZE)
#define IDLE_STACK_SIZE		(0x20 * sizeof(uint32_t))

#endif
