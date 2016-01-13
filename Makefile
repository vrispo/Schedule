#---------------------------------------------------
# Target file to be compiled by default
#------ ---------------------------------------------
MAIN = schedule
#---------------------------------------------------
# CC will be the compiler to use
#---------------------------------------------------
CC = gcc
#---------------------------------------------------
# CC options
#---------------------------------------------------
CFLAGS = -Wall -g
CLIBS = -lpthread -lrt -lm -lalleg
#---------------------------------------------------
# Additional programs
#---------------------------------------------------
RM = rm
#---------------------------------------------------
# Dependencies
#---------------------------------------------------
$(MAIN) : $(MAIN).o taskRT.o timeplus.o
	$(CC) $(CFLAGS) -o $(MAIN) $(MAIN).o taskRT.o timeplus.o $(CLIBS)

$(MAIN).o: $(MAIN).c
	$(CC) $(CFLAGS) -c $(MAIN).c

taskRT.o: taskRT.c
	$(CC) $(CFLAGS) -c taskRT.c

timeplus.o: timeplus.c
	$(CC) $(CFLAGS) -c timeplus.c

clean:
	$(RM) -rf $(MAIN) *.o