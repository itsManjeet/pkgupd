#ifndef _PKGUPD_EXEC_HH_
#define _PKGUPD_EXEC_HH_

#include <vector>
#include <string>
#include <array>
#include <memory>
#include <unistd.h>
#include "colors.hh"

namespace rlxos::libpkgupd
{
    class exec : public object
    {
    private:
        std::string _get_cmd(std::string const &cmd, std::string const &dir,
                             std::vector<std::string> const &env = {})
        {
            std::string _cmd = "set -e; set -u\n";

            for (auto const &i : env)
                _cmd += "export " + i + "\n";

            if (dir != ".")
                _cmd += "cd " + dir + "\n";

            _cmd += cmd;

            return _cmd;
        }

    public:
        exec()
        {
        }
        int execute(std::string const &command,
                    std::string const &dir = ".",
                    std::vector<std::string> const &environ = {})
        {
            auto cmd = _get_cmd(command, dir, environ);
            DEBUG("executing: '" << cmd << "'");
            return WEXITSTATUS(system(cmd.c_str()));
        }

        std::tuple<int, std::string> output(
            std::string const &command,
            std::string const &dir = ".",
            std::vector<std::string> const &environ = {})
        {
            auto cmd = _get_cmd(command, dir, environ);

            std::array<char, 128> buffer;
            std::string result;
            FILE *pipe = popen(cmd.c_str(), "r");
            if (!pipe)
            {
                throw std::runtime_error("popen() failed!");
            }
            while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
            {
                result += buffer.data();
            }

            int status = WEXITSTATUS(pclose(pipe));
            if (status != 0)
                _error = "Failed to execute " + cmd;

            return {status, result};
        }
    };
}

#endif