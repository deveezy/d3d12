#pragma once

#include <Common/defines.hpp>
#include <sys/stat.h>

struct File
{
    File(const char* path, const char* mode);
    File(const File&) = delete;
    File& operator=(const File&) = delete;
    ~File();

    bool ReadText(u64 size);
    i32 Size() const;

// private:
    void*  m_fHandle = nullptr;
    char*  m_fPath   = nullptr;
    char*  m_fBuffer = nullptr;
    u32    m_fSize = 0;

private:
    struct stat m_fileInfo;
};