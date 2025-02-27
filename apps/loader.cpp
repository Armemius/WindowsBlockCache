#include <block_cache/file_handler.hpp>
#include <cerrno>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <wtypes.h>

#undef min

int readFileWithMRU(const char *path, size_t iterations = 1) {
  int handle = mru_open(path);
  if (handle < 0) {
    std::cerr << "Failed to open file (errno: " << errno << ")" << std::endl;
    return 1;
  }

  off_t current_pos = mru_lseek(handle, 0, SEEK_CUR);
  off_t file_size = mru_lseek(handle, 0, SEEK_END);
  mru_lseek(handle, current_pos, SEEK_SET);

  const int BUFFER_SIZE = 4096;

  char buf[BUFFER_SIZE] = {0};

  for (size_t it = 0; it < iterations; ++it) {
    ssize_t total_bytes = 0;

    for (size_t jt = 0; jt < file_size; jt += BUFFER_SIZE) {
      ssize_t n =
          mru_read(handle, buf,
                   std::min(static_cast<size_t>(sizeof(buf)),
                            static_cast<size_t>(file_size - total_bytes)));

      if (n < 0) {
        std::cerr << "Failed to read file (errno: " << errno << ")"
                  << std::endl;
        mru_close(handle);
        return 1;
      }

      total_bytes += n;
    }
    mru_lseek(handle, 0, SEEK_SET);
  }

  int res = mru_close(handle);
  if (res < 0) {
    std::cerr << "Failed to close file (errno: " << errno << ")" << std::endl;
    return 1;
  }

  return 0;
}

int readFileWithoutMRU(const char *path, size_t iterations = 1) {
  int handle = open(path, O_RDONLY);
  if (handle < 0) {
    std::cerr << "Failed to open file (errno: " << errno << ")" << std::endl;
    return 1;
  }

  off_t current_pos = lseek(handle, 0, SEEK_CUR);
  off_t file_size = lseek(handle, 0, SEEK_END);
  lseek(handle, current_pos, SEEK_SET);

  const int BUFFER_SIZE = 4096;

  char buf[BUFFER_SIZE] = {0};
  ssize_t total_bytes = 0;

  for (size_t it = 0; it < iterations; ++it) {
    for (size_t jt = 0; jt < file_size; jt += BUFFER_SIZE) {
      ssize_t n = read(handle, buf,
                       std::min(static_cast<size_t>(sizeof(buf)),
                                static_cast<size_t>(file_size - total_bytes)));

      if (n < 0) {
        std::cerr << "Failed to read file (errno: " << errno << ")"
                  << std::endl;
        close(handle);
        return 1;
      }

      total_bytes += n;
    }
    lseek(handle, 0, SEEK_SET);
  }

  ssize_t bytes_read = total_bytes;

  int res = close(handle);
  if (res < 0) {
    std::cerr << "Failed to close file (errno: " << errno << ")" << std::endl;
    return 1;
  }

  return 0;
}

int readFileNoCache(const char *path, size_t iterations = 1) {
#ifdef _WIN32
  HANDLE file =
      CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING |
                      FILE_FLAG_SEQUENTIAL_SCAN,
                  NULL);
  if (file == INVALID_HANDLE_VALUE) {
    std::cerr << "Failed to open file without cache" << std::endl;
    return 1;
  }

  const DWORD blockSize = 4096;
  void *buffer = _aligned_malloc(blockSize, blockSize);
  if (!buffer) {
    CloseHandle(file);
    return 1;
  }

  for (size_t i = 0; i < iterations; ++i) {
    LARGE_INTEGER offset = {};
    SetFilePointerEx(file, offset, NULL, FILE_BEGIN);
    DWORD bytesRead = 0;
    while (ReadFile(file, buffer, blockSize, &bytesRead, NULL) &&
           bytesRead > 0) {
      volatile char tmp = 0;
      for (DWORD i = 0; i < bytesRead; ++i) {
        tmp ^= static_cast<char *>(buffer)[i];
      }
    }
  }

  _aligned_free(buffer);
  CloseHandle(file);
#else
  int fd = open(path, O_RDONLY | O_DIRECT);
  if (fd < 0) {
    std::cerr << "Failed to open file without cache" << std::endl;
    return 1;
  }

  size_t alignment = 4096;
  void *buffer = nullptr;
  if (posix_memalign(&buffer, alignment, alignment) != 0) {
    close(fd);
    return 1;
  }

  for (size_t i = 0; i < iterations; ++i) {
    lseek(fd, 0, SEEK_SET);
    ssize_t bytesRead = 0;
    while ((bytesRead = read(fd, buffer, alignment)) > 0) {
      volatile char tmp = 0;
      for (ssize_t i = 0; i < bytesRead; ++i) {
        tmp ^= static_cast<char *>(buffer)[i];
      }
    }
  }

  free(buffer);
  close(fd);
#endif
  return 0;
}

int generateFile(const char *path, size_t size) {
  std::ofstream outfile(path, std::ios::binary | std::ios::trunc);
  if (!outfile) {
    std::cerr << "Failed to create file" << std::endl;
    return 1;
  }
  const size_t BUFFER_SIZE = 4096;
  std::vector<char> buffer(BUFFER_SIZE, 0);
  size_t bytesRemaining = size;
  while (bytesRemaining > 0) {
    size_t bytesToWrite = std::min(BUFFER_SIZE, bytesRemaining);
    outfile.write(buffer.data(), bytesToWrite);
    if (!outfile) {
      std::cerr << "Failed to write to file" << std::endl;
      return 1;
    }
    bytesRemaining -= bytesToWrite;
  }
  outfile.close();

  return 0;
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " <iterations> <file size in bytes>"
              << std::endl;
    return 1;
  }
  int iterations = std::stoi(argv[1]);
  const int file_size = std::stoi(argv[2]);
  generateFile("test_data.bin", file_size);

  auto now = std::chrono::high_resolution_clock::now();
  readFileWithMRU("test_data.bin", iterations);
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed_seconds = end - now;
  std::cout << "Time taken with MRU: " << elapsed_seconds.count() << "s"
            << std::endl;

  now = std::chrono::high_resolution_clock::now();
  readFileWithoutMRU("test_data.bin", iterations);
  end = std::chrono::high_resolution_clock::now();
  elapsed_seconds = end - now;
  std::cout << "Time taken with default caching: " << elapsed_seconds.count()
            << "s" << std::endl;

  now = std::chrono::high_resolution_clock::now();
  readFileNoCache("test_data.bin", iterations);
  end = std::chrono::high_resolution_clock::now();
  elapsed_seconds = end - now;
  std::cout << "Time taken without caching: " << elapsed_seconds.count() << "s"
            << std::endl;

  return 0;
}