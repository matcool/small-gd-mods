#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <MinHook.h>
#include <fstream>
#include <iostream>
#include <cocos2d.h>
#include <sstream>
#include "hackpro_ext.h"

using namespace cocos2d;

class PlayerObject : public CCSprite {
	char _pad0[0x498];
public:
	float x_pos;
    float y_pos;
};

class PlayLayer : public CCLayer {
    char _pad0[0x108];
public:
    PlayerObject* player1;
    PlayerObject* player2;
protected:
    char _pad1[0x188];
public:
    float level_length;
protected:
    char _pad2[0xdc];
public:
    bool is_test_mode;
    bool is_practice_mode;
};

float g_start_x = 0.f;
// CCLabelBMFont* status_label = nullptr;
// CCLabelBMFont* info_label = nullptr;
bool g_enabled = true;
bool g_show_icon = true;
bool g_show_status = true; // this can only be false if show_icon is true
bool g_show_info = true;
bool g_show_start_pos_icon = false; // i find it ugly

void* g_ext = nullptr; // to check if u have mhv6

void update_labels(PlayLayer* layer) {
    if (!layer) return;
    auto status_label = reinterpret_cast<CCLabelBMFont*>(layer->getChildByTag(4001));
    auto info_label = reinterpret_cast<CCLabelBMFont*>(layer->getChildByTag(4002));
    auto cool_sprite = reinterpret_cast<CCSprite*>(layer->getChildByTag(4003));
    if (!status_label) {
        status_label = CCLabelBMFont::create("status", "bigFont.fnt");
        status_label->setTag(4001);
        status_label->setOpacity(64);
        status_label->setScale(0.5f);
        status_label->setAnchorPoint({0, 1});
        status_label->setPosition({ 5.f, CCDirector::sharedDirector()->getWinSize().height - 2.f });
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
        info_label->setPosition({ 5.f, status_label->getPositionY() - status_label->getScaledContentSize().height - 1.f });
        layer->addChild(info_label);
    }
    if (!cool_sprite) {
        // cool_sprite = CCSprite::createWithSpriteFrameName("checkpoint_01_001.png");
        cool_sprite = CCSprite::create();
        cool_sprite->setTag(4003);
        cool_sprite->setZOrder(999);
        layer->addChild(cool_sprite);
    }
    if (layer->is_practice_mode)
        status_label->setString("Practice");
    else if (layer->is_test_mode)
        status_label->setString("Testmode");
    else {
        status_label->setString("");
        info_label->setString("");
        cool_sprite->setVisible(false);
    }
    if (layer->is_practice_mode || layer->is_test_mode) {
        std::stringstream stream;
        // stream.precision(2);
        stream << "From " << std::floor(g_start_x / layer->level_length * 100.f) << "%";
        info_label->setString(stream.str().c_str());

        bool _show_icon = g_show_icon && (layer->is_practice_mode || g_show_start_pos_icon);

        cool_sprite->initWithSpriteFrameName(layer->is_practice_mode ? "checkpoint_01_001.png" : "edit_eStartPosBtn_001.png");
        cool_sprite->setOpacity(64);
        cool_sprite->setScale(status_label->getScaledContentSize().height / cool_sprite->getContentSize().height);
        cool_sprite->setAnchorPoint({0, 1});
        cool_sprite->setPosition({ 5.f, status_label->getPositionY() - 1.f });
        if (_show_icon) status_label->setPositionX(cool_sprite->getPositionX() + cool_sprite->getScaledContentSize().width + 2.f);
        else status_label->setPositionX(5.f);
        
        status_label->setVisible(g_enabled && g_show_status);
        info_label->setVisible(g_enabled && g_show_info);
        cool_sprite->setVisible(g_enabled && _show_icon);
    }
}

void load_gm_values();

bool (__thiscall* PlayLayer_init)(PlayLayer*, void*);
bool __fastcall PlayLayer_init_H(PlayLayer* self, int, void* lvl) {
    if (PlayLayer_init(self, lvl)) {
        if (!g_ext)
            load_gm_values();
        if (self->is_test_mode && g_enabled) {
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

        g_start_x = self->player1->x_pos;
        update_labels(self);
        return true;
    }
    return false;
}

void (__thiscall* PlayLayer_togglePracticeMode)(PlayLayer*, bool);
void __fastcall PlayLayer_togglePracticeMode_H(PlayLayer* self, int, bool toggle) {
    PlayLayer_togglePracticeMode(self, toggle);
    update_labels(self);
}

void* (__thiscall* PlayLayer_resetLevel)(PlayLayer*);
void* __fastcall PlayLayer_resetLevel_H(PlayLayer* self) {
    float die_x = self->player1->x_pos;
    auto ret = PlayLayer_resetLevel(self);
    g_start_x = self->player1->x_pos;
    update_labels(self);
    return ret;
}

inline auto add_hook(uintptr_t addr, void* hook, void* tramp) {
    return MH_CreateHook(reinterpret_cast<void*>(addr), hook, reinterpret_cast<void**>(tramp));
}

inline PlayLayer* get_play_layer() {
    return *reinterpret_cast<PlayLayer**>(*reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(GetModuleHandle(0)) + 0x3222D0) + 0x164);
}

inline void* get_game_manager() {
    auto base = reinterpret_cast<uintptr_t>(GetModuleHandle(0));
    return *reinterpret_cast<void**>(base + 0x3222D0);
}

bool get_game_variable(const char* key) {
    auto base = reinterpret_cast<uintptr_t>(GetModuleHandle(0));
    return reinterpret_cast<bool (__thiscall*)(void* self, const char*)>(base + 0xC9D30)(get_game_manager(), key);
}

