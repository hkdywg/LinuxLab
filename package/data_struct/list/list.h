/*
 *  list.h
 *  brief
 *  	list releted definitions of s-kernel 
 *  
 *  (C) 2025.03.14 <hkdywg@163.com>
 *
 *  This program is free software; you can redistribute it and/r modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 * */
#ifndef __LIST_H_
#define __LIST_H_

#include <stdbool.h>


/*
 * container_of
 * brief
 * 		return the the member address of ptr
 */
#define container_of(ptr, type, member) \
	((type *)((char *)ptr - (unsigned long)(&((type *)0)->member)))

/*
 *		----------------------------------------------------- 
 *      |                                                   | 
 *      |    node        node        node        node       | 
 *      |   +++++++     +++++++     +++++++     +++++++     | 
 *      --->|     |     |     |     |     |     |     |     | 
 *          |  n  |---->|  n  |---->|  n  |---->|  n  |------ 
 *      ----|  p  |<----|  p  |<----|  p  |<----|  p  |       
 *      |   |     |     |     |     |     |     |     |<----- 
 *      |   +++++++     +++++++     +++++++     +++++++     | 
 *      |                                                   | 
 *       ---------------------------------------------------- 
 */


/*
 * Double list structure
 */
struct list_node
{
	struct list_node *next;			/* point to next node */
	struct list_node *prev;			/* point to prev node */
};
typedef struct list_node list_t;

/*
 * list_entry
 * brief 
 * 		get the struct for this entry
 * 	param
 * 		ptr: the struct list_node pointer
 * 		type: the type of the struct this is embedded in
 * 		member: the name of the list_struct within the struct
 */
#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

/*
 * list_first_entry
 * brief 
 * 		get the struct for the list first entry
 * 	param
 * 		ptr: the struct list head pointer
 * 		type: the type of the struct this is embedded in
 * 		member: the name of the list_struct within the struct
 */
#define list_first_entry(ptr, type, member) \
	container_of(ptr->next, type, member)

/*
 * list_last_entry
 * brief 
 * 		get the struct for the list last entry
 * 	param
 * 		ptr: the struct list head pointer
 * 		type: the type of the struct this is embedded in
 * 		member: the name of the list_struct within the struct
 */
#define list_last_entry(ptr, type, member) \
	container_of(ptr->prev, type, member)


/*
 * list_for_each
 * brief 
 * 		iterate over a list
 * 	param
 * 		pos: the list_t * to use as a loop cursor
 * 		head: the head for your list
 */
#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/*
 * Initialize a list head
 */
#define LIST_HEAD_INIT(name) {&(name), &(name)}

#define LIST_HEAD(name) \
	list_t name = LIST_HEAD_INIT(name)


/*
 * INIT_LIST_HEAD
 * brief
 * 		Initialize a list head
 * param
 * 		head: list head
 */
static inline void INIT_LIST_HEAD(list_t *head)
{
	head->prev = head->next = head;
}

/*
 * list_init
 * brief 
 * 		initialize of a list
 * 	param
 * 		list: list to be initialized
 */
static inline void list_init(list_t *list)
{
	list->next = list->prev = list;
}

/*
 * __list_add
 * brief 
 * 		Insert a new list between two known consecutive entries
 * 	param
 * 		new: new entry to be added
 */
static inline void __list_add(list_t *new, list_t *prev,
					  list_t *next)
{
	next->prev = new;
	new->next  = next;
	new->prev = prev;
	prev->next = new;
}

/*
 * __list_del
 * brief 
 * 		delte a list entry by making the prev/next enties
 * 	param
 * 		new: new entry to be added
 */
static void inline __list_del(list_t *prev, list_t *next)
{
	next->prev = prev;
	prev->next = next;
}

/*
 * list_add
 * brief 
 * 		Insert a new entry
 * 	param
 * 		new: new entry to be added
 * 		head: list head to be add it after
 */
static inline void list_add(list_t *head, list_t *new)
{
	__list_add(new, head, head->next);
}

/*
 * list_add_tail
 * brief 
 * 		Insert a new entry
 * param
 * 		new: new entry to be added
 * 		head: list head to be add it befor
 */
static inline void list_add_tail(list_t *head, list_t *new)
{
	__list_add(new, head->prev, head);
}

/*
 * list_del
 * brief 
 * 		delete a entry
 * param
 * 		entry: list node of need to be delete 
 */
static inline void list_del(list_t *entry)
{
	__list_del(entry->prev, entry->next);
	INIT_LIST_HEAD(entry);
}

/*
 * list_empty
 * brief 
 * 		test whether a list is empty
 * param
 * 		head: the list to test
 */
static inline bool list_empty(const list_t *head)
{
	return head->next == head;
}

/*
 * list_replace
 * brief
 * 		new list entry replace old	
 * param
 * 		old: old entry to be replaced
 * 		new: new entry
 */
