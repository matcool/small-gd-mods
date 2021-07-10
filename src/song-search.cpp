#define WIN32_LEAN_AND_MEAN
#define FTS_FUZZY_MATCH_IMPLEMENTATION
#include "mapped-hooks.hpp"
#include "Windows.h"
#include <fstream>
#include <iostream>
#include <cocos2d.h>
#include <cocos-ext.h>
#include <gd.h>
#include <sstream>
#include "../libraries/fts_fuzzy_match.h"
#include "utils.hpp"
#include <deque>

using namespace cocos2d;

constexpr int n_widgets = 6;
constexpr float widget_width = 350.f;
constexpr float widget_height = 50.f;
constexpr float widget_spacing = 5.f;

class SimpleSongWidget;

class CustomSongSearchLayer : public gd::FLAlertLayer, gd::TextInputDelegate, gd::FLAlertLayerProtocol {
public:
    gd::CustomSongLayer* m_parent;
    gd::CCTextInputNode* m_input;
    // CCAction* m_throttle_action = nullptr;
    CCNode* m_div;
    gd::LevelSettingsObject* m_settings;
    std::vector<std::pair<std::string, gd::SongInfoObject*>> m_downloaded_songs;
    float m_y_scroll = 0;
    float m_prev_y_scroll = 0;
    std::deque<SimpleSongWidget*> m_scroll_elements;
    std::vector<gd::SongInfoObject*> m_sorted;
    CCLabelBMFont* m_total_songs_label;
    CCPoint m_prev_mouse_pos;
    gd::CCMenuItemSpriteExtra* m_delete_button;
    bool init(gd::CustomSongLayer* parent) {
        if (!initWithColor({0, 0, 0, 150})) return false;
        m_pLayer = CCLayer::create();
        addChild(m_pLayer);

        m_parent = parent;
        m_settings = parent->m_levelSettings;

        auto win_size = CCDirector::sharedDirector()->getWinSize();

        registerWithTouchDispatcher();
        CCDirector::sharedDirector()->getTouchDispatcher()->incrementForcePrio(2);

        m_div = CCNode::create();
        m_div->setPosition({win_size.width / 2, 0});

        constexpr float total_height = widget_height * (n_widgets - 1);

        // look, i really tried to use a CCDrawNode here
        // but it just kept fucking up
        // so instead make my own 1x1 white dot sprite
        // cant rely on square.png as some people replace it
        // for no particles

        auto texture = new CCTexture2D();
        texture->autorelease();
        const unsigned char data[3] = {255, 255, 255};
        texture->initWithData(data, kCCTexture2DPixelFormat_RGB888, 1, 1, {1, 1});
        texture->setAliasTexParameters();
        auto stencil = CCSprite::createWithTexture(texture);
        stencil->setScaleX(win_size.width / stencil->getContentSize().width);
        stencil->setScaleY(total_height / stencil->getContentSize().height);
        stencil->setAnchorPoint({0, 0});

        auto clip = CCClippingNode::create(stencil);
        clip->addChild(m_div);
        clip->setAlphaThreshold(0.1f);
        m_pLayer->addChild(clip);

        m_input = gd::CCTextInputNode::create("Song Search", this, "bigFont.fnt", 200.f, 50.f);
        m_input->setPosition({win_size.width / 2, total_height + 28.51f});
        m_input->setLabelPlaceholderColor({200, 200, 200});
        m_input->setAllowedChars("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQSRTUVWXYZ0123456789-_+=$!#&*\"\':@()[]{}.,;/?\\~^ <>%");
        m_input->setDelegate(this);
        m_pLayer->addChild(m_input);

        auto bg = extension::CCScale9Sprite::create("GJ_square05.png");
        bg->setContentSize({widget_width + 50.f, 60.f});
        bg->setPosition(m_input->getPosition());
        bg->setZOrder(-1);
        m_pLayer->addChild(bg);

        setKeypadEnabled(true);
        setTouchEnabled(true);
        setMouseEnabled(true);

        auto songs_ = gd::MusicDownloadManager::sharedState()->getDownloadedSongs();
        CCObject* ob;
        CCARRAY_FOREACH(songs_, ob) {
            auto song = cast<gd::SongInfoObject*>(ob);
            std::stringstream builder;
            builder << song->m_songName << " " << song->m_artistName;
            m_downloaded_songs.push_back({builder.str(), song});
        }

        m_total_songs_label = CCLabelBMFont::create("", "goldFont.fnt");
        m_total_songs_label->setAnchorPoint({1, 1});
        m_total_songs_label->setScale(0.4f);
        m_total_songs_label->setPosition(win_size - ccp(2, 0));
        m_pLayer->addChild(m_total_songs_label);
        
        auto menu = CCMenu::create();
        menu->setPosition({0, 0});
        m_pLayer->addChild(menu);

        auto close_button = gd::CCMenuItemSpriteExtra::create(
            CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png"),
            this, menu_selector(CustomSongSearchLayer::on_close)
        );
        close_button->setPosition(25, win_size.height - 25);
        menu->addChild(close_button);

        m_delete_button = gd::CCMenuItemSpriteExtra::create(
            CCSprite::createWithSpriteFrameName("GJ_deleteSongBtn_001.png"),
            this, menu_selector(CustomSongSearchLayer::on_delete)
        );
        auto s = 0.5f;
        m_delete_button->getNormalImage()->setScale(s);
        m_delete_button->setPositionX(widget_width / 2.f + m_delete_button->getContentSize().width * s / 2.f + 2.f);
        menu = CCMenu::create();
        menu->setPosition({win_size.width / 2.f, 0});
        menu->addChild(m_delete_button);
        clip->addChild(menu);

        schedule(schedule_selector(CustomSongSearchLayer::check_mouse_movement), 1.f/30.f); // 30fps cuz ynot

        do_search();
        return true;
    }

