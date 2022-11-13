#include <matdash.hpp>
#include <matdash/minhook.hpp>
#include <matdash/boilerplate.hpp>
#include <gd.h>
#include <cocos-ext.h>

using namespace cocos2d;

#include "utils/nodes.hpp"
#include "utils/nodes.cpp"
#include "utils-cocos.hpp"

class CircleToolPopup : public gd::FLAlertLayer, gd::TextInputDelegate, gd::FLAlertLayerProtocol {
public:
	static float m_angle;
	static float m_step;
	static float m_fat;
	CCLabelBMFont* m_label = nullptr;

	static auto* create() {
		auto* node = new (std::nothrow) CircleToolPopup();
		if (node && node->init()) {
			node->autorelease();
		} else {
			delete node;
			node = nullptr;
		}
		return node;
	}

	bool init() {
		if (!this->initWithColor({0, 0, 0, 75})) return false;

		this->m_bNoElasticity = true;

		auto* director = CCDirector::sharedDirector();
		director->getTouchDispatcher()->incrementForcePrio(69);
		this->registerWithTouchDispatcher();

		auto layer = CCLayer::create();
		auto menu = CCMenu::create();
		this->m_pLayer = layer;
		this->m_pButtonMenu = menu;
		
		layer->addChild(menu);
		this->addChild(layer);

		const float width = 300, height = 220;

		const CCPoint offset = director->getWinSize() / 2.f;
		auto bg = extension::CCScale9Sprite::create("GJ_square01.png");
		bg->setContentSize({width, height});
		bg->setPosition(offset);
		menu->setPosition(offset);
		bg->setZOrder(-2);
		layer->addChild(bg);

		auto close_btn = gd::CCMenuItemSpriteExtra::create(
			CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png"),
			this, menu_selector(CircleToolPopup::on_close)
		);

		close_btn->setPosition(ccp(25.f, director->getWinSize().height - 25.f) - offset);
		menu->addChild(close_btn);

		this->setKeypadEnabled(true);


		layer->addChild(
			NodeFactory<CCLabelBMFont>::start("Circle Tool", "goldFont.fnt")
			.setPosition(ccp(0, 95) + offset)
			.setScale(0.75f)
		);

		layer->addChild(
			NodeFactory<CCLabelBMFont>::start("Arc", "goldFont.fnt")
			.setPosition(ccp(-60, 64) + offset)
			.setScale(0.75f)
		);
		auto angle_input = FloatInputNode::create(CCSize(60, 30), 2.f, "bigFont.fnt");
		angle_input->set_value(m_angle);
		angle_input->setPosition(ccp(-60, 38) + offset);
		angle_input->callback = [&](FloatInputNode& input) {
			m_angle = input.get_value().value_or(m_angle);
			this->update_labels();
		};
		this->addChild(angle_input);
		
		layer->addChild(
			NodeFactory<CCLabelBMFont>::start("Step", "goldFont.fnt")
			.setPosition(ccp(60, 64) + offset)
			.setScale(0.75f)
		);
		auto step_input = FloatInputNode::create(CCSize(60, 30), 2.f, "bigFont.fnt");
		step_input->set_value(m_step);
		step_input->setPosition(ccp(60, 38) + offset);
		step_input->callback = [&](FloatInputNode& input) {
			m_step = input.get_value().value_or(m_step);
			if (m_step == 0.f) m_step = 1.f;
			this->update_labels();
		};
		this->addChild(step_input);

		layer->addChild(
			NodeFactory<CCLabelBMFont>::start("Squish", "goldFont.fnt")
			.setPosition(ccp(60, 10) + offset)
			.setScale(0.75f)
		);
		auto fat_input = FloatInputNode::create(CCSize(60, 30), 2.f, "bigFont.fnt");
		fat_input->set_value(m_fat);
		fat_input->setPosition(ccp(60, -16) + offset);
		fat_input->callback = [&](FloatInputNode& input) {
			m_fat = input.get_value().value_or(m_fat);
			this->update_labels();
		};
		this->addChild(fat_input);

		auto apply_btn = gd::CCMenuItemSpriteExtra::create(
			gd::ButtonSprite::create("Apply", 0, false, "goldFont.fnt", "GJ_button_01.png", 0, 0.75f),
			this, menu_selector(CircleToolPopup::on_apply)
		);
		apply_btn->setPosition({0, -85});
		menu->addChild(apply_btn);

		m_label = CCLabelBMFont::create("copies: 69\nobjects: 69420", "chatFont.fnt");
		m_label->setAlignment(kCCTextAlignmentLeft);
		m_label->setPosition(ccp(-83, -41) + offset);
		layer->addChild(m_label);
		this->update_labels();

		auto info_btn = gd::CCMenuItemSpriteExtra::create(
			CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png"), this, menu_selector(CircleToolPopup::on_info)
		);
		info_btn->setPosition(ccp(-width / 2, height / 2) + ccp(20, -20));
		menu->addChild(info_btn);

		info_btn = gd::CCMenuItemSpriteExtra::create(
			NodeFactory<CCSprite>::start(CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png")).setScale(0.6f).finish(),
			this, menu_selector(CircleToolPopup::on_info2)
		);
		info_btn->setPosition(ccp(60, 10) + ccp(42, -1.5));
		menu->addChild(info_btn);

		this->setTouchEnabled(true);

		return true;
	}

	void keyBackClicked() override {
		this->setTouchEnabled(false);
		this->removeFromParentAndCleanup(true);
	}

	void on_close(CCObject*) {
		this->keyBackClicked();
	}

	void update_labels() {
		auto objs = gd::GameManager::sharedState()->getEditorLayer()->m_editorUI->getSelectedObjects();
		const auto amt = static_cast<size_t>(std::ceilf(m_angle / m_step) - 1.f);
		const auto obj_count = amt * objs->count();
		m_label->setString(("Copies: " + std::to_string(amt) + "\nObjects: " + std::to_string(obj_count)).c_str());
	}

	void on_apply(CCObject*) {
		auto* editor = gd::GameManager::sharedState()->getEditorLayer()->m_editorUI;
		auto objs = editor->getSelectedObjects();
		const auto amt = static_cast<size_t>(std::ceilf(m_angle / m_step) - 1.f);
		if (objs && objs->count()) {
			const auto obj_count = objs->count() * amt;
			if (obj_count > 5000) {
				gd::FLAlertLayer::create(this, "Warning", "Cancel", "Ok", "This will create <cy>" + std::to_string(obj_count) + "</c> objects, are you sure?")->show();
			} else {
				perform();
			}
		}
	}

	void FLAlert_Clicked(gd::FLAlertLayer*, bool btn2) override {
		if (btn2)
			perform();
	}

	void perform() {
		auto* editor = gd::GameManager::sharedState()->getEditorLayer();
		auto* editor_ui = editor->m_editorUI;
		auto* objs = CCArray::create();
		const auto calc = [](float angle) {
			return -sinf(angle / 180.f * 3.141592f - 3.141592f / 2.f) * m_fat;
		};
		float off_acc = calc(0);
		for (float i = 1; i * m_step < m_angle; ++i) {
			editor_ui->onDuplicate(nullptr);
			auto selected = editor_ui->getSelectedObjects();
			editor_ui->rotateObjects(selected, m_step, {0.f, 0.f});
			
			const float angle = i * m_step;

			if (m_fat != 0.f) {
				float off_y = calc(angle) - off_acc;

				for (auto obj : AwesomeArray<gd::GameObject>(selected)) {
					editor_ui->moveObject(obj, {0, off_y});
				}

				off_acc = calc(angle);
			}

			// remove undo object for pasting the objs
			editor->m_undoObjects->removeLastObject();
			objs->addObjectsFromArray(selected);
		}
		editor->m_undoObjects->addObject(gd::UndoObject::createWithArray(objs, gd::UndoCommand::Paste));
		// second argument is ignoreSelectFilter
		editor_ui->selectObjects(objs, true);
		this->keyBackClicked();
	}

	void on_info(CCObject*) {
		gd::FLAlertLayer::create(this, "info", "ok", nullptr, 300.f,
			"This will repeatedly copy-paste and rotate the selected objects,\n"
			"rotating by <cy>Step</c> degrees each time, until it gets to <cy>Arc</c> degrees."
		)->show();
	}

	void on_info2(CCObject*) {
		gd::FLAlertLayer::create(this, "info", "ok", nullptr, 400.f,
			"This will move the selection up and down with the rotation angle, as to create an <cy>ellipse</c>.\n"
			"This number will control how much it goes up and down, thus controlling how <cg>\"squished\"</c> the circle is.\n"
			"This works best with a single object as the center, marked as <co>group parent</c>."
		)->show();
	}
};

float CircleToolPopup::m_angle = 180.0f;
float CircleToolPopup::m_step = 5.f;
float CircleToolPopup::m_fat = 0.f;


class MyEditorUI : public gd::EditorUI {
public:
	void on_circle_tool(CCObject*) {
		if (this->getSelectedObjects()->count())
			CircleToolPopup::create()->show();
	}

	void createMoveMenu() {
		matdash::orig<&MyEditorUI::createMoveMenu>(this);
		auto* btn = this->getSpriteButton("player_ball_43_2_001.png", menu_selector(MyEditorUI::on_circle_tool), nullptr, 0.9f);
		if (btn) {
			m_pEditButtonBar3->m_buttonArray->addObject(btn);
			auto rows = gd::GameManager::sharedState()->getIntGameVariable("0049");
			auto cols = gd::GameManager::sharedState()->getIntGameVariable("0050");
			m_pEditButtonBar3->loadFromItems(m_pEditButtonBar3->m_buttonArray, rows, cols, m_pEditButtonBar3->m_unkF8);
		}
	}
};

void mod_main(HMODULE) {
	matdash::add_hook<&MyEditorUI::createMoveMenu>(gd::base + 0x8c0d0);
}