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

#include "queue.h"
#include <string.h>
#include <stdlib.h>

/*
 * initialize a queue
 * 
 * pq: queue to be initialized
 */
void queue_init(struct queue_node *pq)
{
    list_init(&pq->list);
    memset(&pq->qdat, 0, sizeof(struct queue_data));
}

/*
 * destroy a queue
 * 
 * pq: queue to be destroyed
 */
void queue_destroy(struct queue_node *pq)
{
    list_t *pos;
    list_t *head = &pq->list;

    list_for_each(pos, head) {
        list_del(pos);
        free(list_entry(pos, struct queue_node, list));
    }
}

/*
 * push a queue
 * 
 * pq:   queue to be push
 * data: data of pq
 */
void queue_push(struct queue_node *pq, struct queue_data *data)
{
    struct queue_node *q = (struct queue_node *)malloc(sizeof(struct queue_node));

    memcpy(&q->qdat, data, sizeof(struct queue_data));

    list_add(&pq->list, &q->list);
}

/*
 * push queue urgent
 *
 * pq:   queue to be push
 * data: data of pq
 */
void queue_push_urgent(struct queue_node *pq, struct queue_data *data)
{
    struct queue_node *q = (struct queue_node *)malloc(sizeof(struct queue_node));

    memcpy(&q->qdat, data, sizeof(struct queue_data));

    list_add_tail(&pq->list, &q->list);
}


/*
 * pop a queue
 *
 * pq:   queue to be pop
 */
void queue_pop(struct queue_node *pq)
{
    list_t *head = &pq->list;
    list_t *pos = head->next;

    list_del(pos);
    free(list_entry(pos, struct queue_node, list));
}

/*
 * get the head element of pq
 *
 * pq: queue pointer
 */
struct queue_data *queue_head(struct queue_node *pq)
{
    list_t *pos = pq->list.next;
    struct queue_node *q = list_entry(pos, struct queue_node, list);

    return &q->qdat;
}

/*
 * get the tail element of pq
 *
 * pq: queue pointer
 */
struct queue_data *queue_tail(struct queue_node *pq)
{
    list_t *pos = pq->list.prev;
    struct queue_node *q = list_entry(pos, struct queue_node, list);

    return &q->qdat;
}

/*
 * check queue is empty
 */
bool queue_empty(struct queue_node *pq)
{
    return (list_empty(&pq->list));
}

/*
 * get the size of pq
 * 
 * pq: queue pointer
 */
unsigned int queue_size(struct queue_node *pq)
{
    unsigned int len = 0;
    const list_t *p = &pq->list;

    while(p->next != &pq->list) {
        p = p->next;
        len ++;
    }

    return len;
}
