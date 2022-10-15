#ifndef __LIBPKGUPD_COMPILER_HXX__
#define __LIBPKGUPD_COMPILER_HXX__

#include "../../Recipe/Recipe.hxx"
#include "../../Utilities/defines.hxx"

namespace libpkgupd {
class Compiler : public Object {
 public:
  virtual bool compile(Recipe *recipe, Configuration *config,
                       std::string source_dir, std::string destdir,
                       std::vector<std::string> &environ) = 0;

  static std::shared_ptr<Compiler> create(BuildType buildType);
};
}  // namespace libpkgupd

#endif