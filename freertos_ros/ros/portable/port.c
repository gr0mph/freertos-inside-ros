/*
 * FreeRTOS Kernel V10.0.1
 * Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/*
 * GitHub from gr0mph
 * https://github.com/gr0mph
 *	portable --> FreeRTOS inside GNU Linux ROS
 */

/*----------------------------------------------------------------------------
 * Implementation of functions defined in portable.h for the Posix port.
 *----------------------------------------------------------------------------*/

#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <sys/times.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "port.h"

extern uint32_t SystemCoreClock; /* in Kinetis SDK, this contains the system core clock speed */

/*----------------------------------------------------------------------------*/
#ifndef MAX_NUMBER_OF_TASKS
#define MAX_NUMBER_OF_TASKS 		( _POSIX_THREAD_THREADS_MAX )
#endif
/*----------------------------------------------------------------------------*/

/* Parameters to pass to the newly created pthread. */
typedef struct XPARAMS
{
	TaskFunction_t pxCode;
	void *pvParams;
} xParams;

/* Each task maintains its own interrupt status in the critical nesting variable. */
typedef struct THREAD_SUSPENSIONS
{
	pthread_t								hThread;
	TaskHandle_t 						hTask;
	unsigned portBASE_TYPE 	uxCriticalNesting;
	xParams               * uxParams;
	portLONG								uxIndex;
} xThreadState;
/*----------------------------------------------------------------------------*/

static xThreadState 		pxThreads[MAX_NUMBER_OF_TASKS] = { NULL };
static pthread_once_t 	hSigSetupThread = PTHREAD_ONCE_INIT;
static pthread_attr_t 	xThreadAttributes;
static pthread_mutex_t 	xSuspendResumeThreadMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t	xSingleThreadMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t 				hMainThread = ( pthread_t )NULL;
/*----------------------------------------------------------------------------*/

static volatile portBASE_TYPE xSentinel = 0;
static volatile portBASE_TYPE	xWaitForNewTask = 0;
static volatile portBASE_TYPE xSchedulerEnd = pdFALSE;

static volatile portBASE_TYPE xInterruptsEnabled = pdTRUE;
static volatile portBASE_TYPE xServicingTick = pdFALSE; //pdTRUE;
static volatile portBASE_TYPE	xStartFirstTask = pdFALSE;

static volatile portBASE_TYPE xPendYield = pdFALSE;
static volatile portLONG lIndexOfLastAddedTask = 0;
static volatile unsigned portBASE_TYPE uxCriticalNesting;
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 * Setup the timer to generate the tick interrupts.
 *----------------------------------------------------------------------------*/

static void prvSetupSignalsAndSchedulerPolicy( void );

static void *prvWaitForStart( void * pvParams );

static void prvResumeThread( pthread_t xThreadId );

static pthread_t prvGetThreadHandle( TaskHandle_t hTask );

static void prvSetTaskCriticalNesting(
	pthread_t xThreadId,
	unsigned portBASE_TYPE uxNesting );

static unsigned portBASE_TYPE prvGetTaskCriticalNesting(
	pthread_t xThreadId );

static portLONG prvGetFreeThreadState( void );

static void prvSuspendThread( pthread_t xThreadId );

static void prvDeleteThread( void *xThreadId );

/*----------------------------------------------------------------------------*/

/*
 * Start first task is a separate function so it can be tested in isolation.
 */
void prvPortStartFirstTask( void );

/*----------------------------------------------------------------------------*/

extern int main(void);

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 * See header file for description.
 *----------------------------------------------------------------------------*/
