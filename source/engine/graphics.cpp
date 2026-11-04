#include <engine/graphics.hpp>
#include <engine/actions.hpp>
#include <engine/input.hpp>
#include <engine/audio.hpp>
#include <math.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace engine {

    std::shared_ptr<Node> Node::create() {
        return std::make_shared<Node>();
    }

    void Node::addChild(std::shared_ptr<Node> child) {
        if (!child) return;
        child->setParent(this);
        m_children.push_back(child);
        sortAllChildren();
    }

    void Node::removeChild(std::shared_ptr<Node> child) {
        auto it = std::find(m_children.begin(), m_children.end(), child);
        if (it != m_children.end()) {
            (*it)->setParent(nullptr);
            m_children.erase(it);
        }
    }

    void Node::removeFromParent() {
        if (m_parent) {
            m_parent = nullptr;
        }
    }

    void Node::sortAllChildren() {
        std::stable_sort(m_children.begin(), m_children.end(), 
            [](const std::shared_ptr<Node>& a, const std::shared_ptr<Node>& b) {
                return a->m_zOrder < b->m_zOrder;
            });
    }

    void Node::runAction(std::shared_ptr<Action> action) {
        if (!action) return;
        action->startWithTarget(this);
        m_actions.push_back(action);
    }

    void Node::stopAction(std::shared_ptr<Action> action) {
        auto it = std::find(m_actions.begin(), m_actions.end(), action);
        if (it != m_actions.end()) {
            (*it)->stop();
            m_actions.erase(it);
        }
    }

    void Node::stopAllActions() {
        for (auto& action : m_actions) {
            if (action) action->stop();
        }
        m_actions.clear();
    }

    void Node::update(float dt) {
        auto actionsCopy = m_actions;
        for (auto& action : actionsCopy) {
            if (!action) continue;
            if (!action->isDone()) {
                action->step(dt);
            }
        }

        m_actions.erase(
            std::remove_if(m_actions.begin(), m_actions.end(),
                [](const std::shared_ptr<Action>& a) { return !a || a->isDone(); }),
            m_actions.end());

        auto childrenCopy = m_children;
        for (auto& child : childrenCopy) {
            if (child && child->m_visible) {
                child->update(dt);
            }
        }
    }

    void Node::draw(float parentX, float parentY, float parentScaleX, float parentScaleY, float parentRotation) {}

    void Node::visit(float parentX, float parentY, float parentScaleX, float parentScaleY, float parentRotation, float parentOpacity) {
        if (!m_visible) return;

        float absScaleX = m_scaleX * parentScaleX;
        float absScaleY = m_scaleY * parentScaleY;
        float absRot = m_rotation + parentRotation;
        float absOpacity = m_opacity * parentOpacity;

        float rad = parentRotation * (M_PI / 180.0f);
        float cosA = cos(rad);
        float sinA = sin(rad);
        
        float localX = m_x * parentScaleX;
        float localY = m_y * parentScaleY;

        float absX = parentX + (localX * cosA - localY * sinA);
        float absY = parentY + (localX * sinA + localY * cosA);

        draw(absX, absY, absScaleX, absScaleY, absRot);

        for (auto& child : m_children) {
            child->visit(absX, absY, absScaleX, absScaleY, absRot, absOpacity);
        }
    }

    Sprite::Sprite() {
        m_image.tex = nullptr;
        m_image.subtex = nullptr;
    }

    Sprite::~Sprite() {
        if (m_ownsTexture && m_image.tex) {
            C3D_TexDelete(m_image.tex);
            delete m_image.tex;
            if (m_image.subtex) {
                delete m_image.subtex;
            }
        }
    }

    std::shared_ptr<Sprite> Sprite::create(const std::string& path) {
        auto sprite = std::make_shared<Sprite>();
        if (sprite->initWithFile(path)) {
            return sprite;
        }
        return nullptr;
    }

    bool Sprite::initWithFile(const std::string& path) {
        std::vector<uint8_t> fileData = Filesystem::getInstance().readFileBinaryRomfs(path.c_str());
        if (fileData.empty()) {
            FILE* f = std::fopen(path.c_str(), "rb");
            if (!f) {
                // Try standard romfs prefix mapping if absolute sdmc/romfs fails
                std::string altPath = path;
                if (altPath.rfind("romfs:/", 0) == 0) {
                    altPath = "romfs:/" + altPath.substr(7);
                }
                f = std::fopen(altPath.c_str(), "rb");
            }
            if (f) {
                std::fseek(f, 0, SEEK_END);
                long length = std::ftell(f);
                std::fseek(f, 0, SEEK_SET);
                if (length > 0) {
                    fileData.resize(length);
                    std::fread(fileData.data(), 1, length, f);
                }
                std::fclose(f);
            }
        }

        if (fileData.empty()) {
            std::printf("[Sprite] CRITICAL: File truly not found: %s\n", path.c_str());
            return false;
        }

        int width, height, channels;
        stbi_set_flip_vertically_on_load(false);
        unsigned char* imgData = stbi_load_from_memory(
            fileData.data(), (int)fileData.size(), &width, &height, &channels, 4
        );

        if (!imgData) {
            std::printf("[Sprite] CRITICAL: stbi failed: %s\n", stbi_failure_reason());
            return false;
        }

        auto nextPow2 = [](int v) {
            v--;
            v |= v >> 1;
            v |= v >> 2;
            v |= v >> 4;
            v |= v >> 8;
            v |= v >> 16;
            v++;
            return v < 8 ? 8 : v;
        };

        int texWidth = nextPow2(width);
        int texHeight = nextPow2(height);

        std::vector<uint8_t> finalData(texWidth * texHeight * 4, 0);

        auto getMortonOffset = [](int x, int y, int texW) -> int {
            int tileX = x / 8;
            int tileY = y / 8;
            int tilesPerRow = texW / 8;
            int tileIndex = (tileY * tilesPerRow) + tileX;

            int localX = x % 8;
            int localY = y % 8;

            int pixelInTile = (localX & 1)       | ((localY & 1) << 1) |
                            ((localX & 2) << 1) | ((localY & 2) << 2) |
                            ((localX & 4) << 2) | ((localY & 4) << 3);

            return ((tileIndex * 64) + pixelInTile) * 4;
        };
        
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int srcIdx = (y * width + x) * 4;
                int dstIdx = getMortonOffset(x, y, texWidth);
                
                finalData[dstIdx + 0] = imgData[srcIdx + 3];
                finalData[dstIdx + 1] = imgData[srcIdx + 2];
                finalData[dstIdx + 2] = imgData[srcIdx + 1];
                finalData[dstIdx + 3] = imgData[srcIdx + 0];
            }
        }

        m_pixelData = finalData;

        C3D_Tex* tex = new C3D_Tex();
        if (!C3D_TexInit(tex, texWidth, texHeight, GPU_RGBA8)) {
            stbi_image_free(imgData);
            delete tex;
            return false;
        }

        C3D_TexUpload(tex, finalData.data());
        stbi_image_free(imgData);
        
        C3D_TexSetFilter(tex, GPU_LINEAR, GPU_LINEAR);

        m_image.tex = tex;
        
        float right = (float)width / (float)texWidth;
        float bottom = 1.0f - ((float)height / (float)texHeight);
        
        m_image.subtex = new Tex3DS_SubTexture{(u16)width, (u16)height, 0.0f, 1.0f, right, bottom};
        m_ownsTexture = true;

        setContentSize((float)width, (float)height);

        return true;
    }

    void Sprite::setTint(u32 color) {
        C2D_PlainImageTint(&m_tint, color, 1.0f);
        m_hasTint = true;
    }

    void Sprite::clearTint() {
        m_hasTint = false;
    }

    void Sprite::setGreenChannelOnly(bool enabled) {
        if (!m_image.tex || m_pixelData.empty()) return;

        std::vector<uint8_t> pixelData = m_pixelData;
        if (enabled) {
            for (size_t i = 0; i < pixelData.size(); i += 4) {
                pixelData[i + 1] = 0;
                pixelData[i + 3] = 0;
            }
        }

        C3D_TexUpload(m_image.tex, pixelData.data());
    }

    void Sprite::draw(float parentX, float parentY, float parentScaleX, float parentScaleY, float parentRotation) {
        if (!m_image.tex) return;

        C2D_DrawParams params;
        params.pos.x = parentX;
        params.pos.y = parentY;
        params.pos.w = m_contentWidth * parentScaleX;
        params.pos.h = m_contentHeight * parentScaleY;
        params.center.x = m_anchorX * params.pos.w;
        params.center.y = m_anchorY * params.pos.h;
        params.angle = parentRotation * (M_PI / 180.0f);
        params.depth = 0.5f;

        C2D_ImageTint* tintPtr = nullptr;
        C2D_ImageTint drawTint;

        if (m_hasTint || m_opacity < 1.0f) {
            if (m_hasTint) {
                drawTint = m_tint;
            } else {
                C2D_PlainImageTint(&drawTint, C2D_Color32(255, 255, 255, 255), m_opacity);
            }
            tintPtr = &drawTint;
        }

        C2D_DrawImage(m_image, &params, tintPtr);
    }

    std::shared_ptr<NineSliceSprite> NineSliceSprite::create(const std::string& path, float top, float bottom, float left, float right) {
        auto sprite = std::make_shared<NineSliceSprite>();
        if (sprite->initWithFile(path)) {
            sprite->setInsets(top, bottom, left, right);
            return sprite;
        }
        return nullptr;
    }

    void NineSliceSprite::draw(float parentX, float parentY, float parentScaleX, float parentScaleY, float parentRotation) {
        if (!m_image.tex || !m_image.subtex) return;

        float imgW = m_image.subtex->width;
        float imgH = m_image.subtex->height;
        if (imgW <= 0 || imgH <= 0) return;

        float L = std::min(m_insetLeft, imgW);
        float R = std::min(m_insetRight, imgW - L);
        float T = std::min(m_insetTop, imgH);
        float B = std::min(m_insetBottom, imgH - T);

        float W = m_contentWidth;
        float H = m_contentHeight;

        float DL = L * m_borderScale;
        float DR = R * m_borderScale;
        float DT = T * m_borderScale;
        float DB = B * m_borderScale;

        if (DL + DR > W && (DL + DR) > 0.0f) {
            float scale = W / (DL + DR);
            DL *= scale;
            DR *= scale;
        }
        if (DT + DB > H && (DT + DB) > 0.0f) {
            float scale = H / (DT + DB);
            DT *= scale;
            DB *= scale;
        }

        float DW = W - DL - DR;
        float DH = H - DT - DB;
        if (DW < 0) DW = 0;
        if (DH < 0) DH = 0;

        float srcCols[3][2] = {
            {0.0f, L},
            {L, imgW - L - R},
            {imgW - R, R}
        };
        float srcRows[3][2] = {
            {0.0f, T},
            {T, imgH - T - B},
            {imgH - B, B}
        };

        float destCols[3][2] = {
            {0.0f, DL},
            {DL, DW},
            {W - DR, DR}
        };
        float destRows[3][2] = {
            {0.0f, DT},
            {DT, DH},
            {H - DB, DB}
        };

        float baseLeft = m_image.subtex->left;
        float baseTop = m_image.subtex->top;
        float baseRight = m_image.subtex->right;
        float baseBottom = m_image.subtex->bottom;

        C2D_ImageTint* tintPtr = nullptr;
        C2D_ImageTint drawTint;

        if (m_hasTint || m_opacity < 1.0f) {
            if (m_hasTint) {
                drawTint = m_tint;
            } else {
                C2D_PlainImageTint(&drawTint, C2D_Color32(255, 255, 255, 255), m_opacity);
            }
            tintPtr = &drawTint;
        }

        float rad = parentRotation * (M_PI / 180.0f);
        float cosA = cos(rad);
        float sinA = sin(rad);

        float anchorOffsetX = m_anchorX * W;
        float anchorOffsetY = m_anchorY * H;

        for (int r = 0; r < 3; ++r) {
            float srcY = srcRows[r][0];
            float srcH = srcRows[r][1];
            float destY = destRows[r][0];
            float destH = destRows[r][1];

            if (srcH <= 0 || destH <= 0) continue;

            for (int c = 0; c < 3; ++c) {
                float srcX = srcCols[c][0];
                float srcW = srcCols[c][1];
                float destX = destCols[c][0];
                float destW = destCols[c][1];

                if (srcW <= 0 || destW <= 0) continue;

                Tex3DS_SubTexture subtex;
                subtex.width = (u16)srcW;
                subtex.height = (u16)srcH;
                subtex.left = baseLeft + (srcX / imgW) * (baseRight - baseLeft);
                subtex.top = baseTop + (srcY / imgH) * (baseBottom - baseTop);
                subtex.right = baseLeft + ((srcX + srcW) / imgW) * (baseRight - baseLeft);
                subtex.bottom = baseTop + ((srcY + srcH) / imgH) * (baseBottom - baseTop);

                C2D_Image tileImg = { m_image.tex, &subtex };

                float localX = destX - anchorOffsetX;
                float localY = destY - anchorOffsetY;

                float scaledX = localX * parentScaleX;
                float scaledY = localY * parentScaleY;

                float tileAbsX = parentX + (scaledX * cosA - scaledY * sinA);
                float tileAbsY = parentY + (scaledX * sinA + scaledY * cosA);

                C2D_DrawParams params;
                params.pos.x = tileAbsX;
                params.pos.y = tileAbsY;
                params.pos.w = destW * parentScaleX;
                params.pos.h = destH * parentScaleY;
                params.center.x = 0.0f;
                params.center.y = 0.0f;
                params.angle = rad;
                params.depth = 0.5f;

                C2D_DrawImage(tileImg, &params, tintPtr);
            }
        }
    }
    
    std::shared_ptr<DrawNode> DrawNode::createRect(float w, float h, u32 color) {
        auto node = std::make_shared<DrawNode>();
        node->m_shape = Rectangle;
        node->setContentSize(w, h);
        node->m_color = color;
        return node;
    }

    std::shared_ptr<DrawNode> DrawNode::createTriangle(float x1, float y1, float x2, float y2, float x3, float y3, u32 color) {
        auto node = std::make_shared<DrawNode>();
        node->m_shape = Triangle;
        node->m_x1 = x1; node->m_y1 = y1;
        node->m_x2 = x2; node->m_y2 = y2;
        node->m_x3 = x3; node->m_y3 = y3;
        node->m_color = color;
        return node;
    }

    std::shared_ptr<DrawNode> DrawNode::createQuad(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, u32 color) {
        auto node = std::make_shared<DrawNode>();
        node->m_shape = Quad;
        node->m_x1 = x1; node->m_y1 = y1;
        node->m_x2 = x2; node->m_y2 = y2;
        node->m_x3 = x3; node->m_y3 = y3;
        node->m_x4 = x4; node->m_y4 = y4;
        node->m_color = color;
        return node;
    }

    void DrawNode::draw(float parentX, float parentY, float parentScaleX, float parentScaleY, float parentRotation) {
        float offsetX = m_contentWidth * m_anchorX * parentScaleX;
        float offsetY = m_contentHeight * m_anchorY * parentScaleY;
        float startX = parentX - offsetX;
        float startY = parentY - offsetY;

        u32 r = m_color & 0xFF;
        u32 g = (m_color >> 8) & 0xFF;
        u32 b = (m_color >> 16) & 0xFF;
        u32 a = (m_color >> 24) & 0xFF;
        u32 finalColor = C2D_Color32(r, g, b, static_cast<u8>(a * m_opacity));

        if (m_shape == Rectangle) {
            C2D_DrawRectSolid(startX, startY, 0.5f, m_contentWidth * parentScaleX, m_contentHeight * parentScaleY, finalColor);
        } else if (m_shape == Triangle) {
            C2D_DrawTriangle(startX + m_x1 * parentScaleX, startY + m_y1 * parentScaleY, finalColor,
                             startX + m_x2 * parentScaleX, startY + m_y2 * parentScaleY, finalColor,
                             startX + m_x3 * parentScaleX, startY + m_y3 * parentScaleY, finalColor, 0.5f);
        } else if (m_shape == Quad) {
            C2D_DrawTriangle(startX + m_x1 * parentScaleX, startY + m_y1 * parentScaleY, finalColor,
                             startX + m_x2 * parentScaleX, startY + m_y2 * parentScaleY, finalColor,
                             startX + m_x3 * parentScaleX, startY + m_y3 * parentScaleY, finalColor, 0.5f);
            C2D_DrawTriangle(startX + m_x1 * parentScaleX, startY + m_y1 * parentScaleY, finalColor,
                             startX + m_x3 * parentScaleX, startY + m_y3 * parentScaleY, finalColor,
                             startX + m_x4 * parentScaleX, startY + m_y4 * parentScaleY, finalColor, 0.5f);
        }
    }

    Label::Label() {
        m_textBuf = C2D_TextBufNew(1024);
    }

    Label::~Label() {
        if (m_textBuf) {
            C2D_TextBufDelete(m_textBuf);
        }
        if (m_ownsFont && m_fontLoaded) {
            C2D_FontFree(m_font);
        }
    }

    std::shared_ptr<Label> Label::create(const std::string& fontPath, const std::string& text, float scale, u32 color) {
        auto label = std::make_shared<Label>();
        
        label->m_font = C2D_FontLoad(fontPath.c_str());
        if (label->m_font) {
            label->m_fontLoaded = true;
            label->m_ownsFont = true;
        } else {
            label->m_font = C2D_FontLoadSystem(CFG_REGION_USA);
            label->m_fontLoaded = true;
            label->m_ownsFont = true;
        }

        label->m_color = color;
        label->m_textScaleX = scale;
        label->m_textScaleY = scale;
        label->setString(text);

        return label;
    }

    void Label::setString(const std::string& text) {
        m_string = text;
        parseText();
    }

    void Label::parseText() {
        if (!m_fontLoaded) return;
        C2D_TextBufClear(m_textBuf);

        C2D_TextFontParse(&m_text, m_font, m_textBuf, m_string.c_str());
        C2D_TextOptimize(&m_text);

        float w = 0.0f, h = 0.0f;
        C2D_TextGetDimensions(&m_text, 1.0f, 1.0f, &w, &h);
        setContentSize(w, h);
    }

    void Label::draw(float parentX, float parentY, float parentScaleX, float parentScaleY, float parentRotation) {
        if (!m_fontLoaded || m_string.empty()) return;

        float width = m_contentWidth * m_textScaleX * parentScaleX;
        float height = m_contentHeight * m_textScaleY * parentScaleY;

        float renderX = parentX - (width * m_anchorX);
        float renderY = parentY - (height * m_anchorY);

        u32 flags = C2D_WithColor;
        
        if (m_alignment == HAlign::Center) {
            flags |= C2D_AlignCenter;
            renderX += width / 2.0f;
        } else if (m_alignment == HAlign::Right) {
            flags |= C2D_AlignRight;
            renderX += width;
        } else {
            flags |= C2D_AlignLeft;
        }

        u32 r = m_color & 0xFF;
        u32 g = (m_color >> 8) & 0xFF;
        u32 b = (m_color >> 16) & 0xFF;
        u32 a = (m_color >> 24) & 0xFF;
        u32 finalColor = C2D_Color32(r, g, b, static_cast<u8>(a * m_opacity));

        if (m_wrapWidth > 0.0f) {
            flags |= C2D_WordWrap;
            C2D_DrawText(&m_text, flags, renderX, renderY, 0.5f, 
                         m_textScaleX * parentScaleX, m_textScaleY * parentScaleY, finalColor, m_wrapWidth / parentScaleX);
        } else {
            C2D_DrawText(&m_text, flags, renderX, renderY, 0.5f, 
                         m_textScaleX * parentScaleX, m_textScaleY * parentScaleY, finalColor);
        }
    }

    void Layout::updateLayout() {
        float currentOffset = m_padding;
        for (auto& child : m_children) {
            if (!child->isVisible()) continue;
            if (m_type == Vertical) {
                child->setPosition(m_padding, currentOffset);
                currentOffset += child->getHeight() + m_spacing;
            } else {
                child->setPosition(currentOffset, m_padding);
                currentOffset += child->getWidth() + m_spacing;
            }
        }
    }

    void Layout::addChildAndLayout(std::shared_ptr<Node> child) {
        addChild(child);
        updateLayout();
    }

    Button::Button(std::shared_ptr<Node> contentNode, u32 normalColor, u32 pressedColor, std::function<void()> onClick)
        : m_onClick(onClick), m_normalColor(normalColor), m_pressedColor(pressedColor) {

        float w = contentNode ? contentNode->getWidth() : 0.0f;
        float h = contentNode ? contentNode->getHeight() : 0.0f;

        setContentSize(w, h);

        if (normalColor != 0) {
            m_bgRect = DrawNode::createRect(w, h, normalColor);
            addChild(m_bgRect);
        }

        if (contentNode) {
            m_contentNode = contentNode;
            m_contentNode->setAnchorPoint(0.5f, 0.5f);
            m_contentNode->setPosition(w * 0.5f, h * 0.5f);
            addChild(m_contentNode);
        }
    }

    Button::Button(const std::string& spritePath, std::function<void()> onClick)
        : m_onClick(onClick) {
        auto sprite = Sprite::create(spritePath);
        if (sprite) {
            float w = sprite->getWidth();
            float h = sprite->getHeight();
            setContentSize(w, h);
            sprite->setAnchorPoint(0.0f, 0.0f);
            sprite->setPosition(0.0f, 0.0f);
            m_contentNode = sprite;
            addChild(m_contentNode);
        }
    }

    std::shared_ptr<Button> Button::create(std::shared_ptr<Node> contentNode, u32 normalColor, u32 pressedColor, std::function<void()> onClick) {
        return std::make_shared<Button>(contentNode, normalColor, pressedColor, onClick);
    }

    std::shared_ptr<Button> Button::create(const std::string& spritePath, std::function<void()> onClick) {
        return std::make_shared<Button>(spritePath, onClick);
    }

    void Button::onClickAction() {
        if (!m_enabled) return;
        if (!m_clickSoundPath.empty()) {
            engine::AudioEngine::getInstance().playSFX(m_clickSoundPath);
        }
        if (m_onClick) m_onClick();
    }

    void Button::setFocused(bool focused) {
        if (m_focused == focused) return;
        m_focused = focused;

        if (m_contentNode) {
            if (m_focused) {
                m_contentNode->setGreenChannelOnly(true);
            } else {
                m_contentNode->setGreenChannelOnly(false);
            }
        }
    }

    void Button::update(float dt) {
        Node::update(dt);
        if (!m_enabled) return;

        auto& input = engine::Input::getInstance();
        if (input.isTouched()) {
            touchPosition touch = input.getTouch();
            
            float absX = m_x;
            float absY = m_y;
            Node* p = m_parent;
            while (p) {
                absX += p->getX();
                absY += p->getY();
                p = p->getParent();
            }

            float width = m_contentWidth * m_scaleX;
            float height = m_contentHeight * m_scaleY;
            float startX = absX - (width * m_anchorX);
            float startY = absY - (height * m_anchorY);

            if (touch.px >= startX && touch.px <= startX + width && touch.py >= startY && touch.py <= startY + height) {
                if (!m_isPressed) {
                    m_isPressed = true;
                    onClickAction();
                }
            }
        } else {
            if (m_isPressed) {
                m_isPressed = false;
            }
        }
    }

    void Menu::update(float dt) {
        Node::update(dt);
        if (!m_navigationEnabled || m_buttons.empty()) return;

        if (m_navCooldown > 0.0f) {
            m_navCooldown -= dt;
        }

        auto& input = Input::getInstance();

        if (input.isTouched()) {
            touchPosition touch = input.getTouch();
            for (size_t i = 0; i < m_buttons.size(); ++i) {
                auto& btn = m_buttons[i];
                if (!btn || !btn->isEnabled()) continue;

                float absX = btn->getX();
                float absY = btn->getY();
                Node* p = btn->getParent();
                while (p) {
                    absX += p->getX();
                    absY += p->getY();
                    p = p->getParent();
                }

                float width = btn->getWidth() * btn->getScaleX();
                float height = btn->getHeight() * btn->getScaleY();
                float startX = absX - (width * btn->getAnchorX());
                float startY = absY - (height * btn->getAnchorY());

                if (touch.px >= startX && touch.px <= startX + width &&
                    touch.py >= startY && touch.py <= startY + height) {
                    if (m_selectedIndex != (int)i) {
                        setSelectedIndex((int)i, false);
                    }
                    break;
                }
            }
        }

        int dirX = 0;
        int dirY = 0;

        if (m_navCooldown <= 0.0f) {
            auto cp = input.getCirclePad();
            if (input.isDpadLeft() || cp.dx < -50) {
                dirX = -1;
                m_navCooldown = 0.2f;
            } else if (input.isDpadRight() || cp.dx > 50) {
                dirX = 1;
                m_navCooldown = 0.2f;
            } else if (input.isDpadUp() || cp.dy > 50) {
                dirY = -1;
                m_navCooldown = 0.2f;
            } else if (input.isDpadDown() || cp.dy < -50) {
                dirY = 1;
                m_navCooldown = 0.2f;
            }

            if (dirX != 0 || dirY != 0) {
                if (m_selectedIndex < 0 || m_selectedIndex >= (int)m_buttons.size()) {
                    setSelectedIndex(0, false);
                } else {
                    auto currentBtn = m_buttons[m_selectedIndex];
                    float currX = currentBtn->getX();
                    float currY = currentBtn->getY();

                    int bestIdx = m_selectedIndex;
                    float bestScore = 1e9f;

                    for (size_t i = 0; i < m_buttons.size(); ++i) {
                        if ((int)i == m_selectedIndex) continue;
                        auto& b = m_buttons[i];
                        if (!b || !b->isEnabled()) continue;

                        float bx = b->getX();
                        float by = b->getY();

                        float dx = bx - currX;
                        float dy = by - currY;

                        bool inDir = false;
                        if (dirX != 0) {
                            if ((dirX > 0 && dx > 2.0f) || (dirX < 0 && dx < -2.0f)) {
                                if (std::abs(dy) <= std::abs(dx) * 2.5f) {
                                    inDir = true;
                                }
                            }
                        }
                        if (dirY != 0) {
                            if ((dirY > 0 && dy > 2.0f) || (dirY < 0 && dy < -2.0f)) {
                                if (std::abs(dx) <= std::abs(dy) * 2.5f) {
                                    inDir = true;
                                }
                            }
                        }

                        if (inDir) {
                            float dist = dx * dx + dy * dy;
                            if (dist < bestScore) {
                                bestScore = dist;
                                bestIdx = (int)i;
                            }
                        }
                    }

                    if (bestIdx != m_selectedIndex) {
                        setSelectedIndex(bestIdx, false);
                    }
                }
            }
        }

        if (input.isAKeyDown() && m_selectedIndex >= 0 && m_selectedIndex < (int)m_buttons.size()) {
            auto btn = m_buttons[m_selectedIndex];
            if (btn && btn->isEnabled()) {
                btn->setPressed(true);
                btn->onClickAction();
            }
        }
    }

    std::shared_ptr<Scene> Scene::create() {
        return std::make_shared<Scene>();
    }

    Director::Director() {
        m_topClearColor = C2D_Color32(15, 15, 20, 255);
        m_bottomClearColor = C2D_Color32(30, 30, 35, 255);
    }

    Director::~Director() {
        for (auto& pair : m_spriteSheets) {
            C2D_SpriteSheetFree(pair.second);
        }
        m_spriteSheets.clear();
    }

    void Director::setClearColor(u8 r, u8 g, u8 b, u8 a) {
        m_topClearColor = C2D_Color32(r, g, b, a);
        m_bottomClearColor = C2D_Color32(r, g, b, a);
    }

    void Director::setClearColorTop(u8 r, u8 g, u8 b, u8 a) {
        m_topClearColor = C2D_Color32(r, g, b, a);
    }

    void Director::setClearColorBottom(u8 r, u8 g, u8 b, u8 a) {
        m_bottomClearColor = C2D_Color32(r, g, b, a);
    }

    void Director::setTopScene(std::shared_ptr<Scene> scene) {
        m_pendingTopScene = scene;
        m_hasPendingTop = true;
    }

    void Director::setBottomScene(std::shared_ptr<Scene> scene) {
        m_pendingBottomScene = scene;
        m_hasPendingBottom = true;
    }

    void Director::runWithScenes(std::shared_ptr<Scene> topScene, std::shared_ptr<Scene> bottomScene) {
        m_topScene = topScene;
        m_bottomScene = bottomScene;
    }

    void Director::flushPendingScenes() {
        if (m_hasPendingTop) {
            m_topScene = std::move(m_pendingTopScene);
            m_hasPendingTop = false;
        }
        if (m_hasPendingBottom) {
            m_bottomScene = std::move(m_pendingBottomScene);
            m_hasPendingBottom = false;
        }
    }

    C2D_SpriteSheet Director::loadSpriteSheet(const std::string& key, const std::string& path) {
        if (m_spriteSheets.find(key) != m_spriteSheets.end()) {
            return m_spriteSheets[key];
        }
        C2D_SpriteSheet sheet = C2D_SpriteSheetLoad(path.c_str());
        if (sheet) {
            m_spriteSheets[key] = sheet;
        }
        return sheet;
    }

    void Director::unloadSpriteSheet(const std::string& key) {
        auto it = m_spriteSheets.find(key);
        if (it != m_spriteSheets.end()) {
            C2D_SpriteSheetFree(it->second);
            m_spriteSheets.erase(it);
        }
    }

    Size Director::getTopScreenSize() const {
        return Size(400.0f, 240.0f);
    }

    Size Director::getBottomScreenSize() const {
        return Size(320.0f, 240.0f);
    }

    void Director::mainLoop() {
        C3D_RenderTarget* topTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
        C3D_RenderTarget* bottomTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

        u64 lastTick = osGetTime(); // milliseconds since boot

        while (aptMainLoop()) {
            gspWaitForVBlank();

            u64 nowTick = osGetTime();
            float dt = static_cast<float>(nowTick - lastTick) / 1000.0f;
            lastTick = nowTick;

            // Clamp dt to avoid a spiral-of-death on slow frames
            if (dt > 0.1f) dt = 0.1f;
            if (dt <= 0.0f) dt = 1.0f / 60.0f;

            engine::Input::getInstance().poll();

            if (engine::Input::getInstance().isKeyDown(KEY_START)) break;

            engine::AudioEngine::getInstance().update();

            if (m_topScene)    m_topScene->update(dt);
            if (m_bottomScene) m_bottomScene->update(dt);

            flushPendingScenes();

            C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

            C2D_TargetClear(topTarget, m_topClearColor);
            C2D_SceneBegin(topTarget);
            C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_COLOR);
            if (m_topScene) {
                m_topScene->visit(0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f);
            }

            C2D_TargetClear(bottomTarget, m_bottomClearColor);
            C2D_SceneBegin(bottomTarget);
            C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_COLOR);
            if (m_bottomScene) {
                m_bottomScene->visit(0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f);
            }

            C3D_FrameEnd(0);
        }
    }
}