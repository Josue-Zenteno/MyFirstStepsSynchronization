DIROBJ := obj/
DIREXE := exec/
DIRHEA := include/
DIRSRC := src/

CFLAGS := -I$(DIRHEA) -c -Wall -std=c99
LDLIBS := -pthread -lrt -lm
CC := gcc

all : dirs manager factorer

dirs:
	mkdir -p $(DIROBJ) $(DIREXE)

manager: $(DIROBJ)manager.o $(DIROBJ)semaphoreI.o 
	$(CC) -lm -o $(DIREXE)$@ $^ $(LDLIBS)

factorer: $(DIROBJ)factorer.o $(DIROBJ)semaphoreI.o 
	$(CC) -o $(DIREXE)$@ $^ $(LDLIBS)

$(DIROBJ)%.o: $(DIRSRC)%.c
	$(CC) $(CFLAGS) $^ -o $@

test:
	./exec/manager 995742720 26078976

solution:
	./exec/manager 995742720 2935296

clean : 
	rm -rf *~ core $(DIROBJ) $(DIREXE) $(DIRHEA)*~ $(DIRSRC)*~
