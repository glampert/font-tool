
// ================================================================================================
// -*- C++ -*-
// File: data_writer.cpp
// Author: Guilherme R. Lampert
// Created on: 18/02/16
// Brief: Writes the output C/C++ data arrays to file.
//
// This source code is released under the MIT license.
// See the accompanying LICENSE file for details.
// ================================================================================================

#include "data_writer.hpp"

static std::string toEscapedHexaString(const std::uint8_t * data, const int dataSizeBytes,
                                       const int maxColumns = 88, const int padding = 0)
{
    if (data == nullptr)
    {
        error("toEscapedHexaString: Null data pointer!");
    }
    if ((maxColumns % 4) != 0 || (padding % 2) != 0)
    {
        error("toEscapedHexaString: Invalid maxColumns or padding!");
    }

    int column = 0;
    char hexaStr[64] = {'\0'};
    std::string result{ "\"" };

    for (int i = 0; i < dataSizeBytes; ++i, ++data)
    {
        std::snprintf(hexaStr, sizeof(hexaStr), "\\x%02X", static_cast<unsigned>(*data));
        result += hexaStr;
        column += 4;

        if (column == maxColumns)
        {
            if (i != (dataSizeBytes - 1)) // If not the last iteration
            {
                result += "\"\n\"";
            }
            column = 0;
        }
    }

    // Add zero padding at the end to ensure the data size
    // is evenly divisible by the given padding value.
    if (padding > 0)
    {
        for (int i = dataSizeBytes; (i % padding) != 0; ++i)
        {
            result += "\\x00";
            column += 4;

            if (column == maxColumns)
            {
                if ((i + 1) % padding) // If not the last iteration
                {
                    result += "\"\n\"";
                }
                column = 0;
            }
        }
    }

    result += "\"";
    return result;
}

// ========================================================
// DataWriter:
// ========================================================

DataWriter::DataWriter(const ProgramOptions & progOptions)
    : opts{ progOptions }
    , outFile{ nullptr }
{
    verbosePrint(opts, "> Creating output file...");

    #ifdef _MSC_VER
    fopen_s(&outFile, opts.outputFileName.c_str(), "wt");
    #else // !_MSC_VER
    outFile = std::fopen(opts.outputFileName.c_str(), "wt");
    #endif // _MSC_VER

    if (outFile == nullptr)
    {
        error("Unable to open file \"" + opts.outputFileName + "\" for writing!");
    }
}

DataWriter::~DataWriter()
{
    if (outFile != nullptr)
    {
        std::fclose(outFile);
    }
}

void DataWriter::write(const ByteBuffer & bitmapData, const FontCharSet & charSet)
{
    verbosePrint(opts, "> Writing output file...");

    writeComments();
    writeStructures();
    writeBitmapArray(bitmapData);
    writeCharSet(charSet);

    verbosePrint(opts, "> Done!");
}

void DataWriter::writeComments()
{
    std::fprintf(outFile, "\n/*\n");
    std::fprintf(outFile, " * File generated from font '%s' by font-tool.\n", opts.fontFaceName.c_str());
    std::fprintf(outFile, " * Command line:%s\n", opts.cmdLine.c_str());
    std::fprintf(outFile, " */\n");
}

void DataWriter::writeStructures()
{
    if (!opts.outputStructs)
    {
        return;
    }

    const char * xyTypeStr;
    const char * bitmapTypeStr;

    if (opts.stdTypes)
    {
        xyTypeStr = "std::uint16_t";
        bitmapTypeStr = "std::uint8_t";
        std::fprintf(outFile, "\n#include <cstdint>\n"); // You get the include for free, like it or not :P
    }
    else
    {
        xyTypeStr = "unsigned short";
        bitmapTypeStr = "unsigned char";
    }

    std::fprintf(outFile, "\n");
    std::fprintf(outFile, "struct FontChar\n");
    std::fprintf(outFile, "{\n");
    std::fprintf(outFile, "    %s x;\n", xyTypeStr);
    std::fprintf(outFile, "    %s y;\n", xyTypeStr);
    std::fprintf(outFile, "};\n");
    std::fprintf(outFile, "\n");
    std::fprintf(outFile, "struct FontCharSet\n");
    std::fprintf(outFile, "{\n");
    std::fprintf(outFile, "    enum { MaxChars = 256 };\n");
    std::fprintf(outFile, "    const %s * bitmap;\n", bitmapTypeStr);
    std::fprintf(outFile, "    int bitmapWidth;\n");
    std::fprintf(outFile, "    int bitmapHeight;\n");
    std::fprintf(outFile, "    int bitmapColorChannels;\n");
    std::fprintf(outFile, "    int bitmapDecompressSize;\n");
    std::fprintf(outFile, "    int charBaseHeight;\n");
    std::fprintf(outFile, "    int charWidth;\n");
    std::fprintf(outFile, "    int charHeight;\n");
    std::fprintf(outFile, "    int charCount;\n");
    std::fprintf(outFile, "    FontChar chars[MaxChars];\n");
    std::fprintf(outFile, "};\n");
}

