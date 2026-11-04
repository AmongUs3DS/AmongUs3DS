#pragma once

#include <includes.hpp>

namespace game {

    /**
     * @brief Interactive settings panel shown inside a top-screen Popup.
     *
     * The top screen is not touch-enabled, so the panel is controlled with the
     * D-Pad (navigate / adjust), A (activate), X (remove focused server) and B
     * (close, handled by the owning Popup).
     */
    class SettingsPopup : public engine::Node {
    public:
        /** @brief Creates a settings panel content node. @return Shared pointer to SettingsPopup. */
        static std::shared_ptr<SettingsPopup> create();

        ~SettingsPopup() override;

        /** @brief Handles controller input for the settings panel. @param dt Delta time in seconds. */
        void update(float dt) override;

    private:
        SettingsPopup();

        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };

} // namespace game
