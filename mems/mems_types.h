#pragma once

/**
 * @file mems_types.h
 * @brief Common data types for MEMS accelerometer drivers.
 */

#include <stdint.h>

/// Number of sensor axes
#define MEMS_AXES_AMOUNT 3

#pragma pack(push, 1)
/// Acceleration values, mg
typedef union
{
  int16_t buf[MEMS_AXES_AMOUNT];
  struct
  {
    int16_t x;
    int16_t y;
    int16_t z;
  };
} mems_acc_t;

typedef struct
{
  uint8_t is_active : 1;
  uint8_t reserved : 7;

  union
  {
    struct
    {
      uint8_t x_min : 1;
      uint8_t y_min : 1;
      uint8_t z_min : 1;
      uint8_t x_max : 1;
      uint8_t y_max : 1;
      uint8_t z_max : 1;
    };
    struct
    {
      uint8_t min_msk : 3;
      uint8_t max_msk : 3;
    };
    uint8_t msk;
  } self_test_fail;
} mems_status_t;
#pragma pack(pop)

typedef void (*mems_rx_data_handler_t)(uint32_t timestamp, mems_acc_t *acc);
