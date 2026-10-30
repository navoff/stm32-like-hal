#pragma once

/**
 * @file macros.h
 * @brief Preprocessor helpers: token concatenation, ARRAY_SIZE and unused-value suppression.
 */

#define CONCATx(A1, A2) A1##A2
#define CONCAT(A1, A2) CONCATx(A1, A2)

#define CONCATx_3(A1, A2, A3) A1##A2##A3
#define CONCAT_3(A1, A2, A3) CONCATx_3(A1, A2, A3)

#define HAL_UNUSED(x) (void)(x)

#if defined(__GNUC__) // GCC
  // These blackmagic spells was taken from the Linux kernel

  // https://github.com/torvalds/linux/blob/v6.6/include/linux/build_bug.h#L16
  // https://stackoverflow.com/questions/9229601/what-is-in-c
  #define BUILD_BUG_ON_ZERO(e) (sizeof(struct { int : -!!(e); }))

  // https://github.com/torvalds/linux/blob/v6.6/include/linux/compiler_types.h#L366
  #define __same_type(a, b) __builtin_types_compatible_p(typeof(a), typeof(b))

  // https://github.com/torvalds/linux/blob/v6.6/include/linux/compiler.h#L231
  #define __must_be_array(a) BUILD_BUG_ON_ZERO(__same_type((a), &(a)[0]))

  // https://github.com/torvalds/linux/blob/v6.6/include/linux/kernel.h#L57
  #define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]) + __must_be_array(arr))

#else
  #define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0])
#endif
