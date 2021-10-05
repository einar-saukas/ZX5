# ZX5 (experimental)

**ZX5** is an _experimental_ data compressor derived from 
[ZX0](https://github.com/einar-saukas/ZX0), similarly targeted for low-end
platforms, including 8-bit computers like the ZX Spectrum.

Compared to [ZX0](https://github.com/einar-saukas/ZX0) format that supports 3
block types (including copy from last offset), **ZX5** extends this concept
adding 2 more block types: copy from second-to-last and third-to-last offset.
This additional complexity sometimes gives **ZX5** a slightly better compression
than [ZX0](https://github.com/einar-saukas/ZX0) in some cases. However **ZX5** 
decompressor is a little larger, and **ZX5** compressor is very much slower, 
therefore it's not very practical. For this reason, it's highly recommended to 
use [ZX0](https://github.com/einar-saukas/ZX0) instead. Although if you 
desperately need better compression for a very specific file, then it's worth 
to try **ZX5** anyway...


_NOTE: ZX5 decompressors for Z80 modify alternate registers. This will make a 
ZX Spectrum crash if your routine returns to BASIC without first restoring HL' 
to value $2758._


## Usage

To compress a file, use the command-line compressor as follows:

```
zx5 Cobra.scr
```

This will generate a compressed file called "Cobra.scr.zx5".

Afterwards you can choose a decompressor routine in assembly Z80, according to
your requirements for speed and size:

* "Standard" routine: 88 bytes only
* "Turbo" routine: 158 bytes, about 16% faster

Finally compile the chosen decompressor routine and load the compressed file
somewhere in memory. To decompress data, just call the routine specifying the
source address of compressed data in HL and the target address in DE.

For instance, if you compile the decompressor routine to address 65000, load
"Cobra.scr.zx5" at address 51200, and you want to decompress it directly to the
screen, then execute the following code:

```
    LD    HL, 51200  ; source address (put "Cobra.scr.zx5" there)
    LD    DE, 16384  ; target address (screen memory in this case)
    CALL  65000      ; decompress routine compiled at this address
```

It's also possible to decompress data into a memory area that partially overlaps
the compressed data itself (only if you won't need to decompress it again later,
obviously). In this case, the last address of compressed data must be at least
"delta" bytes higher than the last address of decompressed data. The exact value
of "delta" for each case is reported by **ZX5** during compression. See image
below:

```
                       |------------------|    compressed data
    |---------------------------------|       decompressed data
  start >>                            <--->
                                      delta
```

For convenience, there's also a command-line decompressor that works as follows:

```
dzx5 Cobra.scr.zx5
```


## Performance

The **ZX5** compressor algorithm is extremely slow and consumes a lot of memory.
Therefore [ZX0](https://github.com/einar-saukas/ZX0) is a much better choice!
However if you really want to try **ZX5**, then during development you can speed
up compression using **ZX5** in "quick" mode:

```
zx5 -q Cobra.scr
```

Later, when you finish making changes to your file, you can compress it again
without "quick" mode for maximum compression. Notice that using "quick" mode
will only affect the size of the compressed file, not its format. Therefore
all **ZX5** decompressor routines will continue to work exactly the same way.

Fortunately all complexity lies on the compression process only. The **ZX5**
compression format itself is reasonably simple and efficient, providing a high
compression ratio that can be decompressed quickly and easily. The provided
**ZX5** decompressor routines in assembly Z80 are fairly small and fast, they 
only use main (BC, DE, HL, AF) and alternate (BC', DE', HL') registers, consume 
very little stack space, and do not require additional decompression buffer.

The provided **ZX5** decompressor in C writes the output file while reading the
compressed file, without keeping it in memory. Therefore it always use the same
amount of memory, regardless of file size. Thus even large compressed files can
be decompressed in very small computers with limited memory, even if it took
considerable time and memory to compress it originally. It means decompressing
within asymptotically optimal space and time O(n) only, using storage space O(n)
for input and output files, and only memory space O(w) for processing.


## File Format

The **ZX5** compressed format contains 5 block types:

* Literal (copy next N bytes from compressed file)
```
    0  Elias(length)  byte[1]  byte[2]  ...  byte[N]
```

* Copy from last offset (repeat N bytes from last offset)
```
    0  Elias(length)
```

* Copy from 2nd last offset (repeat N bytes from 2nd last offset)
```
    1  0  0  Elias(length)
```

* Copy from 3rd last offset (repeat N bytes from 3rd last offset)
```
    1  0  1  Elias(length)
```

* Copy from new offset (repeat N bytes from new offset)
```
    1  1  *  Elias(MSB(offset)+1)  LSB(offset)  *Elias(length-1)
```

There's no need for extra bits to distinguish between first 2 block types,
because literal blocks cannot be consecutive, and reusing last offset can only
happen after a literal block. The first block is always a literal, so the first
bit is omitted.

The offset MSB and all lengths are stored using interlaced
[Elias Gamma Coding](https://en.wikipedia.org/wiki/Elias_gamma_coding), with the
first bit from `Elias(length-1)` stored separately for efficiency. A special
offset MSB value indicates EOF.


## Advanced Features

The **ZX5** compressor contains a few extra "hidden" features, that are slightly
harder to use properly, and not supported by the **ZX5** decompressor in C. Please
read carefully these instructions before attempting to use any of them!


#### _COMPRESSING BACKWARDS_

When using **ZX5** for "in-place" decompression (decompressing data to overlap the
same memory area storing the compressed data), you must always leave a small
margin of "delta" bytes of compressed data at the end. However it won't work to
decompress some large data that will occupy all the upper memory until the last
memory address, since there won't be even a couple bytes left at the end.

A possible workaround is to compress and decompress data backwards, starting at
the last memory address. Therefore you will only need to leave a small margin of
"delta" bytes of compressed data at the beginning instead. Technically, it will
require that lowest address of compressed data should be at least "delta" bytes
lower than lowest address of decompressed data. See image below:

     compressed data    |------------------|
    decompressed data       |---------------------------------|
                        <--->                            << start
                        delta

To compress a file backwards, use the command-line compressor as follows:

```
zx5 -b Cobra.scr
```

To decompress it later, you must call one of the supplied "backwards" variants
of the Assembly decompressor, specifying last source address of compressed data
in HL and last target address in DE.

For instance, if you compile a "backwards" Assembly decompressor routine to
address 64000, load backwards compressed file "Cobra.scr.zx5" (with size 2086
bytes) to address 51200, and want to decompress it directly to the ZX Spectrum
screen (with 6912 bytes), then execute the following code:

```
    LD    HL, 51200+2086-1  ; source (last address of "Cobra.scr.zx5")
    LD    DE, 16384+6912-1  ; target (last address of screen memory)
    CALL  64000             ; backwards decompress routine
```

Notice that compressing backwards may sometimes produce slightly smaller
compressed files in certain cases, slightly larger compressed files in others.
Overall it shouldn't make much difference either way.


#### _COMPRESSING WITH PREFIX_

The LZ77/LZSS compression is achieved by "abbreviating repetitions", such that
certain sequences of bytes are replaced with much shorter references to previous
occurrences of these same sequences. For this reason, it's harder to get very
good compression ratio on very short files, or in the initial parts of larger
files, due to lack of choices for previous sequences that could be referenced.

A possible improvement is to compress data while also taking into account what
else will be already stored in memory during decompression later. Thus the
compressed data may even contain shorter references to repetitions stored in
some previous "prefix" memory area, instead of just repetitions within the
decompressed area itself.

An input file may contain both some prefix data to be referenced only, and the
actual data to be compressed. An optional parameter can specify how many bytes
must be skipped before compression. See below:

```
                                        compressed data
                                     |-------------------|
         prefix             decompressed data
    |--------------|---------------------------------|
                 start >>
    <-------------->                                 <--->
          skip                                       delta
```

As usual, if you want to decompress data into a memory area that partially
overlaps the compressed data itself, the last address of compressed data must be
at least "delta" bytes higher than the last address of decompressed data.

For instance, if you want the first 6144 bytes of a certain file to be skipped
(not compressed but possibly referenced), then use the command-line compressor
as follows:

```
zx5 +6144 Cobra.cbr
```

In practice, suppose an action game uses a few generic sprites that are common
for all levels (such as player graphics), and other sprites are specific for
each level (such as enemies). All generic sprites must stay always accessible at
a certain memory area, but any level specific data can be only decompressed as
needed, to the memory area immediately following it. In this case, the generic
sprites area could be used as prefix when compressing and decompressing each
level, in an attempt to improve compression. For instance, suppose generic
graphics are loaded from file "generic.gfx" to address 56000, occupying 2500
bytes, and level specific graphics will be decompressed immediately afterwards,
to address 58500. To compress each level using "generic.gfx" as a 2500 bytes
prefix, use the command-line compressor as follows:

```
copy /b generic.gfx+level_1.gfx prefixed_level_1.gfx
zx5 +2500 prefixed_level_1.gfx

copy /b generic.gfx+level_2.gfx prefixed_level_2.gfx
zx5 +2500 prefixed_level_2.gfx

copy /b generic.gfx+level_3.gfx prefixed_level_3.gfx
zx5 +2500 prefixed_level_3.gfx
```

To decompress it later, you simply need to use one of the normal variants of the
Assembly decompressor, as usual. In this case, if you loaded compressed file
"prefixed_level_1.gfx.zx5" to address 48000 for instance, decompressing it will
require the following code:

```
    LD    HL, 48000  ; source address (put "prefixed_level_1.gfx.zx5" there)
    LD    DE, 58500  ; target address (level specific memory area in this case)
    CALL  65000      ; decompress routine compiled at this address
```

However decompression will only work properly if exactly the same prefix data is
present in the memory area immediately preceding the decompression address.
Therefore you must be extremely careful to ensure the prefix area does not store
variables, self-modifying code, or anything else that may change prefix content
between compression and decompression. Also don't forget to recompress your
files whenever you modify a prefix!

In certain cases, compressing with a prefix may considerably help compression.
In others, it may not even make any difference. It mostly depends on how much
similarity exists between data to be compressed and its provided prefix.


#### _COMPRESSING BACKWARDS WITH SUFFIX_

Both features above can be used together. A file can be compressed backwards,
with an optional parameter to specify how many bytes should be skipped (not
compressed but possibly referenced) from the end of the input file instead. See
below:

```
       compressed data
    |-------------------|
                 decompressed data             suffix
        |---------------------------------|--------------|
                                     << start
    <--->                                 <-------------->
    delta                                       skip
```

As usual, if you want to decompress data into a memory area that partially
overlaps the compressed data itself, lowest address of compressed data must be
at least "delta" bytes lower than lowest address of decompressed data.

For instance, if you want to skip the last 768 bytes of a certain input file and
compress everything else (possibly referencing this "suffix" of 768 bytes), then
use the command-line compressor as follows:

```
zx5 -b +768 Cobra.cbr
```

In previous example, suppose the action game now stores level-specific sprites
in the memory area from address 33000 to 33511 (512 bytes), just before generic
sprites that are stored from address 33512 to 34535 (1024 bytes). In this case,
these generic sprites could be used as suffix when compressing and decompressing
level-specific data as needed, in an attempt to improve compression. To compress
each level using "generic.gfx" as a 1024 bytes suffix, use the command-line
compressor as follows:

```
copy /b level_1.gfx+generic.gfx level_1_suffixed.gfx
zx5 -b +1024 level_1_suffixed.gfx

copy /b level_2.gfx+generic.gfx level_2_suffixed.gfx
zx5 -b +1024 level_2_suffixed.gfx

copy /b level_3.gfx+generic.gfx level_3_suffixed.gfx
zx5 -b +1024 level_3_suffixed.gfx
```

To decompress it later, use the backwards variant of the Assembly decompressor.
In this case, if you compile a "backwards" decompressor routine to address
64000, and load compressed file "level_1_suffixed.gfx.zx5" (with 217 bytes) to
address 39000 for instance, decompressing it will require the following code:

```
    LD    HL, 39000+217-1  ; source (last address of "level_1_suffixed.gfx.zx5")
    LD    DE, 33000+512-1  ; target (last address of level-specific data)
    CALL  64000            ; backwards decompress routine
```

Analogously, decompression will only work properly if exactly the same suffix
data is present in the memory area immediately following the decompression area.
Therefore you must be extremely careful to ensure the suffix area does not store
variables, self-modifying code, or anything else that may change suffix content
between compression and decompression. Also don't forget to recompress your
files whenever you modify a suffix!

Also if you are using "in-place" decompression, you must leave a small margin of
"delta" bytes of compressed data just before the decompression area.


## License

The **ZX5** data compression format and algorithm was designed and implemented 
by **Einar Saukas**, based on previous work on [ZX0](https://github.com/einar-saukas/ZX0).

The optimal C compressor is available under the "BSD-3" license. In practice,
this is relevant only if you want to modify its source code and/or incorporate
the compressor within your own products. Otherwise, if you just execute it to
compress files, you can simply ignore these conditions.

The decompressors can be used freely within your own programs (either for the
ZX Spectrum or any other platform), even for commercial releases. The only
condition is that you must indicate somehow in your documentation that you have
used **ZX5**.


## Links

* [ZX0](https://github.com/einar-saukas/ZX0) - The original compressor on
which **ZX5** is based.
