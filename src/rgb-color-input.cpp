#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "mapped-hooks.hpp"
#include "utils.hpp"
#include <cocos2d.h>
#include <extensions/GUI/CCControlExtension/CCControlColourPicker.h>
#include <extensions/GUI/CCControlExtension/CCScale9Sprite.h>
#include <gd.h>
#include <fstream>
#include <iostream>

using namespace cocos2d;

uintptr_t base;

using EffectGameObject = gd::GameObject;

using ColorAction = CCNode;

class ColorPickerDelegate {
    virtual void colorValueChanged(ccColor3B color);
};

class GJSpecialColorSelectDelegate {
    virtual void idfk();
};

class TextInputDelegate {
    virtual void idfk2();
};

void (__thiscall* CCControlColourPicker_setColorValue)(extension::CCControlColourPicker*, const ccColor3B&);

class ColorSelectPopup : public gd::FLAlertLayer, ColorPickerDelegate, TextInputDelegate, GJSpecialColorSelectDelegate {
public:
	extension::CCControlColourPicker* colorPicker; //0x01D8
	PAD(4);
	CCLabelBMFont *label; //0x01E0
	PAD(4);
	CCLayer *slider; //0x01E8
	EffectGameObject *effectGameObject; //0x01EC
	CCArray *N00000659; //0x01F0
	gd::CCMenuItemToggler *toggler1; //0x01F4
	gd::CCMenuItemToggler *toggler2; //0x01F8
	PAD(4);
	CCSprite *N0000065D; //0x0200
	CCSprite *N0000065E; //0x0204
	PAD(4);
	CCLayer *colorSetupLayer; //0x020C
	PAD(16);
	ColorAction *colorAction; //0x0220
	gd::CCTextInputNode *N00000643; //0x0224
	bool N00000644; //0x0228
	bool touchTrigger; //0x0229
	bool N000006B7; //0x22A
	bool isMultipleColorTrigger; //0x022B
	bool N00000645; //0x022C
	bool isColorTrigger; //0x022D
	PAD(2);
	int colorID; //0x0230
	bool N00000647; //0x0234
	PAD(3);
	int copyChannelID; //0x0238
	bool copyOpacity; //0x023C
	PAD(3);
	CCNode *configurehsvwidget; //0x0240
	PAD(16);
	CCArray *N00000650; //0x0254
	CCArray *N00000651; //0x0258
	gd::CCTextInputNode *textinputnode; //0x025C
    PAD(24);
	bool spawnTrigger; //0x0278
	bool multiTrigger; //0x0279
	bool copyColor; //0x027A
	PAD(1);

    virtual void keyDown(int key);
    virtual void colorValueChanged(ccColor3B color);
    virtual void idfk();
    virtual void idfk2();

    inline auto getAlertLayer() { return m_pLayer; }
    inline auto getPickerColor() { return *cast<ccColor3B*>(cast<uintptr_t>(colorPicker) + 0x144); }
    inline void setPickerColor(const ccColor3B color) { CCControlColourPicker_setColorValue(colorPicker, color); }
};

std::string color_to_hex(ccColor3B color) {
    static const auto digits = "0123456789ABCDEF";
    char buffer[6];
    unsigned int num = (color.r << 16) | (color.g << 8) | color.b;
    for (unsigned int i = 0; i < 6; i += 2) {
        auto j = 5 - i - 1;
        buffer[j] = digits[(num >> (i * 4 + 4)) & 0xF];
        buffer[j + 1] = digits[(num >> (i * 4)) & 0xF];
    }
    return std::string(buffer, 6);
}

class RGBColorInputWidget : public CCLayer, public CCTextFieldDelegate {
    ColorSelectPopup* parent;
    gd::CCTextInputNode* red_input;
    gd::CCTextInputNode* green_input;
    gd::CCTextInputNode* blue_input;
    gd::CCTextInputNode* hex_input;

