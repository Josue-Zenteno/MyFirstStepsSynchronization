/*
====================================================================
Concurrent and Real-Time Programming
Faculty of Computer Science
University of Castilla-La Mancha (Spain)

Contact info: http://www.libropctr.com

You can redistribute and/or modify this file under the terms of the
GNU General Public License ad published by the Free Software
Foundation, either version 3 of the License, or (at your option) and
later version. See <http://www.gnu.org/licenses/>.

This file is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.
====================================================================
*/

#define _POSIX_SOURCE
#define _BSD_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <definitions.h>
#include <semaphoreI.h>

/* Total number of processes */
int g_nProcesses;
/* 'Process table' (child processes) */
struct TProcess_t *g_process_table;

/* First n prime numbers */
int g_primes[] = {
  2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67,
  71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157,
  163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251,
  257, 263, 269, 271, 277, 281, 283, 293, 307, 311, 313, 317, 331, 337, 347, 349, 353,
  359, 367, 373, 379, 383, 389, 397, 401, 409, 419, 421, 431, 433, 439, 443, 449, 457,
  461, 463, 467, 479, 487, 491, 499, 503, 509, 521, 523, 541, 547
}; 

/* Process management */

void create_processes_by_class(enum ProcessClass_t class, int n_processes, int index_process_table);
pid_t create_single_process(const char *class, const char *path, const char *argv);
void get_str_process_info(enum ProcessClass_t class, char **path, char **str_process_class);
void init_process_table(int n_factorers);
void terminate_processes();
void wait_processes();

/* Semaphores and shared memory management */

void create_shm_segments(int *shm_data, int *shm_task,
			 struct TData_t **p_data, struct TTask_t **p_task, 
			 int numerator, int denominator, int n_prime_numbers);

void create_sems(sem_t **p_sem_task_ready, sem_t **p_sem_task_read, sem_t **p_sem_task_processed);

void close_shared_memory_segments(int shm_data, int shm_task);

/* Task management */

void notify_tasks(sem_t *sem_task_ready, sem_t *sem_task_read, struct TTask_t *task, int n_tasks);
void wait_tasks_termination(sem_t *sem_task_processed, int n_tasks);

/* Auxiliar functions */

void free_resources();
void install_signal_handler();
void parse_argv(int argc, char *argv[], int *numerator, int *denominator);
void print_result(struct TData_t *data);
void signal_handler(int signo);

/******************** Main function ********************/

int main(int argc, char *argv[]) {
  struct TData_t *data;
  struct TTask_t *task;
  int shm_data, shm_task;
  sem_t *sem_task_ready, *sem_task_read, *sem_task_processed;

  int numerator, denominator;

  /* Install signal handler and parse arguments*/
  install_signal_handler();
  parse_argv(argc, argv, &numerator, &denominator);

  /* Init the process table*/
  init_process_table(N_PRIME_NUMBERS);

  /* Create shared memory segments and semaphores */
  create_shm_segments(&shm_data, &shm_task, &data, &task, numerator, denominator, N_PRIME_NUMBERS);
  create_sems(&sem_task_ready, &sem_task_read, &sem_task_processed);

  /* Create processes */
  create_processes_by_class(FACTORER, N_PRIME_NUMBERS, 0);

  /* Manage tasks */
  notify_tasks(sem_task_ready, sem_task_read, task, N_PRIME_NUMBERS);
  wait_tasks_termination(sem_task_processed, N_PRIME_NUMBERS);

  /* Wait for child processes */
  wait_processes();

  /* Print the obtained result */
  print_result(data);

  /* Free resources and terminate */
  close_shared_memory_segments(shm_data, shm_task);
  free_resources();

  return EXIT_SUCCESS;
}

/******************** Process Management ********************/

void create_processes_by_class(enum ProcessClass_t class, int n_processes, int index_process_table) {
  char *path = NULL, *str_process_class = NULL;
  int i;
  pid_t pid;

  get_str_process_info(class, &path, &str_process_class);

  for (i = index_process_table; i < (index_process_table + n_processes); i++) {
    pid = create_single_process(path, str_process_class, NULL);

    g_process_table[i].class = class;
    g_process_table[i].pid = pid;
    g_process_table[i].str_process_class = str_process_class;
  }

  printf("[MANAGER] %d %s processes created.\n", n_processes, str_process_class);
  sleep(1);
}

pid_t create_single_process(const char *path, const char *class, const char *argv) {
  pid_t pid;

  switch (pid = fork()) {
  case -1 :
    fprintf(stderr, "[MANAGER] Error creating %s process: %s.\n", 
	    class, strerror(errno));
    terminate_processes();
    free_resources();
    exit(EXIT_FAILURE);
  /* Child process */
  case 0 : 
    if (execl(path, class, argv, NULL) == -1) {
      fprintf(stderr, "[MANAGER] Error using execl() in %s process: %s.\n", 
	      class, strerror(errno));
      exit(EXIT_FAILURE);
    }
  }

  /* Child PID */
  return pid;
}

void get_str_process_info(enum ProcessClass_t class, char **path, char **str_process_class) {
  switch (class) {
  case FACTORER:
    *path = FACTORER_PATH;
    *str_process_class = FACTORER_CLASS;
    break;
  }
}

void init_process_table(int n_factorers) {
  int i;

  /* Number of processes to be created */
  g_nProcesses = n_factorers;
  /* Allocate memory for the 'process table' */
  g_process_table = malloc(g_nProcesses * sizeof(struct TProcess_t)); 

  /* Init the 'process table' */
  for (i = 0; i < g_nProcesses; i++) {
    g_process_table[i].pid = 0;
  }
}

