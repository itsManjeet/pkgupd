#ifndef LIBPKGUPD_CMAKE
#define LIBPKGUPD_CMAKE

#include "../builder.hxx"
namespace libpkgupd {
class CMake : public Compiler {
 protected:
  bool compile(Recipe* recipe, Configuration* config, std::string dir,
               std::string destdir, std::vector<std::string>& environ);
};
}  // namespace libpkgupd

#endif