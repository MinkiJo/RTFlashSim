#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <unistd.h>
#include <limits.h>
#include "../include/FS.h"
#include "../include/settings.h"
#include "../include/types.h"
#include "../bench/bench.h"
#include "interface.h"
#include "../include/utils/kvssd.h"

#define TIME_LIMIT 0
#define MAX_THREADS 100


//Declare long list of global variables

int threads_running = 1;

int time_count = 1;
int num_threads = 1;
int current_thread = -1;

sem_t timer;
sem_t ready[MAX_THREADS];

int burst_times[MAX_THREADS];
int current_burst_times[MAX_THREADS];
int period_times[MAX_THREADS];

int next_period[MAX_THREADS];
int finished_current_period[MAX_THREADS];

int base_sleep = 1;

int time_limit;

int cpu_idling = 0;

//Check how many threads are not done
int number_of_running_threads()
{
    int count = 0;
    int i = 0;
    for(; i < 10; i++)
    {
        if(!finished_current_period[i])
            count++;
    }
    return count;
}

void print_deadlines()
{
    int i;
    for(i = 0; i < num_threads; i++)
    {
        printf("Thread %d deadline: %d, finished status is: %d\n", i, next_period[i], finished_current_period[i]);
    }
}

//Get the next earliest deadline
int get_next_thread()
{
    int a;
    int next_deadline = INT_MAX;
    int next_deadline_thread = INT_MAX;
    int i;

    //print_deadlines();

    for(i = 0; i < num_threads; i++)
    {
        //printf("CONSIDERING THREAD %d\n", i);
        if(next_period[i] < next_deadline && finished_current_period[i] == 0)
        {
            next_deadline = next_period[i];
            next_deadline_thread = i;
        }
    }

    return next_deadline_thread;
}

//Check to see if a thread has entered the next period
//If it has then set it so it can run again
void check_threads_for_new_period()
{
    int i;
    for(i = 0; i < num_threads; i++)
    {
        if(next_period[i] <= time_count && finished_current_period[i] == 1)
        {
            next_period[i] += period_times[i];
            finished_current_period[i] = 0;
        }
            //Shouldn't hit, means we missed a deadline
        else if(next_period[i] == time_count && finished_current_period[i] == 0)
        {
            printf("WE MISSED A DEADLINE");
        }
    }
}

//functionality for the cpu idle
void cpu_idle()
{
    if(cpu_idling == 0)
    {
        cpu_idling = 1;
        printf("\tCPU is now idling\n");
    }
}

//Functionality for exiting the cpu idle status
void exit_cpu_idle()
{
    current_thread = get_next_thread();
    cpu_idling = 0;
}

//Main timer function
//Pretty much deals with anything time related
void *timer_thread(void *param)
{
    //Give the threads some time to startup before the timing starts
    nanosleep((const struct timespec[]) {{0, 5000000L}}, NULL);
    sem_post(&timer);

    while(threads_running)
    {
        //Used to help prevent race conditions, mainly with printing
        nanosleep((const struct timespec[]) {{0, 5000000L}}, NULL);

        printf("%d\n", time_count);
        time_count ++;
#ifdef TIME_LIMIT
        if(time_count == time_limit)
        {
            threads_running = 0;
            sem_post(&timer);
            printf("Killed\n");
            break;
        }
#endif
        //update remaining burst times
        if(current_thread != -1 && current_thread != INT_MAX)
        {
            current_burst_times[current_thread]--;
            //Check if the worker thread should be finished
            if(current_burst_times[current_thread] <= 0)
            {
                finished_current_period[current_thread] = 1;
                current_burst_times[current_thread] = burst_times[current_thread];
                sem_post(&timer);
            }
        }
        else
        {
            sem_post(&timer);
        }

    }

    return NULL;
}

//Main scheduler thread
//Deals with the scheduling functionality that is not timing related
void *sched(void *param)
{
    while(threads_running)
    {
        sem_wait(&timer);
#ifdef TIME_LIMIT
        if(time_count == time_limit)
        {
            break;
        }
#endif

        int next_thread = -1;

        //Update the threads available to be run
        check_threads_for_new_period();

        //Find next thread that needs to be run
        next_thread = get_next_thread();

        if(cpu_idling == 1 && next_thread != INT_MAX)
        {
            exit_cpu_idle();
        }

        if(next_thread == INT_MAX)
        {
            cpu_idle();
        }
        else
        {
            sem_post(&ready[next_thread]);
        }
    }
    int i;
    for(i = 0; i < 10; i++)
        sem_post(&ready[i]);

    return NULL;
}

