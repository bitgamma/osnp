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
#include "tlv.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define SCANNING_CHANNELS 0
#define WAITING_ASSOCIATION_REQUEST 1
#define ASSOCIATED 2
#define WAITING_PENDING_DATA 3

static uint8_t OSNP_PAN_ID[2];
static uint8_t OSNP_EUI[8];

static uint8_t tx_frame_buf[128];

static uint8_t seq_no;
static uint8_t state;
static uint8_t channel;

static uint32_t rx_frame_counter;
static uint32_t tx_frame_counter;

static uint32_t rx_saved_frame_counter;
static uint32_t tx_saved_frame_counter;

void osnp_initialize(void) {
  osnp_load_eui(OSNP_EUI);
  osnp_load_pan_id(OSNP_PAN_ID);
  osnp_load_channel(&channel);

  seq_no = 0;

  if (channel == 0xff) {
    channel = 0;
    state = SCANNING_CHANNELS;
    osnp_load_master_key(tx_frame_buf);
    osnp_start_channel_scanning_timer();
  } else {
    state = ASSOCIATED;
    osnp_load_rx_frame_counter((uint8_t *) &rx_frame_counter);
    osnp_load_tx_frame_counter((uint8_t *) &tx_frame_counter);

    rx_saved_frame_counter = rx_frame_counter;
    tx_saved_frame_counter = tx_frame_counter;

    osnp_load_rx_key(tx_frame_buf);
    osnp_load_tx_key(tx_frame_buf);
    osnp_start_poll_timer();
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
      break;
    case WAITING_PENDING_DATA:
      state = ASSOCIATED;
      osnp_start_poll_timer();
      break;
  }
}

void _osnp_handle_discovery_request(ieee802_15_4_frame_t *frame) {
  ieee802_15_4_frame_t tx_frame;
  osnp_initialize_response_frame(frame, &tx_frame, tx_frame_buf);

  tx_frame.payload[0] = OSNP_MCMD_DISCOVER;
  tx_frame.payload_len = 1;

  osnp_transmit_frame(&tx_frame);
  osnp_stop_active_timer();
}

void _osnp_reset_security(ieee802_15_4_frame_t *frame) {
  osnp_write_rx_key(&frame->payload[1]);
  osnp_write_tx_key(&frame->payload[17]);

  rx_frame_counter = 0x00;
  rx_saved_frame_counter = OSNP_FRAME_COUNTER_WINDOW;

  tx_frame_counter = 0x00;
  tx_saved_frame_counter = OSNP_FRAME_COUNTER_WINDOW;

  osnp_write_rx_frame_counter((uint8_t *) & rx_saved_frame_counter);
  osnp_write_tx_frame_counter((uint8_t *) &tx_saved_frame_counter);
}

void _osnp_handle_key_update(ieee802_15_4_frame_t *frame) {
   _osnp_reset_security(frame);

  ieee802_15_4_frame_t tx_frame;
  osnp_initialize_response_frame(frame, &tx_frame, tx_frame_buf);
  tx_frame.payload[0] = OSNP_MCMD_KEY_UPDATE_RES;
  tx_frame.payload_len = 1;

  osnp_transmit_frame(&tx_frame);
}

void _osnp_send_frame_counter(ieee802_15_4_frame_t *frame) {
  uint32_t expected_counter = rx_frame_counter + 1;

  ieee802_15_4_frame_t tx_frame;
  osnp_initialize_response_frame(frame, &tx_frame, tx_frame_buf);
  tx_frame.payload[0] = OSNP_MCMD_FRAME_COUNTER_ALIGN;

#ifdef LITTLE_ENDIAN
    memcpy(&(tx_frame.payload[1]), (uint8_t *) &expected_counter, 4);
#else
    tx_frame.payload[4] = (expected_counter & 0xff);
    tx_frame.payload[3] = ((expected_counter >> 8) & 0xff);
    tx_frame.payload[2] = ((expected_counter >> 16) & 0xff);
    tx_frame.payload[1] = ((expected_counter >> 24) & 0xff);
#endif

  tx_frame.payload_len = 5;

  osnp_transmit_frame(&tx_frame);
}

