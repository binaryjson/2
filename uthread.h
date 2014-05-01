#define THREAD_QUANTA 5

/* Possible states of a thread; */
typedef enum  {T_FREE, T_RUNNING, T_RUNNABLE, T_SLEEPING} uthread_state;

#define STACK_SIZE  4096
#define MAX_THREAD  64

typedef struct uthread uthread_t, *uthread_p;

struct uthread {
	int				tid;
	int 	       	esp;        /* current stack pointer */
	int 	       	ebp;        /* current base pointer */
	char		   *stack;	    /* the thread's stack */
	uthread_state   state;     	/* running, runnable, sleeping */
};
 
int uthread_init(void);
int  uthread_create(void (*func)(void *), void* value);
void uthread_exit(void);
void uthread_yield(void);
int  uthred_self(void);
int  uthred_join(int tid);

// Aliases:
int  uthread_self(void);
int  uthread_join(int tid);

struct sem_queue_node {
    //int tid;
    struct sem_queue_node* next;
};

struct binary_semaphore {
    uint value;
    struct sem_queue_node* first;
    struct sem_queue_node* last;
};

void binary_semaphore_init(struct binary_semaphore* semaphore, int value);

// Calling binary_semaphore_down when the semaphore is already down will block
// until another thread calls binary_semaphore_up
void binary_semaphore_down(struct binary_semaphore* semaphore);

// Calling binary_semaphore_up when the semaphore is already up is an error
void binary_semaphore_up(struct binary_semaphore* semaphore);
