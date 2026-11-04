#include <engine/hazel.hpp>
#include <engine/network.hpp>

namespace engine {
namespace hazel {

    void MessageWriter::writeByte(uint8_t value) {
        m_buffer.push_back(value);
    }

    void MessageWriter::writeBytes(const uint8_t* data, size_t length) {
        m_buffer.insert(m_buffer.end(), data, data + length);
    }

    void MessageWriter::writeUInt16(uint16_t value) {
        writeByte(value & 0xFF);
        writeByte((value >> 8) & 0xFF);
    }

    void MessageWriter::writeUInt32(uint32_t value) {
        writeByte(value & 0xFF);
        writeByte((value >> 8) & 0xFF);
        writeByte((value >> 16) & 0xFF);
        writeByte((value >> 24) & 0xFF);
    }

    void MessageWriter::writeInt32(int32_t value) {
        writeUInt32(static_cast<uint32_t>(value));
    }

    void MessageWriter::writeFloat(float value) {
        uint32_t raw;
        std::memcpy(&raw, &value, sizeof(float));
        writeUInt32(raw);
    }

    void MessageWriter::writeString(const std::string& value) {
        writePackedUInt32(value.length());
        writeBytes(reinterpret_cast<const uint8_t*>(value.c_str()), value.length());
    }

    void MessageWriter::writePackedUInt32(uint32_t value) {
        do {
            uint8_t b = value & 0x7F;
            value >>= 7;
            if (value > 0) b |= 0x80;
            writeByte(b);
        } while (value > 0);
    }

    void MessageWriter::writePackedInt32(int32_t value) {
        writePackedUInt32(static_cast<uint32_t>(value));
    }

    void MessageWriter::startMessage(uint8_t tag) {
        m_messageStack.push_back(m_buffer.size()); 
        writeUInt16(0);
        writeByte(tag);
    }

    void MessageWriter::endMessage() {
        if (m_messageStack.empty()) return;
        size_t startPos = m_messageStack.back();
        m_messageStack.pop_back();
        
        uint16_t length = static_cast<uint16_t>(m_buffer.size() - startPos - 3);
        m_buffer[startPos] = length & 0xFF;
        m_buffer[startPos + 1] = (length >> 8) & 0xFF;
    }

    MessageReader::MessageReader(const std::vector<uint8_t>& buffer) : m_buffer(buffer) {}

    uint8_t MessageReader::readByte() {
        if (m_cursor >= m_buffer.size()) return 0;
        return m_buffer[m_cursor++];
    }

    std::vector<uint8_t> MessageReader::readBytes(size_t length) {
        std::vector<uint8_t> data;
        if (m_cursor + length <= m_buffer.size()) {
            data.insert(data.end(), m_buffer.begin() + m_cursor, m_buffer.begin() + m_cursor + length);
            m_cursor += length;
        }
        return data;
    }

    uint16_t MessageReader::readUInt16() {
        uint16_t value = readByte();
        value |= (readByte() << 8);
        return value;
    }

    uint32_t MessageReader::readUInt32() {
        uint32_t value = readByte();
        value |= (readByte() << 8);
        value |= (readByte() << 16);
        value |= (readByte() << 24);
        return value;
    }

    int32_t MessageReader::readInt32() {
        return static_cast<int32_t>(readUInt32());
    }

    float MessageReader::readFloat() {
        uint32_t raw = readUInt32();
        float value;
        std::memcpy(&value, &raw, sizeof(float));
        return value;
    }

    std::string MessageReader::readString() {
        uint32_t length = readPackedUInt32();
        std::vector<uint8_t> strData = readBytes(length);
        return std::string(strData.begin(), strData.end());
    }

    uint32_t MessageReader::readPackedUInt32() {
        uint32_t value = 0;
        int shift = 0;
        uint8_t b;
        do {
            b = readByte();
            value |= (b & 0x7F) << shift;
            shift += 7;
        } while ((b & 0x80) != 0);
        return value;
    }

    int32_t MessageReader::readPackedInt32() {
        return static_cast<int32_t>(readPackedUInt32());
    }

