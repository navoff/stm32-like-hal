# ARB: binary packet protocol and firmware image format

`protocols/arb_core.c` implements a small byte-oriented packet protocol used over UART (or any
byte stream) for device control and firmware update. `bootloader/make_arbin.py` produces the
firmware image consumed by an ARB bootloader.

## Framing

A packet on the wire:

```
0x7E | header (4 bytes) | payload (0..N bytes) | CRC-16 (2 bytes, little endian) | 0x7E
```

- `0x7E` is the frame delimiter. Two consecutive delimiters are an empty frame and are ignored.
- Byte stuffing: inside the frame every `0x7E` or `0x7D` byte is sent as `0x7D` followed by the
  byte XOR `0x20`. The receiver reverses this before the CRC check.
- CRC-16/MODBUS style (polynomial 0x8005 reflected, init 0xFFFF, no final XOR) is computed over
  header and payload before stuffing. The receiver computes the CRC over header, payload and the
  CRC bytes and expects zero.

Header (`arb_packet_header_t`, packed):

| Byte | Field | Meaning |
|---|---|---|
| 0 | `protocol_id` | `0xFD` for protocol version 1 |
| 1 | `address` | device address on the bus |
| 2 | `component_id` | which component of the device the packet is for (`0x01` is the bootloader) |
| 3 | `packet_id` | command or message type within that component |

The payload size is not transmitted; it is derived from the frame length. The maximum packet
size is limited by the receive buffer passed to `arb_init()`.

## API

`arb_t` is initialized with four callbacks: a function that pulls the next received byte from
the transport, a function that pushes a byte to the transport, a transport readiness check and
a packet callback. `arb_rx_process()` must be called from the main loop; it drains the transport,
reassembles frames and calls the packet callback for every frame with a valid CRC and protocol id.
`arb_tx_packet()` builds and sends one frame.

Common packet ids that every component should answer are in `protocols/arb_ids_common.h`:
"go to bootloader", "get brief device info", bootloader status, plus the bootloader commands
"start firmware load", "load segment", "apply firmware", "leave bootloader".

## Firmware image (`.arbin`)

Every firmware built with this HAL ends with a 128-byte `fw_info_t` structure
(`version/fw_info.h`) placed in a dedicated `.fw_info` linker section at the end of the
application flash area:

| Offset | Size | Field |
|---|---|---|
| 0 | 4 | magic number `0x2B3386EC` |
| 4 | 4 | structure version |
| 8 | 32 | platform type string |
| 40 | 4 | hardware type id |
| 44 | 64 | firmware version string |
| 108 | 16 | reserved |
| 124 | 4 | CRC-32 (see below) |

Two tools fill the CRC after the build:

- `version/add_crc.py image.bin` computes CRC-32 (Ethernet polynomial, reflected, init and
  final XOR 0xFFFFFFFF) over the whole `.bin` except the last 4 bytes and writes it there. The CRC
  of the complete file is then zero, which the application can verify at startup against
  `FW_ROM_BEGIN_ADDRESS`.
- `bootloader/make_arbin.py image.bin` builds the bootloader image: the 128-byte info
  structure first, then the firmware body. The script stamps `fw_crc` the same way as
  `add_crc.py` if the field is still `0xFFFFFFFF`, otherwise it verifies the checksum and
  refuses to write an image that does not match. A bootloader validates the image by
  restoring the original order (body, then structure) and checking that the CRC-32 of the
  result is zero.
