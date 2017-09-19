
// PIC32MM0064GPL036 Configuration Bit Settings

// 'C' source line config statements

// FDEVOPT
#pragma config SOSCHP = OFF             // Secondary Oscillator High Power Enable bit (SOSC oprerates in normal power mode.)
#pragma config USERID = 0xFFFF          // User ID bits (User ID bits)

// FICD
#pragma config JTAGEN = OFF             // JTAG Enable bit (JTAG is disabled)
#pragma config ICS = PGx3               // ICE/ICD Communication Channel Selection bits (Communicate on PGEC3/PGED3)

// FPOR
#pragma config BOREN = BOR3             // Brown-out Reset Enable bits (Brown-out Reset enabled in hardware; SBOREN bit disabled)
#pragma config RETVR = OFF              // Retention Voltage Regulator Enable bit (Retention regulator is disabled)
#pragma config LPBOREN = OFF            // Low Power Brown-out Enable bit (Low power BOR is disabled)

// FWDT
#pragma config SWDTPS = PS1048576       // Sleep Mode Watchdog Timer Postscale Selection bits (1:1048576)
#pragma config FWDTWINSZ = PS25_0       // Watchdog Timer Window Size bits (Watchdog timer window size is 25%)
#pragma config WINDIS = OFF             // Windowed Watchdog Timer Disable bit (Watchdog timer is in non-window mode)
#pragma config RWDTPS = PS1048576       // Run Mode Watchdog Timer Postscale Selection bits (1:1048576)
#pragma config RCLKSEL = LPRC           // Run Mode Watchdog Timer Clock Source Selection bits (Clock source is LPRC (same as for sleep mode))
#pragma config FWDTEN = OFF             // Watchdog Timer Enable bit (WDT is disabled)

// FOSCSEL
#pragma config FNOSC = FRCDIV           // Oscillator Selection bits (Fast RC oscillator (FRC) with divide-by-N)
#pragma config PLLSRC = FRC             // System PLL Input Clock Selection bit (FRC oscillator is selected as PLL reference input on device reset)
#pragma config SOSCEN = OFF             // Secondary Oscillator Enable bit (Secondary oscillator (SOSC) is disabled)
#pragma config IESO = OFF               // Two Speed Startup Enable bit (Two speed startup is disabled)
#pragma config POSCMOD = OFF            // Primary Oscillator Selection bit (Primary oscillator is disabled)
#pragma config OSCIOFNC = OFF           // System Clock on CLKO Pin Enable bit (OSCO pin operates as a normal I/O)
#pragma config SOSCSEL = OFF            // Secondary Oscillator External Clock Enable bit (Crystal is used (RA4 and RB4 are controlled by SOSC))
#pragma config FCKSM = CSECMD           // Clock Switching and Fail-Safe Clock Monitor Enable bits (Clock switching is enabled; Fail-safe clock monitor is disabled)

// FSEC
#pragma config CP = OFF                 // Code Protection Enable bit (Code protection is disabled)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>

#include "Source/include/FreeRTOS.h"
#include "Source/include/task.h"


void vAssertCalled( const char * pcFile, unsigned long ulLine )
{
volatile unsigned long ul = 0;

	( void ) pcFile;
	( void ) ulLine;

	__asm volatile( "di" );
	{
		/* Set ul to a non-zero value using the debugger to step out of this
		function. */
		while( ul == 0 )
		{
			portNOP();
		}
	}
	__asm volatile( "ei" );
}

/* Hardware specific definitions. */
#define hwCHECON_PREFEN_BITS	  		( 0x03UL << 0x04UL )
#define hwCHECON_WAIT_STAT_BITS			( 0x07UL << 0UL )
#define hwMAX_FLASH_SPEED		  		( 30000000UL )
#define hwPERIPHERAL_CLOCK_DIV_BY_2		( 1UL << 0x13UL )
#define hwUNLOCK_KEY_0					( 0xAA996655UL )
#define hwUNLOCK_KEY_1					( 0x556699AAUL )
#define hwLOCK_KEY						( 0x33333333UL )
#define hwGLOBAL_INTERRUPT_BIT			( 0x01UL )
#define hwBEV_BIT 						( 0x00400000 )
#define hwEXL_BIT 						( 0x00000002 )
#define hwIV_BIT 						( 0x00800000 )