portSTACK_TYPE *pxPortInitialiseStack
(
	portSTACK_TYPE *pxTopOfStack,
	TaskFunction_t pxCode,
	void *pvParameters
)
{
	printf("pxPortInitialiseStack \n");

	/* Should actually keep this struct on the stack. */
	xParams pxThisThread;
	xParams *pxThisThreadParams = &pxThisThread;

	(void)pthread_once( &hSigSetupThread, prvSetupSignalsAndSchedulerPolicy );

	if ( (pthread_t)NULL == hMainThread )
	{
		hMainThread = pthread_self();
	}

	/* No need to join the threads. */
	pthread_attr_init( &xThreadAttributes );
	pthread_attr_setdetachstate( &xThreadAttributes, PTHREAD_CREATE_DETACHED );

	/* Add the task parameters. */
	pxThisThreadParams->pxCode = pxCode;
	pxThisThreadParams->pvParams = pvParameters;

	vPortEnterCritical();

	lIndexOfLastAddedTask = prvGetFreeThreadState();

	//printf("pxPortInitialiseStack --> after --> prvGetFreeThreadState\n");

	/* Create the new pThread. */
	//	OLD IF but what next If it failed ?
	//printf("<<lock>> pxPortInitialiseStack -- xSingleThreadMutex \n");
	pthread_mutex_lock( &xSingleThreadMutex );

	//printf("pxPortInitialiseStack --> after --> pthread_mutex_lock\n");
	xSentinel = 0;
	//xWaitForNewTask = 1;

	pthread_create(
		&( pxThreads[ lIndexOfLastAddedTask ].hThread ),
		&xThreadAttributes,
		prvWaitForStart,
		(void *)pxThisThreadParams );

	/* Wait until the task suspends. */
	//printf("unlock>> pxPortInitialiseStack -- xSingleThreadMutex \n");
	(void)pthread_mutex_unlock( &xSingleThreadMutex );
	while ( xSentinel == 0 );
	vPortExitCritical();

	return pxTopOfStack;
}

/*----------------------------------------------------------------------------*/

#define portTASK_RETURN_ADDRESS	prvTaskExitError
void prvTaskExitError( void )
{
	// DEAD-CODE
	for( ;; );
}

/*----------------------------------------------------------------------------*/

void prvPortStartFirstTask( void )
{
	//printf("prvPortStartFirstTask\n");

	/* Initialise the critical nesting count ready for the first task. */
	uxCriticalNesting = 0;

	/* Start the first task. */
	vPortEnableInterrupts();

	/*	Synchronize with system tick */
	//printf("prvPortStartFirstTask -- xServicingTick\n");

	//xServicingTick = pdFALSE;
	xStartFirstTask = pdTRUE;

	/* Start the first task. */
	prvResumeThread( prvGetThreadHandle( xTaskGetCurrentTaskHandle() ) );

	//printf("prvPortStartFirstTask -- end\n");
}
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 * See header file for description.
 *----------------------------------------------------------------------------*/
portBASE_TYPE xPortStartScheduler( void )
{
	portBASE_TYPE xResult;
	int iSignal;
	sigset_t xSignals;
	sigset_t xSignalToBlock;
	sigset_t xSignalsBlocked;
	portLONG lIndex;

	printf("xPortStartScheduler\n");

	/* Establish the signals to block before they are needed. */
	sigfillset( &xSignalToBlock );

	/* Block until the end */
	(void)pthread_sigmask( SIG_SETMASK, &xSignalToBlock, &xSignalsBlocked );

	for ( lIndex = 0; lIndex < MAX_NUMBER_OF_TASKS; lIndex++ )
	{
		pxThreads[ lIndex ].uxCriticalNesting = 0;
	}

	/* Start the first task. Will not return unless all threads are killed. */
	prvPortStartFirstTask();

	//printf("xPortStartScheduler -- after -- prvPortStartFirstTask\n");

	/* This is the end signal we are looking for. */
	sigemptyset( &xSignals );
	sigaddset( &xSignals, SIG_RESUME );

	while ( pdTRUE != xSchedulerEnd )
	{
		sigwait( &xSignals, &iSignal );
	}

	/*	DEAD-CODE */
	/* Cleanup the mutexes */
	xResult = pthread_mutex_destroy( &xSuspendResumeThreadMutex );
	xResult = pthread_mutex_destroy( &xSingleThreadMutex );

	/* Should not get here! */
	return xResult;
}

/*----------------------------------------------------------------------------*/

