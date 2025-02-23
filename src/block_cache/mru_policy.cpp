#include <algorithm>
#include <block_cache/mru_policy.hpp>
#include <cstring>

namespace block_cache {

MRUCache::MRUCache(size_t capacity, size_t blockSize)
    : capacity(capacity), blockSize(blockSize) {}

bool MRUCache::get(uint64_t key, void *data) {
  auto it = cache_map.find(key);
  if (it == cache_map.end()) {
    return false;
  }

  mru_list.erase(it->second);
  mru_list.push_front(key);
  it->second = mru_list.begin();

  auto dataIt = data_map.find(key);
  if (dataIt != data_map.end() && data != nullptr) {
    std::memcpy(data, dataIt->second.data(), blockSize);
  }

  return true;
}

void MRUCache::put(uint64_t key, const void *data) {
  auto it = cache_map.find(key);
  if (it != cache_map.end()) {
    mru_list.erase(it->second);
    mru_list.push_front(key);
    it->second = mru_list.begin();

    if (data != nullptr) {
      data_map[key].resize(blockSize);
      std::memcpy(data_map[key].data(), data, blockSize);
    }
    return;
  }

  if (mru_list.size() >= capacity) {
    uint64_t evict_key = mru_list.front();
    mru_list.pop_front();
    cache_map.erase(evict_key);
    data_map.erase(evict_key);
  }

  mru_list.push_front(key);
  cache_map[key] = mru_list.begin();

  if (data != nullptr) {
    std::vector<char> block(blockSize);
    std::memcpy(block.data(), data, blockSize);
    data_map[key] = std::move(block);
  } else {
    data_map[key] = std::vector<char>(blockSize);
  }
}

} // namespace block_cache