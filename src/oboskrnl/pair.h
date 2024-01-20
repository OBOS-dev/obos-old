/*
    oboskrnl/pair.h

    Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

namespace obos
{
    namespace utils
    {
        template<typename T1, typename T2>
        struct pair
        {
            T1 first;
            T2 second;
            bool operator==(const pair& other) const
            {
                return 
                    first == other.first &&
                    second == other.second;
            }
        };
        
    }
}