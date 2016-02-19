
// ================================================================================================
// -*- C++ -*-
// File: utils.cpp
// Author: Guilherme R. Lampert
// Created on: 18/02/16
// Brief: Miscellaneous utilities.
//
// This source code is released under the MIT license.
// See the accompanying LICENSE file for details.
// ================================================================================================

#include "utils.hpp"
#include <iostream>

// ========================================================
// Assorted helper functions:
// ========================================================

void error(const std::string & message)
{
    throw FontToolError{ message };
}

void verbosePrint(const ProgramOptions & opts, const std::string & message)
{
    if (opts.verbose)
    {
        std::cout << message << "\n";
    }
}

bool strStartsWith(const char * str, const char * prefix)
{
    const auto strLen    = std::strlen(str);
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

std::string formatMemoryUnit(const std::size_t sizeBytes, const int abbreviated)
{
    double adjustedSize;
    const char * memUnitStr;
    char strBuf[256] = {'\0'};
    char fmtBuf[256] = {'\0'};

    if (sizeBytes < MemUnit::Kilobyte)
    {
        memUnitStr = abbreviated ? "B" : "Bytes";
        adjustedSize = static_cast<double>(sizeBytes);
    }
    else if (sizeBytes < MemUnit::Megabyte)
    {
        memUnitStr = abbreviated ? "KB" : "Kilobytes";
        adjustedSize = sizeBytes / 1024.0;
    }
    else if (sizeBytes < MemUnit::Gigabyte)
    {
        memUnitStr = abbreviated ? "MB" : "Megabytes";
        adjustedSize = sizeBytes / 1024.0 / 1024.0;
    }
    else
    {
        memUnitStr = abbreviated ? "GB" : "Gigabytes";
        adjustedSize = sizeBytes / 1024.0 / 1024.0 / 1024.0;
    }

    // We only care about the first 2 decimal digits.
    std::snprintf(strBuf, sizeof(strBuf), "%.2f", adjustedSize);

    // Remove trailing zeros if no significant decimal digits:
    for (char * p = strBuf; *p != '\0'; ++p)
    {
        if (*p == '.')
        {
            // Find the end of the string:
            while (*++p) { }

            // Remove trailing zeros:
            while (*--p == '0')
            {
                *p = '\0';
            }

            // If the dot was left alone at the end, remove it too.
            if (*p == '.')
            {
                *p = '\0';
            }
            break;
        }
    }

    std::snprintf(fmtBuf, sizeof(fmtBuf), "%s %s", strBuf, memUnitStr);
    return fmtBuf;
}

std::string removeFilenameExtension(const std::string & filename)
{
	const auto lastDot = filename.find_last_of('.');
	if (lastDot == std::string::npos)
	{
		return filename;
	}
	return filename.substr(0, lastDot);
}

// ========================================================
// Command line handling:
// ========================================================

bool isCmdFlag(const char * arg)
{
    return arg != nullptr && *arg == '-';
}

bool isHelpRun(const int argc, const char * argv[])
{
    if (argc < 2)
    {
        return false;
    }
    return isCmdFlag(argv[1]) && hasCmdFlag(argv[1], "-h", "--help");
}

bool hasCmdFlag(const char * test, const char * shortForm, const char * longForm)
{
    return std::strcmp(test, shortForm) == 0 ||
           std::strcmp(test, longForm)  == 0;
}

void printHelpText(const char * progName)
{
    std::cout << "\n"
      << "Usage:\n"
      << " $ " << progName << " <fnt-file> [bitmap-file] [output-file] [font-name] [options]\n"
      << " Converts a text FNT file and associated glyph bitmap to C/C++ code that can be embedded into an application.\n"
      << " Parameters are:\n"
      << "  (req) fnt-file     Name of a .FNT file with the glyph info. The Hiero tool can be used to generate those from a TTF typeface.\n"
      << "  (opt) bitmap-file  Name of the image with the glyphs. If not provided, use the filename found inside the FNT file.\n"
      << "  (opt) output-file  Name of the .c/.h file to write, including extension. If not provided, use <fnt-file>.h\n"
      << "  (opt) font-name    Name of the typeface that will be used to name the data arrays. If omitted, use <fnt-file>.\n"
      << " Options are:\n"
      << "  -h, --help         Prints this message and exits.\n"
      << "  -v, --verbose      Prints some verbose stats about the program execution.\n"
      << "  -c, --compress     Compresses the output glyph bitmap array with RLE encoding by default.\n"
      << "  -s, --static       Qualify the C/C++ arrays with the 'static' storage class.\n"
      << "  -m, --mutable      Allow the output data to be mutable, i.e. omit the 'const' qualifier.\n"
      << "  -S, --structs      Also outputs the 'FontChar/FontCharSet' structures at the beginning of the file.\n"
      << "  -T, --stdtypes     Use Standard C++ types like std::uint8_t and std::uint16_t in the output structs/arrays.\n"
      << "  -H, --hex          Write the glyph bitmap data as an escaped hexadecimal string. The default is an array of hexa unsigned bytes.\n"
      << "  -x, --rgba         Write the glyph bitmap in RGBA format. Default is 1-byte-per-pixel grayscale.\n"
      << "  --align=N          Applies GCC/Clang __attribute__((aligned(N))) extension to the output arrays.\n"
      << "  --encoding=method  If combined with -c/--compress, specifies the encoding to use. Methods are: rle,lzw,huff. Defaults to rle.\n"
      << "\n"
      << "Created by Guilherme R. Lampert, " << __DATE__ << ".\n";
}

ProgramOptions parseCmdLine(const int argc, const char * argv[])
{
    ProgramOptions optsOut;

    // Save the command line as a single string for verbose printing:
    for (int i = 1; i < argc; ++i)
    {
        optsOut.cmdLine += " ";
        optsOut.cmdLine += argv[i];
    }

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
        // Else if empty, get from the FNT.
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
        optsOut.fontFaceName = argv[idxFontName];
    }
    else
    {
        optsOut.fontFaceName = removeFilenameExtension(optsOut.fntFileName);

        // We don't want funky characters in the array names. Only letters, numbers and underscore.
        std::replace_if(std::begin(optsOut.fontFaceName), std::end(optsOut.fontFaceName),
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
        else if (hasCmdFlag(argv[i], "-m", "--mutable"))
        {
            optsOut.mutableData = true;
        }
        else if (hasCmdFlag(argv[i], "-S", "--structs"))
        {
            optsOut.outputStructs = true;
        }
        else if (hasCmdFlag(argv[i], "-T", "--stdtypes"))
        {
            optsOut.stdTypes = true;
        }
        else if (hasCmdFlag(argv[i], "-H", "--hex"))
        {
            optsOut.hexadecimalStr = true;
        }
        else if (hasCmdFlag(argv[i], "-x", "--rgba"))
        {
            optsOut.rgbaBitmap = true;
        }
        else if (strStartsWith(argv[i], "--align"))
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
        else if (strStartsWith(argv[i], "--encoding"))
        {
            char encoding[128] = {'\0'};
            if (std::sscanf(argv[i], "--encoding=%s", encoding) == 1)
            {
                if (std::strcmp(encoding, "rle") == 0)
                {
                    optsOut.encoding = Encoding::RLE;
                }
                else if (std::strcmp(encoding, "lzw") == 0)
                {
                    optsOut.encoding = Encoding::LZW;
                }
                else if (std::strcmp(encoding, "huff") == 0)
                {
                    optsOut.encoding = Encoding::Huffman;
                }
                else
                {
                    error("Unknown encoding method \"" + std::string(encoding) + "\".");
                }
            }
            else
            {
                error("Bad '--encoding' flag! Expected rle, lzw or huff after '='.");
            }
        }
    }

    if (!optsOut.compressBitmap)
    {
        optsOut.encoding = Encoding::None;
    }

    if (optsOut.verbose)
    {
        const char * encodings[] = { "None", "RLE", "LZW", "Huffman" };

        std::cout << std::boolalpha;
        std::cout << "> Inputs:\n";
        std::cout << "FNT file...........: " << optsOut.fntFileName << "\n";
        std::cout << "Bitmap file........: " << (optsOut.bitmapFileName.empty() ? "<from FNT>" : optsOut.bitmapFileName) << "\n";
        std::cout << "Output file........: " << optsOut.outputFileName << "\n";
        std::cout << "Font name..........: " << optsOut.fontFaceName << "\n";
        std::cout << "Encode the bitmap..: " << optsOut.compressBitmap << "\n";
        std::cout << "Static arrays......: " << optsOut.staticStorage << "\n";
        std::cout << "Mutable arrays.....: " << optsOut.mutableData << "\n";
        std::cout << "Write structs......: " << optsOut.outputStructs << "\n";
        std::cout << "Standard C++ types.: " << optsOut.stdTypes << "\n";
        std::cout << "Escaped hex string.: " << optsOut.hexadecimalStr << "\n";
        std::cout << "Force RGBA bitmap..: " << optsOut.rgbaBitmap << "\n";
        std::cout << "Alignment..........: " << optsOut.alignmentAmount << "\n";
        std::cout << "Encoding...........: " << encodings[static_cast<int>(optsOut.encoding)] << "\n";
    }

    return optsOut;
}

// ========================================================
// Image loading & decompression via STB Image:
// ========================================================

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_TGA
#define STBI_ONLY_JPEG
#include "extern/stb/stb_image.h"

#ifdef DEBUG_DUMP_FNT_BITMAP
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "extern/stb/stb_image_write.h"
#endif // DEBUG_DUMP_FNT_BITMAP

constexpr float byteToFloat(const std::uint8_t b) { return static_cast<float>(b) * (1.0f / 255.0f); }
constexpr std::uint8_t floatToByte(const float f) { return static_cast<std::uint8_t>(f * 255.0f);   }

static void rgbaToGray(std::uint8_t * dest, const std::uint8_t * src, const int width, const int height)
{
    const int pixelCount = width * height;
    for (int i = 0; i < pixelCount; ++i)
    {
        const float inR = byteToFloat(*src++);
        const float inG = byteToFloat(*src++);
        const float inB = byteToFloat(*src++);
        const float inA = byteToFloat(*src++);

        // This is the "luminosity" grayscale conversion method.
        const float gray = (0.21f * inR) + (0.72f * inG) + (0.07f * inB);
        *dest++ = floatToByte(gray * inA);
    }
}

ByteBuffer loadFontBitmap(const std::string & filename, const bool forceGrayscale,
                          int & widthOut, int & heightOut, int & numChannelsOut)
{
    int x = 0;
    int y = 0;
    int c = 0;
    std::uint8_t * imgData = stbi_load(filename.c_str(), &x, &y, &c, 4); // Load as RGBA

    if (imgData == nullptr)
    {
        error("Unable to load image from \"" + filename + "\": "
              + std::string(stbi_failure_reason()));
    }

    if (x <= 0 || y <= 0 || c <= 0)
    {
        stbi_image_free(imgData);
        error("Unable to load image from \"" + filename + "\": Bad channels/dimensions.");
    }

    widthOut  = x;
    heightOut = y;
    numChannelsOut = (forceGrayscale ? 1 : 4);
    ByteBuffer bitmap(widthOut * heightOut * numChannelsOut);

    if (forceGrayscale)
    {
        rgbaToGray(bitmap.data(), imgData, widthOut, heightOut);
    }
    else
    {
        std::memcpy(bitmap.data(), imgData, bitmap.size());
    }

    // Write the bitmap back to a TGA image file.
    // This is useful to check that our grayscale
    // conversion is working as intended.
    #ifdef DEBUG_DUMP_FNT_BITMAP
    stbi_write_tga("font_bitmap_out.tga", widthOut, heightOut, numChannelsOut, bitmap.data());
    #endif // DEBUG_DUMP_FNT_BITMAP

    stbi_image_free(imgData);
    return bitmap;
}
