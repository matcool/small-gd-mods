#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <MinHook.h>
#include <cocos2d.h>
#include <cocos-ext.h>
#include "utils.hpp"
#include <gd.h>
#include <fstream>
#include <iostream>
#include <string>
#include <CCGL.h>
#include <filesystem>
#include <fmod.hpp>

using namespace cocos2d;

#define STRINGIFY(...) #__VA_ARGS__

// most of this is from
// https://github.com/cocos2d/cocos2d-x/blob/5a25fe75cb8b26b61b14b070e757ec3b17ff7791/samples/Cpp/TestCpp/Classes/ShaderTest/ShaderTest.cpp

class ShaderNode : public CCNode {
    GLuint m_uniformCenter,
        m_uniformResolution,
        m_uniformTime,
        m_uniformMouse,
        m_uniformPulse1,
        m_uniformPulse2,
        m_uniformPulse3,
        m_uniformFft;
    float m_time;
    FMOD::DSP* m_fftDsp = nullptr;
    static constexpr int FFT_SPECTRUM_SIZE = 512;
    // gd cuts frequencies higher than ~16kHz, so why can't we? (the "139/512" part)
    // we also remove the right half (by multiplying by 2), because it's a mirrored version of the left half, so we don't need that
    // (and fmod actually removes it completely, so it's always all zeros anyway)
    // (there are actually 513 empty bins instead of 512 but the last one gets cut off by the "139/512" part)
    // i know, this is weird af
    static constexpr int FFT_WINDOW_SIZE = (FFT_SPECTRUM_SIZE + (FFT_SPECTRUM_SIZE * 139 / 512)) * 2;
    static constexpr float FFT_UPDATE_FREQUENCY = 20.f;
    float m_spectrum[FFT_SPECTRUM_SIZE];
    float m_oldSpectrum[FFT_SPECTRUM_SIZE];
    float m_newSpectrum[FFT_SPECTRUM_SIZE];
    float m_spectrumUpdateAccumulator;
public:
    bool init(const GLchar* vert, const GLchar* frag) {
        auto shader = new CCGLProgram;
        if (!shader->initWithVertexShaderByteArray(vert, frag)) {
            // this doesnt work :c
            auto error = glGetError();
            std::cout << "opengl error " << error << std::endl;
            return false;
        }

        auto engine = gd::FMODAudioEngine::sharedEngine();
        engine->m_pSystem->createDSPByType(FMOD_DSP_TYPE_FFT, &m_fftDsp);
        engine->m_pGlobalChannel->addDSP(1, m_fftDsp);
        m_fftDsp->setParameterInt(FMOD_DSP_FFT_WINDOWTYPE, FMOD_DSP_FFT_WINDOW_HAMMING);
        m_fftDsp->setParameterInt(FMOD_DSP_FFT_WINDOWSIZE, FFT_WINDOW_SIZE);
        m_fftDsp->setActive(true);

        std::memset(m_spectrum, 0, FFT_SPECTRUM_SIZE * sizeof(float));
        std::memset(m_oldSpectrum, 0, FFT_SPECTRUM_SIZE * sizeof(float));
        std::memset(m_newSpectrum, 0, FFT_SPECTRUM_SIZE * sizeof(float));

        shader->addAttribute(kCCAttributeNamePosition, kCCVertexAttrib_Position);
        shader->addAttribute(kCCAttributeNameTexCoord, kCCVertexAttrib_TexCoords);
        shader->link();

        shader->updateUniforms();

        m_uniformCenter = glGetUniformLocation(shader->getProgram(), "center");
        m_uniformResolution = glGetUniformLocation(shader->getProgram(), "resolution");
        m_uniformTime = glGetUniformLocation(shader->getProgram(), "time");
        m_uniformMouse = glGetUniformLocation(shader->getProgram(), "mouse");
        m_uniformPulse1 = glGetUniformLocation(shader->getProgram(), "pulse1");
        m_uniformPulse2 = glGetUniformLocation(shader->getProgram(), "pulse2");
        m_uniformPulse3 = glGetUniformLocation(shader->getProgram(), "pulse3");
        m_uniformFft = glGetUniformLocation(shader->getProgram(), "fft");

        this->setShaderProgram(shader);

        m_time = 0.f;
        auto size = CCDirector::sharedDirector()->getWinSize();

        shader->release();

        this->scheduleUpdate();

        setContentSize(size);
        setAnchorPoint({0.5f, 0.5f});
        return true;
    }

    ~ShaderNode() {
        if (m_fftDsp) {
            gd::FMODAudioEngine::sharedEngine()->m_pGlobalChannel->removeDSP(m_fftDsp);
        }
    }

