#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <limits.h>

#include "type.h"
#include "config.h"
#include "flash.h"
#include "task.h"


uint32_t page_map[RNOP];
flash_t * dev;
uint32_t cur_blk = 0, gc_cur_ch = 0, cur_ch= 0;
uint32_t total_write, total_read, total_gc, gc_write, gc_read,avg_wear, ipage_num; 
int num_tasks = 6;
int main(int argc, char*argv[]){
    int i;
    pthread_t timer_tid, sch_tid, task_tid[num_tasks];
    pthread_attr_t timer_attr, sch_attr, task_attr;

    flash_init();
    task_init();    
    
    int policy = SCHED_RR;
    pthread_attr_init(&task_attr);
    pthread_attr_setscope(&task_attr, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setschedpolicy(&task_attr, policy);
    
    //Create each task
    int nums[] = {0,1,2,3,4,5,6,7,8,9};    
    pthread_create(&task_tid[0], &task_attr, task0_write, (void *)&nums[0]);
    pthread_create(&task_tid[1], &task_attr, task1_write, (void *)&nums[1]);
    pthread_create(&task_tid[2], &task_attr, task2_write, (void *)&nums[2]);

    pthread_create(&task_tid[3], &task_attr, task0_gc, (void *)&nums[3]);
    pthread_create(&task_tid[4], &task_attr, task1_gc, (void *)&nums[4]);
    pthread_create(&task_tid[5], &task_attr, task2_gc, (void *)&nums[5]);

    pthread_attr_init(&timer_attr);
    pthread_create(&timer_tid, &timer_attr, timer_task, NULL);

    pthread_attr_init(&sch_attr);
    pthread_create(&sch_tid, &sch_attr, sched_task, NULL);
    

    for(i = 0; i < num_tasks; i++) pthread_join(task_tid[i], NULL);
    pthread_join(timer_tid, NULL);
    pthread_join(sch_tid, NULL);

    result_info();
    return 0;
}

