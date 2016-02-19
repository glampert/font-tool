
// ================================================================================================
// -*- C++ -*-
// File: fnt.hpp
// Author: Guilherme R. Lampert
// Created on: 18/02/16
// Brief: FNT file parsing and the font/char-set structures.
//
// This source code is released under the MIT license.
// See the accompanying LICENSE file for details.
// ================================================================================================

#ifndef FNT_HPP
#define FNT_HPP

#include "utils.hpp"

//
// Output Font structures:
//

struct FontChar
{
    // Position inside the glyph bitmap.
    std::uint16_t x;
    std::uint16_t y;
};

struct FontCharSet
{
    // The ASCII charset only!
    enum { MaxChars = 256 };

    // Bitmap with the font glyphs/chars.
    // NOTE: It might be stored using RLE/LZW/Huffman compression.
    const std::uint8_t * bitmap;
    int bitmapWidth;
    int bitmapHeight;

    // 1=Graymap : Each byte is a [0,255] opacity scale for the glyph.
    // 4=RGBA    : Full RGB color range plus 1 byte alpha for opacity.
    int bitmapColorChannels;

    // Nonzero if 'bitmap' is compressed, zero otherwise.
    // When > 0 holds the decompressed size of 'bitmap'.
    int bitmapDecompressSize;

    // The number of pixels from the absolute top of the line to the base of the characters.
    // See: http://www.angelcode.com/products/bmfont/doc/file_format.html
    int charBaseHeight;

    // Assumed fixed width & height.
    int charWidth;
    int charHeight;
    int charCount;
    FontChar chars[MaxChars];
};

// Simple text FNT parser that reads only the fields we care about.
// Calls ::error() if something goes wrong.
void parseTextFntFile(const std::string & filename, FontCharSet & charSetOut, std::string * fntBitmapFile);

#endif // FNT_HPP
