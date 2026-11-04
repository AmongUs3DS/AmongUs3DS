#include <game/objects/SettingsPopup.hpp>
#include <game/Settings.hpp>
#include <engine/audio.hpp>
#include <engine/input.hpp>

namespace game {

    namespace {
        constexpr const char* kFontPath = "romfs:/fonts/arial.bcfnt";
        constexpr const char* kButtonSpritePath = "sdmc:/3ds/AmongUs3DS/assets/sprites/buttonEnterText.png";

        constexpr float kContentWidth = 316.0f;
        constexpr float kContentHeight = 164.0f;

        constexpr int kVisibleServerRows = 4;
        constexpr float kServerRowHeight = 22.0f;

        const u32 kWhite = C2D_Color32(255, 255, 255, 255);
        const u32 kBlack = C2D_Color32(20, 20, 20, 255);
        const u32 kGrey = C2D_Color32(92, 92, 92, 255);
        const u32 kRowNormal = C2D_Color32(255, 255, 255, 18);
        const u32 kRowFocused = C2D_Color32(255, 255, 255, 62);
        const u32 kRowHidden = C2D_Color32(255, 255, 255, 0);

        /** @brief Creates a label using the shared UI font. */
        std::shared_ptr<engine::Label> makeLabel(const std::string& text, float scale, u32 color) {
            auto label = engine::Label::create(kFontPath, text, scale, color);
            if (label) {
                label->setAnchorPoint(0.0f, 0.0f);
                label->setAlignment(engine::Label::HAlign::Left);
            }
            return label;
        }

        /** @brief Centers a label inside a button rectangle. */
        void centerLabel(const std::shared_ptr<engine::Label>& label, float centerX, float centerY) {
            if (!label) return;
            label->setAnchorPoint(0.5f, 0.5f);
            label->setAlignment(engine::Label::HAlign::Center);
            label->setPosition(centerX, centerY);
        }

        /** @brief Creates a 9-sliced button background from buttonEnterText.png. */
        std::shared_ptr<engine::NineSliceSprite> makeButtonBackground(float width, float height) {
            auto background = engine::NineSliceSprite::create(kButtonSpritePath, 30.0f, 30.0f, 30.0f, 30.0f);
            if (!background) return nullptr;

            background->setAnchorPoint(0.0f, 0.0f);
            background->setContentSize(width, height);
            background->setBorderScale(0.5f);
            background->setTint(kWhite);
            return background;
        }

        /** @brief Truncates long server entries so they stay inside their row. */
        std::string ellipsize(const std::string& value, size_t maxLength) {
            if (value.size() <= maxLength) {
                return value;
            }
            if (maxLength <= 3) {
                return value.substr(0, maxLength);
            }
            return value.substr(0, maxLength - 3) + "...";
        }

        /** @brief Trims leading and trailing whitespace. */
        std::string trim(const std::string& value) {
            const size_t start = value.find_first_not_of(" \t\r\n");
            if (start == std::string::npos) {
                return "";
            }
            const size_t end = value.find_last_not_of(" \t\r\n");
            return value.substr(start, end - start + 1);
        }
    }

    struct SettingsPopup::Impl {
        enum class Tab { General, Servers };

        SettingsPopup& owner;

        Tab tab = Tab::General;
        int focus = 0;
        int serverScroll = 0;

        std::shared_ptr<engine::Node> tabsRoot;
        std::shared_ptr<engine::NineSliceSprite> generalTabBackground;
        std::shared_ptr<engine::NineSliceSprite> serversTabBackground;
        std::shared_ptr<engine::Label> generalTabLabel;
        std::shared_ptr<engine::Label> serversTabLabel;
        std::shared_ptr<engine::DrawNode> tabFocusLine;

        std::shared_ptr<engine::Node> generalPage;
        std::shared_ptr<engine::DrawNode> censorRowBackground;
        std::shared_ptr<engine::Label> censorLabel;
        std::shared_ptr<engine::NineSliceSprite> censorToggleBackground;
        std::shared_ptr<engine::Label> censorToggleLabel;
        std::shared_ptr<engine::Label> generalHintLabel;

        struct ServerRow {
            std::shared_ptr<engine::Node> root;
            std::shared_ptr<engine::DrawNode> background;
            std::shared_ptr<engine::Label> label;
        };

