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

#define SEM_TASK_READY     "sem_task_ready"
#define SEM_TASK_READ      "sem_task_read"
#define SEM_TASK_PROCESSED "sem_task_processed"
#define SHM_TASK           "shm_task"
#define SHM_DATA           "shm_data"

#define FACTORER_CLASS     "FACTORER"
#define FACTORER_PATH      "./exec/factorer"

#define N_PRIME_NUMBERS      101

struct TData_t {
  /* Numerator/denominator to factor */
  int numerator;
  int denominator;
  /* Exponents for each prime number of the numerator */
  int numerator_exponents[N_PRIME_NUMBERS];
  /* Exponents for each prime number of the denominator */
  int denominator_exponents[N_PRIME_NUMBERS];
};

struct TTask_t {
  /* Actual value of the prime number*/
  int prime_number;
  /* Position of the prime number within the list */
  int prime_number_position;
};

enum ProcessClass_t {FACTORER}; 

struct TProcess_t {          
  enum ProcessClass_t class; /* FACTORER */
  pid_t pid;                 /* Process ID */
  char *str_process_class;   /* String representation of the process class */
};
