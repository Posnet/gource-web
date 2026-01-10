// === File: src/core/resource.cpp ==============================================
// AGENT: PURPOSE    — Resource management without boost::filesystem
// AGENT: STATUS     — in-progress (2026-01-09)
// =============================================================================

#include "resource.h"
#include <sys/stat.h>

ResourceManager::ResourceManager() {
}

std::string ResourceManager::getDir() {
    return resource_dir;
}

void ResourceManager::setDir(const std::string& resource_dir) {
    this->resource_dir = resource_dir;
}

void ResourceManager::purge() {
    for (auto& pair : resources) {
        delete pair.second;
    }
    resources.clear();
}

ResourceManager::~ResourceManager() {
}

bool ResourceManager::fileExists(const std::string& filename) {
    struct stat st;
    if (stat(filename.c_str(), &st) != 0) return false;
    return S_ISREG(st.st_mode);
}

bool ResourceManager::dirExists(const std::string& dirname) {
    struct stat st;
    if (stat(dirname.c_str(), &st) != 0) return false;
    return S_ISDIR(st.st_mode);
}

void ResourceManager::release(Resource* resource) {
    Resource* r = resources[resource->resource_name];
    if (r == nullptr) return;

    r->deref();

    if (r->refcount() <= 0) {
        resources.erase(r->resource_name);
        delete r;
    }
}
