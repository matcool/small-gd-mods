#pragma once
#include "hackpro_ext.h"
#include "../libraries/gdhm-api/includes/gdhm.h"
#include <cstring>
#include <stack>
#include <vector>
#include <string>
#include <sstream>

class Window {
public:
    static auto& get() {
        static Window window;
        return window;
    }

protected:
    enum class ItemType {
        CHECKBOX,
        DROPDOWN
    };

    void* hackproExt = nullptr;

    // this is so ugly
    using CheckboxCallback = void(bool);
    using TextboxCallback = void(const char*);
    using SetupFunc = void(Window&);
    using ButtonCallback = void();
    using ListCallback = void(int);

    using WidgetID = int;

    // this is so cursed lmao
    template <class, class T, auto id>
    static T& getValue() {
        static T value;
        return value;
    }

    struct List;
    struct Checkbox;
    struct Textbox;

    static int getNextID() {
        static int value = 0;
        return value++;
    }
    // CB does not stand for cock ball
    struct hackproCB {
        template <auto callback>
        static void __stdcall checkbox(void*) {
            callback(true);
        }

        template <auto callback>
        static void __stdcall uncheckbox(void*) {
            callback(false);
        }

        template <auto callback>
        static void __stdcall button(void*) {
            callback();
        }

        template <auto callback>
        static void __stdcall list(void* combo, int index, const char* name) {
            const auto size = reinterpret_cast<int>(HackproGetUserData(combo));
            // avoid calls from when first setting the strings
            // why absolute
            if (size == -1) return;
            callback(size - index - 1);
        }

        template <auto callback>
        static void __stdcall textbox(void* box) {
            if (HackproGetUserData(box)) return;
            callback(HackproGetTextBoxText(box));
        }
    };

    Window() {}

    struct HackProWidgetAction {
        enum class Type {
            CHECKBOX,
            BUTTON,
            LIST,
            TEXTBOX,
        } type;
        union {
            struct {
                const char *str;
                void(__stdcall* checked)(void*);
                void(__stdcall* unchecked)(void*);
                void** ptr;
            } checkbox;
            struct {
                const char* name;
                void(__stdcall* callback)(void*);
            } button;
            struct {
                size_t size;
                const char** values;
                void(__stdcall* callback)(void*, int, const char*);
                void** ptr;
            } list;
            struct {
                const char* placeholder;
                void(__stdcall* callback)(void*);
                void** ptr;
            } textbox;
        };
    };

    std::stack<HackProWidgetAction> hackproWidgets;
public:
    enum class ModMenuType {
        NONE,
        HACKPRO,
        GDHM
    };
    ModMenuType type = ModMenuType::NONE;

    template <SetupFunc func, SetupFunc post>
    void setup(const char* name) {
        if (InitialiseHackpro()) {
            type = ModMenuType::HACKPRO;
            if (HackproIsReady()) {
                hackproExt = HackproInitialiseExt(name);
                func(*this);
                while (!hackproWidgets.empty()) {
                    const auto& action = hackproWidgets.top();
                    switch (action.type) {
                        case HackProWidgetAction::Type::CHECKBOX: {
                            const auto& m = action.checkbox;
                            *m.ptr = HackproAddCheckbox(hackproExt, m.str, m.checked, m.unchecked);
                            break;
                        }
                        case HackProWidgetAction::Type::BUTTON: {
                            const auto& m = action.button;
                            HackproAddButton(hackproExt, m.name, m.callback);
                            break;
                        }
                        case HackProWidgetAction::Type::LIST: {
                            const auto& m = action.list;
                            auto combo = HackproAddComboBox(hackproExt, m.callback);
                            HackproSetUserData(combo, reinterpret_cast<void*>(-1));
                            HackproSetComboBoxStrs(combo, m.values);
                            HackproSetUserData(combo, reinterpret_cast<void*>(m.size));
                            *m.ptr = combo;
                            break;
                        }
                        case HackProWidgetAction::Type::TEXTBOX: {
                            const auto& m = action.textbox;
                            auto textbox = HackproAddTextBox(hackproExt, m.callback);
                            if (m.placeholder)
                                HackproSetTextBoxPlaceholder(textbox, m.placeholder);
                            *m.ptr = textbox;
                            break;
                        }
                    }
                    hackproWidgets.pop();
                }
                post(*this);
                HackproCommitExt(hackproExt);
            }
        } else if (gdhm::is_loaded()) {
            type = ModMenuType::GDHM;
            gdhm::gui::window(name, "", nullptr, nullptr, [&]() {
                func(*this);
                post(*this);
            }, nullptr);
        }
    }


