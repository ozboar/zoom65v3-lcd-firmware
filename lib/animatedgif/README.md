# AnimatedGIF

The C decoder from [bitbank2/AnimatedGIF](https://github.com/bitbank2/AnimatedGIF) at `c2478ec` (version 2.2.3), Copyright 2020 BitBank Software, Inc., under the Apache License 2.0, see [LICENSE](LICENSE).

`gif.inl` is changed from upstream:

- Only uses NEON where the compiler has it, as this Cortex-M4 doesn't
- A frame with no delay gets 30 ms instead of 100 ms, the speed stock plays these files at
- Keeps a gap between the Turbo mode output and its root symbols. Copies can run up to a word past the end of the output, which corrupted those symbols and so the last pixels of some frames
- Rejects a frame bigger than the Turbo mode output buffer, and a code that isn't in the dictionary yet, which would otherwise corrupt memory on a malformed file
