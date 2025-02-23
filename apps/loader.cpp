#include <block_cache/file_handler.hpp>
#include <cerrno>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

int readFileWithMRU(const char *path) {
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
  ssize_t total_bytes = 0;

  for (int it = 0; it < file_size; it += BUFFER_SIZE) {
    ssize_t n =
        mru_read(handle, buf,
                 std::min(static_cast<size_t>(sizeof(buf)),
                          static_cast<size_t>(file_size - total_bytes)));

    if (n < 0) {
      std::cerr << "Failed to read file (errno: " << errno << ")" << std::endl;
      mru_close(handle);
      return 1;
    }

    total_bytes += n;
  }

  ssize_t bytes_read = total_bytes;

  int res = mru_close(handle);
  if (res < 0) {
    std::cerr << "Failed to close file (errno: " << errno << ")" << std::endl;
    return 1;
  }

  return 0;
}

int readFileWithoutMRU(const char *path) {
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

  for (int it = 0; it < file_size; it += BUFFER_SIZE) {
    ssize_t n = read(handle, buf,
                     std::min(static_cast<size_t>(sizeof(buf)),
                              static_cast<size_t>(file_size - total_bytes)));

    if (n < 0) {
      std::cerr << "Failed to read file (errno: " << errno << ")" << std::endl;
      close(handle);
      return 1;
    }

    total_bytes += n;
  }

  ssize_t bytes_read = total_bytes;

  int res = close(handle);
  if (res < 0) {
    std::cerr << "Failed to close file (errno: " << errno << ")" << std::endl;
    return 1;
  }

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
  for (int it = 0; it < iterations; ++it) {
    if (readFileWithMRU("test_data.bin") != 0) {
      return 1;
    }
  }
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed_seconds = end - now;
  std::cout << "Time taken with MRU: " << elapsed_seconds.count() << "s"
            << std::endl;

  now = std::chrono::high_resolution_clock::now();
  for (int it = 0; it < iterations; ++it) {
    if (readFileWithoutMRU("test_data.bin") != 0) {
      return 1;
    }
  }
  end = std::chrono::high_resolution_clock::now();
  elapsed_seconds = end - now;
  std::cout << "Time taken with default caching: " << elapsed_seconds.count()
            << "s" << std::endl;

  return 0;
}