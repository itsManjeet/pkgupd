#include "RecipeFile.hh"
#include "Archive.hh"


#include <yaml-cpp/yaml.h>

#include <iostream>
#include <algorithm> // find_if

using std::string;

#define READ_COMMON()                                         \
    READ_LIST(string, sources);                               \
    READ_LIST(string, environ);                               \
    READ_LIST_FROM(string, runtime, depends, runtimeDepends); \
    READ_LIST_FROM(string, buildtime, depends, buildtimeDepends);

namespace rlxos::libpkgupd
{

    std::shared_ptr<RecipeFile> RecipeFile::FromPackage(std::string const &pkgpath)
    {
        Archive archive(pkgpath);
        auto [status, data] = archive.ReadFile("./.info");
        if (status != 0)
            throw std::runtime_error(data);

        auto ptr = std::make_shared<RecipeFile>(YAML::Load(data), pkgpath);
        for (auto &pkg : ptr->packages)
            pkg->SetParent(ptr->shared_from_this());

        return ptr;
    }

    RecipeFile::RecipeFile(YAML::Node const &data, std::string const &file)
    {
        READ_VALUE(string, id);
        READ_VALUE(string, version);
        READ_VALUE(string, about);

        READ_COMMON();

        READ_OBJECT_LIST(User, users);
        READ_OBJECT_LIST(Group, groups);

        READ_OBJECT_LIST(RecipePackageInfo, packages);
    }

    RecipePackageInfo::RecipePackageInfo(YAML::Node const &data, std::string const &file)
    {
        READ_VALUE(string, id);
        READ_VALUE(string, dir);

        READ_COMMON();

        OPTIONAL_VALUE(string, prescript, "");
        OPTIONAL_VALUE(string, postscript, "");
        OPTIONAL_VALUE(string, script, "");
        OPTIONAL_VALUE(string, preinstall, "");
        OPTIONAL_VALUE(string, postscript, "");

        OPTIONAL_VALUE(string, pack, "rlx");

        READ_LIST(string, skipstrip);
        OPTIONAL_VALUE(bool, nostrip, false);

        READ_OBJECT_LIST(Flag, flags);
    }

    Flag::Flag(YAML::Node const &data, std::string const &file)
    {
        READ_VALUE(string, id);
        READ_VALUE(string, value);

        OPTIONAL_VALUE(bool, force, false);
        if (data["only"])
        {
            INFO("Use of 'only' in flags is deprecated, use 'force'")
            force = data["only"].as<bool>();
        }
    }

    User::User(YAML::Node const &data, std::string const &file)
    {
        READ_VALUE(unsigned int, id);
        READ_VALUE(string, name);
        READ_VALUE(string, about);
        READ_VALUE(string, group);
        READ_VALUE(string, dir);
        READ_VALUE(string, shell);
    }

    Group::Group(YAML::Node const &data, std::string const &file)
    {
        READ_VALUE(unsigned int, id);
        READ_VALUE(string, name);
    }

    string RecipePackageInfo::ID() const
    {
        if (id == "lib" ||
            id == "lib32")
            return id + parentRecipeFile->id;

        if (id == "pkg")
            return parentRecipeFile->id;

        return parentRecipeFile->id + "-" + id;
    }

    string RecipePackageInfo::Version() const
    {
        return parentRecipeFile->version;
    }

    string RecipePackageInfo::About() const
    {
        return parentRecipeFile->about;
    }

    std::vector<std::string> RecipePackageInfo::Depends(bool all) const
    {
        std::vector<string> depends = runtimeDepends;
        depends.insert(depends.end(), parentRecipeFile->runtimeDepends.begin(), parentRecipeFile->runtimeDepends.end());

        if (all)
        {
            depends.insert(depends.end(), buildtimeDepends.begin(), buildtimeDepends.end());
            depends.insert(depends.end(), parentRecipeFile->buildtimeDepends.begin(), parentRecipeFile->buildtimeDepends.end());
        }

        return depends;
    }

    std::vector<std::string> RecipePackageInfo::Sources() const
    {
        std::vector<std::string> allSources = sources;
        allSources.insert(allSources.end(), parentRecipeFile->sources.begin(), parentRecipeFile->sources.end());
        return allSources;
    }

    std::vector<std::string> RecipePackageInfo::Environ()
    {
        std::vector<std::string> allEnviron = parentRecipeFile->environ;
        allEnviron.insert(allEnviron.end(), environ.begin(), environ.end());
        return allEnviron;
    }

    std::shared_ptr<RecipePackageInfo> RecipeFile::operator[](std::string const &pkgid) const
    {
        auto pkgiter = std::find_if(
            packages.begin(), packages.end(),
            [&](std::shared_ptr<RecipePackageInfo> const &p)
            {
                if (pkgid == this->ID() && (p->ID() == "pkg"))
                    return true;

                if (pkgid == p->ID())
                    return true;

                return false;
            });

        if (pkgiter == packages.end())
            return nullptr;

        return (*pkgiter);
    }
}