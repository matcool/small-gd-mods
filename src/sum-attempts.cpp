#define WIN32_LEAN_AND_MEAN
#include "mapped-hooks.hpp"
#include "Windows.h"
#include <fstream>
#include <iostream>
#include <cocos2d.h>
#include <gd.h>
#include "utils.hpp"

using namespace cocos2d;

uintptr_t base;

class GJGameLevel : public CCNode {
public:
    PAD(300);
    int attempts;
    PAD(104);
    int levelFolder;
    PAD(185);
    bool levelNotDownloaded;
};

class GameLevelManager : public CCNode {
public:
    PAD(8);
    CCDictionary* dict_saved_levels;

    static inline auto sharedState() {
        return *cast<GameLevelManager**>(base + 0x3222C8);
    }
};

class LocalLevelManager : public CCNode {
public:
    PAD(36);
    CCArray* levels_array;

    static inline auto sharedState() {
        return *cast<LocalLevelManager**>(base + 0x3222E4);
    }
};

class GJSearchObject : public CCNode {
public:
    int search_type;
    PAD(96);
    int current_folder;
};

class LevelBrowserLayer : public CCLayer {
public:
	PAD(52);
	GJSearchObject* search_object;
};

inline std::pair<int, int> get_attempt_count_for_folder(int folder, CCDictionary* dict) {
    int total = 0;
    int downloaded_total = 0;
    CCDictElement* value;
    CCDICT_FOREACH(dict, value) {
        auto lvl = cast<GJGameLevel*>(value->getObject());
        if (lvl && (!folder || lvl->levelFolder == folder)) {
            total += lvl->attempts;
            if (!lvl->levelNotDownloaded) downloaded_total += lvl->attempts;
        }
    }
    return {total, downloaded_total};
}

inline std::pair<int, int> get_attempt_count_for_folder(int folder, CCArray* arr) {
    int total = 0;
    CCObject* value;
    CCARRAY_FOREACH(arr, value) {
        auto lvl = cast<GJGameLevel*>(value);
        if (lvl && (!folder || lvl->levelFolder == folder)) {
            total += lvl->attempts;
        }
    }
    return {total, total};
}

inline const std::string format_commas(int value) {
    // https://stackoverflow.com/a/24192835/9124836
    auto str = std::to_string(value);
    int n = str.length() - 3;
    while (n > 0) {
        str.insert(n, ",");
        n -= 3;
    }
    return str;
}

class DaButtonCallback {
public:
    void callback(CCObject* sender) {
        auto lvl_browser = cast<LevelBrowserLayer*>(this);
        auto folder = lvl_browser->search_object->current_folder;
        int total, downloaded_total;
        std::pair<int, int> result;
        if (lvl_browser->search_object->search_type == 98) {
            result = get_attempt_count_for_folder(folder, LocalLevelManager::sharedState()->levels_array);
        } else {
            result = get_attempt_count_for_folder(folder, GameLevelManager::sharedState()->dict_saved_levels);
        }
        total = result.first;
        downloaded_total = result.second;
        auto str = format_commas(downloaded_total) + " attempts";
        if (total != downloaded_total)
            str += "\n" + format_commas(total) + " attempts <cr>from unloaded levels</c>";
        gd::FLAlertLayer::create(nullptr, "Attempt Count", "OK", nullptr, str.c_str())->show();
    }
};

bool __fastcall LevelBrowserLayer_init(CCLayer* self, void*, GJSearchObject* so) {
    if (!MHook::getOriginal(LevelBrowserLayer_init)(self, 0, so)) return false;
    if (so->search_type == 98 || so->search_type == 99) {
        auto menu = CCMenu::create();
        auto sprite = CCSprite::create("GJ_button_01.png");
        auto skull = CCSprite::createWithSpriteFrameName("miniSkull_001.png");
        skull->setPosition({20, 20});
        sprite->addChild(skull);
        auto btn = gd::CCMenuItemSpriteExtra::create(
            sprite,
            self,
            menu_selector(DaButtonCallback::callback)
        );
        menu->addChild(btn);
        menu->setPosition({30, 80});
        self->addChild(menu);
    }
    return true;
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

    base = reinterpret_cast<uintptr_t>(GetModuleHandle(0));
    
    MHook::registerHook(base + 0x15a040, LevelBrowserLayer_init);

    MH_EnableHook(MH_ALL_HOOKS);

#ifdef _DEBUG
    std::getline(std::cin, std::string());

    MH_Uninitialize();
    conout.close();
    conin.close();
    FreeConsole();
    FreeLibraryAndExitThread(reinterpret_cast<HMODULE>(hModule), 0);
#endif
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
        CreateThread(0, 0x1000, my_thread, hModule, 0, 0);
    return TRUE;
}