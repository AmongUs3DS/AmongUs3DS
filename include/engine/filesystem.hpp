#pragma once

#include <includes.hpp>

/**
 * @brief Main engine namespace.
 */
namespace engine {

    /**
     * @brief Singleton filesystem helper class responsible for path resolution and reading asset files.
     */
    class Filesystem {
    public:
        /** @brief Deleted copy constructor. */
        Filesystem(const Filesystem&) = delete;
        
        /** @brief Deleted copy assignment operator. */
        Filesystem& operator=(const Filesystem&) = delete;

        /**
         * @brief Gets the singleton instance of the Filesystem.
         * @return Reference to the Filesystem instance.
         */
        static Filesystem& getInstance() {
            static Filesystem instance;
            return instance;
        }

        /**
         * @brief Resolves virtual romfs file paths to concrete storage paths on the SD card.
         * @param path Input file path string.
         * @return Resolved absolute or relative path string.
         */
        std::string resolvePath(const std::string& path) const {
            if (path.rfind("romfs:/", 0) == 0) {
                return "sdmc:/3ds/AmongUs3DS/assets/" + path.substr(7);
            }
            return path;
        }

        /**
         * @brief Checks if a file exists.
         * @param path Path to the file.
         * @return True if file exists, false otherwise.
         */
        bool exists(const std::string& path) const {
            std::string resolved = resolvePath(path);
            FILE* f = std::fopen(resolved.c_str(), "r");
            if (f) {
                std::fclose(f);
                return true;
            }
            return false;
        }

        /**
         * @brief Reads a text asset file from storage.
         * @param path Path to the target file.
         * @return String containing file contents.
         */
        std::string readFileRomfs(const char* path);

        /**
         * @brief Reads a binary asset file into a byte vector.
         * @param path Path to the target file.
         * @return Vector of unsigned bytes containing file data.
         */
        std::vector<uint8_t> readFileBinaryRomfs(const char* path);

    private:
        /** @brief Private default constructor for singleton pattern. */
        Filesystem() = default;
    };
}
