#include <game/scenes/MenuScene.hpp>
#include <game/scenes/LocalLobbyScene.hpp>
#include <game/objects/PlayerSprite.hpp>
#include <game/objects/Popup.hpp>
#include <game/objects/SettingsPopup.hpp>
#include <game/objects/colors.hpp>
#include <game/Settings.hpp>
#include <engine/actions.hpp>
#include <engine/input.hpp>

namespace game {

    std::shared_ptr<MenuSceneTop> MenuSceneTop::create() {
        auto scene = std::make_shared<MenuSceneTop>();
        scene->init();
        return scene;
    }

    void MenuSceneTop::init() {
        auto size = engine::Director::getInstance().getTopScreenSize();

        auto& ae = engine::AudioEngine::getInstance();
        auto music = ae.playSFX("sdmc:/3ds/AmongUs3DS/assets/audio/AmongUsTheme.wav", true);
        music.setTag("menu-music");

        m_starfield = Starfield::create(size.width, size.height, StarDirection::Right, 25.f);
        this->addChild(m_starfield);

        m_floatingCrew.reserve(8);
        for (int i = 0; i < 6; ++i) {
            spawnCrewMember(false);
        }

        auto logo = engine::Sprite::create("sdmc:/3ds/AmongUs3DS/assets/sprites/bannerLogo_AmongUs.png");
        logo->setPosition(size.width / 2, size.height / 2);
        logo->setScale(0.55f);
        logo->setAnchorPoint(0.5f, 0.5f);
        this->addChild(logo);

        auto copyrightNotice = engine::Label::create("romfs:/fonts/arial.bcfnt", "This is an unofficial Nintendo 3DS port developed by OmgRod. Among Us is\nproperty of Innersloth LLC. OmgRod is not affiliated with or endorsed by\nInnersloth.");
        copyrightNotice->setTextScale(0.5f);
        copyrightNotice->setPosition(0.f, size.height - (copyrightNotice->getHeight() / 2) - 5.f);
        this->addChild(copyrightNotice);
    }

    void MenuSceneTop::spawnCrewMember(bool outside) {
        auto size = engine::Director::getInstance().getTopScreenSize();
        int crewIndex = engine::Math::randomInt(1, 6);
        int colorId = engine::Math::randomInt(0, 11);
        std::string path = "sdmc:/3ds/AmongUs3DS/assets/sprites/mainscreenCrew" + std::to_string(crewIndex) + ".png";
        
        auto sprite = game::PlayerSprite::create(path, colorId);
        if (!sprite) return;

        float scale = engine::Math::randomFloat(0.15f, 0.30f);
        sprite->setScale(scale);
        sprite->setAnchorPoint(0.5f, 0.5f);

        float x, y;
        if (outside) {
            int edge = engine::Math::randomInt(0, 3);
            switch (edge) {
                case 0: // Top
                    x = engine::Math::randomFloat(0, size.width);
                    y = -50.0f;
                    break;
                case 1: // Bottom
                    x = engine::Math::randomFloat(0, size.width);
                    y = size.height + 50.0f;
                    break;
                case 2: // Left
                    x = -50.0f;
                    y = engine::Math::randomFloat(0, size.height);
                    break;
                default: // Right
                    x = size.width + 50.0f;
                    y = engine::Math::randomFloat(0, size.height);
                    break;
            }
        } else {
            x = engine::Math::randomFloat(30, size.width - 30);
            y = engine::Math::randomFloat(30, size.height - 30);
        }

        float vx = engine::Math::randomFloat(-6.0f, 6.0f);
        float vy = engine::Math::randomFloat(-6.0f, 6.0f);
        if (std::abs(vx) < 1.0f) vx = (vx < 0 ? -2.0f : 2.0f);
        if (std::abs(vy) < 1.0f) vy = (vy < 0 ? -2.0f : 2.0f);

        float rotation = engine::Math::randomFloat(0.0f, 360.0f);
        float vRot = engine::Math::randomFloat(-12.0f, 12.0f);

        sprite->setPosition(x, y);
        sprite->setRotation(rotation);

        this->addChild(sprite);
        m_floatingCrew.push_back({sprite, x, y, vx, vy, rotation, vRot});
    }

