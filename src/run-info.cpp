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
#include "mod-menu-window.hpp"
#include <matdash.hpp>
#include <matdash/minhook.hpp>
#include <matdash/boilerplate.hpp>

using namespace cocos2d;

enum Position : int {
    TOP_LEFT = 0,
    TOP_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT,
};

void* g_ext = nullptr; // to check if u have mhv6

static struct {
    float startX = 0.f;
    bool enabled = true;
    bool showIcon = true;
    bool showStatus = true;
    bool showInfo = true;
    bool showStartPos = false;
    Position position = Position::TOP_LEFT;
} state;

void updateLabels(gd::PlayLayer* layer) {
    if (!layer) return;
    auto statusLabel = cast<CCLabelBMFont*>(layer->getChildByTag(4001));
    auto infoLabel = cast<CCLabelBMFont*>(layer->getChildByTag(4002));
    auto coolSprite = cast<CCSprite*>(layer->getChildByTag(4003));
    const auto win_size = CCDirector::sharedDirector()->getWinSize();

    if (!statusLabel) {
        statusLabel = CCLabelBMFont::create("status", "bigFont.fnt");
        statusLabel->setTag(4001);
        statusLabel->setOpacity(64);
        statusLabel->setScale(0.5f);
        statusLabel->setPosition({ 5.f, win_size.height - 2.f });
        statusLabel->setZOrder(999);
        layer->addChild(statusLabel);
    }
    if (!infoLabel) {
        infoLabel = CCLabelBMFont::create("info", "bigFont.fnt");
        infoLabel->setTag(4002);
        infoLabel->setZOrder(999);
        infoLabel->setScale(0.4f);
        infoLabel->setOpacity(64);
        layer->addChild(infoLabel);
    }
    if (!coolSprite) {
        coolSprite = CCSprite::create();
        coolSprite->setTag(4003);
        coolSprite->setZOrder(999);
        layer->addChild(coolSprite);
    }
    
    if (state.enabled) {
        if (layer->m_isPracticeMode)
            statusLabel->setString("Practice");
        else if (layer->m_isTestMode)
            statusLabel->setString("Testmode");
    } 
    const bool visible = state.enabled && (layer->m_isPracticeMode || layer->m_isTestMode);
    statusLabel->setVisible(visible);
    infoLabel->setVisible(visible);
    coolSprite->setVisible(visible);

    if (visible) {
        const bool top = state.position == Position::TOP_LEFT || state.position == Position::TOP_RIGHT;
        const bool left = state.position == Position::TOP_LEFT || state.position == Position::BOTTOM_LEFT;
        const CCPoint anchor(left ? 0.f : 1.f, top ? 1.f : 0.f);
        const float sign = top ? -1.f : 1.f;
        // it stands for horizontal sign
        const float hign = left ? 1.f : -1.f;

        statusLabel->setAnchorPoint(anchor);
        infoLabel->setAnchorPoint(anchor);
        infoLabel->setAlignment(left ? kCCTextAlignmentLeft : kCCTextAlignmentRight);

        float y = top ? win_size.height - 2.f : 4.f;
        const float baseX = left ? 5.f : win_size.width - 5.f;
        float x = baseX;

        if (state.showIcon && (layer->m_isPracticeMode || state.showStartPos)) {
            coolSprite->initWithSpriteFrameName(layer->m_isPracticeMode ? "checkpoint_01_001.png" : "edit_eStartPosBtn_001.png");
            // init resets the anchor point and opacity, maybe find some other way of updating the texture?
            coolSprite->setAnchorPoint(anchor);
            coolSprite->setOpacity(64);
            coolSprite->setScale(statusLabel->getScaledContentSize().height / coolSprite->getContentSize().height);
            coolSprite->setPositionX(x);
            coolSprite->setPositionY(y - 1.f);
            if (state.showStatus) x += (coolSprite->getScaledContentSize().width + 2.f) * hign;
            else y += coolSprite->getScaledContentSize().height * sign;
        } else
            coolSprite->setVisible(false);
        
        if (state.showStatus) {
            statusLabel->setPositionX(x);
            statusLabel->setPositionY(y);
            y += statusLabel->getScaledContentSize().height * sign;
            if (state.showIcon) x = baseX;
        } else
            statusLabel->setVisible(false);

        if (state.showInfo) {
            infoLabel->setPositionX(x);
            infoLabel->setPositionY(y);
            y += infoLabel->getScaledContentSize().height * sign;

            std::stringstream stream;
            stream << "From " << std::floor(state.startX / layer->m_levelLength * 100.f) << "%"; 
            infoLabel->setString(stream.str().c_str());
        } else
            infoLabel->setVisible(false);
    }
}

