#ifndef LIBPKGUPD_CONFIGURATION_HH
#define LIBPKGUPD_CONFIGURATION_HH

#include <yaml-cpp/yaml.h>
namespace rlxos::libpkgupd {
class Configuration {
 private:
  YAML::Node mNode;

 public:
  Configuration(YAML::Node const& node) : mNode{node} {}

  template <typename T>
  T get(char const* key, T t) {
    if (mNode[key]) {
      return mNode[key].as<T>();
    }
    return t;
  }

  YAML::Node& node() { return mNode; }

  void get(char const* key, std::vector<std::string>& list) {
    for (auto const& i : mNode[key]) {
      list.push_back(i.as<std::string>());
    }
  }
};
}  // namespace rlxos::libpkgupd

#endif