static inline void list_replace(list_t *old, list_t *new)
{
	old->prev->next = new;
	new->prev = old->prev;
	new->next = old->next;
	old->next->prev = new;

	INIT_LIST_HEAD(old);
}

/*
* list_move
 * brief
 * 		move a list entry to another list head
 * param
 * 		entry: list node that need to be moved
 * 		head: list head
 */
static inline void list_move(list_t *entry, list_t *head)
{
	list_del(entry);
	list_add(head, entry);
}

/*
 * list_move_tail
 * brief
 * 		move a list entry to another list tail
 * param
 * 		entry: list node that need to be moved
 * 		head: list head
 */
static inline void list_move_tail(list_t *entry, list_t *head)
{
	list_del(entry);
	list_add_tail(head, entry);
}

/*
 * list_bulk_move_tail
 * brief
 * 		move a list to another list tail
 * param
 * 		first: first list node of separation list
 * 		last: last list node of separation list
 * 		head: list head
 */
static inline void list_bulk_move_tail(list_t *first, list_t *last, list_t *head)
{
	first->prev->next = last->next;
	last->next->prev = first->prev;

	head->prev->next = first;
	first->prev = head->prev;
	
	last->next = head;
	head->prev = last;
}

/*
 * list_is_first
 * brief
 * 		test a list node is the list first
 * param
 * 		entry: list node need to test
 * 		head: list head
 */
static inline bool list_is_first(const list_t *entry, const list_t *head)
{
	return entry->prev == head;
}

/*
 * list_is_last
 * brief
 * 		test a list node is the list last
 * param
 * 		entry: list node need to test
 * 		head: list head
 */
static inline bool list_is_last(const list_t *entry, const list_t *head)
{
	return entry->next == head;
}


/*
 * list_rotate_left
 * brief
 * 		list left rotate
 * param
 * 		head: list head
 */
static inline void list_rotate_left(list_t *head)
{
	list_t *first;
	if(!list_empty(head)) {
		first = head->next;
		list_move_tail(first, head);
	}
}

/*
 * list_rotate_right
 * brief
 * 		list right rotate
 * param
 * 		head: list head
 */
static inline void list_rotate_right(list_t *head)
{
	list_t *first;
	if(!list_empty(head)) {
		first = head->next;
		list_move(first, head);
	}
}

/*
 * list_is_singular
 * breif
 * 		test the list is just include one node
 * param
 * 		head: list to be test
 */
static inline bool list_is_singular(list_t *head)
{
	return !list_empty(head) && (head->next == head->prev);
}

/*
 * list_is_head
 * brief
 * 		test the entry is head
 * param
 * 		head: list head
 * 		entry: list entry to be test
 */
static inline bool list_is_head(list_t *head, list_t *entry)
{
	return head == entry;
}

/*
 *	__list_cut_position
 *	brief
 *		cut list from the entry position to two list(new list include entry)
 *	param
 *		list: new list head
 *		head: origin list head
 *		entry: the specifical list node to cut
 */
static inline void __list_cut_position(list_t *list, list_t *head, list_t *entry)
{
	list_t *new_first = entry->next;
	list->next = head->next;
	head->next->prev = list;
	list->prev = entry;
	entry->next = list;

	head->next = new_first;
	new_first->prev = head;
}

/*
 *	list_cut_position
 *	brief
 *		cut list from the entry position to two list(new list include entry)
 *	param
 *		list: new list head
 *		head: origin list head
 *		entry: the specifical list node to cut
 */
static inline void list_cut_position(list_t *list, list_t *head, list_t *entry)
{
	if(list_empty(head))
		return;
	if(list_is_singular(head) && !(list_is_head(head, entry) && (entry != head->next)))
		return;
	if(list_is_head(head, entry))
		INIT_LIST_HEAD(list);
	else
		__list_cut_position(list, head, entry);
}

/*
 * list_cut_befor
 * brief
 * 		cut list form the entry node to two list(new list not include entry)
 *	param
 *		list: new list head
 *		head: origin list head
 *		entry: the specifical list node to cut
 */
static inline void list_cut_befor(list_t *list, list_t *head, list_t *entry)
{
	if(head->next == entry) {
		INIT_LIST_HEAD(list);
		return;
	}
	list->next = head->next;
	head->next->prev = list;
	list->prev = entry->prev;
	entry->prev->next = list;

	head->next = entry;
	entry->prev = head;
}


/*
 * __list_splice
 * brief
 * 		two list splice
 */
static inline void __list_splice(const list_t *list, list_t *prev, list_t *next)
{
	list_t *first = list->next;
	list_t *last = list->prev;

	last->next = next;
	next->prev = last;

	first->prev = prev;
	prev->next = first;
}

/*
 * list_splice
 * brief
 *		two list splice 		
 * param
 * 		list: new list to be insert
 * 		head: list head
 */
static inline void list_splice(const list_t *list, list_t *head)
{
	if(!list_empty(list))
		__list_splice(list, head, head->next);
}



#endif
