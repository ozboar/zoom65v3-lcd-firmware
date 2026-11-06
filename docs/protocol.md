# Keyboard Link Protocol

UART at 1.5 Mbaud 8N1, with the same wiring as the stock screen: keyboard UART3 to screen USART1. The keyboard switches to this rate when it starts the firmware, and stock firmware stays at 115200.

## UI frames

Frames are `A5 03 <command> <payload>`. They are not acknowledged, so sending never waits on the screen.

| Command | Payload | Meaning |
|---|---|---|
| `01` | None | Tap with no key known, drops the paws in turn |
| `02` | `wpm` (u16, big endian) | Words per minute |
| `03` | `device`, `layer`, `caps`, `battery` | Status values, see below |
| `04` | `row`, `state` (u16, big endian) | Held keys on a matrix row, bit N is column N |
| `05` | `mode` | 0 clock, 1 image and GIF, 2 bongo cat, 3 WPM graph, 4 system stats, 5 weather, 6 RGB status, 7 keyboard info |
| `06` | `on` | Sleep or wake the panel |
| `07` | `on` | Backlight on or off |
| `08` | `iso` | Keyboard layout, used to find a key's side: 0 ANSI, 1 ISO |
| `09` | `row`, `col` | Key tap, drops the paw on that key's side |
| `0A` | `accent` | Accent color, 0-6, used by the cat, clock, WPM graph, system stats and keyboard info pages and the page menu |
| `0B` | `units` | Fahrenheit display: bit 0 system page, bit 1 weather page |
| `0C` | `flags` | Status page: bit 0 charging, bit 1 full, bit 2 NKRO |
| `0D` | `on`, `mode`, `hue`, `sat`, `val`, `speed` | RGB page settings |
| `0E` | 20 bytes | RGB effect name, zero padded |
| `0F` | `face` | Clock face: 0 detailed, 1 simple, 2 digital, 3 binary. Changing it while its page is open shows the selection as dots |
| `10` | `date[10]`, `hash[10]` | Keyboard firmware build date and git hash, each zero padded |
| `11` | `qmk[18]`, `via_hi`, `via_lo` | QMK version, zero padded, and VIA protocol version, 0 without VIA |
| `12` | `setting` | Image and GIF page: 0 image, 1 GIF, 2 GIF that only plays while typing. Changing it while its page is open shows the selection as dots |
| `13` | `percent` | Backlight brightness, 0-100. Stays off while `07` has it off |
| `14` | None | Wake the panel and reply `5A`, see below |

`device` is 0 for USB, 1-5 for BT1-BT5 and 6 for 2.4G.

The keyboard sends all state once a second as well as on changes. That restores a screen which restarted, and covers any dropped frame.

## Ready signal

The screen sends `5A` once its UART is up, without being asked, and the first stage loader sends `5B`. The keyboard waits for these instead of a fixed delay.

The screen stays black until it receives a mode, or for 500 ms if none arrives.

## Deep sleep

Once `06` has turned the panel off, and the link has been quiet for 100 ms with no upload running, the screen MCU deep sleeps. A falling edge on its RX pin wakes it, but it loses what arrives while its clocks restart, and goes back to sleep if nothing else follows.

So the keyboard sends nothing else while the panel is off. To wake it, the keyboard repeats `14` every 20 ms until it gets `5A`, then sends all state. If no `5A` comes, it reloads the firmware.

`board_millis()` counts on through sleep using the RTC. It keeps a low speed source already set up by stock firmware, and otherwise starts LICK. Elapsed time is scaled by the rate measured against the main clock while awake.

## Other frames

| Frame | Meaning |
|---|---|
| `A5 00 FF` | Reset the screen, which returns to stock while running from SRAM. No reply |
| `A5 <other>` | Not supported. Replies with a single `00` when the line goes quiet for 10 ms, so a stock style sender gets a rejection instead of waiting forever |
| `C5 50` + 6 bytes | Ping, replies `STUB`, program id `03`, RTC source (0 none, 1 LEXT, 2 LICK), and the number of deep sleeps (u16, big endian). The loader uses this |

## Stock commands

These use the stock format so `zoom-sync` keeps working. It treats a rejection as a failure, so anything it sends in normal use is accepted, and each gets a single `01` reply.

| Frame | Effect here |
|---|---|
| `A5 01 40 cpu gpu dl_hi dl_lo` | Shown on the system page. `dl` is hundredths, big endian |
| `A5 01 10 YY MM DD hh mm ss` | Sets the clock, which counts on between updates |
| `A5 01 20 icon cur low high` | Shown on the weather page, using the stock icons on the external flash |
| `A5 01 70 n`, `A5 01 FF [t]` | Accepted, ignored |
| `A5 00 20`, `A5 00 21`, `A5 00 22` | Accepted, ignored. The keyboard chooses the mode |

Temperatures are expected in Celsius, so set `fahrenheit = false` in the `zoom-sync` config. The pages convert them for display.

## Uploads

The stock group `02` sequence works, with each step and data packet answered by `01`, or `00` to reject it:

| Frame | Effect |
|---|---|
| `A5 02 F0 ch` | Start an upload, `ch` 1 for image, 2 for GIF, and show the progress screen |
| `A5 02 D0 len` | Big endian length. Invalidates the old upload. An image must be 36300 bytes, 110x110 |
| `[index u16] [data] [crc u32]` | Data packets, in order, with up to 24 bytes of data |
| `A5 02 F1 01` | End the upload and mark it present |
| `A5 02 E0`, `A5 02 E1` | Delete the image or GIF, which brings back the stock default |

Uploads use the stock locations, so stock firmware reads them too: flags at `0x08600000` and `0x08601000`, GIF length at `0x08603000`, image at `0x08610000` and GIF at `0x08620000`. Writes are refused anywhere else, and the theme flag and stock assets are never touched. An upload that goes quiet for 5 s is cancelled.

UI frames can arrive between packets. A frame that could also be the expected packet is treated as the packet.