    void check_mouse_movement(float) {
        const auto director = CCDirector::sharedDirector();
        const auto win_size = director->getWinSize();
        auto gl = director->getOpenGLView();
        const auto frame_size = gl->getFrameSize();
        auto mouse_pos = gl->getMousePosition();
        mouse_pos.x = (mouse_pos.x / frame_size.width) * win_size.width;
        mouse_pos.y = (1.f - mouse_pos.y / frame_size.height) * win_size.height;
        if (mouse_pos.y != m_prev_mouse_pos.y) {
        // if (mouse_pos.y < clip_height) {
            m_prev_mouse_pos = mouse_pos;
            move_delete_btn();
        }

        m_prev_mouse_pos = mouse_pos;
    }

    void move_delete_btn() {
        constexpr float height = widget_height + widget_spacing;
        constexpr float clip_height = widget_height * (n_widgets - 1);
        int i = static_cast<int>((clip_height - m_prev_mouse_pos.y + m_y_scroll) / height);
        bool e = (m_prev_mouse_pos.y < clip_height) && (i >= 0 && i < static_cast<int>(m_sorted.size()));
        if (e) {
            m_delete_button->setPositionY((n_widgets - i) * height - height * 2.f + height / 2.f - 13.f + m_y_scroll); 
            m_delete_button->setTag(i);
        }
        m_delete_button->setVisible(e);
        m_delete_button->setEnabled(e);
    }

    virtual void textChanged(gd::CCTextInputNode* input) {
        // std::cout << "crazy: " << input->getString() << std::endl;
        // if (m_throttle_action) stopAction(m_throttle_action);
        // auto call_func = CCCallFunc::create(this, callfunc_selector(MyAwesomeSongListLayerWithSearching::do_search));
        // auto delay = CCDelayTime::create(0.1f);
        // m_throttle_action = CCSequence::createWithTwoActions(delay, call_func);
        // runAction(m_throttle_action);
        do_search();
    }

    void do_search() {
        if (m_downloaded_songs.empty()) return;
        std::vector<std::pair<int, gd::SongInfoObject*>> songs;
        songs.reserve(m_downloaded_songs.size());
        auto input_value = m_input->getString();
        const bool empty_input = input_value[0] == 0;
        for (const auto& [index_str, song] : m_downloaded_songs) {
            int score = 0;
            if (!empty_input && !fts::fuzzy_match(input_value, index_str.c_str(), score)) continue;
            songs.push_back({score, song});
        }
        if (!empty_input && songs.size() > 1) {
            std::sort(songs.begin(), songs.end(), [&](const auto& a, const auto& b) {
                return a.first > b.first;
            });
        }
        m_sorted.clear();
        m_sorted.reserve(songs.size());
        for (const auto& [score, song] : songs) {
            m_sorted.push_back(song);
        }
        m_y_scroll = 0.f;
        update_scroll_elements(true);

        std::stringstream stream;
        stream << m_sorted.size() << " result";
        if (m_sorted.size() != 1)
            stream << "s";
        m_total_songs_label->setString(stream.str().c_str());

        move_delete_btn();
    }

    void on_close(CCObject*) {
        keyBackClicked();
    }

    void on_delete(CCObject*) {
        auto alert = gd::FLAlertLayer::create(this, "Warning", "No", "Yes", "Are you sure you want to delete this?");
        alert->setTag(m_delete_button->getTag());
        alert->show();
    }
    