void vPortEndScheduler( void )
{
	portBASE_TYPE xNumberOfThreads;
	portBASE_TYPE xResult;
	for ( xNumberOfThreads = 0; xNumberOfThreads < MAX_NUMBER_OF_TASKS; xNumberOfThreads++ )
	{
		if ( ( pthread_t )NULL != pxThreads[ xNumberOfThreads ].hThread )
		{
			/* Kill all of the threads, they are in the detached state. */
			xResult = pthread_cancel( pxThreads[ xNumberOfThreads ].hThread );
      if (xResult)
      	printf("pthread_cancel error!\n");
		}
	}

	/* Signal the scheduler to exit its loop. */
	xSchedulerEnd = pdTRUE;
	(void)pthread_kill( hMainThread, SIG_RESUME );
}

/*----------------------------------------------------------------------------*/

void vPortEnterCritical( void )
{
	//printf("vPortEnterCritical\n");
	vPortDisableInterrupts();
	uxCriticalNesting++;
}

/*----------------------------------------------------------------------------*/

void vPortExitCritical( void )
{
	//printf("%lx : vPortExitCritical\n", pthread_self());
	/* Check for unmatched exits. */
	if ( uxCriticalNesting > 0 )
	{
		uxCriticalNesting--;
	}

	/* If we have reached 0 then re-enable the interrupts. */
	if( uxCriticalNesting == 0 )
	{
		/* Have we missed ticks? This is the equivalent of pending an interrupt. */
		if ( xPendYield == pdTRUE )
		{
			printf("%lx : vPortExitCritical -- call vPortYield\n",  pthread_self());
			xPendYield = pdFALSE;
			vPortYield();
		}
		vPortEnableInterrupts();
	}
}

/*----------------------------------------------------------------------------*/
//void xPortSysTickHandler( int sig )
void vPortSysTickHandler( int sig )
{
	pthread_t xTaskToSuspend;
	pthread_t xTaskToResume;

	if( xStartFirstTask == pdFALSE )
		return;

	//printf("vPortSysTickHandler \n");

	pthread_mutex_lock( &xSingleThreadMutex );

	xServicingTick = pdTRUE;

	xTaskToSuspend = prvGetThreadHandle( xTaskGetCurrentTaskHandle() );

	/* Tick Increment. */
	xTaskIncrementTick();

	/* Select Next Task. */
#if ( configUSE_PREEMPTION == 1 )
	vTaskSwitchContext();
#endif

	xTaskToResume = prvGetThreadHandle( xTaskGetCurrentTaskHandle() );

	/* The only thread that can process this tick is the running thread. */
	if ( xTaskToSuspend != xTaskToResume )
	{
		printf("vPortSysTickHandler - s %lx - r %lx\n", xTaskToSuspend, xTaskToResume);

		/* Remember and switch the critical nesting. */
		prvSetTaskCriticalNesting( xTaskToSuspend, uxCriticalNesting );
		uxCriticalNesting = prvGetTaskCriticalNesting( xTaskToResume );
		/* Resume next task. */
		prvResumeThread( xTaskToResume );
		/* Suspend the current task. */
		prvSuspendThread( xTaskToSuspend );
	}
	else
	{
		/* Yielding to self */
		(void)pthread_mutex_unlock( &xSingleThreadMutex );
	}

	xServicingTick = pdFALSE;
}

/*----------------------------------------------------------------------------*/

//void xPortPendSVHandler( void );

/*----------------------------------------------------------------------------*/

// void xPortPendSVHandler( void );

/*----------------------------------------------------------------------------*/

void vPortYieldFromISR( void )
{
	/* Calling Yield from a Interrupt/Signal handler often doesn't work because the
	 * xSingleThreadMutex is already owned by an original call to Yield. Therefore,
	 * simply indicate that a yield is required soon.
	 */
	xPendYield = pdTRUE;
}
/*----------------------------------------------------------------------------*/

