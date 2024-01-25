How to compile my code 
1. Go onto the OS1 servers for Oregon State University.
2. Go into the directory that you have the program files in.
3. Then in the terminal type gcc --std=gnu99 -o smallsh SmallShell.c and hit enter.
4. Then type ./smallsh If you want to run the program in the terminal.
5. After that you are within the program and running it.
6. If you would like to run the testscript on my smallsh repeat step 3. 
7. Enter: ./p3testscript 2>&1 to see test script run in terminal. 
or: ./p3testscript 2>&1 | more
or: ./p3testscript > mytestresults 2>&1 which outputs the test results to a file named mytestresults.