    bool init(ColorSelectPopup* parent) {
        if (!CCLayer::init()) return false;
        this->parent = parent;

        registerWithTouchDispatcher();
        CCDirector::sharedDirector()->getTouchDispatcher()->incrementForcePrio(2);

        const ccColor3B placeholder_color = {100, 100, 100};

        constexpr float total_w = 115.f;
        constexpr float spacing = 4.f;
        constexpr float comp_width = (total_w - spacing * 2.f) / 3.f; // components (R G B) width
        constexpr float comp_height = 22.5f;
        constexpr float hex_height = 30.f;
        constexpr float hex_y = -hex_height - spacing / 2.f;
        constexpr float r_xpos = -comp_width - spacing;
        constexpr float b_xpos = -r_xpos;
        constexpr float bg_scale = 1.6f;
        constexpr GLubyte opacity = 100;

        red_input = gd::CCTextInputNode::create("R", this, "bigFont.fnt", 30.f, 20.f);
        red_input->setAllowedChars("0123456789");
        red_input->setMaxLabelLength(3);
        red_input->setMaxLabelScale(0.6f);
        red_input->setLabelPlaceholderColor(placeholder_color);
        red_input->setLabelPlaceholerScale(0.5f);
        red_input->setPositionX(r_xpos);

        green_input = gd::CCTextInputNode::create("G", this, "bigFont.fnt", 30.f, 20.f);
        green_input->setAllowedChars("0123456789");
        green_input->setMaxLabelLength(3);
        green_input->setMaxLabelScale(0.6f);
        green_input->setLabelPlaceholderColor(placeholder_color);
        green_input->setLabelPlaceholerScale(0.5f);
        green_input->setPositionX(0.f);
        
        blue_input = gd::CCTextInputNode::create("B", this, "bigFont.fnt", 30.f, 20.f);
        blue_input->setAllowedChars("0123456789");
        blue_input->setMaxLabelLength(3);
        blue_input->setMaxLabelScale(0.6f);
        blue_input->setLabelPlaceholderColor(placeholder_color);
        blue_input->setLabelPlaceholerScale(0.5f);
        blue_input->setPositionX(b_xpos);

        hex_input = gd::CCTextInputNode::create("hex", this, "bigFont.fnt", 100.f, 20.f);
        hex_input->setAllowedChars("0123456789ABCDEFabcdef");
        hex_input->setMaxLabelLength(6);
        hex_input->setMaxLabelScale(0.7f);
        hex_input->setLabelPlaceholderColor(placeholder_color);
        hex_input->setLabelPlaceholerScale(0.5f);
        hex_input->setPositionY(hex_y);

        addChild(red_input);
        addChild(green_input);
        addChild(blue_input);
        addChild(hex_input);

        update_labels(true, true);

        auto bg = extension::CCScale9Sprite::create("square02_small.png");
        bg->setContentSize({total_w * bg_scale, hex_height * bg_scale});
        bg->setScale(1.f / bg_scale);
        bg->setOpacity(opacity);
        bg->setZOrder(-1);
        bg->setPositionY(hex_y);
        addChild(bg);

        bg = extension::CCScale9Sprite::create("square02_small.png");
        bg->setContentSize({comp_width * bg_scale, comp_height * bg_scale});
        bg->setScale(1.f / bg_scale);
        bg->setOpacity(opacity);
        bg->setZOrder(-1);
        bg->setPositionX(r_xpos);
        addChild(bg);

        bg = extension::CCScale9Sprite::create("square02_small.png");
        bg->setContentSize({comp_width * bg_scale, comp_height * bg_scale});
        bg->setScale(1.f / bg_scale);
        bg->setOpacity(opacity);
        bg->setZOrder(-1);
        addChild(bg);

        bg = extension::CCScale9Sprite::create("square02_small.png");
        bg->setContentSize({comp_width * bg_scale, comp_height * bg_scale});
        bg->setScale(1.f / bg_scale);
        bg->setOpacity(opacity);
        bg->setZOrder(-1);
        bg->setPositionX(b_xpos);
        addChild(bg);
        
        setKeypadEnabled(true);
        setKeyboardEnabled(true);
        setTouchEnabled(true);

        return true;
    }

    bool ignore = false; // lmao this is such a hacky fix
public:
    void update_labels(bool hex, bool rgb) {
        if (ignore) return;
        ignore = true;
        auto color = parent->getPickerColor();
        if (hex) {
            hex_input->setString(color_to_hex(color).c_str());
        }
        if (rgb) {
            red_input->setString(std::to_string(color.r).c_str());
            green_input->setString(std::to_string(color.g).c_str());
            blue_input->setString(std::to_string(color.b).c_str());
        }
        ignore = false;
    }

