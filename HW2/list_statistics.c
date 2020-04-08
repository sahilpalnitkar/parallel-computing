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
pthread_mutex_t lock_mean_count, lock_stddev_count;
pthread_mutex_t lock_barrier;
pthread_cond_t co1,co2;

long double true_mean;
long double global_mean_sum = 0;
long double global_stddev_sum = 0;
long double mean;			// Minimum value in the list
long double true_stddev;
long double stddev;
int mean_count = 0;			// Count of threads that have updated minimum
int stddev_count = 0;
int mean_done = 0;
int stddev_done = 0;

int list[MAX_LIST_SIZE];	// List of values
int list_size;			// List size

// Thread routine to compute minimum of sublist assigned to thread; 
// update global value of minimum if necessary

void barrier_simple_mean() {
	pthread_mutex_lock(&lock_barrier);
	mean_count++;
	if (mean_count < num_threads){
		pthread_cond_wait(&co1, &lock_barrier);
	}
	else{
		for (int i = 0; i < num_threads; i++) 
			pthread_cond_signal(&co1);
	}
	pthread_mutex_unlock(&lock_barrier);

}
void barrier_simple_stddev() {
	pthread_mutex_lock(&lock_barrier);
	stddev_count++;
	if (stddev_count < num_threads){
		pthread_cond_wait(&co1, &lock_barrier);
	}
	else{
		for (int i = 0; i < num_threads; i++) 
			pthread_cond_signal(&co1);
	}
	pthread_mutex_unlock(&lock_barrier);

}

long double calculate_stddev(long double mean, int list[]){
    long double variance_sum = 0;
    for (int i =0; i < list_size; i++){
        long double var = ((double)list[i]-mean)*((double)list[i]-mean);
        variance_sum += var;
        
    }
    variance_sum = variance_sum/(double)list_size;
    
    long double calc_stddev = sqrt(variance_sum);
    return calc_stddev;
}

void *find_statistics (void *s) {
    int j;
    int my_thread_id = *((int *)s);
    long double my_local_sum = 0;
    long double my_stddev_sum = 0;

    int block_size = list_size/num_threads;
    int my_start = my_thread_id*block_size;
    int my_end = (my_thread_id+1)*block_size-1;
    if (my_thread_id == num_threads-1) my_end = list_size-1;
    long double my_mean = 0;

    // Thread computes sum of list
	for (int i = my_start; i <= my_end; i++){
		my_local_sum += (long double)list[i];
	}
	
	// *
    // *
    pthread_mutex_lock(&lock_mean_count);
    global_mean_sum += my_local_sum;
    pthread_mutex_unlock(&lock_mean_count);
    barrier_simple_mean();
    
    pthread_mutex_lock(&lock_mean_count);
    if (mean_done == 0){
        mean = (long double)global_mean_sum/(long double)list_size;
        mean_done++;
    }
    
    pthread_mutex_unlock(&lock_mean_count);
    
    
    
    for (int i = my_start; i <= my_end; i++){
        long double element = ((double)list[i]-mean)*((double)list[i]-mean)/(double)list_size;
        my_stddev_sum += element;
    }
	
	pthread_mutex_lock(&lock_stddev_count);
    global_stddev_sum += my_stddev_sum;
	pthread_mutex_unlock(&lock_stddev_count);
	barrier_simple_stddev();
	
    pthread_mutex_lock(&lock_stddev_count);
    if (stddev_done == 0){
        stddev = sqrt(global_stddev_sum);
        stddev_done++;
    }
    pthread_mutex_unlock(&lock_stddev_count);
    // Thread exits
    pthread_exit(NULL);
}




// Main program - set up list of randon integers and use threads to find
// the minimum value; assign minimum value to global variable called minimum
int main(int argc, char *argv[]) {

    struct timespec start, stop;
    double total_time, time_res;
    int i, j; 
    
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
    pthread_mutex_init(&lock_mean_count, NULL);
    pthread_mutex_init(&lock_stddev_count, NULL); 
    pthread_cond_init(&co1, NULL); 
    pthread_cond_init(&co2, NULL); 
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    // Initialize list, compute minimum to verify result
    srand48(0); 	// seed the random number generator
    list[0] = lrand48(); 
    long double true_mean = 0;
    long double avg_sum = (long double)list[0];
    for (j = 1; j < list_size; j++) {
	list[j] = lrand48(); 
	avg_sum += (long double)list[j];
	}
    true_mean = avg_sum/(long double)list_size;
    mean = true_mean;
    
    
    
    true_stddev = calculate_stddev(true_mean, list);
    stddev = true_stddev;
   
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
    
    
    // if (true_mean != mean){
    //     printf("Houston, we have a problem with the mean!\n");
    //     printf("true_mean %Lf\n", true_mean);
    //     printf("mean %Lf\n", mean);
    // } 
    // if (true_stddev != stddev) {
	   // printf("Houston, we have a problem with the stddev!\n"); 
	   // printf("true_stddev %Lf\n", true_stddev);
    //     printf("stddev %Lf\n", stddev);
    // }

    printf("true_mean %Lf\n", true_mean);
    printf("true_stddev %Lf\n", true_stddev);
    // // Print time taken
    printf("Threads = %d, mean = %Lf, stddev = %Lf, time (sec) = %8.4f\n", 
	    num_threads, mean, stddev, total_time);
    // Destroy mutex and attribute structures
    pthread_attr_destroy(&attr);
    pthread_mutex_destroy(&lock_mean_count);
    pthread_mutex_destroy(&lock_stddev_count);
    pthread_mutex_destroy(&lock_barrier);
    pthread_cond_destroy(&co1);
    pthread_cond_destroy(&co2);
}

