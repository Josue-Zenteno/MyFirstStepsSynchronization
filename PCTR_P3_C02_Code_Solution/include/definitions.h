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

#define MQ_LINES         "/mq_lines"
#define MQ_RESULTS       "/mq_results"
#define MQ_WORDS         "/mq_words"
#define MQ_NUMBER_DIGITS "/mq_number_digits"
#define MQ_MUTEX         "/mq_mutex"

#define PROCESSOR_CLASS   "PROCESSOR"
#define PROCESSOR_PATH    "./exec/processor"
#define COUNTER_CLASS     "COUNTER"
#define COUNTER_PATH      "./exec/counter"

#define MAX_LINE_SIZE 255
#define NUM_COUNTERS 1
#define WORD_SEPARATOR " "
#define TRUE 1
#define FALSE 0

/* Used in MQ_LINES */
struct MsgLine_t {
  char line[MAX_LINE_SIZE];
  char pattern[MAX_LINE_SIZE];
};

/* Used in MQ_RESULTS */
struct MsgResult_t {
  int n_words;
  int n_digits;
};

enum ProcessClass_t {PROCESSOR, COUNTER}; 

struct TProcess_t {          
  enum ProcessClass_t class; /* PROCESSOR or COUNTER */
  pid_t pid;                 /* Process ID */
  char *str_process_class;   /* String representation of the process class */
};
