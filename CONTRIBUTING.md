# Contributing

This is a personal firmware foundation that happens to be public. Issues and pull requests are
welcome, but there is no roadmap and no promise of support for families or peripherals the
author does not use.

## Code style

- C11, no dynamic memory, no RTOS assumptions, no C++.
- Formatting is enforced by `.clang-format` (LLVM base, Allman braces, 100 columns). Run
  `tests/fix_format.sh` before committing. Vendor code in `std_periph_lib/` and the ST USB stacks
  in `usb/stm32*` are not formatted.
- Naming: `module_verb_object()` for functions (`gpio_pin_set_mode`, `uart_x_tx_byte`),
  `module_name_t` for types, `MODULE_NAME` for macros. Enumerators use a double underscore to
  separate the type from the value: `GPIO_PIN_PULL__UP`, `UART_LINE_MODE__DMA`.
- Peripherals are addressed by index (`gpio_idx` 0 = GPIOA, `uart_number` 1 = USART1 on STM32),
  not by vendor pointers. Vendor types must not appear in public headers where a plain C type
  will do.
- Every header starts with `#pragma once`, a Doxygen `@file`/`@brief` block, then `<stdint.h>` /
  `<stdbool.h>` if needed, then project includes. Headers must compile on their own.
- Comments and messages are in English.

## Adding a driver or a family

- Family-specific code goes behind `#if defined(STM32xxxx)` blocks inside the driver, with an
  `#error This MCU not supported` fallback.
- Instances are gated by `FEATURE_ENABLE_<PERIPHERAL><N>` macros from the project `config.h`;
  see `templates/config_template.h`.
- Add the source to `CMakeLists.txt` and, if a family cannot build it yet, to the per-family
  `list(REMOVE_ITEM ...)` block so the support matrix in README.md stays truthful.
- Make sure `tests/build_all.sh` still passes for every preset.

## Commits

One commit per logical change. The subject line is imperative and under 72 characters.
