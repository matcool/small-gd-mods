#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <MinHook.h>
#include <fstream>
#include <iostream>
#include <cocos2d.h>
#include <sstream>
#include <gd.h>
#include "utils.hpp"
#include "hackpro_ext.h"

using namespace cocos2d;

float g_start_x = 0.f;
bool g_enabled = true;
bool g_show_icon = true;
bool g_show_status = true;
bool g_show_info = true;
bool g_show_start_pos_icon = false; // i find it ugly

enum Position {
    TOP_LEFT,
    TOP_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT,
};
auto g_position = Position::TOP_LEFT;

void* g_ext = nullptr; // to check if u have mhv6

void update_labels(gd::PlayLayer* layer) {
    if (!layer) return;
    auto status_label = cast<CCLabelBMFont*>(layer->getChildByTag(4001));
    auto info_label = cast<CCLabelBMFont*>(layer->getChildByTag(4002));
    auto cool_sprite = cast<CCSprite*>(layer->getChildByTag(4003));
    const auto win_size = CCDirector::sharedDirector()->getWinSize();
    if (!status_label) {
        status_label = CCLabelBMFont::create("status", "bigFont.fnt");
        status_label->setTag(4001);
        status_label->setOpacity(64);
        status_label->setScale(0.5f);
        status_label->setAnchorPoint({0, 1});
        status_label->setPosition({ 5.f, win_size.height - 2.f });
        status_label->setZOrder(999);
        layer->addChild(status_label);
    }
    if (!info_label) {
        info_label = CCLabelBMFont::create("info", "bigFont.fnt");
        info_label->setTag(4002);
        info_label->setZOrder(999);
        info_label->setScale(0.4f);
        info_label->setOpacity(64);
        info_label->setAnchorPoint({0, 1});
        layer->addChild(info_label);
    }
    if (!cool_sprite) {
        cool_sprite = CCSprite::create();
        cool_sprite->setTag(4003);
        cool_sprite->setZOrder(999);
        layer->addChild(cool_sprite);
    }
    if (layer->m_isPracticeMode)
        status_label->setString("Practice");
    else if (layer->m_isTestMode)
        status_label->setString("Testmode");
    else {
        status_label->setString("");
        info_label->setString("");
        cool_sprite->setVisible(false);
    }
    if (layer->m_isPracticeMode || layer->m_isTestMode) {
        std::stringstream stream;
        stream << "From " << std::floor(g_start_x / layer->m_levelLength * 100.f) << "%";
        info_label->setString(stream.str().c_str());

        bool _show_icon = g_show_icon && (layer->m_isPracticeMode || g_show_start_pos_icon);

        cool_sprite->initWithSpriteFrameName(layer->m_isPracticeMode ? "checkpoint_01_001.png" : "edit_eStartPosBtn_001.png");
        cool_sprite->setOpacity(64);
        cool_sprite->setScale(status_label->getScaledContentSize().height / cool_sprite->getContentSize().height);

        bool top = !(g_position & 0b10);
        bool left = !(g_position & 0b01);

        cool_sprite->setAnchorPoint(ccp(!left, 1));
        status_label->setAnchorPoint(cool_sprite->getAnchorPoint());
        info_label->setAnchorPoint(cool_sprite->getAnchorPoint());

        float x = left ? 5.f : win_size.width - 5.f;
        float y = top ? win_size.height - 2.f : info_label->getScaledContentSize().height + status_label->getScaledContentSize().height + 5.f;
        float offset = cool_sprite->getScaledContentSize().width + 2.f;
        if (!left) offset *= -1.f;

        status_label->setPositionY(y);
        info_label->setPositionY(y - status_label->getScaledContentSize().height - 1.f);

        cool_sprite->setPosition({ x, y - 1.f });
        if (_show_icon) status_label->setPositionX(cool_sprite->getPositionX() + offset);
        else status_label->setPositionX(cool_sprite->getPositionX());
        info_label->setPositionX(cool_sprite->getPositionX());

        status_label->setVisible(g_enabled && g_show_status);
        info_label->setVisible(g_enabled && g_show_info);
        cool_sprite->setVisible(g_enabled && _show_icon);
    }
}

void update_labels() {
    return update_labels(gd::GameManager::sharedState()->getPlayLayer());
}

void load_gm_values();

bool (__thiscall* PlayLayer_init)(gd::PlayLayer*, void*);
bool __fastcall PlayLayer_init_H(gd::PlayLayer* self, int, void* lvl) {
    if (PlayLayer_init(self, lvl)) {
        if (!g_ext)
            load_gm_values();
        if (self->m_isPracticeMode && g_enabled) {
            auto children = self->getChildren();
            for (unsigned int i = 0; i < children->count(); ++i) {
                auto label = dynamic_cast<CCLabelBMFont*>(children->objectAtIndex(i));
                // amazing
                if (label && label->getTag() != 4001 && label->getString()[0] == 'T') {
                    label->setVisible(false);
                    break;
                }
            }
        }

        g_start_x = self->m_player1->m_position.x;
        update_labels(self);
        return true;
    }
    return false;
}

void (__thiscall* PlayLayer_togglePracticeMode)(gd::PlayLayer*, bool);
void __fastcall PlayLayer_togglePracticeMode_H(gd::PlayLayer* self, int, bool toggle) {
    PlayLayer_togglePracticeMode(self, toggle);
    update_labels(self);
}

void* (__thiscall* PlayLayer_resetLevel)(gd::PlayLayer*);
void* __fastcall PlayLayer_resetLevel_H(gd::PlayLayer* self) {
    float die_x = self->m_player1->m_position.x;
    auto ret = PlayLayer_resetLevel(self);
    g_start_x = self->m_player1->m_position.x;
    update_labels(self);
    return ret;
}

