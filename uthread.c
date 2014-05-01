#include "types.h"
#include "user.h"
#include "uthread.h"

struct uthread threads[MAX_THREAD];
int next_tid;
int current_thread_index;

int uthread_init(void)
{
  int i;

  // Initialize the thread table:

  for(i = 0; i < MAX_THREAD; ++i) {
    threads[i].state = T_FREE;
  }

  next_tid = 1;

  // Create the main thread:

  current_thread_index = 0;
  threads[current_thread_index].tid = 0;
  threads[current_thread_index].stack = 0; // Null pointer means default process stack
  threads[current_thread_index].state = T_RUNNING;
  // Registers are not initialized here, they are saved before the thread is
  // switched

  // Register SIGALRM:

  if(signal(SIGALRM, uthread_yield) != 0) {
    return -1;
  }

  // Execute alarm
  alarm(THREAD_QUANTA);

  return 0;
}

static void run_thread_func(void (*func)(void *), void* value) {
  alarm(THREAD_QUANTA);
  func(value);
  uthread_exit();
}

int  uthread_create(void (*func)(void *), void* value)
{
  int i;

  // The following is a critical section, so disable thread switching
  alarm(0);

  for(i = 0; i < MAX_THREAD; ++i)
    if (threads[i].state == T_FREE)
      goto found;

  // Error: no threads are free

  // Re-enable thread switching
  alarm(THREAD_QUANTA);
  return -1;

found:
  threads[i].tid = next_tid;
  next_tid++;
  threads[i].stack = malloc(STACK_SIZE);
  *((int*)(threads[i].stack + STACK_SIZE - 3*sizeof(int))) = 0; // This return address will never be used (The run_thread_func will never return)
  *((int*)(threads[i].stack + STACK_SIZE - 2*sizeof(int))) = (int)func;
  *((int*)(threads[i].stack + STACK_SIZE - 1*sizeof(int))) = (int)value;

  // esp set to 0 means the thread has not yet run
  threads[i].esp = 0;
  threads[i].ebp = 0;
  threads[i].state = T_RUNNABLE;

  // Re-enable thread switching
  alarm(THREAD_QUANTA);

  return threads[i].tid;
}

void uthread_exit(void)
{
  int i;

  // The following is a critical section, so disable thread switching
  alarm(0);

  // Free the memory used for the stack:

  if (threads[current_thread_index].stack) {
    free(threads[current_thread_index].stack);
  }

  // Remove the current thread from the thread table
  threads[current_thread_index].state = T_FREE;

  // Wake up any sleeping threads that might be waiting to join on the current
  // thread
  for(i = 0; i < MAX_THREAD; ++i) {
    if (threads[i].state == T_SLEEPING) {
      threads[i].state = T_RUNNABLE;
    }
  }

  // Check if there are any other threads that can be switched to
  for(i = 0; i < MAX_THREAD; ++i) {
    if (threads[i].state != T_FREE) {
      // There exists at least one other thread, so it is safe to yield
      uthread_yield();
      // Control will never continue to here, since this thread has already
      // been removed from the thread table
    }
  }

  // If we got to here it means that no other threads were found. We exit the
  // process since the last running thread has exited
  exit();
}

void uthread_yield(void)
{
  // This is necessary incase uthread_yield was explicitly called rather than
  // being called from the alarm signal:
  alarm(0);

  if (threads[current_thread_index].state == T_RUNNING) {
    threads[current_thread_index].state = T_RUNNABLE;
  }

  asm("movl %%esp, %0;" : "=r" (threads[current_thread_index].esp));
  asm("movl %%ebp, %0;" : "=r" (threads[current_thread_index].ebp));

  // Switch to new thread

  current_thread_index++;
  while(threads[current_thread_index].state != T_RUNNABLE) {
    current_thread_index++;
    if(current_thread_index == MAX_THREAD) {
      current_thread_index = 0;
    }
  }

  threads[current_thread_index].state = T_RUNNING;

  if(threads[current_thread_index].esp == 0) {
    // First time the thread is being run. Set the stack to initial values and
    // jump to the run_thread_func

    asm("movl %0, %%esp;" : : "r" (threads[current_thread_index].stack + STACK_SIZE - 3*sizeof(int)));
    asm("movl %0, %%ebp;" : : "r" (threads[current_thread_index].stack + STACK_SIZE - 3*sizeof(int)));

    asm("jmp *%0;" : : "r" (run_thread_func));
  } else {
    // The thread is already running. Restore its stack, and then when the
    // current call to uthread_yield returns, execution will return to where
    // the thread was previously executing.

    asm("movl %0, %%esp;" : : "r" (threads[current_thread_index].esp));
    asm("movl %0, %%ebp;" : : "r" (threads[current_thread_index].ebp));

    alarm(THREAD_QUANTA);
  }
}

int  uthred_self(void)
{
  return threads[current_thread_index].tid;
}

int  uthred_join(int tid)
{
  int i;

  if(tid < 0 || tid >= next_tid) {
    return -1;
  }

search:

  // The following is a critical section, so disable thread switching
  alarm(0);

  for(i = 0; i < MAX_THREAD; ++i) {
    if(threads[i].state != T_FREE && threads[i].tid == tid) {
      // Found the matching thread. Put the current thread to sleep
      threads[current_thread_index].state = T_SLEEPING;
      uthread_yield();

      // We arrive here after the current thread has been woken up. Jump up and
      // try again
      goto search;
    }
  }

  // If we got to here, then the requested tid is not running and must have
  // already completed.

  // Re-enable thread switching
  alarm(THREAD_QUANTA);
  return 0;
}

// Aliases:

int  uthread_self(void)
{
  return uthred_self();
}

int  uthread_join(int tid)
{
  return uthred_join(tid);
}
