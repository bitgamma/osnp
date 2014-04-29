/*
 * Copyright (C) 2014, Michele Balistreri
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

#include "osnp.h"
#include "config.h"

#include <stdlib.h>
#include <string.h>

#define SCANNING_CHANNELS 0
#define WAITING_ASSOCIATION_REQUEST 1
#define ASSOCIATED 2

static unsigned char OSNP_PAN_ID[2] = { 0x00, 0x00 };
static unsigned char OSNP_SHORT_ADDRESS[2] = { 0x00, 0x00 };
static unsigned char OSNP_EUI[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

static unsigned char seq_no;
static unsigned char state;
static unsigned char channel;

void osnp_initialize(void) {
  osnp_initialize_radio();

  osnp_load_eui(OSNP_EUI);
  osnp_load_pan_id(OSNP_PAN_ID);
  osnp_load_short_address(OSNP_SHORT_ADDRESS);
  osnp_load_channel(&channel);

  seq_no = 0;

  if (channel == 0xff) {
    channel = 0;
    state = SCANNING_CHANNELS;
  } else {
    state = ASSOCIATED;
  }

  osnp_switch_channel(channel);
}

void osnp_timer_expired_cb() {
  switch(state) {
    case SCANNING_CHANNELS:
      channel = (channel + 1) % 16;
      osnp_switch_channel(channel);
      osnp_start_channel_scanning_timer();
      break;
    case WAITING_ASSOCIATION_REQUEST:
      state = SCANNING_CHANNELS;
      osnp_start_channel_scanning_timer();
      break;
    case ASSOCIATED:
      osnp_poll();
      osnp_start_poll_timer();
      break;
  }
}

void _osnp_handle_discovery_request(struct ieee802_15_4_frame *frame) {
  //TODO: send discovery frame first!
  
  state = WAITING_ASSOCIATION_REQUEST;

  if (state == SCANNING_CHANNELS) {
    osnp_stop_channel_scanning_timer();
  } else {
    osnp_stop_association_wait_timer();
  }
  
  osnp_start_association_wait_timer();
}

void _osnp_handle_association_request(struct ieee802_15_4_frame *frame) {
  //TODO: handle association
}

_osnp_handle_disassociation_notification(struct ieee802_15_4_frame *frame) {
  //TODO: verify security
  OSNP_PAN_ID[0] = 0xff;
  OSNP_PAN_ID[1] = 0xff;

  OSNP_SHORT_ADDRESS[0] = 0xff;
  OSNP_SHORT_ADDRESS[1] = 0xff;

  osnp_write_pan_id(OSNP_PAN_ID);
  osnp_write_short_address(OSNP_SHORT_ADDRESS);

  channel = 0xff;
  osnp_write_channel(&channel);
  channel = 0;
}


void _osnp_mac_command_frame_received_cb(struct ieee802_15_4_frame *frame) {
  if (state != ASSOCIATED) {
    switch (frame->payload[0]) {
      case OSNP_MCMD_DISCOVER:
        _osnp_handle_discovery_request(frame);
        break;
      case OSNP_MCMD_ASSOCIATION_REQ:
        _osnp_handle_association_request(frame);
        break;
    }
  } else {
    switch (frame->payload[0]) {
      case OSNP_MCMD_DISASSOCIATED:
        _osnp_handle_disassociation_notification(frame);
        break;
    }
  }
}


void _osnp_data_frame_received_cb(struct ieee802_15_4_frame *frame) {

}

void osnp_frame_received_cb(unsigned char *frame_buf, int frame_len) {
  struct ieee802_15_4_frame frame;
  osnp_parse_frame(frame_buf, frame_len, &frame);

  switch (EXTRACT_FCFRTYP(*frame.fc_low)) {
    case FCFRTYP_DATA:
      _osnp_data_frame_received_cb(&frame);
      break;
    case FCFRTYP_MCMD:
      _osnp_mac_command_frame_received_cb(&frame);
      break;
  }
}

void osnp_frame_sent_cb(unsigned char status) {

}

void osnp_poll() {
  //TODO: send a poll packet
}

void osnp_enter_runloop() {
  while(1) {
    switch(state) {
    case SCANNING_CHANNELS:
      osnp_start_channel_scanning_timer();
      break;
    case ASSOCIATED:
      osnp_start_poll_timer();
      break;
    }

    osnp_idle();
  }
}

unsigned char *_osnp_parse_basic_header(unsigned char *buf, struct ieee802_15_4_frame *frame) {
  frame->backing_buffer = buf;
  frame->fc_low = buf++;
  frame->fc_high = buf++;
  frame->seq_no = buf++;

  if (EXTRACT_FCDSTADDR(*frame->fc_high) != FCADDR_NONE) {
    frame->dst_pan = buf;
    buf += 2;
  } else {
    frame->dst_pan = NULL;
    frame->dst_addr = NULL;
  }

  if (EXTRACT_FCDSTADDR(*frame->fc_high) == FCADDR_SHORT) {
    frame->dst_addr = buf;
    buf += 2;
  } else if (EXTRACT_FCDSTADDR(*frame->fc_high) == FCADDR_EXT) {
    frame->dst_addr = buf;
    buf += 8;
  }

  if (!((EXTRACT_FCSRCADDR(*frame->fc_high) == FCADDR_NONE) || EXTRACT_FCPANCOMP(*frame->fc_low))) {
    frame->src_pan = buf;
    buf += 2;
  } else {
    frame->src_pan = NULL;
  }

  if (EXTRACT_FCSRCADDR(*frame->fc_high) == FCADDR_SHORT) {
    frame->src_addr = buf;
    buf += 2;
  } else if (EXTRACT_FCSRCADDR(*frame->fc_high) == FCADDR_EXT) {
    frame->src_addr = buf;
    buf += 8;
  } else {
    frame->src_addr = NULL;
  }

  return buf;
}

unsigned char *_osnp_parse_security_header(unsigned char *buf, struct ieee802_15_4_frame *frame) {
  if (EXTRACT_FCSECEN(*frame->fc_low) && EXTRACT_FCFRVER(*frame->fc_high)) {
    frame->sc = buf++;
    frame->frame_counter = buf;
    buf += 4;

    switch(EXTRACT_KEYIDM(*frame->sc)) {
      case KIM_IMPLICIT:
        frame->key_id = NULL;
        break;
      case KIM_1IDX:
        frame->key_id = buf;
        buf += 1;
        break;
      case KIM_4SRC_1IDX:
        frame->key_id = buf;
        buf += 5;
        break;
      case KIM_8SRC_1IDX:
        frame->key_id = buf;
        buf += 9;
        break;
    }
  } else {
    frame->sc = NULL;
    frame->frame_counter = NULL;
    frame->key_id = NULL;
  }

  frame->header_len = buf - frame->backing_buffer;
  frame->payload = buf;

  return buf;
}

void osnp_parse_frame(unsigned char *buf, unsigned int frame_len, struct ieee802_15_4_frame *frame) {
  buf = _osnp_parse_basic_header(buf, frame);
  buf = _osnp_parse_security_header(buf, frame);
  
  // The last two bytes are the fcs, which is calculated/verified at a lower layer
  frame->payload_len = frame_len - frame->header_len - 2;
}

void osnp_initialize_frame(unsigned char fc_low, unsigned char fc_high, unsigned char sc, unsigned char *buf, struct ieee802_15_4_frame *frame) {
  buf[0] = fc_low;
  buf[1] = fc_high;
  buf[2] = seq_no++;
  buf = _osnp_parse_basic_header(buf, frame);
  buf[0] = sc;
  buf = _osnp_parse_security_header(buf, frame);
  frame->payload_len = 0;

  if (frame->src_pan) {
    memcpy(frame->src_pan, OSNP_PAN_ID, 2);
  } else if (frame->dst_pan && EXTRACT_FCPANCOMP(*frame->fc_low)) {
    memcpy(frame->dst_pan, OSNP_PAN_ID, 2);
  }

  if (frame->src_addr) {
    if (EXTRACT_FCSRCADDR(*frame->fc_high) == FCADDR_SHORT) {
      memcpy(frame->src_addr, OSNP_SHORT_ADDRESS, 2);
    } else {
      memcpy(frame->src_addr, OSNP_EUI, 8);
    }
  }
}

void osnp_initialize_response_frame(struct ieee802_15_4_frame *src_frame, struct ieee802_15_4_frame *dst_frame, unsigned char *dst_buf) {
  unsigned char fc_low = (*src_frame->fc_low & ~FCFRPEN) | FCREQACK;
  unsigned char fc_high = ((*src_frame->fc_high & 0xC0) >> 4) | ((*src_frame->fc_high & 0x0C) << 4) | (*src_frame->fc_high & 0x30);
  osnp_initialize_frame(fc_low, fc_high, *src_frame->sc, dst_buf, dst_frame);
  
  if (dst_frame->dst_pan) {
    if (src_frame->src_pan) {
      memcpy(dst_frame->dst_pan, src_frame->src_pan, 2);
    } else {
      memcpy(dst_frame->dst_pan, src_frame->dst_pan, 2);
    }
  }

  if (dst_frame->dst_addr && src_frame->src_addr) {
    memcpy(dst_frame->dst_addr, src_frame->src_addr, ((EXTRACT_FCDSTADDR(*dst_frame->fc_high) == FCADDR_SHORT) ? 2 : 8));
  }

  if (dst_frame->key_id) {
    memcpy(dst_frame->key_id, src_frame->key_id, (src_frame->payload - src_frame->key_id));
  }
}
