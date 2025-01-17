/*
 * Copyright (c) 2004, Bull S.A..  All rights reserved.
 * Created by: Sebastien Decugis

 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it would be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write the Free Software Foundation, Inc., 59
 * Temple Place - Suite 330, Boston MA 02111-1307, USA.

 
 * This sample test aims to check the following assertion:
 *
 * If the mutex is of type PTHREAD_MUTEX_RECURSIVE, 
 * and the calling thread already owns the mutex, 
 * the call is successful (the lock count is incremented).

 * The steps are:
 *
 *   -> trylock the mutex. It shall suceed.
 *   -> trylock the mutex again. It shall suceed again
 *   -> unlock once
 *   -> create a new child (either thread or process)
 *      -> the new child trylock the mutex. It shall fail.
 *   -> Unlock. It shall succeed.
 *   -> Unlock again. It shall fail.
 *   -> undo everything.
 */
 
 /* We are testing conformance to IEEE Std 1003.1, 2003 Edition */
 #define _POSIX_C_SOURCE 200112L
 
 /* We need the XSI extention for the mutex attributes
   and the mkstemp() routine */
#ifndef WITHOUT_XOPEN
 #define _XOPEN_SOURCE	600
#endif
 /********************************************************************************************/
/****************************** standard includes *****************************************/
/********************************************************************************************/
 #include <pthread.h>
 #include <stdarg.h>
 #include <stdio.h>
 #include <stdlib.h> 
 #include <unistd.h>

 #include <errno.h>
 #include <sys/mman.h>
 #include <string.h>
 
/********************************************************************************************/
/******************************   Test framework   *****************************************/
/********************************************************************************************/
 #include "testfrmw.h"
 #include "testfrmw.c"
 /* This header is responsible for defining the following macros:
  * UNRESOLVED(ret, descr);  
  *    where descr is a description of the error and ret is an int (error code for example)
  * FAILED(descr);
  *    where descr is a short text saying why the test has failed.
  * PASSED();
  *    No parameter.
  * 
  * Both three macros shall terminate the calling process.
  * The testcase shall not terminate in any other maneer.
  * 
  * The other file defines the functions
  * void output_init()
  * void output(char * string, ...)
  * 
  * Those may be used to output information.
  */

/********************************************************************************************/
/********************************** Configuration ******************************************/
/********************************************************************************************/
#ifndef VERBOSE
#define VERBOSE 1
#endif

/********************************************************************************************/
/***********************************    Test case   *****************************************/
/********************************************************************************************/
#ifndef WITHOUT_XOPEN

typedef struct 
{
	pthread_mutex_t mtx;
	int status; /* error code */
} testdata_t;

struct _scenar
{
	int m_type; /* Mutex type to use */
	int m_pshared; /* 0: mutex is process-private (default) ~ !0: mutex is process-shared, if supported */
	int fork; /* 0: Test between threads. ~ !0: Test across processes, if supported (mmap) */
	const char * descr; /* Case description */
}
scenarii[] =
{
	 {PTHREAD_MUTEX_RECURSIVE,  0, 0, "Recursive mutex"}
	,{PTHREAD_MUTEX_RECURSIVE,  1, 0, "Pshared Recursive mutex"}
	,{PTHREAD_MUTEX_RECURSIVE,  1, 1, "Pshared Recursive mutex across processes"}
};
#define NSCENAR (sizeof(scenarii)/sizeof(scenarii[0]))

/* The test function will only perform a trylock operation then return. */
void * tf(void * arg)
{
	testdata_t * td = (testdata_t *)arg;
	
	td->status = pthread_mutex_trylock(&(td->mtx));
	
	if (td->status == 0)
	{
		int ret;
		
		ret = pthread_mutex_unlock(&(td->mtx));
		if (ret != 0)  {  UNRESOLVED(ret, "Failed to unlock a locked semaphore");  }
	}
	
	return NULL;
}

