#pragma once

#include <includes.hpp>
#include <game/objects/Starfield.hpp>
#include <game/objects/Popup.hpp>

namespace game {

    class LocalLobbyTop : public engine::Scene {
    private:
        std::shared_ptr<Starfield> m_starfield;
        std::shared_ptr<engine::Label> m_status;
        std::shared_ptr<engine::Label> m_playerList;

    public:
        static std::shared_ptr<LocalLobbyTop> create();
        void init();
        void setLobbyCreated(bool created);
        void setNearbyPlayers(int count);
        void update(float dt) override;
    };

    class LocalLobbyBottom : public engine::Scene {
    private:
        std::shared_ptr<Starfield> m_starfield;
        std::shared_ptr<engine::Button> m_createButton;
        std::shared_ptr<engine::Button> m_helpButton;
        std::shared_ptr<engine::Button> m_backButton;
        std::shared_ptr<Popup> m_helpPopup;
        int m_socket = -1;
        bool m_lobbyCreated = false;
        float m_beaconTimer = 0.0f;
        int m_nearbyPlayers = 0;
        std::vector<uint32_t> m_seenPeers;

        void openHelp();
        void closeLobby();
        void createLobby();
        void updateLocalDiscovery();
        void sendLobbyBeacon();

    public:
        static std::shared_ptr<LocalLobbyBottom> create();
        ~LocalLobbyBottom() override;
        void init();
        void update(float dt) override;
    };

} // namespace game
