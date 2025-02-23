#pragma once

#include <block_cache/block_cache_export.h>
#include <cstdint>
#include <list>
#include <unordered_map>
#include <vector>

namespace block_cache {

/**
 * @class MRUCache
 *
 * @brief A Most Recently Used (MRU) cache implementation.
 *
 * The MRUCache class provides an in-memory cache with a Most Recently Used
 * (MRU) eviction policy. When the cache reaches its capacity, it evicts the
 * entry that was most recently used.
 */
class BLOCK_CACHE_EXPORT MRUCache {
public:
  /**
   * @brief Constructs an MRU cache with the specified capacity and block size.
   *
   * This constructor initializes an MRU cache with the specified capacity and
   * block size. The capacity determines the maximum number of entries that the
   * cache can hold, while the block size specifies the size of each entry in
   * bytes.
   *
   * @param capacity An integer representing the maximum number of entries that
   * the cache can hold.
   * @param blockSize An integer representing the size of each entry in bytes.
   */
  MRUCache(size_t capacity, size_t blockSize);

  /**
   * @brief Retrieves data from the cache given a key.
   *
   * This method retrieves data from the cache based on the provided key. If the
   * key is found in the cache, the corresponding data is copied to the buffer
   * pointed to by data. A successful call returns true, while a failure returns
   * false.
   *
   * @param key An unsigned 64-bit integer representing the key of the entry to
   * be retrieved.
   * @param data A pointer to the buffer where the data will be copied.
   * @return A boolean value indicating the success of the operation. A value of
   * true indicates success, while false indicates failure.
   */
  bool get(uint64_t key, void *data);

  /**
   * @brief Inserts data into the cache with the specified key.
   *
   * This method inserts data into the cache with the specified key. If the key
   * already exists in the cache, the data is updated. If the cache reaches its
   * capacity, the entry that was most recently used is evicted. A successful
   * call results in the data being stored in the cache.
   *
   * @param key An unsigned 64-bit integer representing the key of the entry to
   * be inserted.
   * @param data A pointer to the data to be inserted into the cache.
   */
  void put(uint64_t key, const void *data);

private:
  size_t capacity;
  size_t blockSize;
  std::list<uint64_t> mru_list;
  std::unordered_map<uint64_t, std::list<uint64_t>::iterator> cache_map;
  std::unordered_map<uint64_t, std::vector<char>> data_map;
};

} // namespace block_cache