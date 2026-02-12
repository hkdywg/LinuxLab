/*
 *  queue.h
 *  brief
 *      queue operation basic definition  	
 *  
 *  (C) 2026.02.12 <hkdywg@163.com>
 *
 *  This program is free software; you can redistribute it and/r modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 * */

#ifndef __QUEUE_H_
#define __QUEUE_H_

#include "list.h"

/*
 * Queue list struture
 */
struct queue_data {
    unsigned int type;
    void *data;
    unsigned int size;
};
typedef struct queue_data queue_data_t;

/*
 * Queue node structure
 */
struct queue_node {
    list_t list;
    struct queue_data qdat;
};
typedef struct queue_node queue_node_t;

/*
 * initialize a queue
 * 
 * pq: queue to be initialized
 */
void queue_init(struct queue_node *pq);

/*
 * destroy a queue
 * 
 * pq: queue to be destroyed
 */
void queue_destroy(struct queue_node *pq);

/*
 * push a queue
 * 
 * pq:   queue to be push
 * data: data of pq
 */
void queue_push(struct queue_node *pq, struct queue_data *data);

/*
 * push queue urgent
 *
 * pq:   queue to be push
 * data: data of pq
 */
void queue_push_urgent(struct queue_node *pq, struct queue_data *data);


/*
 * pop a queue
 *
 * pq:   queue to be pop
 */
void queue_pop(struct queue_node *pq);

/*
 * get the head element of pq
 *
 * pq: queue pointer
 */
struct queue_data *queue_head(struct queue_node *pq);

/*
 * get the tail element of pq
 *
 * pq: queue pointer
 */
struct queue_data *queue_tail(struct queue_node *pq);

/*
 * check queue is empty
 */
bool queue_empty(struct queue_node *pq);

/*
 * get the size of pq
 * 
 * pq: queue pointer
 */
unsigned int queue_size(struct queue_node *pq);

#endif
