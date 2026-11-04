#pragma once

#include <includes.hpp>
#include <engine/math.hpp>

/**
 * @brief Main engine namespace.
 */
namespace engine {

    class Action;

    /**
     * @brief Specifies target hardware display screens on the 3DS.
     */
    enum class TargetScreen {
        Top,    /**< Top stereoscopic 3DS screen. */
        Bottom  /**< Bottom touchscreen 3DS display. */
    };

    /**
     * @brief Base class for all visual elements, render nodes, containers, and scenes in the engine.
     */
    class Node {
    protected:
        float m_x = 0.0f;               /**< Local X coordinate relative to parent. */
        float m_y = 0.0f;               /**< Local Y coordinate relative to parent. */
        float m_scaleX = 1.0f;          /**< Local horizontal scaling factor. */
        float m_scaleY = 1.0f;          /**< Local vertical scaling factor. */
        float m_rotation = 0.0f;        /**< Local rotation angle in degrees. */
        float m_anchorX = 0.0f;         /**< Anchor point X offset normalized [0, 1]. */
        float m_anchorY = 0.0f;         /**< Anchor point Y offset normalized [0, 1]. */
        float m_zOrder = 0.0f;          /**< Z-index ordering value for rendering and sorting. */
        float m_contentWidth = 0.0f;    /**< Bounding box width in pixels. */
        float m_contentHeight = 0.0f;   /**< Bounding box height in pixels. */
        float m_opacity = 1.0f;         /**< Local opacity value [0, 1]. */
        bool m_visible = true;          /**< Visibility flag. */

        Node* m_parent = nullptr;                       /**< Pointer to parent node container. */
        std::vector<std::shared_ptr<Node>> m_children;  /**< List of child nodes. */
        std::vector<std::shared_ptr<Action>> m_actions; /**< List of currently executing actions. */

    public:
        /** @brief Default constructor. */
        Node() = default;
        
        /** @brief Virtual destructor. */
        virtual ~Node() = default;

        /**
         * @brief Factory method to create a shared pointer instance of Node.
         * @return Shared pointer to the created Node.
         */
        static std::shared_ptr<Node> create();

        /**
         * @brief Updates node state and running actions per frame.
         * @param dt Delta time elapsed since last frame in seconds.
         */
        virtual void update(float dt);

        /**
         * @brief Renders the node graphics.
         * @param parentX Accumulated parent world X coordinate.
         * @param parentY Accumulated parent world Y coordinate.
         * @param parentScaleX Accumulated parent horizontal scale.
         * @param parentScaleY Accumulated parent vertical scale.
         * @param parentRotation Accumulated parent rotation angle.
         */
        virtual void draw(float parentX, float parentY, float parentScaleX, float parentScaleY, float parentRotation);

        /**
         * @brief Visits and processes node transformation tree before drawing children.
         * @param parentX Accumulated parent world X coordinate.
         * @param parentY Accumulated parent world Y coordinate.
         * @param parentScaleX Accumulated parent horizontal scale.
         * @param parentScaleY Accumulated parent vertical scale.
         * @param parentRotation Accumulated parent rotation angle.
         * @param parentOpacity Accumulated parent transparency factor.
         */
        virtual void visit(float parentX, float parentY, float parentScaleX, float parentScaleY, float parentRotation, float parentOpacity = 1.0f);

        /**
         * @brief Adds a child node to this container.
         * @param child Shared pointer to the child Node.
         */
        void addChild(std::shared_ptr<Node> child);

        /**
         * @brief Removes a child node from this container.
         * @param child Shared pointer to the child Node.
         */
        void removeChild(std::shared_ptr<Node> child);

        /** @brief Removes this node from its parent container. */
        void removeFromParent();

        /** @brief Sorts child nodes based on their z-order values. */
        void sortAllChildren();

        /**
         * @brief Attaches and runs an action on this node.
         * @param action Shared pointer to the Action.
         */
        void runAction(std::shared_ptr<Action> action);

