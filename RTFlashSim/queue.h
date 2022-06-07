#ifndef QUEUE_H
#define QUEUE_H

typedef struct node 
{
    int data;
    struct node *next;
}node;

typedef struct queue 
{
    node *front; 
    node *rear; 
    int count;
}queue;

void init_queue(queue *queue);
int isEmpty(queue *queue); 
void enqueue(queue *queue, int data); 
int dequeue(queue *queue); 


void init_queue(queue *queue)
{    
    queue->front = queue->rear = NULL; 
    queue->count = 0;
}

int isEmpty(queue *queue)
{
    return queue->count == 0;   
}

void enqueue(queue *queue, int data)
{
    node *now = (node *)malloc(sizeof(node)); 
    now->data = data;
    now->next = NULL;

    if (isEmpty(queue))
    {
        queue->front = now;   
    }
    else
    {
        queue->rear->next = now;
    }
    queue->rear = now;  
    queue->count++;
}

int dequeue(queue *queue)
{
    int re = 0;
    node *now;
    if (isEmpty(queue))
    {
        printf("queue empty!\n");
        return -1;
    }
    now = queue->front;
    re = now->data;
    queue->front = now->next;
    free(now);
    queue->count--;
    return re;
}
#endif