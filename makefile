CC = gcc

CFLAGS += -g -Wall -std=c99 -I ./headers

objects = parse_arguments.o collector.o workers_pool.o queue_utils.o results_utils.o valid_file.o pthread_utils.o master.o farm.o 


.PHONY : clean farm run cleanFiles

cleanAll: 
	-rm -f file*.dat
	-rm -f -R testdir
	-rm -f expected.txt

clean : 
	-rm -f $(objects)

run: farm clean

farm : $(objects) 
	$(CC) $(CFLAGS) $(objects) -o farm
	

workers_pool.o : workers_pool.c 

parse_arguments.o : parse_arguments.c 

master.o : master.c

valid_file.o : valid_file.c

queue_utils.o : queue_utils.c

results_utils.o : results_utils.c

collector.o : collector.c

farm.o : farm.c




