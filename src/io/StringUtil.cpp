#include <io/StringUtil.hpp>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <memory>

namespace StringUtil
{
    char* ToUpper(const char* str) 
    {
        size_t len = strlen(str);
        // SL_ASSERT_DEBUG(len > 0);
        char* upperCaseString;
        if (len > 0)
        {
            upperCaseString = (char*)malloc(len + 1);
            memset(upperCaseString, 0, len + 1);
            for (size_t i = 0; i < len; ++i)
                upperCaseString[i] = toupper(str[i]);
        }
        return upperCaseString; 
    }
    
    i64 ExtractNumber(char* string) 
    {
        i64 val = 0;
        while (*string) 
        { 
            // While there are more characters to process...
            if (isdigit(*string) || ((*string == '-' || *string == '+') && isdigit(*(string + 1))) ) 
            {
                // Found a number
                val *= 10;
                val += strtol(string, &string, 10); // Read number
            } 
            else 
            {
                // Otherwise, move on to the next character.
                string++;
            }
        }
        return val;
    }
    
    char** Split(const char* str, const char delim, size_t* n) 
    {
        const u32 rate = 10;
        if (strlen(str) < 1) return nullptr;

        char** out = (char**)calloc(rate, sizeof(char*));
        size_t ctr = 0;
        u32 blocks = rate;
        const char* where;
        while ((where = strchr(str, delim)) != NULL) 
        {
            ptrdiff_t size = where - str;
            char* token = (char*)malloc(size + 1);
            strncpy(token, str, size);
            token[size] = '\0';
            out[ctr] = token;
            ctr++;
            if (ctr == blocks) 
            {
                blocks += rate;
                out = (char**) realloc(out, sizeof(char*) * blocks);
            }
            str += size + 1;
        }
        // if delim not 0 we should to get the rest
        if (str[0] != '\0') 
        {
            size_t size = strlen(str);
            char* token = (char*)malloc(size + 1);
            strncpy(token, str, size + 1);
            out[ctr] = token;
            ctr++;
        }
        // Alloc correct size
        out = (char**)realloc(out, sizeof(char*) * (ctr + 1));
        out[ctr] = NULL;
        if (n != nullptr) *n = ctr;
        return out;
    }
    
    char** SplitFormatted(const char* str, const char startSymbol, const char endSymbol, size_t* stringCount)
    {
        *stringCount = 0;
        const u32 rate = 10;
        if (strlen(str) < 1) return nullptr;

        char** out = (char**)calloc(rate, sizeof(char*));
        size_t ctr = 0;
        u32 blocks = rate;
        size_t startSymbolIndex = 0;
        i32 i = 0;
        while ( (*(str + i) != '\0') )
        {
            startSymbolIndex = 0;
            if (*(str + i) == startSymbol)
            {
                startSymbolIndex = i + 1;
                while (*(str + i) != endSymbol)
                {
                    i++;
                }
                size_t diff = i - startSymbolIndex;
                char* token = (char*)malloc(diff + 1);
                strncpy(token, str + startSymbolIndex, diff);
                token[diff] = '\0';
                out[ctr] = token;
                ctr++;
                if (ctr == blocks) 
                {
                    blocks += rate;
                    out = (char**) realloc(out, sizeof(char*) * blocks);
                }
            }
            else
            {
                i++;
            }
        }
        // Allocate correct size
        out = (char**)realloc(out, sizeof(char*) * (ctr + 1));
        out[ctr] = NULL;
        *stringCount = ctr;
        return out;
    }

    void ClearStrings(char** s) 
    {
        if (!s) { return; }
        for (int i = 0; s[i] != NULL; i++) 
        {
            free(s[i]);
        }
        free(s);
    }
    
