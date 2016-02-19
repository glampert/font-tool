
// ================================================================================================
// -*- C++ -*-
// File: compressor.cpp
// Author: Guilherme R. Lampert
// Created on: 18/02/16
// Brief: Handles the font bitmap compression.
//
// This source code is released under the MIT license.
// See the accompanying LICENSE file for details.
// ================================================================================================

#include "compressor.hpp"

// ========================================================
// NoOpCompressor:
// ========================================================

class NoOpCompressor final
    : public Compressor
{
public:
    // Return the input unchanged.
    ByteBuffer compress(const ByteBuffer & uncompressed) override { return uncompressed; }
    ByteBuffer decompress(const ByteBuffer & compressed) override { return compressed;   }
};

// ========================================================
// RLECompressor:
// ========================================================

class RLECompressor final
    : public Compressor
{
public:

    ByteBuffer compress(const ByteBuffer & uncompressed) override
    {
        //TODO
        return uncompressed;
    }

    ByteBuffer decompress(const ByteBuffer & compressed) override
    {
        //TODO
        return compressed;
    }
};

// ========================================================
// LZWCompressor:
// ========================================================

class LZWCompressor final
    : public Compressor
{
public:

    ByteBuffer compress(const ByteBuffer & uncompressed) override
    {
        //TODO
        return uncompressed;
    }

    ByteBuffer decompress(const ByteBuffer & compressed) override
    {
        //TODO
        return compressed;
    }
};

// ========================================================
// HuffmanCompressor:
// ========================================================

class HuffmanCompressor final
    : public Compressor
{
public:

    ByteBuffer compress(const ByteBuffer & uncompressed) override
    {
        //TODO
        return uncompressed;
    }

    ByteBuffer decompress(const ByteBuffer & compressed) override
    {
        //TODO
        return compressed;
    }
};

// ========================================================
// Compressor factory:
// ========================================================

std::unique_ptr<Compressor> Compressor::create(const Encoding encoding)
{
    switch (encoding)
    {
    case Encoding::None :
        return std::make_unique<NoOpCompressor>();

    case Encoding::RLE :
        return std::make_unique<RLECompressor>();

    case Encoding::LZW :
        return std::make_unique<LZWCompressor>();

    case Encoding::Huffman :
        return std::make_unique<HuffmanCompressor>();

    default :
        error("Invalid compressor encoding enum!");
    } // switch (encoding)
}

std::string Compressor::getMemorySaved(const ByteBuffer & compressed, const ByteBuffer & uncompressed)
{
    const long diff = uncompressed.size() - compressed.size();
    return formatMemoryUnit(diff >= 0 ? diff : 0);
}

double Compressor::getCompressionRatio(const ByteBuffer & compressed, const ByteBuffer & uncompressed)
{
    return static_cast<double>(uncompressed.size()) / static_cast<double>(compressed.size());
}