//Main work thread, doesn't do much
void *work(void *param)
{
    bench_value *value;
    int number = *((int *)param);
    while(threads_running && !finished_current_period[number])
    {
        sem_wait(&ready[number]);

        if(finished_current_period[number])
            break;
        current_thread = number;
        //Fixes bug where sometimes one or more of these lines will be printed out after the time limit has been reached
        if(threads_running) {
            printf("\tThread %d is now being executed\n", number);
            for(int i=0;i<5;i++) {
                value = get_bench();
                //printf("%d   ", value->key);
                if(value->type==FS_SET_T){
                    inf_make_req(value->type,value->key,NULL,value->length,value->mark);
                }
                else if(value->type==FS_GET_T){
                    inf_make_req(value->type,value->key,NULL,value->length,value->mark);
                }
            }//printf("\n");
        }
    }
    return NULL;
}






int main(int argc,char* argv[]){
	/*
	   to use the custom benchmark setting the first parameter of 'inf_init' set false
	   if not, set the parameter as true.
	   the second parameter is not used in anycase.
    */

    int i;

    //Take input with some error checking
    if(argc < 2)
    {
        printf("Correct usage is <command> <number of threads>\n");
        exit(1);
    }

    num_threads = atoi(argv[1]);

    //Init burst times and done status
    for(i = 0; i < 10; i++)
    {
        burst_times[i] = 0;
        finished_current_period[i] = 0;
    }

    //Set threads that are not being used to be done
    for(i = num_threads; i < 10; i++)
    {
        finished_current_period[i] = 1;
    }

    //Take input for the burst times
    for(i = 0; i < num_threads; i++)
    {
        printf("Burst time for Thread %d: ", i);
        scanf("%d", &burst_times[i]);
        current_burst_times[i] = burst_times[i];
    }

    //Take input for the period times
    for(i = 0; i < num_threads; i++)
    {
        printf("Period for Thread %d: ", i);
        scanf("%d", &period_times[i]);
        next_period[i] = period_times[i];
    }

    printf("How long do you want to execute this program (sec): ");
    scanf("%d", &time_limit);


    float sum = 0;
    for(i = 0; i < num_threads; i++)
    {
        sum += ((float)burst_times[i])/period_times[i];
    }

    if(sum > 1)
    {
        printf("These threads cannot be scheduled\n");
        return -1;
    }


    //Init each ready semaphore
    for(i = 0; i < num_threads; i++)
    {
        sem_init(&ready[i], 0, 0);
    }
    //Init timer semaphore
    sem_init(&timer, 0, 0);





	inf_init(0,0,0,NULL);
	
	/*initialize the cutom bench mark*/
	bench_init();

	/*adding benchmark type for testing*/
	bench_add(SEQSET,0,RANGE,RANGE*2);
    bench_value *value;


    pthread_t timer_tid, sch_tid, work_tid[num_threads];
    pthread_attr_t timer_attr, sch_attr, work_attr;

    //Set up the attribute for the workers
    int policy = SCHED_RR;
    pthread_attr_init(&work_attr);
    pthread_attr_setscope(&work_attr, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setschedpolicy(&work_attr, policy);

    //Create each worker
    int nums[] = {0,1,2,3,4,5,6,7,8,9};
    for(i = 0; i < num_threads; i++)
    {
        pthread_create(&work_tid[i], &work_attr, work, (void *)&nums[i]);
    }

    //Create timer and scheduler
    pthread_attr_init(&timer_attr);
    pthread_create(&timer_tid, &timer_attr, timer_thread, NULL);

    pthread_attr_init(&sch_attr);
    pthread_create(&sch_tid, &sch_attr, sched, NULL);

    //Wait for each thread to finish
    for(i = 0; i < num_threads; i++)
    {
        pthread_join(work_tid[i], NULL);
    }

    pthread_join(timer_tid, NULL);
    pthread_join(sch_tid, NULL);

	/*while((value=get_bench())){

    printf("value  : 0x%x %d %d %d\n", value->key,value->length, value->mark, value->type);

		if(value->type==FS_SET_T){
			inf_make_req(value->type,value->key,NULL,value->length,value->mark);
		}
		else if(value->type==FS_GET_T){
			inf_make_req(value->type,value->key,NULL,value->length,value->mark);
		}
	}*/

	inf_free();
	return 0;
}
