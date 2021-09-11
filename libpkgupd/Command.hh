#ifndef _PKGUPD_COMMAND_HH_
#define _PKGUPD_COMMAND_HH_

#include "Defines.hh"

namespace rlxos::libpkgupd
{
    class Command : public Object
    {
    private:
        std::string binary;
        std::vector<std::string> args;
        std::string dir;

        std::vector<std::string> environ;

    public:
        Command(std::string const &binary, std::vector<std::string> const &args)
            : binary(binary), args{args}
        {
        }

        void SetDirectory(std::string const &dir)
        {
            this->dir = dir;
        }

        void SetEnviron(std::vector<std::string> const &env)
        {
            environ = env;
        }

        void AddEnviron(std::string const &env)
        {
            environ.push_back(env);
        }

        std::tuple<int, std::string> GetOutput();

        int Execute();
    };
}

#endif