#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "utils.hpp"
#include <cocos2d.h>
#include <cocos-ext.h>
#include <gd.h>
#include <fstream>
#include <iostream>
#include <matdash.hpp>
#include <matdash/minhook.hpp>
#include <matdash/boilerplate.hpp>

using namespace cocos2d;
using namespace matdash;

inline std::string color_to_hex(ccColor3B color) {
    static constexpr auto digits = "0123456789ABCDEF";
    std::string output;
    output += digits[color.r >> 4 & 0xF];
    output += digits[color.r      & 0xF];
    output += digits[color.g >> 4 & 0xF];
    output += digits[color.g      & 0xF];
    output += digits[color.b >> 4 & 0xF];
    output += digits[color.b      & 0xF];
    return output;
}

class RGBColorInputWidget : public CCLayer, public gd::TextInputDelegate {
    gd::ColorSelectPopup* parent;
    gd::CCTextInputNode* red_input;
    gd::CCTextInputNode* green_input;
    gd::CCTextInputNode* blue_input;
    gd::CCTextInputNode* hex_input;

    bool init(gd::ColorSelectPopup* parent) {
        if (!CCLayer::init()) return false;
        this->parent = parent;

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
        red_input->setLabelPlaceholderScale(0.5f);
        red_input->setPositionX(r_xpos);
        red_input->setDelegate(this);

        green_input = gd::CCTextInputNode::create("G", this, "bigFont.fnt", 30.f, 20.f);
        green_input->setAllowedChars("0123456789");
        green_input->setMaxLabelLength(3);
        green_input->setMaxLabelScale(0.6f);
        green_input->setLabelPlaceholderColor(placeholder_color);
        green_input->setLabelPlaceholderScale(0.5f);
        green_input->setPositionX(0.f);
        green_input->setDelegate(this);
        
        blue_input = gd::CCTextInputNode::create("B", this, "bigFont.fnt", 30.f, 20.f);
        blue_input->setAllowedChars("0123456789");
        blue_input->setMaxLabelLength(3);
        blue_input->setMaxLabelScale(0.6f);
        blue_input->setLabelPlaceholderColor(placeholder_color);
        blue_input->setLabelPlaceholderScale(0.5f);
        blue_input->setPositionX(b_xpos);
        blue_input->setDelegate(this);

        hex_input = gd::CCTextInputNode::create("hex", this, "bigFont.fnt", 100.f, 20.f);
        hex_input->setAllowedChars("0123456789ABCDEFabcdef");
        hex_input->setMaxLabelLength(6);
        hex_input->setMaxLabelScale(0.7f);
        hex_input->setLabelPlaceholderColor(placeholder_color);
        hex_input->setLabelPlaceholderScale(0.5f);
        hex_input->setPositionY(hex_y);
        hex_input->setDelegate(this);

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

        return true;
    }

    bool ignore = false; // lmao this is such a hacky fix

    virtual void textChanged(gd::CCTextInputNode* input) {
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

    static auto create(gd::ColorSelectPopup* parent) {
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

bool ColorSelectPopup_init(gd::ColorSelectPopup* self, gd::EffectGameObject* eff_obj, CCArray* arr, gd::ColorAction* action) {
    if (!orig<&ColorSelectPopup_init>(self, eff_obj, arr, action)) return false;
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

void ColorSelectPopup_colorValueChanged(gd::ColorSelectPopup* self) {
    orig<&ColorSelectPopup_colorValueChanged>(self);
    if (g_widget)
        g_widget->update_labels(true, true);
}

void ColorSelectPopup_updateCopyColorIdfk(gd::ColorSelectPopup* self) {
    orig<&ColorSelectPopup_updateCopyColorIdfk>(self);
    if (g_widget)
        g_widget->setVisible(!self->copyColor);
}

void ColorSelectPopup_dtor(void* self) {
    g_widget = nullptr;
    orig<&ColorSelectPopup_dtor>(self);
}

bool SetupPulsePopup_init(gd::SetupPulsePopup* self, gd::EffectGameObject* eff_obj, CCArray* arr) {
    if (!orig<&SetupPulsePopup_init>(self, eff_obj, arr)) return false;

    auto layer = cast<gd::ColorSelectPopup*>(self)->getAlertLayer();
    auto widget = RGBColorInputWidget::create(cast<gd::ColorSelectPopup*>(self)); // the color picker is in the same offset in both classe s
    g_widget = widget;
    auto center = CCDirector::sharedDirector()->getWinSize() / 2.f;
    self->colorPicker->setPositionX(self->colorPicker->getPositionX() + 3.7f);
    widget->setPosition({center.width - 132.f, center.height + 32.f});
    auto square_width = self->currentColorSpr->getScaledContentSize().width;
    auto x = widget->getPositionX() - square_width / 2.f;
    self->currentColorSpr->setPosition({x, center.height + 85.f});
    self->prevColorSpr->setPosition({x + square_width, center.height + 85.f});
    layer->addChild(widget);
    widget->setVisible(self->pulseMode == 0);

    return true;
}
void SetupPulsePopup_colorValueChanged(void* self) {
    orig<&SetupPulsePopup_colorValueChanged>(self);
    if (g_widget)
        g_widget->update_labels(true, true);
}
void SetupPulsePopup_switchPulseModeIDK(gd::SetupPulsePopup* self) {
    orig<&SetupPulsePopup_switchPulseModeIDK>(self);
    if (g_widget)
        g_widget->setVisible(self->pulseMode == 0);
}
void SetupPulsePopup_dtor(void* self) {
    g_widget = nullptr;
    orig<&SetupPulsePopup_dtor>(self);
}

void mod_main(HMODULE) {
    add_hook<&ColorSelectPopup_init>(gd::base + 0x43ae0);
    add_hook<&ColorSelectPopup_colorValueChanged>(gd::base + 0x46f30);
    add_hook<&ColorSelectPopup_updateCopyColorIdfk>(gd::base + 0x479c0);
    add_hook<&ColorSelectPopup_dtor>(gd::base + 0x43900);

    add_hook<&SetupPulsePopup_init>(gd::base + 0x23e980);
    add_hook<&SetupPulsePopup_colorValueChanged>(gd::base + 0x2426b0);
    add_hook<&SetupPulsePopup_switchPulseModeIDK>(gd::base + 0x242cf0);
    add_hook<&SetupPulsePopup_dtor>(gd::base + 0x23e7b0);
}