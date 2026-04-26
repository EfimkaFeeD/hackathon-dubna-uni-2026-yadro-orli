#include "objectStorage.h"
#include <sstream>

objectStorage::objectStorage(const std::string& path, size_t cacheLimit)
        : basePath(path), maxCacheSize(cacheLimit), currentCacheSize(0) {
    ensureDirectoryExists();
}

void objectStorage::ensureDirectoryExists() {
    try {
        if (!std::filesystem::exists(basePath)) {
            std::filesystem::create_directories(basePath);
            std::cout << "Storage directory created: " << basePath << std::endl;
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Failed to create storage directory: " << e.what() << std::endl;
    }
}

std::string objectStorage::makeCacheKey(size_t fileId, const std::string& key) const {
    return std::to_string(fileId) + "/" + key;
}

std::filesystem::path objectStorage::getFilePath(size_t fileId, const std::string& key) const {
    return basePath / std::to_string(fileId) / (key + ".bin");
}

void objectStorage::addToCache(size_t fileId, const std::string& key, const std::vector<std::byte>& data) {
    currentCacheSize += data.size();
    cache[makeCacheKey(fileId, key)] = data;
    evictIfNeeded();
}

void objectStorage::evictIfNeeded() {
    if (currentCacheSize <= maxCacheSize) return;

    size_t toRemove = currentCacheSize / 2;
    size_t removed = 0;

    for (auto it = cache.begin(); it != cache.end() && removed < toRemove;) {
        removed += it->second.size();
        it = cache.erase(it);
    }
    currentCacheSize -= removed;
}

bool objectStorage::save(size_t fileId, const std::string& key, const std::vector<std::byte>& data) {
    if (data.empty()) {
        std::cerr << "Cannot save empty data for: " << fileId << "/" << key << std::endl;
        return false;
    }

    std::lock_guard<std::mutex> lock(mtx);

    auto filePath = getFilePath(fileId, key);

    try {
        std::filesystem::create_directories(filePath.parent_path());
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Failed to create directory: " << e.what() << std::endl;
        return false;
    }

    std::ofstream file(filePath, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file: " << filePath << std::endl;
        return false;
    }

    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    if (!file.good()) {
        std::cerr << "Failed to write data to: " << filePath << std::endl;
        return false;
    }
    file.close();

    if (data.size() < 1024 * 1024) {
        addToCache(fileId, key, data);
    }

    std::cout << "Saved: " << fileId << "/" << key << " (" << data.size() << " bytes)" << std::endl;
    return true;
}

std::optional<std::vector<std::byte>> objectStorage::load(size_t fileId, const std::string& key) {
    std::lock_guard<std::mutex> lock(mtx);

    std::string cacheKey = makeCacheKey(fileId, key);

    auto cacheIt = cache.find(cacheKey);
    if (cacheIt != cache.end()) {
        return cacheIt->second;
    }

    auto filePath = getFilePath(fileId, key);
    if (!std::filesystem::exists(filePath)) {
        std::cerr << "Object not found: " << fileId << "/" << key << std::endl;
        return std::nullopt;
    }

    std::error_code ec;
    auto fileSize = std::filesystem::file_size(filePath, ec);
    if (ec) {
        std::cerr << "Failed to get file size: " << filePath << std::endl;
        return std::nullopt;
    }

    if (fileSize == 0) {
        std::cerr << "File is empty: " << filePath << std::endl;
        return std::nullopt;
    }

    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file: " << filePath << std::endl;
        return std::nullopt;
    }

    std::vector<std::byte> buffer(fileSize);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

    if (!file) {
        std::cerr << "Failed to read file: " << filePath << std::endl;
        return std::nullopt;
    }

    file.close();

    if (buffer.size() < 1024 * 1024) {
        addToCache(fileId, key, buffer);
    }

    std::cout << "Loaded: " << fileId << "/" << key << " (" << buffer.size() << " bytes)" << std::endl;
    return buffer;
}