    void MenuSceneTop::update(float dt) {
        engine::Scene::update(dt);

        auto size = engine::Director::getInstance().getTopScreenSize();

        for (auto& crew : m_floatingCrew) {
            crew.x += crew.vx * dt;
            crew.y += crew.vy * dt;
            crew.rotation += crew.vRot * dt;

            crew.sprite->setPosition(crew.x, crew.y);
            crew.sprite->setRotation(crew.rotation);

            bool outOfBounds = (crew.x < -70.0f || crew.x > size.width + 70.0f ||
                                crew.y < -70.0f || crew.y > size.height + 70.0f);

            if (outOfBounds) {
                int edge = engine::Math::randomInt(0, 3);
                switch (edge) {
                    case 0:
                        crew.x = engine::Math::randomFloat(0, size.width);
                        crew.y = -50.0f;
                        break;
                    case 1:
                        crew.x = engine::Math::randomFloat(0, size.width);
                        crew.y = size.height + 50.0f;
                        break;
                    case 2:
                        crew.x = -50.0f;
                        crew.y = engine::Math::randomFloat(0, size.height);
                        break;
                    default:
                        crew.x = size.width + 50.0f;
                        crew.y = engine::Math::randomFloat(0, size.height);
                        break;
                }
                crew.vx = engine::Math::randomFloat(-6.0f, 6.0f);
                crew.vy = engine::Math::randomFloat(-6.0f, 6.0f);
                if (std::abs(crew.vx) < 1.0f) crew.vx = (crew.vx < 0 ? -2.0f : 2.0f);
                if (std::abs(crew.vy) < 1.0f) crew.vy = (crew.vy < 0 ? -2.0f : 2.0f);
                crew.vRot = engine::Math::randomFloat(-12.0f, 12.0f);
            }
        }
    }

    std::shared_ptr<MenuSceneBottom> MenuSceneBottom::create() {
        auto scene = std::make_shared<MenuSceneBottom>();
        scene->init();
        return scene;
    }