void set_gm_values() {
    auto base = reinterpret_cast<uintptr_t>(GetModuleHandle(0));
    auto set_value = reinterpret_cast<void (__thiscall*)(void*, const char*, bool)>(base + 0xc9b50);
    auto gm = get_game_manager();
    set_value(gm, "4001", g_enabled);
    set_value(gm, "4002", g_show_icon);
    set_value(gm, "4003", g_show_status);
    set_value(gm, "4004", g_show_info);
    set_value(gm, "4005", g_show_start_pos_icon);
}

void load_gm_values() {
    g_enabled = get_game_variable("4001");
    g_show_icon = get_game_variable("4002");
    g_show_status = get_game_variable("4003");
    g_show_info = get_game_variable("4004");
    g_show_start_pos_icon = get_game_variable("4005");
}

// this isnt what its called but im lazy
bool (__thiscall* OptionsLayer_init)(void*);
bool __fastcall OptionsLayer_init_H(void* self) {
    auto ret = OptionsLayer_init(self);
    auto base = reinterpret_cast<uintptr_t>(GetModuleHandle(0));
    auto add_toggle = reinterpret_cast<int (__thiscall*)(void*, const char*, const char*, const char*)>(base + 0x1DF6B0);
    add_toggle(self, "Toggle Run Info", "4001", "Toggles the run info display mod.");
    add_toggle(self, "RI: Show icon", "4002", nullptr);
    add_toggle(self, "RI: Show status", "4003", nullptr);
    add_toggle(self, "RI: Show info", "4004", nullptr);
    add_toggle(self, "RI: Show Start Pos icon", "4005", nullptr);
    return ret;
}

void __stdcall cb_toggle_check(void* checkbox) {
    g_enabled = true;
    update_labels(get_play_layer());
}
void __stdcall cb_toggle_uncheck(void* checkbox) {
    g_enabled = false;
    update_labels(get_play_layer());
}

void* g_cb_show_icon;
void* g_cb_show_status;

void __stdcall cb_show_icon_check(void* checkbox) {
    g_show_icon = true;
    update_labels(get_play_layer());
}
void __stdcall cb_show_status_check(void*); // crazy
void __stdcall cb_show_icon_uncheck(void* checkbox) {
    g_show_icon = false;
    if (!g_show_status) {
        HackproSetCheckbox(g_cb_show_status, true);
        cb_show_status_check(g_cb_show_status);
    }
    update_labels(get_play_layer());
}

void __stdcall cb_show_status_check(void* checkbox) {
    g_show_status = true;
    update_labels(get_play_layer());
}
void __stdcall cb_show_status_uncheck(void* checkbox) {
    g_show_status = false;
    if (!g_show_icon) {
        HackproSetCheckbox(g_cb_show_icon, true);
        cb_show_icon_check(g_cb_show_icon);
    }
    update_labels(get_play_layer());
}

void __stdcall cb_show_info_check(void* checkbox) {
    g_show_info = true;
    update_labels(get_play_layer());
}
void __stdcall cb_show_info_uncheck(void* checkbox) {
    g_show_info = false;
    update_labels(get_play_layer());
}

void __stdcall cb_show_start_pos_icon_check(void* checkbox) {
    g_show_start_pos_icon = true;
    update_labels(get_play_layer());
}
void __stdcall cb_show_start_pos_icon_uncheck(void* checkbox) {
    g_show_start_pos_icon = false;
    update_labels(get_play_layer());
}

DWORD WINAPI my_thread(void* hModule) {
#ifdef _DEBUG
    AllocConsole();
    std::ofstream conout("CONOUT$", std::ios::out);
    std::ifstream conin("CONIN$", std::ios::in);
    std::cout.rdbuf(conout.rdbuf());
    std::cin.rdbuf(conin.rdbuf());
#endif
    MH_Initialize();

    auto base = reinterpret_cast<uintptr_t>(GetModuleHandle(0));
    
    add_hook(base + 0x1FB780, PlayLayer_init_H, &PlayLayer_init);
    add_hook(base + 0x20D0D0, PlayLayer_togglePracticeMode_H, &PlayLayer_togglePracticeMode);
    add_hook(base + 0x20BF00, PlayLayer_resetLevel_H, &PlayLayer_resetLevel);

    if (InitialiseHackpro()) {
        if (HackproIsReady()) {
            g_ext = HackproInitialiseExt("Run info");
            HackproAddCheckbox(g_ext, "Show Start Pos icon", cb_show_start_pos_icon_check, cb_show_start_pos_icon_uncheck);
            HackproSetCheckbox(HackproAddCheckbox(g_ext, "Show info", cb_show_info_check, cb_show_info_uncheck), true);
            HackproSetCheckbox((g_cb_show_status = HackproAddCheckbox(g_ext, "Show status", cb_show_status_check, cb_show_status_uncheck)), true);
            HackproSetCheckbox((g_cb_show_icon = HackproAddCheckbox(g_ext, "Show icon", cb_show_icon_check, cb_show_icon_uncheck)), true);
            HackproSetCheckbox(HackproAddCheckbox(g_ext, "Toggle", cb_toggle_check, cb_toggle_uncheck), true);
            HackproCommitExt(g_ext);
        }
    } else {
        set_gm_values();
        add_hook(base + 0x1DE8F0, OptionsLayer_init_H, &OptionsLayer_init);
    }
    
    MH_EnableHook(MH_ALL_HOOKS);

#ifdef _DEBUG
    std::getline(std::cin, std::string());
    
    if (g_ext)
        HackproWithdrawExt(g_ext);
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