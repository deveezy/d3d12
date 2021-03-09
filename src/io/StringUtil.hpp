#pragma once

#include <Common/defines.hpp>

namespace StringUtil
{
    char*  ToUpper(const char* str); // heap allocation
    i64    ExtractNumber(char* string); // stack
    char** Split(const char* str, const char delim, size_t* stringCount); // heap allocation
    char** SplitFormatted(const char* str, const char startSymbol, const char endSymbol, size_t* stringCount); // heap allocation
    char*  SubstringViewSingle(char* strings, const char* find, size_t* offset);
    void   ClearStrings(char** strings); // free pp
    char** Substring(char** strings, const char* find, size_t* n); // heap
    char*  SubstringView(char** strings, const char* find, size_t* next); // stack 
    char*  SubstringFormatted(char* str, const char startSymbol, const char endSymbol); // heap
    bool   Contains(const char* str, const char* find);
    void   RemoveEndl(char* str);
    void   StripExtraSpace(char* str);
};