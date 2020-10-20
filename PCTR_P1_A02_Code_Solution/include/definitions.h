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

#define PATTERN_CLASS "PATTERN"
#define PATTERN_PATH "./exec/pattern"
#define COUNTER_CLASS "COUNTER"
#define COUNTER_PATH "./exec/counter"

/* Process class */
enum ProcessClass_t {PATTERN, COUNTER}; 

/* Process info */
struct TProcess_t {          
  enum ProcessClass_t class; /* PATTERN or COUNTER */
  pid_t pid;                 /* Process ID */
  char *str_process_class;   /* String representation of the process class */
};
