#include <game/scenes/SplashScene.hpp>
#include <game/scenes/MenuScene.hpp>
#include <game/objects/Starfield.hpp>
#include <engine/actions.hpp>
#include <engine/input.hpp>

namespace game {

    std::shared_ptr<SplashSceneTop> SplashSceneTop::create() {
        auto scene = std::make_shared<SplashSceneTop>();
        scene->init();
        return scene;
    }

    void SplashSceneTop::init() {
        auto size = engine::Director::getInstance().getTopScreenSize();

        m_starfield = Starfield::create(size.width, size.height, StarDirection::Down, 0.0f);
        m_starfield->setZOrder(0.0f);
        this->addChild(m_starfield);

        m_innerslothLogo = engine::Sprite::create("sdmc:/3ds/AmongUs3DS/assets/sprites/logoImage.png");
        if (m_innerslothLogo) {
            m_innerslothLogo->setPosition(size.width / 2.0f, -100.0f);
            m_innerslothLogo->setAnchorPoint(0.5f, 0.5f);
            m_innerslothLogo->setScale(0.3f);
            m_innerslothLogo->setZOrder(1.0f);
            this->addChild(m_innerslothLogo);

            m_fadeOut = engine::DrawNode::createRect(size.width, size.height, C2D_Color32(0, 0, 0, 255));
            m_fadeOut->setOpacity(0.f);
            m_fadeOut->setZOrder(2.0f);
            this->addChild(m_fadeOut);

            auto fadePtr = m_fadeOut;
            auto starsPtr = m_starfield;

            m_innerslothLogo->runAction(engine::Sequence::create(
                engine::DelayTime::create(1.0f),
                engine::Spawn::create(
                    engine::Sequence::create(
                        engine::DelayTime::create(0.5f),
                        engine::PlaySound::create("sdmc:/3ds/AmongUs3DS/assets/audio/panel_admin_cardmove1.wav")
                    ),
                    engine::MoveTo::create(1.0f, size.width / 2.0f, size.height / 2.0f, engine::Ease::SineInOut),
                    engine::CallFunc::create([starsPtr]() {
                        starsPtr->setSpeed(0.0f);
                        starsPtr->runAction(engine::Sequence::create(
                            StarSpeedTo::create(0.5f, 100.f, engine::Ease::SineIn),
                            StarSpeedTo::create(0.5f, 0.f, engine::Ease::SineOut)
                        ));
                    })
                ),
                engine::DelayTime::create(0.2f),
                engine::PlaySound::create("sdmc:/3ds/AmongUs3DS/assets/audio/task_Complete.wav"),
                engine::ScaleTo::create(0.15f, 0.325f, engine::Ease::SineIn),
                engine::ScaleTo::create(0.15f, 0.3f, engine::Ease::SineOut),
                engine::DelayTime::create(1.0f),
                engine::CallFunc::create([fadePtr]() {
                    fadePtr->runAction(engine::FadeIn::create(0.5f));
                    auto bottom = std::static_pointer_cast<SplashSceneBottom>(engine::Director::getInstance().getBottomScene());
                    if (bottom) {
                        bottom->startFadeOut(0.5f);
                    }
                }),
                engine::DelayTime::create(0.5f),
                engine::CallFunc::create([this]() {
                    m_menuTop = game::MenuSceneTop::create();
                    m_menuBottom = game::MenuSceneBottom::create();
                    engine::Director::getInstance().setTopScene(m_menuTop);
                    engine::Director::getInstance().setBottomScene(m_menuBottom);
                })
            ));
        }
    }

    void SplashSceneTop::update(float dt) {
        engine::Scene::update(dt);
    }

    std::shared_ptr<SplashSceneBottom> SplashSceneBottom::create() {
        auto scene = std::make_shared<SplashSceneBottom>();
        scene->init();
        return scene;
    }

    void SplashSceneBottom::init() {
        auto size = engine::Director::getInstance().getBottomScreenSize();

        m_starfield = Starfield::create(size.width, size.height, StarDirection::Down, 0.0f);
        this->addChild(m_starfield);
        m_starfield->runAction(engine::Sequence::create(
            engine::DelayTime::create(1.0f),
            StarSpeedTo::create(0.5f, 100.f, engine::Ease::SineIn),
            StarSpeedTo::create(0.5f, 0.f, engine::Ease::SineOut)
        ));

        m_rodCredit = engine::Label::create("romfs:/fonts/arial.bcfnt", "Ported to 3DS\nby OmgRod");
        m_rodCredit->setAlignment(engine::Label::HAlign::Center);
        m_rodCredit->setAnchorPoint(0.5f, 0.5f);
        m_rodCredit->setPosition(size.width / 2.0f, size.height / 2.0f - 15.0f);
        m_rodCredit->setTextScale(1.0f);
        this->addChild(m_rodCredit);

        auto warningLabel = engine::Label::create("romfs:/fonts/arial.bcfnt", "Note: Screen may freeze for a moment, this is normal.");
        warningLabel->setAlignment(engine::Label::HAlign::Center);
        warningLabel->setAnchorPoint(0.5f, 1.0f);
        warningLabel->setPosition(size.width / 2.0f, size.height - 25.0f);
        warningLabel->setTextScale(0.5f);
        this->addChild(warningLabel);

        m_fadeOut = engine::DrawNode::createRect(size.width, size.height, C2D_Color32(0, 0, 0, 255));
        m_fadeOut->setOpacity(0.0f);
        this->addChild(m_fadeOut);
    }

    void SplashSceneBottom::update(float dt) {
        engine::Scene::update(dt);
    }

    void SplashSceneBottom::startFadeOut(float duration) {
        if (m_fadeOut) {
            m_fadeOut->runAction(engine::FadeIn::create(duration));
        }
    }

}
