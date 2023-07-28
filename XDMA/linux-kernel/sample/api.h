#pragma once

#include <stdbool.h>
#include <stdint.h>

#define TSN_QUEUE_COUNT 8
#define MAX_TAS_SCHEDULES 20

typedef uint64_t timestamp_t;  // nanoseconds. TODO: Import from gptp.h

struct api_timespec {
  uint32_t tv_sec;
  uint32_t tv_nsec;
};

struct api_timex {
  uint8_t mode;
  union {
    struct api_timespec offset;
    int16_t freq;
  };
};

struct api_tas_entry {
  uint8_t gates_bit;  // 0b00000001 = gate 0 open, 0b00000010 = gate 1 open, etc.
  uint32_t duration_ns;
};

struct api_tas_schedules {
  /* RW */ uint32_t queue_max_sdu_table[TSN_QUEUE_COUNT];
  /* RW */ bool gate_enabled;  // enable TAS
  /* RW */ uint8_t admin_gate_states;  // Note: Initial values for gates. Maybe used if current_time < base_time
  /* R  */ uint8_t oper_gate_states;
  /* RW */ uint32_t admin_control_list_length;
  /* R  */ uint32_t oper_control_list_length;
  /* RW */ struct api_tas_entry admin_control_list[MAX_TAS_SCHEDULES];
  /* R  */ struct api_tas_entry oper_control_list[MAX_TAS_SCHEDULES];
  /* RW */ uint32_t admin_cycle_time;  // Seconds. TODO: should be a rational number
  /* R  */ uint32_t oper_cycle_time;  // Seconds. TODO: should be a rational number
  /* RW */ uint32_t admin_cycle_time_extention;  // Nanoseconds
  /* R  */ uint32_t oper_cycle_time_extention;  // Nanoseconds
  /* RW */ struct api_timespec admin_base_time;
  /* R  */ struct api_timespec oper_base_time;
  /* RW */ bool config_change;
  /* R  */ struct api_timespec config_change_time;
  /* R  */ uint32_t tick_granularity;  // tenth of nanoseconds; 1 tick = 100ns
  /* R  */ struct api_timespec current_time;
  /* R  */ uint32_t config_change_error;
  /* R  */ uint32_t supported_list_max;  // XXX: Should return MAX_TAS_SCHEDULES

  // Additional precomputed data goes here
};

struct api_cbs_entry {
  bool is_enabled;
  int16_t idleslope;
  int16_t sendslope;
  int16_t hicredit;
  int16_t locredit;

  // Note that fraction part should be enough if >= 32bit. But 48(16+32) is weird. So we use 64bit integer.
  // Additional data
  timestamp_t last_update_time;
  int64_t current_credit; // 16 high bits are integer, 48 low bits are fraction
};

struct api_cbs_configs {
  struct api_cbs_entry configs[TSN_QUEUE_COUNT];
};

// TODO: Are these needed?
// Or just use tx/rx_metadata from plio.h

struct api_tx_metadata {
  // uint8_t hw_port;
  int8_t hw_queue;  // -1 = unset
  uint16_t vlan;  // 0 = unset
  struct api_timespec tx_time;  // TAS half mode
  bool enable_tx_timestamp;
};

struct api_rx_metadata {
  // uint8_t hw_port;
  bool is_vlan_offloaded;
  bool has_rx_timestamp;
  uint16_t vlan;
  struct api_timespec rx_timestamp;
};

// What needed for TX
// - Packet obvioulsy
// - hw port (Not yet)
// - hw queue (Default first queue)
// - vlan tag (Optional, For Offloading)
// - tx time (For TAS half mode, Not TX Timestamp)
// Returns: packet id if success, -1 if failed
int32_t api_enqueue(const uint8_t* pkt, const uint16_t pkt_len, const struct api_tx_metadata* metadata);

// Returns: 0 if success, negative if fail
int32_t api_get_tx_timestamp(uint32_t pkt_id, struct api_timespec* tx_timestamp);


// What needed for RX
// - Packet buffer obvioulsy
// - hw port (Not yet)
// - vlan filter (Optional, For Offloading)
// - rx timestamp
// Returns: packet length if success, -1 if failed
int32_t api_dequeue(uint8_t* const buf, const uint16_t buf_len, struct api_rx_metadata* metadata);


int32_t api_get_hw_clock(struct api_timespec* hw_clock);
int32_t api_adj_hw_clock(const struct api_timex* argument);


// Returns: 0 if success, negative if fail
int32_t api_get_tas_entry_count();

// Returns: count of entries if success, -1 if failed
/*
 * Set api_tas_schedules struct as follows before pass to this function
   struct api_tas_schedules {
     uint32_t base_time_ns = 0 (ignored);
     uint8_t entry_count = Your tas_entry count;
     struct tas_entry* entries = Your tas_entry array pointer;
     uint32_t total_duration_ns = 0 (ignored);
   };
Example:
   int32_t entry_count = api_get_tas_entry_count();
   struct tas_entry* entries = calloc(entry_count, sizeof(struct tas_entry));
   struct api_tas_schedules schedules = {
     .base_time_ns = 0,
     .entry_count = entry_count,
     .entries = entries,
     .total_duration_ns = 0,
   };
   api_get_tas_schedules(&schedules);
*/
int32_t api_get_tas_schedules(struct api_tas_schedules* schedules);

int32_t api_set_tas_schedules(const struct api_tas_schedules* schedules);

// Get all configs
int32_t api_get_cbs_configs(struct api_cbs_configs* configs);
// Get specific queue's config
int32_t api_get_cbs_config(int8_t queue, struct api_cbs_entry* entry);

int32_t api_set_cbs_config(int8_t queue, const struct api_cbs_entry* entry);