/* Main entry point. */
int main(int argc, char * argv[])
{
	int ret;
	int sc;
	pthread_mutexattr_t ma;
	
	testdata_t * td;
	testdata_t alternativ;
	
	pid_t child_pr=0, chkpid;
	int status;
	pthread_t child_th;
	
	/* Initialize output */
	output_init();
	
/**********
 * Allocate space for the testdata structure
 */
	/* Cannot mmap a file (or not interested in this), we use an alternative method */
	td = &alternativ;
	#if VERBOSE > 0
	output("Testdata allocated in the process memory.\n");
	#endif
	
/**********
 * For each test scenario, initialize the attributes and other variables.
 * Do the whole thing for each time to test.
 */
	for ( sc=0; sc < NSCENAR ; sc++)
	{
		#if VERBOSE > 1
		output("[parent] Preparing attributes for: %s\n", scenarii[sc].descr);
		#endif
		/* set / reset everything */
		ret = pthread_mutexattr_init(&ma);
		if (ret != 0)  {  UNRESOLVED(ret, "[parent] Unable to initialize the mutex attribute object");  }
		
		/* Set the mutex type */
		ret = pthread_mutexattr_settype(&ma, scenarii[sc].m_type);
		if (ret != 0)  {  UNRESOLVED(ret, "[parent] Unable to set mutex type");  }
		#if VERBOSE > 1
		output("[parent] Mutex type : %i\n", scenarii[sc].m_type);
		#endif
			
/**********
 * Initialize the testdata_t structure with the previously defined attributes 
 */
		/* Initialize the mutex */
		ret = pthread_mutex_init(&(td->mtx), &ma);
		if (ret != 0)
		{  UNRESOLVED(ret, "[parent] Mutex init failed");  }
		
		/* Initialize the other datas from the test structure */
		td->status=0;
		
/**********
 * Proceed to the actual testing 
 */
		/* Trylock the mutex twice before creating children */
		ret = pthread_mutex_trylock(&(td->mtx));
		if (ret != 0)  {  UNRESOLVED(ret, "[parent] Unable to trylock the mutex");  }
		ret = pthread_mutex_trylock(&(td->mtx));

		if (scenarii[sc].m_type == PTHREAD_MUTEX_RECURSIVE)
		{
			if (ret != 0)  {  FAILED("Failed to pthread_mutex_trylock() twice a recursive mutex");  }
			
			/* Unlock once so the count is "1" */
			ret = pthread_mutex_unlock(&(td->mtx));
			if (ret != 0)  {  UNRESOLVED(ret, "Failed to unlock the mutex");  }
		}
		else
			if (ret == 0)  {  UNRESOLVED(-1, "Main was able to pthread_mutex_trylock() twice without error");  }
			
		/* Create the children */
		/* We are testing across two threads */
		ret = pthread_create(&child_th, NULL, tf, td);
		if (ret != 0)  {  UNRESOLVED(ret, "[parent] Unable to create the child thread.");  }
			
		/* Wait for the child to terminate */
		ret = pthread_join(child_th, NULL);
		if (ret != 0)  {  UNRESOLVED(ret, "[parent] Unable to join the thread");  }
		
		/* Check the child status */
		if (td->status != EBUSY)  
		{
			output("Unexpected return value: %d (%s)\n", td->status, strerror(td->status));
			FAILED("pthread_mutex_trylock() did not return EBUSY in the child");
		}
		
		/* Unlock the mutex */
		ret= pthread_mutex_unlock(&(td->mtx));
		if (ret != 0)  {  FAILED("Failed to unlock the mutex -- count is broken?");  }
		
		ret= pthread_mutex_unlock(&(td->mtx));
		if (ret == 0)  {  FAILED("Was able to unlock once more the mutex -- count is broken?");  }
		
/**********
 * Destroy the data 
 */
		ret = pthread_mutex_destroy(&(td->mtx));
		if (ret != 0)  {  UNRESOLVED(ret, "Failed to destroy the mutex");  }
		
		ret = pthread_mutexattr_destroy(&ma);
		if (ret != 0)  {  UNRESOLVED(ret, "Failed to destroy the mutex attribute object");  }
			
	}  /* Proceed to the next scenario */
	
	#if VERBOSE > 0
	output("Test passed\n");
	#endif

	PASSED;
	return 0;
}

#else /* WITHOUT_XOPEN */
int main(int argc, char * argv[])
{
	output_init();
	UNTESTED("This test requires XSI features");
	return 0;
}
#endif

