#include <io/FileUtil.hpp>
#include <stdio.h>
#include <stdlib.h>

File::File(const char* path, const char* mode) 
{
    stat(path, &m_fileInfo);
    FILE* fHandle = fopen(path, mode);
    SL_ASSERT_MSG(fHandle != nullptr, "Could not get file handle.\n");
    if (!fHandle)
    {
        // TODO: Exception handler
        exit(EXIT_FAILURE);
    }
    else
    {
        m_fHandle = fHandle;
    }
}

File::~File() 
{
    if (m_fBuffer != nullptr)
    {
        free(m_fBuffer);
        m_fBuffer = nullptr;
    }
    fclose((FILE*)m_fHandle);
    m_fHandle = nullptr;
}

bool File::ReadText(u64 size) 
{
    // SL_ASSERT_MSG(m_fHandle != nullptr, "Could not get file handle.\n");
    // SL_ASSERT_MSG(size > 0, "Incorrect size\n");
    if (m_fHandle != nullptr && size > 0)
    {
        m_fBuffer = (char*)malloc(size);
        size_t newLen = fread(m_fBuffer, sizeof(char), size, (FILE*)m_fHandle);
        if (newLen < size) return false;
        if ( ferror( (FILE*)m_fHandle ) != 0 ) 
        {
            fputs("Error reading file", stderr);
            return false;
        } 
        else 
        {
            m_fBuffer[newLen++] = '\0'; /* Just to be safe. */
            return true;
        }
    }
    return false;
}

i32 File::Size() const
{
    return m_fileInfo.st_size;
}
