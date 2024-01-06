/*
    oboskrnl/string.cpp

    Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <string.h>
#include <memory_manipulation.h>

#include <allocators/liballoc.h>

namespace obos
{
    namespace utils
    {
        String::String()
        {
            m_len = 0;
            m_str = new char[m_len + 1];
        }
        String::String(const char* str)
        {
            initialize(str);
        }
        String::String(const char* str, size_t len)
        {
            initialize(str, len);
        }

        String& String::operator=(const char* str)
        {
            initialize(str);
            return *this;
        }

        String::String(const String& other)
        {
            if (!other.m_len)
                return;
            m_str = (char*)utils::memcpy(new char[other.m_len + 1], other.m_str, other.m_len);
            m_len = other.m_len;
        }
        String& String::operator=(const String& other)
        {
            if (!other.m_len)
                return *this;
            m_str = (char*)utils::memcpy(new char[other.m_len + 1], other.m_str, other.m_len);
            m_len = other.m_len;            
            return *this;
        }
        String::String(String&& other)
        {
            if (!other.m_len)
                return;
            m_str = (char*)utils::memcpy(new char[other.m_len + 1], other.m_str, other.m_len);
            m_len = other.m_len;
            delete[] other.m_str;
            other.m_str = nullptr;
            other.m_len = 0;
        }
        String& String::operator=(String&& other)
        {
            if (!other.m_len)
                return *this;
            m_str = (char*)utils::memcpy(new char[other.m_len + 1], other.m_str, other.m_len);
            m_len = other.m_len;
            delete[] other.m_str;
            other.m_str = nullptr;
            other.m_len = 0;
            return *this;
        }

        void String::initialize(const char* str)
        {
            initialize(str, strlen(str));
        }
        void String::initialize(const char* str, size_t len)
        {
            if (m_str)
            {
                delete[] m_str;
                m_str = nullptr;
                m_len = 0;
            }
            m_len = len;
            m_str = (char*)utils::memcpy(new char[m_len + 1], str, m_len);
        }

        void String::push_back(char ch)
        {
            m_str = (char*)krealloc(m_str, ++m_len);
            m_str[m_len - 1] = ch;
            m_str[m_len]     = 0;
        }
        void String::pop_back()
        {
            if (!m_len)
                return;
            m_str = (char*)krealloc(m_str, --m_len);
        }

        char& String::front() const
        {
            if (!m_len)
                return *(char*)nullptr;
            return m_str[0];
        }
        char& String::back() const
        {
            if (!m_len)
                return *(char*)nullptr;
            return m_str[m_len - 1];
        }
        char &String::at(size_t i) const
        {
            if (i >= m_len)
                return *(char*)nullptr;
            return m_str[i];
        }
        void String::append(const char* str)
        {
            append(str, strlen(str));
        }
        void String::append(const char* str, size_t len)
        {
            m_str = (char*)krealloc(m_str, m_len + len);
            utils::memcpy(m_str + m_len, str, len);
            m_len += len;
        }
        size_t String::shrink(size_t newSize)
        {
            if (newSize > m_len)
                return 0;
            m_str = (char*)krealloc(m_str, newSize);
            size_t ret = m_len;
            m_len = newSize;
            return ret;
        }
        void String::erase()
        {
            shrink(0);
        }
    }
}