    MessageReader MessageReader::readMessage() {
        if (m_cursor + 3 > m_buffer.size()) {
            MessageReader empty(m_buffer);
            empty.m_cursor = empty.m_buffer.size();
            return empty;
        }

        uint16_t length = readUInt16();
        uint8_t tag = readByte();
        if (m_cursor + length > m_buffer.size()) {
            m_cursor = m_buffer.size();
            MessageReader empty(m_buffer);
            empty.m_cursor = empty.m_buffer.size();
            return empty;
        }

        std::vector<uint8_t> payload = readBytes(length);
        
        MessageReader nestedReader(payload);
        nestedReader.m_tag = tag;
        return nestedReader;
    }

    HazelClient::~HazelClient() {
        disconnect();
    }

    bool HazelClient::connect(const std::string& ip, uint16_t port) {
        m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (m_socket < 0) return false;

        int flags = fcntl(m_socket, F_GETFL, 0);
        fcntl(m_socket, F_SETFL, flags | O_NONBLOCK);

        m_serverAddr.sin_family = AF_INET;
        m_serverAddr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &m_serverAddr.sin_addr);

        m_connected = true;
        m_nonce = 0;
        return true;
    }

    void HazelClient::disconnect() {
        if (m_socket >= 0) {
            uint8_t discPacket[1] = { static_cast<uint8_t>(PacketType::Disconnect) };
            sendto(m_socket, discPacket, 1, 0, (struct sockaddr*)&m_serverAddr, sizeof(m_serverAddr));
            
            close(m_socket);
            m_socket = -1;
        }
        m_connected = false;
    }

    void HazelClient::sendHello(const std::string& username, uint32_t clientVersion) {
        MessageWriter handshake;
        handshake.writeInt32(static_cast<int32_t>(clientVersion));
        handshake.writeString(username);

        std::vector<uint8_t> packet;
        packet.push_back(static_cast<uint8_t>(PacketType::Hello));
        const uint16_t nonce = ++m_nonce;
        packet.push_back((nonce >> 8) & 0xFF);
        packet.push_back(nonce & 0xFF);
        packet.insert(packet.end(), handshake.getBuffer().begin(), handshake.getBuffer().end());

        sendto(m_socket, packet.data(), packet.size(), 0,
               (struct sockaddr*)&m_serverAddr, sizeof(m_serverAddr));
    }

    void HazelClient::sendHostGame() {
        if (!m_connected) return;

        MessageWriter options;
        options.writeByte(4);          // GameOptionsData version
        options.writeByte(10);         // MaxPlayers
        options.writeUInt32(256);      // English keywords
        options.writeByte(0);          // Skeld
        options.writeFloat(1.0f);      // PlayerSpeedMod
        options.writeFloat(1.0f);      // CrewLightMod
        options.writeFloat(1.0f);      // ImpostorLightMod
        options.writeFloat(15.0f);     // KillCooldown
        options.writeByte(1);          // Common tasks
        options.writeByte(2);          // Long tasks
        options.writeByte(2);          // Short tasks
        options.writeInt32(1);         // Emergency meetings
        options.writeByte(1);          // Impostors
        options.writeByte(1);          // Normal kill distance
        options.writeUInt32(15);       // Discussion time
        options.writeUInt32(120);      // Voting time
        options.writeByte(1);          // IsDefaults
        options.writeByte(15);         // Emergency cooldown
        options.writeByte(1);          // Confirm impostor
        options.writeByte(1);          // Visual tasks
        options.writeByte(0);          // Anonymous votes
        options.writeByte(0);          // Task bar always

        MessageWriter message;
        message.startMessage(0);       // MessageFlags.HostGame
        message.writePackedUInt32(options.getBuffer().size());
        message.writeBytes(options.getBuffer().data(), options.getBuffer().size());
        message.endMessage();
        sendReliable(message);
    }

    void HazelClient::sendJoinGame(int32_t gameCode) {
        if (!m_connected) return;

        MessageWriter message;
        message.startMessage(1);       // MessageFlags.JoinGame
        message.writeInt32(gameCode);
        message.writeByte(0);          // Protocol-reserved field
        message.endMessage();
        sendReliable(message);
    }

