#include "Compiler.hh"
#include "../libpkgupd/Command.hh"

namespace rlxos::libpkgupd
{
    Compiler::Configurator Compiler::getConfigurator(std::string const &path)
    {
        std::string touse;

        for (auto const &i : package->Flags())
        {
            if (i->ID() == "configurator")
            {
                touse = i->Value();
                break;
            }
        }

        if (touse.size())
        {
            if (configurators.find(touse) == configurators.end())
                return Configurator::INVALID;
            else
                return configurators[touse];
        }

        for (auto const &i : configurators)
            if (std::filesystem::exists(path + "/" + i.first))
                return i.second;

        return Configurator::INVALID;
    }

    Compiler::Builder Compiler::getBuilder(std::string const &path)
    {
        std::string touse;

        for (auto const &i : package->Flags())
        {
            if (i->ID() == "builder")
            {
                touse = i->Value();
                break;
            }
        }

        if (touse.size())
        {
            if (builders.find(touse) == builders.end())
                return Builder::INVALID;
            else
                return builders[touse];
        }

        for (auto const &i : builders)
            if (std::filesystem::exists(path + "/" + i.first))
                return i.second;

        return Builder::INVALID;
    }

    bool Compiler::Compile(std::string const &srcdir, std::string const &destdir)
    {
        std::string builddir = srcdir;

        if (package->Script().size() != 0)
        {
            auto binary = "/bin/sh";
            std::vector<std::string> args = {"-e", "-u", "-c", package->Script()};
            auto cmd = Command(binary, args);
            cmd.SetDirectory(srcdir);
            cmd.SetEnviron(package->Environ());

            if (int status = cmd.Execute(); status != 0)
            {
                error = "Script failed with exit code: " + std::to_string(status);
                return false;
            }

            return true;
        }

        auto getargs = [&](std::string const &var, std::vector<std::string> fallback)
        {
            for (auto const &i : this->package->Flags())
            {
                if (i->ID() == var)
                {
                    std::stringstream ss(i->Value());
                    std::string arg;
                    std::vector<std::string> args;
                    while (ss >> arg)
                        args.push_back(arg);

                    if (i->Force())
                        return args;
                    else
                    {
                        fallback.insert(fallback.end(), args.begin(), args.end());
                        return fallback;
                    }
                }
            }
            return fallback;
        };

        auto config = getConfigurator(srcdir);

        builddir = srcdir + "/pkgupd_build_" + package->ID();
        std::filesystem::create_directories(builddir);

        std::string binary;
        std::vector<std::string> args;

        switch (config)
        {
        case Configurator::AUTOCONF:
            args = getargs("configure", {"--prefix=/usr"});
            binary = srcdir + "/configure";
            break;

        case Configurator::MESON:
            args = getargs("configure", {"--prefix=/usr", srcdir});
            binary = "meson";
            break;

        case Configurator::CMAKE:
            args = getargs("configure", {"-DCMAKE_INSTALL_PREFIX=/usr", "-S", srcdir});
            binary = "cmake";
            break;

        default:
            error = "No known configurator found";
            return false;
        }

        {
            auto exec = Command(binary, args);
            exec.SetDirectory(builddir);
            exec.SetEnviron(package->Environ());

            if (int status = exec.Execute(); status != 0)
            {
                error = "Failed to configure, exit code: " + std::to_string(status);
                return false;
            }
        }

        auto builder = getBuilder(builddir);
        switch (builder)
        {
        case Builder::MAKE:
            binary = "make";
            args = getargs("compile", {});
            break;
        case Builder::NINJA:
            binary = "ninja";
            args = getargs("compile", {});
            break;

        default:
            error = "No known configurator found";
            return false;
        }

        {
            auto exec = Command(binary, args);
            exec.SetDirectory(builddir);
            exec.SetEnviron(package->Environ());

            if (int status = exec.Execute(); status != 0)
            {
                error = "Failed to compile, exit code: " + std::to_string(status);
                return false;
            }
        }

        std::vector<std::string> environ = package->Environ();

        switch (builder)
        {
        case Builder::MAKE:
            binary = "make";
            args = getargs("compile", {"DESTDIR=" + destdir,"install"});
            break;
        case Builder::NINJA:
            binary = "ninja";
            args = getargs("compile", {"install"});
            break;

        default:
            error = "No known configurator found";
            return false;
        }

        environ.push_back("DESTDIR=" + destdir);

        {
            auto exec = Command(binary, args);
            exec.SetDirectory(builddir);
            exec.SetEnviron(environ);

            if (int status = exec.Execute(); status != 0)
            {
                error = "Failed to compile, exit code: " + std::to_string(status);
                return false;
            }
        }

        return true;
    }
}