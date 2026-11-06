# Zoom65 v3 LCD Firmware

Custom firmware for the stock Zoom65 v3 screen module. The screen MCU does the drawing and animation, and the keyboard only sends it data.

It ports the ScreenKey UI to the 118x118 visible LCD area: bongo cat with WPM and real key presses on its keyboard, and keyboard status. It also adds a clock with four faces, a live WPM graph, an RGB settings page, and system and weather pages that show `zoom-sync` data.

## Status

The firmware only ever runs from SRAM, and never writes the screen MCU's internal flash, by design. The screen keeps its stock firmware, and a reset without the keyboard's help returns to it. The external flash is only written for image and GIF uploads, in the same slots stock uses. The weather icons are read from the stock art on it.

It ships inside the keyboard firmware, on the [`meletrix_zoom65_v3`](https://github.com/ozboar/qmk_firmware/tree/meletrix_zoom65_v3) branch of the QMK fork, so there is nothing to flash separately. Release binaries are on [that fork's releases page](https://github.com/ozboar/qmk_firmware/releases).

The backlight follows the keyboard's RGB brightness, and dims when idle. While the screen is off, its MCU deep sleeps, and a byte from the keyboard wakes it. See [docs/protocol.md](docs/protocol.md#deep-sleep).

SC_DOWN goes to the next page, SC_UP goes to the previous one, and SC_TOGG turns the screen on and off. SC_SWCH changes a setting on the current page: on the cat, WPM graph and keyboard info pages it cycles the accent color, on the clock page it cycles the face, on the image and GIF page it cycles the image, the GIF and a GIF that only plays while typing, and on the system and weather pages it switches between Celsius and Fahrenheit. The graph, RGB and status pages have no setting yet.

## How it boots

The keyboard carries this firmware and loads it at every boot, while typing keeps working:

1. Resets the screen to stock and sends `A5 00 FE`, which enters the ROM bootloader.
2. Loads the first stage loader in `stage1/` through the ROM at 230400 baud.
3. Sends the firmware to the first stage at 3 Mbaud, and checks its CRC.
4. Starts the firmware, moves the link to 1.5 Mbaud, and sends it the UI state.

If this fails after 3 attempts, the screen stays on stock and the keyboard works normally.

## Build and run

```sh
git submodule update --init
nix develop
make
```

To put a new build on the keyboard, export it to the `meletrix_zoom65_v3` QMK branch, then build and flash that:

```sh
make export QMK=<path to QMK checkout>
qmk compile -kb meletrix/zoom65_v3/ansi -km via  # in the QMK checkout
```

To switch between this firmware and stock, press `Fn+/` on the VIA keymap, or use the Custom screen firmware toggle in VIA.

## Layout

| Path | Contents |
|---|---|
| `src/lcd.c` | Panel init, rotation and visible area mapping |
| `src/draw.c` | Rectangles, images and text |
| `src/extflash.c` | Reads stock images and uploads from the external flash |
| `src/media.c` | Image and GIF page, and stock uploads to the external flash |
| `src/gif.c` | GIF decoder |
| `src/link.c` | UART frames from the keyboard, see [docs/protocol.md](docs/protocol.md) |
| `src/ui.c` | The UI modes |
| `src/assets.c` | Generated images and fonts |
| `stage1/` | First stage loader, receives the firmware at 3 Mbaud |
| `tools/export_qmk.py` | Writes the firmware as a C file for the keyboard |

## Regenerating assets

`src/assets.c` is checked in. To rebuild it:

```sh
git clone https://github.com/Externalizable/bongo.cat /tmp/bongo.cat
mkdir -p /tmp/frames
python3 tools/generate_bongo.py /tmp/bongo.cat /tmp/frames
python3 tools/gen_assets.py /tmp/frames assets/fonts src
```

`generate_bongo.py` prints the paw positions, which have to match the `BONGO_*` values in `src/ui.c`.

## Screenshots

`make screenshots` builds the UI for this PC and saves each page to `build/screenshots`, with a contact sheet in `sheet.png`. It runs the firmware's real UI, drawing, GIF and flash reading code against the flash dumps from `zoom65v3-re`. Only the LCD and clock are replaced, so colors are the raw values rather than what the panel shows, and uploads do not write anything.

The pages and sample data come from `tools/sim/screenshots.txt`. To use your own uploads, point it at a raw external flash dump with `make screenshots EXTERNAL_FLASH=path/to/dump.bin`.

## Credits

- The cat comes from the dark layers of [bongo.cat](https://github.com/Externalizable/bongo.cat) (MIT, Copyright (c) 2018 Eric Huber), redrawn with a keyboard. That site credits the meme to DitzyFlama, using a drawing by StrayRogue.
- The font is Thintel, from QMK. The large digits are its digits at 3x.
- `lib/at32` is Artery's AT32F403A/407 firmware library (BSD 3-Clause).

## License

GPL-2.0-or-later, see [LICENSE](LICENSE).
