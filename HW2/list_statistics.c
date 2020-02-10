//
// Computes the mean and stddev of a list using multiple threads
//
// Warning: Return values of calls are not checked for error to keep 
// the code simple.
//
// Compilation command on ADA ($ sign is the shell prompt):
//  $ module load intel/2017A
//  $ icc -o list_statistics.exe list_statistics.c -lpthread -lc -lrt
//
// Sample execution and output ($ sign is the shell prompt):
//  $ ./list_statistics.exe 1000000 9
// Threads = 9, minimum = 148, time (sec) =   0.0013
//
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define MAX_THREADS     65536
#define MAX_LIST_SIZE   268435456


int num_threads;		// Number of threads to create - user input 

int thread_id[MAX_THREADS];	// User defined id for thread
pthread_t p_threads[MAX_THREADS];// Threads
pthread_attr_t attr;		// Thread attributes 
pthread_mutex_t lock_minimum;
pthread_mutex_t lock_mean;	// Protects mean, count
pthread_mutex_t lock_stdev;	// Protects stdev, count
int minimum;
int mean;			// Minimum value in the list
int stdev;
int count;			// Count of threads that have updated minimum

int list[MAX_LIST_SIZE];	// List of values
int list_size;			// List size

// Thread routine to compute minimum of sublist assigned to thread; 
// update global value of minimum if necessary

int calculateSD(int data[]) {
    int sum = 0, means, SD = 0;
    int i;
    for (i = 0; i < list_size; ++i) {
        sum += data[i];
    }
    means = sum / list_size;
    for (i = 0; i < list_size; ++i)
        SD += pow(data[i] - means, 2);
    return sqrt(SD / list_size);
}

void *find_statistics (void *s) {
    int j;
    int my_thread_id = *((int *)s);
	int sum = 0;

    int block_size = list_size/num_threads;
    int my_start = my_thread_id*block_size;
    int my_end = (my_thread_id+1)*block_size-1;
    if (my_thread_id == num_threads-1) my_end = list_size-1;

    // Thread computes mean of list
	for (int i = 0; i < list_size; i++){
		sum += list[i];
	}
	int my_mean = sum / list_size;
	
    // Thread updates minimum 
    // *
    // *
    // Put your code here ...
	pthread_mutex_lock(&lock_mean);
	if (my_mean != mean){
		mean = my_mean;
		printf("my mean is %d\n", my_mean);
		printf("mean is %d\n", mean);
	}
	pthread_mutex_unlock(&lock_mean);
    // *
    // *
	int mysd = calculateSD(list);
	pthread_mutex_lock(&lock_stdev);
	if (mysd != stdev){
		stdev = mysd;
	}
	printf("my_stdev is %d\n", mysd);
	printf("stdev is %d\n", stdev);
	pthread_mutex_unlock(&lock_stdev);

    // Thread exits
    pthread_exit(NULL);
}




// Main program - set up list of randon integers and use threads to find
// the minimum value; assign minimum value to global variable called minimum
int main(int argc, char *argv[]) {

    struct timespec start, stop;
    double total_time, time_res;
    int i, j; 
    int true_minimum;

    if (argc != 3) {
	printf("Need two integers as input \n"); 
	printf("Use: <executable_name> <list_size> <num_threads>\n"); 
	exit(0);
    }
    if ((list_size = atoi(argv[argc-2])) > MAX_LIST_SIZE) {
	printf("Maximum list size allowed: %d.\n", MAX_LIST_SIZE);
	exit(0);
    }; 
    if ((num_threads = atoi(argv[argc-1])) > MAX_THREADS) {
	printf("Maximum number of threads allowed: %d.\n", MAX_THREADS);
	exit(0);
    }; 
    if (num_threads > list_size) {
	printf("Number of threads (%d) < list_size (%d) not allowed.\n", num_threads, list_size);
	exit(0);
    }; 

    // Initialize mutex and attribute structures
    pthread_mutex_init(&lock_minimum, NULL); 
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    // Initialize list, compute minimum to verify result
    srand48(0); 	// seed the random number generator
    list[0] = lrand48(); 
    true_minimum = list[0];
    for (j = 1; j < list_size; j++) {
	list[j] = lrand48(); 
	if (true_minimum > list[j]) {
	    true_minimum = list[j];
	}
    }

    // Initialize count
    count = 0;

    // Create threads; each thread executes find_statistics
    clock_gettime(CLOCK_REALTIME, &start);
    for (i = 0; i < num_threads; i++) {
	thread_id[i] = i; 
	pthread_create(&p_threads[i], &attr, find_statistics, (void *) &thread_id[i]); 
    }
    // Join threads
    for (i = 0; i < num_threads; i++) {
	pthread_join(p_threads[i], NULL);
    }

    // Compute time taken
    clock_gettime(CLOCK_REALTIME, &stop);
    total_time = (stop.tv_sec-start.tv_sec)
	+0.000000001*(stop.tv_nsec-start.tv_nsec);

    // Check answer
    if (true_minimum != minimum) {
	printf("Houston, we have a problem!\n"); 
    }
    // Print time taken
    printf("Threads = %d, minimum = %d, time (sec) = %8.4f\n", 
	    num_threads, minimum, total_time);
	printf("mean is %d, stdev is %d",mean, stdev);
    // Destroy mutex and attribute structures
    pthread_attr_destroy(&attr);
    pthread_mutex_destroy(&lock_minimum);
}

