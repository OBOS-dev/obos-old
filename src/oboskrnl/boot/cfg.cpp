/*
    oboskrnl/boot/cfg.cpp

    Copyright (c) 2023,2024 Omar Berrow
*/

#include <int.h>
#include <hashmap.h>
#include <vector.h>
#include <pair.h>

#include <boot/cfg.h>

namespace obos
{
    enum class tokenType
    {
        VARIABLE_NAME,
        OPERATOR,
        DATA,
        ARRAY_INITIALIZER,
    };
    uintptr_t hex2bin(const char* str, unsigned size)
	{
		uintptr_t ret = 0;
		str += *str == '\n';
		for (int i = size - 1, j = 0; i > -1; i--, j++)
			{
				char c = str[i];
				uintptr_t digit = 0;
				switch (c)
				{
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					digit = c - '0';
					break;
				case 'A':
				case 'B':
				case 'C':
				case 'D':
				case 'E':
				case 'F':
					digit = (c - 'A') + 10;
					break;
				case 'a':
				case 'b':
				case 'c':
				case 'd':
				case 'e':
				case 'f':
					digit = (c - 'a') + 10;
					break;
				default:
					break;
				}
				ret |= digit << (j * 4);
			}
		return ret;
	}
    uintptr_t dec2bin(const char* str, size_t size)
	{
		uintptr_t ret = 0;
		for (size_t i = 0; i < size; i++)
		{
			if ((str[i] - '0') < 0 || (str[i] - '0') > 9)
				continue;
			ret *= 10;
			ret += str[i] - '0';
		}
		return ret;
	}
    const char* tokenTypeToStr(tokenType type)
    {
        if ((int)type >= 3)
            return "INVALID";
        const char* table[3] = {
            "VARIABLE_NAME",
            "OPERATOR",
            "DATA",
        };
        return table[(int)type];
    }
    struct token
    {
        const char* contents;
        size_t len;
        void(*freer)(void*);
        tokenType type;
        uint64_t line = 0, character = 0;
    };
    enum integerType
    {
        INVALID,
        DECIMAL = 10,
        HEX = 16,
    };
    static integerType strToIntegerType(const char* str, size_t len)
    {
        integerType ret = integerType::INVALID;
        if (utils::memcmp(str, "0x", 2))
            ret = integerType::HEX;
        else
            ret = integerType::DECIMAL;
        switch (ret)
        {
        case integerType::HEX:
        {
            str += 2;
            len -= 2;
            for (size_t i = 0; i < len; i++)
            {
                if (!(str[i] >= '0' && str[i] <= '9') &&
                    !(str[i] >= 'A' && str[i] <= 'F') &&
                    !(str[i] >= 'a' && str[i] <= 'f'))
                {
                    ret = integerType::INVALID;
                    break;
                }
            }   
            break;
        }
        case integerType::DECIMAL:
        {
            for (size_t i = 0; i < len; i++)
            {
                if (!(str[i] >= '0' && str[i] <= '9'))
                {
                    ret = integerType::INVALID;
                    break;
                }
            }
            break;
        }
        default:
            break;
        }
        return ret;
    }
    static bool isWhitespace(char ch)
    {
        return
            ch == ' '  ||
            ch == '\t' ||
            ch == '\n' ||
            ch == '\v' ||
            ch == '\f' ||
            ch == '\r'; 
    }
    // static size_t countToWhitespace(const char* str)
    // {
    //     size_t i = 0;
    //     for(; str[i] && !isWhitespace(str[i]); i++);
    //     return i;
    // }
    static size_t countToWhitespaceOrChar(const char* str, char ch)
    {
        size_t i = 0;
        for(; (str[i] != 0) && (!isWhitespace(str[i])) && (str[i] != ch); i++);
        return i;
    }
    void ProcessDataToken(Element& element, const token& tok)
    {
        const char* data = tok.contents;
        integerType isInteger = strToIntegerType(data, tok.len);
        // Integer
        if (isInteger != integerType::INVALID)
        {
            element.integer = isInteger == integerType::HEX ? hex2bin(data, tok.len) : dec2bin(data, tok.len);
            element.type = Element::ELEMENT_INTEGER;
        }
        // Array
        else if (*data == '[')
        {
            // Loop over the array elements, adding them to the array.
            utils::Vector<token> arrayTokens;
            size_t nElements = 1;
            for (size_t i = 0; i < tok.len; i++)
                nElements += data[i] == ',';
            size_t currentElement = 0;
            for (const char* iter = data; iter < (data + tok.len); iter++)
            {
                if (isWhitespace(*iter))
                    continue;
                if (*iter == ']')
                    break;
                if (*iter == '[')
                    continue;
                if (currentElement == nElements)
                    break;
                token ctok;
                ctok.type = tokenType::ARRAY_INITIALIZER;
                if (((currentElement + 1) == nElements))
                {
                    // Find the first whitespace before the terminating bracket.
                    bool inQuotations = false;
                    for (const char* iter2 = iter; iter2 < (data + tok.len); iter2++)
                    {
                        if (isWhitespace(*iter2) && !inQuotations)
                        {
                            ctok.len = (iter2 - iter);
                            break;
                        }
                        if (*iter2 == '\"')
                            inQuotations = !inQuotations;
                        if (*iter2 == ']')
                        {
                            ctok.len = (iter2 - iter);
                            break;
                        }
                    }
                }
                else
                    ctok.len = utils::strCountToChar(iter, ',');
                ctok.contents = (char*)utils::memcpy(new char[ctok.len + 1], iter, ctok.len);
                ctok.freer = kfree;
                iter += ctok.len;
                arrayTokens.push_back(ctok);
                element.type = Element::ELEMENT_ARRAY;
                currentElement++;
            }
            for (auto& arrTok : arrayTokens)
            {
                Element newEle;
                ProcessDataToken(newEle, arrTok); // Maybe a bad idea?
                element.array.push_back(newEle);
            }
            for (auto& ctok : arrayTokens)
                if (ctok.freer)
                    ctok.freer((void*)ctok.contents);
        }
        // Boolean
        else if (utils::strcmp(data, "true") || utils::strcmp(data, "false"))
        {
            element.boolean = utils::strcmp(data, "true");
            element.type = Element::ELEMENT_BOOLEAN;
        }
        else
        {
            element.string = data;
            element.type = Element::ELEMENT_STRING;
        }
    }
    bool Parser::Parse(const char* data, size_t size, utils::Vector<const char*>& errorMessages)
    {
        // Parse the file, looking for three tokens:
        // (variable name)
        // '='
        // (variable contents)
        utils::Vector<token> tokens;
        tokenType searchingFor = tokenType::VARIABLE_NAME;
        size_t lxCurrentLine = 1;
        size_t lxCurrentChar = 0;
        for (const char* iter = data; iter < (data + size); iter++, lxCurrentChar++)
        {
            if (!(*iter))
                break;
            if (*iter == '#')
            {
                iter += utils::strCountToChar(iter, '\n');
                lxCurrentLine++;
                lxCurrentChar = 0;
                continue;
            }
            if (isWhitespace(*iter))
            {
                if (*iter == '\n')
                {
                    lxCurrentLine++;
                    lxCurrentChar = 0;
                }
                continue;
            }
            if (searchingFor == tokenType::OPERATOR && (*iter != '='))
            {
                char* msg = new char[sizeof("Syntax error: Expected \'=\', got #")];
                logger::sprintf(msg, "Line %d:%d, Syntax error: Expected \'=\', got %c", lxCurrentLine, lxCurrentChar, *iter);
                errorMessages.push_back(msg);
                // Go on to the next line.
                iter += utils::strCountToChar(iter, '\n');
                lxCurrentLine++;
                lxCurrentChar = 0;
                searchingFor = tokenType::VARIABLE_NAME;
                continue;
            }
            token tok;
            tok.type = searchingFor;
            tok.line = lxCurrentLine;
            tok.character = lxCurrentChar;
            switch (searchingFor)
            {
            case tokenType::VARIABLE_NAME:
                tok.len = countToWhitespaceOrChar(iter, '=');
                tok.contents = (char*)utils::memcpy(kcalloc(tok.len + 1, sizeof(char)), iter, tok.len);
                tok.freer = kfree;
                searchingFor = tokenType::OPERATOR;
                break;
            case tokenType::OPERATOR:
                tok.len = 1;
                tok.contents = "=";
                searchingFor = tokenType::DATA;
                break;
            case tokenType::DATA:
                tok.len = utils::strCountToChar(iter, '\n');
                tok.contents = (char*)utils::memcpy(kcalloc(tok.len + 1, sizeof(char)), iter, tok.len);
                tok.freer = kfree;
                searchingFor = tokenType::VARIABLE_NAME;
                break;
            default:
                break;
            }
            lxCurrentChar += (tok.len - 1);
            iter += tok.len - 1;
            tokens.push_back(tok);
        }
        if (errorMessages.length() > 0)
            goto finish; // Lexer error.
        {
            // Loop over the tokens.
            utils::pair<const char*, Element> currentElement;
            bool isLookingForVariableName = true;
            for (auto& tok : tokens)
            {
                bool canContinue = true;
                switch (tok.type)
                {
                case tokenType::OPERATOR:
                    isLookingForVariableName = false;
                    break;
                case tokenType::VARIABLE_NAME:
                    if (!isLookingForVariableName)
                    {
                        size_t szMsg = 
                        logger::sprintf(nullptr, "Line %d:%d, Internal error: Unexcepted token of type \"VARIABLE_NAME.\"", tok.line, tok.character);
                        char* msg = new char[szMsg + 1];
                        logger::sprintf(msg, "Line %d:%d, Internal error: Unexcepted token of type \"VARIABLE_NAME.\"", tok.line, tok.character);
                        errorMessages.push_back(msg);
                        canContinue = false;
                        break;
                    }
                    currentElement.first = (char*)utils::memcpy(new char[tok.len + 1], tok.contents, tok.len);
                    break;
                case tokenType::DATA:
                {
                    if (isLookingForVariableName)
                    {
                        size_t szMsg = 
                        logger::sprintf(nullptr, "Line %d:%d, Internal error: Unexcepted token of type \"DATA.\"", tok.line, tok.character);
                        char* msg = new char[szMsg + 1];
                        logger::sprintf(msg, "Line %d:%d, Internal error: Unexcepted token of type \"DATA.\"", tok.line, tok.character);
                        errorMessages.push_back(msg);
                        canContinue = false;
                        break;
                    }
                    ProcessDataToken(currentElement.second, tok);
                    isLookingForVariableName = true;
                    m_elements.emplace_at(currentElement.first, currentElement.second);
                    currentElement.~pair();
                    utils::memzero(&currentElement, sizeof(currentElement));
                    break;
                }
                default:
                    break;
                if (!canContinue)
                    break;
                }    
            }
        }
        finish:
        // Free the tokens.
        for (auto& tok : tokens)
            if (tok.freer)
                tok.freer((void*)tok.contents);
        return errorMessages.length() == 0;
    }
    const Element* Parser::GetElement(const char* key)
    {
        return &m_elements.at(key);
    }
}