    virtual void update(float dt) {
        m_time += dt;
        m_spectrumUpdateAccumulator += dt;

        const float speed = 1.f / FFT_UPDATE_FREQUENCY;
        if (m_spectrumUpdateAccumulator >= speed) {
            auto engine = gd::FMODAudioEngine::sharedEngine();
            if (m_fftDsp) {
                FMOD_DSP_PARAMETER_FFT* data;
                unsigned int length;
                m_fftDsp->getParameterData(FMOD_DSP_FFT_SPECTRUMDATA, (void**)&data, &length, nullptr, 0);
                if (length) {
                    for (size_t i = 0; i < min(data->length, FFT_SPECTRUM_SIZE); i++) {
                        m_oldSpectrum[i] = m_newSpectrum[i];
                        // average out all the channels
                        m_newSpectrum[i] = 0.f;
                        for (size_t j = 0; j < data->numchannels; j++) {
                            m_newSpectrum[i] += data->spectrum[j][i];
                        }
                        m_newSpectrum[i] /= data->numchannels;
                    }
                }
            }
            m_spectrumUpdateAccumulator = 0.f;
        }
        float t = m_spectrumUpdateAccumulator * FFT_UPDATE_FREQUENCY;
        for (int i = 0; i < FFT_SPECTRUM_SIZE; i++) {
            m_spectrum[i] = (1.f - t) * m_oldSpectrum[i] + t * m_newSpectrum[i];
        }
    }
    virtual void draw() {
        CC_NODE_DRAW_SETUP();

        auto glv = CCDirector::sharedDirector()->getOpenGLView();
        auto size = CCDirector::sharedDirector()->getWinSize();
        auto frSize = glv->getFrameSize();
        float w = frSize.width, h = frSize.height;
        GLfloat vertices[12] = {0,0, w,0, w,h, 0,0, 0,h, w,h};

        // TODO: actually set the center
        getShaderProgram()->setUniformLocationWith2f(m_uniformCenter, 0.f, 0.f);
        getShaderProgram()->setUniformLocationWith2f(m_uniformResolution, w, h);
        const auto mousePos = glv->getMousePosition();
        getShaderProgram()->setUniformLocationWith2f(m_uniformMouse, mousePos.x, h - mousePos.y);

        // ccGLBindTexture2D()

        glUniform1f(m_uniformTime, m_time);

        // thx adaf for telling me where these are
        auto engine = gd::FMODAudioEngine::sharedEngine();
        glUniform1f(m_uniformPulse1, engine->m_fPulse1);
        glUniform1f(m_uniformPulse2, engine->m_fPulse2);
        glUniform1f(m_uniformPulse3, engine->m_fPulse3);

        glUniform1fv(m_uniformFft, FFT_SPECTRUM_SIZE, m_spectrum);

        ccGLEnableVertexAttribs(kCCVertexAttribFlag_Position);

        glVertexAttribPointer(kCCVertexAttrib_Position, 2, GL_FLOAT, GL_FALSE, 0, vertices);

        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        // CC_INCREMENT_GL_DRAWS(1); // this crashes :c
    }
    static auto create(const GLchar* vert, const GLchar* frag) {
        auto node = new ShaderNode;
        if (node->init(vert, frag)) {
            node->autorelease();
        } else {
            CC_SAFE_DELETE(node);
        }
        return node;
    }
};

// ptr -> ref
template <typename T>
T& ref(T* value) { return *value; }

inline void patch(void* loc, std::vector<std::uint8_t> bytes) {
    auto size = bytes.size();
    DWORD old_prot;
    VirtualProtect(loc, size, PAGE_EXECUTE_READWRITE, &old_prot);
    memcpy(loc, bytes.data(), size);
    VirtualProtect(loc, size, old_prot, &old_prot);
}

static const auto menuShadersKey = "menu-shader-enabled";
bool g_enabled = false;
bool g_hasPatched = false;

