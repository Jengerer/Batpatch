#include "common.h"
#include "patching.h"
#include <algorithm>
#include <cstdint>
#include <iostream>

// Buffers for converting.
constexpr uint32_t HeaderSize = 4096;
constexpr uint32_t HeaderElements = HeaderSize / sizeof(uint32_t);
uint32_t header[HeaderElements];

// Write new App ID and get the decryption key values.
bool ProcessHeader()
{
    std::wcout << TEXT("Processing executable header and patching App ID into executable...\n");

    const uint32_t HeaderStart = 0xC86B9CA8U;
    char* end = exeBuffer + exeSize - HeaderSize;
    char* headerChunk = std::find_if(
        exeBuffer,
        end,
        [HeaderStart](char& current) { return reinterpret_cast<uint32_t&>(current) == HeaderStart; }
    );
    if (headerChunk == end)
    {
        std::wcout << TEXT("Failed to find header with App ID and decryption keys.\n");
        return false;
    }

    // It doesn't seem like it's aligned, but should work.
    uint32_t* encryptedHeader = reinterpret_cast<uint32_t*>(headerChunk);
    uint32_t mask = 0; // First value goes unchanged.
    for (uint32_t i = 0; i < HeaderElements; ++i)
    {
        uint32_t newMask = encryptedHeader[i];
        header[i] = newMask ^ mask;
        mask = newMask;
    }

    // Edit the value...
    const uint32_t oldId = 200260;
    const uint32_t newId = 57400;
    uint32_t* idLocation = std::find(std::begin(header), std::end(header), oldId);
    if (idLocation == std::end(header))
    {
        std::wcout << TEXT("Failed to find old App ID in executable... already patched?\n");
        return false;
    }
    *idLocation = newId;

    // Re-encrypt into the EXE buffer.
    mask = 0;
    for (uint32_t i = 0; i < HeaderElements; ++i)
    {
        uint32_t newMask = header[i] ^ mask;
        encryptedHeader[i] = newMask;
        mask = newMask;
    }
    return true;
}

bool PatchAssembly()
{
    std::wcout << TEXT("Patching instructions to skip executable verification and decryption...\n");

    // Patch decryption to just do a read.
    const char oldLoad[] = "\x33\x85\xBC\xFB\xFF\xFF\x8B\x8D\x94\xFB\xFF\xFF\x8B\x95\xB4\xFB\xFF\xFF\x89\x04\x8A\x8B\x85\x98\xFB\xFF\xFF\x33\x85\xA4\xFB\xFF";
    const char newLoad[] = "\x8B\x85\x88\xFB\xFF\xFF\x8B\x8D\x94\xFB\xFF\xFF\x8B\x95\xB4\xFB\xFF\xFF\x89\x04\x8A\x8B\x85\x98\xFB\xFF\xFF\x8B\x85\x8C\xFB\xFF";
    const size_t loadSize = sizeof(oldLoad) - 1;
    char* end = exeBuffer + exeSize - loadSize;
    char* loadStart = std::find_if(
        exeBuffer,
        end,
        [&oldLoad, loadSize](char& current) { return memcmp(&current, oldLoad, loadSize) == 0; }
    );
    if (loadStart == end)
    {
        std::wcout << TEXT("Failed to find code load fingerprint.\n");
        return false;
    }
    memcpy(loadStart, newLoad, loadSize);

    // Patch checksum jump out.
    constexpr uint32_t checksumOld = 0x4D8B1E75;
    constexpr uint32_t checksumNew = 0x4D8B9090;
    end = exeBuffer + exeSize - sizeof(checksumOld);
    char* checksumStart = std::find_if(
        exeBuffer,
        end,
        [checksumOld](char& current) { return memcmp(&current, &checksumOld, sizeof(checksumOld)) == 0; }
    );
    if (checksumStart == end)
    {
        std::wcout << TEXT("Failed to patch out checksum!\n");
        return false;
    }
    memcpy(checksumStart, &checksumNew, sizeof(checksumNew));
    return true;
}

bool ExtractCode()
{
    std::wcout << TEXT("Decrypting code and overwriting encrypted section...\n");

    const uint32_t codeStart = 0x4C54CA21U;
    const uint32_t codeSize = 0x86400;
    char* end = exeBuffer + exeSize - codeSize;
    char* codeBuffer = std::find_if(
        exeBuffer,
        end,
        [codeStart](char& current) { return memcmp(&current, &codeStart, sizeof(uint32_t)) == 0; }
    );
    if ((codeBuffer == end) || (reinterpret_cast<uintptr_t>(codeBuffer) % sizeof(uint32_t) != 0))
    {
        std::wcout << TEXT("Failed to find compressed code fingerprint. Already patched?\n");
        return false;
    }

    // Run this spaghetti decryption algorithm.
    uint32_t* code = reinterpret_cast<uint32_t*>(codeBuffer);
    const uint32_t pairs = codeSize / (sizeof(uint32_t) * 2);
    uint32_t maskA = 0x55555555U;
    uint32_t maskB = 0x55555555U;
    for (uint32_t i = 0; i < pairs; ++i)
    {
        uint32_t valueA = code[i * 2];
        uint32_t valueB = code[(i * 2) + 1];
        uint32_t newValueA = valueA;
        uint32_t newValueB = valueB;
        const uint32_t iterations = 32;
        uint32_t magic = (iterations * 0x9E3779B9U) & 0xFFFFFFFFU;
        for (unsigned int j = 0; j < iterations; ++j)
        {
            // Yeah, I got nothing as to how this works.
            // Once for first value...
            constexpr uint32_t KeyIndexMask = 0x3;
            constexpr uint32_t KeyIndexOffset = 0x7D;
            uint32_t keyIndex = KeyIndexOffset + ((magic >> 0xB) & KeyIndexMask);
            uint32_t keyMask = magic + header[keyIndex];
            newValueB -= (newValueA + ((newValueA << 4) ^ (newValueA >> 5))) ^ keyMask;
            magic += 0x61C88647U;

            // Slightly different thing for second value...
            keyIndex = KeyIndexOffset + (magic & KeyIndexMask);
            keyMask = magic + header[keyIndex];
            newValueA -= (newValueB + ((newValueB << 4) ^ (newValueB >> 5))) ^ keyMask;
        }
        newValueA = newValueA ^ maskA;
        newValueB = newValueB ^ maskB;
        code[i * 2] = newValueA;
        code[(i * 2) + 1] = newValueB;
        maskA = valueA;
        maskB = valueB;
    }
    return true;
}