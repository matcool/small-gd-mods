#define WIN32_LEAN_AND_MEAN
#include "Windows.h"
#include <fstream>
#include <iostream>
#include <cocos2d.h>
#include <gd.h>
#include "utils.hpp"
#include <MinHook.h>
#include <chrono>
#include <CCGL.h>
#include <thread>

using namespace cocos2d;

class MyRenderTexture : public CCRenderTexture {
public:
    unsigned char* getData() {
        const auto size = m_pTexture->getContentSizeInPixels();
        auto savedBufferWidth = (int)size.width;
        auto savedBufferHeight = (int)size.height;
        GLubyte* pTempData = nullptr;
        pTempData = new GLubyte[savedBufferWidth * savedBufferWidth * 4];
        this->begin();
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, savedBufferWidth, savedBufferHeight, GL_RGBA, GL_UNSIGNED_BYTE, pTempData);
        this->end();
        return pTempData;
    }
    const auto& getSizeInPixels() {
        return m_pTexture->getContentSizeInPixels();
    }
};

using uint = unsigned int;
struct _thread_data {
    unsigned char* data;
    uint src_width, src_height;
    uint x, y, width, height;
};

void copy_screenshot(unsigned char* data, const CCSize& size, uint x = 0, uint y = 0, uint a = 0, uint b = 0) {
    const auto src_width = static_cast<uint>(size.width);
    const auto src_height = static_cast<uint>(size.height);
    a = a ? a : src_width;
    b = b ? b : src_height;
    std::thread([=]() {
        auto len = static_cast<size_t>((a - x) * (b - y));
        auto buffer = cast<uint32_t*>(malloc(len * sizeof(uint32_t)));
        auto cx = x, cy = y;
        for (size_t i = 0; i < len; ++i) {
            const size_t j = ((src_height - cy) * src_width + cx) << 2;
            if (++cx == a) ++cy, cx = x;
            buffer[i] = data[j] << 16 | data[j + 1] << 8 | data[j + 2];

            // const auto n = reinterpret_cast<uint32_t*>(data)[i];
            // buffer[i] = (n & 0x0000FF) << 16 | (n & 0x00FF00) | (n & 0xFF0000) >> 16;
        }
        auto bitmap = CreateBitmap((a - x), (b - y), 1, 32, buffer);

        if (OpenClipboard(NULL))
            if (EmptyClipboard()) {
                SetClipboardData(CF_BITMAP, bitmap);
                CloseClipboard();
            }
        free(buffer);
        free(data);
    }).detach();
}
void copy_screenshot(MyRenderTexture* texture) {
    copy_screenshot(texture->getData(), texture->getSizeInPixels());
}

class ImageAreaSelectLayer : public gd::FLAlertLayer {
    unsigned char* m_data;
    CCSize m_texture_size;
    CCDrawNode* m_stencil;
    CCPoint m_start_pos, m_end_pos;
    CCRect m_sprite_rect;
    bool init(MyRenderTexture* tex) {
        if (!initWithColor({0, 0, 0, 100})) return false;

        m_pLayer = CCLayer::create();
        addChild(m_pLayer);

        m_data = tex->getData();
        m_texture_size = tex->getSizeInPixels();

        const auto& winSize = CCDirector::sharedDirector()->getWinSize();

        auto image = tex->newCCImage();
        image->autorelease();
        auto texture2d = new CCTexture2D;
        texture2d->autorelease();
        texture2d->initWithImage(image);
        auto sprite = CCSprite::createWithTexture(texture2d);
        sprite->setPosition(winSize / 2);
        sprite->setScale(0.75f);
        m_pLayer->addChild(sprite);

        {
            const auto size = sprite->getScaledContentSize();
            const auto pos = sprite->getPosition() - size / 2;
            m_sprite_rect = CCRect(pos.x, pos.y, size.width, size.height);
            m_start_pos = pos;
            m_end_pos = pos + size;
        }

        this->setKeypadEnabled(true);
        this->setTouchEnabled(true);

        auto overlay = CCSprite::create("square.png");
        overlay->setColor({0, 0, 0});
        overlay->setOpacity(200);
        overlay->setScaleX(sprite->getScaledContentSize().width / overlay->getContentSize().width);
        overlay->setScaleY(sprite->getScaledContentSize().height / overlay->getContentSize().height);
        overlay->setPosition(winSize / 2);

        m_stencil = CCDrawNode::create();
        update_stencil();

        auto clip = CCClippingNode::create(m_stencil);
        clip->addChild(overlay);
        clip->setAlphaThreshold(0.0);
        clip->setInverted(true);
        m_pLayer->addChild(clip);
        
        return true;
    }

