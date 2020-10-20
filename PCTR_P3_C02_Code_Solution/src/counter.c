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
#include <ctype.h>


#include <definitions.h>

/* Message queue management */
void open_message_queue(const char *mq_name, mode_t mode, mqd_t *q_handler);
/* Task management */
void count_number(mqd_t q_handler_words, mqd_t q_handler_number_digits);

int main (int argc, char * argv[]){

  mqd_t q_handler_words, q_handler_number_digits;
  mode_t mode_read_only = O_RDONLY;
  mode_t mode_write_only = O_WRONLY;

    /* Open message queues */
  
  open_message_queue(MQ_WORDS, mode_read_only, &q_handler_words);
  open_message_queue(MQ_NUMBER_DIGITS, mode_write_only, &q_handler_number_digits);
  while(TRUE){
    count_number(q_handler_words, q_handler_number_digits);
  }
  return EXIT_SUCCESS;
}

void open_message_queue(const char *mq_name, mode_t mode, mqd_t *q_handler){
    *q_handler = mq_open(mq_name, mode);
}

void count_number(mqd_t q_handler_words, mqd_t q_handler_number_digits){
    
    char word[MAX_LINE_SIZE];
    int i, n_digits;
    n_digits = 0;

    mq_receive(q_handler_words,word,sizeof(word),NULL);

    for (i=0;i<strlen(word);i++){
      if((isdigit(word[i]) != 0)){
        n_digits++;
      }
    }

    mq_send(q_handler_number_digits,(char*)&n_digits,sizeof(int),0);
}
