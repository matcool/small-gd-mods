#include <matdash.hpp>
#include <matdash/minhook.hpp>
#include <matdash/boilerplate.hpp>
#include <gd.h>
#include <fstream>
#include "utils.hpp"
#include "mod-menu-window.hpp"

using namespace cocos2d;

bool enabled = false;
bool active = false;
GLuint texName, myFBO, oldFBO;
static unsigned width = 856;
static unsigned height = 480;

bool PlayLayer_init(gd::PlayLayer* self, gd::GJGameLevel* level) {
    if (enabled) {
        active = true;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, (GLint*)&oldFBO);
        
        glGenTextures(1, &texName);
        glBindTexture(GL_TEXTURE_2D, texName);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        
        glGenFramebuffersEXT(1, &myFBO);
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, myFBO);
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, texName, 0);
    }

    return orig<&PlayLayer_init>(self, level);
}

void PlayLayer_dtor(gd::PlayLayer* self) {
    orig<&PlayLayer_dtor>(self);
    if (active) {
        active = false;
        const auto windowSize = CCDirector::sharedDirector()->getOpenGLView()->getFrameSize();

        glViewport(0, 0, windowSize.width, windowSize.height);
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, oldFBO);
        glDeleteTextures(1, &texName);
        glDeleteFramebuffersEXT(1, &myFBO);
    }
}

void CCDirector_drawScene(CCDirector* self) {
    const auto windowSize = self->getOpenGLView()->getFrameSize();
    if (active) {
        glViewport(0, 0, width, height);
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, myFBO);
    }

    orig<&CCDirector_drawScene>(self);

    if (active) {
        glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, myFBO);
        glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, oldFBO);
        glBlitFramebufferEXT(0, 0, width, height, 0, 0, windowSize.width, windowSize.height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glViewport(0, 0, windowSize.width, windowSize.height);
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, oldFBO);
    }
}

template <class T>
uintptr_t cocos_addr(T func) {
    return **cast<uintptr_t**>(union_cast<uintptr_t>(func) + 2);
}

void mod_main() {
    // AllocConsole();
    // static std::ofstream conout("CONOUT$", std::ios::out);
    // std::cout.rdbuf(conout.rdbuf());
    add_hook<&PlayLayer_init>(gd::base + 0x1FB780);
    add_hook<&PlayLayer_dtor>(gd::base + 0x1FB2C0);
    add_hook<&CCDirector_drawScene>(cocos_addr(&CCDirector::drawScene));

    constexpr auto setup = [](Window& window) {
        constexpr auto callback = [](bool value) {
            enabled = value;
        }; window.addCheckbox<0, callback>("Toggle");

        constexpr auto callback2 = [](const char* value) {
            if (value[0]) {
                if (!active) {
                    try {
                        width = std::stoul(value);
                    } catch (std::invalid_argument) {}
                }
                Window::get().setTextboxText<0>(std::to_string(width).c_str());
            }
        }; window.addTextbox<0, callback2>("width");
        
        constexpr auto callback3 = [](const char* value) {
            if (value[0]) {
                if (!active) {
                    try {
                        height = std::stoul(value);
                    } catch (std::invalid_argument) {}
                }
                Window::get().setTextboxText<1>(std::to_string(height).c_str());
            }
        }; window.addTextbox<1, callback3>("height");
    };
    constexpr auto post = [](Window& window) {
        window.setCheckboxState<0>(enabled);
        window.setTextboxText<0>(std::to_string(width).c_str());
        window.setTextboxText<1>(std::to_string(height).c_str());
    };
    Window::get().setup<setup, post>("internal res");
}
