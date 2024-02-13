#include "../src/SysImage.h"
#include "../src/SysUpgrade.h"
#include "common.h"

PKGUPD_MODULE_HELP(hijack) {
    os << "Hijack your system with rlxos" << std::endl;
}

PKGUPD_MODULE(hijack) {
    std::filesystem::path root_dir =
            config->get<std::string>("hijack.root", "/");
    auto deployment_dir = root_dir / "system" / "deploy";
    auto boot_dir = root_dir / "boot";

    PROCESS("Creating Hierarchy")
    for (const auto& dir : {deployment_dir, boot_dir}) {
        std::filesystem::create_directories(dir);
    }

    auto sysroot = Sysroot(
            config->get<std::string>("hijack.osname", "rlxos"), root_dir);

    auto upgrader = SysUpgrade(&sysroot);

    PROCESS("Installing system image");
    upgrader.update(
            config->get<std::string>("hijack.server", "https://repo.rlxos.dev"),
            config->get<bool>("hijack.dry-run", false));

    return 0;
}