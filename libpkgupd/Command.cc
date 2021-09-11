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

        if (pclose(pipe) == EXIT_FAILURE)
            return {1, result};

        return {0, result};
    }

    int Command::Execute()
    {
        std::vector<char const *> argv;
        argv.push_back(binary.c_str());
        for (auto const &i : args)
            argv.push_back(i.c_str());
        argv.push_back(nullptr);

        std::vector<char const *> env;
        for (char **e = ::environ; *e != 0; e++)
            env.push_back(*e);

        for (auto const &i : environ)
            env.push_back(i.c_str());
        env.push_back(nullptr);

        if (getenv("DEBUG"))
        {
            std::cout << binary;
            for (auto const &a : argv)
                std::cout << " " << a;
            std::cout << std::endl;

            if (environ.size())
            {
                std::cout << "Environ: " << std::endl;
                for (auto const &a : env)
                    std::cout << "  " << a << std::endl;
            }
        }

        if (dir.length())
        {
            if (!std::filesystem::exists(dir))
            {
                error = dir + " not exists";
                return 1;
            }
        }

        pid_t pid = fork();

        if (pid == 0) // check
        {
            if (dir.length())
                chdir(dir.c_str());

            execvpe(argv[0], (char *const *)argv.data(), (char *const *)env.data());
            printf("Failed to execute %s\n", argv[0]);
            exit(EXIT_FAILURE);
        }
        else if (pid > 0) // parent
        {
            int waitstatus;
            wait(&waitstatus);
            return WEXITSTATUS(waitstatus);
        }

        error = "fork() failed";
        return -1;
    }
}