void DataWriter::writeBitmapArray(const ByteBuffer & bitmapData)
{
    const auto arrayNameStr  = getArrayName();
    const auto storageStr    = getStorageQualifiers();
    const auto alignStr      = getAlignDirective();
    const auto memSizeStr    = formatMemoryUnit(bitmapData.size(), true); // For a code comment.
    const auto bitmapTypeStr = (opts.stdTypes ? "std::uint8_t" : "unsigned char");

    std::fprintf(outFile, "\n%sint font%sBitmapSizeBytes = %zu;\n",
                 storageStr.c_str(), arrayNameStr.c_str(), bitmapData.size());

    std::fprintf(outFile, "%s%s font%sBitmap[] %s=", storageStr.c_str(),
                 bitmapTypeStr, arrayNameStr.c_str(), alignStr.c_str());

    if (opts.hexadecimalStr) // Escaped hexadecimal C string:
    {
        const auto hexaStr = toEscapedHexaString(bitmapData.data(), bitmapData.size());
        std::fprintf(outFile, " // ~%s\n%s;\n", memSizeStr.c_str(), hexaStr.c_str());
    }
    else // "Traditional" array of comma-separated hexadecimal bytes:
    {
        std::fprintf(outFile, " { // ~%s\n  ", memSizeStr.c_str());

        const std::size_t maxColumns = 15;
        const std::size_t dataSize = bitmapData.size();

        for (std::size_t i = 0, column = 1; i < dataSize; ++i, ++column)
        {
            std::fprintf(outFile, "0x%02X", bitmapData[i]);

            if (i != (dataSize - 1))
            {
                std::fprintf(outFile, ", ");
            }
            if (column == maxColumns)
            {
                std::fprintf(outFile, "\n  ");
                column = 0;
            }
        }

        std::fprintf(outFile, "\n};\n");
    }
}

void DataWriter::writeCharSet(const FontCharSet & charSet)
{
    const auto arrayNameStr = getArrayName();
    const auto storageStr   = getStorageQualifiers();
    const auto alignStr     = getAlignDirective();

    std::fprintf(outFile, "\n%sFontCharSet font%sCharSet %s= {\n",
                 storageStr.c_str(), arrayNameStr.c_str(), alignStr.c_str());

    std::fprintf(outFile, "  /* bitmap               = */ font%sBitmap,\n", arrayNameStr.c_str());
    std::fprintf(outFile, "  /* bitmapWidth          = */ %d,\n", charSet.bitmapWidth);
    std::fprintf(outFile, "  /* bitmapHeight         = */ %d,\n", charSet.bitmapHeight);
    std::fprintf(outFile, "  /* bitmapColorChannels  = */ %d,\n", charSet.bitmapColorChannels);
    std::fprintf(outFile, "  /* bitmapDecompressSize = */ %d,\n", charSet.bitmapDecompressSize);
    std::fprintf(outFile, "  /* charBaseHeight       = */ %d,\n", charSet.charBaseHeight);
    std::fprintf(outFile, "  /* charWidth            = */ %d,\n", charSet.charWidth);
    std::fprintf(outFile, "  /* charHeight           = */ %d,\n", charSet.charHeight);
    std::fprintf(outFile, "  /* charCount            = */ %d,\n", charSet.charCount);
    std::fprintf(outFile, "  {\n");

    for (int i = 0, j = 0; i < FontCharSet::MaxChars; ++i)
    {
        if (j == 0)
        {
            std::fprintf(outFile, "   ");
        }

        const FontChar chr = charSet.chars[i];
        std::fprintf(outFile, "{ %3u, %3u }", chr.x, chr.y);

        // 4 char defs per line.
        if (j == (4 - 1))
        {
            j = 0;
            if (i != (FontCharSet::MaxChars - 1)) // Not the last iteration?
            {
                std::fprintf(outFile, ",\n");
            }
        }
        else
        {
            ++j;
            std::fprintf(outFile, ", ");
        }
    }

    std::fprintf(outFile, "\n  }\n");
    std::fprintf(outFile, "};\n\n");
}

std::string DataWriter::getArrayName() const
{
    auto arrayNameStr = opts.fontFaceName;

    // Capitalize first letter for CamelCase name:
    arrayNameStr[0] = std::toupper(arrayNameStr[0]);
    return arrayNameStr;
}

std::string DataWriter::getAlignDirective() const
{
    std::string alignStr;

    // NOTE: Currently GCC/Clang-style alignment only.
    if (opts.alignmentAmount > 0)
    {
        alignStr += "__attribute__((aligned(" + std::to_string(opts.alignmentAmount) + "))) ";
    }

    return alignStr;
}

std::string DataWriter::getStorageQualifiers() const
{
    std::string storageStr;

    // Static or const qualifiers:
    if (opts.staticStorage)
    {
        storageStr += "static ";
    }
    if (!opts.mutableData)
    {
        storageStr += "const ";
    }

    return storageStr;
}