        /**
         * @brief Stops and removes a specific action from this node.
         * @param action Shared pointer to the Action.
         */
        void stopAction(std::shared_ptr<Action> action);

        /** @brief Stops and removes all currently running actions on this node. */
        void stopAllActions();

        /** @brief Sets local 2D position. @param x X coordinate. @param y Y coordinate. */
        void setPosition(float x, float y) { m_x = x; m_y = y; }
        
        /** @brief Sets uniform local scale. @param scale Uniform scale factor. */
        void setScale(float scale) { m_scaleX = scale; m_scaleY = scale; }
        
        /** @brief Sets non-uniform local scale. @param x Horizontal scale. @param y Vertical scale. */
        void setScale(float x, float y) { m_scaleX = x; m_scaleY = y; }
        
        /** @brief Sets rotation angle. @param angle Angle in degrees. */
        void setRotation(float angle) { m_rotation = angle; }
        
        /** @brief Sets local anchor point. @param x Normalized X [0, 1]. @param y Normalized Y [0, 1]. */
        void setAnchorPoint(float x, float y) { m_anchorX = x; m_anchorY = y; }
        
        /** @brief Sets z-order rendering priority and resorts children. @param z Z-order index. */
        void setZOrder(float z) { m_zOrder = z; sortAllChildren(); }
        
        /** @brief Sets visibility flag. @param visible True to show, false to hide. */
        void setVisible(bool visible) { m_visible = visible; }
        
        /** @brief Toggles current node visibility state. */
        void toggleVisibility() { m_visible = !m_visible; }
        
        /** @brief Sets bounding box content size. @param w Width in pixels. @param h Height in pixels. */
        void setContentSize(float w, float h) { m_contentWidth = w; m_contentHeight = h; }
        
        /** @brief Sets local opacity. @param opacity Value clamped between 0.0 and 1.0. */
        void setOpacity(float opacity) { m_opacity = std::clamp(opacity, 0.0f, 1.0f); }
        
        /** @brief Gets local opacity value. @return Opacity factor [0, 1]. */
        float getOpacity() const { return m_opacity; }

        /** @brief Gets local X coordinate. @return X position. */
        float getX() const { return m_x; }
        
        /** @brief Gets local Y coordinate. @return Y position. */
        float getY() const { return m_y; }
        
        /** @brief Gets horizontal scale factor. @return Scale X. */
        float getScaleX() const { return m_scaleX; }
        
        /** @brief Gets vertical scale factor. @return Scale Y. */
        float getScaleY() const { return m_scaleY; }
        
        /** @brief Gets rotation angle. @return Angle in degrees. */
        float getRotation() const { return m_rotation; }
        
        /** @brief Checks if node is visible. @return True if visible. */
        bool isVisible() const { return m_visible; }
        
        /** @brief Gets pointer to parent node. @return Parent Node pointer. */
        Node* getParent() const { return m_parent; }
        
        /** @brief Sets pointer to parent node. @param parent Parent Node pointer. */
        void setParent(Node* parent) { m_parent = parent; }
        
        /** @brief Gets node content width. @return Width in pixels. */
        virtual float getWidth() const { return m_contentWidth; }
        
        /** @brief Gets node content height. @return Height in pixels. */
        virtual float getHeight() const { return m_contentHeight; }

        /** @brief Gets anchor X. @return Anchor X. */
        float getAnchorX() const { return m_anchorX; }

        /** @brief Gets anchor Y. @return Anchor Y. */
        float getAnchorY() const { return m_anchorY; }

        /** @brief Sets tint color. */
        virtual void setTint(u32 color) {}

        /** @brief Clears tint color. */
        virtual void clearTint() {}

        /** @brief Enables or disables a green-channel-only display mode. */
        virtual void setGreenChannelOnly(bool enabled) {}
    };

