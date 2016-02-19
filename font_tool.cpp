
// ================================================================================================
// -*- C++ -*-
// File: font_tool.cpp
// Author: Guilherme R. Lampert
// Created on: 12/12/15
// Brief: Command line tool that converts a .FNT file + its bitmap to C/C++ code.
//
// This source code is released under the MIT license.
// See the accompanying LICENSE file for details.
// ================================================================================================

#include "fnt.hpp"
#include "compressor.hpp"
#include "data_writer.hpp"

#include <iostream>
#include <utility>

// ========================================================
// compressFontBitmapData():
// ========================================================

static void compressFontBitmapData(ByteBuffer & bitmapData, const int width,
                                   const int height, const ProgramOptions & opts)
{
    auto compressor = Compressor::create(opts.encoding);
    auto compressedBitmapData = compressor->compress(bitmapData);

    // Run again without '-c/--compress'
    if (compressedBitmapData.empty())
    {
        error("Failed to compress the glyph bitmap!");
    }
    else if (compressedBitmapData.size() > bitmapData.size())
    {
        error("Compression would produce a bigger bitmap! Cowardly refusing to compress it...");
    }

    // Print compression stats:
    if (opts.verbose)
    {
        std::cout << "> Compression stats:\n";
        std::cout << "Bitmap dimensions..: " << width << "x" << height << "\n";
        std::cout << "Original size......: " << formatMemoryUnit(bitmapData.size()) << "\n";
        std::cout << "Compressed size....: " << formatMemoryUnit(compressedBitmapData.size()) << "\n";
        std::cout << "Space saved........: " << compressor->getMemorySaved(compressedBitmapData, bitmapData) << "\n";
        std::cout << "Compression ratio..: " << compressor->getCompressionRatio(compressedBitmapData, bitmapData) << "\n";
    }

    // Store new data:
    bitmapData = std::move(compressedBitmapData);
}

// ========================================================
// runFontTool():
// ========================================================

static void runFontTool(const int argc, const char * argv[])
{
    FontCharSet charSet{};
    ProgramOptions opts{ parseCmdLine(argc, argv) };

    // Process the FNT:
    verbosePrint(opts, "> Parsing the FNT file...");
    parseTextFntFile(opts.fntFileName, charSet, (opts.bitmapFileName.empty() ? &opts.bitmapFileName : nullptr));

    // Process the glyph bitmap image:
    int width    = 0;
    int height   = 0;
    int channels = 0;
    verbosePrint(opts, "> Loading the glyph bitmap...");
    auto bitmapData = loadFontBitmap(opts.bitmapFileName, !opts.rgbaBitmap, width, height, channels);

    // Optional compression of the glyph bitmap:
    const int uncompressedSize = static_cast<int>(bitmapData.size());
    if (opts.compressBitmap)
    {
        verbosePrint(opts, "> Attempting to compress the glyph bitmap data...");
        compressFontBitmapData(bitmapData, width, height, opts);
    }

    // Update them from the just loaded image:
    charSet.bitmapWidth          = width;
    charSet.bitmapHeight         = height;
    charSet.bitmapColorChannels  = channels;
    charSet.bitmapDecompressSize = (opts.compressBitmap ? uncompressedSize : 0);

    // Write the C/C++ file and we are done:
    DataWriter dataWriter{ opts };
    dataWriter.write(bitmapData, charSet);
}

// ========================================================
// main():
// ========================================================

int main(int argc, const char * argv[])
{
    try
    {
        // Just the prog name? Not enough.
        if (argc <= 1)
        {
            printHelpText(argv[0]);
            return EXIT_FAILURE;
        }

        // Check for "font-tool -h" or "font-tool --help"
        if (isHelpRun(argc, argv))
        {
            printHelpText(argv[0]);
            return EXIT_SUCCESS;
        }

        runFontTool(argc, argv);
        return EXIT_SUCCESS;
    }
    catch (std::exception & e)
    {
        std::cerr << "Font Tool error: " << e.what() << std::endl;
        std::cerr << "Aborting..." << std::endl;
        return EXIT_FAILURE;
    }
}