    char** Substring(char** strings, const char* find, size_t* n) 
    {
        const u32 rate = 10;
        if (strlen(find) < 1) return nullptr;

        char** out = (char**)calloc(rate, sizeof(char*));
        size_t ctr = 0;
        u32 blocks = rate;

        std::unique_ptr<char[]> uniqueFindUpper(ToUpper(find));

        char* findStrUpper = uniqueFindUpper.get();
        for (size_t idx = 0; strings[idx] != NULL; idx++) 
        {
            i32 i = 0;
            i32 j = 0;
            const char* str = strings[idx];
            std::unique_ptr<char[]> uniqueStrUpper(ToUpper(str));
            char* strUpper = uniqueStrUpper.get(); 
            while ( (*(strUpper + j) != '\0') && (*(findStrUpper + i) != '\0')) 
            {
                if (*(findStrUpper + i) != *(strUpper + j)) 
                {

                    j++;
                    i = 0;
                }
                else 
                {
                    i++; j++;
                }
            }
            if (i == strlen(findStrUpper))
            {
                size_t len = strlen(strUpper) + 1;
                char* token = (char*)malloc(len + 1);
                strncpy(token, str, len);
                token[len] = '\0';
                out[ctr] = token;
                ctr++;
                if (ctr == blocks) 
                {
                    blocks += rate;
                    out = (char**) realloc(out, sizeof(char*) * blocks);
                }
            }
        }
        // Allocate correct size
        out = (char**)realloc(out, sizeof(char*) * (ctr + 1));
        out[ctr] = NULL;
        if (n != nullptr) *n = ctr;
        return out;
    }
    
    char* SubstringViewSingle(char* strings, const char* find, size_t* offset)
    {
        std::unique_ptr<char[]> uniqueFindUpper(ToUpper(find));
        char* findStrUpper = uniqueFindUpper.get();       
        char* str;
        i32 i = 0;
        i32 j = 0;
        size_t temp = 0;
        str = strings;
        std::unique_ptr<char[]> uniqueStrUpper(ToUpper(str));
        char* strUpper = uniqueStrUpper.get();
        while ( (*(strUpper + j) != '\0') && (*(findStrUpper + i) != '\0')) 
        {
            if (*(strUpper + j) != '\n')
            {
                if (*(findStrUpper + i) != *(strUpper + j)) 
                {
                    j++;
                    i = 0;
                }
                else 
                {
                    i++; j++;
                }
            }
        }
        if (i == strlen(findStrUpper))
        {
            while (*(str + i + temp) != '\n') 
            {
                temp++;
            }
            str[temp + i] = 0;
            *offset = temp + i + 1;
            return str;
        }
        *offset = temp + i + 1;
        return nullptr;
    }

    char* SubstringView(char** strings, const char* find, size_t* next)
    {
        std::unique_ptr<char[]> uniqueFindUpper(ToUpper(find));
        char* findStrUpper = uniqueFindUpper.get();       
        char* str;
        for (size_t idx = 0; strings[idx] != NULL; idx++) 
        {

            i32 i = 0;
            i32 j = 0;
            str = strings[idx];
            std::unique_ptr<char[]> uniqueStrUpper(ToUpper(str));
            char* strUpper = uniqueStrUpper.get();
            while ( (*(strUpper + j) != '\0') && (*(findStrUpper + i) != '\0')) 
            {
                if (*(findStrUpper + i) != *(strUpper + j)) 
                {

                    j++;
                    i = 0;
                }
                else 
                {
                    i++; j++;
                }
            }
            if (i == strlen(findStrUpper))
            {
                *next = idx + 1;
                return strings[idx];
            }
        }
        return nullptr;
    }
    
    char* SubstringFormatted(char* str, const char startSymbol, const char endSymbol) 
    {
        char* out;
        size_t startSymbolIndex = 0;
        i32 i = 0;
        i32 j = 0;
        while ( (*(str + i) != '\0') )
        {
            startSymbolIndex = 0;
            if (*(str + i) == startSymbol)
            {
                j = 0;
                startSymbolIndex = i + 1;
                while (*(str + i) != endSymbol)
                {
                    i++;
                }
                j = (i - (i32)startSymbolIndex);
                char* token = (char*)malloc(j + 1);
                strncpy(token, str + startSymbolIndex, j);
                token[j] = '\0';
                out = token;
            }
            else
            {
                i++;
            }
        }
        return out;
    }
    
    bool Contains(const char* str, const char* find) 
    {
        if (strstr(str, find) != NULL) 
            return true;
        return false;
    }
    
    void RemoveEndl(char* str)
    {
        i32 i, x;
        for(i = x = 0; str[i]; ++i)
        {
            if(str[i] != '\n')
            {
                str[x++] = str[i];
            }
        }
        str[x] = '\0';
    }
    
    void StripExtraSpace(char* str) 
    {
        i32 i, x;
        for(i = x = 0; str[i]; ++i)
        {
            if(!isspace(str[i]) || (i > 0 && !isspace(str[i - 1])))
            {
                str[x++] = str[i];
            }
        }
        str[x] = '\0';       
    }
}