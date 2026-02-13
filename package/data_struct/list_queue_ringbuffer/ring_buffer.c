/*                                                                                                                                                                     
 *  ring_buffer.c
 *
 *  brief
 *      ring buffer 
 *  
 *  (C) 2025.02.08 <hkdywg@163.com>
 *
 *  This program is free software; you can redistribute it and/r modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */
#include "ring_buffer.h"
#include <string.h>
#include <stdlib.h>

enum ring_buffer_state get_ring_buffer_status(struct ring_buffer *rb)
{
	if(rb->read_index == rb->write_index)
	{
		if(rb->read_mirror == rb->write_mirror)
			return RING_BUFFER_EMPTY;
		else
			return RING_BUFFER_FULL;
	}
	return RING_BUFFER_HALFFULL;
}

/*
 * ring_buffer_data_len
 * brief
 * 		get the size of data in ring buffer
 * param
 * 		rb: the pointer of ring buffer object
 */
uint16_t ring_buffer_data_len(struct ring_buffer *rb)
{
	switch(get_ring_buffer_status(rb))
	{
		case RING_BUFFER_EMPTY:
			return 0;
		case RING_BUFFER_FULL:
			return rb->buffer_size;
		case RING_BUFFER_HALFFULL:
			if(rb->write_index > rb->read_index)
				return rb->write_index - rb->read_index;
			else
				return rb->buffer_size - (rb->read_index - rb->write_index);
	}

    return 0;
}

/*
 * ring_buffer_space_len
 * brief
 * 		get the size of empty space in ring buffer 
 * param
 * 		rb: the pointer of ring buffer object
 */
uint16_t ring_buffer_space_len(struct ring_buffer *rb)
{
	return (rb->buffer_size - ring_buffer_data_len(rb));
}

/*
 * ring_buffer_init
 * brief
 * 		initialize the ring buffer
 * param
 * 		rb: the pointer of ring buffer object
 * 		buf: data buffer
 * 		size: the size of data buffer
 */
void ring_buffer_init(struct ring_buffer *rb,
						 uint8_t *buf,
						 uint16_t size)
{
	/* initialize read write index */
	rb->read_mirror = rb->read_index = 0;
	rb->write_mirror = rb->write_index = 0;

	/* set ring buffer and size */
	rb->buffer  	= buf;
	rb->buffer_size = size;

}

/*
 * ring_buffer_create
 * brief
 * 		create a ring buffer with given size
 * param
 * 		size: the size of ring buffer
 */
struct ring_buffer *ring_buffer_create(uint16_t size)
{
	struct ring_buffer *rb;
	uint8_t *buf;

	rb = (struct ring_buffer *)malloc(sizeof(struct ring_buffer));
	if(rb == NULL)
		return NULL;

	buf = (uint8_t *)malloc(size);
	if(buf == NULL) {
		free(rb);
		return NULL;
	}
	/* initialize the ring buffer */
	ring_buffer_init(rb, buf, size);

	return rb;
}

/*
 * ring_buffer_destroy
 * brief
 * 		destroy the ring buffer object
 * param
 * 		rb: the pointer of ring buffer, need to be destroyed
 * 
 */
void ring_buffer_destroy(struct ring_buffer *rb)
{
	if(rb != NULL) {
		free(rb->buffer);
		free(rb);
	}
}

/*
 * ring_buffer_put
 * brief
 * 		put a block data to the ring buffer
 * param
 * 		rb: the pointer of ring buffer object
 * 		buf: the pointer of data buffer, need to be put
 * 		len: the size of data 
 */
uint16_t  ring_buffer_put(struct ring_buffer *rb,
							  const uint8_t *buf,
							  uint16_t len)
{
	uint16_t size, side_len, put_len = len;

	/* whether has enough space */
	size = ring_buffer_space_len(rb);

	/* if no space */
	if(size == 0)
		return 0;

	/* drop some data */
	if(size < len)
		put_len = size;

	side_len = rb->buffer_size - rb->write_index;

	if(side_len > put_len) {
		memcpy((void *)&rb->buffer[rb->write_index], (const void *)buf, put_len);	
		rb->write_index += put_len;
		return put_len;
	}

	memcpy(&rb->buffer[rb->write_index], &buf[0], side_len);
	memcpy(&rb->buffer[0], &buf[side_len], put_len - side_len);
	/* need to use other side of the mirror */
	rb->write_mirror = ~rb->write_mirror;
	rb->write_index = put_len - side_len;

	return put_len;
}

