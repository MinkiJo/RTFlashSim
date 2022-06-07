#include "config.h"
#include "type.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

void set_write_target();

extern uint32_t page_map[RNOP];
extern flash_t * dev;
extern uint32_t cur_blk, gc_cur_ch, cur_ch;
extern uint32_t total_write, total_read, total_gc, gc_write, gc_read,avg_wear; 

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
    }

    for(i=0;i<NOC;i++){
        init_queue(&(dev->channel[i].freeblock));
        init_heap(&(dev->channel[i].usingblock));
    }

    for(i=0;i<NOB;i++){
        for(int j=0;j<NOC;j++){
            enqueue(&(dev->channel[j].freeblock),i*NOC + j);
        }
    }

    for(i=0;i<RNOP;i++){
        page_map[i] = UINT32_MAX;
    }
   set_write_target();
}

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

void invlidate_page(uint32_t ppa){
    uint32_t block_idx = ppa / PPB;
    uint32_t page_idx = ppa % PPB ;

    dev->block[block_idx].page[page_idx].valid = INVALID;    
    dev->block[block_idx].invalid_num++;
}

uint32_t get_write_addr(){
    return cur_blk*PPB + dev->block[cur_blk].cur;
}

void set_write_target(){
    cur_blk = dequeue(&(dev->channel[cur_ch].freeblock));
    if(cur_blk == -1){ // no more free block!
        abort(); 
    }    
    cur_ch = (cur_ch + 1) % NOC;
}


void write_flash(uint32_t data){    
    total_write++;
    uint32_t cur = dev->block[cur_blk].cur++;
    uint32_t ch = dev->block[cur_blk].channel;
    uint32_t inv = dev->block[cur_blk].invalid_num;

    dev->block[cur_blk].page[cur].data = data;
    dev->block[cur_blk].page[cur].valid = VALID; 

    if(cur >= PPB-1) {
        insert_heap(&(dev->channel[ch].usingblock), inv, cur_blk);
        set_write_target();
    }
    return;
}

void write_page(uint32_t lba, uint32_t data){
    uint32_t ppa = page_map[lba];
    if(page_map[lba] != UINT32_MAX){
        invlidate_page(ppa);
    }
    page_map[lba] = get_write_addr();
    write_flash(data);
    return;
}  

uint32_t read_flash(uint32_t bidx, uint32_t pidx){
    total_read++;
    return dev->block[bidx].page[pidx].data;
}

uint32_t read_page(uint32_t lba){
    uint32_t ppa = page_map[lba];
    if(ppa == UINT32_MAX){
        printf("wrong read!!\n");
        abort();
    }    

    uint32_t block_idx = ppa / PPB;
    uint32_t page_idx = ppa % PPB ;
    return read_flash(block_idx, page_idx);    
}

void gc_flash(){    
    uint32_t gc_target =  delete_heap(&(dev->channel[gc_cur_ch].usingblock));
    total_gc++;
    //printf("target = %d, target_ch = %d\n", gc_target, gc_cur_ch);
    for(int i=0;i<PPB;i++){        
        gc_read++;
        if(dev->block[gc_target].page[i].valid == VALID){
            uint32_t data = read_flash(gc_target,i);
            gc_write++;
            write_flash(data);
        }            
        // dev->block[gc_target].page[i].valid == ERASE;
    }
    enqueue(&(dev->channel[gc_cur_ch].freeblock),gc_target);
    dev->block[gc_target].wear += 5;
    gc_cur_ch = (gc_cur_ch + 1) % NOC;
}

