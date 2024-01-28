/*
    oboskrnl/boot/cfg.h

    Copyright (c) 2023,2024 Omar Berrow
*/

#pragma once

// Utility to parse the kernel config file.

#include <int.h>
#include <utils/hashmap.h>
#include <utils/vector.h>
#include <utils/string.h>

namespace obos
{
    struct Element
    {
        Element() = default;
        enum
        {
            ELEMENT_INVALID,
            ELEMENT_STRING,
            ELEMENT_ARRAY,
            ELEMENT_INTEGER,
            ELEMENT_BOOLEAN,
        } type = ELEMENT_INVALID;
        utils::String string{};
        utils::Vector<Element> array{};
        uintptr_t integer = 0;
        bool boolean = false;
        Element(const Element& other)
        {
            type = other.type;
            switch (other.type)
            {
            case ELEMENT_ARRAY:
                array = other.array;
                break;
            case ELEMENT_STRING:
                string = other.string;
                break;
            case ELEMENT_BOOLEAN:
                boolean = other.boolean;
                break;
            case ELEMENT_INTEGER:
                integer = other.integer;
                break;
            default:
                break;
            }
            return;
        }
        Element& operator=(const Element& other)
        {
            type = other.type;
            switch (other.type)
            {
            case ELEMENT_ARRAY:
                array = other.array;
                break;
            case ELEMENT_STRING:
                string = other.string;
                break;
            case ELEMENT_BOOLEAN:
                boolean = other.boolean;
                break;
            case ELEMENT_INTEGER:
                integer = other.integer;
                break;
            default:
                break;
            }
            return *this;
        }
        Element& operator=(Element&&) = delete;
        Element(Element&&) = delete;
        ~Element(){}
    };
    class Parser final
    {
    public:
        Parser() = default;
        
        Parser(const Parser&) = delete;
        Parser& operator=(const Parser&) = delete;
        Parser(Parser&&) = delete;
        Parser& operator=(Parser&&) = delete;

        bool Parse(const char* data, size_t size, utils::Vector<const char*>& errorMessages);

        const Element* GetElement(const char* key);
        const auto& GetMap() { return m_elements; }

        ~Parser() {}
    private:
        static bool equals(const char* const& key1, const char* const& key2)
        { return utils::strcmp(key1, key2); }
        static size_t hasher(const char* const& key)
        {
            const byte* _key = (byte*)key;

            size_t h = 0, g = 0;

            while (*_key)
			{
				h = (h << 4) + *_key++;
				if ((g = h & 0xf0000000))
					h ^= g >> 24;
				h &= ~g;
			}
			return h;
        }
        utils::Hashmap<const char*, Element, equals, hasher> m_elements;
    };
}