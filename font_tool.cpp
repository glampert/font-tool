
// ================================================================================================
// -*- C++ -*-
// File: font_tool.cpp
// Author: Guilherme R. Lampert
// Created on: 12/12/15
//
// Brief: Command line tool that converts a .FNT file + its bitmap to C code.
//
// This source code is released under the MIT license.
// See the accompanying LICENSE file for details.
//
// ================================================================================================

//
// c++ -std=c++14 -O3 -Wall -Wextra -Weffc++ font_tool.cpp -o font-tool
//
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

static void error(const std::string & message)
{
    std::cerr << "ERROR: " << message << std::endl;
    throw std::runtime_error(message);
}

// ========================================================
// Run Length Encoding for the font bitmap:
// ========================================================

// 16-bits run-length word allows for very big sequences,
// but is also very inefficient if the run-lengths are generally
// short. Byte-size words are used if this is not defined.
//#define RLE_WORD_SIZE_16 1

#ifdef RLE_WORD_SIZE_16
    typedef unsigned short RLE_Word;
    static const RLE_Word RLE_MaxCount = 0xFFFF; // Max run length: 65535 => 4 bytes.
#else // !RLE_WORD_SIZE_16
    typedef unsigned char RLE_Word;
    static const RLE_Word RLE_MaxCount = 0xFF; // Max run length: 255 => 2 bytes.
#endif // RLE_WORD_SIZE_16

typedef unsigned char RLE_Byte;

template<class T>
inline int rleWrite(RLE_Byte *& output, const T val)
{
    *reinterpret_cast<T *>(output) = val;
    output += sizeof(T);
    return sizeof(T);
}

template<class T>
inline void rleRead(const RLE_Byte *& input, T & val)
{
    val = *reinterpret_cast<const T *>(input);
    input += sizeof(T);
}

int rleEncode(RLE_Byte * output, const int outSizeBytes, const RLE_Byte * input, const int inSizeBytes)
{
    if (!output || !input)
    {
        return -1;
    }
    if (outSizeBytes <= 0 || inSizeBytes <= 0)
    {
        return -1;
    }

    int bytesWritten  = 0;
    RLE_Word rleCount = 0;
    RLE_Byte rleByte  = *input;

    for (int i = 0; i < inSizeBytes; ++i, ++rleCount)
    {
        const RLE_Byte b = *input++;

        // Output when we hit the end of a sequence or the max size of a RLE word:
        if (b != rleByte || rleCount == RLE_MaxCount)
        {
            if ((bytesWritten + sizeof(RLE_Word) + sizeof(RLE_Byte)) > unsigned(outSizeBytes))
            {
                // Can't fit anymore data! Stop with an error.
                return -1;
            }

            bytesWritten += rleWrite(output, rleCount);
            bytesWritten += rleWrite(output, rleByte);

            rleCount = 0;
            rleByte  = b;
        }
    }

    // Residual count at the end:
    if (rleCount != 0)
    {
        if ((bytesWritten + sizeof(RLE_Word) + sizeof(RLE_Byte)) > unsigned(outSizeBytes))
        {
            return -1; // No more space! Output not complete.
        }
        rleWrite(output, rleCount);
        rleWrite(output, rleByte);
    }

    return bytesWritten;
}

int rleDecode(RLE_Byte * output, const int outSizeBytes, const RLE_Byte * input, const int inSizeBytes)
{
    if (!output || !input)
    {
        return -1;
    }
    if (outSizeBytes <= 0 || inSizeBytes <= 0)
    {
        return -1;
    }

    int bytesWritten = 0;
    RLE_Word rleCount;
    RLE_Byte rleByte;

    for (int i = 0; i < inSizeBytes; ++i)
    {
        rleRead(input, rleCount);
        rleRead(input, rleByte);

        // Replicate the RLE packet.
        while (rleCount--)
        {
            *output++ = rleByte;
            if (++bytesWritten == outSizeBytes && rleCount > 0)
            {
                // Reached end of output and we are not done yet, stop with an error.
                return -1;
            }
        }
    }

    return bytesWritten;
}

// ========================================================
// Image loading & decompression via STB Image:
// ========================================================

#define STB_IMAGE_IMPLEMENTATION 1
#define STBI_ONLY_PNG  1
#define STBI_ONLY_TGA  1
#define STBI_ONLY_JPEG 1
#include "stb_image.h"

