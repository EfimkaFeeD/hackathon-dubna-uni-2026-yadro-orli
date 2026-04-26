#ifndef HACATON_OBJECTSTORAGE_H
#define HACATON_OBJECTSTORAGE_H

#include <unordered_map>
#include <string>
#include <vector>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <mutex>
#include <optional>

class objectStorage {
private:
    std::filesystem::path basePath;
    mutable std::mutex mtx;

    std::unordered_map<std::string, std::vector<std::byte>> cache;
    size_t maxCacheSize;
    size_t currentCacheSize;

    void ensureDirectoryExists();
    std::filesystem::path getFilePath(size_t fileId, const std::string& key) const;
    std::string makeCacheKey(size_t fileId, const std::string& key) const;
    void addToCache(size_t fileId, const std::string& key, const std::vector<std::byte>& data);
    void evictIfNeeded();

public:
    explicit objectStorage(const std::string& path = "storage", size_t cacheLimit = 1024 * 1024 * 100);

    bool save(size_t fileId, const std::string& key, const std::vector<std::byte>& data);
    std::optional<std::vector<std::byte>> load(size_t fileId, const std::string& key);
    bool remove(size_t fileId, const std::string& key);
    bool exists(size_t fileId, const std::string& key) const;

    bool removeAllForFile(size_t fileId);
    std::vector<std::string> listKeysForFile(size_t fileId) const;
    size_t getFileSize(size_t fileId) const;

    std::vector<size_t> listAllFileIds() const;
    void clearCache();
};

#endif //HACATON_OBJECTSTORAGE_H