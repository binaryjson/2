#include "types.h"
#include "uthread.h"
#include "user.h"

#include "x86.h"

#define MAX_SOLDIERS 64

int n;
int cells[MAX_SOLDIERS];

struct binary_semaphore soldier_ready[MAX_SOLDIERS];
struct binary_semaphore soldier_waiting[MAX_SOLDIERS];
struct binary_semaphore soldier_prepared[MAX_SOLDIERS];
struct binary_semaphore soldier_update[MAX_SOLDIERS];

typedef enum { Q=0, R=1, L=2, S=3, F=4, X=5 } s_state;

int state_transition[4][5][5] =
{
  {
    { Q, Q, L, Q, Q },
    { R, R, X, X, R },
    { Q, X, L, S, Q },
    { Q, S, X, X, X },
    { Q, Q, L, Q, X },
  },
  {
    { R, R, Q, L, X },
    { Q, R, L, X, L },
    { S, X, Q, F, X },
    { Q, R, L, X, L },
    { S, X, Q, F, X },
  },
  {
    { L, S, Q, Q, X },
    { Q, Q, R, R, Q },
    { L, X, L, L, X },
    { R, F, X, X, F },
    { X, X, R, R, X },
  },
  {
    { Q, S, L, Q, X },
    { R, X, X, F, X },
    { S, X, X, S, X },
    { Q, S, F, X, X },
    { Q, S, F, X, X },
  }
};

int next_state(int i)
{
  int left;
  int right;

  if(i == 0) {
    left = 4;
  } else {
    left = cells[i-1];
  }

  if(i == n - 1) {
    right = 4;
  } else {
    right = cells[i+1];
  }

  return state_transition[cells[i]][left][right];
}

void run_soldier(void* data)
{
  int i = (int)data;
  int next;

  // Set initial state:

  if (i == 0) {
    // The general
    cells[i] = R;
  } else {
    // Regular soldiers
    cells[i] = Q;
  }

  binary_semaphore_up(&soldier_ready[i]);

  while(cells[i] != F) {
    binary_semaphore_down(&soldier_waiting[i]);

    // Calculate the next state:
    next = next_state(i);

    // Sanity checking:
    if(next == X) {
      printf(2, "Error, invalid state transition!\n");
      exit();
    }

    binary_semaphore_up(&soldier_prepared[i]);
    binary_semaphore_down(&soldier_update[i]);

    cells[i] = next;

    binary_semaphore_up(&soldier_ready[i]);
  }
}

void print_cells()
{
  int i;
  for(i = 0; i < n; ++i) {
    switch(cells[i]) {
      case Q:
        printf(2, "Q");
        break;
      case L:
        printf(2, "L");
        break;
      case R:
        printf(2, "R");
        break;
      case S:
        printf(2, "S");
        break;
      case F:
        printf(2, "F");
        break;
    }
  }
  printf(2, "\n");
}

int all_firing()
{
  int i;

  for(i = 0; i < n; ++i) {
    if(cells[i] != F) {
      return 0;
    }
  }

  return 1;
}

int
main(int argc, char *argv[])
{
  int i;

  if(argc != 2) {
    printf(2, "You must pass the number of soldiers\n");
    exit();
  }

  n = atoi(argv[1]);

  if(n < 1) {
    printf(2, "Number of soldiers must be positive\n");
    exit();
  }

  uthread_init();

  for(i = 0; i < n; ++i) {
    binary_semaphore_init(&soldier_ready[i], 0);
    binary_semaphore_init(&soldier_waiting[i], 0);
    binary_semaphore_init(&soldier_prepared[i], 0);
    binary_semaphore_init(&soldier_update[i], 0);
  }

  for(i = 0; i < n; ++i) {
    uthread_create(run_soldier, (void*)i);
  }

  for(;;) {
    for(i = 0; i < n; ++i) {
      binary_semaphore_down(&soldier_ready[i]);
    }

    print_cells();
    if(all_firing()) {
      exit();
    }

    for(i = 0; i < n; ++i) {
      binary_semaphore_up(&soldier_waiting[i]);
    }

    for(i = 0; i < n; ++i) {
      binary_semaphore_down(&soldier_prepared[i]);
    }

    for(i = 0; i < n; ++i) {
      binary_semaphore_up(&soldier_update[i]);
    }
  }
}
