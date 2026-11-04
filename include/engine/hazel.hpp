#pragma once

#include <includes.hpp>

/**
 * @brief Main engine namespace.
 */
namespace engine {
/**
 * @brief Hazel networking protocol namespace.
 */
namespace hazel {

    /**
     * @brief Hazel packet type enumeration.
     */
    enum class PacketType : uint8_t {
        Unreliable = 0x00,  ///< Unreliable packet.
        Reliable = 0x01,    ///< Reliable packet requiring acknowledgement.
        Hello = 0x08,       ///< Connection handshake hello packet.
        Disconnect = 0x09,  ///< Disconnection notification packet.
        Acknowledge = 0x0A, ///< Packet acknowledgement receipt.
        Ping = 0x0C         ///< Keep-alive ping packet.
    };

    /**
     * @brief Utility class for serializing and writing Hazel networking packets.
     */
    class MessageWriter {
    private:
        std::vector<uint8_t> m_buffer; ///< Internal byte buffer for serialized data.

    public:
        /** @brief Constructs an empty MessageWriter. */
        MessageWriter() = default;
        
        /** 
         * @brief Writes a single byte to the buffer.
         * @param value Byte value.
         */
        void writeByte(uint8_t value);
        
        /** 
         * @brief Writes a raw byte array to the buffer.
         * @param data Pointer to source byte data.
         * @param length Number of bytes to write.
         */
        void writeBytes(const uint8_t* data, size_t length);
        
        /** 
         * @brief Writes a 16-bit unsigned integer to the buffer in network byte order.
         * @param value 16-bit integer value.
         */
        void writeUInt16(uint16_t value);
        
        /** 
         * @brief Writes a 32-bit unsigned integer to the buffer.
         * @param value 32-bit integer value.
         */
        void writeUInt32(uint32_t value);
        
        /** 
         * @brief Writes a 32-bit signed integer to the buffer.
         * @param value 32-bit signed integer value.
         */
        void writeInt32(int32_t value);
        
        /** 
         * @brief Writes a 32-bit floating-point number to the buffer.
         * @param value Float value.
         */
        void writeFloat(float value);
        
        /** 
         * @brief Writes a length-prefixed string to the buffer.
         * @param value String value.
         */
        void writeString(const std::string& value);
        
        /** 
         * @brief Writes a variable-length packed signed 32-bit integer.
         * @param value Integer value.
         */
        void writePackedInt32(int32_t value);
        
        /** 
         * @brief Writes a variable-length packed unsigned 32-bit integer.
         * @param value Unsigned integer value.
         */
        void writePackedUInt32(uint32_t value);

        /** 
         * @brief Begins a nested message block with a given tag.
         * @param tag Message tag byte.
         */
        void startMessage(uint8_t tag);
        
        /** @brief Finalizes and ends the current nested message block. */
        void endMessage();

        /** @brief Gets a const reference to the underlying serialized byte buffer. @return Byte vector reference. */
        const std::vector<uint8_t>& getBuffer() const { return m_buffer; }
        
        /** @brief Clears the internal write buffer. */
        void clear() { m_buffer.clear(); }

    private:
        std::vector<size_t> m_messageStack; ///< Stack tracking nested message offsets.
    };

    /**
     * @brief Utility class for parsing and reading Hazel networking packets.
     */
    class MessageReader {
    private:
        std::vector<uint8_t> m_buffer;         ///< Owned packet byte buffer.
        size_t m_cursor = 0;                    ///< Current read cursor position in bytes.
        uint8_t m_tag = 0;                      ///< Active message tag byte.

    public:
        /** 
         * @brief Constructs a MessageReader from a byte buffer.
         * @param buffer Byte vector containing packet data.
         */
        MessageReader(const std::vector<uint8_t>& buffer);
        
        /** @brief Reads a single byte from the buffer. @return Byte value. */
        uint8_t readByte();
        
        /** @brief Reads a raw byte array of specified length. @param length Number of bytes to read. @return Byte vector. */
        std::vector<uint8_t> readBytes(size_t length);
        
        /** @brief Reads a 16-bit unsigned integer. @return 16-bit integer value. */
        uint16_t readUInt16();
        
        /** @brief Reads a 32-bit unsigned integer. @return 32-bit integer value. */
        uint32_t readUInt32();
        
        /** @brief Reads a 32-bit signed integer. @return 32-bit signed integer value. */
        int32_t readInt32();
        
