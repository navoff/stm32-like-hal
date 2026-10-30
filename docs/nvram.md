# NVRAM: settings storage in flash

`nvram/` stores a fixed-size settings structure in flash with wear leveling across several
erase segments, a CRC and a version number. It is used for device settings that change rarely
(calibration, user preferences, counters) and must survive power cycles.

Two backends are available, selected per project:

| Backend | Macro in config.h | CMake option | Driver |
|---|---|---|---|
| Internal MCU flash | `FEATURE_ENABLE_INT_NVRAM` | `HAL_NVRAM_INT_FLASH_ENABLE` | `drivers/int_flash.c` |
| External SPI NOR (M25Pxx / N25Qxx style) | `FEATURE_ENABLE_EXT_NOR_XX25_NVRAM` or `..._XX45_NVRAM` | `HAL_NVRAM_EXT_FLASH_ENABLE` | `drivers/ext_flash_xx25.c` / `ext_flash_xx45.c` |

## Segment layout

The unit of storage is a *segment* of `NVRAM_SIZE` bytes (`nvram_map_t` in `nvram/nvram_map.h`):

```
offset                         size
0                              NVRAM_SIZE - 10   application settings (nvram_settings_t, 0xFF padded)
NVRAM_SIZE - 10                2                 nvram_settings_version
NVRAM_SIZE - 8                 4                 segment_id (monotonic write counter)
NVRAM_SIZE - 4                 2                 segment_correct marker, 0xA5C3
NVRAM_SIZE - 2                 2                 CRC-16/X-25 of the preceding NVRAM_SIZE - 2 bytes
```

The flash area reserved for NVRAM holds `segment_amount` consecutive segments. Every write goes
to the *next* segment (round robin) with `segment_id` incremented, so each flash cell is erased
once per `segment_amount` writes.

Constraints on `NVRAM_SIZE`:

- internal flash: a multiple of 8 bytes, not larger than one erasable page, and the page size
  must be a multiple of `NVRAM_SIZE`; the area must start on a page boundary;
- external flash: a multiple of 256 bytes, not larger than the erasable unit of the chip;
- `sizeof(nvram_settings_t) <= NVRAM_SIZE - 10`.

## Startup

`nvram_init()` scans all segments and picks the one that has the correct marker, a valid CRC and
`nvram_settings_version` equal to the version requested by the application, with the highest
`segment_id`. The segment is copied into a RAM cache. If nothing valid is found the function
returns an error; the application is expected to call `nvram_write_default_struct()` to store
the defaults (see `nvram/nvram_settings_template.c`).

Changing `NVRAM_SETTINGS_VERSION` therefore invalidates all stored data and resets the device to
defaults. There is no migration between versions; add new fields at the end of the structure and
bump the version only when the layout of existing fields changes.

## Runtime

All reads and writes go through the RAM cache:

- `NVRAM_SETTINGS_READ_FIELD(field, &dst)` and `NVRAM_SETTINGS_WRITE_FIELD(field, &src)` copy a
  field of `nvram_settings_t` from or to the cache. A write that does not change the bytes does
  not mark the cache dirty.
- `nvram_process()` must be called from the main loop. When the cache is dirty it selects the
  next segment, erases it if the segment starts an erasable page, writes the cache and clears
  the dirty flag. With the internal backend this is a blocking operation (tens of milliseconds);
  with the external backend it is a state machine driven by repeated `nvram_process()` calls.
- `nvram_is_busy()` tells whether a write is pending or in progress. Check it before reset or
  power down.
- `nvram_lock(nvram, true)` makes the cache read-only: writes are silently ignored. It is used
  to freeze settings while a firmware update is in progress.

## Internal flash: linker script

The internal backend takes the NVRAM area from linker symbols. For GCC add a memory region and
a section to the `.ld` file:

```ld
MEMORY
{
  ROM   (rx) : ORIGIN = 0x08000000,          LENGTH = 64K - 4K
  NVRAM (r)  : ORIGIN = 0x08000000 + 64K - 4K, LENGTH = 4K
  RAM  (xrw) : ORIGIN = 0x20000000,          LENGTH = 8K
}

SECTIONS
{
  .nvram :
  {
    . = ALIGN(4);
    __nvram_begin = .;
    KEEP(*(.nvram))
    __nvram_end = .;
  } > NVRAM
}
```

`LENGTH / NVRAM_SIZE` becomes `segment_amount`. Complete scripts for several chips are in
`tests/system/`.
The region must start on an erasable page boundary and be a whole number of pages.

## Integration checklist

1. Copy `nvram/nvram_settings_template.h` and `.c` to the project as `nvram_settings.h` and
   `nvram_settings.c`, define `nvram_settings_t`, `nvram_settings_default` and
   `NVRAM_SETTINGS_VERSION`.
2. Set `NVRAM_SIZE`, `NVRAM_CHECKSUM_CRC_ALG` and the backend macros in `config.h`.
3. Enable the matching `HAL_NVRAM_*_ENABLE` CMake option.
4. Reserve the flash area in the linker script (internal backend) or initialize the external
   flash driver before `nvram_settings_init()`.
5. Call `nvram_settings_init()` once and `nvram_settings_process()` from the main loop.
