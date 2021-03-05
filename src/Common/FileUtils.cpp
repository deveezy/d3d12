#include <Common/FileUtils.hpp>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

void report_assertion_failure(const char *expression, const char *message, const char *file, int line) {
    printf("Assertion Failure: %s message: '%s', in file: %s, line: %d\n",
           expression, message, file, line);
}

void ToUpper(const char* in, char* out)
{
    size_t len = strlen(in);
    for (size_t i = 0; i < len; ++i)
    {
        out[i] = toupper(in[i]);
    }
}

i64 GetNumber(char* p)
{
    i64 val = 0;
	while (*p) 
    { // While there are more characters to process...
		if (isdigit(*p) || ((*p == '-' || *p == '+') && isdigit(*(p + 1))) ) 
        {
			// Found a number
            val *= 10;
			val += strtol(p, &p, 10); // Read number
			//printf("%ld\n", val); // and print it.
		} else 
        {
			// Otherwise, move on to the next character.
			p++;
		}
	}
    return val;
}

i32 FindSubstring(const char *str, const char *substr)
{
    char strOut[1024] = { 0 };
    char substrOut[1024] = { 0 };
    ToUpper(str, strOut);
    ToUpper(substr, substrOut);
    i32 i = 0, j = 0;

    while ((*(strOut + j) != '\0') && (*(substrOut + i) != '\0')) 
    {
        if (*(substrOut + i) != *(strOut + j)) 
        {
            j++;
            i = 0;
        }
        else 
        {
            i++;
            j++;
        }
    }
    if (*(substrOut + i) == '\0')
        return 1;
    else
        return -1;
}

FILE* FileOpen(const char* path, const char* mode)
{
    FILE* fHandle = fopen(path, mode);
    if (!fHandle) 
    {
        SL_ASSERT_MSG(fHandle != nullptr, "Could not open file '%s' with %s mode\n", path, mode);
        exit(EXIT_FAILURE);
    }
    return fHandle;
}

File::File(const char* path, FILE_MODE mode) 
{
    stat(path, &mFileInfo);
    FILE* fHandle = nullptr;
    switch (mode)
    {
        case BINARY_READ         : fHandle = FileOpen(path, "rb");  mFileMode = mode;    break;
        case BINARY_WRITE        : fHandle = FileOpen(path, "wb");  mFileMode = mode;    break;
        case TEXT_READ           : fHandle = FileOpen(path, "rt");  mFileMode = mode;    break;
        case TEXT_WRITE          : fHandle = FileOpen(path, "wt");  mFileMode = mode;    break;
        case BINARY_WRITE_APPEND : fHandle = FileOpen(path, "ab");  mFileMode = mode;    break;
        case TEXT_WRITE_APPEND   : fHandle = FileOpen(path, "at");  mFileMode = mode;    break;
        case BINARY_RW           : fHandle = FileOpen(path, "r+b"); mFileMode = mode;    break;
        case BINARY_RW_APPEND    : fHandle = FileOpen(path, "a+b"); mFileMode = mode;    break;
        case TEXT_RW             : fHandle = FileOpen(path, "r+t"); mFileMode = mode;    break;
        case TEXT_RW_APPEND      : fHandle = FileOpen(path, "a+t"); mFileMode = mode;    break;
        default                  : fHandle = nullptr;               mFileMode = UNKNOWN; break;
    }

    SL_ASSERT_MSG(fHandle != nullptr, "Could not get file handle.\n");
    if (!fHandle)
    {
        // TODO: Exception handler
        exit(EXIT_FAILURE);
    }

    if (fHandle != 0) { mHandle = fHandle; }
}

File::~File() 
{
    if (mBuffer != nullptr)
    {
        free(mBuffer);
        mBuffer = nullptr;
    }
    fclose((FILE*)mHandle);
}

u64 File::WriteBinary(const char* data, u64 size) 
{
    return 0;
}