    /**
     * @brief Renders 2D textures and image sprites on screen.
     */
    class Sprite : public Node {
    protected:
        C2D_Image m_image;              /**< Underlying Citro2D image structure. */
        C2D_ImageTint m_tint;           /**< Color tint filter applied to sprite. */
        bool m_hasTint = false;         /**< Flag indicating if a tint is active. */
        bool m_ownsTexture = false;     /**< Flag indicating if texture memory is owned. */
        std::vector<uint8_t> m_pixelData;/**< Raw decoded pixel byte buffer. */

    public:
        /** @brief Default constructor. */
        Sprite();
        
        /** @brief Virtual destructor. */
        virtual ~Sprite();

        /**
         * @brief Factory method to create a Sprite from an image file path.
         * @param path Asset file path string.
         * @return Shared pointer to the created Sprite.
         */
        static std::shared_ptr<Sprite> create(const std::string& path);

        /**
         * @brief Initializes sprite texture data from a file path.
         * @param path Asset file path string.
         * @return True on success, false otherwise.
         */
        bool initWithFile(const std::string& path);
        
        /** @brief Sets a color tint overlay on the sprite. @param color 32-bit RGBA color integer. */
        void setTint(u32 color) override;
        
        /** @brief Clears any active color tint on the sprite. */
        void clearTint() override;

        /** @brief Shows only each pixel's original green channel. */
        void setGreenChannelOnly(bool enabled) override;

        void draw(float parentX, float parentY, float parentScaleX, float parentScaleY, float parentRotation) override;
    };

    /**
     * @brief Renders 9-slice scalable image sprites.
     */
    class NineSliceSprite : public Sprite {
    protected:
        float m_insetTop = 0.0f;
        float m_insetBottom = 0.0f;
        float m_insetLeft = 0.0f;
        float m_insetRight = 0.0f;
        float m_borderScale = 1.0f;

    public:
        NineSliceSprite() = default;
        virtual ~NineSliceSprite() = default;

        static std::shared_ptr<NineSliceSprite> create(const std::string& path, float top, float bottom, float left, float right);

        void setBorderScale(float scale) { m_borderScale = scale; }
        float getBorderScale() const { return m_borderScale; }

        void setInsets(float top, float bottom, float left, float right) {
            m_insetTop = top;
            m_insetBottom = bottom;
            m_insetLeft = left;
            m_insetRight = right;
        }

        void draw(float parentX, float parentY, float parentScaleX, float parentScaleY, float parentRotation) override;
    };

    /**
     * @brief Procedural primitive shape drawing node (rectangles, triangles, quads).
     */
    class DrawNode : public Node {
    public:
        /** @brief Supported geometric shape types. */
        enum ShapeType { Rectangle, Triangle, Quad };

    private:
        ShapeType m_shape;                      /**< Active geometric shape type. */
        u32 m_color = C2D_Color32(255, 255, 255, 255); /**< 32-bit RGBA drawing fill color. */
        float m_x1 = 0, m_y1 = 0, m_x2 = 0, m_y2 = 0, m_x3 = 0, m_y3 = 0, m_x4 = 0, m_y4 = 0; /**< Vertex coordinate data. */

    public:
        /** @brief Default constructor. */
        DrawNode() = default;
        
        /** @brief Virtual destructor. */
        virtual ~DrawNode() = default;

        /**
         * @brief Factory method to create a filled rectangle shape.
         * @param w Width in pixels.
         * @param h Height in pixels.
         * @param color 32-bit RGBA color.
         * @return Shared pointer to the created DrawNode.
         */
        static std::shared_ptr<DrawNode> createRect(float w, float h, u32 color);

        /**
         * @brief Factory method to create a filled triangle shape.
         * @param x1 Vertex 1 X. @param y1 Vertex 1 Y.
         * @param x2 Vertex 2 X. @param y2 Vertex 2 Y.
         * @param x3 Vertex 3 X. @param y3 Vertex 3 Y.
         * @param color 32-bit RGBA color.
         * @return Shared pointer to the created DrawNode.
         */
        static std::shared_ptr<DrawNode> createTriangle(float x1, float y1, float x2, float y2, float x3, float y3, u32 color);

