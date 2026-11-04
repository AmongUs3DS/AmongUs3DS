#pragma once

#include <includes.hpp>

/**
 * @brief Main engine namespace.
 */
namespace engine {

    /** @brief Unique identifier type for active sound instances. */
    using SoundId = uint32_t;
    
    /** @brief Constant representing an invalid or uninitialized sound ID. */
    constexpr SoundId INVALID_SOUND_ID = 0;

    /**
     * @brief Supported audio file formats.
     */
    enum class SoundFormat {
        Unknown, ///< Unrecognized or uninitialized format.
        Wav      ///< Standard Waveform Audio File Format.
    };

    /**
     * @brief Configuration parameters used when initializing or playing a sound.
     */
    struct SoundParams {
        bool loop = false;          ///< True to loop playback indefinitely, false for one-shot.
        float volume = 1.0f;        ///< Playback volume level ranging from 0.0 (silent) to 1.0 (full).
        float pan = 0.0f;           ///< Stereo balance pan ranging from -1.0 (full left) to 1.0 (full right).
        float pitch = 1.0f;         ///< Playback pitch multiplier (1.0 = normal speed/pitch).
        std::string tag;            ///< Optional grouping tag string (e.g., "bgm", "sfx", "ambience").
    };

    /**
     * @brief Internal runtime representation of an active audio stream/sound.
     */
    struct Sound {
        SoundId id = INVALID_SOUND_ID;  ///< Unique instance ID.
        std::string path;               ///< File asset path.
        std::string tag;                ///< Grouping tag.
        SoundFormat format = SoundFormat::Unknown; ///< Audio format type.
        u8* audioBuffer = nullptr;      ///< Raw loaded PCM data buffer.
        uint32_t dataSize = 0;          ///< Size of the PCM data buffer in bytes.
        int channel = -1;               ///< Hardware audio channel index (-1 if unassigned).
        uint32_t baseSampleRate = 22050;///< Native audio sample rate in Hz.
        float volume = 1.0f;            ///< Current volume.
        float pan = 0.0f;               ///< Current stereo pan.
        float pitch = 1.0f;             ///< Current pitch modifier.
        bool looping = false;           ///< Loop state flag.
        bool paused = false;            ///< Pause state flag.
        ndspWaveBuf waveBuf;            ///< 3DS hardware NDSP wave buffer structure.
    };

    class AudioEngine;

    /**
     * @brief Lightweight helper handle for managing and modifying active sounds fluently.
     */
    class SoundHandle {
    public:
        /** @brief Default constructor creating an invalid handle. */
        SoundHandle() = default;
        
        /** 
         * @brief Constructs a handle pointing to a specific sound ID.
         * @param id Unique sound instance ID.
         */
        explicit SoundHandle(SoundId id) : m_id(id) {}

        /** @brief Gets the underlying sound ID. @return Sound ID. */
        SoundId id()    const { return m_id; }
        
        /** @brief Checks if the handle references a valid active sound. @return True if valid, false otherwise. */
        bool    valid() const { return m_id != INVALID_SOUND_ID; }

        /** @brief Stops playback for this sound. @return Reference to self for method chaining. */
        SoundHandle& stop();
        
        /** @brief Pauses playback for this sound. @return Reference to self for method chaining. */
        SoundHandle& pause();
        
        /** @brief Resumes playback for this sound. @return Reference to self for method chaining. */
        SoundHandle& resume();

        /** 
         * @brief Sets the volume for this sound.
         * @param v Volume level [0.0, 1.0].
         * @return Reference to self for method chaining.
         */
        SoundHandle& setVolume(float v);

        /** 
         * @brief Sets the stereo pan for this sound.
         * @param p Pan level [-1.0, 1.0].
         * @return Reference to self for method chaining.
         */
        SoundHandle& setPan(float p);

        /** 
         * @brief Sets the pitch multiplier for this sound.
         * @param p Pitch modifier.
         * @return Reference to self for method chaining.
         */
        SoundHandle& setPitch(float p);

        /** 
         * @brief Sets whether this sound loops.
         * @param l True to enable looping.
         * @return Reference to self for method chaining.
         */
        SoundHandle& setLooping(bool l);

        /** 
         * @brief Assigns a tag string to this sound.
         * @param tag Tag string.
         * @return Reference to self for method chaining.
         */
        SoundHandle& setTag(const std::string& tag);