void _osnp_handle_association_request(ieee802_15_4_frame_t *frame) {
  memcpy(OSNP_PAN_ID, frame->src_pan, 2);
  osnp_write_pan_id(OSNP_PAN_ID);
  osnp_write_channel(&channel);

  _osnp_reset_security(frame);

  osnp_stop_active_timer();

  state = ASSOCIATED;

  ieee802_15_4_frame_t tx_frame;
  
  uint8_t fc_low = FCFRTYP(FCFRTYP_MCMD) | FCREQACK | FCSECEN;
  uint8_t fc_high = FCDSTADDR(FCADDR_NONE) | FCSRCADDR(FCADDR_EXT);

  osnp_initialize_frame(fc_low, fc_high, tx_frame_buf, &tx_frame);
  tx_frame.payload[0] = OSNP_MCMD_ASSOCIATION_RES;
  tx_frame.payload[1] = OSNP_DEVICE_CAPABILITES;
  tx_frame.payload[2] = OSNP_SECURITY_LEVEL;

  tx_frame.payload_len = 3;

  osnp_transmit_frame(&tx_frame);
}

_osnp_handle_disassociation_notification() {
  OSNP_PAN_ID[0] = 0x00;
  OSNP_PAN_ID[1] = 0x00;

  osnp_write_pan_id(OSNP_PAN_ID);
  osnp_load_master_key(tx_frame_buf);

  channel = 0xff;
  osnp_write_channel(&channel);
  channel = 0;

  state = SCANNING_CHANNELS;
  osnp_stop_active_timer();
  osnp_start_channel_scanning_timer();
}

void _osnp_mac_command_frame_received_cb(ieee802_15_4_frame_t *frame) {
  if (state < ASSOCIATED) {
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
        _osnp_handle_disassociation_notification();
        break;
      case OSNP_MCMD_KEY_UPDATE_REQ:
        _osnp_handle_key_update(frame);
        break;
    }
  }
}

void _osnp_data_frame_received_cb(ieee802_15_4_frame_t *frame) {
  uint16_t i = 0;
  uint16_t tag;
  uint16_t end;
  
  i += tlv_read_tag(&frame->payload[i], &tag);
  
  if (tag != 0xE0) {
    return;
  }
  
  i += tlv_read_length(&frame->payload[i], &end);
  end += i;

  ieee802_15_4_frame_t tx_frame;
  osnp_initialize_response_frame(frame, &tx_frame, tx_frame_buf);

  uint16_t j = 0;
  j += tlv_write_tag(&tx_frame.payload[j], 0xE1);
  j += tlv_write_undefined_length(&tx_frame.payload[j]);

  while(i < end) {
    osnp_process_command(frame, &i, &tx_frame, &j, (state >= ASSOCIATED));
  }

  j += tlv_write_undefined_length_terminator(&tx_frame.payload[j]);
  tx_frame.payload_len = j;

  osnp_transmit_frame(&tx_frame);
}

void osnp_frame_received_cb(uint8_t *frame_buf, int16_t frame_len) {
  ieee802_15_4_frame_t frame;
  osnp_parse_frame(frame_buf, frame_len, &frame);

  if (state == SCANNING_CHANNELS) {
    state = WAITING_ASSOCIATION_REQUEST;
  } else if (state == ASSOCIATED && EXTRACT_FCFRPEN(*frame.fc_low)) {
    state = WAITING_PENDING_DATA;
  }

  if (state >= ASSOCIATED) {
    if (!EXTRACT_FCSECEN(*frame.fc_low)) {
      osnp_start_poll_timer();
      return;
    }

    uint32_t current_frame_counter;
#ifdef LITTLE_ENDIAN
    current_frame_counter = *((uint32_t *) frame.frame_counter);
#else
    current_frame_counter = frame.frame_counter[3] << 24 | frame.frame_counter[2] << 16 | frame.frame_counter[1] << 8 frame.frame_counter[0];
#endif

    if (current_frame_counter <= rx_frame_counter) {
      _osnp_send_frame_counter(&frame);
      return;
    } else {
      rx_frame_counter = current_frame_counter;
      if (rx_frame_counter >= rx_saved_frame_counter) {
        rx_saved_frame_counter += OSNP_FRAME_COUNTER_WINDOW;
        osnp_write_rx_frame_counter((uint8_t *) & rx_saved_frame_counter);
      }
    }
  }
  
  switch (EXTRACT_FCFRTYP(*frame.fc_low)) {
    case FCFRTYP_DATA:
      _osnp_data_frame_received_cb(&frame);
      break;
    case FCFRTYP_MCMD:
      _osnp_mac_command_frame_received_cb(&frame);
      break;
  }
}

