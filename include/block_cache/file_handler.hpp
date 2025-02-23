#pragma once

#include <basetsd.h>
#include <block_cache/block_cache_export.h>
#include <block_cache/mru_policy.hpp>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <mutex>
#include <sys/stat.h>
#include <sys/types.h>
#include <unordered_map>
#include <vector>

#ifndef O_DIRECT
#define O_DIRECT 0
#endif

#ifdef _MSC_VER
typedef SSIZE_T ssize_t;
#endif

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

extern "C" {

static const size_t BLOCK_SIZE = 4096;
static const size_t CACHE_CAPACITY = 1024;

struct FileContext {
  int fd;
  off_t offset;
  block_cache::MRUCache cache;
  std::unordered_map<uint64_t, bool> dirty_blocks;

  FileContext(int fd_)
      : fd(fd_), offset(0), cache(CACHE_CAPACITY, BLOCK_SIZE) {}
};

BLOCK_CACHE_EXPORT int mru_open(const char *path);

BLOCK_CACHE_EXPORT int mru_close(int handle);

BLOCK_CACHE_EXPORT off_t mru_lseek(int handle, off_t offset, int whence);

BLOCK_CACHE_EXPORT ssize_t mru_read(int handle, void *buf, size_t count);

BLOCK_CACHE_EXPORT ssize_t mru_write(int handle, const void *buf, size_t count);

BLOCK_CACHE_EXPORT int mru_fsync(int handle);

} // extern "C"