/*
 * ring_buffer_get
 * brief
 * 		get a block data from ring buffer
 * param
 * 		rb: the pointer of ring buffer object
 * 		buf: the pointer of data buffer, save the get data
 * 		len: the size of data needed to be read from ring buffer 
 */
uint16_t  ring_buffer_get(struct ring_buffer *rb,
							  const uint8_t *buf,
							  uint16_t len)
{
	uint16_t size, side_len, get_len  = len;

	/*  whether has enough data */
	size = ring_buffer_data_len(rb);

	/* no data */
	if(size == 0)
		return 0;

	/* less data */
	if(size < len)
		get_len = size;

	side_len = rb->buffer_size - rb->read_index;
	if(side_len > get_len) {
		memcpy((void *)buf, (const void *)&rb->buffer[rb->read_index], get_len);
		rb->read_index += get_len;
		return get_len;
	}

	memcpy((void *)&buf[0], (const void  *)&rb->buffer[rb->read_index], side_len);
	memcpy((void *)&buf[side_len], (const void  *)&rb->buffer[0], get_len - side_len);

	/* need to use other side of the mirror */
	rb->read_mirror = ~rb->read_mirror;
	rb->read_index = get_len - side_len;

	return get_len;
}

/*
 * ring_buffer_putchar
 * brief
 * 		put one data to the ring buffer
 */
uint16_t ring_buffer_putchar(struct ring_buffer *rb, const uint8_t data)
{
	/* whether has enougn space */
	if(!ring_buffer_space_len(rb))
		return 0;

	rb->buffer[rb->write_index] = data;

	if(rb->write_index + 1 == rb->buffer_size) {
		rb->write_index = 0;
		rb->write_mirror = ~rb->write_mirror;
	} else {
		rb->write_index++;
	}
		
	return 1;
}

/*
 * ring_buffer_getchar
 * brief
 * 		get one data from ring buffer
 */
uint16_t ring_buffer_getchar(struct  ring_buffer *rb, uint8_t *data)
{
	/* whether ring buffer is empty */
	if(!ring_buffer_data_len(rb))
		return 0;

	*data = rb->buffer[rb->read_index];

	if(rb->read_index + 1 == rb->buffer_size) {
		rb->read_index = 0;
		rb->read_mirror =  ~rb->read_mirror;
	} else {
		rb->read_index ++;
	}
	
	return 1;
}

/*
 * ring_buffer_put_force
 */
uint16_t ring_buffer_put_force(struct ring_buffer *rb,
								  const uint8_t *buf,
								  uint16_t len)
{
	uint16_t space_size, side_len, put_len = len;

	/* whether has enough space */
	space_size = ring_buffer_space_len(rb);

	side_len = rb->buffer_size - rb->write_index;

	if(side_len > put_len) {
		memcpy(&rb->buffer[rb->write_index], buf, put_len);	
		rb->write_index += put_len;
		return put_len;
	}

	memcpy(&rb->buffer[rb->write_index], &buf[0], side_len);
	memcpy(&rb->buffer[0], &buf[side_len], put_len - side_len);
	/* need to use other side of the mirror */
	rb->write_mirror = ~rb->write_mirror;
	rb->write_index = put_len - side_len;

	if(put_len > space_size) {
		if(rb->write_index <= rb->read_index)
			rb->read_mirror = ~rb->read_mirror;
		rb->read_index = rb->write_index;
	}

	return put_len;
}

/*
 * ring_buffer_putchar_force
 * brief
 * 		put one data to the ring buffer
 */
uint16_t ring_buffer_putchar_force(struct ring_buffer *rb, const uint8_t data)
{
	enum ring_buffer_state state = get_ring_buffer_status(rb);
	rb->buffer[rb->write_index] = data;

	if(rb->write_index + 1 == rb->buffer_size) {
		rb->write_index = 0;
		rb->write_mirror = ~rb->write_mirror;
		if(state == RING_BUFFER_FULL) {
			rb->write_mirror = ~rb->write_mirror;
			rb->read_index = rb->write_index;
		}
	} else {
		rb->write_index++;
		if(state == RING_BUFFER_FULL) {
			rb->read_index = rb->write_index;
		}
	}
		
	return 1;
}

