#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "mapped-hooks.hpp"
#include "utils.hpp"
#include <gd.h>
#include <fstream>
#include <iostream>
#include <vector>

using namespace cocos2d;

std::vector<gd::GameObject*> g_activated_objects;
std::vector<gd::GameObject*> g_activated_objects_p2; // amazing
std::vector<std::pair<size_t, size_t>> g_checkpoints;

void handle(bool a, bool b, gd::GameObject* object) {
    auto play_layer = gd::GameManager::sharedState()->getPlayLayer();
    if (play_layer && play_layer->is_practice_mode) {
        if (object->m_hasBeenActivated && !a)
            g_activated_objects.push_back(object);
        if (object->m_hasBeenActivatedP2 && !b)
            g_activated_objects_p2.push_back(object);
    }
}

void __fastcall PlayerObject_ringJump(gd::PlayerObject* self, int, gd::GameObject* ring) {
    bool a = ring->m_hasBeenActivated;
    bool b = ring->m_hasBeenActivatedP2;
    MHook::getOriginal(PlayerObject_ringJump)(self, 0, ring);
    handle(a, b, ring);
}

void __fastcall GameObject_activateObject(gd::GameObject* self, int, gd::PlayerObject* player) {
    bool a = self->m_hasBeenActivated;
    bool b = self->m_hasBeenActivatedP2;
    MHook::getOriginal(GameObject_activateObject)(self, 0, player);
    handle(a, b, self);
}

void __fastcall GJBaseGameLayer_bumpPlayer(gd::GJBaseGameLayer* self, int, gd::PlayerObject* player, gd::GameObject* object) {
    bool a = object->m_hasBeenActivated;
    bool b = object->m_hasBeenActivatedP2;
    MHook::getOriginal(GJBaseGameLayer_bumpPlayer)(self, 0, player, object);
    handle(a, b, object);
}

void __fastcall PlayLayer_resetLevel(gd::PlayLayer* self) {
    MHook::getOriginal(PlayLayer_resetLevel)(self);
    if (self->checkpoint_array->count() == 0) {
        g_activated_objects.clear();
        g_activated_objects_p2.clear();
        g_checkpoints.clear();
    } else {
        const auto [p1, p2] = g_checkpoints.back();
        g_activated_objects.erase(
            g_activated_objects.begin() + p1,
            g_activated_objects.end()
        );
        g_activated_objects_p2.erase(
            g_activated_objects_p2.begin() + p2,
            g_activated_objects_p2.end()
        );
        for (const auto& object : g_activated_objects) {
            object->m_hasBeenActivated = true;
        }
        // amazing
        for (const auto& object : g_activated_objects_p2) {
            object->m_hasBeenActivatedP2 = true;
        }
    }
}

void* __fastcall PlayLayer_createCheckpoint(gd::PlayLayer* self) {
    g_checkpoints.push_back({g_activated_objects.size(), g_activated_objects_p2.size()});
    return MHook::getOriginal(PlayLayer_createCheckpoint)(self);
}

void* __fastcall PlayLayer_removeLastCheckpoint(gd::PlayLayer* self) {
    g_checkpoints.pop_back();
    return MHook::getOriginal(PlayLayer_removeLastCheckpoint)(self);
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

    MHook::registerHook(base + 0x1F4FF0, PlayerObject_ringJump);
    MHook::registerHook(base + 0xEF0E0, GameObject_activateObject);
    MHook::registerHook(base + 0x10ed50, GJBaseGameLayer_bumpPlayer);
    MHook::registerHook(base + 0x20BF00, PlayLayer_resetLevel);
    MHook::registerHook(base + 0x20B050, PlayLayer_createCheckpoint);
    MHook::registerHook(base + 0x20B830, PlayLayer_removeLastCheckpoint);

    MH_EnableHook(MH_ALL_HOOKS);

#ifdef _DEBUG
    std::getline(std::cin, std::string());

    MH_Uninitialize();
    conout.close();
    conin.close();
    FreeConsole();
    FreeLibraryAndExitThread(cast<HMODULE>(hModule), 0);
#endif
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
        CreateThread(0, 0, my_thread, hModule, 0, 0);
    return TRUE;
}