        /**
         * @brief Factory method to create a filled quad shape.
         * @param x1 Vertex 1 X. @param y1 Vertex 1 Y.
         * @param x2 Vertex 2 X. @param y2 Vertex 2 Y.
         * @param x3 Vertex 3 X. @param y3 Vertex 3 Y.
         * @param x4 Vertex 4 X. @param y4 Vertex 4 Y.
         * @param color 32-bit RGBA color.
         * @return Shared pointer to the created DrawNode.
         */
        static std::shared_ptr<DrawNode> createQuad(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, u32 color);

        /** @brief Sets fill color. @param color 32-bit RGBA color. */
        void setColor(u32 color) { m_color = color; }
        
        /** @brief Gets fill color. @return 32-bit RGBA color. */
        u32 getColor() const { return m_color; }
        
        /** @brief Sets shape dimensions. @param w Width in pixels. @param h Height in pixels. */
        void setSize(float w, float h) { setContentSize(w, h); }

        void draw(float parentX, float parentY, float parentScaleX, float parentScaleY, float parentRotation) override;
    };

    /**
     * @brief Renders text strings and paragraph labels using BCFNT fonts.
     */
    class Label : public Node {
    public:
        /** @brief Text horizontal alignment options. */
        enum HAlign { Left, Center, Right };

    private:
        C2D_Font m_font;            /**< Citro2D font handle reference. */
        C2D_TextBuf m_textBuf;      /**< Citro2D text buffer container. */
        C2D_Text m_text;            /**< Renderable Citro2D text object. */
        std::string m_string;       /**< Raw UTF-8 string content. */
        u32 m_color = C2D_Color32(255, 255, 255, 255); /**< Text RGBA color. */
        float m_textScaleX = 0.5f;  /**< Horizontal text font scale. */
        float m_textScaleY = 0.5f;  /**< Vertical text font scale. */
        float m_wrapWidth = 0.0f;   /**< Line wrapping width threshold. */
        HAlign m_alignment = HAlign::Left; /**< Horizontal text alignment. */
        bool m_fontLoaded = false;  /**< Font resource loaded flag. */
        bool m_ownsFont = false;    /**< Font ownership flag. */

    public:
        /** @brief Default constructor. */
        Label();
        
        /** @brief Virtual destructor. */
        virtual ~Label();

        /**
         * @brief Factory method to create a text Label.
         * @param fontPath Path to .bcfnt font asset file.
         * @param text Initial text string.
         * @param scale Text font scale factor (default: 0.5f).
         * @param color 32-bit RGBA color.
         * @return Shared pointer to the created Label.
         */
        static std::shared_ptr<Label> create(const std::string& fontPath, const std::string& text, float scale = 0.5f, u32 color = C2D_Color32(255, 255, 255, 255));
        
        /** @brief Updates the text string. @param text New UTF-8 string. */
        void setString(const std::string& text);
        
        /** @brief Sets text color. @param color 32-bit RGBA color. */
        void setColor(u32 color) { m_color = color; }
        
        /** @brief Sets font scaling. @param scale Uniform scale factor. */
        void setTextScale(float scale) { m_textScaleX = scale; m_textScaleY = scale; }
        
        /** @brief Sets text line wrap width. @param width Width limit in pixels. */
        void setWrapWidth(float width) { m_wrapWidth = width; parseText(); }
        
        /** @brief Sets text horizontal alignment. @param align HAlign alignment mode. */
        void setAlignment(HAlign align) { m_alignment = align; }

        void draw(float parentX, float parentY, float parentScaleX, float parentScaleY, float parentRotation) override;

    private:
        /** @brief Parses and buffers text layout data. */
        void parseText();
    };

