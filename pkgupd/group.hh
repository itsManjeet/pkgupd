#ifndef LIBPKGUPD_GROUP
#define LIBPKGUPD_GROUP

#include <yaml-cpp/yaml.h>

#include <string>

namespace rlxos::libpkgupd {
class Group {
 private:
  unsigned int m_ID;
  std::string m_Name;

 public:
  Group(unsigned int id, std::string const &name) : m_ID(id), m_Name(name) {}

  Group(YAML::Node const &data, std::string const &file);

  std::string const &name() const { return m_Name; }

  bool exists() const;

  bool create() const;

  void dump(std::ostream &os) const;
};
}  // namespace rlxos::libpkgupd

#endif