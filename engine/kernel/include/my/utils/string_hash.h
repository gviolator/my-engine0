// #my_engine_source_file

#pragma once

namespace my::strings
{
    constexpr inline size_t constHash(const char* input)
    {
        size_t hash = sizeof(size_t) == 8 ? 0xcbf29ce484222325 : 0x811c9dc5;
        const size_t prime = sizeof(size_t) == 8 ? 0x00000100000001b3 : 0x01000193;

        while(*input)
        {
            hash ^= static_cast<size_t>(*input);
            hash *= prime;
            ++input;
        }

        return hash;
    }
}  // namespace my::strings