    /**
     * @brief Automatic layout container managing vertical or horizontal element stacking.
     */
    class Layout : public Node {
    public:
        /** @brief Layout arrangement flow types. */
        enum Type { Vertical, Horizontal };

    private:
        Type m_type;            /**< Layout arrangement type. */
        float m_spacing = 10.0f;/**< Gap space between child elements in pixels. */
        float m_padding = 10.0f;/**< Outer container padding margin in pixels. */

    public:
        /**
         * @brief Constructs a Layout container.
         * @param type Layout Type (Vertical or Horizontal).
         */
        Layout(Type type) : m_type(type) {}
        
        /** @brief Virtual destructor. */
        virtual ~Layout() = default;

        /**
         * @brief Factory method to create a vertical stacking layout.
         * @param spacing Inter-item gap in pixels.
         * @param padding Outer border padding in pixels.
         * @return Shared pointer to the created Layout.
         */
        static std::shared_ptr<Layout> createVertical(float spacing = 10.0f, float padding = 10.0f) {
            auto l = std::make_shared<Layout>(Vertical);
            l->m_spacing = spacing;
            l->m_padding = padding;
            return l;
        }

        /**
         * @brief Factory method to create a horizontal stacking layout.
         * @param spacing Inter-item gap in pixels.
         * @param padding Outer border padding in pixels.
         * @return Shared pointer to the created Layout.
         */
        static std::shared_ptr<Layout> createHorizontal(float spacing = 10.0f, float padding = 10.0f) {
            auto l = std::make_shared<Layout>(Horizontal);
            l->m_spacing = spacing;
            l->m_padding = padding;
            return l;
        }

        /** @brief Sets item spacing. @param s Spacing in pixels. */
        void setSpacing(float s) { m_spacing = s; updateLayout(); }
        
        /** @brief Sets container padding. @param p Padding in pixels. */
        void setPadding(float p) { m_padding = p; updateLayout(); }
        
        /** @brief Recalculates and repositions all child node positions. */
        void updateLayout();

        /**
         * @brief Adds a child node and instantly recalculates layout positions.
         * @param child Shared pointer to child Node.
         */
        void addChildAndLayout(std::shared_ptr<Node> child);
    };

    /**
     * @brief Interactive UI button supporting touch interaction and callback execution.
     */
    class Button : public Node {
    private:
        std::shared_ptr<DrawNode> m_bgRect;     /**< Background visual drawing rectangle. */
        std::shared_ptr<Node> m_contentNode;    /**< Foreground content node (icon or label). */
        std::function<void()> m_onClick;        /**< Click action callback function. */
        std::string m_clickSoundPath;           /**< Audio path played on click. */
        u32 m_normalColor = 0;                  /**< Background color in normal state. */
        u32 m_pressedColor = 0;                 /**< Background color when pressed. */
        bool m_isPressed = false;               /**< Current pressed state flag. */
        bool m_enabled = true;                  /**< Enabled state flag. */
        bool m_isRound = false;                 /**< Round button flag. */
        bool m_focused = false;                 /**< Current focused/hovered state flag. */

    public:
        Button(std::shared_ptr<Node> contentNode, u32 normalColor, u32 pressedColor, std::function<void()> onClick);
        Button(const std::string& spritePath, std::function<void()> onClick);
        virtual ~Button() = default;

        static std::shared_ptr<Button> create(std::shared_ptr<Node> contentNode, u32 normalColor, u32 pressedColor, std::function<void()> onClick);
        static std::shared_ptr<Button> create(const std::string& spritePath, std::function<void()> onClick);

        void setIsRound(bool round) { m_isRound = round; }
        bool isRound() const { return m_isRound; }

        void setClickSound(const std::string& path) { m_clickSoundPath = path; }
        void setEnabled(bool enabled) { m_enabled = enabled; }
        bool isEnabled() const { return m_enabled; }