    virtual void FLAlert_Clicked(gd::FLAlertLayer* alert, bool btn2) {
        if (btn2) {
            int i = alert->getTag();
            if (i < 0 || i >= static_cast<int>(m_sorted.size())) return; // sanity check
            auto song = m_sorted[i];
            m_sorted.erase(m_sorted.begin() + i);
            auto path = gd::MusicDownloadManager::sharedState()->pathForSong(song->m_songID);
            if (gd::FMODAudioEngine::sharedEngine()->isBackgroundMusicPlaying(path)) {
                gd::GameSoundManager::sharedState()->stopBackgroundMusic();
            }
            remove(path.c_str()); // inlined MusicDownloadManager::deleteSong
            update_scroll_elements(true);
            
            m_parent->m_songWidget->updateSongObject(song);
        }
    }

    void update_scroll_elements(bool a = false);

    virtual void ccTouchMoved(CCTouch *pTouch, CCEvent *pEvent) {
        gd::FLAlertLayer::ccTouchMoved(pTouch, pEvent);
        m_y_scroll += pTouch->getDelta().y;
        update_scroll_elements();
    }

    virtual void scrollWheel(float a, float b) {
        m_y_scroll += a * 1.5f;
        update_scroll_elements();
        move_delete_btn();
    }

    static auto create(gd::CustomSongLayer* parent) {
        auto ret = new CustomSongSearchLayer();
        if (ret && ret->init(parent)) {
            ret->autorelease();
        } else {
            CC_SAFE_DELETE(ret);
        }
        return ret;
    }
};

class SimpleSongWidget : public CCNode {
protected:
    gd::SongInfoObject* m_song;
    CCLabelBMFont* m_name_label;
    CCLabelBMFont* m_artist_label;
    gd::CCMenuItemSpriteExtra* m_play_button;
    CustomSongSearchLayer* m_parent;
public:
    bool init(CustomSongSearchLayer* s) {
        m_parent = s;
        m_song = nullptr;
        constexpr float width = 350.f;
        constexpr float height = widget_height;
        auto bg = extension::CCScale9Sprite::create("GJ_square01.png");
        bg->setContentSize({width, height});
        bg->setZOrder(-1);
        addChild(bg);
        m_name_label = CCLabelBMFont::create("?", "bigFont.fnt");
        m_name_label->setAnchorPoint({0, 0.5f});
        m_name_label->setPosition({-width / 2.f + 8.f, height / 2.f - 13.5f});
        addChild(m_name_label);
        m_artist_label = CCLabelBMFont::create("who", "goldFont.fnt");
        m_artist_label->setAnchorPoint({0, 1});
        m_artist_label->setPosition({-width / 2.f + 8.f, height / 2.f - 21.6f});
        m_artist_label->setScale(0.6f);
        addChild(m_artist_label);
        auto menu = CCMenu::create();
        addChild(menu);
        m_play_button = gd::CCMenuItemSpriteExtra::create(
            CCSprite::createWithSpriteFrameName("GJ_playMusicBtn_001.png"),
            this,
            menu_selector(SimpleSongWidget::on_play)
        );
        menu->addChild(m_play_button);
        auto select_button = gd::CCMenuItemSpriteExtra::create(
            CCSprite::createWithSpriteFrameName("GJ_selectSongBtn_001.png"),
            this,
            menu_selector(SimpleSongWidget::on_select)
        );
        auto winsize = CCDirector::sharedDirector()->getWinSize();
        m_play_button->setPositionX(width / 2.f - m_play_button->getScaledContentSize().width / 2.f - 3.f);
        select_button->setPositionX(
            m_play_button->getPositionX() - m_play_button->getScaledContentSize().width / 2.f - select_button->getScaledContentSize().width / 2.f - 2.f
        );
        menu->setPosition({0, 1});
        menu->addChild(select_button);
        return true;
    }

    void set_song(gd::SongInfoObject* song) {
        m_song = song;
        m_artist_label->setString(m_song->m_artistName.c_str());
        m_name_label->setString(m_song->m_songName.c_str());
        float scale = min(0.8f, 250.f / m_name_label->getContentSize().width);
        m_name_label->setScale(min(scale, 15.f / m_name_label->getContentSize().height));
        update_play_btn();
    }

    void update_play_btn() {
        auto fmod = gd::FMODAudioEngine::sharedEngine();
        auto song_path = gd::MusicDownloadManager::sharedState()->pathForSong(m_song->m_songID);
        const auto sprite = fmod->isBackgroundMusicPlaying(song_path) ? "GJ_stopMusicBtn_001.png" : "GJ_playMusicBtn_001.png";
        m_play_button->setNormalImage(CCSprite::createWithSpriteFrameName(sprite));
    }

