#include "Command.hh"

#include <unistd.h>
#include <sys/wait.h>
#include <iostream>

namespace rlxos::libpkgupd
{
    std::tuple<int, std::string> Command::GetOutput()
    {
        std::array<char, 128> buffer;
        std::string result;

        std::string command;

        if (dir.length())
            command = "cd " + dir + ";";

        if (dir.length())
        {
            if (!std::filesystem::exists(dir))
            {
                error = dir + " not exists";
                return {1, error};
            }
        }

        for (auto const &i : environ)
            command += i + " ";

        command += binary;
        for (auto const &i : args)
            command += " " + i;

        auto pipe = popen(command.c_str(), "r");
        if (!pipe)
            return {1, "popen() failed"};

        while (!feof(pipe))
            if (fgets(buffer.data(), 128, pipe) != nullptr)
                result += buffer.data();

        // cleaning last '\n'
        result = result.substr(0, result.length() - 1);

        return {WEXITSTATUS(pclose(pipe)), result};
    }

    int Command::Execute()
    {
        std::string script = binary;
        for (auto const &i : args)
            script += " " + i;

        return ExecuteScript(script, dir, environ);
    }

    int Command::ExecuteScript(std::string const &script, std::string dir, std::vector<std::string> env)
    {
        std::string cmd = "set -e; set -u\n";

        auto env_iter = ::environ;

        for (char **e = env_iter; *e != 0; e++)
        {
            std::string environ_variable(*e);

            size_t idx = environ_variable.find_first_of('=');
            if (environ_variable.length() == 0 ||
                idx == std::string::npos)
                continue;

            auto val = environ_variable.substr(idx + 1, environ_variable.length() - (idx + 1));
            if (val[0] == '"' || val[0] == '\'')
            {
            }
            else
                val = "\"" + val + "\"";

            cmd += "export " + environ_variable.substr(0, idx) + "=" + val + "\n";
        }

        for (auto const &i : env)
            cmd += "export " + i + "\n";

        if (dir != ".")
            cmd += "cd " + dir + "\n";

        cmd += script;

        DEBUG("Executing '" << cmd << "'");
        return WEXITSTATUS(system(cmd.c_str()));
    }

}