        /** @brief Checks if the sound is currently playing. @return True if playing. */
        bool isPlaying() const;
        
        /** @brief Checks if the sound is currently paused. @return True if paused. */
        bool isPaused()  const;
        
        /** @brief Checks if the sound is set to loop. @return True if looping. */
        bool isLooping() const;

        /** @brief Gets the current volume level. @return Volume value. */
        float getVolume()  const;
        
        /** @brief Gets the current stereo pan balance. @return Pan value. */
        float getPan()     const;
        
        /** @brief Gets the current pitch multiplier. @return Pitch value. */
        float getPitch()   const;
        
        /** @brief Gets the assigned hardware audio channel index. @return Channel ID. */
        int getChannel() const;
        
        /** @brief Gets the grouping tag string. @return Tag string. */
        std::string getTag() const;
        
        /** @brief Gets the asset file path. @return File path string. */
        std::string getPath() const;

    private:
        SoundId m_id = INVALID_SOUND_ID; ///< Stored sound instance ID.
    };

    /**
     * @brief Singleton manager class handling sound loading, playback channels, and global audio settings.
     */
    class AudioEngine {
    private:
        std::vector<std::unique_ptr<Sound>> m_playingSounds; ///< List of currently loaded/playing sound instances.
        bool m_initialized = false;                         ///< Initialization state flag.
        SoundId m_nextSoundId = 1;                          ///< Counter for generating unique sound IDs.
        float m_masterVolume = 1.0f;                        ///< Global master volume scale factor.
        float m_sfxVolume = 1.0f;                           ///< Global sound-effect volume scale factor.
        float m_musicVolume = 1.0f;                         ///< Global music volume scale factor.

        /** @brief Default constructor for singleton pattern. */
        AudioEngine() = default;

        /** @brief Finds a mutable pointer to a sound by its ID. @param id Target sound ID. @return Pointer to Sound, or nullptr. */
        Sound* findSound(SoundId id);
        
        /** @brief Finds a const pointer to a sound by its ID. @param id Target sound ID. @return Const pointer to Sound, or nullptr. */
        const Sound* findSound(SoundId id) const;
        
        /** @brief Applies volume and panning mix configuration to hardware. @param sound Sound reference. */
        void applyChannelMix(Sound& sound);
        
        /** @brief Applies pitch sample rate adjustments to hardware. @param sound Sound reference. */
        void applyChannelRate(Sound& sound);

    public:
        /** @brief Destructor shutting down the audio system. */
        ~AudioEngine();
        
        /** @brief Deleted copy constructor. */
        AudioEngine(const AudioEngine&) = delete;
        
        /** @brief Deleted copy assignment operator. */
        AudioEngine& operator=(const AudioEngine&) = delete;

        /**
         * @brief Gets the singleton instance of the AudioEngine.
         * @return Reference to the AudioEngine instance.
         */
        static AudioEngine& getInstance() {
            static AudioEngine instance;
            return instance;
        }

        /** @brief Initializes the hardware audio subsystem. @return True on success, false otherwise. */
        bool init();
        
        /** @brief Shuts down the audio subsystem and frees active streams. */
        void shutdown();
        
        /** @brief Updates frame-level audio playback status and garbage collects finished tracks. */
        void update();
        
        /** @brief Checks if the audio engine is initialized. @return True if initialized. */
        bool isInitialized() const { return m_initialized; }

        /**
         * @brief Plays a sound effect file with optional loop setting.
         * @param path File path to the sound asset.
         * @param loop True to loop playback.
         * @return SoundHandle referencing the active sound.
         */
        SoundHandle playSFX(const std::string& path, bool loop = false);

        /**
         * @brief Plays a sound effect using a structured parameter block.
         * @param path File path to the sound asset.
         * @param params SoundParams configuration struct.
         * @return SoundHandle referencing the active sound.
         */
        SoundHandle playSFX(const std::string& path, const SoundParams& params);

        /**
         * @brief Plays a sound effect with explicit configuration parameters.
         * @param path File path to the sound asset.
         * @param volume Volume level [0.0, 1.0].
         * @param pan Stereo pan [-1.0, 1.0].
         * @param pitch Pitch modifier.
         * @param loop True to loop playback.
         * @param tag Optional group tag string.
         * @return SoundHandle referencing the active sound.
         */
        SoundHandle playSFX(const std::string& path, float volume, float pan = 0.0f, float pitch = 1.0f, bool loop = false, const std::string& tag = "");

