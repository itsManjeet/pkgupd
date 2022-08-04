#include <byteswap.h>
#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <squashfuse.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <iostream>
#include <memory>

#define SELF "/proc/self/exe"

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define ELFDATANATIVE ELFDATA2LSB
#elif __BYTE_ORDER == __BIG_ENDIAN
#define ELFDATANATIVE ELFDATA2MSB
#else
#error "unknown machine endian"
#endif

class Elf {
 private:
  Elf64_Ehdr mEhdr;
  Elf64_Phdr* mPhdr;
  int mFd;
  std::string mError;

  uint64_t parse64(uint64_t value) {
    if (mEhdr.e_ident[EI_DATA] != ELFDATANATIVE) {
      value = bswap_64(value);
    }
    return value;
  }

 public:
  Elf(std::string filepath) {
    mFd = open(filepath.c_str(), O_RDONLY);
    if (mFd == -1) {
      throw std::runtime_error("cannot open " + filepath + ", " +
                               std::string(strerror(errno)));
    }
  }
  ~Elf() {
    if (mFd < 0) close(mFd);
  }

  std::string error() const { return mError; }

  size_t size() {
    ssize_t ret = pread(mFd, mEhdr.e_ident, EI_NIDENT, 0);
    if (ret != EI_NIDENT) {
      mError = "EI_NIDENT read failure " + std::string(strerror(errno));
      return 1;
    }

    if ((mEhdr.e_ident[EI_DATA] != ELFDATA2LSB) &&
        (mEhdr.e_ident[EI_DATA] != ELFDATA2MSB)) {
      mError = "unknown EI_NIDENT[EI_DATA] data order " +
               std::to_string(mEhdr.e_ident[EI_DATA]);
      return 1;
    }

    if (mEhdr.e_ident[EI_CLASS] == ELFCLASS32) {
      // TODO: add support for 32bit
      mError = "elf32 is not yet supported";
      return 1;
    } else if (mEhdr.e_ident[EI_CLASS] == ELFCLASS64) {
      Elf64_Ehdr ehdr64;
      ssize_t ret, i;

      ret = pread(mFd, &ehdr64, sizeof(ehdr64), 0);
      if (ret < 0 || (size_t)ret != sizeof(mEhdr)) {
        mError = "Fail to read ELF header " + std::string(strerror(errno));
        return 1;
      }

      mEhdr.e_shoff = parse64(ehdr64.e_shoff);
      mEhdr.e_shentsize = parse64(ehdr64.e_shentsize);
      mEhdr.e_shnum = parse64(ehdr64.e_shnum);
      return (mEhdr.e_shoff + (mEhdr.e_shentsize * mEhdr.e_shnum));
    }
    mError = "unknown ELF class " + std::to_string(mEhdr.e_ident[EI_CLASS]);
    return 1;
  }
};

int main(int argc, char** argv) {
  char self[PATH_MAX];
  ssize_t path_size = readlink(SELF, self, PATH_MAX);
  if (path_size == -1) {
    std::cerr << "Error! failed to read selfpath: " << strerror(errno)
              << std::endl;
    return 1;
  }
  self[path_size] = '\0';
  size_t size;

  {
    Elf elf(self);
    size = elf.size();
    if (size == 1) {
      std::cerr << "Error! failed to get elf size " << elf.error() << std::endl;
      return 1;
    }
  }
  
  

  std::cout << "size: " << size << std::endl;
  return 0;
}