        void setPressed(bool pressed) { m_isPressed = pressed; }

        bool isFocused() const { return m_focused; }
        void setFocused(bool focused);

        void onClickAction();

        void update(float dt) override;
    };

    /**
     * @brief Interactive menu container managing buttons, navigation, and selection state.
     */
    class Menu : public Node {
    private:
        std::vector<std::shared_ptr<Button>> m_buttons;
        bool m_navigationEnabled = true;
        int m_selectedIndex = -1;
        float m_navCooldown = 0.0f;

    public:
        Menu() = default;
        virtual ~Menu() = default;

        static std::shared_ptr<Menu> create() {
            return std::make_shared<Menu>();
        }

        void addButton(std::shared_ptr<Button> btn) {
            if (btn) {
                m_buttons.push_back(btn);
                addChild(btn);
                if (m_selectedIndex < 0 && m_buttons.size() == 1) {
                    setSelectedIndex(0, false);
                }
            }
        }

        const std::vector<std::shared_ptr<Button>>& getButtons() const {
            return m_buttons;
        }

        void setNavigationEnabled(bool enabled) {
            m_navigationEnabled = enabled;
            if (!enabled && m_selectedIndex >= 0 && m_selectedIndex < (int)m_buttons.size()) {
                m_buttons[m_selectedIndex]->setFocused(false);
            }
        }

        bool isNavigationEnabled() const {
            return m_navigationEnabled;
        }

        void setSelectedIndex(int index, bool animate) {
            if (m_selectedIndex >= 0 && m_selectedIndex < (int)m_buttons.size()) {
                m_buttons[m_selectedIndex]->setFocused(false);
            }
            m_selectedIndex = index;
            if (m_selectedIndex >= 0 && m_selectedIndex < (int)m_buttons.size()) {
                m_buttons[m_selectedIndex]->setFocused(true);
            }
        }

        int getSelectedIndex() const {
            return m_selectedIndex;
        }

        void update(float dt) override;
    };

    /**
     * @brief Root scene container representing a game screen or menu state.
     */
    class Scene : public Node {
    public:
        /**
         * @brief Factory method to create a Scene.
         * @return Shared pointer to the created Scene.
         */
        static std::shared_ptr<Scene> create();
    };

    /**
     * @brief Scene transition animation styles.
     */
    enum class TransitionType {
        Fade,       /**< Fade opacity transition. */
        SlideLeft,  /**< Slide screen to the left. */
        SlideRight, /**< Slide screen to the right. */
        Zoom        /**< Zoom scale transition. */
    };

    /**
     * @brief Core engine director singleton managing dual-screen scenes, rendering loops, sprite sheets, and transitions.
     */
    class Director {
    private:
        std::shared_ptr<Scene> m_topScene;          /**< Currently active scene on top screen. */
        std::shared_ptr<Scene> m_bottomScene;       /**< Currently active scene on bottom screen. */
        std::map<std::string, C2D_SpriteSheet> m_spriteSheets; /**< Loaded sprite sheets map. */
        u32 m_topClearColor;                        /**< Top screen background clear color. */
        u32 m_bottomClearColor;                     /**< Bottom screen background clear color. */

        bool m_hasPendingTop = false;               /**< Pending top scene flag. */
        bool m_hasPendingBottom = false;            /**< Pending bottom scene flag. */
        std::shared_ptr<Scene> m_pendingTopScene;   /**< Queued top scene. */
        std::shared_ptr<Scene> m_pendingBottomScene;/**< Queued bottom scene. */

