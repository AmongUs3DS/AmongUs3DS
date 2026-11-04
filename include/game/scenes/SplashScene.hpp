#pragma once

#include <includes.hpp>
#include <game/objects/Starfield.hpp>

/**
 * @brief Game namespace containing game-specific objects, scenes, and color definitions.
 */
namespace game {
    class MenuSceneTop;
    class MenuSceneBottom;

    /**
     * @brief Scene class rendering the splash screen top screen.
     */
    class SplashSceneTop : public engine::Scene {
    private:
        std::shared_ptr<engine::Sprite> m_innerslothLogo;   ///< Innersloth logo sprite.
        std::shared_ptr<engine::DrawNode> m_fadeOut;        ///< Fade out overlay node.
        std::shared_ptr<Starfield> m_starfield;             ///< Background starfield effect.
        std::shared_ptr<MenuSceneTop> m_menuTop;            ///< Pre-loaded menu top scene.
        std::shared_ptr<MenuSceneBottom> m_menuBottom;      ///< Pre-loaded menu bottom scene.

    public:
        /**
         * @brief Factory method to create a shared SplashSceneTop instance.
         * @return Shared pointer to SplashSceneTop.
         */
        static std::shared_ptr<SplashSceneTop> create();
        
        /** @brief Initializes scene nodes and UI. */
        void init();
        
        /** @brief Updates scene state per frame. @param dt Delta time in seconds. */
        void update(float dt);
    };

    /**
     * @brief Scene class rendering the splash screen bottom touchscreen.
     */
    class SplashSceneBottom : public engine::Scene {
    private:
        std::shared_ptr<engine::Label> m_rodCredit;         ///< Credit text label.
        std::shared_ptr<engine::DrawNode> m_fadeOut;        ///< Fade out overlay node.
        std::shared_ptr<Starfield> m_starfield;             ///< Background starfield effect.

    public:
        /**
         * @brief Factory method to create a shared SplashSceneBottom instance.
         * @return Shared pointer to SplashSceneBottom.
         */
        static std::shared_ptr<SplashSceneBottom> create();
        
        /** @brief Initializes scene nodes and UI. */
        void init();
        
        /** @brief Updates scene state per frame. @param dt Delta time in seconds. */
        void update(float dt);
        
        /**
         * @brief Initiates screen fade out transition.
         * @param duration Fade duration in seconds.
         */
        void startFadeOut(float duration);
    };

}
