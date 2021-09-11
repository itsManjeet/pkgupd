#include "Archive.hh"
#include "Command.hh"

namespace rlxos::libpkgupd
{

    bool Archive::Compress(std::string const &srcdir, std::vector<std::string> excludefile)
    {
        std::string pardir = std::filesystem::path(pkgfile).parent_path();
        if (!std::filesystem::exists(pardir))
        {
            std::error_code err;
            std::filesystem::create_directories(pardir, err);
            if (err)
            {
                error = "Failed to create " + pardir + ", " + err.message();
                return false;
            }
        }
        std::vector<std::string> commandArgs = args;

        for (auto i : excludefile)
        {
            commandArgs.push_back("--exclude");
            commandArgs.push_back(i);
        }

        commandArgs.push_back("-cf");
        commandArgs.push_back(pkgfile);
        commandArgs.push_back("-C");
        commandArgs.push_back(srcdir);
        commandArgs.push_back(".");

        return Command(archiveTool, commandArgs).Execute() == 0;
    }

    bool Archive::Extract(std::string const &outdir, std::vector<std::string> excludefile)
    {
        std::vector<std::string> commandArgs = args;

        for (auto i : excludefile)
        {
            commandArgs.push_back("--exclude");
            commandArgs.push_back(i);
        }

        commandArgs.push_back("-xf");
        commandArgs.push_back(pkgfile);
        commandArgs.push_back("-C");
        commandArgs.push_back(outdir);

        return Command(archiveTool, commandArgs).Execute() == 0;
    }

    std::tuple<int, std::string> Archive::ReadFile(std::string const &path) const
    {
        std::vector<std::string> commandArgs = args;

        commandArgs.push_back("-O");
        commandArgs.push_back("-xf");
        commandArgs.push_back(pkgfile);
        commandArgs.push_back(path);

        return Command(archiveTool, commandArgs).GetOutput();
    }

    std::vector<std::string> Archive::List()
    {
        std::vector<std::string> commandArgs = args;

        commandArgs.push_back("-tf");
        commandArgs.push_back(pkgfile);

        auto [status, output] = Command(archiveTool, commandArgs).GetOutput();
        if (status != 0)
        {
            error = output;
            return {};
        }

        std::stringstream ss(output);
        std::string file;

        std::vector<std::string> filesList;

        while (std::getline(ss, file, '\n'))
            filesList.push_back(file);

        return filesList;
    }
}