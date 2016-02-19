
// ================================================================================================
// -*- C++ -*-
// File: data_writer.hpp
// Author: Guilherme R. Lampert
// Created on: 18/02/16
// Brief: Writes the output C/C++ data arrays to file.
//
// This source code is released under the MIT license.
// See the accompanying LICENSE file for details.
// ================================================================================================

#ifndef DATA_WRITER_HPP
#define DATA_WRITER_HPP

#include "utils.hpp"
#include "fnt.hpp"
#include <cstdio>

class DataWriter final
{
public:

    explicit DataWriter(const ProgramOptions & progOptions);
    void write(const ByteBuffer & bitmapData, const FontCharSet & charSet);
    ~DataWriter();

private:

    void writeComments();
    void writeStructures();
    void writeBitmapArray(const ByteBuffer & bitmapData);
    void writeCharSet(const FontCharSet & charSet);

    std::string getArrayName() const;
    std::string getAlignDirective() const;
    std::string getStorageQualifiers() const;

    const ProgramOptions & opts;
    FILE * outFile;
};

#endif // DATA_WRITER_HPP
