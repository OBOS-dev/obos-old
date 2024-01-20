/*
    oboskrnl/string.h

    Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>
#include <export.h>

namespace obos
{
    namespace utils
    {
        class String final
        {
        public:
            OBOS_EXPORT String();
            OBOS_EXPORT String(const char* str);
            OBOS_EXPORT String(const char* str, size_t len);

            OBOS_EXPORT String& operator=(const char* str);

            OBOS_EXPORT String(const String& other);
            OBOS_EXPORT String& operator=(const String& other);
            OBOS_EXPORT String(String&& other);
            OBOS_EXPORT String& operator=(String&& other);

            OBOS_EXPORT void initialize(const char* str);
            OBOS_EXPORT void initialize(const char* str, size_t len);
        
            OBOS_EXPORT void push_back(char ch);
            OBOS_EXPORT void pop_back();

            OBOS_EXPORT char& front() const;
            OBOS_EXPORT char& back() const;

            OBOS_EXPORT char& at(size_t i) const;
            OBOS_EXPORT char& operator[](size_t i) const { return at(i); }

            OBOS_EXPORT void append(const char* str);
            OBOS_EXPORT void append(const char* str, size_t len);

            OBOS_EXPORT char* data() const { return m_str; }

            OBOS_EXPORT size_t length() const { return m_len; }

            OBOS_EXPORT size_t shrink(size_t newSize);

            OBOS_EXPORT void erase();

            ~String() 
            {
                delete[] m_str;
                m_len = 0;
            }
        private:
            char* m_str = nullptr;
            size_t m_len = 0;
        };
    }
}