        bool m_inTransition = false;                /**< Transition active flag. */
        TransitionType m_transitionType = TransitionType::Fade; /**< Active transition style. */
        float m_transitionTimer = 0.0f;             /**< Elapsed transition time in seconds. */
        float m_transitionDuration = 0.0f;          /**< Total transition duration in seconds. */
        std::function<float(float)> m_transitionEasing = [](float t){ return t; }; /**< Transition easing curve function. */
        std::shared_ptr<Scene> m_oldTopScene;       /**< Outgoing top scene during transition. */
        std::shared_ptr<Scene> m_oldBottomScene;    /**< Outgoing bottom scene during transition. */
        std::shared_ptr<Scene> m_nextTopScene;      /**< Incoming top scene during transition. */
        std::shared_ptr<Scene> m_nextBottomScene;   /**< Incoming bottom scene during transition. */

        /** @brief Applies any queued scene switches. */
        void flushPendingScenes();

        /** @brief Private constructor for singleton pattern. */
        Director();
        
        /** @brief Destructor releasing sprite sheets and engine resources. */
        ~Director();

    public:
        /** @brief Deleted copy constructor. */
        Director(const Director&) = delete;
        
        /** @brief Deleted copy assignment operator. */
        Director& operator=(const Director&) = delete;

        /**
         * @brief Gets the singleton instance of the Director.
         * @return Reference to the Director instance.
         */
        static Director& getInstance() {
            static Director instance;
            return instance;
        }

        /** @brief Sets background clear color for both screens. @param r Red. @param g Green. @param b Blue. @param a Alpha (default: 255). */
        void setClearColor(u8 r, u8 g, u8 b, u8 a = 255);
        
        /** @brief Sets background clear color for top screen. @param r Red. @param g Green. @param b Blue. @param a Alpha (default: 255). */
        void setClearColorTop(u8 r, u8 g, u8 b, u8 a = 255);
        
        /** @brief Sets background clear color for bottom screen. @param r Red. @param g Green. @param b Blue. @param a Alpha (default: 255). */
        void setClearColorBottom(u8 r, u8 g, u8 b, u8 a = 255);

        /** @brief Sets active top scene. @param scene Shared pointer to Scene. */
        void setTopScene(std::shared_ptr<Scene> scene);
        
        /** @brief Sets active bottom scene. @param scene Shared pointer to Scene. */
        void setBottomScene(std::shared_ptr<Scene> scene);
        
        /** @brief Initializes both screens with scenes simultaneously. @param topScene Top Scene. @param bottomScene Bottom Scene. */
        void runWithScenes(std::shared_ptr<Scene> topScene, std::shared_ptr<Scene> bottomScene);
        
        /**
         * @brief Replaces current scenes with a transition effect.
         * @param newTop New top Scene.
         * @param newBottom New bottom Scene.
         * @param type TransitionType style.
         * @param duration Duration in seconds.
         * @param easing Optional easing function curve.
         */
        void replaceSceneWithTransition(std::shared_ptr<Scene> newTop, std::shared_ptr<Scene> newBottom,
                                        TransitionType type, float duration,
                                        std::function<float(float)> easing = [](float t){ return t; });

        /** @brief Gets active top scene. @return Shared pointer to top Scene. */
        std::shared_ptr<Scene> getTopScene() const { return m_topScene; }
        
        /** @brief Gets active bottom scene. @return Shared pointer to bottom Scene. */
        std::shared_ptr<Scene> getBottomScene() const { return m_bottomScene; }

        /**
         * @brief Loads and caches a Citro2D sprite sheet asset.
         * @param key Unique string identifier key.
         * @param path File path to sprite sheet file.
         * @return Loaded C2D_SpriteSheet handle.
         */
        C2D_SpriteSheet loadSpriteSheet(const std::string& key, const std::string& path);
        
        /** @brief Unloads and frees a cached sprite sheet. @param key Sprite sheet key string. */
        void unloadSpriteSheet(const std::string& key);

        /** @brief Gets top screen resolution dimensions. @return Size structure (width, height). */
        Size getTopScreenSize() const;
        
        /** @brief Gets bottom screen resolution dimensions. @return Size structure (width, height). */
        Size getBottomScreenSize() const;

        /** @brief Main engine frame execution and rendering loop. */
        void mainLoop();
    };

}