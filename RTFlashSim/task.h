#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <limits.h>

#define MAX_TASKS 100


extern int tasks_running;

extern int time_count;
extern int num_tasks;
extern int current_task;

extern sem_t timer;
extern sem_t ready[MAX_TASKS];

extern int exec_times[MAX_TASKS];
extern int current_exec_times[MAX_TASKS];
extern int period_times[MAX_TASKS];

extern int next_period[MAX_TASKS];
extern int finished_current_period[MAX_TASKS];

extern int base_sleep ;

extern int time_limit;

extern int cpu_idling;


void task_init(){
    num_tasks = 6;

    int i;
    
    for(i = 0; i < 10; i++)
    {
        exec_times[i] = 0;
        finished_current_period[i] = 0;
    }

    for(i = num_tasks; i < 10; i++)
    {
        finished_current_period[i] = 1;
    }

    /* config task e,p */
    exec_times[0] = 10;
    exec_times[1] = 10;
    exec_times[2] = 10;
     exec_times[3] = 30;
    exec_times[4] = 30;
    exec_times[5] = 30;

    period_times[0] = 40;
    period_times[1] = 40;
    period_times[2] = 40;
    period_times[3] = 800;
    period_times[4] = 300;
    period_times[5] = 500;

    for(i = 0; i < num_tasks; i++)
    {
        current_exec_times[i] = exec_times[i];
        next_period[i] = period_times[i];
    }
    
    //printf("How long do you want to execute this program : ");
    //scanf("%d", &time_limit);

    time_limit = 300;


    /* edf schedulability check */
    float sum = 0;
    for(i = 0; i < num_tasks; i++)
    {
        sum += ((float)exec_times[i])/period_times[i];
    }
    
    if(sum > 1)
    {
        printf("These tasks cannot be scheduled\n");
        abort();
    }


    for(i = 0; i < num_tasks; i++)
    {
        sem_init(&ready[i], 0, 0);
    }
    sem_init(&timer, 0, 0);
}

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

int get_next_thread()
{
    int next_deadline = INT_MAX;
    int next_deadline_thread = INT_MAX;  
    int i;
        
    for(i = 0; i < num_tasks; i++)
    {
      
        if(next_period[i] < next_deadline && finished_current_period[i] == 0)
        {
            next_deadline = next_period[i];
            next_deadline_thread = i;
        }
    }
    
    return next_deadline_thread;
}


void check_threads_for_new_period()
{
    int i;
    for(i = 0; i < num_tasks; i++)
    {
        if(next_period[i] <= time_count && finished_current_period[i] == 1)
        {
            next_period[i] += period_times[i];
            finished_current_period[i] = 0;
        }
  
        else if(next_period[i] == time_count && finished_current_period[i] == 0)
        {
            printf("WE MISSED A DEADLINE");
        }
    }
}

void cpu_idle()
{
    if(cpu_idling == 0)
    {
        cpu_idling = 1;
        printf("\tCPU is now idling\n");
    }
    
}


void exit_cpu_idle()
{
    current_task = get_next_thread();
    cpu_idling = 0;
}

void *timer_task(void *param)
{
  
    nanosleep((const struct timespec[]) {{0, 5000000L}}, NULL);
    sem_post(&timer);
    
    while(tasks_running)
    {
     
        nanosleep((const struct timespec[]) {{0, 5000000L}}, NULL);

        printf("%d\n", time_count);
        time_count++;
        
        if(time_count == time_limit)
        {
            tasks_running = 0;
            sem_post(&timer);
            printf("killed\n");
            break;
        }


        if(current_task != -1 && current_task != INT_MAX)
        {
            current_exec_times[current_task]--;
   
            if(current_exec_times[current_task] <= 0)
            {
                finished_current_period[current_task] = 1; 
                current_exec_times[current_task] = exec_times[current_task];
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

void *sched_task(void *param)
{
    while(tasks_running)
    {
        sem_wait(&timer);
        
        if(time_count == time_limit)
        {
            printf("Test end!!!!\n");
            break;
        }
        
        int next_thread = -1;

        check_threads_for_new_period();
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


void *task0_write(void *param)
{
    int number = *((int*) param);
    while(tasks_running && !finished_current_period[number])
    {
        sem_wait(&ready[number]);
        if(finished_current_period[number])
            break;
        current_task = number;
     
        if(tasks_running)
            printf("\ttask[%d] [write]\n", number);

        for(int i=0;i<20;i++){
            write_page(0x01, 3);
        }
    }    

    return NULL;
}


void *task1_write(void *param)
{
    int number = *((int *)param);
    while(tasks_running && !finished_current_period[number])
    {
        sem_wait(&ready[number]);
        if(finished_current_period[number])
            break;
        current_task = number;
     
        if(tasks_running)
            printf("\ttask[%d] [write]\n", number);

        for(int i=0;i<32;i++)
            write_page(0x01, 3);
    
    }    
    return NULL;
}


void *task2_write(void *param)
{
    int number = *((int *)param);
    while(tasks_running && !finished_current_period[number])
    {
        sem_wait(&ready[number]);
        if(finished_current_period[number])
            break;
        current_task = number;
     
        if(tasks_running)
            printf("\ttask[%d] [write]\n", number);

        for(int i=0;i<40;i++)
            write_page(0x01, 3);
    
    }    
    return NULL;
}


void *task0_gc(void *param)
{
    int number = *((int*) param);
    while(tasks_running && !finished_current_period[number])
    {
        sem_wait(&ready[number]);
        if(finished_current_period[number])
            break;
        current_task = number;
     
        if(tasks_running)
            printf("\ttask[%d] [garbage collection]\n", number);

        gc_flash();
    
    }    

    return NULL;
}

void *task1_gc(void *param)
{
    int number = *((int*) param);
    while(tasks_running && !finished_current_period[number])
    {        
        sem_wait(&ready[number]);
        if(finished_current_period[number])
            break;
        current_task = number;
     
        if(tasks_running)
            printf("\ttask[%d] [garbage collection]\n", number);

         gc_flash();
    
    
    }    

    return NULL;
}

void *task2_gc(void *param)
{
    int number = *((int*) param);
    while(tasks_running && !finished_current_period[number])
    {
        sem_wait(&ready[number]);
        if(finished_current_period[number])
            break;
        current_task = number;
     
        if(tasks_running)
            printf("\ttask[%d] [garbage collection]\n", number);

        gc_flash();
    
    
    }    

    return NULL;
}