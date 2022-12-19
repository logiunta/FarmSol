CC = gcc

CFLAGS = -g -Wall -std=c99 -I ./headers -pthread

objectsPath = src

generafilePath = tests

objects = $(objectsPath)/parse_arguments.o $(objectsPath)/collector.o $(objectsPath)/workers_pool.o $(objectsPath)/queue_utils.o $(objectsPath)/results_utils.o $(objectsPath)/valid_file.o $(objectsPath)/pthread_utils.o $(objectsPath)/master.o $(objectsPath)/main.o

generafileObj = $(generafilePath)/generafile


.PHONY : clean cleanAll run test 

run: runFarm clean

cleanAll: 
	-rm -f file*.dat
	-rm -f -r tests/testdir
	-rm -f -r tests/file*.dat
	-rm -f -r tests/expected.txt
	-rm -f expected.txt
	-rm -f $(objects) $(generafile)

clean : 
	-rm -f $(objects) $(generafile)

runFarm : $(objects) 
		$(CC) $(CFLAGS) $(objects) -o farm


test: runTest clean

runTest : $(generafileObj) $(objects)
		$(CC) $(CFLAGS) $(objects) -o tests/farm
		cd tests;./test.sh
			
		
generafile :$(generafilePath)/generafile.c
			$(CC) -std=c99 $(generafilePath)/generafile.c -o generafile
	
workers_pool.o : $(objectsPath)/workers_pool.c 

parse_arguments.o : $(objectsPath)/parse_arguments.c 

master.o : $(objectsPath)/master.c

valid_file.o : $(objectsPath)/valid_file.c

queue_utils.o : $(objectsPath)/queue_utils.c

results_utils.o : $(objectsPath)/results_utils.c

collector.o : $(objectsPath)/collector.c

main.o : $(objectsPath)/main.c




