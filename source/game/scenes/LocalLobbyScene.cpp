#include <game/scenes/LocalLobbyScene.hpp>
#include <game/scenes/MenuScene.hpp>
#include <engine/audio.hpp>
#include <engine/input.hpp>
#include <engine/network.hpp>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

namespace game {

    namespace {
        constexpr int kLobbyPort = 47777;
        constexpr const char* kLobbyBeacon = "AMONGUS3DS_LOCAL_LOBBY_1";
        constexpr u32 kWhite = C2D_Color32(255, 255, 255, 255);
        constexpr u32 kBlack = C2D_Color32(0, 0, 0, 230);
        constexpr u32 kBlue = C2D_Color32(70, 150, 255, 255);

        std::shared_ptr<engine::Label> makeLabel(const std::string& text, float scale, u32 color = kWhite) {
            auto label = engine::Label::create("romfs:/fonts/arial.bcfnt", text, scale, color);
            if (label) {
                label->setAnchorPoint(0.5f, 0.5f);
                label->setAlignment(engine::Label::HAlign::Center);
            }
            return label;
        }

        std::shared_ptr<engine::Button> makeButton(const std::string& text, float x, float y, std::function<void()> callback) {
            auto label = makeLabel(text, 0.5f);
            if (!label) return nullptr;
            auto button = engine::Button::create(label, C2D_Color32(15, 15, 20, 230), C2D_Color32(35, 100, 150, 255), callback);
            if (!button) return nullptr;
            button->setAnchorPoint(0.5f, 0.5f);
            button->setPosition(x, y);
            button->setClickSound("sdmc:/3ds/AmongUs3DS/assets/audio/UI_Select.wav");
            return button;
        }
    }

    std::shared_ptr<LocalLobbyTop> LocalLobbyTop::create() {
        auto scene = std::make_shared<LocalLobbyTop>();
        scene->init();
        return scene;
    }

    void LocalLobbyTop::init() {
        auto size = engine::Director::getInstance().getTopScreenSize();
        m_starfield = Starfield::create(size.width, size.height, StarDirection::Right, 18.0f);
        addChild(m_starfield);

        auto version = makeLabel("v2020.9.22s", 0.36f);
        version->setAnchorPoint(0.0f, 1.0f);
        version->setAlignment(engine::Label::HAlign::Left);
        version->setPosition(10.0f, 8.0f);
        addChild(version);

        auto title = makeLabel("LOCAL WIRELESS", 0.78f);
        title->setPosition(size.width * 0.5f, 34.0f);
        addChild(title);

        auto rule = engine::DrawNode::createRect(310.0f, 2.0f, kWhite);
        rule->setPosition(45.0f, 52.0f);
        addChild(rule);

        m_status = makeLabel("Create a game on the bottom screen", 0.48f);
        m_status->setPosition(size.width * 0.5f, 78.0f);
        addChild(m_status);

        auto panel = engine::DrawNode::createRect(330.0f, 108.0f, kBlack);
        panel->setAnchorPoint(0.5f, 0.5f);
        panel->setPosition(size.width * 0.5f, 143.0f);
        addChild(panel);

        m_playerList = makeLabel("Players\n\nNo local players connected", 0.48f);
        m_playerList->setPosition(size.width * 0.5f, 143.0f);
        addChild(m_playerList);
    }

    void LocalLobbyTop::setLobbyCreated(bool created) {
        if (m_status) {
            m_status->setString(created ? "Game created - waiting for local players" : "Create a game on the bottom screen");
        }
    }

    void LocalLobbyTop::setNearbyPlayers(int count) {
        if (m_playerList) {
            m_playerList->setString("Players\n\nHost: This 3DS\nNearby consoles: " + std::to_string(count));
        }
    }

    void LocalLobbyTop::update(float dt) {
        engine::Scene::update(dt);
    }

    std::shared_ptr<LocalLobbyBottom> LocalLobbyBottom::create() {
        auto scene = std::make_shared<LocalLobbyBottom>();
        scene->init();
        return scene;
    }

    LocalLobbyBottom::~LocalLobbyBottom() {
        if (m_socket >= 0) {
            engine::NetworkManager::getInstance().closeSocket(m_socket);
            m_socket = -1;
        }
    }