        /** @brief Reads a 32-bit floating-point number. @return Float value. */
        float readFloat();
        
        /** @brief Reads a length-prefixed string. @return String value. */
        std::string readString();
        
        /** @brief Reads a variable-length packed signed 32-bit integer. @return Integer value. */
        int32_t readPackedInt32();
        
        /** @brief Reads a variable-length packed unsigned 32-bit integer. @return Unsigned integer value. */
        uint32_t readPackedUInt32();

        /** @brief Gets the current active message tag. @return Tag byte. */
        uint8_t getTag() const { return m_tag; }
        
        /** @brief Reads a nested sub-message from the stream. @return MessageReader instance for sub-message. */
        MessageReader readMessage(); 

        /** @brief Checks if there are more bytes available to read. @return True if unread bytes remain. */
        bool hasMore() const { return m_cursor < m_buffer.size(); }
        
        /** @brief Gets the current read cursor position. @return Cursor position offset in bytes. */
        size_t getPosition() const { return m_cursor; }
    };

    /**
     * @brief Client network connection class implementing the Hazel protocol.
     */
    class HazelClient {
    private:
        int m_socket = -1;             ///< UDP socket file descriptor.
        sockaddr_in m_serverAddr;      ///< Server socket address structure.
        uint16_t m_nonce = 0;          ///< Packet sequence nonce counter.
        bool m_connected = false;      ///< Connection state flag.
        int32_t m_hostGameCode = 0;
        int32_t m_gameCode = 0;
        int32_t m_playerId = 0;
        int32_t m_hostId = 0;
        bool m_joinedGame = false;
        bool m_gameStarted = false;
        std::vector<int32_t> m_otherPlayerIds;

    public:
        /** @brief Constructs a HazelClient instance. */
        HazelClient() = default;
        
        /** @brief Destructor disconnecting and cleaning up sockets. */
        ~HazelClient();

        /** 
         * @brief Connects to a Hazel server at a given IP address and port.
         * @param ip Server IPv4 address string.
         * @param port Server port number.
         * @return True if connection initiated successfully, false otherwise.
         */
        bool connect(const std::string& ip, uint16_t port);
        
        /** @brief Disconnects from the server. */
        void disconnect();
        
        /** @brief Processes network updates, receives incoming packets, and sends pings. */
        void update();

        /** @brief Sends an unreliable message packet. @param msg MessageWriter instance. */
        void sendUnreliable(const MessageWriter& msg);
        
        /** @brief Sends a reliable message packet requiring acknowledgement. @param msg MessageWriter instance. */
        void sendReliable(const MessageWriter& msg);
        
        /** @brief Sends a handshake hello packet with player credentials and version. @param username Player username string. @param clientVersion Game client version identifier. */
        void sendHello(const std::string& username, uint32_t clientVersion);

        /** @brief Requests a new lobby using the server's version-4 game options format. */
        void sendHostGame();

        /** @brief Requests to join a lobby using its packed game code. */
        void sendJoinGame(int32_t gameCode);
        
        /** @brief Checks if connected to a server. @return True if connected. */
        bool isConnected() const { return m_connected; }

        /** @brief Returns the game code received from a successful HostGame response, or zero. */
        int32_t getHostGameCode() const { return m_hostGameCode; }

        /** @brief Returns whether a JoinGame success response has been received. */
        bool hasJoinedGame() const { return m_joinedGame; }

        /** @brief Returns the assigned player ID from a JoinGame response. */
        int32_t getPlayerId() const { return m_playerId; }

        /** @brief Returns the current lobby game code. */
        int32_t getGameCode() const { return m_gameCode != 0 ? m_gameCode : m_hostGameCode; }

        /** @brief Returns the server-assigned host ID for the current lobby. */
        int32_t getHostId() const { return m_hostId; }

        /** @brief Returns IDs of players already present when joining. */
        const std::vector<int32_t>& getOtherPlayerIds() const { return m_otherPlayerIds; }

        /** @brief Returns whether the server has started the current game. */
        bool hasGameStarted() const { return m_gameStarted; }

    private:
        /** @brief Sends an acknowledgement packet for a received nonce. @param nonce Packet nonce to acknowledge. */
        void sendAcknowledge(uint16_t nonce);
        
        /** @brief Handles and parses an incoming packet byte buffer. @param data Raw packet byte vector. */
        void handleIncomingPacket(const std::vector<uint8_t>& data);
    };

}
}