void updateLabels() {
    return updateLabels(gd::GameManager::sharedState()->getPlayLayer());
}

bool PlayLayer_init(gd::PlayLayer* self, gd::GJGameLevel* lvl) {
    if (!orig<&PlayLayer_init>(self, lvl)) return false;
    state.startX = self->m_player1->m_position.x;

    // remove the vanilla testmode text, if needed
    static const auto addr = cast<void*>(gd::base + 0x2d7dd8);
    DWORD oldProt;
    VirtualProtect(addr, 1, PAGE_EXECUTE_READWRITE, &oldProt);
    *cast<char*>(addr) = state.enabled ? 0 : 'T';
    VirtualProtect(addr, 1, oldProt, &oldProt);

    self->m_attemptLabel->setVisible(false);
    
    updateLabels(self);
    return true;
}

void PlayLayer_togglePracticeMode(gd::PlayLayer* self, bool toggle) {
    orig<&PlayLayer_togglePracticeMode>(self, toggle);
    updateLabels(self);
}

void PlayLayer_resetLevel(gd::PlayLayer* self) {
    float die_x = self->m_player1->m_position.x;
    orig<&PlayLayer_resetLevel>(self);
    state.startX = self->m_player1->m_position.x;
    updateLabels(self);
}


void mod_main() {
    add_hook<&PlayLayer_init>(gd::base + 0x1FB780);
    add_hook<&PlayLayer_togglePracticeMode>(gd::base + 0x20D0D0);
    add_hook<&PlayLayer_resetLevel>(gd::base + 0x20BF00);

    constexpr auto setup = [](Window& window) {
        // its a msvc only thing to put lambdas directly in templates :(
        // so do this mess instead
        {
            constexpr auto callback = [](int index) {
                state.position = Position(index);
                updateLabels();
            }; window.addList<0, callback>({"Top Left", "Top Right", "Bottom Left", "Bottom Right"});
        }
        {
            constexpr auto callback = [](bool value) {
                state.enabled = value;
                updateLabels();
            }; window.addCheckbox<0, callback>("Toggle");
        }
        {
            constexpr auto callback = [](bool value) {
                state.showIcon = value;
                updateLabels();
            }; window.addCheckbox<1, callback>("Show icon");
        }
        {
            constexpr auto callback = [](bool value) {
                state.showStatus = value;
                updateLabels();
            }; window.addCheckbox<2, callback>("Show status");
        }
        {
            constexpr auto callback = [](bool value) {
                state.showInfo = value;
                updateLabels();
            }; window.addCheckbox<3, callback>("Show info");
        }
        {
            constexpr auto callback = [](bool value) {
                state.showStartPos = value;
                updateLabels();
            }; window.addCheckbox<4, callback>("Show Start Pos icon");
        }
    };
    constexpr auto post = [](Window& window) {
        window.setListIndex<0>(state.position);
        window.setCheckboxState<0>(state.enabled);
        window.setCheckboxState<1>(state.showIcon);
        window.setCheckboxState<2>(state.showInfo);
        window.setCheckboxState<3>(state.showStatus);
        window.setCheckboxState<4>(state.showStartPos);
    };
    Window::get().setup<setup, post>("Run Info");

    if (Window::get().type == Window::ModMenuType::NONE) {
        // TODO:
    }

}
