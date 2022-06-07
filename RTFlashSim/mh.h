#ifndef MH_H
#define MH_H

#include "config.h"
 
typedef struct{    
    int key;      
    int index;  
}element;    
 
typedef struct{   
    element heap[NOB];     
    int size;
}heap;
 
 
 void init_heap(heap*h){
     h->size = 0;
 }
 
void insert_heap(heap* h, int key, int index){
    element item;
    item.key = key;
    item.index = index;

    int i;
    i = ++(h->size);   
    
 
    while( (i != 1) && (item.key < h->heap[i/2].key) ){
        h->heap[i] = h->heap[i/2];    
        i /= 2;     
    }    
    h->heap[i] = item;      
}
 
int delete_heap(heap* h){
    int parent, child;
    element item, temp;
    
    item = h->heap[1];    
    temp = h->heap[(h->size)--];   
    parent = 1;     
    child = 2; 
    
    while(child <= h->size){

        if( (child < h->size) && ((h->heap[child].key) < h->heap[child+1].key )){
            child++;   
        }
        
 
        if(temp.key >= h->heap[child].key){
            break;
        } 
        

        h->heap[parent] = h->heap[child];
        

        parent = child;
        child *= 2;
    }    
    h->heap[parent] = temp;  
    return item.index;        
} 
 #endif