bool (__thiscall* MenuLayer_init)(gd::MenuLayer*);
bool __fastcall MenuLayer_init_H(gd::MenuLayer* self){
    auto& gm = ref(gd::GameManager::sharedState());
    g_enabled = gm.getGameVariableDefault(menuShadersKey, true);
    if (g_enabled && !g_hasPatched) {
        // huge thanks to adaf for this!
        // im doing the patch here instead of the dll entry thread because
        // that could cause a crash as its in a different thread
        patch(cast<void*>(gd::base + 0x23b56), {0x90, 0x90, 0x90, 0x90, 0x90, 0x90});
        g_hasPatched = true;
    }

    if (!MenuLayer_init(self)) return false;
    if (!g_enabled) return true;

    // hm yes very safe, although i cant think of a better way
    auto gameLayer = dynamic_cast<CCLayer*>(self->getChildren()->objectAtIndex(0));
    if (gameLayer != nullptr) {
        gameLayer->removeFromParentAndCleanup(true);
        gameLayer = nullptr;
    }
    auto& utils = ref(CCFileUtils::sharedFileUtils());
    const auto& shaderPath = utils.fullPathForFilename("menu-shader.fsh", false);
    std::string shaderSource;
    // yes im using std::filesystem just for this
    // CCFileUtils::isFileExist is really weird and broken
    if (std::filesystem::exists(shaderPath)) {
        std::cout << "loading from " << shaderPath << std::endl;
        std::ifstream file;
        file.open(shaderPath, std::ios::in);
        // this is the best way i could find of reading an entire file
        std::ostringstream sstr;
        sstr << file.rdbuf();
        shaderSource = sstr.str();
        file.close();
    } else {
        // default shader is a blue swirly pattern
        // which i made some time ago in shadertoy
        // https://www.shadertoy.com/view/wdG3Wh
        shaderSource = STRINGIFY(
            uniform vec2 center; // useless atm
            uniform vec2 resolution;
            uniform float time;
            uniform vec2 mouse; // not used here

            vec2 hash(vec2 p) {
                p = vec2(dot(p,vec2(127.1,311.7)), dot(p,vec2(269.5,183.3)));
                return -1.0 + 2.0*fract(sin(p)*43758.5453123);
            }

            float noise(in vec2 p) {
                const float K1 = 0.366025404; // (sqrt(3)-1)/2;
                const float K2 = 0.211324865; // (3-sqrt(3))/6;

                vec2  i = floor( p + (p.x+p.y)*K1 );
                vec2  a = p - i + (i.x+i.y)*K2;
                float m = step(a.y,a.x); 
                vec2  o = vec2(m,1.0-m);
                vec2  b = a - o + K2;
                vec2  c = a - 1.0 + 2.0*K2;
                vec3  h = max( 0.5-vec3(dot(a,a), dot(b,b), dot(c,c) ), 0.0 );
                vec3  n = h*h*h*h*vec3( dot(a,hash(i+0.0)), dot(b,hash(i+o)), dot(c,hash(i+1.0)));
                return dot(n, vec3(70.0));
            }

            void main() {
                // mat wtf is this
                vec2 uv = (gl_FragCoord.xy - 0.5*resolution.xy ) / resolution.y;
    
                uv = vec2(noise(uv+time*0.1), noise(uv+10.));
                float d = uv.x - uv.y;
                d *= 20.;
                d = sin(d);
                d = d * 0.5 + 0.5;
                d = 1.0 - d;
                
                d = smoothstep(0.1, 0.1, d);
                
                vec3 col = vec3(mix(vec3(0.1), vec3(0.2, 0.2, 0.6), d));

                gl_FragColor = vec4(col,1.0);
            }
        );
    }
    /*
    Heres an extremely simple shader that just shows the uv,
    while also showing all available uniforms

    uniform vec2 resolution;
    uniform float time;
    uniform vec2 mouse;
    uniform float pulse1; // i recommend this one
    uniform float pulse2;
    uniform float pulse3;
    uniform float fft[512];

    void main() {
        vec2 uv = gl_FragCoord.xy / resolution;
        uv.x *= resolution.x / resolution.y; // optional: fix aspect ratio
        gl_FragColor = vec4(uv, 0.0, 1.0);
    }
    */

    auto shader = ShaderNode::create(STRINGIFY(
        attribute vec4 a_position;

        void main() {
            gl_Position = CC_MVPMatrix * a_position;
        }
    ), shaderSource.c_str());

    if (shader == nullptr)
        return true;

    shader->setZOrder(-10);
    self->addChild(shader);
    auto size = CCDirector::sharedDirector()->getWinSize();
    shader->setPosition(size / 2);
    return true;
}

bool (__thiscall* MoreOptionsLayer_init)(gd::MoreOptionsLayer*);
bool __fastcall MoreOptionsLayer_init_H(gd::MoreOptionsLayer* self) {
    if (!MoreOptionsLayer_init(self)) return false;

    self->addToggle("Menu shaders", menuShadersKey, "Enable fancy shaders on the main menu");

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

    auto base = cast<uintptr_t>(GetModuleHandle(0));

    MH_CreateHook(cast<void*>(base + 0x1907B0), MenuLayer_init_H, (void**)&MenuLayer_init);
    MH_CreateHook(cast<void*>(base + 0x1DE8F0), MoreOptionsLayer_init_H, (void**)&MoreOptionsLayer_init);

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