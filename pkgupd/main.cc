#include "PKGUPD.hh"

int main(int ac, char **av)
{
    return rlxos::libpkgupd::PKGUPD().Execute(ac, av);
}