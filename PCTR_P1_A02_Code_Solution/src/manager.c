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

#include <errno.h>
#include <linux/limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>

#include <definitions.h>

/* Total number of processes */
int g_nProcesses;        
/* 'Process table' (child processes) */
struct TProcess_t *g_process_table; 

/* Process management */
void create_processes(const char *filename, const char *pattern);
void create_processes_by_class(enum ProcessClass_t class, int n_new_processes, int index_process_table,
			       const char* line, const char* line_number_str, const char *pattern);
pid_t create_single_process(const char *path, const char *str_process_class,
			    const char* line, const char* line_number_str, const char *pattern);
void get_str_process_info(enum ProcessClass_t class, char **path, char **str_process_class);
void init_process_table(int n_processes_pattern, int n_processes_counter);
void terminate_processes(void);
void wait_processes();

/* Auxiliar functions */
void free_resources();
void install_signal_handler();
void parse_argv(int argc, char *argv[], char **filename, char **pattern, int *lines);
void signal_handler(int signo);

/******************** Main function ********************/

int main(int argc, char *argv[]) {
  char *filename = NULL, *pattern = NULL;
  int lines = 0;

  parse_argv(argc, argv, &filename, &pattern, &lines);
  install_signal_handler();

  init_process_table(lines, lines);
  create_processes(filename, pattern);
  wait_processes();

  printf("\n[MANAGER] Program termination (all the processes terminated).\n");
  free_resources();

  return EXIT_SUCCESS;
}

/******************** Process management ********************/

void create_processes(const char *filename, const char *pattern) {
  FILE *fp;
  char line[PATH_MAX], line_number_str[3];
  int line_number = 0;

  if ((fp = fopen(filename, "r")) == NULL) { 
    fprintf(stderr, "Error opening file %s\n", filename); 
    exit(EXIT_FAILURE); 
  } 
    
  while (fgets(line, sizeof(line), fp) != NULL) { 
    sprintf(line_number_str, "%d", line_number);
    create_processes_by_class(PATTERN, 1, line_number * 2, line, line_number_str, pattern);
    create_processes_by_class(COUNTER, 1, line_number * 2 + 1, line, line_number_str, NULL);
    line_number++;
  }

  printf("[MANAGER] %d processes created.\n", line_number * 2);
  sleep(1);

  fclose(fp);
}

void create_processes_by_class(enum ProcessClass_t class, int n_new_processes, int index_process_table,const char* line, const char* line_number_str, const char *pattern) {
  char *path = NULL, *str_process_class = NULL;
  int i;
  pid_t pid;

  get_str_process_info(class, &path, &str_process_class);

  for (i = index_process_table; i < (index_process_table + n_new_processes); i++) {
    pid = create_single_process(path, str_process_class, line, line_number_str, pattern);

    g_process_table[i].class = class;
    g_process_table[i].pid = pid;
    g_process_table[i].str_process_class = str_process_class;
  }
}

pid_t create_single_process(const char *path, const char *str_process_class,
			    const char* line, const char* line_number_str, const char *pattern) {
  pid_t pid;

  switch (pid = fork()) {
  case -1 :
    fprintf(stderr, "[MANAGER] Error creating %s process: %s.\n", 
	    str_process_class, strerror(errno));
    terminate_processes();
    free_resources();
    exit(EXIT_FAILURE);
    /* Child process */
  case 0 : 
    if (execl(path, str_process_class, line, line_number_str, pattern, NULL) == -1) {
      fprintf(stderr, "[MANAGER] Error using execl() in %s process: %s.\n", 
	      str_process_class, strerror(errno));
      exit(EXIT_FAILURE);
    }
  }

  /* Parent process */
  return pid;
}

void get_str_process_info(enum ProcessClass_t class, char **path, char **str_process_class) {
  switch (class) {
  case PATTERN:
    *path = PATTERN_PATH;
    *str_process_class = PATTERN_CLASS;
    break;
  case COUNTER:
    *path = COUNTER_PATH;
    *str_process_class = COUNTER_CLASS;
    break;
  }
}

void init_process_table(int n_processes_pattern, int n_processes_counter) {
  
  int i;

  g_nProcesses = n_processes_counter+n_processes_pattern; 
  g_process_table=malloc(sizeof(struct TProcess_t)*g_nProcesses);

  for(i=0;i<g_nProcesses;i++){
   g_process_table[i].pid=0;
  }

}

void terminate_processes(void) {

  int i;
  printf("MANAGER Terminating processes");
  for(i=0;i<g_nProcesses;i++){
    if(g_process_table[i].pid != 0){
      printf("[MANAGER] Process %s terminated [%d]...\n",
      g_process_table[i].str_process_class,g_process_table[i].pid);
      if(kill(g_process_table[i].pid,SIGINT) == -1){
        fprintf(stderr,"Error using the kill() function");
      }
    }
  } 
  
}

void wait_processes() {

  int i = 0;
  int j;
  int pid;

  while(i<g_nProcesses){

    pid = wait(NULL);

    for(j=0;j<g_nProcesses;j++){
      if(g_process_table[j].pid == pid){
        g_process_table[j].pid = 0;
        break;
      }
    }

    i++;
  }

}

/******************** Auxiliar functions ********************/

void free_resources() {
  /* Free the 'process table' memory */
  free(g_process_table); 
}

void install_signal_handler() {
  if (signal(SIGINT, signal_handler) == SIG_ERR) {
    fprintf(stderr, "[MANAGER] Error installing signal handler: %s.\n", 
	    strerror(errno));    
    exit(EXIT_FAILURE);
  }
}

void parse_argv(int argc, char *argv[], char **filename, char **pattern, int *lines) {
  FILE *fp; 
  int ch;
  
  if (argc != 3) {
    fprintf(stderr, "Error. Use: ./exec/manager <file> <pattern>.\n");    
    exit(EXIT_FAILURE); 
  }

  *filename = argv[1];
  *pattern = argv[2];

  if ((fp = fopen(*filename, "r")) == NULL) { 
    fprintf(stderr, "Error opening file %s\n", *filename); 
    exit(EXIT_FAILURE); 
  } 
 
  while ((ch = fgetc(fp)) != EOF) { 
    if (ch == '\n') { 
      ++*lines; 
    } 
  } 
 
  fclose(fp);
}

void signal_handler(int signo) {
  printf("\n[MANAGER] Program termination (Ctrl + C).\n");
  terminate_processes();
  free_resources();
  exit(EXIT_SUCCESS);
}
