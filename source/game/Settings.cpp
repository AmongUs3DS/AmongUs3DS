#include <game/Settings.hpp>
#include <fstream>
#include <sys/stat.h>

namespace game {

    namespace {
        constexpr const char* kSettingsPath = "sdmc:/3ds/AmongUs3DS/settings.txt";
        constexpr const char* kServersPath = "sdmc:/3ds/AmongUs3DS/servers.txt";

        /** @brief Trims leading and trailing whitespace from a string. */
        std::string trim(const std::string& value) {
            const size_t start = value.find_first_not_of(" \t\r\n");
            if (start == std::string::npos) {
                return "";
            }
            const size_t end = value.find_last_not_of(" \t\r\n");
            return value.substr(start, end - start + 1);
        }

        /** @brief Clamps a floating point value to [0, 1]. */
        float clamp01(float value) {
            return std::max(0.0f, std::min(1.0f, value));
        }
    }

    Settings& Settings::getInstance() {
        static Settings instance;
        return instance;
    }

    void Settings::load() {
        if (m_loaded) {
            return;
        }
        m_loaded = true;

        mkdir("sdmc:/3ds/AmongUs3DS", 0777);

        std::ifstream input(kSettingsPath);
        std::string line;
        while (std::getline(input, line)) {
            line = trim(line);
            if (line.empty() || line[0] == '#') {
                continue;
            }

            const size_t equals = line.find('=');
            if (equals == std::string::npos) {
                continue;
            }

            const std::string key = trim(line.substr(0, equals));
            const std::string value = trim(line.substr(equals + 1));

            if (key == "sfxVolume") {
                sfxVolume = clamp01(std::strtof(value.c_str(), nullptr));
            } else if (key == "musicVolume") {
                musicVolume = clamp01(std::strtof(value.c_str(), nullptr));
            } else if (key == "censorChat") {
                censorChat = (value == "1" || value == "true" || value == "on");
            } else if (key == "controlScheme") {
                const int scheme = static_cast<int>(std::strtol(value.c_str(), nullptr, 10));
                controlScheme = std::clamp(scheme, 0, 1);
            }
        }

        std::ifstream serversInput(kServersPath);
        while (std::getline(serversInput, line)) {
            line = trim(line);
            if (!line.empty()) {
                servers.push_back(line);
            }
        }
    }

    void Settings::save() const {
        mkdir("sdmc:/3ds/AmongUs3DS", 0777);

        std::ofstream output(kSettingsPath, std::ios::trunc);
        if (output) {
            output << "sfxVolume=" << sfxVolume << "\n";
            output << "musicVolume=" << musicVolume << "\n";
            output << "censorChat=" << (censorChat ? 1 : 0) << "\n";
            output << "controlScheme=" << controlScheme << "\n";
        }

        std::ofstream serversOutput(kServersPath, std::ios::trunc);
        if (serversOutput) {
            for (const auto& server : servers) {
                serversOutput << server << "\n";
            }
        }
    }

} // namespace game