static void loadFontBitmap(int & width, int & height, int & channels, std::vector<RLE_Byte> & bitmap,
                           const std::string & filename, const bool grayscale)
{
    int x = 0, y = 0, c = 0;
    RLE_Byte * data = stbi_load(filename.c_str(), &x, &y, &c, /* require RGBA */ 4);

    if (!data)
    {
        error("Unable to load image from \"" + filename + "\": "
              + std::string(stbi_failure_reason()));
    }

    if (x <= 0 || y <= 0 || c <= 0)
    {
        stbi_image_free(data);
        error("Unable to load image from \"" + filename + "\": bad dimensions");
    }

    if (grayscale)
    {
        const int count = x * y;
        const RLE_Byte * ptr = data;

        c = 1; // 1 byte per-pixel
        bitmap.resize(count);

        for (int i = 0; i < count; ++i)
        {
            bitmap[i] = ptr[3]; // Store the alpha intensity only.
            ptr += 4;
        }
    }
    else
    {
        c = 4; // RGBA
        bitmap.resize(x * y * c);
        std::memcpy(bitmap.data(), data, x * y * c);
    }

    width    = x;
    height   = y;
    channels = c;

    stbi_image_free(data);
}

// ========================================================
// Font Tools:
// ========================================================

static const char * formatMemoryUnit(const std::size_t sizeBytes, const int abbreviated = true)
{
    enum
    {
        KILOBYTE = 1024,
        MEGABYTE = 1024 * KILOBYTE,
        GIGABYTE = 1024 * MEGABYTE
    };

    const char * memUnitStr;
    double adjustedSize;

    if (sizeBytes < KILOBYTE)
    {
        memUnitStr = abbreviated ? "B" : "Bytes";
        adjustedSize = static_cast<double>(sizeBytes);
    }
    else if (sizeBytes < MEGABYTE)
    {
        memUnitStr = abbreviated ? "KB" : "Kilobytes";
        adjustedSize = sizeBytes / 1024.0;
    }
    else if (sizeBytes < GIGABYTE)
    {
        memUnitStr = abbreviated ? "MB" : "Megabytes";
        adjustedSize = sizeBytes / 1024.0 / 1024.0;
    }
    else
    {
        memUnitStr = abbreviated ? "GB" : "Gigabytes";
        adjustedSize = sizeBytes / 1024.0 / 1024.0 / 1024.0;
    }

    char * fmtBuf;
    char strBuf[200];

    // Max chars reserved for the output string, max `strBuf + 28` chars for the unit str:
    enum { MEM_UNIT_STR_MAXLEN = sizeof(strBuf) + 28 };

    static int bufNum = 0;
    static char localStrBuf[8][MEM_UNIT_STR_MAXLEN];

    fmtBuf = localStrBuf[bufNum];
    bufNum = (bufNum + 1) & 7;

    // We only care about the first 2 decimal digits.
    std::snprintf(strBuf, sizeof(strBuf), "%.2f", adjustedSize);

    // Remove trailing zeros if no significant decimal digits:
    for (char * p = strBuf; *p != '\0'; ++p)
    {
        if (*p == '.')
        {
            // Find the end of the string
            while (*++p) { }

            // Remove trailing zeros
            while (*--p == '0')
            {
                *p = '\0';
            }
            // If the dot was left alone at the end, remove it too
            if (*p == '.')
            {
                *p = '\0';
            }
            break;
        }
    }

    std::snprintf(fmtBuf, MEM_UNIT_STR_MAXLEN, "%s %s", strBuf, memUnitStr);
    return fmtBuf;
}

struct ProgramOptions
{
    std::string fntFileName;
    std::string bitmapFileName;
    std::string outputFileName;
    std::string fontName;

    bool verbose         = false;
    bool compressBitmap  = false;
    bool staticStorage   = false;
    bool outputStructs   = false;
    bool rgbaBitmap      = false;
    int  alignmentAmount = 0;
};

struct FontChar
{
    // Position inside the glyph bitmap.
    unsigned short x;
    unsigned short y;
};

struct FontCharSet
{
    // The ASCII charset only!
    enum { MaxChars = 256 };