bool objectStorage::remove(size_t fileId, const std::string& key) {
    std::lock_guard<std::mutex> lock(mtx);

    std::string cacheKey = makeCacheKey(fileId, key);

    auto cacheIt = cache.find(cacheKey);
    if (cacheIt != cache.end()) {
        currentCacheSize -= cacheIt->second.size();
        cache.erase(cacheIt);
    }

    auto filePath = getFilePath(fileId, key);
    if (std::filesystem::exists(filePath)) {
        std::error_code ec;
        if (!std::filesystem::remove(filePath, ec)) {
            std::cerr << "Failed to delete: " << filePath << ", error: " << ec.message() << std::endl;
            return false;
        }

        auto dirPath = filePath.parent_path();
        try {
            if (std::filesystem::exists(dirPath) &&
                std::filesystem::is_directory(dirPath) &&
                std::filesystem::is_empty(dirPath)) {
                std::filesystem::remove(dirPath);
                std::cout << "Removed empty directory: " << dirPath << std::endl;
            }
        } catch (const std::filesystem::filesystem_error& e) {
            std::cout << "Directory not empty, keeping: " << dirPath << std::endl;
        }
    }

    std::cout << "Deleted: " << fileId << "/" << key << std::endl;
    return true;
}

bool objectStorage::removeAllForFile(size_t fileId) {
    std::lock_guard<std::mutex> lock(mtx);

    auto dirPath = basePath / std::to_string(fileId);

    if (!std::filesystem::exists(dirPath)) {
        return true;
    }

    std::string prefix = std::to_string(fileId) + "/";
    for (auto it = cache.begin(); it != cache.end();) {
        if (it->first.find(prefix) == 0) {
            currentCacheSize -= it->second.size();
            it = cache.erase(it);
        } else {
            ++it;
        }
    }

    std::error_code ec;
    std::filesystem::remove_all(dirPath, ec);

    if (ec) {
        std::cerr << "Failed to remove directory: " << dirPath << ", error: " << ec.message() << std::endl;
        return false;
    }

    std::cout << "Deleted all objects for fileId: " << fileId << std::endl;
    return true;
}

bool objectStorage::exists(size_t fileId, const std::string& key) const {
    std::lock_guard<std::mutex> lock(mtx);

    std::string cacheKey = makeCacheKey(fileId, key);
    if (cache.find(cacheKey) != cache.end()) {
        return true;
    }

    auto filePath = getFilePath(fileId, key);
    return std::filesystem::exists(filePath);
}

std::vector<std::string> objectStorage::listKeysForFile(size_t fileId) const {
    std::lock_guard<std::mutex> lock(mtx);
    std::vector<std::string> keys;

    auto dirPath = basePath / std::to_string(fileId);
    if (!std::filesystem::exists(dirPath)) {
        return keys;
    }

    for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
        if (entry.path().extension() == ".bin") {
            keys.push_back(entry.path().stem().string());
        }
    }

    return keys;
}

size_t objectStorage::getFileSize(size_t fileId) const {
    std::lock_guard<std::mutex> lock(mtx);
    size_t total = 0;

    auto dirPath = basePath / std::to_string(fileId);
    if (!std::filesystem::exists(dirPath)) {
        return 0;
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(dirPath)) {
        if (entry.is_regular_file()) {
            total += entry.file_size();
        }
    }

    return total;
}

std::vector<size_t> objectStorage::listAllFileIds() const {
    std::lock_guard<std::mutex> lock(mtx);
    std::vector<size_t> ids;

    for (const auto& entry : std::filesystem::directory_iterator(basePath)) {
        if (entry.is_directory()) {
            try {
                size_t id = std::stoull(entry.path().filename().string());
                ids.push_back(id);
            } catch (...) {
            }
        }
    }

    return ids;
}

void objectStorage::clearCache() {
    std::lock_guard<std::mutex> lock(mtx);
    cache.clear();
    currentCacheSize = 0;
    std::cout << "Cache cleared" << std::endl;
}