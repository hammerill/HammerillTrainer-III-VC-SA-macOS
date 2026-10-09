// Implemented by the embedded_files.cpp that cmake/EmbedFiles.cmake generates.
#pragma once

#include <cstddef>

struct EmbeddedFile
{
    const char *name;
    const unsigned char *data;
    size_t size;
};

extern const EmbeddedFile kEmbeddedFiles[];
extern const size_t kEmbeddedFileCount;
