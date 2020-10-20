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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <definitions.h>
#include <semaphoreI.h>

/* Semaphores and shared memory retrieval */
void close_shared_memory_segments(int shm_data, int shm_task);
void get_shm_segments(int *shm_data, int *shm_task, struct TData_t **p_data, struct TTask_t **p_task);

void get_sems(sem_t **p_sem_task_ready, sem_t **p_sem_task_read, sem_t **p_sem_task_processed);

/* Task management */
void get_and_process_task(sem_t *sem_task_ready, sem_t *sem_task_read, 
			  struct TData_t *data, const struct TTask_t *task);
void notify_task_completed(sem_t *sem_task_processed);

/* Auxiliar functions */
int how_many_times_divisible(int number, int prime);

/******************** Main function ********************/

int main(int argc, char *argv[]) {
  struct TData_t *data;
  struct TTask_t *task;
  int shm_data, shm_task;
  sem_t *sem_task_ready, *sem_task_read, *sem_task_processed;

  /* Get shared memory segments and semaphores */
  get_shm_segments(&shm_data, &shm_task, &data, &task);
  get_sems(&sem_task_ready, &sem_task_read, &sem_task_processed);

  /* One single iteration */
  get_and_process_task(sem_task_ready, sem_task_read, data, task);
  notify_task_completed(sem_task_processed);

  close_shared_memory_segments(shm_data, shm_task);

  return EXIT_SUCCESS;
}

/******************** Semaphores and shared memory retrieval ********************/

void close_shared_memory_segments(int shm_data, int shm_task) {
  close(shm_data);
  close(shm_task);
}

void get_shm_segments(int *shm_data, int *shm_task, struct TData_t **data, struct TTask_t **task) {
  *shm_data = shm_open(SHM_DATA, O_RDWR, 0644);
  *data = mmap(NULL, sizeof(struct TData_t), PROT_READ | PROT_WRITE, MAP_SHARED, *shm_data, 0);

  *shm_task = shm_open(SHM_TASK, O_RDWR, 0644);
  *task = mmap(NULL, sizeof(struct TTask_t), PROT_READ | PROT_WRITE, MAP_SHARED, *shm_task, 0);
}

void get_sems(sem_t **p_sem_task_ready, sem_t **p_sem_task_read, sem_t **p_sem_task_processed) {
  *p_sem_task_ready = get_semaphore(SEM_TASK_READY);
  *p_sem_task_read = get_semaphore(SEM_TASK_READ);
  *p_sem_task_processed = get_semaphore(SEM_TASK_PROCESSED);
}

/******************** Task management ********************/
void get_and_process_task(sem_t *sem_task_ready, sem_t *sem_task_read, struct TData_t *data, const struct TTask_t *task){
  
  int prime_number, prime_number_position, numerator, denominator, xnumerator, xdenominator;
 
  wait_semaphore(sem_task_ready);
  prime_number = task->prime_number;
  prime_number_position = task->prime_number_position;
  numerator = data->numerator;
  denominator = data->denominator; 
  signal_semaphore(sem_task_read);

  if((xnumerator=how_many_times_divisible(numerator,prime_number)) > (xdenominator = how_many_times_divisible(denominator,prime_number)) ){
    data->numerator_exponents[prime_number_position] = xnumerator-xdenominator;
    data->denominator_exponents[prime_number_position] = 0;
  }else{
    data->denominator_exponents[prime_number_position] = xdenominator-xnumerator;
    data->numerator_exponents[prime_number_position] = 0;
  }
  
}
void notify_task_completed(sem_t *sem_task_processed){
  signal_semaphore(sem_task_processed);
}
/******************** Auxiliar functions ********************/

int how_many_times_divisible(int number, int prime) {
  int times;

  for (times = 0; !(number % prime); times++, number = (number / prime));

  return times;
}
