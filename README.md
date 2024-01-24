# RTOS_assignment


1) Design an application with 3 threads J1, J2, J3, whose periods are 300ms, 500ms, and 800ms, plus an aperiodic thread J4 in background which is triggered by J2.

(2) The threads shall just "waste time," as we did in the exercise with threads.

(3) Design a simple driver with only open, close, and write system calls.

(4) During its execution, every task 

	(i) opens the special file associated with the driver;

	(ii ) writes to the driver its identifier plus open square brackets (i.e., [1, [2, [3, or [4)

	(iii) close the special files

	(iv) performs operations (i.e., wasting time)

	(v) performs (i)(ii) and (iii) again to write to the driver its identifier, but with closed square brackets (i.e., 1], 2], 3] or 4]).

(5) The write system call writes on the kernel log the string received from the thread. A typical output of the system, when reading the kernel log, can be the following [11][2[11]2][3[11]3][4]. This sequence clearly shows that some threads can be preempted by other threads (if this does not happen, try to increase the computational time of longer tasks).

(6) Finally, modify the code of all tasks to use semaphores. Every thread now protects all its operations (i) to (v) with a semaphore, which prevents other tasks from preempting. Specifically, use semaphores with a priority ceiling access protocol.  
