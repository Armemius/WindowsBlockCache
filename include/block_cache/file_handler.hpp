#pragma once

#include <block_cache/block_cache_export.h>
#include <mutex>
#include <unordered_map>
#include <windows.h>

#ifdef _MSC_VER
typedef SSIZE_T ssize_t;
#endif

namespace block_cache {

struct BLOCK_CACHE_EXPORT FileHandle {
  HANDLE handle;
  uint64_t position;
};

/**
 * @class FileHandleManager
 * @brief Manages file handles for file operations.
 *
 * The FileHandleManager class encapsulates functions for file handling such as
 * opening, closing, reading, writing, seeking, and syncing file descriptors. It
 * manages an internal mapping between file descriptors and corresponding file
 * handle objects.
 *
 * The class provides thread-safe access to file operations using a mutex. File
 * descriptors start from 3, and the manager assigns them incrementally.
 *
 */
class BLOCK_CACHE_EXPORT FileHandleManager {
public:
  FileHandleManager() = default;

  /**
   * @brief Opens a file given its file path.
   *
   * This method attempts to open the file specified by the provided path and
   * returns an associated file descriptor. A successful call results in a
   * unique file descriptor which can be used with subsequent operations, while
   * a failure returns a negative value.
   *
   * @param path A pointer to a null-terminated string that represents the
   * location of the file to be opened.
   * @return An integer representing the file descriptor if successful;
   * otherwise, a negative value indicating an error.
   */
  int open(const char *path);

  /**
   * @brief Closes a file given its file descriptor.
   *
   * This method closes the file associated with the provided file descriptor.
   * A successful call returns 0, while a failure returns a negative value.
   *
   * @param fd An integer representing the file descriptor of the file to be
   * closed.
   * @return An integer indicating the status of the operation. A value of 0
   * indicates success, while a negative value indicates an error.
   */
  int close(int fd);

  /**
   * @brief Reads data from a file given its file descriptor.
   *
   * This method reads data from the file associated with the provided file
   * descriptor and stores it in the buffer pointed to by buf. The number of
   * bytes read is determined by the count parameter. A successful call returns
   * the number of bytes read, while a failure returns a negative value.
   *
   * @param fd An integer representing the file descriptor of the file to be
   * read from.
   * @param buf A pointer to the buffer where the data read from the file will
   * be stored.
   * @param count An integer representing the number of bytes to be read.
   * @return An integer indicating the number of bytes read if successful;
   * otherwise, a negative value indicating an error.
   */
  ssize_t read(int fd, void *buf, size_t count);

  /**
   * @brief Writes data to a file given its file descriptor.
   *
   * This method writes data from the buffer pointed to by buf to the file
   * associated with the provided file descriptor. The number of bytes written
   * is determined by the count parameter. A successful call returns the number
   * of bytes written, while a failure returns a negative value.
   *
   * @param fd An integer representing the file descriptor of the file to be
   * written to.
   * @param buf A pointer to the buffer containing the data to be written to the
   * file.
   * @param count An integer representing the number of bytes to be written.
   * @return An integer indicating the number of bytes written if successful;
   * otherwise, a negative value indicating an error.
   */
  ssize_t write(int fd, const void *buf, size_t count);

  /**
   * @brief Changes the file offset of a file given its file descriptor.
   *
   * This method changes the file offset of the file associated with the
   * provided file descriptor based on the offset and whence parameters. A
   * successful call returns the new file offset, while a failure returns a
   * negative value.
   *
   * @param fd An integer representing the file descriptor of the file to be
   * operated on.
   * @param offset An integer representing the offset to be applied.
   * @param whence An integer representing the reference point for the offset.
   * @return An integer indicating the new file offset if successful; otherwise,
   * a negative value indicating an error.
   */
  off_t lseek(int fd, off_t offset, int whence);

  /**
   * @brief Synchronizes the file data to disk given its file descriptor.
   *
   * This method synchronizes the file data to disk for the file associated with
   * the provided file descriptor. A successful call returns 0, while a failure
   * returns a negative value.
   *
   * @param fd An integer representing the file descriptor of the file to be
   * synced.
   * @return An integer indicating the status of the operation. A value of 0
   * indicates success, while a negative value indicates an error.
   */
  int fsync(int fd);

private:
  std::mutex mtx;
  std::unordered_map<int, FileHandle> handles;
  int next_fd = 3;
};

} // namespace block_cache