    void HazelClient::sendUnreliable(const MessageWriter& msg) {
        std::vector<uint8_t> packet;
        packet.push_back(static_cast<uint8_t>(PacketType::Unreliable));
        packet.insert(packet.end(), msg.getBuffer().begin(), msg.getBuffer().end());
        
        sendto(m_socket, packet.data(), packet.size(), 0, 
               (struct sockaddr*)&m_serverAddr, sizeof(m_serverAddr));
    }

    void HazelClient::sendReliable(const MessageWriter& msg) {
        std::vector<uint8_t> packet;
        packet.push_back(static_cast<uint8_t>(PacketType::Reliable));
        
        uint16_t currentNonce = m_nonce++;
        packet.push_back((currentNonce >> 8) & 0xFF);
        packet.push_back(currentNonce & 0xFF);
        
        packet.insert(packet.end(), msg.getBuffer().begin(), msg.getBuffer().end());
        
        sendto(m_socket, packet.data(), packet.size(), 0, 
               (struct sockaddr*)&m_serverAddr, sizeof(m_serverAddr));
    }

    void HazelClient::sendAcknowledge(uint16_t nonce) {
        uint8_t ackPacket[4];
        ackPacket[0] = static_cast<uint8_t>(PacketType::Acknowledge);
        ackPacket[1] = (nonce >> 8) & 0xFF;
        ackPacket[2] = nonce & 0xFF;
        ackPacket[3] = 0xFF;
        
        sendto(m_socket, ackPacket, 4, 0, (struct sockaddr*)&m_serverAddr, sizeof(m_serverAddr));
    }

    void HazelClient::update() {
        if (!m_connected) return;

        uint8_t buffer[2048];
        sockaddr_in sender;
        socklen_t senderLen = sizeof(sender);

        while (true) {
            int bytesRead = recvfrom(m_socket, buffer, sizeof(buffer), 0, 
                                     (struct sockaddr*)&sender, &senderLen);
            
            if (bytesRead <= 0) break;

            std::vector<uint8_t> data(buffer, buffer + bytesRead);
            handleIncomingPacket(data);
        }
    }

    void HazelClient::handleIncomingPacket(const std::vector<uint8_t>& data) {
        if (data.empty()) return;

        PacketType type = static_cast<PacketType>(data[0]);
        MessageReader reader(data);
        reader.readByte();

        if (type == PacketType::Reliable || type == PacketType::Ping) {
            if (data.size() < 3) return;
            uint16_t nonce = (static_cast<uint16_t>(data[1]) << 8) | data[2];
            sendAcknowledge(nonce);

            if (type == PacketType::Reliable) {
                if (data.size() <= 3) return;

                std::vector<uint8_t> messageData(data.begin() + 3, data.end());
                MessageReader messages(messageData);
                while (messages.hasMore()) {
                    MessageReader message = messages.readMessage();
                    if (message.getTag() == 0 && message.hasMore()) {
                        m_hostGameCode = message.readInt32();
                        m_gameCode = m_hostGameCode;
                    } else if (message.getTag() == 1 && message.hasMore()) {
                        message.readInt32();
                        if (message.hasMore()) {
                            m_playerId = message.readInt32();
                            if (message.hasMore()) {
                                message.readInt32();
                                m_joinedGame = true;
                            }
                        }
                    } else if (message.getTag() == 2) {
                        m_gameStarted = true;
                    } else if (message.getTag() == 7 && message.hasMore()) {
                        m_gameCode = message.readInt32();
                        m_playerId = message.readInt32();
                        m_hostId = message.readInt32();
                        m_otherPlayerIds.clear();
                        const uint32_t count = message.readPackedUInt32();
                        for (uint32_t i = 0; i < count && message.hasMore(); ++i) {
                            m_otherPlayerIds.push_back(message.readPackedInt32());
                        }
                        m_joinedGame = true;
                    } else if (message.getTag() == 10 && message.hasMore()) {
                        m_gameCode = message.readInt32();
                    }
                }
            }
        }
    }

}
}