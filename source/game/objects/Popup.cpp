#include <game/objects/Popup.hpp>

namespace game {

    namespace {
        constexpr const char* kBackgroundPath = "sdmc:/3ds/AmongUs3DS/assets/sprites/buttonEnterText.png";
        constexpr const char* kFontPath = "romfs:/fonts/arial.bcfnt";
        constexpr float kBackgroundInset = 30.0f; // Adjusted for smoother rounded corners when scaled
    }

    Popup::Popup() {
        build();
    }

    std::shared_ptr<Popup> Popup::create(const std::string& titleText, const std::string& body, std::function<void()> onClose) {
        auto popup = std::shared_ptr<Popup>(new Popup());
        popup->m_onClose = std::move(onClose);
        popup->setTitle(titleText);
        popup->setBody(body);
        popup->applyLayout();
        popup->hide();
        return popup;
    }

    std::shared_ptr<Popup> Popup::create(std::shared_ptr<engine::Sprite> titleSprite, const std::string& body, std::function<void()> onClose) {
        auto popup = std::shared_ptr<Popup>(new Popup());
        popup->m_onClose = std::move(onClose);
        popup->setTitleSprite(titleSprite);
        popup->setBody(body);
        popup->applyLayout();
        popup->hide();
        return popup;
    }

    void Popup::build() {
        m_background = engine::NineSliceSprite::create(
            kBackgroundPath,
            kBackgroundInset, kBackgroundInset, kBackgroundInset, kBackgroundInset);
            
        if (m_background) {
            m_background->setScale(m_bgScale);
            m_background->setAnchorPoint(0.5f, 0.5f);
            m_background->setZOrder(0.0f);
            addChild(m_background);
        } else {
            m_backgroundFallback = engine::DrawNode::createRect(m_panelWidth, m_panelHeight, C2D_Color32(0, 0, 0, 255));
            m_backgroundFallback->setAnchorPoint(0.5f, 0.5f);
            m_backgroundFallback->setZOrder(0.0f);
            addChild(m_backgroundFallback);
        }

        m_titleLabel = engine::Label::create(kFontPath, "", m_titleScale, C2D_Color32(255, 255, 255, 255));
        if (m_titleLabel) {
            m_titleLabel->setAnchorPoint(0.5f, 0.0f);
            m_titleLabel->setAlignment(engine::Label::HAlign::Center);
            m_titleLabel->setZOrder(1.0f);
            addChild(m_titleLabel);
        }

        m_contentRoot = engine::Node::create();
        if (m_contentRoot) {
            m_contentRoot->setZOrder(2.0f);
            addChild(m_contentRoot);
        }

        m_bodyLabel = engine::Label::create(
            kFontPath, "", m_bodyScale, C2D_Color32(255, 255, 255, 255));
        if (m_bodyLabel) {
            m_bodyLabel->setAnchorPoint(0.0f, 0.0f);
            m_bodyLabel->setAlignment(engine::Label::HAlign::Left);
            m_bodyLabel->setZOrder(0.0f);
            if (m_contentRoot) {
                m_contentRoot->addChild(m_bodyLabel);
            }
        }
    }

    void Popup::setTitle(const std::string& titleText) {
        if (m_titleSprite) m_titleSprite->setVisible(false);
        
        if (m_titleLabel) {
            m_titleLabel->setString(titleText);
            m_titleLabel->setVisible(!titleText.empty());
        }
        applyLayout();
    }

    void Popup::setTitleSprite(std::shared_ptr<engine::Sprite> titleSprite) {
        if (m_titleLabel) m_titleLabel->setVisible(false);
        
        if (m_titleSprite) {
            removeChild(m_titleSprite);
        }
        
        m_titleSprite = std::move(titleSprite);
        if (m_titleSprite) {
            m_titleSprite->setAnchorPoint(0.5f, 0.0f); // Top-center alignment
            m_titleSprite->setZOrder(1.0f);
            addChild(m_titleSprite);
        }
        applyLayout();
    }