    void MenuSceneBottom::init() {
        auto size = engine::Director::getInstance().getBottomScreenSize();

        auto& settings = game::Settings::getInstance();
        settings.load();
        auto& audio = engine::AudioEngine::getInstance();
        audio.setSfxVolume(settings.sfxVolume);
        audio.setMusicVolume(settings.musicVolume);

        m_starfield = Starfield::create(size.width, size.height, StarDirection::Right, 25.f);
        this->addChild(m_starfield);

        m_menu = engine::Menu::create();

        struct ButtonDef {
            std::string path;
            float x;
            float y;
            float scale;
            bool isRound;
            const char* name;
            std::function<void()> onClick;
        };

        const ButtonDef defs[] = {
            { 
                "sdmc:/3ds/AmongUs3DS/assets/sprites/local.png", 84.5f, 53.0f, 0.70f, false, "Local", 
                []() {
                    std::printf("[Menu] Opening local lobby\n");
                    auto& director = engine::Director::getInstance();
                    director.setTopScene(LocalLobbyTop::create());
                    director.setBottomScene(LocalLobbyBottom::create());
                }
            },
            { 
                "sdmc:/3ds/AmongUs3DS/assets/sprites/online.png", 235.5f, 53.0f, 0.70f, false, "Online", 
                []() { std::printf("[Menu] Online button clicked\n"); } 
            },
            { 
                "sdmc:/3ds/AmongUs3DS/assets/sprites/htp.png", 84.5f, 112.0f, 0.70f, false, "How to Play", 
                [this]() {
                    std::printf("[Menu] Opening instructions\n");

                    if (instructionsPopup) {
                        instructionsPopup->close();
                        return;
                    }

                    auto popup = Popup::create(
                        "HOW TO PLAY",
                        "Move: Circle Pad or D-pad\n"
                        "Touch: Select a menu button\n"
                        "A: Confirm selection\n"
                        "B: Close this manual\n\n"
                        "Complete tasks and watch for impostors.\n"
                        "Use the menu buttons to access game modes.");
                    if (!popup) return;

                    auto size = engine::Director::getInstance().getBottomScreenSize();
                    popup->setPanelSize(292.0f, 210.0f);
                    popup->centerAt(size.width, size.height);
                    popup->setZOrder(10.0f);

                    std::weak_ptr<Popup> weakPopup = popup;
                    popup->setOnClose([this, weakPopup]() {
                        if (auto active = weakPopup.lock()) {
                            removeChild(active);
                        }
                        instructionsPopup.reset();
                        setMenuLocked(false);
                    });

                    instructionsPopup = popup;
                    setMenuLocked(true);
                    addChild(popup);
                    popup->show();
                } 
            },
            { 
                "sdmc:/3ds/AmongUs3DS/assets/sprites/freeplay.png", 235.5f, 112.0f, 0.70f, false, "Freeplay", 
                []() { std::printf("[Menu] Freeplay button clicked\n"); } 
            },
            { 
                "sdmc:/3ds/AmongUs3DS/assets/sprites/annoucementButton.png", 37.5f, 186.0f, 0.60f, true, "Announcements", 
                [this]() {
                    std::printf("[Menu] Announcements button clicked\n");

                    if (announcementsPopup) {
                        auto popup = announcementsPopup;
                        popup->close();
                        return;
                    }
                    
                    auto titleSprite = engine::Sprite::create("sdmc:/3ds/AmongUs3DS/assets/sprites/announcementsBanner.png");
                    titleSprite->setScale(0.6f);

                    auto popup = Popup::create(
                        titleSprite,
                        "Whooah there, Crewmates!\n\n"
                        "Welcome to Among Us: 3DS Edition.\n"
                        "Use the touch screen to browse the menus.\n"
                        "Press B to dismiss this message.");
                    if (!popup) return;

                    auto& director = engine::Director::getInstance();
                    auto topScene = director.getTopScene();
                    if (!topScene) return;

                    auto screen = director.getTopScreenSize();
                    popup->setPanelSize(320.0f, 200.0f);
                    popup->centerAt(screen.width, screen.height);
                    popup->setZOrder(10.0f);

                    std::weak_ptr<Popup> weakPopup = popup;
                    popup->setOnClose([this, weakPopup]() {
                        if (auto active = weakPopup.lock()) {
                            auto top = engine::Director::getInstance().getTopScene();
                            if (top) top->removeChild(active);
                        }
                        announcementsPopup.reset();
                        setMenuLocked(false);
                    });

                    announcementsPopup = popup;
                    setMenuLocked(true, announcementsButton);
                    topScene->addChild(popup);
                    popup->show();
                }
            },
            { 
                "sdmc:/3ds/AmongUs3DS/assets/sprites/settingsButton.png", 98.75f, 186.0f, 0.60f, true, "Settings", 
                [this]() {
                    std::printf("[Menu] Settings button clicked\n");

                    if (settingsPopup) {
                        // Keep a local reference: close() clears settingsPopup inside
                        // its own callback, which would otherwise free the popup
                        // while close() is still running.
                        auto popup = settingsPopup;
                        popup->close();
                        return;
                    }

                    auto content = SettingsPopup::create();
                    if (!content) return;

                    auto popup = Popup::create("");
                    if (!popup) return;

                    auto& director = engine::Director::getInstance();
                    auto topScene = director.getTopScene();
                    if (!topScene) return;

                    auto screen = director.getTopScreenSize();
                    popup->setPanelSize(380.0f, 190.0f);
                    popup->setContent(content);
                    popup->centerAt(screen.width, screen.height);
                    popup->setZOrder(10.0f);

                    std::weak_ptr<Popup> weakPopup = popup;
                    popup->setOnClose([this, weakPopup]() {
                        if (auto active = weakPopup.lock()) {
                            auto top = engine::Director::getInstance().getTopScene();
                            if (top) top->removeChild(active);
                        }
                        settingsPopup.reset();
                        setMenuLocked(false);
                    });

                    settingsPopup = popup;
                    setMenuLocked(true, settingsButton);
                    topScene->addChild(popup);
                    popup->show();
                }
            },
            { 
                "sdmc:/3ds/AmongUs3DS/assets/sprites/credits.png", 160.0f, 186.0f, 0.60f, true, "Credits", 
                [this]() {
                    if (creditsPopup) {
                        creditsPopup->close();
                        return;
                    }

                    auto popup = Popup::create(
                        "CREDITS",
                        "Among Us: 3DS Edition\n\n"
                        "Porting\n"
                        "OmgRod\n\n"
                        "Built for Nintendo 3DS\n"
                        "with the devkitPro toolchain\n\n"
                        "Press B to close.");
                    if (!popup) return;

                    auto& director = engine::Director::getInstance();
                    auto screen = director.getTopScreenSize();
                    popup->setPanelSize(300.0f, 190.0f);
                    popup->centerAt(screen.width, screen.height);
                    popup->setZOrder(10.0f);

                    std::weak_ptr<Popup> weakPopup = popup;
                    popup->setOnClose([this, weakPopup]() {
                        if (auto active = weakPopup.lock()) {
                            auto top = engine::Director::getInstance().getTopScene();
                            if (top) top->removeChild(active);
                        }
                        creditsPopup.reset();
                        setMenuLocked(false);
                    });

                    creditsPopup = popup;
                    setMenuLocked(true);
                    director.getTopScene()->addChild(popup);
                    popup->show();
                }
            },
            { 
                "sdmc:/3ds/AmongUs3DS/assets/sprites/statsButton.png", 221.25f, 186.0f, 0.60f, true, "Stats", 
                []() { std::printf("[Menu] Stats button clicked\n"); } 
            },
            { 
                "sdmc:/3ds/AmongUs3DS/assets/sprites/store.png", 282.5f, 186.0f, 0.60f, true, "Store", 
                []() { std::printf("[Menu] Store button clicked\n"); } 
            }
        };

        for (const auto& def : defs) {
            auto btn = engine::Button::create(def.path, def.onClick);

            if (btn) {
                btn->setScale(def.scale);
                btn->setAnchorPoint(0.0f, 0.0f);
                btn->setPosition(
                    def.x - (btn->getWidth() * def.scale * 0.5f),
                    def.y - (btn->getHeight() * def.scale * 0.5f));
                btn->setIsRound(def.isRound);
                btn->setClickSound("sdmc:/3ds/AmongUs3DS/assets/audio/UI_Select.wav");
                m_menu->addButton(btn);

                if (def.name && std::string(def.name) == "Announcements") {
                    announcementsButton = btn;
                } else if (def.name && std::string(def.name) == "Settings") {
                    settingsButton = btn;
                }
            }
        }

        this->addChild(m_menu);
    }

    void MenuSceneBottom::setMenuLocked(bool locked, const std::shared_ptr<engine::Button>& keepEnabled) {
        if (!m_menu) return;

        for (const auto& button : m_menu->getButtons()) {
            if (!button) continue;

            button->setEnabled(!locked || button == keepEnabled);
            if (locked) {
                button->setPressed(false);
            }
        }

        m_menu->setNavigationEnabled(!locked);

        if (locked) {
            m_menu->setSelectedIndex(-1, false);
        }
    }

    void MenuSceneBottom::update(float dt) {
        engine::Scene::update(dt);
    }

} // namespace game