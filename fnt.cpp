
// ================================================================================================
// -*- C++ -*-
// File: fnt.cpp
// Author: Guilherme R. Lampert
// Created on: 18/02/16
// Brief: FNT file parsing.
//
// This source code is released under the MIT license.
// See the accompanying LICENSE file for details.
// ================================================================================================

#include "fnt.hpp"
#include <cassert>
#include <cstdio>

// ========================================================
// Text FNT file processing:
// ========================================================

struct TextFntParser
{
    FILE * fntFile         = nullptr;
    FontChar * currentChar = nullptr;
    bool prevTokenWasChar  = false;
    int largestHeight      = 0;
    int largestWidth       = 0;
    int lineNum            = 0;

    enum { LineCharsMax = 4096 };
    char lineBuffer[LineCharsMax];

    void closeFile()
    {
        if (fntFile != nullptr)
        {
            std::fclose(fntFile);
            fntFile = nullptr;
        }
    }

    // Ensures the file is always closed, even if an exception is thrown.
    ~TextFntParser() { closeFile(); }
};

static int scanInt(TextFntParser & parser, const char * token)
{
    char * endPtr = nullptr;
    int result = static_cast<int>(std::strtol(token, &endPtr, 0));

    if (endPtr == nullptr || endPtr == token)
    {
        error("FNT parse error at line " + std::to_string(parser.lineNum) +
              ": Expected number instead of token '" + std::string(token) + "'!");
    }

    return result;
}

static void handleFntToken(TextFntParser & parser, const char * token,
                           FontCharSet & charSetOut, std::string * fntBitmapFile)
{
    assert(token != nullptr);
    if (std::strcmp(token, "char") == 0)
    {
        parser.prevTokenWasChar = true;
        return;
    }

    if (strStartsWith(token, "base="))
    {
        charSetOut.charBaseHeight = scanInt(parser, token + 5);
    }
    else if (strStartsWith(token, "file="))
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
    // Need prevTokenWasChar because there's "char id=" and "page id=".
    else if (strStartsWith(token, "id=") && parser.prevTokenWasChar)
    {
        const int charIndex = scanInt(parser, token + 3);
        if (charIndex < 0 || charIndex >= FontCharSet::MaxChars)
        {
            error("FNT line " + std::to_string(parser.lineNum) + ": Char index out-of-range!");
        }
        parser.currentChar = &charSetOut.chars[charIndex];
        charSetOut.charCount++;
    }
    else if (strStartsWith(token, "x="))
    {
        assert(parser.currentChar != nullptr);
        parser.currentChar->x = scanInt(parser, token + 2);
    }
    else if (strStartsWith(token, "y="))
    {
        assert(parser.currentChar != nullptr);
        parser.currentChar->y = scanInt(parser, token + 2);
    }
    else if (strStartsWith(token, "height="))
    {
        const int height = scanInt(parser, token + 7);
        if (height > parser.largestHeight)
        {
            parser.largestHeight = height;
        }
    }
    else if (strStartsWith(token, "xadvance="))
    {
        const int width = scanInt(parser, token + 9);
        if (width > parser.largestWidth)
        {
            parser.largestWidth = width;
        }
    }

    charSetOut.charWidth    = parser.largestWidth;
    charSetOut.charHeight   = parser.largestHeight;
    parser.prevTokenWasChar = false;
}

void parseTextFntFile(const std::string & filename, FontCharSet & charSetOut, std::string * fntBitmapFile)
{
    TextFntParser parser;

    #ifdef _MSC_VER
    fopen_s(&parser.fntFile, filename.c_str(), "rt");
    #else // !_MSC_VER
    parser.fntFile = std::fopen(filename.c_str(), "rt");
    #endif // _MSC_VER

    if (parser.fntFile == nullptr)
    {
        error("Unable to open FNT file \"" + filename + "\" for reading!");
    }

    while (!std::feof(parser.fntFile))
    {
        char * line = std::fgets(parser.lineBuffer, TextFntParser::LineCharsMax, parser.fntFile);
        parser.lineNum++;

        if (line == nullptr)
        {
            break;
        }

        const char * token = std::strtok(line, " \t\n\r"); // Split each line by whitespace.
        while (token != nullptr)
        {
            handleFntToken(parser, token, charSetOut, fntBitmapFile);
            token = std::strtok(nullptr, " \t\n\r");
        }
    }

    // TextFntParser destructor closes the file.
}