void vPortYield( void )
{
	pthread_t xTaskToSuspend;
	pthread_t xTaskToResume;

	//printf("<<lock>> vPortYield -- xSingleThreadMutex \n");
	pthread_mutex_lock( &xSingleThreadMutex );

	xTaskToSuspend = prvGetThreadHandle( xTaskGetCurrentTaskHandle() );
	vTaskSwitchContext();

	xTaskToResume = prvGetThreadHandle( xTaskGetCurrentTaskHandle() );
	if ( xTaskToSuspend != xTaskToResume )
	{
		printf("vPortYield - s %lx - r %lx\n", xTaskToSuspend, xTaskToResume);

		/* Remember and switch the critical nesting. */
		prvSetTaskCriticalNesting( xTaskToSuspend, uxCriticalNesting );
		uxCriticalNesting = prvGetTaskCriticalNesting( xTaskToResume );
		/* Switch tasks. */
		prvResumeThread( xTaskToResume );
		prvSuspendThread( xTaskToSuspend );
	}
	else
	{
		/* Yielding to self */
		//	printf("unlock>> vPortYield -- xSingleThreadMutex \n");
		(void)pthread_mutex_unlock( &xSingleThreadMutex );
	}
}

/*----------------------------------------------------------------------------*/

void vPortDisableInterrupts( void )
{
	xInterruptsEnabled = pdFALSE;
}

/*----------------------------------------------------------------------------*/

void vPortEnableInterrupts( void )
{
	xInterruptsEnabled = pdTRUE;
}

/*----------------------------------------------------------------------------*/

portBASE_TYPE xPortSetInterruptMask( void )
{
	portBASE_TYPE xReturn = xInterruptsEnabled;
	xInterruptsEnabled = pdFALSE;
	return xReturn;
}

/*----------------------------------------------------------------------------*/

void vPortClearInterruptMask( portBASE_TYPE xMask )
{
	xInterruptsEnabled = xMask;
}

/*----------------------------------------------------------------------------*/

void vPortForciblyEndThread( void *pxTaskToDelete )
{

}

/*----------------------------------------------------------------------------*/

void *prvWaitForStart( void * pvParams )
{
	printf("prvWaitForStart %lx \n", pthread_self());

	xParams * pxParams = ( xParams * )pvParams;

	TaskFunction_t pvCode = pxParams->pxCode;
	void * pParams = pxParams->pvParams;

	//pthread_cleanup_push( prvDeleteThread, (void *)pthread_self() );

	//printf("<<lock>> prvWaitForStart -- xSingleThreadMutex %lx \n",
	//pthread_self());
	pthread_mutex_lock( &xSingleThreadMutex );

	prvSuspendThread( pthread_self() );

	//printf("prvWaitForStart -- start function\n");

	/*	Call task 	*/
	pvCode( pParams );

	//pthread_cleanup_pop( 1 );
	//printf("prvWaitForStart -- end %lx \n", pthread_self());
	return (void *)NULL;
}
/*----------------------------------------------------------------------------*/

void prvSuspendSignalHandler(int sig)
{
	//printf("%lx prvSuspendSignalHandler %d\n", pthread_self(), sig);
	sigset_t xSignals;

	/* Only interested in the resume signal. */
	sigemptyset( &xSignals );
	sigaddset( &xSignals, SIG_RESUME );
	xSentinel = 1;

	/* Unlock the Single thread mutex to allow the resumed task to continue. */
	//printf("unlock>> prvSuspendSignalHandler -- xSingleThreadMutex %lx\n", pthread_self());
	pthread_mutex_unlock( &xSingleThreadMutex );

	/* Wait on the resume signal. */
	sigwait( &xSignals, &sig );

	/* Will resume here when the SIG_RESUME signal is received. */
	/* Need to set the interrupts based on the task's critical nesting. */
	//printf("prvSuspendSignalHandler %lx\n", pthread_self());
	if ( uxCriticalNesting == 0 )
	{
		vPortEnableInterrupts();
	}
	else
	{
		vPortDisableInterrupts();
	}
}
/*----------------------------------------------------------------------------*/

void prvSuspendThread( pthread_t xThreadId )
{
	//printf("<<lock>> prvSuspendThread -- xSuspendResumeThreadMutex %8lx\n", xThreadId);

	pthread_mutex_lock( &xSuspendResumeThreadMutex );

	/* Set-up for the Suspend Signal handler? */
	xSentinel = 0;
	//printf("unlock>> prvSuspendThread -- xSuspendResumeThreadMutex %8lx\n", xThreadId);
	pthread_mutex_unlock( &xSuspendResumeThreadMutex );
	pthread_kill( xThreadId, SIG_SUSPEND );

	//printf("prvSuspendThread %8lx -- send suspend signal\n");
	while ( xSentinel == 0 && xServicingTick == pdFALSE )
	{
			sched_yield();
	}
	//printf("prvSuspendThread %lx -- end", xThreadId);
}

