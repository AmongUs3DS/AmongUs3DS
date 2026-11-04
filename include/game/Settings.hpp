#pragma once

#include <includes.hpp>

namespace game {

    /**
     * @brief Persistent user settings shared across popups and sessions.
     */
    class Settings {
    public:
        /** @brief Gets the singleton settings instance. @return Reference to Settings. */
        static Settings& getInstance();

        /** @brief Loads settings and the saved server list from the SD card once. */
        void load();

        /** @brief Saves the current settings and server list to the SD card. */
        void save() const;

        float sfxVolume = 1.0f;          ///< Sound-effect volume [0, 1].
        float musicVolume = 1.0f;        ///< Music volume [0, 1].
        bool censorChat = true;          ///< Whether chat censoring is enabled.
        int controlScheme = 1;           ///< 0 = Mouse, 1 = Mouse + Keyboard.
        std::vector<std::string> servers; ///< Saved server addresses/names.

    private:
        Settings() = default;

        bool m_loaded = false;           ///< Whether the files have been read.
    };

} // namespace game