        std::shared_ptr<engine::Node> serversPage;
        std::shared_ptr<engine::DrawNode> addButtonFocusBackground;
        std::shared_ptr<engine::NineSliceSprite> addButtonBackground;
        std::shared_ptr<engine::Label> addButtonLabel;
        std::shared_ptr<engine::Label> serverCountLabel;
        std::shared_ptr<engine::Label> emptyLabel;
        std::shared_ptr<engine::Label> serversHintLabel;
        std::vector<ServerRow> serverRows;

        Impl(SettingsPopup& ownerRef) : owner(ownerRef) {
            build();
        }

        void build() {
            auto& settings = Settings::getInstance();
            settings.load();

            owner.setContentSize(kContentWidth, kContentHeight);

            buildTabs();
            buildGeneralPage();
            buildServersPage();

            refresh();
        }

        void buildTabs() {
            tabsRoot = engine::Node::create();
            owner.addChild(tabsRoot);

            generalTabBackground = makeButtonBackground(96.0f, 22.0f);
            if (generalTabBackground) {
                tabsRoot->addChild(generalTabBackground);
            }

            generalTabLabel = makeLabel("General", 0.45f, kWhite);
            centerLabel(generalTabLabel, 48.0f, 11.0f);
            if (generalTabLabel) {
                tabsRoot->addChild(generalTabLabel);
            }

            serversTabBackground = makeButtonBackground(96.0f, 22.0f);
            if (serversTabBackground) {
                serversTabBackground->setPosition(102.0f, 0.0f);
                tabsRoot->addChild(serversTabBackground);
            }

            serversTabLabel = makeLabel("Servers", 0.45f, kWhite);
            centerLabel(serversTabLabel, 150.0f, 11.0f);
            if (serversTabLabel) {
                tabsRoot->addChild(serversTabLabel);
            }

            tabFocusLine = engine::DrawNode::createRect(kContentWidth, 2.0f, C2D_Color32(255, 255, 255, 170));
            if (tabFocusLine) {
                tabFocusLine->setPosition(0.0f, 23.0f);
                tabsRoot->addChild(tabFocusLine);
            }
        }

        std::shared_ptr<engine::DrawNode> addRowBackground(
            const std::shared_ptr<engine::Node>& page,
            float x, float y, float width, float height, u32 color) {
            auto background = engine::DrawNode::createRect(width, height, color);
            if (background) {
                background->setPosition(x, y);
                page->addChild(background);
            }
            return background;
        }

        void buildGeneralPage() {
            generalPage = engine::Node::create();
            owner.addChild(generalPage);

            censorRowBackground = addRowBackground(generalPage, 0.0f, 32.0f, kContentWidth, 22.0f, kRowHidden);

            censorLabel = makeLabel("Censor Chat", 0.42f, kWhite);
            if (censorLabel) {
                censorLabel->setPosition(0.0f, 36.0f);
                generalPage->addChild(censorLabel);
            }

            censorToggleBackground = makeButtonBackground(76.0f, 22.0f);
            if (censorToggleBackground) {
                censorToggleBackground->setPosition(240.0f, 32.0f);
                generalPage->addChild(censorToggleBackground);
            }
            censorToggleLabel = makeLabel("ON", 0.42f, kWhite);
            centerLabel(censorToggleLabel, 278.0f, 43.0f);
            if (censorToggleLabel) {
                generalPage->addChild(censorToggleLabel);
            }

            generalHintLabel = makeLabel(
                "L/R: Switch Tab   D-Pad: Navigate   A: Toggle   B: Close",
                0.32f, kWhite);
            if (generalHintLabel) {
                generalHintLabel->setPosition(0.0f, 145.0f);
                generalPage->addChild(generalHintLabel);
            }
        }

