#pragma once

#include <Common/defines.hpp>

enum FILE_MODE : u8
{
    BINARY_READ,
    BINARY_WRITE,
    TEXT_READ,
    TEXT_WRITE,
    BINARY_WRITE_APPEND,
    TEXT_WRITE_APPEND,
    BINARY_RW,
    BINARY_RW_APPEND,
    TEXT_RW,
    TEXT_RW_APPEND,
    UNKNOWN
};

struct ParseEntry
{
    u64 vertexCount;
    u64 triangleCount;
};

struct DefaultHeaderEntry
{
    static const char* count;
    static const char* vertex;
    static const char* triangle;
};

inline const char* DefaultHeaderEntry::count    = "count";
inline const char* DefaultHeaderEntry::vertex   = "vertex";
inline const char* DefaultHeaderEntry::triangle = "triangle";

struct File
{
    File(const char* path, FILE_MODE mode);
    File(const File&) = delete;
    File& operator=(const File&) = delete;
    ~File();

    u64 WriteBinary(const char* data, u64 size);
    bool ReadBinary(u64 size);
    u64 WriteText(const char* data, u64 size);
    bool ReadText(u64 size);
    i64 GetDelimitedView(char delimiter, char* str);
    i64 GetLineView(char* str);
    void ParseHeader(const char* delim, ParseEntry* entryOut);
    i32 Size() const;
    void SetBufferPos(u32 value);
    u32  GetBufferPos() const;
public:
    char* mPath = nullptr;
    char* mBuffer = nullptr;
    void* mHandle = nullptr;
    FILE_MODE mFileMode = UNKNOWN;
private:
    struct stat mFileInfo;
    u32 mCurBufferPos = 0u;
};