    void Popup::setBody(const std::string& body) {
        m_body = body;

        if (m_contentNode && m_contentRoot) {
            m_contentRoot->removeChild(m_contentNode);
            m_contentNode.reset();
        }

        if (m_bodyLabel) {
            m_bodyLabel->setString(body);
            m_bodyLabel->setVisible(!body.empty());
            if (m_contentRoot && m_bodyLabel->getParent() != m_contentRoot.get()) {
                m_contentRoot->addChild(m_bodyLabel);
            }
        }

        applyLayout();
    }

    void Popup::setContent(std::shared_ptr<engine::Node> content) {
        if (m_contentNode && m_contentRoot) {
            m_contentRoot->removeChild(m_contentNode);
        }

        m_contentNode = std::move(content);

        if (m_bodyLabel) {
            m_bodyLabel->setVisible(m_contentNode == nullptr && !m_body.empty());
        }

        if (m_contentNode && m_contentRoot) {
            m_contentNode->setPosition(0.0f, 0.0f);
            m_contentRoot->addChild(m_contentNode);
        }

        applyLayout();
    }

    void Popup::setPanelSize(float width, float height) {
        m_panelWidth = std::max(120.0f, width);
        m_panelHeight = std::max(100.0f, height);
        applyLayout();
    }

    void Popup::centerAt(float screenWidth, float screenHeight) {
        setPosition(screenWidth * 0.5f, screenHeight * 0.5f);
    }

    void Popup::applyLayout() {
        const float panelCenterX = 0.0f;
        const float panelLeft = -(m_panelWidth * 0.5f);
        const float panelTop = -(m_panelHeight * 0.5f);

        setContentSize(m_panelWidth, m_panelHeight);

        if (m_background) {
            m_background->setPosition(panelCenterX, 0.0f);
            // Multiply layout size by inverse of scale so the physical footprint remains identical
            m_background->setContentSize(m_panelWidth / m_bgScale, m_panelHeight / m_bgScale);
        }
        if (m_backgroundFallback) {
            m_backgroundFallback->setPosition(panelCenterX, 0.0f);
            m_backgroundFallback->setSize(m_panelWidth, m_panelHeight);
        }

        const float contentWidth = m_panelWidth - (m_padding * 2.0f);
        const float contentLeft = panelLeft + m_padding;
        const float titleTop = panelTop + m_padding;

        float titleHeight = 0.0f;

        if (m_titleSprite && m_titleSprite->isVisible()) {
            m_titleSprite->setPosition(panelCenterX, titleTop);
            titleHeight = m_titleSprite->getHeight() * m_titleSprite->getScaleY();
        } 
        else if (m_titleLabel && m_titleLabel->isVisible()) {
            m_titleLabel->setTextScale(m_titleScale);
            m_titleLabel->setPosition(panelCenterX, titleTop);
            titleHeight = m_titleLabel->getHeight() * m_titleScale;
        }

        // Space out the body content below the title
        const float contentTop = (titleHeight > 0.0f) ? (titleTop + titleHeight + 10.0f) : titleTop;
        
        if (m_contentRoot) {
            m_contentRoot->setPosition(contentLeft, contentTop);
        }

        if (m_bodyLabel) {
            m_bodyLabel->setPosition(0.0f, 0.0f);
            m_bodyLabel->setWrapWidth(contentWidth);
        }
    }

    void Popup::show() {
        stopAllActions();
        setScale(0.0f, 0.0f);
        setVisible(true);
        runAction(engine::ScaleTo::create(m_openDuration, 1.0f, 1.0f, engine::Ease::Linear));
    }

    void Popup::hide() {
        stopAllActions();
        setVisible(false);
    }

    void Popup::close() {
        if (!isVisible()) return;

        hide();

        // The close callback may remove this popup from its scene and release the
        // last external reference. Keep the popup alive until the callback returns,
        // otherwise we would destroy m_onClose while it is still being executed.
        auto keepAlive = shared_from_this();
        auto onClose = m_onClose;
        if (onClose) {
            onClose();
        }
    }

    void Popup::update(float dt) {
        engine::Node::update(dt);

        if (!isVisible()) return;

        if (engine::Input::getInstance().isBKeyDown()) {
            close();
        }
    }

} // namespace game