        /** @brief Stops a specific sound by ID. @param id Target sound ID. @return True if found and stopped. */
        bool stop(SoundId id);
        
        /** @brief Pauses a specific sound by ID. @param id Target sound ID. @return True if found and paused. */
        bool pause(SoundId id);
        
        /** @brief Resumes a specific sound by ID. @param id Target sound ID. @return True if found and resumed. */
        bool resume(SoundId id);
        
        /** @brief Checks if a sound ID is currently playing. @param id Target sound ID. @return True if playing. */
        bool isPlaying(SoundId id) const;
        
        /** @brief Checks if a sound ID is currently paused. @param id Target sound ID. @return True if paused. */
        bool isPaused(SoundId id) const;

        /** @brief Sets volume for a specific sound ID. @param id Target sound ID. @param volume Volume level. @return True on success. */
        bool setVolume(SoundId id, float volume);
        
        /** @brief Gets volume for a specific sound ID. @param id Target sound ID. @return Volume level. */
        float getVolume(SoundId id) const;

        /** @brief Sets pan for a specific sound ID. @param id Target sound ID. @param pan Pan level. @return True on success. */
        bool setPan(SoundId id, float pan);
        
        /** @brief Gets pan for a specific sound ID. @param id Target sound ID. @return Pan level. */
        float getPan(SoundId id) const;

        /** @brief Sets pitch for a specific sound ID. @param id Target sound ID. @param pitchMultiplier Pitch modifier. @return True on success. */
        bool setPitch(SoundId id, float pitchMultiplier);
        
        /** @brief Gets pitch for a specific sound ID. @param id Target sound ID. @return Pitch modifier. */
        float getPitch(SoundId id) const;

        /** @brief Sets looping state for a specific sound ID. @param id Target sound ID. @param loop Loop flag. @return True on success. */
        bool setLooping(SoundId id, bool loop);
        
        /** @brief Checks if a specific sound ID is looping. @param id Target sound ID. @return True if looping. */
        bool isLooping(SoundId id) const;

        /** @brief Sets the grouping tag for a specific sound ID. @param id Target sound ID. @param tag Tag string. @return True on success. */
        bool setTag(SoundId id, const std::string& tag);

        /** @brief Gets a const pointer to the underlying Sound structure. @param id Target sound ID. @return Const pointer to Sound. */
        const Sound* getSound(SoundId id) const { return findSound(id); }
        
        /** @brief Gets the hardware channel index for a sound ID. @param id Target sound ID. @return Channel index. */
        int getChannel(SoundId id) const;
        
        /** @brief Gets the grouping tag for a sound ID. @param id Target sound ID. @return Tag string. */
        std::string getTag(SoundId id) const;
        
        /** @brief Gets the asset file path for a sound ID. @param id Target sound ID. @return File path string. */
        std::string getPath(SoundId id) const;

        /** @brief Stops all sounds playing on a specific hardware channel. @param channel Channel index. @return True on success. */
        bool stopChannel(int channel);
        
        /** @brief Pauses all sounds playing on a specific hardware channel. @param channel Channel index. @return True on success. */
        bool pauseChannel(int channel);
        
        /** @brief Resumes all sounds playing on a specific hardware channel. @param channel Channel index. @return True on success. */
        bool resumeChannel(int channel);
        
        /** @brief Sets volume across a specific hardware channel. @param channel Channel index. @param volume Volume level. @return True on success. */
        bool setChannelVolume(int channel, float volume);
        
        /** @brief Sets stereo pan across a specific hardware channel. @param channel Channel index. @param pan Pan level. @return True on success. */
        bool setChannelPan(int channel, float pan);
        
        /** @brief Sets pitch across a specific hardware channel. @param channel Channel index. @param pitchMultiplier Pitch modifier. @return True on success. */
        bool setChannelPitch(int channel, float pitchMultiplier);
        
        /** @brief Gets the sound ID currently active on a specific channel. @param channel Channel index. @return SoundId. */
        SoundId getSoundIdForChannel(int channel) const;

        /** @brief Stops all active sounds matching a given tag. @param tag Group tag string. */
        void stopByTag(const std::string& tag);
        
        /** @brief Pauses all active sounds matching a given tag. @param tag Group tag string. */
        void pauseByTag(const std::string& tag);
        
        /** @brief Resumes all active sounds matching a given tag. @param tag Group tag string. */
        void resumeByTag(const std::string& tag);
        