    void on_select(CCObject* obj) {
        m_parent->m_parent->m_levelSettings->m_level->songID = m_song->m_songID;
        gd::MusicDownloadManager::sharedState()->songStateChanged();
        m_parent->m_parent->m_songWidget->updateSongObject(m_song);
    }

    void on_play(CCObject* obj) {
        auto mdm = gd::MusicDownloadManager::sharedState();
        auto song_path = mdm->pathForSong(m_song->m_songID);
        auto gsm = gd::GameSoundManager::sharedState();
        auto fmod = gd::FMODAudioEngine::sharedEngine();
        if (fmod->isBackgroundMusicPlaying(song_path)) {
            gsm->stopBackgroundMusic();
            update_play_btn();
            return;
        }
        gsm->stopBackgroundMusic();
        gsm->playBackgroundMusic(false, song_path);
        for (auto& widget : m_parent->m_scroll_elements) {
            widget->update_play_btn();
        }
    }

    static auto create(CustomSongSearchLayer* parent) {
        auto ret = new SimpleSongWidget;
        if (ret && ret->init(parent)) {
            ret->autorelease();
        } else {
            CC_SAFE_DELETE(ret);
        }
        return ret;
    }
};

void CustomSongSearchLayer::update_scroll_elements(bool force) {
    constexpr float spacing = widget_spacing;
    constexpr float height = widget_height + spacing;
    if (m_scroll_elements.empty()) {
        for (int _ = 0; _ < n_widgets; ++_) {
            auto widget = SimpleSongWidget::create(this);
            m_div->addChild(widget);
            m_scroll_elements.push_back(widget);
        }
    }
    const auto m = height * (static_cast<int>(m_sorted.size()) - n_widgets + 3);
    // std::clamp with a max < min is weird
    if (m < 0)
        m_y_scroll = 0.f;
    else
        m_y_scroll = std::clamp(m_y_scroll, 0.f, m);
    const auto i_offset = static_cast<size_t>(m_y_scroll / height);
    int diff = static_cast<int>(m_prev_y_scroll / height) - i_offset;
    // :worried:
    // all of this to avoid calling set_song when not needed,
    // which ends up saving like 10 FPS on my machine
    if (!force && diff != 0) {
        if (diff < 0) {
            for (int i = -diff; i > 0; --i) {
                auto element = m_scroll_elements.front();
                m_scroll_elements.pop_front();
                const auto j = i_offset + n_widgets - i;
                if (j < m_sorted.size())
                    element->set_song(m_sorted[j]);
                m_scroll_elements.push_back(element);
            }
        } else {
            for (int i = 0; i < diff; ++i) {
                auto element = m_scroll_elements.back();
                m_scroll_elements.pop_back();
                const auto j = i_offset + (diff - i - 1);
                if (j < m_sorted.size())
                    element->set_song(m_sorted[j]);
                m_scroll_elements.push_front(element);
            }
        }
    }
    for (size_t i = 0; i < n_widgets; ++i) {
        auto widget = m_scroll_elements[i];
        const size_t actual_i = i + i_offset;
        if (actual_i < m_sorted.size()) {
            auto song = m_sorted[actual_i];
            widget->setVisible(true);
            if (force) widget->set_song(song);
            widget->setPositionY((n_widgets - i) * height + fmod(m_y_scroll, height) - height * 2.f);
        } else {
            widget->setVisible(false);
        }
    }
    m_prev_y_scroll = m_y_scroll;
}

void __fastcall CustomSongLayer_onSongBrowser(gd::CustomSongLayer* self, void*, CCObject* penis) {
    CustomSongSearchLayer::create(self)->show();
}

// #define _DEBUG

DWORD WINAPI my_thread(void* module) {
#ifdef _DEBUG
    AllocConsole();
    std::ofstream conout("CONOUT$", std::ios::out);
    std::ifstream conin("CONIN$", std::ios::in);
    std::cout.rdbuf(conout.rdbuf());
    std::cin.rdbuf(conin.rdbuf());
#endif
    MH_Initialize();

    const auto base = cast<uintptr_t>(GetModuleHandle(0));
    
    MHook::registerHook(base + 0x67080, CustomSongLayer_onSongBrowser);

    MH_EnableHook(cast<void*>(base + 0x67080));


#ifdef _DEBUG
    std::getline(std::cin, std::string());

    MH_Uninitialize();
    conout.close();
    conin.close();
    FreeConsole();
    FreeLibraryAndExitThread(cast<HMODULE>(module), 0);
#endif
    return 0;
}

BOOL APIENTRY DllMain(HMODULE module, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
        CreateThread(0, 0, my_thread, module, 0, 0);
    return TRUE;
}