
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

#define RLE_IMPLEMENTATION
#include "extern/compression/rle.hpp"

#define LZW_IMPLEMENTATION
#include "extern/compression/lzw.hpp"

#define HUFFMAN_IMPLEMENTATION
#include "extern/compression/huffman.hpp"

// ========================================================
// NoOpCompressor:
// ========================================================

class NoOpCompressor final
    : public Compressor
{
public:
    // Return the input unchanged.
    ByteBuffer compress(const ByteBuffer & uncompressed) override { return uncompressed; }
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
        // RLE might make things bigger, so make the compress buffer the double.
        ByteBuffer compressed(uncompressed.size() * 2);

        // Compress it:
        const int compressedSize = rle::easyEncode(uncompressed.data(), uncompressed.size(),
                                                   compressed.data(),   compressed.size());

        // Error / no compression?
        if (compressedSize <= 0)
        {
            compressed.clear();
        }
        // If we compressed or got the same size, ignore the extra overhead in the output buffer:
        else if (static_cast<std::size_t>(compressedSize) <= uncompressed.size())
        {
            compressed.resize(compressedSize);
        }
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
        int compressedSizeBits  = 0;
        int compressedSizeBytes = 0;
        lzw::UByte * compressedDataPtr = nullptr;

        // Compress:
        lzw::easyEncode(uncompressed.data(), uncompressed.size(),
                        &compressedDataPtr, &compressedSizeBytes,
                        &compressedSizeBits);

        ByteBuffer compressedBuffer((sizeof(std::uint32_t) * 2) + compressedSizeBytes);

        // Append the compressed size in bytes and bits to
        // the output buffer. We will need those for decompression.
        auto bitsPtr = reinterpret_cast<std::uint32_t *>(compressedBuffer.data());
        *bitsPtr++ = compressedSizeBytes;
        *bitsPtr++ = compressedSizeBits;

        // Append the rest of the compressed bit stream:
        std::memcpy(bitsPtr, compressedDataPtr, compressedSizeBytes);

        // Done! We can now free the LZW buffer.
        LZW_MFREE(compressedDataPtr);
        return compressedBuffer;
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
        int compressedSizeBits  = 0;
        int compressedSizeBytes = 0;
        huffman::UByte * compressedDataPtr = nullptr;

        // Compress:
        huffman::easyEncode(uncompressed.data(), uncompressed.size(),
                            &compressedDataPtr, &compressedSizeBytes,
                            &compressedSizeBits);

        ByteBuffer compressedBuffer((sizeof(std::uint32_t) * 2) + compressedSizeBytes);

        // Append the compressed size in bytes and bits to
        // the output buffer. We will need those for decompression.
        auto bitsPtr = reinterpret_cast<std::uint32_t *>(compressedBuffer.data());
        *bitsPtr++ = compressedSizeBytes;
        *bitsPtr++ = compressedSizeBits;

        // Append the rest of the compressed bit stream:
        std::memcpy(bitsPtr, compressedDataPtr, compressedSizeBytes);

        // Done! We can now free the Huffman buffer.
        HUFFMAN_MFREE(compressedDataPtr);
        return compressedBuffer;
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