/*----------------------------------------------------------------------------*/

void prvResumeSignalHandler(int sig)
{
	/* Yield the Scheduler to ensure that the yielding thread completes. */
	//printf("<<lock>> prvResumeSignalHandler -- xSingleThreadMutex %lx\n",
	//pthread_self());
	pthread_mutex_lock( &xSingleThreadMutex );

	//printf("unlock>> prvResumeSignalHandler -- xSingleThreadMutex %lx\n",
	//pthread_self());
	(void)pthread_mutex_unlock( &xSingleThreadMutex );
}
/*----------------------------------------------------------------------------*/

void prvResumeThread( pthread_t xThreadId )
{
	//printf("<<lock>> prvResumeThread -- xSuspendResumeThreadMutex %8lx\n",
	//xThreadId);
	portBASE_TYPE xResult;
	pthread_mutex_lock( &xSuspendResumeThreadMutex );

	if ( pthread_self() != xThreadId )
	{
			xResult = pthread_kill( xThreadId, SIG_RESUME );
      if (xResult)
        printf("pthread_kill error!\n");
	}

	//printf("unlock>> prvResumeThread -- xSuspendResumeThreadMutex %8lx\n",
	//xThreadId);
	pthread_mutex_unlock( &xSuspendResumeThreadMutex );

	//printf("prvResumeThread -- end\n");
}

/*----------------------------------------------------------------------------
 *	Call by pthread_once_
 *		--> so only one time
 *----------------------------------------------------------------------------*/

void prvSetupSignalsAndSchedulerPolicy( void )
{
/* The following code would allow for configuring the scheduling of this task as a Real-time task.
 * The process would then need to be run with higher privileges for it to take affect.
	int iPolicy;
	int iResult;
	int iSchedulerPriority;
	iResult = pthread_getschedparam( pthread_self(), &iPolicy, &iSchedulerPriority );
	iResult = pthread_attr_setschedpolicy( &xThreadAttributes, SCHED_FIFO );
	iPolicy = SCHED_FIFO;
	iResult = pthread_setschedparam( pthread_self(), iPolicy, &iSchedulerPriority );		*/

	//printf("prvSetupSignalsAndSchedulerPolicy\n");
	struct sigaction  sigsuspend;
	struct sigaction  sigresume;
	portLONG lIndex;

	for ( lIndex = 0; lIndex < MAX_NUMBER_OF_TASKS; lIndex++ )
	{
		pxThreads[ lIndex ].hThread = ( pthread_t )NULL;
		pxThreads[ lIndex ].hTask = ( TaskHandle_t )NULL;
		pxThreads[ lIndex ].uxCriticalNesting = 0;
	}

	sigsuspend.sa_flags = 0;
	sigsuspend.sa_handler = prvSuspendSignalHandler;
	sigfillset( &sigsuspend.sa_mask );

	sigresume.sa_flags = 0;
	sigresume.sa_handler = prvResumeSignalHandler;
	sigfillset( &sigresume.sa_mask );

	sigaction( SIG_SUSPEND, &sigsuspend, NULL );
	sigaction( SIG_RESUME, &sigresume, NULL );

	//	Now we can use mutex
	pthread_mutex_init(&xSingleThreadMutex, NULL);
	pthread_mutex_init(&xSuspendResumeThreadMutex, NULL);
}
/*----------------------------------------------------------------------------*/

pthread_t prvGetThreadHandle( TaskHandle_t hTask )
{
	//printf("prvGetThreadHandle\n");
	pthread_t hThread = ( pthread_t )NULL;
	portLONG lIndex;
	for ( lIndex = 0; lIndex < MAX_NUMBER_OF_TASKS; lIndex++ )
	{
		if ( pxThreads[ lIndex ].hTask == hTask )
		{
			hThread = pxThreads[ lIndex ].hThread;
			break;
		}
	}
	return hThread;
}

/*----------------------------------------------------------------------------*/

/*
 *	Return a free thread
 */