        void buildServersPage() {
            serversPage = engine::Node::create();
            owner.addChild(serversPage);

            addButtonFocusBackground = addRowBackground(serversPage, 0.0f, 32.0f, kContentWidth, 26.0f, kRowHidden);

            addButtonBackground = makeButtonBackground(120.0f, 22.0f);
            if (addButtonBackground) {
                addButtonBackground->setPosition(0.0f, 34.0f);
                serversPage->addChild(addButtonBackground);
            }
            addButtonLabel = makeLabel("Add Server", 0.42f, kWhite);
            centerLabel(addButtonLabel, 60.0f, 45.0f);
            if (addButtonLabel) {
                serversPage->addChild(addButtonLabel);
            }

            serverCountLabel = makeLabel("0 saved", 0.38f, kWhite);
            if (serverCountLabel) {
                serverCountLabel->setAnchorPoint(1.0f, 0.0f);
                serverCountLabel->setAlignment(engine::Label::HAlign::Right);
                serverCountLabel->setPosition(kContentWidth, 38.0f);
                serversPage->addChild(serverCountLabel);
            }

            emptyLabel = makeLabel("No servers saved yet.", 0.42f, kWhite);
            if (emptyLabel) {
                emptyLabel->setPosition(0.0f, 66.0f);
                serversPage->addChild(emptyLabel);
            }

            serverRows.reserve(kVisibleServerRows);
            for (int i = 0; i < kVisibleServerRows; ++i) {
                ServerRow row;
                row.root = engine::Node::create();
                row.root->setPosition(0.0f, 60.0f + static_cast<float>(i) * kServerRowHeight);
                row.root->setVisible(false);

                row.background = engine::DrawNode::createRect(kContentWidth, 20.0f, kRowNormal);
                if (row.background) {
                    row.root->addChild(row.background);
                }

                row.label = makeLabel("", 0.40f, kWhite);
                if (row.label) {
                    row.label->setPosition(8.0f, 5.0f);
                    row.root->addChild(row.label);
                }

                serversPage->addChild(row.root);
                serverRows.push_back(row);
            }

            serversHintLabel = makeLabel(
                "L/R: Switch Tab   D-Pad: Navigate   A: Add   X: Remove   B: Close",
                0.32f, kWhite);
            if (serversHintLabel) {
                serversHintLabel->setPosition(0.0f, 156.0f);
                serversPage->addChild(serversHintLabel);
            }
        }

        void refreshTabs() {
            const bool generalSelected = (tab == Tab::General);

            if (generalTabBackground) {
                generalTabBackground->setTint(generalSelected ? kWhite : kGrey);
            }
            if (generalTabLabel) {
                generalTabLabel->setColor(kWhite);
            }
            if (serversTabBackground) {
                serversTabBackground->setTint(generalSelected ? kGrey : kWhite);
            }
            if (serversTabLabel) {
                serversTabLabel->setColor(kWhite);
            }
            if (tabFocusLine) {
                tabFocusLine->setVisible(false);
            }
            if (generalPage) {
                generalPage->setVisible(generalSelected);
            }
            if (serversPage) {
                serversPage->setVisible(!generalSelected);
            }
        }

        void refreshGeneral() {
            auto& settings = Settings::getInstance();

            if (censorToggleBackground) {
                censorToggleBackground->setTint(settings.censorChat ? kWhite : kGrey);
            }
            if (censorToggleLabel) {
                censorToggleLabel->setString(settings.censorChat ? "ON" : "OFF");
                censorToggleLabel->setColor(kWhite);
            }
        }

        void setFocusBackground(const std::shared_ptr<engine::DrawNode>& background, bool focused) {
            if (background) {
                background->setColor(focused ? kRowFocused : kRowHidden);
            }
        }

        void refreshFocus() {
            const bool generalSelected = (tab == Tab::General);

            setFocusBackground(censorRowBackground, generalSelected && focus == 0);

            const int serverCount = static_cast<int>(Settings::getInstance().servers.size());
            setFocusBackground(addButtonFocusBackground, !generalSelected && focus == 0);

            for (int i = 0; i < static_cast<int>(serverRows.size()); ++i) {
                const int serverIndex = serverScroll + i;
                const bool focused = !generalSelected && serverIndex < serverCount && focus == serverIndex + 1;
                setFocusBackground(serverRows[i].background, focused);
            }
        }

        bool updateScrollForFocus() {
            if (tab != Tab::Servers) {
                serverScroll = 0;
                return false;
            }
            const int serverCount = static_cast<int>(Settings::getInstance().servers.size());
            const int oldScroll = serverScroll;

            if (serverCount <= 0 || focus == 0) {
                serverScroll = 0;
            } else {
                const int row = focus - 1;
                if (row >= 0 && row < serverCount) {
                    if (row < serverScroll) {
                        serverScroll = row;
                    }
                    if (row >= serverScroll + kVisibleServerRows) {
                        serverScroll = row - kVisibleServerRows + 1;
                    }
                }
                serverScroll = std::max(0, std::min(serverScroll, std::max(0, serverCount - kVisibleServerRows)));
            }

            return serverScroll != oldScroll;
        }

