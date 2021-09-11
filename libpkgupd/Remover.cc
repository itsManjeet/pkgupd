#include "Remover.hh"

namespace rlxos::libpkgupd
{
    bool Remover::Remove(std::shared_ptr<SystemPackageInfo> pkginfo)
    {
        auto files = pkginfo->Files();

        bool status = true;

        fileslist.push_back(std::vector<std::string>());

        for (auto i = files.rbegin(); i != files.rend(); i++)
        {
            std::error_code err;

            std::string path = rootdir + "/" + (*i)->Path();
            if (std::filesystem::exists(path))
            {
                if (std::filesystem::is_directory(path))
                {
                    fileslist.back().push_back(path);

                    if (std::filesystem::is_empty(path))
                        std::filesystem::remove_all(path, err);
                }
                else
                {
                    std::filesystem::remove(path, err);
                    fileslist.back().push_back(path);
                }
            }

            if (err)
            {
                error += "\n" + err.message();
                status = false;
            }
        }

        return status;
    }

    bool Remover::Remove(std::vector<std::string> const &pkgs)
    {
        std::vector<std::shared_ptr<SystemPackageInfo>> pkgsInfo;

        for (auto const &i : pkgs)
        {
            auto pkginfo = (*mSystemDatabase)[i];
            if (pkginfo == nullptr)
            {
                error = mSystemDatabase->Error();
                return false;
            }

            pkgsInfo.push_back(std::dynamic_pointer_cast<SystemPackageInfo>(pkginfo));
        }

        for (auto const &i : pkgsInfo)
        {
            PROCESS("cleaning file of " << i->ID());
            if (!Remove(i))
                return false;
        }

        if (!mTriggerer.Trigger(fileslist))
        {
            error = mTriggerer.Error();
            return false;
        }

        return true;
    }
}