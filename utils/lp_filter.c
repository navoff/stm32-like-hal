/**
 * @file lp_filter.c
 * @brief First-order IIR low-pass filter implementation (integer and float).
 */

#include "config.h"

#include "lp_filter.h"

void lp_filter_i16_init(lp_filter_i16_t *filter, uint16_t b, uint8_t k)
{
  filter->b = b;
  filter->k = k;
}

void lp_filter_i16_reset(lp_filter_i16_t *filter, int16_t reset_value)
{
  filter->out = reset_value;
}

void lp_filter_i16_process(lp_filter_i16_t *filter, int16_t value)
{
  int32_t delta = value - filter->out;
  filter->out += (delta * filter->b + (1 << (filter->k - 1))) >> filter->k;
}

void lp_filter_float_init(lp_filter_float_t *filter, uint16_t b, uint8_t k)
{
  filter->b = b;
  filter->k = k;
  filter->exp_k = 1 << filter->k;
}

void lp_filter_float_reset(lp_filter_float_t *filter, float reset_value)
{
  filter->out = reset_value;
}

void lp_filter_float_process(lp_filter_float_t *filter, float value)
{
  float delta = value - filter->out;
  filter->out += delta * filter->b / filter->exp_k;
}
