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
#include <fcntl.h>
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <definitions.h>

/* Message queue management */
void open_message_queue(const char *mq_name, mode_t mode, mqd_t *q_handler);

/* Task management */
void process_line(struct MsgResult_t *partial_results, mqd_t q_handler_lines, 
		  mqd_t q_handler_mutex, mqd_t q_handler_words, mqd_t q_handler_number_digits);
void send_partial_results(struct MsgResult_t *partial_results, mqd_t q_handler_results);

/******************** Main function ********************/

int main(int argc, char *argv[]) {
  mqd_t q_handler_lines, q_handler_results;
  mqd_t q_handler_words, q_handler_number_digits, q_handler_mutex;
  mode_t mode_read_only = O_RDONLY;
  mode_t mode_write_only = O_WRONLY;
  mode_t mode_read_write = O_RDWR;
  struct MsgResult_t partial_results;
  
  /* Open message queues */
  open_message_queue(MQ_LINES, mode_read_only, &q_handler_lines);
  open_message_queue(MQ_RESULTS, mode_write_only, &q_handler_results);
  open_message_queue(MQ_WORDS, mode_write_only, &q_handler_words);
  open_message_queue(MQ_NUMBER_DIGITS, mode_read_only, &q_handler_number_digits);
  open_message_queue(MQ_MUTEX, mode_read_write, &q_handler_mutex);

  /* Task management */
  while (TRUE) {
    process_line(&partial_results, q_handler_lines, q_handler_mutex, q_handler_words, q_handler_number_digits);
    send_partial_results(&partial_results, q_handler_results);
  }

  return EXIT_SUCCESS;
}

/******************** Message queue management ********************/

void open_message_queue(const char *mq_name, mode_t mode, mqd_t *q_handler) {
  *q_handler = mq_open(mq_name, mode);
}

/******************** Task management ********************/

void process_line(struct MsgResult_t *partial_results, mqd_t q_handler_lines, mqd_t q_handler_mutex, mqd_t q_handler_words, mqd_t q_handler_number_digits) {
  
  char *word, *pattern, word_copy[MAX_LINE_SIZE], token;
  int n_words = 0, n_digits = 0;
  struct MsgLine_t msg_line;

  /* Initialize number of digits */
  partial_results->n_digits = 0;

  /* Wait for a new task*/
  mq_receive(q_handler_lines, (char *)&msg_line, sizeof(struct MsgLine_t), NULL);
  pattern = msg_line.pattern;
  
  /* Word processing */
  word = strtok(msg_line.line, WORD_SEPARATOR);

  while (word != NULL) {
    
    /* Start with 'pattern'? */
    if (strncmp(word, pattern, strlen(pattern)) == 0) {
      n_words++;

      /* Extra copy to avoid inconsistencies*/ 
      strcpy(word_copy, word);

      /* Mutex to guarantee the exclusive use of the COUNTER */
      mq_receive(q_handler_mutex, &token, sizeof(char), NULL);
        /* Rendezvous with the counter process */
      mq_send(q_handler_words, word_copy, sizeof(word_copy), 0);
	    mq_receive(q_handler_number_digits, (char *)&n_digits, sizeof(int), NULL);
      
      mq_send(q_handler_mutex, &token, sizeof(char), 0);

      /* Update the number of digits for the processed line */
      partial_results->n_digits += n_digits;

      printf("[PROCESSOR %d]: '%s' found in '%s' with %d digits\n", getpid(), pattern, word, n_digits);
    }

    word = strtok(NULL, WORD_SEPARATOR);
  }

  /* Dont remove (simulates complexity) */
  sleep(1);

  /* Update the number of words */
  partial_results->n_words = n_words;
}

void send_partial_results(struct MsgResult_t *partial_results, mqd_t q_handler_results) {
  mq_send(q_handler_results, (const char *)partial_results, sizeof(struct MsgResult_t), 0);
}