/*
 * Set the flash wait states for the configured CPU clock speed.
 */
static void prvConfigureWaitStates( void );

/*
 * Use a divisor of 2 on the peripheral bus.
 */
static void prvConfigurePeripheralBus( void );

/*
 * Enable the cache.
 */
static void __attribute__ ((nomips16)) prvKSeg0CacheOn( void );

/*-----------------------------------------------------------*/


void vHardwareConfigurePerformance( void )
{
unsigned long ulStatus;
#ifdef _PCACHE
	unsigned long ulCacheStatus;
#endif

	/* Disable interrupts - note taskDISABLE_INTERRUPTS() cannot be used here as
	FreeRTOS does not globally disable interrupt. */
	ulStatus = _CP0_GET_STATUS();
	_CP0_SET_STATUS( ulStatus & ~hwGLOBAL_INTERRUPT_BIT );

	prvConfigurePeripheralBus();
	prvConfigureWaitStates();

	/* Disable DRM wait state. */
	//BMXCONCLR = _BMXCON_BMXWSDRM_MASK;

	#ifdef _PCACHE
	{
		/* Read the current CHECON value. */
		ulCacheStatus = CHECON;

		/* All the PREFEN bits are being set, so no need to clear first. */
		ulCacheStatus |= hwCHECON_PREFEN_BITS;

		/* Write back the new value. */
		CHECON = ulCacheStatus;
		prvKSeg0CacheOn();
	}
	#endif

	/* Reset the status register back to its original value so the original
	interrupt enable status is retored. */
	_CP0_SET_STATUS( ulStatus );
}
/*-----------------------------------------------------------*/

void vHardwareUseMultiVectoredInterrupts( void )
{
unsigned long ulStatus, ulCause;
extern unsigned long _ebase_address[];

	/* Get current status. */
	ulStatus = _CP0_GET_STATUS();

	/* Disable interrupts. */
	ulStatus &= ~hwGLOBAL_INTERRUPT_BIT;

	/* Set BEV bit. */
	ulStatus |= hwBEV_BIT;

	/* Write status back. */
	_CP0_SET_STATUS( ulStatus );

	/* Setup EBase. */
	_CP0_SET_EBASE( ( unsigned long ) _ebase_address );
	
	/* Space vectors by 0x20 bytes. */
	_CP0_XCH_INTCTL( 0x20 );

	/* Set the IV bit in the CAUSE register. */
	ulCause = _CP0_GET_CAUSE();
	ulCause |= hwIV_BIT;
	_CP0_SET_CAUSE( ulCause );

	/* Clear BEV and EXL bits in status. */
	ulStatus &= ~( hwBEV_BIT | hwEXL_BIT );
	_CP0_SET_STATUS( ulStatus );

	/* Set MVEC bit. */
	INTCONbits.MVEC = 1;
	
	/* Finally enable interrupts again. */
	ulStatus |= hwGLOBAL_INTERRUPT_BIT;
	_CP0_SET_STATUS( ulStatus );
}

/*-----------------------------------------------------------*/

static void prvConfigurePeripheralBus( void )
{
unsigned long ulDMAStatus;
//__OSCCONbits_t xOSCCONBits;

	/* Unlock after suspending. */
/*
	ulDMAStatus = DMACONbits.SUSPEND;
	if( ulDMAStatus == 0 )
	{
		DMACONSET = _DMACON_SUSPEND_MASK;

		// Wait until actually suspended. 
		while( DMACONbits.SUSPEND == 0 );
	}
*/
	SYSKEY = 0;
	SYSKEY = hwUNLOCK_KEY_0;
	SYSKEY = hwUNLOCK_KEY_1;

	/* Read to start in sync. */
	//xOSCCONBits = OSCCON;
	//xOSCCONBits.PBDIV = 0;
	OSCCON |= hwPERIPHERAL_CLOCK_DIV_BY_2;

	/* Write back. */
	//OSCCON = xOSCCONBits;

	/* Ensure the write occurred. */
	//xOSCCONBits.w = OSCCON;

	/* Lock again. */
	SYSKEY = hwLOCK_KEY;

	/* Resume DMA activity. */
    /*
	if( ulDMAStatus == 0 )
	{
		DMACONCLR=_DMACON_SUSPEND_MASK;
	}*/
}
/*-----------------------------------------------------------*/

