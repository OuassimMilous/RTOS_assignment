#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>

#include <string.h>
#include <stdio.h>
#include <fcntl.h>

//code of tasks
void task1_code( );
void task2_code( );
void task3_code( );
void task4_code( );

//otherfunctions
void write_driver(const char *str);
void do_nothing();


//characteristic function of the thread, only for timing and synchronization
void *task1( void *);
void *task2( void *);
void *task3( void *);
void *task4( void *);

#define INNERLOOP 1000
#define OUTERLOOP 100

#define NPERIODICTASKS 3
#define NAPERIODICTASKS 1
#define NTASKS NPERIODICTASKS + NAPERIODICTASKS


// Thread periods in milliseconds
long int periods[NTASKS] = {300000000, 500000000, 800000000, 0}; // J4 is aperiodic
struct timespec next_arrival_time[NTASKS];
double WCET[NTASKS];
pthread_attr_t attributes[NTASKS];
pthread_t thread_id[NTASKS];
struct sched_param parameters[NTASKS];
int missed_deadlines[NTASKS];


// initialization of mutexes and condition for the aperiodic thread
pthread_mutex_t task4_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t task4_condition = PTHREAD_COND_INITIALIZER;

// initialization of mutexes for the schueduling
pthread_mutex_t mutex_sec = PTHREAD_MUTEX_INITIALIZER;
pthread_mutexattr_t mutexattr_sec;


