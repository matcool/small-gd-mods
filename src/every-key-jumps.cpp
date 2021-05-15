#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <MinHook.h>
#include "hackpro_ext.h"
#include <iostream>
#include <fstream>
#include <string>
#include <set>

typedef enum {
    KEY_Enter = 0x0D,
    KEY_Shift = 0x10,
    KEY_OEMComma = 0xBC,
    KEY_OEMPeriod = 0xBE,
} enumKeyCodes;

bool should_key_jump(int key) {
    return (key >= 'A' && key <= 'Z') ||
    (key >= '0' && key <= '9') ||
    key == KEY_Shift ||
    key == KEY_OEMComma ||
    key == KEY_OEMPeriod ||
    key == KEY_Enter;
}

static const std::set<int> p1_keys = {'1','2','3','4','5','Q','W','E','R','T','A','S','D','F','G','Z','X','C','V','B'};

bool is_player1(int key) {
    return p1_keys.find(key) != p1_keys.end();
}

bool g_enabled = true;

// i hate this
bool g_left_shift = false;
bool g_right_shift = false;

void(__thiscall* dispatchKeyboardMSG)(void* self, int key, bool down);
void __fastcall dispatchKeyboardMSGHook(void* self, void*, int key, bool down) {
    if (!g_enabled) return dispatchKeyboardMSG(self, key, down);
    auto base = reinterpret_cast<uintptr_t>(GetModuleHandle(0));
    auto play_layer = *reinterpret_cast<uintptr_t*>(*reinterpret_cast<uintptr_t*>(base + 0x3222D0) + 0x164);
    if (play_layer && should_key_jump(key)) {
        auto is_practice_mode = *reinterpret_cast<bool*>(play_layer + 0x495);
        if (!is_practice_mode || (key != 'Z' && key != 'X')) {
            bool player1 = true;
            if (key == KEY_Shift) {
                bool left_shift = GetAsyncKeyState(VK_LSHIFT) < 0;
                bool right_shift = GetAsyncKeyState(VK_RSHIFT) < 0;
                if (down) {
                    if (right_shift && !g_right_shift) {
                        g_right_shift = true;
                    } else {
                        g_left_shift = true;
                        right_shift = false;
                    }
                } else {
                    if (g_right_shift && !right_shift) {
                        right_shift = true;
                        g_right_shift = false;
                    } else {
                        right_shift = false;
                        g_left_shift = false;
                    }
                }
                player1 = !right_shift;
            }
            else {
                player1 = is_player1(key);
            }

            if (down)
                reinterpret_cast<int(__thiscall*)(uintptr_t, int, bool)>(base + 0x111500)(play_layer, 0, player1);
            else
                reinterpret_cast<int(__thiscall*)(uintptr_t, int, bool)>(base + 0x111660)(play_layer, 0, player1);
        }
    }

    dispatchKeyboardMSG(self, key, down);
}

inline void* get_game_manager() {
    auto base = reinterpret_cast<uintptr_t>(GetModuleHandle(0));
    return *reinterpret_cast<void**>(base + 0x3222D0);
}

bool get_game_variable(const char* key) {
    auto base = reinterpret_cast<uintptr_t>(GetModuleHandle(0));
    return reinterpret_cast<bool (__thiscall*)(void* self, const char*)>(base + 0xC9D30)(get_game_manager(), key);
}

void set_game_variable(const char* key, bool value) {
    auto base = reinterpret_cast<uintptr_t>(GetModuleHandle(0));
    reinterpret_cast<void (__thiscall*)(void*, const char*, bool)>(base + 0xc9b50)(get_game_manager(), key, value);
}

const char* enabled_key = "ekj_enabled";

void __stdcall cb_check(void* cb) {
    set_game_variable(enabled_key, true);
    g_enabled = true;
}

void __stdcall cb_uncheck(void* cb) {
    set_game_variable(enabled_key, false);
    g_enabled = false;
}

DWORD WINAPI myThread(void* hModule) {
#ifdef _DEBUG
    AllocConsole();
    static std::ofstream conout("CONOUT$", std::ios::out);
    static std::ofstream conin("CONIN$", std::ios::in);
    std::cout.rdbuf(conout.rdbuf());
    std::cin.rdbuf(conin.rdbuf());
#endif
    MH_Initialize();

    auto cocos = GetModuleHandleA("libcocos2d.dll");
    auto addr = GetProcAddress(cocos, "?dispatchKeyboardMSG@CCKeyboardDispatcher@cocos2d@@QAE_NW4enumKeyCodes@2@_N@Z");
    MH_CreateHook(
        addr,
        dispatchKeyboardMSGHook,
        reinterpret_cast<void**>(&dispatchKeyboardMSG)
    );

    void* ext = nullptr;
    if (InitialiseHackpro()) {
        if (HackproIsReady()) {
            ext = HackproInitialiseExt("every key jump");
            auto cb = HackproAddCheckbox(ext, "Toggle", cb_check, cb_uncheck);
            if (g_enabled = get_game_variable(enabled_key))
                HackproSetCheckbox(cb, true);
            HackproCommitExt(ext);
        }
    } else {
        g_enabled = true;
    }
    MH_EnableHook(addr);

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
        CreateThread(0, 0, myThread, hModule, 0, 0);
    return TRUE;
}