    void update_stencil() {
        m_stencil->clear();
        CCPoint verts[4] = {
            ccp(m_start_pos.x, m_start_pos.y),
            ccp(m_start_pos.x, m_end_pos.y),
            ccp(m_end_pos.x, m_end_pos.y),
            ccp(m_end_pos.x, m_start_pos.y)
        };
        m_stencil->drawPolygon(verts, 4, {1, 1, 1, 1}, 0, {0, 0, 0, 0});
    }

    virtual void ccTouchMoved(CCTouch *pTouch, CCEvent *pEvent) {
        gd::FLAlertLayer::ccTouchMoved(pTouch, pEvent);
        m_start_pos = pTouch->getStartLocation();
        m_end_pos = pTouch->getLocation();
        update_stencil();
    }

    virtual void keyDown(enumKeyCodes key) {
        gd::FLAlertLayer::keyDown(key);
        if (key == enumKeyCodes::KEY_Enter) {
            auto xf = (m_start_pos.x - m_sprite_rect.origin.x) / m_sprite_rect.size.width;
            auto yf = (m_start_pos.y - m_sprite_rect.origin.y) / m_sprite_rect.size.height;
            auto af = (m_end_pos.x - m_sprite_rect.origin.x) / m_sprite_rect.size.width;
            auto bf = (m_end_pos.y - m_sprite_rect.origin.y) / m_sprite_rect.size.height;
            if (xf > af) std::swap(xf, af);
            if (bf > yf) std::swap(yf, bf);
            const auto width = m_texture_size.width;
            const auto height = m_texture_size.height;
            uint x = static_cast<uint>(std::clamp(xf * width, 0.f, width));
            uint y = static_cast<uint>(std::clamp((1.f - yf) * height, 0.f, height));
            uint a = static_cast<uint>(std::clamp(af * width, 0.f, width));
            uint b = static_cast<uint>(std::clamp((1.f - bf) * height, 0.f, height));
            copy_screenshot(m_data, m_texture_size, x, y, a, b);
            m_data = 0;
            keyBackClicked();
        }
    }

    virtual void keyBackClicked() {
        if (m_data) free(m_data);
        gd::FLAlertLayer::keyBackClicked();
    }

public:
    static auto create(MyRenderTexture* tex) {
        auto ret = new ImageAreaSelectLayer();
        if (ret && ret->init(tex)) {
            ret->autorelease();
        } else {
            CC_SAFE_DELETE(ret);
        }
        return ret;
    }
};

void (__thiscall* dispatchKeyboardMSG)(void* self, int key, bool down);
void __fastcall dispatchKeyboardMSG_H(void* self, void*, int key, bool down) {
    if (down && key == enumKeyCodes::KEY_F2) {
        auto start = std::chrono::high_resolution_clock::now();
        auto director = CCDirector::sharedDirector();
        auto winSize = director->getWinSize();
        auto texture = cast<MyRenderTexture*>(CCRenderTexture::create(winSize.width, winSize.height));
        auto scene = director->getRunningScene();
        
        texture->begin();
        scene->visit();
        texture->end();

        if (director->getKeyboardDispatcher()->getShiftKeyPressed()) {
            ImageAreaSelectLayer::create(texture)->show();
        } else {
            copy_screenshot(texture);
        }
    }
    dispatchKeyboardMSG(self, key, down);
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

    auto cocos = GetModuleHandleA("libcocos2d.dll");
    MH_CreateHook(
        GetProcAddress(cocos, "?dispatchKeyboardMSG@CCKeyboardDispatcher@cocos2d@@QAE_NW4enumKeyCodes@2@_N@Z"),
        dispatchKeyboardMSG_H,
        reinterpret_cast<void**>(&dispatchKeyboardMSG)
    );

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