#ifndef LIBPKGUPD_COMMAND_HXX
#define LIBPKGUPD_COMMAND_HXX

#include <filesystem>
#include <optional>
#include <vector>
#include <string>

namespace rlxos::libpkgupd {
    class Command {
        std::vector<std::string> args;
        std::string bin;
        std::optional<std::filesystem::path> dir_;
        std::vector<std::string> env_;

    public:
        Command(const std::string& bin) : bin{bin} {
        }

        Command& arg(const std::string& a) {
            args.emplace_back(a);
            return *this;
        }

        Command& dir(const std::filesystem::path& d) {
            dir_ = d;
            return *this;
        }

        Command& env(const std::string& e) {
            env_.emplace_back(e);
            return *this;
        }


    };
} // rlxos

#endif
