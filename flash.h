#include "config.h"
#include "type.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef FLASH_H
#define FLASH_H
extern uint32_t page_map[RNOP];
extern flash_t * dev;
extern uint32_t cur_blk, gc_cur_ch, cur_ch;
extern uint32_t total_write, total_read, total_gc, gc_write, gc_read,avg_wear, ipage_num; 


void result_info(){
    uint32_t fblk_num = 0;
    uint32_t ublk_num = 0;
    uint32_t wear= 0;
    for(int i=0;i<NOC;i++){
        fblk_num +=  dev->channel[i].freeblock.count;
        ublk_num +=  dev->channel[i].usingblock.size;
    }

    for(int i=0;i<RNOB;i++){
        wear += dev->block[i].wear;
    }
    wear = wear / RNOB;
    
    printf("\n\n**************result**************\n");
    printf("total write : %d\n", total_write);
    printf("total read : %d\n", total_read);
    printf("gc count : %d\n", total_gc);
    printf("using block num: %d\n", ublk_num);
    printf("free block num : %d\n", fblk_num); 
    printf("average wear : %d\n", wear);
}


void set_write_target(){
    cur_blk = dequeue(&(dev->channel[cur_ch].freeblock));   

    if(cur_blk == -1){ // no more free block!
        result_info();
        abort(); 
    }    
    cur_ch = (cur_ch + 1) % NOC;
}

void flash_init(){ 
    int i,j;
    dev = (flash_t*)malloc(sizeof(flash_t));

    dev->channel = (channel_t*)malloc(sizeof(channel_t) * NOC);
    dev->block = (block_t*)malloc(sizeof(block_t) * RNOB);    
    
    int ch_cnt = 0;
    for(i=0;i<NOB * NOC;i++){
        for(j=0;j<PPB;j++){
            dev->block[i].page[j].valid = ERASE;
        }

        dev->block[i].channel = ch_cnt++;
        if(ch_cnt == NOC)
            ch_cnt = 0;
        dev->block[i].cur = 0;
        dev->block[i].index  = i;
        dev->block[i].wear = 0;
        dev->block[i].invalid_num = 0;
        dev->block[i].free = TRUE;
    }

    /* each channel manages own freeblock */
    for(i=0;i<NOC;i++){
        init_queue(&(dev->channel[i].freeblock));
    }

    for(i=0;i<NOB;i++){
        for(int j=0;j<NOC;j++){
            enqueue(&(dev->channel[j].freeblock),i*NOC + j);            
        }
    }

    /* page map init */
    for(i=0;i<RNOP;i++){
        page_map[i] = UINT32_MAX;
    }

    set_write_target();
}


void invlidate_page(uint32_t ppa){
    uint32_t block_idx = ppa / PPB;
    uint32_t page_idx = ppa % PPB ;

    dev->block[block_idx].page[page_idx].valid = INVALID;    
    dev->block[block_idx].invalid_num++;
}

uint32_t get_write_addr(){
    return cur_blk*PPB + dev->block[cur_blk].cur;
}


void write_flash(uint32_t lba, uint32_t data){    
    total_write++;
    uint32_t cur = dev->block[cur_blk].cur++;

    dev->block[cur_blk].page[cur].data = data;
    dev->block[cur_blk].page[cur].oob = lba;
    dev->block[cur_blk].page[cur].valid = VALID; 

    if(cur >= PPB) {        
        dev->block[cur_blk].free = FALSE;
        set_write_target();
    }
    return;
}

void write_page(uint32_t lba, uint32_t data){
    // global var 'cur_blk' indicate ppa for write

    uint32_t ppa = page_map[lba];
    if(page_map[lba] != UINT32_MAX){
        invlidate_page(ppa);
        ipage_num++;
    }
    page_map[lba] = get_write_addr();
    write_flash(lba, data);
    return;
}  

uint32_t read_flash(uint32_t ppa){    

    uint32_t block_idx = ppa / PPB;
    uint32_t page_idx = ppa % PPB ;
    total_read++;
    return dev->block[block_idx].page[page_idx].data;
}

uint32_t read_page(uint32_t lba){
    uint32_t ppa = page_map[lba];
    if(ppa == UINT32_MAX){
        printf("wrong read!!\n");
        abort();
    }        
    return read_flash(ppa);    
}
int get_gc_target(){
     int max = 0, maxi;
     for(int i=0;i<RNOB;i++){
        /* seq search for using blocks */
        if(dev->block[i].free == FALSE){
            //printf("[%d] ",dev->block[i].invalid_num);
            if(dev->block[i].invalid_num > max){
                max = dev->block[i].invalid_num;
                maxi = i;
            }
        }
    }

    if(max == 0){
        printf("\ngc is not needed\n");
        return -1;
    }
    return maxi;
}

void gc_flash(){    
    uint32_t gc_target;  
    total_gc++;    

    gc_target = get_gc_target();    
    if(gc_target == -1)
        return;
        
    for(int i=0;i<PPB;i++){        
        gc_read++;
        if(dev->block[gc_target].page[i].valid == VALID){
            uint32_t data = read_flash(gc_target*PPB + i);
            gc_write++;
            write_page(dev->block[gc_target].page[i].oob, data);
        }else{
            ipage_num--;
        }            
        // dev->block[gc_target].page[i].valid == ERASE;
    }
    
    enqueue(&(dev->channel[gc_cur_ch].freeblock),gc_target);
    dev->block[gc_target].cur = 0;
    dev->block[gc_target].invalid_num = 0;
    dev->block[gc_target].wear += 5;
    dev->block[gc_target].free = TRUE;

    gc_cur_ch = (gc_cur_ch + 1) % NOC;
}

#endif