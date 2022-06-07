#include "mh.h"
#include "queue.h"

#ifndef TYPE_H
#define TYPE_H
typedef struct page_t{
    uint32_t data;
    uint8_t valid;
    uint32_t oob;
}page_t;

typedef struct block_t{
    page_t page[PPB];
    uint32_t channel;
    bool free;
    uint32_t index;
    uint32_t cur; //for current write index
    uint32_t wear;
    uint32_t invalid_num; //for garbage collection selection
}block_t;

typedef struct channel_t{
    queue freeblock;
    heap usingblock;
}channel_t;

typedef struct flash{
    block_t * block;
    channel_t * channel;
}flash_t;

#endif