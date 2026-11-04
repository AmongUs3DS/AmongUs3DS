#include <engine/audio.hpp>
#include <engine/filesystem.hpp>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <sys/stat.h>

namespace {
    struct WAVHeader {
        char chunkId[4];          // "RIFF"
        uint32_t chunkSize;       // File size - 8
        char format[4];           // "WAVE"
    };

    bool hasWavExtension(const std::string& path) {
        if (path.length() >= 4) {
            std::string ext = path.substr(path.length() - 4);
            return (ext == ".wav" || ext == ".WAV");
        }
        return false;
    }

    engine::SoundFormat getSoundFormatByExtension(std::string path) {
        if (hasWavExtension(path)) return engine::SoundFormat::Wav;
        return engine::SoundFormat::Unknown;
    }
}

namespace engine {

AudioEngine::~AudioEngine() {
    shutdown();
}

Sound* AudioEngine::findSound(SoundId id) {
    if (id == INVALID_SOUND_ID) return nullptr;
    for (auto& s : m_playingSounds) {
        if (s && s->id == id) return s.get();
    }
    return nullptr;
}

const Sound* AudioEngine::findSound(SoundId id) const {
    if (id == INVALID_SOUND_ID) return nullptr;
    for (const auto& s : m_playingSounds) {
        if (s && s->id == id) return s.get();
    }
    return nullptr;
}

void AudioEngine::applyChannelMix(Sound& sound) {
    if (sound.channel < 0 || sound.channel >= 24) return;
    float mix[12] = {0};
    const bool isMusic = sound.tag.find("music") != std::string::npos;
    const float categoryVolume = isMusic ? m_musicVolume : m_sfxVolume;
    float effectiveVol = m_masterVolume * categoryVolume * sound.volume;
    effectiveVol = std::max(0.0f, std::min(1.0f, effectiveVol));

    float pan = std::max(-1.0f, std::min(1.0f, sound.pan));
    float l = effectiveVol * std::min(1.0f, 1.0f - pan);
    float r = effectiveVol * std::min(1.0f, 1.0f + pan);

    mix[0] = l;
    mix[1] = r;
    ndspChnSetMix(sound.channel, mix);
}

void AudioEngine::applyChannelRate(Sound& sound) {
    if (sound.channel < 0 || sound.channel >= 24) return;
    float rate = static_cast<float>(sound.baseSampleRate) * sound.pitch;
    if (rate < 100.0f) rate = 100.0f;
    ndspChnSetRate(sound.channel, rate);
}

bool AudioEngine::init() {
    FILE* checkFirm = std::fopen("sdmc:/3ds/dspfirm.cdc", "rb");
    if (!checkFirm) {
        mkdir("sdmc:/3ds", 0777);
        FILE* f = std::fopen("sdmc:/3ds/dspfirm.cdc", "wb");
        if (f) {
            std::fclose(f);
        }
    } else {
        std::fclose(checkFirm);
    }

    Result res = ndspInit();
    if (R_FAILED(res)) {
        mkdir("sdmc:/3ds", 0777);
        FILE* f = std::fopen("sdmc:/3ds/dspfirm.cdc", "wb");
        if (f) {
            std::fclose(f);
            res = ndspInit();
        }
    }

    if (R_FAILED(res)) {
        std::printf("AudioEngine: ndspInit failed with error 0x%08lX. Audio subsystem disabled.\n", (unsigned long)res);
        m_initialized = false;
        return false;
    }

    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    m_initialized = true;
    std::printf("AudioEngine: Initialized NDSP successfully.\n");
    return true;
}

void AudioEngine::shutdown() {
    if (!m_initialized) return;

    for (auto& soundPtr : m_playingSounds) {
        if (soundPtr->channel >= 0) {
            ndspChnReset(soundPtr->channel);
        }
        if (soundPtr->audioBuffer) {
            linearFree(soundPtr->audioBuffer);
        }
    }
    m_playingSounds.clear();
    ndspExit();
    m_initialized = false;
}

void AudioEngine::update() {
    if (!m_initialized) return;

    for (auto it = m_playingSounds.begin(); it != m_playingSounds.end(); ) {
        auto& sound = *it;
        if (sound->channel >= 0 && !sound->looping && sound->waveBuf.status == NDSP_WBUF_DONE) {
            ndspChnReset(sound->channel);
            if (sound->audioBuffer) {
                linearFree(sound->audioBuffer);
            }
            it = m_playingSounds.erase(it);
        } else {
            ++it;
        }
    }
}

SoundHandle AudioEngine::playSFX(const std::string& path, bool loop) {
    SoundParams params;
    params.loop = loop;
    return playSFX(path, params);
}

SoundHandle AudioEngine::playSFX(const std::string& path, float volume, float pan, float pitch, bool loop, const std::string& tag) {
    SoundParams params;
    params.volume = volume;
    params.pan = pan;
    params.pitch = pitch;
    params.loop = loop;
    params.tag = tag;
    return playSFX(path, params);
}

SoundHandle AudioEngine::playSFX(const std::string& path, const SoundParams& params) {
    if (!m_initialized) {
        std::printf("AudioEngine: Skipping SFX '%s' (audio subsystem not initialized).\n", path.c_str());
        return SoundHandle();
    }

    update();

    SoundFormat format = getSoundFormatByExtension(path);
    if (format != SoundFormat::Wav) {
        std::printf("AudioEngine: Unsupported or unknown format for %s\n", path.c_str());
        return SoundHandle();
    }

    std::vector<uint8_t> fileData = Filesystem::getInstance().readFileBinaryRomfs(path.c_str());
    if (fileData.empty() || fileData.size() < sizeof(WAVHeader)) {
        std::printf("AudioEngine: Failed to read or invalid WAV file: %s\n", path.c_str());
        return SoundHandle();
    }

    WAVHeader header;
    std::memcpy(&header, fileData.data(), sizeof(WAVHeader));

    if (std::strncmp(header.chunkId, "RIFF", 4) != 0 || 
        std::strncmp(header.format, "WAVE", 4) != 0) {
        std::printf("AudioEngine: Invalid WAV file structure: %s\n", path.c_str());
        return SoundHandle();
    }

    uint32_t sampleRate = 22050;
    uint16_t numChannels = 2;
    uint16_t bitsPerSample = 16;
    size_t dataOffset = 0;
    uint32_t dataSize = 0;

    size_t cursor = 12;
    while (cursor + 8 <= fileData.size()) {
        char chunkId[4];
        uint32_t chunkSize;
        std::memcpy(chunkId, &fileData[cursor], 4);
        std::memcpy(&chunkSize, &fileData[cursor + 4], 4);

        if (std::strncmp(chunkId, "fmt ", 4) == 0 && cursor + 24 <= fileData.size()) {
            std::memcpy(&numChannels, &fileData[cursor + 10], 2);
            std::memcpy(&sampleRate, &fileData[cursor + 12], 4);
            std::memcpy(&bitsPerSample, &fileData[cursor + 22], 2);
        } else if (std::strncmp(chunkId, "data", 4) == 0) {
            dataOffset = cursor + 8;
            dataSize = chunkSize;
            break;
        }

        size_t pad = (chunkSize % 2 != 0) ? 1 : 0;
        if (cursor + 8 + chunkSize + pad < cursor) break;
        cursor += 8 + chunkSize + pad;
    }

    if (dataOffset == 0 || dataOffset + dataSize > fileData.size()) {
        std::printf("AudioEngine: Could not find valid 'data' chunk in WAV: %s\n", path.c_str());
        return SoundHandle();
    }

    u8* audioBuffer = (u8*)linearAlloc(dataSize);
    if (!audioBuffer) {
        std::printf("AudioEngine: Failed to allocate linear memory for audio.\n");
        return SoundHandle();
    }

    std::memcpy(audioBuffer, fileData.data() + dataOffset, dataSize);
    DSP_FlushDataCache(audioBuffer, (dataSize + 31) & ~31);

    int channel = -1;
    for (int i = 0; i < 24; i++) {
        bool inUse = false;
        for (auto& s : m_playingSounds) {
            if (s->channel == i) { inUse = true; break; }
        }
        if (!inUse) {
            channel = i;
            break;
        }
    }

    if (channel == -1) {
        std::printf("AudioEngine: No free NDSP channels available!\n");
        linearFree(audioBuffer);
        return SoundHandle();
    }

    ndspChnReset(channel);
    ndspChnSetInterp(channel, NDSP_INTERP_LINEAR);

    u16 ndspFormat = NDSP_FORMAT_MONO_PCM16;
    if (numChannels == 2) {
        ndspFormat = (bitsPerSample == 8) ? NDSP_FORMAT_STEREO_PCM8 : NDSP_FORMAT_STEREO_PCM16;
    } else {
        ndspFormat = (bitsPerSample == 8) ? NDSP_FORMAT_MONO_PCM8 : NDSP_FORMAT_MONO_PCM16;
    }
    ndspChnSetFormat(channel, ndspFormat);

    SoundId soundId = m_nextSoundId++;
    if (m_nextSoundId == INVALID_SOUND_ID) m_nextSoundId = 1;

    auto sound = std::make_unique<Sound>();
    sound->id = soundId;
    sound->path = path;
    sound->tag = params.tag;
    sound->format = format;
    sound->audioBuffer = audioBuffer;
    sound->dataSize = dataSize;
    sound->channel = channel;
    sound->baseSampleRate = sampleRate;
    sound->volume = params.volume;
    sound->pan = params.pan;
    sound->pitch = params.pitch;
    sound->looping = params.loop;
    sound->paused = false;

    applyChannelRate(*sound);
    applyChannelMix(*sound);

    std::memset(&sound->waveBuf, 0, sizeof(ndspWaveBuf));
    sound->waveBuf.data_vaddr = audioBuffer;
    sound->waveBuf.nsamples = dataSize / (numChannels * (bitsPerSample / 8));
    sound->waveBuf.looping = params.loop;

    ndspChnWaveBufAdd(channel, &sound->waveBuf);

    m_playingSounds.push_back(std::move(sound));

    std::printf("Playing SFX [ID: %lu, Ch: %d, Tag: '%s', Loop: %s]: %s\n",
                (unsigned long)soundId, channel, params.tag.c_str(), params.loop ? "true" : "false", path.c_str());
    return SoundHandle(soundId);
}

bool AudioEngine::stop(SoundId id) {
    if (id == INVALID_SOUND_ID) return false;
    for (auto it = m_playingSounds.begin(); it != m_playingSounds.end(); ++it) {
        if ((*it)->id == id) {
            if ((*it)->channel >= 0) {
                ndspChnReset((*it)->channel);
            }
            if ((*it)->audioBuffer) {
                linearFree((*it)->audioBuffer);
            }
            m_playingSounds.erase(it);
            return true;
        }
    }
    return false;
}

bool AudioEngine::pause(SoundId id) {
    auto* s = findSound(id);
    if (!s || s->channel < 0 || s->paused) return false;
    ndspChnSetPaused(s->channel, true);
    s->paused = true;
    return true;
}

bool AudioEngine::resume(SoundId id) {
    auto* s = findSound(id);
    if (!s || s->channel < 0 || !s->paused) return false;
    ndspChnSetPaused(s->channel, false);
    s->paused = false;
    return true;
}

bool AudioEngine::isPlaying(SoundId id) const {
    const auto* s = findSound(id);
    return (s != nullptr && !s->paused);
}

bool AudioEngine::isPaused(SoundId id) const {
    const auto* s = findSound(id);
    return (s != nullptr && s->paused);
}

bool AudioEngine::setVolume(SoundId id, float volume) {
    auto* s = findSound(id);
    if (!s) return false;
    s->volume = std::max(0.0f, std::min(1.0f, volume));
    applyChannelMix(*s);
    return true;
}

float AudioEngine::getVolume(SoundId id) const {
    const auto* s = findSound(id);
    return s ? s->volume : 0.0f;
}

bool AudioEngine::setPan(SoundId id, float pan) {
    auto* s = findSound(id);
    if (!s) return false;
    s->pan = std::max(-1.0f, std::min(1.0f, pan));
    applyChannelMix(*s);
    return true;
}

float AudioEngine::getPan(SoundId id) const {
    const auto* s = findSound(id);
    return s ? s->pan : 0.0f;
}

bool AudioEngine::setPitch(SoundId id, float pitchMultiplier) {
    auto* s = findSound(id);
    if (!s) return false;
    s->pitch = std::max(0.1f, std::min(4.0f, pitchMultiplier));
    applyChannelRate(*s);
    return true;
}

float AudioEngine::getPitch(SoundId id) const {
    const auto* s = findSound(id);
    return s ? s->pitch : 1.0f;
}

bool AudioEngine::setLooping(SoundId id, bool loop) {
    auto* s = findSound(id);
    if (!s) return false;
    s->looping = loop;
    s->waveBuf.looping = loop;
    return true;
}

bool AudioEngine::isLooping(SoundId id) const {
    const auto* s = findSound(id);
    return s ? s->looping : false;
}

bool AudioEngine::setTag(SoundId id, const std::string& tag) {
    auto* s = findSound(id);
    if (!s) return false;
    s->tag = tag;
    return true;
}

int AudioEngine::getChannel(SoundId id) const {
    const auto* s = findSound(id);
    return s ? s->channel : -1;
}

std::string AudioEngine::getTag(SoundId id) const {
    const auto* s = findSound(id);
    return s ? s->tag : "";
}

std::string AudioEngine::getPath(SoundId id) const {
    const auto* s = findSound(id);
    return s ? s->path : "";
}

bool AudioEngine::stopChannel(int channel) {
    if (channel < 0 || channel >= 24) return false;
    for (auto it = m_playingSounds.begin(); it != m_playingSounds.end(); ++it) {
        if ((*it)->channel == channel) {
            ndspChnReset(channel);
            if ((*it)->audioBuffer) {
                linearFree((*it)->audioBuffer);
            }
            m_playingSounds.erase(it);
            return true;
        }
    }
    ndspChnReset(channel);
    return false;
}

bool AudioEngine::pauseChannel(int channel) {
    if (channel < 0 || channel >= 24) return false;
    ndspChnSetPaused(channel, true);
    for (auto& s : m_playingSounds) {
        if (s->channel == channel) {
            s->paused = true;
            return true;
        }
    }
    return false;
}

bool AudioEngine::resumeChannel(int channel) {
    if (channel < 0 || channel >= 24) return false;
    ndspChnSetPaused(channel, false);
    for (auto& s : m_playingSounds) {
        if (s->channel == channel) {
            s->paused = false;
            return true;
        }
    }
    return false;
}

bool AudioEngine::setChannelVolume(int channel, float volume) {
    for (auto& s : m_playingSounds) {
        if (s->channel == channel) {
            s->volume = std::max(0.0f, std::min(1.0f, volume));
            applyChannelMix(*s);
            return true;
        }
    }
    return false;
}

bool AudioEngine::setChannelPan(int channel, float pan) {
    for (auto& s : m_playingSounds) {
        if (s->channel == channel) {
            s->pan = std::max(-1.0f, std::min(1.0f, pan));
            applyChannelMix(*s);
            return true;
        }
    }
    return false;
}

bool AudioEngine::setChannelPitch(int channel, float pitchMultiplier) {
    for (auto& s : m_playingSounds) {
        if (s->channel == channel) {
            s->pitch = std::max(0.1f, std::min(4.0f, pitchMultiplier));
            applyChannelRate(*s);
            return true;
        }
    }
    return false;
}

SoundId AudioEngine::getSoundIdForChannel(int channel) const {
    for (const auto& s : m_playingSounds) {
        if (s->channel == channel) return s->id;
    }
    return INVALID_SOUND_ID;
}

void AudioEngine::stopByTag(const std::string& tag) {
    if (tag.empty()) return;
    for (auto it = m_playingSounds.begin(); it != m_playingSounds.end(); ) {
        if ((*it)->tag == tag) {
            if ((*it)->channel >= 0) ndspChnReset((*it)->channel);
            if ((*it)->audioBuffer) linearFree((*it)->audioBuffer);
            it = m_playingSounds.erase(it);
        } else {
            ++it;
        }
    }
}

void AudioEngine::pauseByTag(const std::string& tag) {
    if (tag.empty()) return;
    for (auto& s : m_playingSounds) {
        if (s->tag == tag && s->channel >= 0) {
            ndspChnSetPaused(s->channel, true);
            s->paused = true;
        }
    }
}

void AudioEngine::resumeByTag(const std::string& tag) {
    if (tag.empty()) return;
    for (auto& s : m_playingSounds) {
        if (s->tag == tag && s->channel >= 0) {
            ndspChnSetPaused(s->channel, false);
            s->paused = false;
        }
    }
}

void AudioEngine::setVolumeByTag(const std::string& tag, float volume) {
    if (tag.empty()) return;
    for (auto& s : m_playingSounds) {
        if (s->tag == tag) {
            s->volume = std::max(0.0f, std::min(1.0f, volume));
            applyChannelMix(*s);
        }
    }
}

void AudioEngine::stopByPath(const std::string& path) {
    for (auto it = m_playingSounds.begin(); it != m_playingSounds.end(); ) {
        if ((*it)->path == path) {
            if ((*it)->channel >= 0) ndspChnReset((*it)->channel);
            if ((*it)->audioBuffer) linearFree((*it)->audioBuffer);
            it = m_playingSounds.erase(it);
        } else {
            ++it;
        }
    }
}

void AudioEngine::pauseByPath(const std::string& path) {
    for (auto& s : m_playingSounds) {
        if (s->path == path && s->channel >= 0) {
            ndspChnSetPaused(s->channel, true);
            s->paused = true;
        }
    }
}

void AudioEngine::resumeByPath(const std::string& path) {
    for (auto& s : m_playingSounds) {
        if (s->path == path && s->channel >= 0) {
            ndspChnSetPaused(s->channel, false);
            s->paused = false;
        }
    }
}

void AudioEngine::stopAll() {
    for (auto& s : m_playingSounds) {
        if (s->channel >= 0) ndspChnReset(s->channel);
        if (s->audioBuffer) linearFree(s->audioBuffer);
    }
    m_playingSounds.clear();
}

void AudioEngine::pauseAll() {
    for (auto& s : m_playingSounds) {
        if (s->channel >= 0) {
            ndspChnSetPaused(s->channel, true);
            s->paused = true;
        }
    }
}

void AudioEngine::resumeAll() {
    for (auto& s : m_playingSounds) {
        if (s->channel >= 0) {
            ndspChnSetPaused(s->channel, false);
            s->paused = false;
        }
    }
}

void AudioEngine::setMasterVolume(float volume) {
    m_masterVolume = std::max(0.0f, std::min(1.0f, volume));
    for (auto& s : m_playingSounds) {
        if (s) applyChannelMix(*s);
    }
}

void AudioEngine::setSfxVolume(float volume) {
    m_sfxVolume = std::max(0.0f, std::min(1.0f, volume));
    for (auto& s : m_playingSounds) {
        if (s) applyChannelMix(*s);
    }
}

void AudioEngine::setMusicVolume(float volume) {
    m_musicVolume = std::max(0.0f, std::min(1.0f, volume));
    for (auto& s : m_playingSounds) {
        if (s) applyChannelMix(*s);
    }
}

std::vector<SoundId> AudioEngine::getSoundIdsByTag(const std::string& tag) const {
    std::vector<SoundId> result;
    for (const auto& s : m_playingSounds) {
        if (s && s->tag == tag) result.push_back(s->id);
    }
    return result;
}

std::vector<SoundId> AudioEngine::getSoundIdsByPath(const std::string& path) const {
    std::vector<SoundId> result;
    for (const auto& s : m_playingSounds) {
        if (s && s->path == path) result.push_back(s->id);
    }
    return result;
}

}