static void prvConfigureWaitStates( void )
{
unsigned long ulSystemClock = configCPU_CLOCK_HZ - 1;
unsigned long ulWaitStates, ulCHECONVal;

	/* 1 wait state for every hwMAX_FLASH_SPEED MHz. */
	ulWaitStates = 0;

	while( ulSystemClock > hwMAX_FLASH_SPEED )
	{
		ulWaitStates++;
		ulSystemClock -= hwMAX_FLASH_SPEED;
	}

    #ifdef _PCACHE
	/* Obtain current CHECON value. */
	ulCHECONVal = CHECON;

	/* Clear the wait state bits, then set the calculated wait state bits. */
	ulCHECONVal &= ~hwCHECON_WAIT_STAT_BITS;
	ulCHECONVal |= ulWaitStates;

	/* Write back the new value. */
	CHECON = ulWaitStates;
    #endif
}

/*-----------------------------------------------------------*/

static void __attribute__ ((nomips16)) prvKSeg0CacheOn( void )
{
unsigned long ulValue;

	__asm volatile( "mfc0 %0, $16, 0" :  "=r"( ulValue ) );
	ulValue = ( ulValue & ~0x07) | 0x03;
	__asm volatile( "mtc0 %0, $16, 0" :: "r" ( ulValue ) );
}


void main(void){
    
    /* Configure the hardware for maximum performance. */
	vHardwareConfigurePerformance();

	/* Setup to use the external interrupt controller. */
	vHardwareUseMultiVectoredInterrupts();

	portDISABLE_INTERRUPTS();

	/* Setup the digital IO for the LED's. */
	TRISAbits.TRISA0 = 0;
    TRISCbits.TRISC9 = 0;
    
    main_blinky();
    for(;;){
        LATAbits.LATA0 = PORTBbits.RB7;
    }
}



void vApplicationMallocFailedHook( void )
{
	/* vApplicationMallocFailedHook() will only be called if
	configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
	function that will get called if a call to pvPortMalloc() fails.
	pvPortMalloc() is called internally by the kernel whenever a task, queue,
	timer or semaphore is created.  It is also called by various parts of the
	demo application.  If heap_1.c or heap_2.c are used, then the size of the
	heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
	FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
	to query the size of free heap space that remains (although it does not
	provide information on how the remaining heap might be fragmented). */
	taskDISABLE_INTERRUPTS();
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
	/* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
	to 1 in FreeRTOSConfig.h.  It will be called on each iteration of the idle
	task.  It is essential that code added to this hook function never attempts
	to block in any way (for example, call xQueueReceive() with a block time
	specified, or call vTaskDelay()).  If the application makes use of the
	vTaskDelete() API function (as this demo application does) then it is also
	important that vApplicationIdleHook() is permitted to return to its calling
	function, because it is the responsibility of the idle task to clean up
	memory allocated by the kernel to any task that has since been deleted. */
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time task stack overflow checking is performed if
	configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook	function is 
	called if a task stack overflow is detected.  Note the system/interrupt
	stack is not checked. */
	taskDISABLE_INTERRUPTS();
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationTickHook( void )
{
	/* This function will be called by each tick interrupt if
	configUSE_TICK_HOOK is set to 1 in FreeRTOSConfig.h.  User code can be
	added here, but the tick hook is called from an interrupt context, so
	code must not attempt to block, and only the interrupt safe FreeRTOS API
	functions can be used (those that end in FromISR()). */
}
/*-----------------------------------------------------------*/