main()
{
	//asigning a name to the max and min priorities
 	struct sched_param priomax;
  	priomax.sched_priority=sched_get_priority_max(SCHED_FIFO);
  	struct sched_param priomin;
  	priomin.sched_priority=sched_get_priority_min(SCHED_FIFO);

	// Initialize the mutex attribute
    pthread_mutexattr_init(&mutexattr_sec);

    // Set the protocol to PTHREAD_PRIO_PROTECT
    pthread_mutexattr_setprotocol(&mutexattr_sec, PTHREAD_PRIO_PROTECT);

    // Set the priority ceiling 
    pthread_mutexattr_setprioceiling(&mutexattr_sec, 10);

    // Initialize the mutex with the attribute
    pthread_mutex_init(&mutex_sec, &mutexattr_sec);

	// set the maximum priority to the current thread
  	if (getuid() == 0) // current thread supper user
    	pthread_setschedparam(pthread_self(),SCHED_FIFO,&priomax);


 	int i;
  	for (i =0; i < NTASKS; i++)
    	{
		// initializa time_1 and time_2 required to read the clock
		struct timespec time_1, time_2;
		clock_gettime(CLOCK_REALTIME, &time_1);

		//we should execute each task more than one for computing the WCET
		//periodic tasks
			if (i==0)
			task1_code();
      		if (i==1)
			task2_code();
      		if (i==2)
			task3_code();	
      	
		//aperiodic tasks
      		if (i==3)
			task4_code();
		
		clock_gettime(CLOCK_REALTIME, &time_2);

		// compute the Worst Case Execution Time 

        WCET[i]= 1000000000*(time_2.tv_sec - time_1.tv_sec)+(time_2.tv_nsec-time_1.tv_nsec);
        printf("\nWorst Case Execution Time %d=%f \n", i, WCET[i]);
    	}

    	// compute U
		double U = WCET[0]/periods[0]+WCET[1]/periods[1]+WCET[2]/periods[2];

    	// compute Ulub by considering the fact that we have harmonic relationships between periods
		double Ulub = 1;

	//check the sufficient conditions: if they are not satisfied, exit  
  	if (U > Ulub){
        printf("\n U=%lf Ulub=%lf Non schedulable Task Set", U, Ulub);
        return(-1);
    }

  	printf("\n U=%lf Ulub=%lf Scheduable Task Set", U, Ulub);

  	fflush(stdout);

  	sleep(5);

    //set the current thread to lowest priority
  	if (getuid() == 0)
    		pthread_setschedparam(pthread_self(),SCHED_FIFO,&priomin);

  
  	// set the attributes of each task, including scheduling policy and priority
  	for (i =0; i < NPERIODICTASKS; i++)
    	{
      
		//initializa the attribute structure of task i
        pthread_attr_init(&(attributes[i]));

		//set the attributes to not inherite from the main thread
      	pthread_attr_setinheritsched(&(attributes[i]), PTHREAD_EXPLICIT_SCHED);
      
		//set the SCHED_FIFO policy 
        pthread_attr_setschedpolicy(&(attributes[i]), SCHED_FIFO);	

		//assign the priority inversely proportional to the period
        parameters[i].sched_priority = sched_get_priority_max(SCHED_FIFO) - i;

		//set the attributes and the parameters of the current thread (pthread_attr_setschedparam)
        pthread_attr_setschedparam(&(attributes[i]), &(parameters[i]));
    	}

 	// aperiodic tasks
	for (int i = NPERIODICTASKS; i < NTASKS; i++)
	{
		pthread_attr_init(&(attributes[i]));
		pthread_attr_setschedpolicy(&(attributes[i]), SCHED_FIFO);

		// set minimum priority (background scheduling)
		parameters[i].sched_priority = 0;
		pthread_attr_setschedparam(&(attributes[i]), &(parameters[i]));
	}

	//delare the variable to contain the return values of pthread_create	
  	int iret[NTASKS];

	//declare variables to read the current time
	struct timespec time_1;
	clock_gettime(CLOCK_REALTIME, &time_1);

  	// set the next arrival time for each task. This is not the beginning of the first
	// period, but the end of the first period and beginning of the next one. 
	for (int i = 0; i < NPERIODICTASKS; i++)
	{
		long int next_arrival_nanoseconds = time_1.tv_nsec + periods[i];
		// then we compute the end of the first period and beginning of the next one
		next_arrival_time[i].tv_nsec = next_arrival_nanoseconds % 1000000000;
		next_arrival_time[i].tv_sec = time_1.tv_sec + next_arrival_nanoseconds / 1000000000;
		missed_deadlines[i] = 0;
	}

    // create all threads
  	iret[0] = pthread_create( &(thread_id[0]), &(attributes[0]), task1, NULL);
  	iret[1] = pthread_create( &(thread_id[1]), &(attributes[1]), task2, NULL);
  	iret[2] = pthread_create( &(thread_id[2]), &(attributes[2]), task3, NULL);
   	iret[3] = pthread_create( &(thread_id[3]), &(attributes[3]), task4, NULL);

  	// join all periodic threads 
  	pthread_join( thread_id[0], NULL);
  	pthread_join( thread_id[1], NULL);
  	pthread_join( thread_id[2], NULL);

  	for (i = 0; i < NTASKS; i++){
      	printf ("\nMissed Deadlines Task %d=%d", i, missed_deadlines[i]);
		fflush(stdout);
    }

	// Destroy the mutex and its attribute when done
    pthread_mutex_destroy(&mutex_sec);
    pthread_mutexattr_destroy(&mutexattr_sec);

  	exit(0);
}

// application specific task_1 code
void task1_code()
{
	 // pass the id string
    const char *str ="[1";
    write_driver(str);
	do_nothing();
    str ="1]";
	write_driver(str);

}

//thread code for task_1 (used only for temporization)
void *task1( void *ptr)
{

	// set thread affinity, that is the processor on which threads shall run
	cpu_set_t cset;
	CPU_ZERO(&cset);
	CPU_SET(0, &cset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cset);

	// execute the task one hundred times... it should be an infinite loop (too dangerous)
	for (int i = 0; i < 100; i++)
	{
		// lock mutex
		pthread_mutex_lock(&mutex_sec);
		// execute application specific code
		task1_code();

		// unlock mutex
		pthread_mutex_unlock(&mutex_sec);

		// check if we didnt miss the deadline
		clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next_arrival_time[0], NULL);

		// the thread is ready and can compute the end of the current period for
		// the next iteration
		long int next_arrival_nanoseconds = next_arrival_time[0].tv_nsec + periods[0];
		next_arrival_time[0].tv_nsec = next_arrival_nanoseconds % 1000000000;
		next_arrival_time[0].tv_sec = next_arrival_time[0].tv_sec + next_arrival_nanoseconds / 1000000000;
	}

}

