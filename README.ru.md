# stm32-like-hal

[English](README.md) | Русский

[![CI](https://github.com/navoff/stm32-like-hal/actions/workflows/ci.yml/badge.svg)](https://github.com/navoff/stm32-like-hal/actions/workflows/ci.yml)

Небольшой слой аппаратной абстракции на C11 для микроконтроллеров STM32 и совместимых с ними
(GD32). Написан для собственных устройств автора и опубликован как есть. Работает поверх ST
Standard Peripheral Library (F1, F3, F4) и драйверов ST LL (F0, L4), прячет различия между
семействами на уровне регистров за одним API и добавляет то, чего у ST нет: хранение настроек
во flash, UART с поддержкой RS-485, формат образа прошивки с CRC и протокол загрузчика.

API не заморожен, между тегами возможны ломающие изменения.

## Что это такое и чем не является

- Bare metal: без RTOS и без динамической памяти. Драйверы - статические синглтоны, периферия
  адресуется индексами (`gpio_idx` 0 = GPIOA, `uart_number` 1 = USART1), настройка через
  `config.h` проекта.
- Обработчики прерываний для включённых экземпляров периферии определены внутри драйверов.
- Блокирующие и неблокирующие варианты соседствуют: у UART есть режимы polling, прерывания и
  DMA; запись NVRAM во внутреннюю flash блокирующая, во внешнюю - через конечный автомат.
- Единственный поддерживаемый тулчейн - GCC (`arm-none-eabi-gcc`) и CMake. Зависимый от
  компилятора код ограничен файлами `utils/hal_config.h`, `utils/assert.h`, `version/fw_info.h`
  и символами компоновщика в `nvram/nvram_int_flash.c`.
- Это не замена ST HAL или Zephyr. Если нужны широкое покрытие периферии или сообщество,
  берите их.

## Матрица поддержки

| Драйвер | F0 | F1 | F3 | F4 | L4 | Примечания |
|---|:-:|:-:|:-:|:-:|:-:|---|
| gpio (режимы, EXTI) | ✅ | ✅ | ✅ | ✅ | ✅ | |
| uart (polling / IRQ / DMA, RS-485 DIR) | | ✅ | | ✅ | ✅ | |
| dma | ✅ | ✅ | ✅ | ✅ | ✅ | |
| spi | ✅ | ✅ | | ✅ | ✅ | F1: только SPI1/SPI2 |
| adc | | ✅ | ✅ | ✅ | ✅ | |
| can | | | | ✅ | | |
| tim | ✅ | ✅ | ✅ | ✅ | ✅ | помечен на рефакторинг |
| rtc | | ✅ | | | | |
| iwdg | ✅ | ✅ | ✅ | ✅ | ✅ | |
| int_flash | | ✅ | ✅ | ✅ | ✅ | |
| ext_flash (SPI NOR xx25 / xx45) | ✅ | ✅ | | ✅ | ✅ | нужен spi |
| tick_timer (SysTick) | ✅ | ✅ | ✅ | ✅ | ✅ | |
| mcu (сброс, таблица векторов) | ✅ | ✅ | ✅ | ✅ | ✅ | |
| nvram (внутренняя flash) | | ✅ | ✅ | ✅ | ✅ | |
| nvram (внешняя NOR) | ✅ | ✅ | | ✅ | ✅ | нужен spi |
| i2c | | | | | | исходники есть, не собираются |
| crc (аппаратный блок) | | | | | | исходники есть, не собираются |
| usb (CDC) | | | | | | три стека ST в репозитории, не собираются |

Ячейка с ✅ означает, что драйвер компилируется и линкуется в `tests/` для этого семейства.
Больше матрица ничего не гарантирует. В коде также есть ветки `#if defined(GD32E11X)` /
`GD32L23X` в gpio, uart, dma, int_flash, iwdg и tick_timer (GigaDevice GD32E11x и GD32L23x).
Они используются в проектах автора, но библиотека GigaDevice и тестовая цель в этот репозиторий
пока не входят, поэтому здесь эти семейства не собираются.

Платформонезависимые утилиты: `utils/fifo` (байтовый и универсальный кольцевые буферы),
`utils/checksum` (CRC-16/MODBUS, CRC-16/X-25, CRC-32), `utils/lp_filter`, `utils/assert`,
`protocols/arb_core` (пакетный протокол), `protocols/nmea_core` (парсер NMEA 0183).

## Где используется

- [nixie](https://github.com/navoff/nixie) - настольные часы на индикаторах ИН-14, STM32F100

## Структура

```
drivers/        драйверы периферии, по паре .c/.h на блок, в uart/ ядра по производителям
nvram/          хранение настроек, см. docs/nvram.md
protocols/      пакетный протокол ARB и парсер NMEA, см. docs/arb_protocol.md
version/        структура fw_info в конце образа и add_crc.py
bootloader/     make_arbin.py, формат образа для загрузчика ARB
utils/          fifo, контрольные суммы, фильтры, assert, макросы компилятора
usb/            обвязка USB CDC и стеки ST (не собираются)
mems/           типы данных акселерометра
std_periph_lib/ CMSIS и библиотеки периферии ST, см. std_periph_lib/README.md
system/         файл тулчейна arm-none-eabi-gcc.cmake
templates/      config_template.h, контракт между проектом и HAL
tests/          проверка сборки для каждого поддерживаемого семейства
```

## Требования

- `arm-none-eabi-gcc` 10 или новее
- CMake 3.21 или новее, Ninja
- clang-format 20 для `tests/fix_format.sh`; другие мажорные версии форматируют часть
  конструкций иначе

## Сборка проверочного проекта

```sh
git clone https://github.com/navoff/stm32-like-hal.git
cd stm32-like-hal/tests
cmake --preset stm32f1xx_hal_testing
cmake --build --preset stm32f1xx_hal_testing_debug
```

Пресеты есть для `stm32f0xx`, `stm32f1xx`, `stm32f30x`, `stm32f4xx` и `stm32l4xx`.
`tests/build_all.sh` конфигурирует и собирает все пресеты в Debug и Release. Каждый пресет
линкует минимальную прошивку (`tests/main.c`) для одного представителя семейства, используя
стартовый файл из библиотеки производителя и скрипт компоновщика из `tests/system/`. Архив HAL
линкуется целиком и без удаления неиспользуемых секций, поэтому каждый объект драйвера обязан
разрешить все свои символы, иначе сборка не пройдёт.

`tests/fix_format.sh --check` показывает файлы, которые расходятся с `.clang-format`, и ничего
не меняет. CI (`.github/workflows/ci.yml`) запускает оба скрипта на каждый push и pull request
с Arm GNU Toolchain 10.3-2021.07 и clang-format 20.1.8.

## Использование HAL в проекте

1. Подключите репозиторий как git submodule, например в `src/hal`.
2. Скопируйте `templates/config_template.h` в проект под именем `config.h`, задайте
   идентификатор платы, флаги `FEATURE_ENABLE_*` для используемых экземпляров периферии и
   параметры NVRAM. Каждый исходник HAL включает `config.h`, так что его каталог должен быть
   в путях поиска заголовков.
3. Если используете NVRAM, сделайте `nvram_settings.h` / `nvram_settings.c` из
   `nvram/nvram_settings_template.*`.
4. В своём `CMakeLists.txt`:

   ```cmake
   set(MCU_FAMILY STM32F1XX)
   set(HAL_INCLUDES ${CMAKE_SOURCE_DIR}/src/application)   # где лежит config.h
   set(HAL_DEFINITIONS STM32F1XX HW_TYPE=1 HSE_VALUE=8000000)
   set(STD_PERIPH_DEFINITIONS STM32F10X_LD_VL)              # определение чипа для CMSIS
   set(HAL_NVRAM_INT_FLASH_ENABLE ON)
   add_subdirectory(src/hal)
   target_link_libraries(firmware PRIVATE hal_lib)
   ```

   `MCU_FAMILY` выбирает библиотеку производителя и исходники, зависящие от семейства. Файл
   тулчейна `system/arm-none-eabi-gcc.cmake` берёт из той же переменной флаги процессора.

   `hal_lib` и `std_periph_lib` компилируются с `-ffunction-sections -fdata-sections`, поэтому
   при линковке прошивки с `-Wl,--gc-sections` в образ не попадают драйверы и функции
   производителя, которые она не вызывает. На `fw_info` никто не ссылается, поэтому нужен
   `-Wl,--undefined=fw_info`: одного `KEEP` в скрипте компоновщика мало, он не вытаскивает
   объект без ссылок из статической библиотеки.
5. Предоставьте стартовый файл, `SystemInit` и скрипт компоновщика. В скрипте должна быть
   секция `.fw_info`, а для NVRAM во внутренней flash - регион `.nvram` (см. docs/nvram.md).
6. Сначала вызовите `tick_timer_init()`, затем нужные драйверы; функции `*_process()` (uart,
   nvram, rtc) вызывайте из главного цикла.

## Стиль кода

`.clang-format` (база LLVM, скобки Allman, 100 колонок) применяется ко всему, кроме
`std_periph_lib/` и стеков USB от ST. Правила именования описаны в CONTRIBUTING.md.

## Лицензии

Сам HAL распространяется под лицензией MIT (см. LICENSE).

В `std_periph_lib/` лежит сторонний код со своими лицензиями, файлы лицензий хранятся рядом с
исходниками:

- CMSIS 5 и заголовки `cmsis-device-*`: Apache-2.0 / BSD-3-Clause (ARM, STMicroelectronics).
- Драйверы HAL/LL для STM32F0xx и STM32L4xx: BSD-3-Clause (STMicroelectronics).
- Standard Peripheral Library для STM32F0xx, STM32F10x, STM32F30x и STM32F4xx: лицензия ST
  "Ultimate Liberty" SLA0044. Распространение в исходниках разрешено, использование ограничено
  микроконтроллерами STMicroelectronics.

Стеки USB от ST в `usb/stm32*` подпадают под те же условия SLA0044 / BSD-3, что и библиотека
периферии соответствующего семейства.
