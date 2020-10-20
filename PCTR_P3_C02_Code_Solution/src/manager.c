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
#include <mqueue.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <definitions.h>

/* Total number of processes */
int g_nProcesses;
/* 'Process table' (child processes) */
struct TProcess_t *g_process_table;

/* Process management */
void create_processes_by_class(enum ProcessClass_t class, int n_processes, int index_process_table);
pid_t create_single_process(const char *class, const char *path, const char *argv);
void get_str_process_info(enum ProcessClass_t class, char **path, char **str_process_class);
void init_process_table(int n_processors, int n_counters);
void terminate_processes();

/* Message queue management */
void create_message_queue(const char *mq_name, mode_t mode, long mq_maxmsg, long mq_msgsize, mqd_t *q_handler);
void close_message_queues(mqd_t q_handler_lines, mqd_t q_handler_results,  mqd_t q_handler_words, mqd_t q_handler_number_digits, mqd_t q_handler_mutex);

/* Task management */
void send_lines(const char *filename, char *pattern, int *n_lines, mqd_t q_handler_lines);
void receive_partial_results(int n_lines, struct MsgResult_t *global_results, mqd_t q_handler_results);

/* Auxiliar functions */
void free_resources();
void install_signal_handler();
void parse_argv(int argc, char *argv[], int *n_processors, char **p_pattern, char **p_filename);
void print_result(struct MsgResult_t *global_results);
void signal_handler(int signo);

/******************** Main function ********************/

int main(int argc, char *argv[]) {
  mqd_t q_handler_lines, q_handler_results;
  mqd_t q_handler_words, q_handler_number_digits, q_handler_mutex;
  mode_t mode_creat_only = O_CREAT;
  mode_t mode_creat_read_only = (O_RDONLY | O_CREAT);
  mode_t mode_creat_write_only = (O_WRONLY | O_CREAT);
  struct MsgResult_t global_results;
  global_results.n_words = global_results.n_digits = 0;
  
  char *pattern, *filename, token;
  int n_processors, n_lines = 0;

  /* Install signal handler and parse arguments*/
  install_signal_handler();
  parse_argv(argc, argv, &n_processors, &pattern, &filename);

  /* Init the process table*/
  init_process_table(n_processors, NUM_COUNTERS);

  /* Create message queues */
  create_message_queue(MQ_LINES, mode_creat_write_only, n_processors, sizeof(struct MsgLine_t), &q_handler_lines);
  create_message_queue(MQ_RESULTS, mode_creat_read_only, n_processors,sizeof(struct MsgResult_t), &q_handler_results);
  create_message_queue(MQ_WORDS, mode_creat_only, 1, MAX_LINE_SIZE * sizeof(char), &q_handler_words);
  create_message_queue(MQ_NUMBER_DIGITS, mode_creat_only, 1,sizeof(int), &q_handler_number_digits);
  create_message_queue(MQ_MUTEX, mode_creat_write_only, 1,sizeof(char), &q_handler_mutex);  
  /* Init the mutex mailbox */
  mq_send(q_handler_mutex, &token, sizeof(char), 0);

  /* Create processes */
  create_processes_by_class(PROCESSOR, n_processors, 0);
  create_processes_by_class(COUNTER, NUM_COUNTERS, n_processors);

  /* Manage tasks */
  send_lines(filename, pattern, &n_lines, q_handler_lines);
  receive_partial_results(n_lines, &global_results, q_handler_results);

  /* Print the decoded text */
  print_result(&global_results);

  /* Free resources and terminate */
  close_message_queues(q_handler_lines, q_handler_results, q_handler_words, q_handler_number_digits, q_handler_mutex);
  terminate_processes();
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
  case PROCESSOR:
    *path = PROCESSOR_PATH;
    *str_process_class = PROCESSOR_CLASS;
    break;    
  case COUNTER:
    *path = COUNTER_PATH;
    *str_process_class = COUNTER_CLASS;
    break;
  }
}

