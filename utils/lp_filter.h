#pragma once

/**
 * @file lp_filter.h
 * @brief First-order IIR low-pass filter (integer and float versions).
 *
 * In short
 *    Yn = A * Yn-1 + B * Xn
 *      Yn - filter output in the current step
 *      Yn-1 - filter output in the previous step
 *      Xn - filter input in the current step
 *      A + B = 1 - define the filter frequency response (time constant)
 *    After rearranging
 *    Yn = Yn-1 + (Xn - Yn-1) * B
 *    B = b / (2 ^ k) - to keep integer arithmetic simple
 *
 * How to choose b and k
 *   Given:
 *     T - filter update period
 *     Ts - settling time
 *     N = Ts / T - number of filter steps during the settling time
 *   Choose b and k so that:
 *     1 - exp(-3 / N) <= b / (2 ^ k) <= 1 - exp(-7 / N)
 *              |                                 |
 *              95% of amplitude in N steps       99.9% of amplitude in N steps
 */

#include <stdint.h>

#pragma pack(push, 1)
typedef struct
{
  uint16_t b;
  uint8_t k;
  int16_t out;
} lp_filter_i16_t;

typedef struct
{
  uint16_t b;
  uint8_t k;
  float exp_k;
  float out;
} lp_filter_float_t;
#pragma pack(pop)

void lp_filter_i16_init(lp_filter_i16_t *filter, uint16_t b, uint8_t k);
void lp_filter_i16_reset(lp_filter_i16_t *filter, int16_t reset_value);
void lp_filter_i16_process(lp_filter_i16_t *filter, int16_t value);

void lp_filter_float_init(lp_filter_float_t *filter, uint16_t b, uint8_t k);
void lp_filter_float_reset(lp_filter_float_t *filter, float reset_value);
void lp_filter_float_process(lp_filter_float_t *filter, float value);
