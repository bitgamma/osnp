/*
 * Copyright (C) 2014, Michele Balistreri
 *
 * Derived from code originally Copyright (C) 2011, Alex Hornung
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef TLV_H
#define	TLV_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Reads the TLV tag in the buffer and stores it in the out_tag parameter.
 *
 * @param buf the buffer
 * @param out_tag the output tag
 * @return the length, in bytes of the read tag
 */
uint16_t tlv_read_tag(uint8_t *buf, uint16_t *out_tag);

/**
 * Reads the TLV length in the buffer and stores it in the out_len parameter.
 *
 * @param buf the buffer
 * @param out_len the output length
 * @return the length, in bytes of the read length
 */
uint16_t tlv_read_length(uint8_t *buf, uint16_t *out_len);

/**
 * Writes the given TLV tag in the given buffer.
 *
 * @param buf the buffer
 * @param in_tag the tag to write
 * @return the length of the written tag
 */
uint16_t tlv_write_tag(uint8_t *buf, uint16_t in_tag);

/**
 * Writes the given TLV length in the given buffer.
 *
 * @param buf the buffer
 * @param in_len the length to write
 * @return the length of the written length
 */
uint16_t tlv_write_length(uint8_t *buf, uint16_t in_len);

/**
 * Writes the undefined length tag in the given buffer.
 *
 * @param buf the buffer
 * @return the length of the written length
 */
uint16_t tlv_write_undefined_length(uint8_t *buf);

/**
 * Writes the terminator of the undefined length tag in the given buffer.
 *
 * @param buf the buffer
 * @return the length of the written length
 */
uint16_t tlv_write_undefined_length_terminator(uint8_t *buf);

#endif	/* TLV_H */