void init_process_table(int n_processors, int n_counters) {
  int i;

  /* Number of processes to be created */
  g_nProcesses = n_processors + n_counters; 
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
      printf("[MANAGER] Terminating %s process [%d]...\n", 
	     g_process_table[i].str_process_class, g_process_table[i].pid);
      if (kill(g_process_table[i].pid, SIGINT) == -1) {
	fprintf(stderr, "[MANAGER] Error using kill() on process %d: %s.\n", 
		g_process_table[i].pid, strerror(errno));
      }
    }
  }
}

/******************** Message queue management ********************/

void create_message_queue(const char *mq_name, mode_t mode, long mq_maxmsg, long mq_msgsize, mqd_t *q_handler) {
  struct mq_attr attr;

  attr.mq_maxmsg = mq_maxmsg;
  attr.mq_msgsize = mq_msgsize;
  *q_handler = mq_open(mq_name, mode, S_IWUSR | S_IRUSR, &attr);
  
}

void close_message_queues(mqd_t q_handler_lines, mqd_t q_handler_results,  mqd_t q_handler_words,mqd_t q_handler_number_digits, mqd_t q_handler_mutex) {
  mq_close(q_handler_lines);
  mq_close(q_handler_results);
  mq_close(q_handler_words);
  mq_close(q_handler_number_digits);
  mq_close(q_handler_mutex);
}

/******************** Task management ********************/

void send_lines(const char *filename, char *pattern, int *n_lines, mqd_t q_handler_lines) {
  FILE *fp;
  char line[MAX_LINE_SIZE];
  struct MsgLine_t msg_line;

  /* Open the file */
  if ((fp = fopen(filename, "r")) == NULL) {
    fprintf (stderr, "\n----- [MANAGER] Error opening %s ----- \n\n", filename);
    terminate_processes();
    free_resources();
    exit(EXIT_FAILURE);
  }

  /* Read one line at a time and send tasks */
  while (fgets(line, sizeof(line), fp) != NULL) {
    strcpy(msg_line.line, line);
    strcpy(msg_line.pattern, pattern);
    mq_send(q_handler_lines, (const char *)&msg_line, sizeof(struct MsgLine_t), 0);
    ++*n_lines;
  }

  fclose(fp);
}

void receive_partial_results(int n_lines, struct MsgResult_t *global_results, mqd_t q_handler_results) {
  int i;
  int n_words = 0;
  int n_digits = 0;

  for(i=0;i<n_lines;i++){
    mq_receive(q_handler_results,(char*)global_results,sizeof(struct MsgResult_t),NULL);
    n_words += global_results->n_words;
    n_digits += global_results->n_digits;
  }
  global_results->n_words = n_words;
  global_results->n_digits = n_digits;

}

/******************** Auxiliar functions ********************/

void free_resources() {
  printf("\n----- [MANAGER] Freeing resources ----- \n");

  /* Free the 'process table' memory */
  free(g_process_table); 

  /* Remove message queues */
  mq_unlink(MQ_LINES);
  mq_unlink(MQ_RESULTS);
  mq_unlink(MQ_WORDS);
  mq_unlink(MQ_NUMBER_DIGITS);
  mq_unlink(MQ_MUTEX);
}

void install_signal_handler() {
  if (signal(SIGINT, signal_handler) == SIG_ERR) {
    fprintf(stderr, "[MANAGER] Error installing signal handler: %s.\n", strerror(errno));    
    exit(EXIT_FAILURE);
  }
}

void parse_argv(int argc, char *argv[], int *n_processors, char **p_pattern, char **p_filename) {
  if (argc != 4) {
    fprintf(stderr, "Synopsis: ./exec/manager <n_processors> <pattern> <file>.\n");
    exit(EXIT_FAILURE); 
  }
  
  *n_processors = atoi(argv[1]);  
  *p_pattern = argv[2];
  *p_filename = argv[3];
}

void print_result(struct MsgResult_t *global_results) {
  printf("\n----- [MANAGER] Printing result ----- \n");
  printf("\t%d words -- %d digits\n", global_results->n_words, global_results->n_digits);
}

void signal_handler(int signo) {
  printf("\n[MANAGER] Program termination (Ctrl + C).\n");
  terminate_processes();
  free_resources();
  exit(EXIT_SUCCESS);
}
