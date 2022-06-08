#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <limits.h>
#include "config.h"
#include "flash.h"

#define MAX_TASKS 10
extern int num_tasks;

int tasks_running = 1;
int time_count = 1;
int current_task = -1;

sem_t timer;
sem_t ready[MAX_TASKS];

int exec_times[MAX_TASKS];
int current_exec_times[MAX_TASKS];
int period_times[MAX_TASKS];
int next_period[MAX_TASKS];
int finished_current_period[MAX_TASKS];
int task_read[MAX_TASKS];
int task_write[MAX_TASKS];

int base_sleep = 1;
int time_limit;
int cpu_idling = 0;



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
    /* task read, write pages */
    /*  Page Read = 10 us
        Page Write = 100 us
        Page Erase = 500 us
    */

    task_read[0] = 0;
    task_read[1] = 0;
    task_read[2] = 0;

    task_write[0] = 5;
    task_write[1] = 20;
    task_write[2] = 18;

    exec_times[0] = 1 * task_write[0];
    exec_times[1] = 1 * task_write[1];
    exec_times[2] = 1 * task_write[2];

    /* E + (pi-a)(R+W) * gc_num */
    //exec_times[3] = (500 + (PPB-ALPHA)*(10 + 100)); 
    //exec_times[4] = (500 + (PPB-ALPHA)*(10 + 100));
    //exec_times[5] = (500 + (PPB-ALPHA)*(10 + 100));

    exec_times[3] = (5 + (PPB-ALPHA)*(1)); 
    exec_times[4] = (5 + (PPB-ALPHA)*(1));
    exec_times[5] = (5 + (PPB-ALPHA)*(1));

    period_times[0] = 100; // 10 ms
    period_times[1] = 240; // 24 ms
    period_times[2] = 160; // 16 ms
    
    if(task_write[0] > ALPHA){
        period_times[3] = period_times[0] / (task_write[0]/ALPHA + 1); 
    }else{
        period_times[3] = period_times[0] * (ALPHA / task_write[0]); 
    }
    
    if(task_write[1] > ALPHA){
        period_times[4] = period_times[1] / (task_write[1]/ALPHA + 1); 
    }else{
        period_times[4] = period_times[1] * (ALPHA / task_write[1]); 
    }

    if(task_write[2] > ALPHA){
        period_times[5] = period_times[2] / (task_write[1]/ALPHA + 1); 
    }else{
        period_times[5] = period_times[2] * (ALPHA / task_write[1]); 
    }


    for(i = 0; i < num_tasks; i++)
    {
        current_exec_times[i] = exec_times[i];
        next_period[i] = period_times[i];
    }
    
    //printf("How long do you want to execute this program : ");
    //scanf("%d", &time_limit);

    time_limit = 100000;


    /* edf schedulability check */
    float sum = 0;
    for(i = 0; i < num_tasks; i++)
    {
        sum += ((float)exec_times[i])/period_times[i];
    }
    
    
    if(sum > 1)
    {
        printf("These tasks cannot be scheduled, total utilization = %f\n", sum);
        abort();
    }else{
        printf("total utilization : %f\n", sum);
    }


    for(i = 0; i < num_tasks; i++)
    {
        sem_init(&ready[i], 0, 0);
    }
    sem_init(&timer, 0, 0);
}

int num_of_running_threads()
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
            printf("Missed Deadline! \n");
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
        time_count += 1;
        
        if(time_count >= time_limit)
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
        
        if(time_count >= time_limit)
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
 
    uint32_t w_addr;
    int number = *((int*) param);
    while(tasks_running && !finished_current_period[number])
    {
        sem_wait(&ready[number]);
        if(finished_current_period[number])
            break;
        current_task = number;
 
        if(tasks_running)
            printf("\ttask[%d] [write]\n", number);
        
        for(int i=0;i<task_write[0];i++){
            w_addr = (int)(rand() % RNOP * (1-OP));          
            write_page(w_addr, 3);
        }
    }    
    return NULL;
}


void *task1_write(void *param)
{
    uint32_t w_addr;
    int number = *((int *)param);
    while(tasks_running && !finished_current_period[number])
    {
        sem_wait(&ready[number]);
        if(finished_current_period[number])
            break;
        current_task = number;
     
        if(tasks_running)
            printf("\ttask[%d] [write]\n", number);

    
        for(int i=0;i<task_write[1];i++){
            w_addr = (int)(rand() % RNOP * (1-OP));
            write_page(w_addr, 3);
        }
    
    }    
    return NULL;
}


void *task2_write(void *param)
{
    uint32_t w_addr;
    int number = *((int *)param);
    while(tasks_running && !finished_current_period[number])
    {
        sem_wait(&ready[number]);
        if(finished_current_period[number])
            break;
        current_task = number;
     
        if(tasks_running)
            printf("\ttask[%d] [write]\n", number);        
      
        for(int i=0;i<task_write[2];i++){
            w_addr = (int)(rand() % RNOP * (1-OP));
            write_page(w_addr, 3);
        }
    
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
  
        if(tasks_running){
            printf("\ttask[%d] [garbage collection]\n", number);        
            gc_flash();
        }

    
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
 
        if(tasks_running){
            printf("\ttask[%d] [garbage collection]\n", number);         
            gc_flash();
        }   
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
  
        if(tasks_running ){
                printf("\ttask[%d] [garbage collection]\n", number);
                gc_flash();
        } 
    }    
    return NULL;
}