/*                                                                                                                                                                     
 *  ring_buffer.h
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
#ifndef __RING_BUFFER_H_
#define __RING_BUFFER_H_

/* basic data type definitions */
typedef signed char       int8_t;      /* 8bit integer type */
typedef signed short      int16_t;     /* 16bit integer type */
typedef signed int        int32_t;     /* 32bit integer type */
typedef unsigned char     uint8_t;     /* 8bit unsigned integer type */
typedef unsigned short    uint16_t;    /* 16bit unsigned integer type */
typedef unsigned int      uint32_t;    /* 32bit unsigned integer type */
typedef signed long       int64_t;     /* 64bit integer type */
typedef unsigned long     uint64_t;    /* 64bit unsigned integer type */
typedef int               bool_t;      /* boolean type */

/*
 * ring buffer structure
 */
struct ring_buffer
{
	uint8_t *buffer;

	uint16_t read_mirror : 1;
	uint16_t read_index : 15;
	uint16_t write_mirror : 1;
	uint16_t write_index : 15;

	int16_t buffer_size;
};

enum ring_buffer_state
{
	RING_BUFFER_EMPTY,
	RING_BUFFER_FULL,
	RING_BUFFER_HALFFULL,
};

/*
 *  ring buffer operation interfaces
 */
uint16_t ring_buffer_data_len(struct ring_buffer *rb);
uint16_t ring_buffer_space_len(struct ring_buffer *rb);
void ring_buffer_init(struct ring_buffer *rb,
						 uint8_t *buf,
						 uint16_t size);
struct ring_buffer *ring_buffer_create(uint16_t size);
void ring_buffer_destroy(struct ring_buffer *rb);
uint16_t  ring_buffer_put(struct ring_buffer *rb,
							  const uint8_t *buf,
							  uint16_t len);
uint16_t  ring_buffer_get(struct ring_buffer *rb,
							  const uint8_t *buf,
							  uint16_t len);
uint16_t ring_buffer_putchar(struct ring_buffer *rb, const uint8_t data);
uint16_t ring_buffer_getchar(struct  ring_buffer *rb, uint8_t *data);
uint16_t ring_buffer_put_force(struct ring_buffer *rb,
								  const uint8_t *buf,
								  uint16_t len);
uint16_t ring_buffer_putchar_force(struct ring_buffer *rb, const uint8_t data);
#endif