        /** @brief Sets volume for all active sounds matching a given tag. @param tag Group tag string. @param volume Volume level. */
        void setVolumeByTag(const std::string& tag, float volume);

        /** @brief Stops all active instances of a specific file path asset. @param path Asset file path. */
        void stopByPath(const std::string& path);
        
        /** @brief Pauses all active instances of a specific file path asset. @param path Asset file path. */
        void pauseByPath(const std::string& path);
        
        /** @brief Resumes all active instances of a specific file path asset. @param path Asset file path. */
        void resumeByPath(const std::string& path);

        /** @brief Stops all currently active sounds across the engine. */
        void stopAll();
        
        /** @brief Pauses all currently active sounds across the engine. */
        void pauseAll();
        
        /** @brief Resumes all currently paused sounds across the engine. */
        void resumeAll();

        /** @brief Sets the global master volume scale factor. @param volume Master volume level [0.0, 1.0]. */
        void setMasterVolume(float volume);
        
        /** @brief Gets the global master volume scale factor. @return Master volume level. */
        float getMasterVolume() const { return m_masterVolume; }

        /** @brief Sets the global sound-effect volume. @param volume Volume level [0.0, 1.0]. */
        void setSfxVolume(float volume);

        /** @brief Gets the global sound-effect volume. @return Sound-effect volume level. */
        float getSfxVolume() const { return m_sfxVolume; }

        /** @brief Sets the global music volume. @param volume Volume level [0.0, 1.0]. */
        void setMusicVolume(float volume);

        /** @brief Gets the global music volume. @return Music volume level. */
        float getMusicVolume() const { return m_musicVolume; }

        /** @brief Gets the total count of currently playing/active sounds. @return Sound count. */
        size_t getPlayingSoundCount() const { return m_playingSounds.size(); }
        
        /** @brief Gets a vector of all sound IDs matching a specific tag. @param tag Group tag string. @return Vector of SoundIds. */
        std::vector<SoundId> getSoundIdsByTag(const std::string& tag) const;
        
        /** @brief Gets a vector of all sound IDs matching a specific file path. @param path Asset file path. @return Vector of SoundIds. */
        std::vector<SoundId> getSoundIdsByPath(const std::string& path) const;
    };

    // Inline SoundHandle method definitions forwarding to AudioEngine singleton
    inline SoundHandle& SoundHandle::stop() { AudioEngine::getInstance().stop(m_id);   return *this; }
    inline SoundHandle& SoundHandle::pause() { AudioEngine::getInstance().pause(m_id);  return *this; }
    inline SoundHandle& SoundHandle::resume() { AudioEngine::getInstance().resume(m_id); return *this; }

    inline SoundHandle& SoundHandle::setVolume(float v) { AudioEngine::getInstance().setVolume(m_id, v);  return *this; }
    inline SoundHandle& SoundHandle::setPan(float p) { AudioEngine::getInstance().setPan(m_id, p);     return *this; }
    inline SoundHandle& SoundHandle::setPitch(float p) { AudioEngine::getInstance().setPitch(m_id, p);   return *this; }
    inline SoundHandle& SoundHandle::setLooping(bool l) { AudioEngine::getInstance().setLooping(m_id, l); return *this; }
    inline SoundHandle& SoundHandle::setTag(const std::string& tag) { AudioEngine::getInstance().setTag(m_id, tag);   return *this; }

    inline bool SoundHandle::isPlaying() const { return AudioEngine::getInstance().isPlaying(m_id); }
    inline bool SoundHandle::isPaused()  const { return AudioEngine::getInstance().isPaused(m_id);  }
    inline bool SoundHandle::isLooping() const { return AudioEngine::getInstance().isLooping(m_id); }
    inline float SoundHandle::getVolume() const { return AudioEngine::getInstance().getVolume(m_id); }
    inline float SoundHandle::getPan()    const { return AudioEngine::getInstance().getPan(m_id);    }
    inline float SoundHandle::getPitch()  const { return AudioEngine::getInstance().getPitch(m_id);  }
    inline int SoundHandle::getChannel() const { return AudioEngine::getInstance().getChannel(m_id); }
    inline std::string SoundHandle::getTag() const { return AudioEngine::getInstance().getTag(m_id);    }
    inline std::string SoundHandle::getPath() const { return AudioEngine::getInstance().getPath(m_id);   }

}