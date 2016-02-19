
# font-tool

Command line tool that converts text FNT files and font bitmaps to C/C++ code.

To generate a FNT file and a glyph bitmap from a TTF typeface, I suggest using
[Hiero](https://github.com/libgdx/libgdx/wiki/Hiero) or [BMFont](http://www.angelcode.com/products/bmfont/).

----

<pre>
Usage:
 $ font-tool file.fnt [bitmap-file] [output-file] [font-name] [options]
 Converts a text FNT file and associated glyph bitmap to C/C++ code that can be embedded into an application.
 Parameters are:
  (req) file.fnt     Name of a .FNT file with the glyph info. The Hiero tool can be used to generate those from a TTF typeface.
  (opt) bitmap-file  Name of the image with the glyphs. If not provided, use the filename found inside the FNT file.
  (opt) output-file  Name of the .c/.h file to write, including extension. If not provided, use file.h.
  (opt) font-name    Name of the typeface that will be used to name the data arrays. If omitted, use the FNT file name.
 Options are:
  -h, --help         Prints this message and exits.
  -v, --verbose      Prints some verbose stats about the program execution.
  -c, --compress     Compresses the output glyph bitmap array with RLE encoding by default.
  -s, --static       Qualify the C/C++ arrays with the 'static' storage class.
  -m, --mutable      Allow the output data to be mutable, i.e. omit the 'const' qualifier.
  -S, --structs      Also outputs the 'FontChar/FontCharSet' structures at the beginning of the file.
  -T, --stdtypes     Use Standard C++ types like std::uint8_t and std::uint16_t in the output structs/arrays.
  -H, --hex          Write the glyph bitmap data as an escaped hexadecimal string. The default is an array of hexa unsigned bytes.
  -x, --rgba         Write the glyph bitmap in RGBA format. Default is 1-byte-per-pixel grayscale.
  --align=N          Applies GCC/Clang __attribute__((aligned(N))) extension to the output arrays.
  --encoding=method  If combined with -c/--compress, specifies the encoding to use. Methods are: rle,lzw,huff. Defaults to rle.
</pre>

