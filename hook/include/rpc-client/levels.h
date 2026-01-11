#pragma once
#include <Windows.h>
#include <cstdint>
#include <initializer_list>

template<typename T, typename Ptr>
T* safe_deref_chain(Ptr base, std::initializer_list<uintptr_t> offsets, bool applyOffsetAfterLastDeref = true);

extern DWORD levelIdBaseAddr;
extern DWORD playerObjectsAddr;
extern DWORD getIDAddr;

struct Level {
    char key;
    const char* name;
    const char* imageName;
};

void initRat();
Level getLevel();
const char* getCharName();