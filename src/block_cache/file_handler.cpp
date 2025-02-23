#include <block_cache/file_handler.hpp>
#include <iostream>
#include <stdexcept>

int block_cache::FileHandleManager::open(const char *path) {
  std::lock_guard<std::mutex> lock(mtx);
  HANDLE handle = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (handle == INVALID_HANDLE_VALUE) {
    return -1;
  }
  int fd = next_fd++;
  handles[fd] = {handle, 0};
  return fd;
}

int block_cache::FileHandleManager::close(int fd) {
  std::lock_guard<std::mutex> lock(mtx);
  auto it = handles.find(fd);
  if (it == handles.end()) {
    return -1;
  }
  CloseHandle(it->second.handle);
  handles.erase(it);
  return 0;
}

ssize_t block_cache::FileHandleManager::read(int fd, void *buf, size_t count) {
  std::lock_guard<std::mutex> lock(mtx);
  auto it = handles.find(fd);
  if (it == handles.end()) {
    return -1;
  }
  DWORD bytesRead;
  if (!ReadFile(it->second.handle, buf, static_cast<DWORD>(count), &bytesRead,
                NULL)) {
    return -1;
  }
  it->second.position += bytesRead;
  return bytesRead;
}

ssize_t block_cache::FileHandleManager::write(int fd, const void *buf,
                                              size_t count) {
  std::lock_guard<std::mutex> lock(mtx);
  auto it = handles.find(fd);
  if (it == handles.end()) {
    return -1;
  }
  DWORD bytesWritten;
  if (!WriteFile(it->second.handle, buf, static_cast<DWORD>(count),
                 &bytesWritten, NULL)) {
    return -1;
  }
  it->second.position += bytesWritten;
  return bytesWritten;
}

off_t block_cache::FileHandleManager::lseek(int fd, off_t offset, int whence) {
  std::lock_guard<std::mutex> lock(mtx);
  auto it = handles.find(fd);
  if (it == handles.end()) {
    return -1;
  }
  DWORD moveMethod;
  switch (whence) {
  case SEEK_SET:
    moveMethod = FILE_BEGIN;
    break;
  case SEEK_CUR:
    moveMethod = FILE_CURRENT;
    break;
  case SEEK_END:
    moveMethod = FILE_END;
    break;
  default:
    return -1;
  }
  LARGE_INTEGER li;
  li.QuadPart = offset;
  li.LowPart =
      SetFilePointer(it->second.handle, li.LowPart, &li.HighPart, moveMethod);
  if (li.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR) {
    return -1;
  }
  it->second.position = li.QuadPart;
  return li.QuadPart;
}

int block_cache::FileHandleManager::fsync(int fd) {
  std::lock_guard<std::mutex> lock(mtx);
  auto it = handles.find(fd);
  if (it == handles.end()) {
    return -1;
  }
  if (!FlushFileBuffers(it->second.handle)) {
    return -1;
  }
  return 0;
}