    void text_changed(gd::CCTextInputNode* input) {
        if (ignore) return;
        if (input == hex_input) {
            std::string value(input->getString());
            ccColor3B color;
            if (value.empty()) return;
            auto num_value = std::stoi(value, 0, 16);
            if (value.size() == 6) {
                // please shut up about narrowing conversions
                auto r = static_cast<uint8_t>((num_value & 0xFF0000) >> 16);
                auto g = static_cast<uint8_t>((num_value & 0x00FF00) >> 8);
                auto b = static_cast<uint8_t>((num_value & 0x0000FF));
                color = {r, g, b};
            } else if (value.size() == 2) {
                auto number = static_cast<uint8_t>(num_value); // please shut up c++
                color = {number, number, number};
            } else {
                return;
            }
            ignore = true;
            parent->setPickerColor(color);
            ignore = false;
            update_labels(false, true);
        } else if (input == red_input || input == green_input || input == blue_input) {
            std::string value(input->getString());
            auto _num = value.empty() ? 0 : std::stoi(value);
            if (_num > 255) {
                _num = 255;
                input->setString("255");
            }
            GLubyte num = static_cast<GLubyte>(_num);
            auto color = parent->getPickerColor();
            if (input == red_input)
                color.r = num;
            else if (input == green_input)
                color.g = num;
            else if (input == blue_input)
                color.b = num;
            ignore = true;
            parent->setPickerColor(color);
            ignore = false;
            update_labels(true, false);
        }
    }

    static auto create(ColorSelectPopup* parent) {
        auto pRet = new RGBColorInputWidget();
        if (pRet && pRet->init(parent)) {
            pRet->autorelease();
            return pRet;
        } else {
            delete pRet;
            pRet = 0;
            return pRet;
        }
    }
};

RGBColorInputWidget* g_widget = nullptr;

bool __fastcall ColorSelectPopup_init(ColorSelectPopup* self, void*, EffectGameObject* eff_obj, CCArray* arr, ColorAction* action) {
    if (!MHook::getOriginal(ColorSelectPopup_init)(self, 0, eff_obj, arr, action)) return false;
    auto layer = self->getAlertLayer();
    auto widget = RGBColorInputWidget::create(self);
    g_widget = widget;
    auto center = CCDirector::sharedDirector()->getWinSize() / 2.f;
    if (self->isColorTrigger)
        widget->setPosition({center.width - 155.f, center.height + 29.f});
    else
        widget->setPosition({center.width + 127.f, center.height - 90.f});
    layer->addChild(widget);
    widget->setVisible(!self->copyColor);

    return true;
}

void __fastcall CCTextInputNode_refreshLabel(gd::CCTextInputNode* self) {
    MHook::getOriginal(CCTextInputNode_refreshLabel)(self);
    if (g_widget)
        g_widget->text_changed(self);
}

void __fastcall ColorSelectPopup_colorValueChanged(ColorSelectPopup* self) {
    MHook::getOriginal(ColorSelectPopup_colorValueChanged)(self);
    if (g_widget)
        g_widget->update_labels(true, true);
}

void __fastcall ColorSelectPopup_updateCopyColorIdfk(ColorSelectPopup* self) {
    MHook::getOriginal(ColorSelectPopup_updateCopyColorIdfk)(self);
    if (g_widget)
        g_widget->setVisible(!self->copyColor);
}

void __fastcall ColorSelectPopup_dtor(void* self) {
    g_widget = nullptr;
    MHook::getOriginal(ColorSelectPopup_dtor)(self);
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

    auto addr = GetProcAddress(GetModuleHandleA("libExtensions.dll"), "?setColorValue@CCControlColourPicker@extension@cocos2d@@UAEXABU_ccColor3B@3@@Z");
    CCControlColourPicker_setColorValue = cast<decltype(CCControlColourPicker_setColorValue)>(addr);
    
    MHook::registerHook(base + 0x43ae0, ColorSelectPopup_init);
    MHook::registerHook(base + 0x21330, CCTextInputNode_refreshLabel);
    MHook::registerHook(base + 0x46f30, ColorSelectPopup_colorValueChanged);
    MHook::registerHook(base + 0x479c0, ColorSelectPopup_updateCopyColorIdfk);
    MHook::registerHook(base + 0x20c60, ColorSelectPopup_dtor);

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