        void refreshServers() {
            auto& settings = Settings::getInstance();
            const int serverCount = static_cast<int>(settings.servers.size());

            if (emptyLabel) {
                emptyLabel->setVisible(serverCount == 0);
            }

            if (serverCountLabel) {
                if (serverCount > kVisibleServerRows) {
                    const int first = serverScroll + 1;
                    const int last = std::min(serverScroll + kVisibleServerRows, serverCount);
                    serverCountLabel->setString(std::to_string(first) + "-" + std::to_string(last) + " / " + std::to_string(serverCount));
                } else {
                    serverCountLabel->setString(std::to_string(serverCount) + (serverCount == 1 ? " server" : " servers"));
                }
            }

            for (int i = 0; i < static_cast<int>(serverRows.size()); ++i) {
                const int serverIndex = serverScroll + i;
                const bool visible = serverIndex < serverCount;

                if (serverRows[i].root) {
                    serverRows[i].root->setVisible(visible);
                }
                if (visible && serverRows[i].label) {
                    serverRows[i].label->setString(ellipsize(settings.servers[serverIndex], 48));
                }
                if (serverRows[i].background) {
                    serverRows[i].background->setColor(kRowNormal);
                }
            }
        }

        void refresh() {
            refreshTabs();
            refreshGeneral();
            refreshServers();
            refreshFocus();
        }

        void moveFocus(int delta) {
            int maxFocus = 0; // General: 0 (Censor)
            if (tab == Tab::Servers) {
                maxFocus = static_cast<int>(Settings::getInstance().servers.size()); // 0 = Add button, 1..N = servers
            }

            const int newFocus = std::max(0, std::min(focus + delta, maxFocus));
            if (newFocus == focus) {
                return;
            }

            focus = newFocus;

            if (tab == Tab::Servers) {
                if (updateScrollForFocus()) {
                    refreshServers();
                }
            }
            refreshFocus();
        }

        void switchTab() {
            tab = (tab == Tab::General) ? Tab::Servers : Tab::General;
            focus = 0;
            serverScroll = 0;
            refresh();
        }

        void adjust(int direction) {
            // No volume sliders on General tab
        }

        void addServer() {
            auto& settings = Settings::getInstance();

            std::string value = trim(engine::Input::getInstance().getTextInput("Server address", "", 128));
            if (value.empty()) {
                return;
            }

            if (std::find(settings.servers.begin(), settings.servers.end(), value) != settings.servers.end()) {
                return;
            }

            settings.servers.push_back(value);
            settings.save();

            focus = static_cast<int>(settings.servers.size());
            updateScrollForFocus();
            refreshServers();
            refreshFocus();
        }

        void removeFocusedServer() {
            if (tab != Tab::Servers) {
                return;
            }

            auto& settings = Settings::getInstance();
            const int serverCount = static_cast<int>(settings.servers.size());
            if (focus < 1 || focus > serverCount) {
                return;
            }

            settings.servers.erase(settings.servers.begin() + (focus - 1));
            settings.save();

            const int newCount = static_cast<int>(settings.servers.size());
            focus = std::min(focus, newCount);
            updateScrollForFocus();
            refreshServers();
            refreshFocus();
        }

        void activate() {
            if (tab == Tab::General) {
                auto& settings = Settings::getInstance();
                if (focus == 0) {
                    settings.censorChat = !settings.censorChat;
                    settings.save();
                    refreshGeneral();
                    refreshFocus();
                }
                return;
            }

            const int serverCount = static_cast<int>(Settings::getInstance().servers.size());
            if (focus == 0) {
                addServer();
            }
        }

        void update() {
            auto& input = engine::Input::getInstance();

            if (input.isLDown()) {
                switchTab();
                return;
            }
            if (input.isRDown()) {
                switchTab();
                return;
            }

            if (input.isDpadUp()) {
                moveFocus(-1);
            } else if (input.isDpadDown()) {
                moveFocus(1);
            }

            if (input.isDpadLeft()) {
                adjust(-1);
            } else if (input.isDpadRight()) {
                adjust(1);
            }

            if (input.isAKeyDown()) {
                activate();
            } else if (input.isXKeyDown()) {
                removeFocusedServer();
            }

            refreshFocus();
        }
    };

    SettingsPopup::SettingsPopup() : m_impl(std::make_unique<Impl>(*this)) {
    }

    SettingsPopup::~SettingsPopup() = default;

    std::shared_ptr<SettingsPopup> SettingsPopup::create() {
        return std::shared_ptr<SettingsPopup>(new SettingsPopup());
    }

    void SettingsPopup::update(float dt) {
        engine::Node::update(dt);
        if (m_impl) {
            m_impl->update();
        }
    }

} // namespace game