    void LocalLobbyBottom::init() {
        auto size = engine::Director::getInstance().getBottomScreenSize();
        m_starfield = Starfield::create(size.width, size.height, StarDirection::Right, 14.0f);
        addChild(m_starfield);

        auto title = makeLabel("LOCAL GAME", 0.72f);
        title->setPosition(size.width * 0.5f, 27.0f);
        addChild(title);

        auto subtitle = makeLabel("Play with nearby 3DS consoles", 0.38f, C2D_Color32(210, 210, 220, 255));
        subtitle->setPosition(size.width * 0.5f, 49.0f);
        addChild(subtitle);

        m_createButton = makeButton("Create Game", size.width * 0.5f, 101.0f, [this]() { createLobby(); });
        if (m_createButton) addChild(m_createButton);

        m_helpButton = makeButton("?", 278.0f, 31.0f, [this]() { openHelp(); });
        if (m_helpButton) addChild(m_helpButton);

        m_backButton = makeButton("Back", 50.0f, 208.0f, [this]() { closeLobby(); });
        if (m_backButton) addChild(m_backButton);

        auto hint = makeLabel("Create Game starts a local lobby instantly", 0.35f, C2D_Color32(190, 190, 200, 255));
        hint->setPosition(size.width * 0.5f, 151.0f);
        addChild(hint);

        auto& network = engine::NetworkManager::getInstance();
        if (network.isInitialized()) {
            m_socket = network.createUdpSocket();
            if (m_socket >= 0) {
                int broadcast = 1;
                setsockopt(m_socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
                network.setNonBlocking(m_socket, true);
                sockaddr_in address{};
                address.sin_family = AF_INET;
                address.sin_port = htons(kLobbyPort);
                address.sin_addr.s_addr = htonl(INADDR_ANY);
                bind(m_socket, reinterpret_cast<sockaddr*>(&address), sizeof(address));
            }
        }
    }

    void LocalLobbyBottom::createLobby() {
        if (m_lobbyCreated) return;
        m_lobbyCreated = true;
        if (m_createButton) m_createButton->setEnabled(false);
        if (auto top = std::static_pointer_cast<LocalLobbyTop>(engine::Director::getInstance().getTopScene())) {
            top->setLobbyCreated(true);
        }
        sendLobbyBeacon();
    }

    void LocalLobbyBottom::sendLobbyBeacon() {
        if (m_socket < 0 || !m_lobbyCreated) return;
        const char prefix[] = { 0x04, 0x02 };
        std::vector<u8> packet(prefix, prefix + sizeof(prefix));
        packet.insert(packet.end(), kLobbyBeacon, kLobbyBeacon + std::strlen(kLobbyBeacon));

        sockaddr_in target{};
        target.sin_family = AF_INET;
        target.sin_port = htons(kLobbyPort);
        target.sin_addr.s_addr = htonl(INADDR_BROADCAST);
        sendto(m_socket, packet.data(), packet.size(), 0,
               reinterpret_cast<sockaddr*>(&target), sizeof(target));
    }

    void LocalLobbyBottom::updateLocalDiscovery() {
        if (m_socket < 0) return;
        char buffer[64] = {};
        sockaddr_in sender{};
        while (true) {
            socklen_t senderLength = sizeof(sender);
            int received = recvfrom(m_socket, buffer, sizeof(buffer) - 1, MSG_DONTWAIT,
                                    reinterpret_cast<sockaddr*>(&sender), &senderLength);
            if (received <= 0) break;
            if (received < 3 || buffer[0] != 0x04 || buffer[1] != 0x02) break;
            if (std::strcmp(buffer + 2, kLobbyBeacon) != 0) continue;

            const uint32_t peer = sender.sin_addr.s_addr;
            if (std::find(m_seenPeers.begin(), m_seenPeers.end(), peer) == m_seenPeers.end()) {
                m_seenPeers.push_back(peer);
            }
        }
        if (m_nearbyPlayers != static_cast<int>(m_seenPeers.size())) {
            m_nearbyPlayers = std::min(static_cast<int>(m_seenPeers.size()), 9);
            if (auto top = std::static_pointer_cast<LocalLobbyTop>(engine::Director::getInstance().getTopScene())) {
                top->setNearbyPlayers(m_nearbyPlayers);
            }
        }
    }

    void LocalLobbyBottom::openHelp() {
        if (m_helpPopup) {
            m_helpPopup->close();
            return;
        }
        auto popup = Popup::create(
            "LOCAL WIRELESS",
            "Create Game starts a lobby immediately.\n\n"
            "Other nearby 3DS consoles can see the\n"
            "host beacon over local Wi-Fi.\n\n"
            "Press B to close this message.");
        if (!popup) return;
        auto size = engine::Director::getInstance().getBottomScreenSize();
        popup->setPanelSize(292.0f, 190.0f);
        popup->centerAt(size.width, size.height);
        popup->setZOrder(10.0f);
        std::weak_ptr<Popup> weakPopup = popup;
        popup->setOnClose([this, weakPopup]() {
            if (auto active = weakPopup.lock()) removeChild(active);
            m_helpPopup.reset();
        });
        m_helpPopup = popup;
        addChild(popup);
        popup->show();
    }

    void LocalLobbyBottom::closeLobby() {
        auto& director = engine::Director::getInstance();
        director.setTopScene(MenuSceneTop::create());
        director.setBottomScene(MenuSceneBottom::create());
    }

    void LocalLobbyBottom::update(float dt) {
        engine::Scene::update(dt);
        m_beaconTimer -= dt;
        if (m_beaconTimer <= 0.0f) {
            m_beaconTimer = 1.0f;
            sendLobbyBeacon();
            updateLocalDiscovery();
        }
    }

} // namespace game
