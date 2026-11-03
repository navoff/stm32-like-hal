# stm32-like-hal

English | [Русский](README.ru.md)

[![CI](https://github.com/navoff/stm32-like-hal/actions/workflows/ci.yml/badge.svg)](https://github.com/navoff/stm32-like-hal/actions/workflows/ci.yml)

A small C11 hardware abstraction layer for STM32 and STM32-compatible microcontrollers (GD32),
written for the author's own devices and published as is. It sits on top of the ST Standard
Peripheral Library (F1, F3, F4) and ST LL drivers (F0, L4), hides register-level differences
between families behind one API, and adds the pieces that ST does not ship: settings storage in
flash, RS-485 aware UART, a firmware image format with CRC and a bootloader protocol.

The API is not frozen; expect breaking changes between tags.

## What it is and is not

- Bare metal, no RTOS, no dynamic memory. Drivers are static singletons addressed by index
  (`gpio_idx` 0 = GPIOA, `uart_number` 1 = USART1), configured from a project-level `config.h`.
- Interrupt handlers for the enabled instances are defined inside the drivers.
- Blocking and non-blocking variants coexist: UART has polling, interrupt and DMA modes, NVRAM
  writes are blocking on internal flash and state-machine driven on external flash.
- GCC (`arm-none-eabi-gcc`) and CMake are the only supported toolchain. Compiler-specific code
  is limited to `utils/hal_config.h`, `utils/assert.h`, `version/fw_info.h` and the linker
  symbols used by `nvram/nvram_int_flash.c`.
- It is not a replacement for ST HAL or Zephyr. If you need broad peripheral coverage or a
  community, use those.

## Support matrix

| Driver | F0 | F1 | F3 | F4 | L4 | Notes |
|---|:-:|:-:|:-:|:-:|:-:|---|
| gpio (modes, EXTI) | ✅ | ✅ | ✅ | ✅ | ✅ | |
| uart (polling / IRQ / DMA, RS-485 DIR) | | ✅ | | ✅ | ✅ | |
| dma | ✅ | ✅ | ✅ | ✅ | ✅ | |
| spi | ✅ | ✅ | | ✅ | ✅ | F1: SPI1/SPI2 only |
| adc | | ✅ | ✅ | ✅ | ✅ | |
| can | | | | ✅ | | |
| tim | ✅ | ✅ | ✅ | ✅ | ✅ | marked for refactoring |
| rtc | | ✅ | | | | |
| iwdg | ✅ | ✅ | ✅ | ✅ | ✅ | |
| int_flash | | ✅ | ✅ | ✅ | ✅ | |
| ext_flash (SPI NOR xx25 / xx45) | ✅ | ✅ | | ✅ | ✅ | needs spi |
| tick_timer (SysTick) | ✅ | ✅ | ✅ | ✅ | ✅ | |
| mcu (reset, vector table) | ✅ | ✅ | ✅ | ✅ | ✅ | |
| nvram (internal flash) | | ✅ | ✅ | ✅ | ✅ | |
| nvram (external NOR) | ✅ | ✅ | | ✅ | ✅ | needs spi |
| i2c | | | | | | source present, not built |
| crc (hardware unit) | | | | | | source present, not built |
| usb (CDC) | | | | | | three ST stacks vendored, not built |

Cells marked ✅ compile and link under `tests/` for that family; that is the only guarantee
the matrix gives. The code also contains `#if defined(GD32E11X)` / `GD32L23X` branches in gpio,
uart, dma, int_flash, iwdg and tick_timer (GigaDevice GD32E11x and GD32L23x). They are in use in
the author's projects, but the GigaDevice firmware library and a test target are not part of this
repository yet, so these families are not built here.

Platform-independent utilities: `utils/fifo` (byte and generic ring buffers),
`utils/checksum` (CRC-16/MODBUS, CRC-16/X-25, CRC-32), `utils/lp_filter`, `utils/assert`,
`protocols/arb_core` (packet protocol), `protocols/nmea_core` (NMEA 0183 parser).

## Layout

```
drivers/        peripheral drivers, one .c/.h per peripheral, uart/ has per-vendor cores
nvram/          settings storage, see docs/nvram.md
protocols/      ARB packet protocol and NMEA parser, see docs/arb_protocol.md
version/        fw_info structure at the end of the image and add_crc.py
bootloader/     make_arbin.py, image format for an ARB bootloader
utils/          fifo, checksum, filters, assert, compiler macros
usb/            USB CDC glue and vendored ST USB stacks (not built)
mems/           accelerometer data types
std_periph_lib/ CMSIS and ST peripheral libraries, see std_periph_lib/README.md
system/         arm-none-eabi-gcc.cmake toolchain file
templates/      config_template.h, the contract between a project and the HAL
tests/          compile check for every supported family
```

## Requirements

- `arm-none-eabi-gcc` 10 or newer
- CMake 3.21 or newer, Ninja
- clang-format 20 for `tests/fix_format.sh`; other major versions format some constructs
  differently

## Building the compile check

```sh
git clone https://github.com/navoff/stm32-like-hal.git
cd stm32-like-hal/tests
cmake --preset stm32f1xx_hal_testing
cmake --build --preset stm32f1xx_hal_testing_debug
```

Presets exist for `stm32f0xx`, `stm32f1xx`, `stm32f30x`, `stm32f4xx` and `stm32l4xx`.
`tests/build_all.sh` configures and builds every preset in Debug and Release. Each preset
links a minimal firmware (`tests/main.c`) for one representative chip, using the vendor startup
file and a linker script from `tests/system/`. The whole HAL archive is linked without section
garbage collection, so every driver object must resolve all its symbols for the build to pass.

`tests/fix_format.sh --check` reports files that differ from `.clang-format` without changing
them. CI (`.github/workflows/ci.yml`) runs both scripts on every push and pull request, with
Arm GNU Toolchain 10.3-2021.07 and clang-format 20.1.8.

## Using the HAL in a project

1. Add the repository as a git submodule, for example under `src/hal`.
2. Copy `templates/config_template.h` to your project as `config.h`, set the board id, the
   `FEATURE_ENABLE_*` flags for the peripheral instances you use, and the NVRAM options.
   Every HAL source includes `config.h`, so its directory must be on the include path.
3. Provide `nvram_settings.h` / `nvram_settings.c` from `nvram/nvram_settings_template.*` if you
   use NVRAM.
4. In your `CMakeLists.txt`:

   ```cmake
   set(MCU_FAMILY STM32F1XX)
   set(HAL_INCLUDES ${CMAKE_SOURCE_DIR}/src/application)   # where config.h lives
   set(HAL_DEFINITIONS STM32F1XX HW_TYPE=1 HSE_VALUE=8000000)
   set(STD_PERIPH_DEFINITIONS STM32F10X_LD_VL)              # device define for CMSIS
   set(HAL_NVRAM_INT_FLASH_ENABLE ON)
   add_subdirectory(src/hal)
   target_link_libraries(firmware PRIVATE hal_lib)
   ```

   `MCU_FAMILY` selects the vendor library and the family-specific sources. The toolchain file
   `system/arm-none-eabi-gcc.cmake` sets CPU flags from the same variable.

   `hal_lib` and `std_periph_lib` are compiled with `-ffunction-sections -fdata-sections`, so
   linking the firmware with `-Wl,--gc-sections` drops the drivers and vendor functions it does
   not call. Nothing references `fw_info`, so pass `-Wl,--undefined=fw_info`: `KEEP` in the
   linker script alone does not pull an unreferenced object out of a static library.
5. Provide the startup file, `SystemInit` and the linker script. The linker script must define
   the `.fw_info` section and, for internal-flash NVRAM, the `.nvram` region (see docs/nvram.md).
6. Call `tick_timer_init()` first, then the drivers you need; call the `*_process()` functions
   (uart, nvram, rtc) from the main loop.

## Code style

`.clang-format` (LLVM base, Allman braces, 100 columns) is applied to everything except
`std_periph_lib/` and the vendored USB stacks. See CONTRIBUTING.md for naming rules.

## Licenses

The HAL itself is released under the MIT license (see LICENSE).

`std_periph_lib/` contains third-party code with its own licenses, kept next to the sources:

- CMSIS 5 and the `cmsis-device-*` headers: Apache-2.0 / BSD-3-Clause (ARM, STMicroelectronics).
- STM32F0xx and STM32L4xx HAL/LL drivers: BSD-3-Clause (STMicroelectronics).
- STM32F0xx, STM32F10x, STM32F30x and STM32F4xx Standard Peripheral Libraries: ST "Ultimate
  Liberty" license SLA0044. Redistribution in source form is permitted, use is restricted to
  STMicroelectronics microcontrollers.

The ST USB stacks under `usb/stm32*` are covered by the same SLA0044 / BSD-3 terms as the
peripheral library of the matching family.