bool File::ReadBinary(u64 size) 
{
    SL_ASSERT_MSG(mHandle != nullptr, "Could not get file handle.\n");
    SL_ASSERT_MSG(mFileMode == BINARY_READ
        || mFileMode == BINARY_RW
        || mFileMode == BINARY_RW_APPEND,
         "Incorrect file mode.\n");
    SL_ASSERT_MSG(size > 0, "Incorrect size: %llu.\n", size);
    if (mHandle != nullptr && size > 0)
    {
        if (mFileMode == BINARY_READ
        || mFileMode == BINARY_RW
        || mFileMode == BINARY_RW_APPEND)
        {
            mBuffer = (char*)malloc(size);
            SL_ASSERT_MSG(mBuffer != nullptr, "Could not allocate %llu bytes.\n", size);
            if (!mBuffer) { return false; }
            // Read one block of size bytes i.e whole file at a time.
            u64 blocksRead = fread(mBuffer, size, 1, (FILE*)mHandle);
            if (blocksRead != 1) { return false; }
            return true;
        }
        return false; 
    }
    return false;
}

u64 File::WriteText(const char* data, u64 size) 
{
   return 0; 
}

bool File::ReadText(u64 size) 
{
    SL_ASSERT_MSG(mHandle != nullptr, "Could not get file handle.\n");
    SL_ASSERT_MSG(mFileMode == TEXT_READ
        || mFileMode == TEXT_RW
        || mFileMode == TEXT_RW_APPEND,
         "Incorrect file mode.\n");
    SL_ASSERT_MSG(size > 0, "Incorrect size: %llu.\n", size);
    if (mHandle != nullptr && size > 0)
    {
        if (mFileMode == TEXT_READ
        || mFileMode == TEXT_RW
        || mFileMode == TEXT_RW_APPEND)
        {
            mBuffer = (char*)malloc(size);
            SL_ASSERT_MSG(mBuffer != nullptr, "Could not allocate %llu bytes.\n", size);
            if (!mBuffer) { return false; }
            char temp[1024] = { 0 };
            size_t startLocation = 0u;
            while (fgets(temp, 128, (FILE*)mHandle) != nullptr)
            {
                strcpy(mBuffer + startLocation, temp);
                startLocation += strlen(temp);
            }
            return true;
        }
        return false; 
    }
    return false;
}

i64 File::GetDelimitedView(char delimiter, char* str)
{
    u64 substringLength = 0u;
    u32 ch = 0;
    u32 next_ch = 0;
    const char* temp = mBuffer + mCurBufferPos;

    while ((ch = *temp) != 0)
    {
        next_ch = *(temp + 1);
        temp++;
        ++substringLength;
        mCurBufferPos++;
        if (ch == delimiter)
        {
            *str = 0;
            return substringLength;
        }
        else if (next_ch == 0)
        {
			*str++ = ch;
            *str = 0;
            return substringLength;
        }
        *str++ = ch;
    }
    return -1;
}

i64 File::GetLineView(char* str) 
{
    return GetDelimitedView('\n', str);
}

void File::ParseHeader(const char* delim, ParseEntry* entryOut) 
{
    char temp[BUF_1K] = { 0 };
    for (;;)
    {
		if (GetLineView(temp) != -1)
        {
            if ((FindSubstring(temp, DefaultHeaderEntry::count) == 1) && (FindSubstring(temp, delim) == 1))
            {
                if (FindSubstring(temp, DefaultHeaderEntry::vertex) == 1)
                {
                    entryOut->vertexCount = GetNumber(temp);
                    continue;
                }
                else if ((FindSubstring(temp, DefaultHeaderEntry::triangle) == 1) && (FindSubstring(temp, delim) == 1))
                {
                    entryOut->triangleCount = GetNumber(temp);
                    continue;
                }
            }
            else
                break;
        }
    }
}

i32 File::Size() const
{
    return mFileInfo.st_size;
}

void File::SetBufferPos(u32 value) 
{
    mCurBufferPos = value;
}

u32 File::GetBufferPos() const
{
    return mCurBufferPos; 
}

