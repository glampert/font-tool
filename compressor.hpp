
// ================================================================================================
// -*- C++ -*-
// File: compressor.hpp
// Author: Guilherme R. Lampert
// Created on: 18/02/16
// Brief: Handles the font bitmap compression.
//
// This source code is released under the MIT license.
// See the accompanying LICENSE file for details.
// ================================================================================================

#ifndef COMPRESSOR_HPP
#define COMPRESSOR_HPP

#include "utils.hpp"

class Compressor
{
public:

    // Compressor factory:
    static std::unique_ptr<Compressor> create(Encoding encoding);

    // Compression stats:
    static std::string getMemorySaved(const ByteBuffer & compressed, const ByteBuffer & uncompressed);
    static double getCompressionRatio(const ByteBuffer & compressed, const ByteBuffer & uncompressed);

    // Compressor interface:
    virtual ByteBuffer compress(const ByteBuffer & uncompressed) = 0;
    virtual ByteBuffer decompress(const ByteBuffer & compressed) = 0;
    virtual ~Compressor() = default;
};

#endif // COMPRESSOR_HPP
