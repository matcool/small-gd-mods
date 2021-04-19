#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <MinHook.h>
#include <string>
#include "hackpro_ext.h"
#include <iostream>
#include <fstream>

char* g_clipboard = nullptr;

inline void*(__thiscall* EditorUI_updateButtons)(void* self);

inline int(__thiscall* EditorUI_init)(void* self, int);
int __fastcall EditorUI_initHook(void* self, void*, int idk) {
    auto ret = EditorUI_init(self, idk);
    if (g_clipboard && g_clipboard[0]) {
        auto clipboard = reinterpret_cast<uintptr_t>(self) + 0x2D0;
        auto len = strlen(g_clipboard);
        *reinterpret_cast<size_t*>(clipboard + 16) = len; // size
        *reinterpret_cast<size_t*>(clipboard + 20) = max(len, 15); // capacity
        if (len <= 15) {
            memcpy(reinterpret_cast<char*>(clipboard), g_clipboard, len + 1);
        } else {
            void* newb = malloc(len + 1);
            memcpy(newb, g_clipboard, len + 1);
            *reinterpret_cast<void**>(clipboard) = newb;
        }
        EditorUI_updateButtons(self);
    }
    return ret;
}

inline void(__thiscall* EditorUI_destructor)(void* self);
void __fastcall EditorUI_destructorHook(void* self) {
    // why not just cast this to a std::string? idk it kept crashing
    auto addr = reinterpret_cast<uintptr_t>(self) + 0x2D0;
    auto str_len = *reinterpret_cast<size_t*>(addr + 16);
    if (str_len) {
        char* str_buf;
        if (str_len < 16) {
            // string is small enough to be directly here
            str_buf = reinterpret_cast<char*>(addr);
        } else {
            str_buf = *reinterpret_cast<char**>(addr);
        }
        g_clipboard = reinterpret_cast<char*>(realloc(g_clipboard, str_len + 1));
        memcpy(g_clipboard, str_buf, str_len + 1);
    }
    return EditorUI_destructor(self);
}

void __stdcall cb_check(void* cb) {
    MH_EnableHook(MH_ALL_HOOKS);
}

void __stdcall cb_uncheck(void* cb) {
    MH_DisableHook(MH_ALL_HOOKS);
    if (g_clipboard) {
        free(g_clipboard);
        g_clipboard = nullptr;
    }
}

DWORD WINAPI myThread(void* hModule) {
#ifdef _DEBUG
    AllocConsole();
    freopen_s(reinterpret_cast<FILE**>(stdout), "CONOUT$", "w", stdout);
    static std::ofstream conout("CONOUT$", std::ios::out);
    static std::ofstream conin("CONIN$", std::ios::in);
    std::cout.rdbuf(conout.rdbuf());
    std::cin.rdbuf(conin.rdbuf());
#endif

    MH_Initialize();

    auto base = reinterpret_cast<uintptr_t>(GetModuleHandle(0));
    MH_CreateHook(reinterpret_cast<void*>(base + 0x76310), EditorUI_initHook, reinterpret_cast<void**>(&EditorUI_init));
    MH_CreateHook(reinterpret_cast<void*>(base + 0x76090), EditorUI_destructorHook, reinterpret_cast<void**>(&EditorUI_destructor));

    EditorUI_updateButtons = reinterpret_cast<decltype(EditorUI_updateButtons)>(base + 0x78280);

    MH_EnableHook(MH_ALL_HOOKS);

    void* ext;
    if (InitialiseHackpro()) {
        if (HackproIsReady()) {
            ext = HackproInitialiseExt("GlobalClipboard");
            HackproSetCheckbox(HackproAddCheckbox(ext, "Toggle", cb_check, cb_uncheck), true);
            HackproCommitExt(ext);
        }
    }

#ifdef _DEBUG
    std::getline(std::cin, std::string());
    conout.close();
    conin.close();
    FreeConsole();
    MH_Uninitialize();
    if (ext)
        HackproWithdrawExt(ext);
    FreeLibraryAndExitThread(reinterpret_cast<HMODULE>(hModule), 0);
#endif

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
        CreateThread(0, 0x1000, myThread, hModule, 0, 0);
    return TRUE;
}