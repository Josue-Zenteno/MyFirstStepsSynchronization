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

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Program logic */
void run(char *line, int line_number);

/* Auxiliar functions */
void install_signal_handler();
void parse_argv(int argc, char *argv[], char **line, int *line_number);
void signal_handler(int signo);

/******************** Main function ********************/

int main (int argc, char *argv[]) {
  char *line = NULL;
  int line_number;
     
  install_signal_handler(); 
  parse_argv(argc, argv, &line, &line_number);

  run(line, line_number);

  return EXIT_SUCCESS;
}

/******************** Program logic ********************/

void run(char *line, int line_number) {
  int n_words = 0, inside_word = 0; 
  const char *it = line;

  do {
    switch (*it) {
    case '\0': 
    case ' ': case '\t': case '\n': case '\r':
      if (inside_word) { 
	      inside_word = 0; 
      	n_words++; 
      }
      break;
    default: 
      inside_word = 1;
    }
  } while(*it++);

  printf("[COUNTER %d] The line '%d' has %d words\n", getpid(), line_number, n_words);
}

/******************** Auxiliar functions ********************/

void install_signal_handler() {
  if (signal(SIGINT, signal_handler) == SIG_ERR) {
    fprintf(stderr, "[PA %d] Error installing handler: %s.\n", 
	    getpid(), strerror(errno));    
    exit(EXIT_FAILURE);
  }
}

void parse_argv(int argc, char *argv[], char **line, int *line_number) {
  if (argc != 3) {
    fprintf(stderr, "[COUNTER %d] Error in the command line.\n", getpid());
    exit(EXIT_FAILURE);
  }

  *line = argv[1];
  *line_number = atoi(argv[2]);
}

void signal_handler(int signo) {
  printf("[COUNTER %d] terminated (SIGINT).\n", getpid());
  exit(EXIT_SUCCESS);
}