    template <WidgetID id, CheckboxCallback callback>
    void addCheckbox(const char* name) {
        if (type == ModMenuType::HACKPRO) {
            HackProWidgetAction action;
            // tfw no c style struct initialization
            action.type = HackProWidgetAction::Type::CHECKBOX;
            action.checkbox = { name, &hackproCB::checkbox<callback>, &hackproCB::uncheckbox<callback>, &getValue<Checkbox, void*, id>() };
            hackproWidgets.push(action);
        } else if (type == ModMenuType::GDHM) {
            // gdhm::gui::checkbox(getNextID(), name, &getValue<Checkbox, bool, id>(), nullptr, []() {
            //     callback(getValue<Checkbox, bool, id>());
            // });
        }
    }
    
    template <WidgetID id>
    void setCheckboxState(bool state) {
        if (type == ModMenuType::HACKPRO) {
            HackproSetCheckbox(getValue<Checkbox, void*, id>(), state);
        } else if (type == ModMenuType::GDHM) {
            getValue<Checkbox, bool, id>() = state;
        }
    }

    template <ButtonCallback callback>
    void addButton(const char* name) {
        if (type == ModMenuType::HACKPRO) {
            HackProWidgetAction action;
            action.type = HackProWidgetAction::Type::BUTTON;
            action.button = { name, &hackproCB::button<callback> };
            hackproWidgets.push(action);
        } else if (type == ModMenuType::GDHM) {
            // gdhm::gui::button(getNextID(), name, nullptr, []() {
            //     callback();
            // });
        }
    }

    template <WidgetID id, ListCallback callback>
    void addList(const std::vector<const char*>& values) {
        if (type == ModMenuType::HACKPRO) {
            auto output = new const char*[values.size() + 1];
            output[values.size()] = 0;
            for (size_t i = 0; i < values.size(); ++i) {
                output[i] = values[values.size() - i - 1];
            }
            HackProWidgetAction action;
            action.type = HackProWidgetAction::Type::LIST;
            action.list = { values.size(), output, &hackproCB::list<callback>, &getValue<List, void*, id>() };
            hackproWidgets.push(action);
        } else if (type == ModMenuType::GDHM) {

        }
    }

    template <WidgetID id>
    void setListIndex(const int index) {
        if (type == ModMenuType::HACKPRO) {
            const auto combo = getValue<List, void*, id>();
            const auto size = reinterpret_cast<int>(HackproGetUserData(combo));
            HackproSetComboBoxIndex(combo, size - index - 1);
        }
    }

    template <WidgetID id, TextboxCallback callback>
    void addTextbox(const char* placeholder = nullptr) {
        if (type == ModMenuType::HACKPRO) {
            HackProWidgetAction action;
            action.type = HackProWidgetAction::Type::TEXTBOX;
            action.textbox = { placeholder, &hackproCB::textbox<callback>, &getValue<Textbox, void*, id>() };
            hackproWidgets.push(action);
        } else if (type == ModMenuType::GDHM) {
        }
    }

    template <WidgetID id>
    void setTextboxText(const char* text) {
        if (type == ModMenuType::HACKPRO) {
            const auto box = getValue<Textbox, void*, id>();
            HackproSetUserData(box, (void*)1);
            HackproSetTextBoxText(box, text);
            HackproSetUserData(box, 0);
        }
    }
};