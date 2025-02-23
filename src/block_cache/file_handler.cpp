#include <block_cache/file_handler.hpp>
#include <iostream>

static std::unordered_map<int, FileContext *> g_file_map;
static int next_handle = 3;
static std::mutex g_file_mutex;
static std::mutex g_file_mutex_fsync;

static inline ssize_t pread(int fd, void *buf, size_t count, off_t offset) {
  off_t current = _lseek(fd, 0, SEEK_CUR);
  if (_lseek(fd, offset, SEEK_SET) == -1)
    return -1;
  int res = _read(fd, buf, count);
  _lseek(fd, current, SEEK_SET);
  return res;
}

static inline ssize_t pwrite(int fd, const void *buf, size_t count,
                             off_t offset) {
  off_t current = _lseek(fd, 0, SEEK_CUR);
  if (_lseek(fd, offset, SEEK_SET) == -1)
    return -1;
  int res = _write(fd, static_cast<const char *>(buf), count);
  _lseek(fd, current, SEEK_SET);
  return res;
}

int mru_open(const char *path) {
  if (!path)
    return -1;
  int fd = open(path, O_RDWR | O_DIRECT);
  if (fd < 0) {
    return -1;
  }
  FileContext *ctx = new FileContext(fd);
  std::lock_guard<std::mutex> lock(g_file_mutex);
  int handle = next_handle++;
  g_file_map[handle] = ctx;
  return handle;
}

int mru_close(int handle) {
  std::lock_guard<std::mutex> lock(g_file_mutex);
  auto it = g_file_map.find(handle);
  if (it == g_file_map.end())
    return -1;
  FileContext *ctx = it->second;
  mru_fsync(handle);
  close(ctx->fd);
  delete ctx;
  g_file_map.erase(it);
  return 0;
}

off_t mru_lseek(int handle, off_t offset, int whence) {
  std::lock_guard<std::mutex> lock(g_file_mutex);
  auto it = g_file_map.find(handle);
  if (it == g_file_map.end())
    return -1;
  FileContext *ctx = it->second;
  off_t new_offset = 0;
  if (whence == SEEK_SET) {
    new_offset = offset;
  } else if (whence == SEEK_CUR) {
    new_offset = ctx->offset + offset;
  } else if (whence == SEEK_END) {
    struct stat st;
    if (fstat(ctx->fd, &st) < 0)
      return -1;
    new_offset = st.st_size + offset;
  } else {
    return -1;
  }
  ctx->offset = new_offset;
  return new_offset;
}

ssize_t mru_read(int handle, void *buf, size_t count) {
  if (buf == nullptr)
    return -1;
  std::lock_guard<std::mutex> lock(g_file_mutex);
  auto it = g_file_map.find(handle);
  if (it == g_file_map.end())
    return -1;
  FileContext *ctx = it->second;

  size_t bytes_read = 0;
  char *out = static_cast<char *>(buf);

  while (count > 0) {
    uint64_t block_num = ctx->offset / BLOCK_SIZE;
    size_t block_offset = ctx->offset % BLOCK_SIZE;
    size_t bytes_to_copy = std::min(BLOCK_SIZE - block_offset, count);

    std::vector<char> block(BLOCK_SIZE);
    if (!ctx->cache.get(block_num, block.data())) {
      ssize_t res =
          pread(ctx->fd, block.data(), BLOCK_SIZE, block_num * BLOCK_SIZE);
      if (res < 0)
        return -1;
      ctx->cache.put(block_num, block.data());
    } else {
      ctx->cache.get(block_num, block.data());
    }

    std::memcpy(out, block.data() + block_offset, bytes_to_copy);
    out += bytes_to_copy;
    ctx->offset += bytes_to_copy;
    bytes_read += bytes_to_copy;
    count -= bytes_to_copy;
  }

  return bytes_read;
}

ssize_t mru_write(int handle, const void *buf, size_t count) {
  if (buf == nullptr)
    return -1;
  std::lock_guard<std::mutex> lock(g_file_mutex);
  auto it = g_file_map.find(handle);
  if (it == g_file_map.end())
    return -1;
  FileContext *ctx = it->second;

  size_t bytes_written = 0;
  const char *in = static_cast<const char *>(buf);

  while (count > 0) {
    uint64_t block_num = ctx->offset / BLOCK_SIZE;
    size_t block_offset = ctx->offset % BLOCK_SIZE;
    size_t bytes_to_copy = std::min(BLOCK_SIZE - block_offset, count);

    std::vector<char> block(BLOCK_SIZE);
    if (!ctx->cache.get(block_num, block.data())) {
      ssize_t res =
          pread(ctx->fd, block.data(), BLOCK_SIZE, block_num * BLOCK_SIZE);
      if (res < 0)
        return -1;
    }

    std::memcpy(block.data() + block_offset, in, bytes_to_copy);
    ctx->cache.put(block_num, block.data());
    ctx->dirty_blocks[block_num] = true;

    in += bytes_to_copy;
    ctx->offset += bytes_to_copy;
    bytes_written += bytes_to_copy;
    count -= bytes_to_copy;
  }

  return bytes_written;
}

int mru_fsync(int handle) {
  std::lock_guard<std::mutex> lock(g_file_mutex_fsync);
  auto it = g_file_map.find(handle);
  if (it == g_file_map.end())
    return -1;
  FileContext *ctx = it->second;

  for (const auto &pair : ctx->dirty_blocks) {
    uint64_t block_num = pair.first;
    std::vector<char> block(BLOCK_SIZE);
    if (ctx->cache.get(block_num, block.data())) {
      ssize_t res =
          pwrite(ctx->fd, block.data(), BLOCK_SIZE, block_num * BLOCK_SIZE);
      if (res < 0)
        return -1;
    }
  }
  ctx->dirty_blocks.clear();

#ifdef _WIN32
  return _commit(ctx->fd);
#else
  return fsync(ctx->fd);
#endif
}
