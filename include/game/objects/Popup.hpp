#pragma once

#include <includes.hpp>
#include <utility>

namespace game {

    class Popup : public engine::Node, public std::enable_shared_from_this<Popup> {
    private:
        std::shared_ptr<engine::NineSliceSprite> m_background;
        std::shared_ptr<engine::DrawNode> m_backgroundFallback;
        std::shared_ptr<engine::Label> m_titleLabel;
        std::shared_ptr<engine::Sprite> m_titleSprite;
        std::shared_ptr<engine::Label> m_bodyLabel;
        std::shared_ptr<engine::Node> m_contentRoot;
        std::shared_ptr<engine::Node> m_contentNode;

        std::string m_body;
        std::function<void()> m_onClose;

        float m_panelWidth = 260.0f;
        float m_panelHeight = 190.0f;
        float m_padding = 32.0f;          // Increased padding to keep text inside the border
        float m_bgScale = 0.5f;           // Scales down the 9-slice border thickness
        float m_titleScale = 0.70f;
        float m_bodyScale = 0.40f;
        float m_openDuration = 0.1f;

        Popup();
        void build();
        void applyLayout();

    public:
        virtual ~Popup() = default;

        // Creates a popup with a text title
        static std::shared_ptr<Popup> create(
            const std::string& titleText,
            const std::string& body = "",
            std::function<void()> onClose = nullptr
        );

        // Creates a popup with a sprite title
        static std::shared_ptr<Popup> create(
            std::shared_ptr<engine::Sprite> titleSprite,
            const std::string& body = "",
            std::function<void()> onClose = nullptr
        );

        void setTitle(const std::string& titleText);
        void setTitleSprite(std::shared_ptr<engine::Sprite> titleSprite);
        void setBody(const std::string& body);
        void setContent(std::shared_ptr<engine::Node> content);
        void setPanelSize(float width, float height);
        void setOnClose(std::function<void()> onClose) { m_onClose = std::move(onClose); }
        void centerAt(float screenWidth, float screenHeight);
        void show();
        void hide();
        void close();
        bool isOpen() const { return isVisible(); }
        void update(float dt) override;
    };

} // namespace game