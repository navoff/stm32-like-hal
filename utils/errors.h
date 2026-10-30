#pragma once

/**
 * @file errors.h
 * @brief Common error codes returned by HAL functions.
 */

typedef enum
{
  ERR_OK = 0,
  ERR_BUSY = 1,
  ERR_RESULT = 2,
  ERR_PARAM = 3,
  ERR_BOF = 4,
  ERR_BUF = 5,
  ERR_TIMEOUT = 6,
  ERR_NOT_READY = 7
} error_t;
