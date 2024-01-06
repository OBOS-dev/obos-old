/*
    oboskrnl/string.h

    Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>

namespace obos
{
    namespace utils
    {
        class String final
        {
        public:
            String();
            String(const char* str);
            String(const char* str, size_t len);

            String& operator=(const char* str);

            String(const String& other);
            String& operator=(const String& other);
            String(String&& other);
            String& operator=(String&& other);

            void initialize(const char* str);
            void initialize(const char* str, size_t len);
        
            void push_back(char ch);
            void pop_back();

            char& front() const;
            char& back() const;

            char& at(size_t i) const;
            char& operator[](size_t i) const { return at(i); }

            void append(const char* str);
            void append(const char* str, size_t len);

            char* data() const { return m_str; }

            size_t length() const { return m_len; }

            size_t shrink(size_t newSize);

            void erase();

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