void osnp_frame_sent_cb(uint8_t status) {
  //todo: add error handling

  switch(state) {
    case SCANNING_CHANNELS:
      osnp_start_channel_scanning_timer();
      break;
    case WAITING_ASSOCIATION_REQUEST:
      osnp_start_association_wait_timer();
      break;
    case ASSOCIATED:
    case WAITING_PENDING_DATA:
      if ((status == OSNP_TX_STATUS_OK) && osnp_get_pending_frames()) {
        state = WAITING_PENDING_DATA;
        osnp_start_pending_data_wait_timer();
      } else {
        state = ASSOCIATED;
        osnp_start_poll_timer();
      }
      break;
  }
}

void osnp_poll() {
  ieee802_15_4_frame_t tx_frame;
  uint8_t fc_low = FCFRTYP(FCFRTYP_MCMD) | FCREQACK;
  uint8_t fc_high = FCDSTADDR(FCADDR_NONE) | FCSRCADDR(FCADDR_EXT);

  osnp_initialize_frame(fc_low, fc_high, tx_frame_buf, &tx_frame);
  tx_frame.payload[0] = OSNP_MCMD_DATA_REQ;
  tx_frame.payload_len = 1;
  
  osnp_transmit_frame(&tx_frame);
}

uint8_t *_osnp_parse_header(uint8_t *buf, ieee802_15_4_frame_t *frame) {
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

  frame->header_len = buf - frame->backing_buffer;

  if (EXTRACT_FCSECEN(*frame->fc_low)) {
    frame->frame_counter = buf;
    buf += 4;
    frame->key_counter = buf++;
    frame->sec_header_len = 5;
  } else {
    frame->frame_counter = NULL;
    frame->key_counter = NULL;
    frame->sec_header_len = 0;
  }

  frame->payload = buf;

  return buf;
}

void osnp_parse_frame(uint8_t *buf, uint16_t frame_len, ieee802_15_4_frame_t *frame) {
  buf = _osnp_parse_header(buf, frame);
  
  // Remove mic and fcs, which is calculated/verified at a lower layer
  frame->payload_len = frame_len - frame->header_len - 2;

  if (frame->sec_header_len) {
    frame->payload_len -= OSNP_MIC_LENGTH - frame->sec_header_len;
  }
}

void osnp_initialize_frame(uint8_t fc_low, uint8_t fc_high, uint8_t *buf, ieee802_15_4_frame_t *frame) {
  buf[0] = fc_low;
  buf[1] = fc_high;
  buf[2] = seq_no++;
  buf = _osnp_parse_header(buf, frame);
  frame->payload_len = 0;

  if (frame->src_pan) {
    memcpy(frame->src_pan, OSNP_PAN_ID, 2);
  }

  if (frame->src_addr) {
    memcpy(frame->src_addr, OSNP_EUI, 8);
  }

  if (EXTRACT_FCSECEN(*frame->fc_low)) {
#ifdef LITTLE_ENDIAN
    memcpy(frame->frame_counter, (uint8_t *) &tx_frame_counter, 4);
#else
    frame->frame_counter[3] = (tx_frame_counter & 0xff);
    frame->frame_counter[2] = ((tx_frame_counter >> 8) & 0xff);
    frame->frame_counter[1] = ((tx_frame_counter >> 16) & 0xff);
    frame->frame_counter[0] = ((tx_frame_counter >> 24) & 0xff);
#endif
    tx_frame_counter++;

    if (tx_frame_counter >= tx_saved_frame_counter) {
      tx_saved_frame_counter += OSNP_FRAME_COUNTER_WINDOW;
      osnp_write_tx_frame_counter((uint8_t *) &tx_saved_frame_counter);
    }

    *frame->key_counter = 0x01;
  }
}

void osnp_initialize_response_frame(ieee802_15_4_frame_t *src_frame, ieee802_15_4_frame_t *dst_frame, uint8_t *dst_buf) {
  uint8_t fc_low = (*src_frame->fc_low & ~FCFRPEN);
  uint8_t fc_high = ((*src_frame->fc_high & 0xC0) >> 4) | (*src_frame->fc_high & 0x30) | FCSRCADDR(FCADDR_EXT);

  if (state >= ASSOCIATED) {
    fc_low |= FCSECEN;
  }

  osnp_initialize_frame(fc_low, fc_high, dst_buf, dst_frame);
  
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
}