    // Bitmap with the font glyphs/chars.
    // NOTE: It might be stored using RLE compression!
    const unsigned char * bitmap;
    int bitmapWidth;
    int bitmapHeight;

    // 1=Graymap : Each byte is a [0,255] opacity scale for the glyph.
    // 4=RGBA    : Full RGB color range plus 1 byte alpha for opacity.
    int bitmapColorChannels;

    // Nonzero if 'bitmap' is RLE encoded.
    // If it is > 0, holds the decompressed size of 'bitmap'.
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

static void writeStructures(FILE * fileOut)
{
    std::fprintf(fileOut, "\n");
    std::fprintf(fileOut, "struct FontChar\n");
    std::fprintf(fileOut, "{\n");
    std::fprintf(fileOut, "    unsigned short x;\n");
    std::fprintf(fileOut, "    unsigned short y;\n");
    std::fprintf(fileOut, "};\n");
    std::fprintf(fileOut, "\n");
    std::fprintf(fileOut, "struct FontCharSet\n");
    std::fprintf(fileOut, "{\n");
    std::fprintf(fileOut, "    enum { MaxChars = 256 };\n");
    std::fprintf(fileOut, "    const unsigned char * bitmap;\n");
    std::fprintf(fileOut, "    int bitmapWidth;\n");
    std::fprintf(fileOut, "    int bitmapHeight;\n");
    std::fprintf(fileOut, "    int bitmapColorChannels;\n");
    std::fprintf(fileOut, "    int bitmapDecompressSize;\n");
    std::fprintf(fileOut, "    int charBaseHeight;\n");
    std::fprintf(fileOut, "    int charWidth;\n");
    std::fprintf(fileOut, "    int charHeight;\n");
    std::fprintf(fileOut, "    int charCount;\n");
    std::fprintf(fileOut, "    FontChar chars[MaxChars];\n");
    std::fprintf(fileOut, "};\n");
}

static void writeComment(FILE * fileOut, const std::string & fontName, const int argc, const char * argv[])
{
    std::string cmdLine;
    for (int i = 1; i < argc; ++i)
    {
        cmdLine += " ";
        cmdLine += argv[i];
    }

    fprintf(fileOut, "\n/*\n");
    fprintf(fileOut, " * File generated from font '%s' by font-tool.\n", fontName.c_str());
    fprintf(fileOut, " * Command line:%s\n", cmdLine.c_str());
    fprintf(fileOut, " */\n");
}

static void writeCharSet(FILE * fileOut, std::string arrayName, const bool staticStorage,
                         const int alignment, const FontCharSet & charSet)
{
    std::fprintf(fileOut, "\n");

    arrayName[0] = std::toupper(arrayName[0]); // Capitalize first letter for CamelCase name
    char alignStr[256] = {'\0'};

    // NOTE: Currently CGG/Clang alignment only!
    if (alignment > 0)
    {
        std::snprintf(alignStr, sizeof(alignStr), " __attribute__((aligned(%d))) ", alignment);
    }
    else
    {
        alignStr[0] = ' ';
    }

    if (staticStorage)
    {
        std::fprintf(fileOut, "static const FontCharSet font%sCharSet%s= {\n",
                     arrayName.c_str(), alignStr);
    }
    else
    {
        std::fprintf(fileOut, "const FontCharSet font%sCharSet%s= {\n",
                     arrayName.c_str(), alignStr);
    }

    std::fprintf(fileOut, "  /* bitmap               = */ font%sBitmap,\n", arrayName.c_str());
    std::fprintf(fileOut, "  /* bitmapWidth          = */ %d,\n", charSet.bitmapWidth);
    std::fprintf(fileOut, "  /* bitmapHeight         = */ %d,\n", charSet.bitmapHeight);
    std::fprintf(fileOut, "  /* bitmapColorChannels  = */ %d,\n", charSet.bitmapColorChannels);
    std::fprintf(fileOut, "  /* bitmapDecompressSize = */ %d,\n", charSet.bitmapDecompressSize);
    std::fprintf(fileOut, "  /* charBaseHeight       = */ %d,\n", charSet.charBaseHeight);
    std::fprintf(fileOut, "  /* charWidth            = */ %d,\n", charSet.charWidth);
    std::fprintf(fileOut, "  /* charHeight           = */ %d,\n", charSet.charHeight);
    std::fprintf(fileOut, "  /* charCount            = */ %d,\n", charSet.charCount);
    std::fprintf(fileOut, "  {\n  /*   x,    y */\n");

    const int count = FontCharSet::MaxChars;
    for (int i = 0; i < count; ++i)
    {
        const FontChar & chr = charSet.chars[i];
        std::fprintf(fileOut, "  { %4u, %4u }", chr.x, chr.y);

        if (i != count - 1)
        {
            std::fprintf(fileOut, ",\n");
        }
    }

    std::fprintf(fileOut, "}\n");
    std::fprintf(fileOut, "};\n");
}

static void writeBitmapArray(FILE * fileOut, std::string arrayName, const bool staticStorage,
                             const int alignment, const std::vector<RLE_Byte> & bitmap, const int encodedSize)
{
    std::fprintf(fileOut, "\n");

    const char * memSize = formatMemoryUnit(encodedSize, true); // For a comment
    arrayName[0] = std::toupper(arrayName[0]); // Capitalize first letter for CamelCase name
    char alignStr[256] = {'\0'};

    // NOTE: Currently CGG/Clang alignment only!
    if (alignment > 0)
    {
        std::snprintf(alignStr, sizeof(alignStr), " __attribute__((aligned(%d))) ", alignment);
    }
    else
    {
        alignStr[0] = ' ';
    }

    if (staticStorage)
    {
        std::fprintf(fileOut, "static const int font%sBitmapSizeBytes = %d;\n",
                     arrayName.c_str(), encodedSize);
        std::fprintf(fileOut, "static const unsigned char font%sBitmap[]%s= { // ~%s\n  ",
                     arrayName.c_str(), alignStr, memSize);
    }
    else
    {
        std::fprintf(fileOut, "const int font%sBitmapSizeBytes = %d;\n",
                     arrayName.c_str(), encodedSize);
        std::fprintf(fileOut, "const unsigned char font%sBitmap[]%s= { // ~%s\n  ",
                     arrayName.c_str(), alignStr, memSize);
    }

    int column = 1;
    const int maxColumns = 15;

    for (int i = 0; i < encodedSize; ++i, ++column)
    {
        assert(unsigned(i) < bitmap.size());
        std::fprintf(fileOut, "0x%02X", bitmap[i]);

        if (i != encodedSize - 1)
        {
            std::fprintf(fileOut, ", ");
        }
        if (column == maxColumns)
        {
            std::fprintf(fileOut, "\n  ");
            column = 0;
        }
    }

    std::fprintf(fileOut, " };\n");
}

static inline bool startsWith(const char * str, const char * prefix)
{
    const auto strLen = std::strlen(str);
    const auto prefixLen = std::strlen(prefix);
    if (strLen < prefixLen)
    {
        return false;
    }
    if (strLen == 0 || prefixLen == 0)
    {
        return false;
    }
    return std::strncmp(str, prefix, prefixLen) == 0;
}

static inline int scanInt(const char * token)
{
    char * endPtr = nullptr;
    int result = static_cast<int>(std::strtol(token, &endPtr, 0));

    if (!endPtr || endPtr == token)
    {
        error("FNT parse error: Expected number instead of token '" + std::string(token) + "'!");
    }

    return result;
}

static void handleFntToken(const char * token, FontCharSet & charSetOut, std::string * fntBitmapFile)
{
    assert(token != nullptr);

    // This is a one-shot function, so I don't really care about this shared state right now...
    static FontChar * currentChar = nullptr;
    static bool prevTokenWasChar  = false;
    static int  largestHeight     = 0;
    static int  largestWidth      = 0;

    if (std::strcmp(token, "char") == 0)
    {
        prevTokenWasChar = true;
        return;
    }

    if (startsWith(token, "base="))
    {
        charSetOut.charBaseHeight = scanInt(token + 5);
    }
    else if (startsWith(token, "file="))
    {
        if (fntBitmapFile)
        {
            (*fntBitmapFile) = token + 5;
            // Normally quoted, strip 'em:
            if (fntBitmapFile->front() == '"')
            {
                (*fntBitmapFile) = fntBitmapFile->substr(1, fntBitmapFile->length() - 2);
            }
        }
    }
    else if (startsWith(token, "id=") && prevTokenWasChar) // Need prevTokenWasChar because there's "char id=" and "page id=".
    {
        const int charIndex = scanInt(token + 3);
        if (charIndex < 0 || charIndex >= FontCharSet::MaxChars)
        {
            error("Char index out-of-range!");
        }
        currentChar = &charSetOut.chars[charIndex];
        charSetOut.charCount++;
    }
    else if (startsWith(token, "x="))
    {
        assert(currentChar != nullptr);
        currentChar->x = scanInt(token + 2);
    }
    else if (startsWith(token, "y="))
    {
        assert(currentChar != nullptr);
        currentChar->y = scanInt(token + 2);
    }
    else if (startsWith(token, "height="))
    {
        const int height = scanInt(token + 7);
        if (height > largestHeight)
        {
            largestHeight = height;
        }
    }
    else if (startsWith(token, "xadvance="))
    {
        const int width = scanInt(token + 9);
        if (width > largestWidth)
        {
            largestWidth = width;
        }
    }

    charSetOut.charWidth  = largestWidth;
    charSetOut.charHeight = largestHeight;
    prevTokenWasChar      = false;
}

static void parseTextFntFile(const std::string & filename, FontCharSet & charSetOut, std::string * fntBitmapFile)
{
    FILE * fileIn = nullptr;
    #ifdef _MSC_VER
    fopen_s(&fileIn, filename.c_str(), "rt");
    #else // !_MSC_VER
    fileIn = std::fopen(filename.c_str(), "rt");
    #endif // _MSC_VER

    if (!fileIn)
    {
        error("Unable to open FNT file \"" + filename + "\" for reading!");
    }

    enum { LineCharsMax = 4096 };
    static char lineBuffer[LineCharsMax];

    char *line, *token;
    while (!std::feof(fileIn))
    {
        if (!(line = std::fgets(lineBuffer, LineCharsMax, fileIn)))
        {
            break;
        }

        token = std::strtok(line, " \t\n\r"); // Split each line by whitespace.
        while (token)
        {
            handleFntToken(token, charSetOut, fntBitmapFile);
            token = std::strtok(nullptr, " \t\n\r");
        }
    }

    std::fclose(fileIn);
}

static void runFontTool(ProgramOptions & opts, const int argc, const char * argv[])
{
    //
    // Process the FNT:
    //
    if (opts.verbose) { std::cout << "> Parsing the FNT file...\n"; }
    FontCharSet charSet;
    std::memset(&charSet, 0, sizeof(charSet));
    parseTextFntFile(opts.fntFileName, charSet, (opts.bitmapFileName.empty() ? &opts.bitmapFileName : nullptr));

    //
    // Process the glyph bitmap image:
    //
    if (opts.verbose) { std::cout << "> Loading the glyph bitmap...\n"; }
    int width, height, channels;
    std::vector<RLE_Byte> inputBitmap;
    loadFontBitmap(width, height, channels, inputBitmap, opts.bitmapFileName, !opts.rgbaBitmap);

    int encodedSize      = 0;
    const int inputSize  = width * height * channels;
    const int outputSize = inputSize; // If we can't make it smaller, don't compress.
    std::vector<RLE_Byte> outputBitmap;

    assert(unsigned(inputSize) == inputBitmap.size());

    // Optional RLE compression of the output bitmap array:
    if (opts.compressBitmap)
    {
        if (opts.verbose) { std::cout << "> Attempting to RLE compress the glyph bitmap...\n"; }

        outputBitmap.resize(outputSize);
        encodedSize = rleEncode(outputBitmap.data(), outputSize, inputBitmap.data(), inputSize);

        // Run again without '-c/--compress'
        if (encodedSize <= 0)
        {
            error("RLE compression would produce a bigger bitmap! Cowardly refusing to RLE compress...");
        }

        // RLE compression stats:
        if (opts.verbose)
        {
            const int diff = inputSize - encodedSize;
            const double ratio = static_cast<double>(inputSize) / static_cast<double>(encodedSize);

            std::cout << "> Compression stats:\n";
            std::cout << "Bitmap dimensions..: " << width << "x" << height << "\n";
            std::cout << "Original size......: " << formatMemoryUnit(inputSize)   << "\n";
            std::cout << "Compressed size....: " << formatMemoryUnit(encodedSize) << "\n";
            std::cout << "Space saved........: " << formatMemoryUnit(diff)        << "\n";
            std::cout << "Compression ratio..: " << ratio << "\n";
        }
    }
    else
    {
        outputBitmap = std::move(inputBitmap);
        encodedSize  = inputSize;
    }

    // Update them from the just loaded image:
    charSet.bitmapWidth          = width;
    charSet.bitmapHeight         = height;
    charSet.bitmapColorChannels  = channels;
    charSet.bitmapDecompressSize = (opts.compressBitmap ? inputSize : 0);

    //
    // Write the C++ code:
    //
    if (opts.verbose) { std::cout << "> Writing output file...\n"; }

    FILE * fileOut = nullptr;
    #ifdef _MSC_VER
    fopen_s(&fileOut, opts.outputFileName.c_str(), "wt");
    #else // !_MSC_VER
    fileOut = std::fopen(opts.outputFileName.c_str(), "wt");
    #endif // _MSC_VER

    if (!fileOut)
    {
        error("Unable to open file \"" + opts.outputFileName + "\" for writing!");
    }

    writeComment(fileOut, opts.fntFileName, argc, argv);

    if (opts.outputStructs)
    {
        writeStructures(fileOut);
    }

    writeBitmapArray(fileOut, opts.fontName, opts.staticStorage,
                     opts.alignmentAmount, outputBitmap, encodedSize);

    writeCharSet(fileOut, opts.fontName, opts.staticStorage,
                 opts.alignmentAmount, charSet);

    std::fprintf(fileOut, "\n");
    std::fclose(fileOut);

    if (opts.verbose) { std::cout << "> Done!\n"; }
}

static inline std::string removeFilenameExtension(const std::string & filename)
{
	const auto lastDot = filename.find_last_of('.');
	if (lastDot == std::string::npos)
	{
		return filename;
	}
	return filename.substr(0, lastDot);
}

static inline bool hasCmdFlag(const char * test, const char * shortForm, const char * longForm)
{
    return std::strcmp(test, shortForm) == 0 ||
           std::strcmp(test, longForm)  == 0;
}

static inline bool isCmdFlag(const char * arg)
{
    return arg != nullptr && *arg == '-';
}

static void parseCommandLine(ProgramOptions & optsOut, const int argc, const char * argv[])
{
    // First thing must be the font file name:
    optsOut.fntFileName = argv[1];

    // Check for a flag in the wrong place/empty string...
    if (optsOut.fntFileName.empty() || optsOut.fntFileName[0] == '-')
    {
        error("Invalid filename \"" + optsOut.fntFileName + "\".");
    }

    const int idxBitmapFile = argc >= 3 && !isCmdFlag(argv[2]) ? 2 : -1;
    const int idxOutputFile = argc >= 4 && !isCmdFlag(argv[3]) ? 3 : -1;
    const int idxFontName   = argc >= 5 && !isCmdFlag(argv[4]) ? 4 : -1;

    // Get the user provided names or use defaults:
    if (idxBitmapFile >= 0)
    {
        optsOut.bitmapFileName = argv[idxBitmapFile];
        // Else if empty, get from the FNT
    }

    if (idxOutputFile >= 0)
    {
        optsOut.outputFileName = argv[idxOutputFile];
    }
    else
    {
        optsOut.outputFileName = removeFilenameExtension(optsOut.fntFileName) + ".h";
    }

    if (idxFontName >= 0)
    {
        optsOut.fontName = argv[idxFontName];
    }
    else
    {
        optsOut.fontName = removeFilenameExtension(optsOut.fntFileName);

        // We don't want funky characters in the array names. Only letters, numbers and underscore.
        std::replace_if(std::begin(optsOut.fontName), std::end(optsOut.fontName),
                        [](char c) { return !std::isalnum(c) && c != '_'; }, '_');
    }

    // Whatever is left must be optional flags:
    const int start = std::max({ 2, idxBitmapFile, idxOutputFile, idxFontName });
    for (int i = start; i < argc; ++i)
    {
        if (!isCmdFlag(argv[i]))
        {
            continue;
        }

        if (hasCmdFlag(argv[i], "-v", "--verbose"))
        {
            optsOut.verbose = true;
        }
        else if (hasCmdFlag(argv[i], "-c", "--compress"))
        {
            optsOut.compressBitmap = true;
        }
        else if (hasCmdFlag(argv[i], "-s", "--static"))
        {
            optsOut.staticStorage = true;
        }
        else if (hasCmdFlag(argv[i], "-S", "--structs"))
        {
            optsOut.outputStructs = true;
        }
        else if (hasCmdFlag(argv[i], "-x", "--rgba"))
        {
            optsOut.rgbaBitmap = true;
        }
        else if (startsWith(argv[i], "--align"))
        {
            int alignN = 0;
            if (std::sscanf(argv[i], "--align=%d", &alignN) == 1)
            {
                optsOut.alignmentAmount = alignN;
            }
            else
            {
                error("Bad '--align' flag! Expected a number after '=', e.g.: '--align=16'");
            }
        }
    }
}

static void printHelpText(const char * progName)
{
    std::cout << "\n"
        << "Usage:\n"
        << " $ " << progName << " <fnt-file> [bitmap-file] [output-file] [font-name] [options]\n"
        << " Converts a text FNT file and associated glyph bitmap to C/C++ code that can be embedded into an application.\n"
        << " Parameters are:\n"
        << "  (req) fnt-file     Name of a .FNT file with the glyph info. The Hiero tool can be used to generate those from a .TTF typeface.\n"
        << "  (opt) bitmap-file  Name of the image with the glyphs. If not provided, use the filename found inside the FNT file.\n"
        << "  (opt) output-file  Name of the .c/.h file to write, including extension. If not provided, use <fnt-file>.h\n"
        << "  (opt) font-name    Name of the typeface that will be used to name the data arrays. If omitted, use <fnt-file>.\n"
        << " Options are:\n"
        << "  -h, --help         Prints this message and exits.\n"
        << "  -v, --verbose      Prints some verbose stats about the program execution.\n"
        << "  -c, --compress     Compresses the output glyph bitmap array with simple RLE encoding.\n"
        << "  -s, --static       Qualify the C arrays with the 'static' storage class.\n"
        << "  -S, --structs      Also outputs the 'FontChar/FontCharSet' structures at the beginning of the file.\n"
        << "  -x, --rgba         Write the glyph bitmap in RGBA format. Default is 1-byte-per-pixel grayscale.\n"
        << "  --align=N          Applies GCC/Clang '__attribute__((aligned(N)))' extension to the output arrays.\n"
        << "\n"
        << "Created by Guilherme R. Lampert, " << __DATE__ << ".\n";
}

// ========================================================
// main():
// ========================================================

int main(int argc, const char * argv[])
{
    try
    {
        if (argc <= 1) // Just the prog name?
        {
            printHelpText(argv[0]);
            return EXIT_FAILURE;
        }

        // Check for "font-tool -h" or "font-tool --help"
        if (isCmdFlag(argv[1]) && hasCmdFlag(argv[1], "-h", "--help"))
        {
            printHelpText(argv[0]);
            return EXIT_SUCCESS;
        }

        ProgramOptions opts;
        parseCommandLine(opts, argc, argv);

        if (opts.verbose)
        {
            std::cout << std::boolalpha;
            std::cout << "> Inputs:\n";
            std::cout << "FNT file...........: " << opts.fntFileName     << "\n";
            std::cout << "Bitmap file........: " << (opts.bitmapFileName.empty() ? "<from FNT>" : opts.bitmapFileName) << "\n";
            std::cout << "Output file........: " << opts.outputFileName  << "\n";
            std::cout << "Font name..........: " << opts.fontName        << "\n";
            std::cout << "RLE encode.........: " << opts.compressBitmap  << "\n";
            std::cout << "Static arrays......: " << opts.staticStorage   << "\n";
            std::cout << "Write structs......: " << opts.outputStructs   << "\n";
            std::cout << "Force RGBA bitmap..: " << opts.rgbaBitmap      << "\n";
            std::cout << "Alignment..........: " << opts.alignmentAmount << "\n";
        }

        runFontTool(opts, argc, argv);
        return EXIT_SUCCESS;
    }
    catch (...)
    {
        std::cerr << "Aborting..." << std::endl;
        return EXIT_FAILURE;
    }
}