void terminate_processes() {
  int i;
  
  printf("\n----- [MANAGER] Terminating running child processes ----- \n");
  for (i = 0; i < g_nProcesses; i++) {
    /* Child process alive */
    if (g_process_table[i].pid != 0) { 
      if (kill(g_process_table[i].pid, SIGINT) == -1) {
	fprintf(stderr, "[MANAGER] Error using kill() on process %d: %s.\n", 
		g_process_table[i].pid, strerror(errno));
      }
    }
  }
}

void wait_processes() {
  int i, n_processes = g_nProcesses;
  pid_t pid;

  /* Wait for the termination of FACTORER processes */
  while (n_processes > 0) {
    /* Wait for any FACTORER process */
    pid = wait(NULL); 
    for (i = 0; i < g_nProcesses; i++) {
      if (pid == g_process_table[i].pid) {
	/* Update the 'process table' */
        g_process_table[i].pid = 0;           
	n_processes--; 
	/* Child process found */
        break; 
      }
    }
  }
}

/******************** Semaphores and shared memory management ********************/

void create_shm_segments(int *shm_data, int *shm_task,
			 struct TData_t **p_data, struct TTask_t **p_task, 
			 int numerator, int denominator, int n_prime_numbers) {
  int i;

  /* Create and initialize shared memory segments */
  *shm_data = shm_open(SHM_DATA, O_CREAT | O_RDWR, 0644); 
  ftruncate(*shm_data, sizeof(struct TData_t));          
  *p_data = mmap(NULL, sizeof(struct TData_t), PROT_READ | PROT_WRITE, MAP_SHARED, *shm_data, 0);

  *shm_task = shm_open(SHM_TASK, O_CREAT | O_RDWR, 0644);
  ftruncate(*shm_task, sizeof(struct TTask_t));
  *p_task = mmap(NULL, sizeof(struct TTask_t), PROT_READ | PROT_WRITE, MAP_SHARED, *shm_task, 0);

  /* SHM data initialization */
  (*p_data)->numerator = numerator;
  (*p_data)->denominator = denominator;
  for (i = 0; i < n_prime_numbers; i++) {
    (*p_data)->numerator_exponents[i] = 0;
    (*p_data)->denominator_exponents[i] = 0;
  }
}

void create_sems(sem_t **p_sem_task_ready, sem_t **p_sem_task_read, sem_t **p_sem_task_processed) {  
  /* Create and initialize semaphores */
  *p_sem_task_ready = create_semaphore(SEM_TASK_READY,0); 
  *p_sem_task_read = create_semaphore(SEM_TASK_READ,0);
  *p_sem_task_processed = create_semaphore(SEM_TASK_PROCESSED,0);
}

void close_shared_memory_segments(int shm_data, int shm_task) {
  close(shm_data);
  close(shm_task);
}

/******************** Task management ********************/

void notify_tasks(sem_t *sem_task_ready, sem_t *sem_task_read, struct TTask_t *task, int n_tasks) {
  int i;

  for (i = 0; i < n_tasks; i++) {
    task->prime_number = g_primes[i];
    task->prime_number_position = i;
    /* Task notification through rendezvous */
    signal_semaphore(sem_task_ready);
    wait_semaphore(sem_task_read);
  }

}

void wait_tasks_termination(sem_t *sem_task_processed, int n_tasks) {
 int n_tasks_processed = 0;

  while (n_tasks_processed < n_tasks) {
    wait_semaphore(sem_task_processed);                         
    n_tasks_processed++;   
  }
}

/******************** Auxiliar functions ********************/

void free_resources() {
  printf("\n----- [MANAGER] Freeing resources ----- \n");

  /* Free the 'process table' memory */
  free(g_process_table); 

  /* Semaphores */ 
  remove_semaphore(SEM_TASK_READY);
  remove_semaphore(SEM_TASK_READ);
  remove_semaphore(SEM_TASK_PROCESSED);

  /* Shared memory segments*/
  shm_unlink(SHM_TASK);
  shm_unlink(SHM_DATA);
}

void install_signal_handler() {
  if (signal(SIGINT, signal_handler) == SIG_ERR) {
    fprintf(stderr, "[MANAGER] Error installing signal handler: %s.\n", strerror(errno));    
    exit(EXIT_FAILURE);
  }
}

void parse_argv(int argc, char *argv[], int *numerator, int *denominator) {
  if (argc != 3) {
    fprintf(stderr, "Synopsis: ./exec/manager <numerator> <denominator>.\n");    
    exit(EXIT_FAILURE); 
  }
  
  *numerator = atoi(argv[1]);
  *denominator = atoi(argv[2]);
}

void print_result(struct TData_t *data) {
  int i, n_prime_numbers;

  n_prime_numbers = sizeof(g_primes) / sizeof(g_primes[0]);

  printf("\nResult: ( ");
  for (i = 0; i < n_prime_numbers; i++) {
    if (data->numerator_exponents[i] > 0) {
      printf("%d^%d ", g_primes[i], data->numerator_exponents[i]);
    }
  }
  printf(")/( ");
  for (i = 0; i < n_prime_numbers; i++) {
    if (data->denominator_exponents[i] > 0) {
      printf("%d^%d ", g_primes[i], data->denominator_exponents[i]);
    }
  }
  printf(")\n");
}

void signal_handler(int signo) {
  printf("\n[MANAGER] Program termination (Ctrl + C).\n");
  terminate_processes();
  free_resources();
  exit(EXIT_SUCCESS);
}
