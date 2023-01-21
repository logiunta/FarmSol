# FarmSol

The "farm" program is composed of two processes, "MasterWorker" and "Collector". MasterWorker is a multi-threaded process that takes a list of binary files 
containing long integers as input and various optional arguments such as:<br>

-n <nthread> which specifies the number of Worker threads in the MasterWorker process (default value is 4)<br>
-q <qlen> which specifies the length of the concurrent queue between the Master thread and the Worker threads (default value is 8)<br>
-d <directory-name> which specifies a directory containing binary files and possibly other directories containing binary files; the binary files will be 
used as input files for the calculation<br>
-t <delay> which specifies a time in milliseconds between the sending of two consecutive requests to the Worker threads by the Master thread 
(default value is 0)<br>

The program reads the arguments passed to the main function one by one and verifies that 
they are regular files. If the '-d' option is passed, which specifies a directory name as an argument, the program navigates the specified directory and 
considers all files and directories within it.

The name of the generic input file (along with any additional information) is sent to one of the Worker threads in the pool through a shared concurrent 
queue (called "concurrent task queue" in Figure 1). The generic Worker thread reads the contents of the entire file from disk, performs a calculation on 
the elements read, and then sends the result obtained, along with the name of the file, to the Collector process through the previously established socket 
connection. The Collector process waits to receive all the results from the Workers and then prints the values obtained on the standard output, ordering 
the print in the following format:<br>

result1 filepath1<br>
result2 filepath2<br>
result3 filepath3<br>
. . .

The print is ordered based on the result in ascending order (result1<=result2<=result3, ...). 
The calculation that must be performed on each file is as follows:
$$\sum_{i=0}^{N-1} (i * file[i])$$
where N is the number of long integers contained in the file, and result is the long integer that will be sent to the Collector.

The MasterWorker process must handle the signals SIGHUP, SIGINT, SIGQUIT, SIGTERM, SIGUSR1. Upon receiving the SIGUSR1 signal, the MasterWorker process notifies the Collector process to print the results received so far (always in an ordered manner), while upon receiving the other signals, the process must complete any tasks in the task queue, no longer reading any other input files, and then terminate after waiting for the Collector process to terminate and after deleting the socket file. The Collector process masks all signals handled by the MasterWorker process. The SIGPIPE signal must be handled appropriately by both processes.
