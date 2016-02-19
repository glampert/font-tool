
// ================================================================================================
// -*- C++ -*-
// File: utils.hpp
// Author: Guilherme R. Lampert
// Created on: 18/02/16
// Brief: Miscellaneous utilities.
//
// This source code is released under the MIT license.
// See the accompanying LICENSE file for details.
// ================================================================================================

#ifndef UTILS_HPP
#define UTILS_HPP

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <memory>
#include <vector>
#include <string>
#include <stdexcept>

//
// Constants / Type aliases:
//

struct MemUnit
{
    // Want the scoping but also want the implicit int conversions...
    enum
    {
        Kilobyte = 1024,
        Megabyte = 1024 * Kilobyte,
        Gigabyte = 1024 * Megabyte
    };
};

enum class Encoding
{
    None,
    RLE,
    LZW,
    Huffman
};

class FontToolError final
    : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

struct ProgramOptions;
using ByteBuffer = std::vector<std::uint8_t>;

//
// Application-wide helper functions:
//

void error(const std::string & message);
void verbosePrint(const ProgramOptions & opts, const std::string & message);
bool strStartsWith(const char * str, const char * prefix);
std::string formatMemoryUnit(std::size_t sizeBytes, int abbreviated = true);
std::string removeFilenameExtension(const std::string & filename);

// Font bitmap image loader (performs the grayscale conversion if specified).
ByteBuffer loadFontBitmap(const std::string & filename, bool forceGrayscale,
                          int & widthOut, int & heightOut, int & numChannelsOut);

//
// Command line handling:
//

struct ProgramOptions
{
    std::string cmdLine;
    std::string fntFileName;
    std::string bitmapFileName;
    std::string outputFileName;
    std::string fontFaceName;

    bool verbose        = false;
    bool compressBitmap = false;
    bool rgbaBitmap     = false;
    bool staticStorage  = false;
    bool mutableData    = false;
    bool outputStructs  = false;
    bool stdTypes       = false;
    bool hexadecimalStr = false;
    int alignmentAmount = 0;
    Encoding encoding   = Encoding::RLE;
};

bool isCmdFlag(const char * arg);
bool isHelpRun(const int argc, const char * argv[]);
bool hasCmdFlag(const char * test, const char * shortForm, const char * longForm);
void printHelpText(const char * progName);
ProgramOptions parseCmdLine(int argc, const char * argv[]);

#endif // UTILS_HPP
