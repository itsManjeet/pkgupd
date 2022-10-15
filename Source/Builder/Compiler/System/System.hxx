#ifndef _LIBPKGUPD_COMPILERS_SYSTEM_HH_
#define _LIBPKGUPD_COMPILERS_SYSTEM_HH_

#include "../builder.hxx"
namespace libpkgupd {
class System : public Compiler {
 private:
  bool mountPseudo(std::string destdir);
  bool umountPseudo(std::string destdir);

 protected:
  bool compile(Recipe* recipe, Configuration* config, std::string dir,
               std::string destdir, std::vector<std::string>& environ);
};
}  // namespace libpkgupd

#endif