void set_gm_values() {
    auto gm = gd::GameManager::sharedState();
    gm->setGameVariable("RI_enabled", g_enabled);
    gm->setGameVariable("RI_show_icon", g_show_icon);
    gm->setGameVariable("RI_show_status", g_show_status);
    gm->setGameVariable("RI_show_info", g_show_info);
    gm->setGameVariable("RI_show_startpos", g_show_start_pos_icon);
    gm->setIntGameVariable("RI_position", g_position);
}

void load_gm_values() {
    auto gm = gd::GameManager::sharedState();
    g_enabled = gm->getGameVariableDefault("RI_enabled", true);
    g_show_icon = gm->getGameVariableDefault("RI_show_icon", true);
    g_show_status = gm->getGameVariableDefault("RI_show_status", true);
    g_show_info = gm->getGameVariableDefault("RI_show_info", true);
    g_show_start_pos_icon = gm->getGameVariableDefault("RI_show_startpos", false);
    g_position = Position(gm->getIntGameVariableDefault("RI_position", 0));
}

// this isnt what its called but im lazy
bool (__thiscall* OptionsLayer_init)(void*);
bool __fastcall OptionsLayer_init_H(void* self) {
    auto ret = OptionsLayer_init(self);
    load_gm_values();
    auto base = cast<uintptr_t>(GetModuleHandle(0));
    auto add_toggle = cast<int (__thiscall*)(void*, const char*, const char*, const char*)>(base + 0x1DF6B0);
    add_toggle(self, "Toggle Run Info", "RI_enabled", "Toggles the run info display mod.");
    add_toggle(self, "RI: Show icon", "RI_show_icon", nullptr);
    add_toggle(self, "RI: Show status", "RI_show_status", nullptr);
    add_toggle(self, "RI: Show info", "RI_show_info", nullptr);
    add_toggle(self, "RI: Show Start Pos icon", "RI_show_startpos", nullptr);
    return ret;
}

void __stdcall cb_toggle_check(void* checkbox) {
    g_enabled = true;
    set_gm_values();
    update_labels();
}
void __stdcall cb_toggle_uncheck(void* checkbox) {
    g_enabled = false;
    set_gm_values();
    update_labels();
}

#define GEN_CHECK_CALLBACK(name) \
void __stdcall cb_##name##_check(void*) { \
    g_##name = true; \
    set_gm_values(); \
    update_labels(); \
} \
void __stdcall cb_##name##_uncheck(void*) { \
    g_##name = false; \
    set_gm_values(); \
    update_labels(); \
}

GEN_CHECK_CALLBACK(show_icon)
GEN_CHECK_CALLBACK(show_status)
GEN_CHECK_CALLBACK(show_info)
GEN_CHECK_CALLBACK(show_start_pos_icon)

void __stdcall cb_combo_changed(void* combo, int i, const char* what) {
    g_position = Position(3 - i);
    set_gm_values();
    update_labels();
}

inline void add_hook(uintptr_t addr, void* hook, void* tramp) {
    MH_CreateHook(cast<void*>(addr), hook, cast<void**>(tramp));
    MH_QueueEnableHook(cast<void*>(addr));
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
    
    add_hook(base + 0x1FB780, PlayLayer_init_H, &PlayLayer_init);
    add_hook(base + 0x20D0D0, PlayLayer_togglePracticeMode_H, &PlayLayer_togglePracticeMode);
    add_hook(base + 0x20BF00, PlayLayer_resetLevel_H, &PlayLayer_resetLevel);

    if (InitialiseHackpro()) {
        if (HackproIsReady()) {
            g_ext = HackproInitialiseExt("Run info");
            if (g_ext) {
                // it is safe to load the values here since mhv6 waits until MenuLayer::init
                // im kinda surprised this works on a different thread
                load_gm_values();
                HackproSetCheckbox(HackproAddCheckbox(g_ext, "Show Start Pos icon", cb_show_start_pos_icon_check, cb_show_start_pos_icon_uncheck), g_show_start_pos_icon);
                HackproSetCheckbox(HackproAddCheckbox(g_ext, "Show info", cb_show_info_check, cb_show_info_uncheck), g_show_info);
                HackproSetCheckbox(HackproAddCheckbox(g_ext, "Show status", cb_show_status_check, cb_show_status_uncheck), g_show_status);
                HackproSetCheckbox(HackproAddCheckbox(g_ext, "Show icon", cb_show_icon_check, cb_show_icon_uncheck), g_show_icon);
                HackproSetCheckbox(HackproAddCheckbox(g_ext, "Toggle", cb_toggle_check, cb_toggle_uncheck), g_enabled);
                auto i = g_position;
                auto combo = HackproAddComboBox(g_ext, cb_combo_changed);
                const char* strs[] = {"Bottom Right", "Bottom Left", "Top Right", "Top Left", 0};
                HackproSetComboBoxStrs(combo, strs);
                HackproSetComboBoxIndex(combo, 3 - i);
                HackproCommitExt(g_ext);
            }
        }
    } else {
        add_hook(base + 0x1DE8F0, OptionsLayer_init_H, &OptionsLayer_init);
    }
    
    MH_ApplyQueued();

#ifdef _DEBUG
    std::getline(std::cin, std::string());
    
    if (g_ext)
        HackproWithdrawExt(g_ext);
    conout.close();
    conin.close();
    FreeConsole();
    FreeLibraryAndExitThread(cast<HMODULE>(hModule), 0);
#endif
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
        CreateThread(0, 0x1000, my_thread, hModule, 0, 0);
    return TRUE;
}