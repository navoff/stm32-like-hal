/**
 * @file assert.c
 * @brief Assertion failure handler.
 */

#include "config.h"

#include "utils/assert.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void hal_assert_handler(char const *expr, char const *file, uint32_t line)
{
  HALT_IF_DEBUGGING();
  while (1)
    ;
}
#pragma GCC diagnostic pop