// application specific task_2 code
void task2_code()
{	
	
	 // pass the id string
    const char *str ="[2";
    write_driver(str);

	//signal for the aperiodic task to start
	pthread_cond_signal(&task4_condition);

	do_nothing();
    str ="2]";
	write_driver(str);
 
}

//thread code for task_2 (used only for temporization)
void *task2( void *ptr )
{
	// set thread affinity, that is the processor on which threads shall run
	cpu_set_t cset;
	CPU_ZERO(&cset);
	CPU_SET(0, &cset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cset);

	// execute the task one hundred times... it should be an infinite loop (too dangerous)
	for (int i = 0; i < 100; i++)
	{
		// lock mutex
		pthread_mutex_lock(&mutex_sec);
		// execute application specific code
		task2_code();

		// unlock mutex
		pthread_mutex_unlock(&mutex_sec);

		// check if we didnt miss the deadline
        clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next_arrival_time[1], NULL);

		// the thread is ready and can compute the end of the current period for
		// the next iteration 		
		long int next_arrival_nanoseconds = next_arrival_time[1].tv_nsec + periods[1];
		next_arrival_time[1].tv_nsec= next_arrival_nanoseconds%1000000000;
		next_arrival_time[1].tv_sec= next_arrival_time[1].tv_sec + next_arrival_nanoseconds/1000000000;
 	}

}

// application specific task_3 code
void task3_code()
{
	 // pass the id string
    const char *str ="[3";
    write_driver(str);
	do_nothing();
    str ="3]";
	write_driver(str);

}

//thread code for task_3 (used only for temporization)
void *task3( void *ptr)
{
	// set thread affinity, that is the processor on which threads shall run
	cpu_set_t cset;
	CPU_ZERO(&cset);
	CPU_SET(0, &cset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cset);

	// execute the task one hundred times... it should be an infinite loop (too dangerous)
	for (int i = 0; i < 100; i++)
	{
		// lock mutex
		pthread_mutex_lock(&mutex_sec);
		// execute application specific code
		task3_code();

		// unlock mutex
		pthread_mutex_unlock(&mutex_sec);

		// check if we didnt miss the deadline
        clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next_arrival_time[2], NULL);

		// the thread is ready and can compute the end of the current period for
		// the next iteration 		
		long int next_arrival_nanoseconds = next_arrival_time[2].tv_nsec + periods[2];
		next_arrival_time[2].tv_nsec= next_arrival_nanoseconds%1000000000;
		next_arrival_time[2].tv_sec= next_arrival_time[2].tv_sec + next_arrival_nanoseconds/1000000000;
 	}
}

// application specific task_4 code
void task4_code()
{
	 // pass the id string
    const char *str ="[4";
    write_driver(str);
	do_nothing();
    str ="4]";
	write_driver(str);

}

//thread code for task_4 (used only for temporization)
void *task4( void *)
{
	cpu_set_t cset;
	CPU_ZERO (&cset);
	CPU_SET(0, &cset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cset);

	while (1){
		pthread_cond_wait(&task4_condition, &task4_mutex);
 		task4_code();
	}
}

// Function to write in the driver
void write_driver(const char *str){
    int fd;
    if ((fd = open ("/dev/driveras", O_RDWR)) == -1) {
        perror("open failed");
		exit(1);
    }
    if (write(fd, str, sizeof(str)) != sizeof(str)) {
        perror("write failed");
		exit(1);
    }
    printf("%s", str);
    close(fd);
}

// Function to waste the time
void do_nothing()
{
	double wt;
	for (int i = 0; i < OUTERLOOP; i++)
	{
		for (int j = 0; j < INNERLOOP; j++)
		{
			wt = rand() * rand() % 10;
		}
	}
}
