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

#include "tlv.h"
#include "config.h"

unsigned int tlv_read_tag(unsigned char *buf, unsigned int *out_tag) {
  unsigned int i = 0;

  *out_tag = buf[i++];

  if((*out_tag & 0x1F) == 0x1F) {
    do {
      *out_tag = *out_tag << 8 | buf[i++];
    } while(*out_tag & 0x80);
  }

  return i;
}

unsigned int tlv_read_length(unsigned char *buf, unsigned int *out_len) {
  unsigned int i = 0;
  *out_len = buf[i++];

  if (*out_len > 0x7f) {
    unsigned int lenOfLen = *out_len & 0x7f;
    *out_len = 0;

    while(lenOfLen--) {
      *out_len = (*out_len << 8) | buf[i++];
    }
  }

  return i;
}

unsigned int tlv_write_tag(unsigned char *buf, unsigned int in_tag) {
  int max_shift = (sizeof(unsigned int) - 1) * 8;
  unsigned int i = 0;

  while((in_tag >> max_shift) == 0x00) {
    max_shift -= 8;
  }

  while(max_shift >= 0) {
    buf[i++] = ((in_tag >> max_shift) & 0xff);
    max_shift -= 8;
  }

  return i;
}

unsigned int tlv_write_length(unsigned char *buf, unsigned int in_len) {
  unsigned int i = 0;

  if (in_len <= 0x7f) {
    buf[i++] = in_len;
  } else if (in_len <= 0xff) {
    buf[i++] = 0x81;
    buf[i++] = in_len;
  } else 
#ifndef TLV_MAX_INT_16_BIT    
  if (in_len <= 0xffff)
#endif
  {
    buf[i++] = 0x82;
    buf[i++] = in_len >> 8;
    buf[i++] = in_len & 0xff;
  } 
#ifdef TLV_MAX_INT_32_BIT
    else if (in_len <= 0xffff) {
    buf[i++] = 0x83;
    buf[i++] = in_len >> 16;
    buf[i++] = (in_len >> 8) & 0xff;
    buf[i++] = in_len & 0xff;
  } else {
    buf[i++] = 0x84;
    buf[i++] = (in_len >> 24) & 0xff;
    buf[i++] = (in_len >> 16) & 0xff;
    buf[i++] = (in_len >> 8) & 0xff;
    buf[i++] = in_len & 0xff;
  }
#endif

  return i;
}

unsigned int tlv_write_undefined_length(unsigned char *buf) {
  buf[0] = 0x80;
  
  return 1;
}

unsigned int tlv_write_undefined_length_terminator(unsigned char *buf) {
  buf[0] = 0x00;
  buf[1] = 0x00;

  return 2;
}