portLONG prvGetFreeThreadState( void )
{
	//printf("prvGetFreeThreadState\n");
	portLONG lIndex;
	for ( lIndex = 0; lIndex < MAX_NUMBER_OF_TASKS; lIndex++ )
	{
		if ( pxThreads[ lIndex ].hThread == ( pthread_t )NULL )
		{
			break;
		}
	}
	return lIndex;
}

/*----------------------------------------------------------------------------*/

void prvSetTaskCriticalNesting( pthread_t xThreadId, unsigned portBASE_TYPE uxNesting )
{
	//printf("prvSetTaskCriticalNesting %8lx\n", xThreadId);
	portLONG lIndex;
	for ( lIndex = 0; lIndex < MAX_NUMBER_OF_TASKS; lIndex++ )
	{
		if ( pxThreads[ lIndex ].hThread == xThreadId )
		{
			pxThreads[ lIndex ].uxCriticalNesting = uxNesting;
			break;
		}
	}
}

/*----------------------------------------------------------------------------*/

unsigned portBASE_TYPE prvGetTaskCriticalNesting( pthread_t xThreadId )
{
	//printf("prvGetTaskCriticalNesting %8lx\n", xThreadId);
	unsigned portBASE_TYPE uxNesting = 0;
	portLONG lIndex;
	for ( lIndex = 0; lIndex < MAX_NUMBER_OF_TASKS; lIndex++ )
	{
		if ( pxThreads[ lIndex ].hThread == xThreadId )
		{
			uxNesting = pxThreads[ lIndex ].uxCriticalNesting;
			break;
		}
	}
	return uxNesting;
}

/*----------------------------------------------------------------------------*/

void prvDeleteThread( void *xThreadId )
{
	printf("prvDeleteThread\n");
	portLONG lIndex;
	for ( lIndex = 0; lIndex < MAX_NUMBER_OF_TASKS; lIndex++ )
	{
		if ( pxThreads[ lIndex ].hThread == ( pthread_t )xThreadId )
		{
			pxThreads[ lIndex ].hThread = (pthread_t)NULL;
			pxThreads[ lIndex ].hTask = (TaskHandle_t)NULL;
			if ( pxThreads[ lIndex ].uxCriticalNesting > 0 )
			{
				uxCriticalNesting = 0;
				vPortEnableInterrupts();
			}
			pxThreads[ lIndex ].uxCriticalNesting = 0;
			break;
		}
	}
}

/*----------------------------------------------------------------------------*/

void vPortAddTaskHandle( void *pxTaskHandle )
{
	//printf("vPortAddTaskHandle\n");
	portLONG lIndex;
	pxThreads[ lIndexOfLastAddedTask ].hTask = ( TaskHandle_t )pxTaskHandle;
	for ( lIndex = 0; lIndex < MAX_NUMBER_OF_TASKS; lIndex++ )
	{
		if ( pxThreads[ lIndex ].hThread == pxThreads[ lIndexOfLastAddedTask ].hThread )
		{
			if ( pxThreads[ lIndex ].hTask != pxThreads[ lIndexOfLastAddedTask ].hTask )
			{
				pxThreads[ lIndex ].hThread = ( pthread_t )NULL;
				pxThreads[ lIndex ].hTask = NULL;
				pxThreads[ lIndex ].uxCriticalNesting = 0;
			}
		}
	}
}

/*----------------------------------------------------------------------------*/

unsigned long ulPortGetTimerValue( void )
{
	//	TODO
	printf("((TODO)) ulPortGetTimerValue\n");
	struct tms xTimes;
	unsigned long ulTotalTime = times( &xTimes );
	/* Return the application code times.
	 * The timer only increases when the application code is actually running
	 * which means that the total execution times should add up to 100%.
	 */
	return ( unsigned long ) xTimes.tms_utime;

	/* Should check ulTotalTime for being clock_t max minus 1. */
	(void)ulTotalTime;
}

/*---------------------------------------------------------------------------*/

extern int freeRTOS_main(void);

void vStartFreeRTOS( void )
{
	printf("vStartFreeRTOS\n");

  freeRTOS_main();
}

/*----------------------------------------------------------------------------*/
