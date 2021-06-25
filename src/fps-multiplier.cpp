#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <MinHook.h>
#include "utils.hpp"
#include <cocos2d.h>
#include <fstream>
#include <iostream>
#include "hackpro_ext.h"

using namespace cocos2d;

float g_target_fps = 60.f;
bool g_enabled = false;
bool g_disable_render = false;
float g_left_over = 0.f;

void (__thiscall* CCScheduler_update)(CCScheduler*, float);
void __fastcall CCScheduler_update_H(CCScheduler* self, int, float dt) {
    if (!g_enabled)
        return CCScheduler_update(self, dt);
    auto speedhack = self->getTimeScale();

    const float newdt = 1.f / g_target_fps / speedhack;
    g_disable_render = true;

    const int times = min(static_cast<int>((dt + g_left_over) / newdt), 100); // limit it to 100x just in case
    for (int i = 0; i < times; ++i) {
        if (i == times - 1)
            g_disable_render = false;
        CCScheduler_update(self, newdt);
    }
    g_left_over += dt - newdt * times;
}

void (__thiscall* PlayLayer_updateVisibility)(void*);
void __fastcall PlayLayer_updateVisibility_H(void* self) {
    if (!g_disable_render)
        PlayLayer_updateVisibility(self);
}

void __stdcall textbox_cb(void* textbox) {
    auto text = HackproGetTextBoxText(textbox);
    try {
        g_target_fps = max(std::stof(text), 1.f);
    } catch (...) {
        g_target_fps = 60.f;
    }
}

void __stdcall check_cb(void*) {
    g_enabled = true;
}

void __stdcall uncheck_cb(void*) {
    g_enabled = false;
    g_disable_render = false;
}

// #define _DEBUG

DWORD WINAPI my_thread(void* hModule) {
#ifdef _DEBUG
    AllocConsole();
    std::ofstream conout("CONOUT$", std::ios::out);
    std::ifstream conin("CONIN$", std::ios::in);
    std::cout.rdbuf(conout.rdbuf());
    std::cin.rdbuf(conin.rdbuf());
#endif

    MH_Initialize();

    auto base = cast<uintptr_t>(GetModuleHandle(0));
    auto cocos = GetModuleHandleA("libcocos2d.dll");

    MH_CreateHook((void*)(base + 0x205460), PlayLayer_updateVisibility_H, (void**)&PlayLayer_updateVisibility);

    MH_CreateHook(GetProcAddress(cocos, "?update@CCScheduler@cocos2d@@UAEXM@Z"), CCScheduler_update_H, (void**)&CCScheduler_update);

    MH_EnableHook(MH_ALL_HOOKS);

    void* ext = nullptr;
    if (InitialiseHackpro()) {
        if (HackproIsReady()) {
            ext = HackproInitialiseExt("fps mult");
            if (ext) {
                auto textbox = HackproAddTextBox(ext, textbox_cb);
                HackproSetTextBoxText(textbox, "60");
                HackproAddCheckbox(ext, "Enable", check_cb, uncheck_cb);
                HackproCommitExt(ext);
            }
        }
    }

#ifdef _DEBUG
    std::getline(std::cin, std::string());

    MH_Uninitialize();
    conout.close();
    conin.close();
    FreeConsole();
    if (ext)
        HackproWithdrawExt(ext);
    FreeLibraryAndExitThread(cast<HMODULE>(hModule), 0);
#endif
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
        CreateThread(0, 0, my_thread, hModule, 0, 0);
    return TRUE;
}