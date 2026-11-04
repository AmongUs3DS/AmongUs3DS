#pragma once

#include <includes.hpp>
#include <game/objects/Starfield.hpp>

namespace game {

    class Popup;

    struct FloatingCrew {
        std::shared_ptr<engine::Sprite> sprite;
        float x, y;
        float vx, vy;
        float rotation;
        float vRot;
    };

    class MenuSceneTop : public engine::Scene {
    private:
        std::shared_ptr<Starfield> m_starfield;
        std::vector<FloatingCrew> m_floatingCrew;

        void spawnCrewMember(bool outside);

    public:
        static std::shared_ptr<MenuSceneTop> create();
        void init();
        void update(float dt) override;
    };

    class MenuSceneBottom : public engine::Scene {
    private:
        std::shared_ptr<Starfield> m_starfield;
        std::shared_ptr<engine::Menu> m_menu;
        std::shared_ptr<Popup> announcementsPopup;
        std::shared_ptr<Popup> instructionsPopup;
        std::shared_ptr<Popup> creditsPopup;
        std::shared_ptr<engine::Button> announcementsButton;
        std::shared_ptr<Popup> settingsPopup;
        std::shared_ptr<engine::Button> settingsButton;

        void setMenuLocked(bool locked, const std::shared_ptr<engine::Button>& keepEnabled = nullptr);

    public:
        static std::shared_ptr<MenuSceneBottom> create();
        void init();
        void update(float dt) override;
    };

} // namespace game