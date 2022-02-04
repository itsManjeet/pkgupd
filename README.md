# PKGUPD
Usage: ./build/pkgupd [TASK] [ARGS]... [PKGS]..
PKGUPD is a system package manager for rlxos.
Perfrom system level package transactions like installations, upgradations and removal.

TASK:
  in,  install                 download and install specified package(s) from repository into the system
  rm,  remove                  remove specified package(s) from the system if already installed
  rf,  refresh                 synchronize local data with repositories
  up,  update                  upgarde specifiedpackage(s) to their latest avaliable version
  co,  compile                 try to compile specified package(s) from repository recipe files
  depends                      perform dependencies test for specified package
  search                       search package from matched query
  info                         print information of specified package
  trigger                      execute require triggers and create required users & groups

To override default values simply pass argument as --config=config.yml
Parameters:
  SystemDatabase               specify system database of installed packages
  CachePath                    specify dynamic cache for path for
  RootDir                      override the default root directory path
  Version                      alter the release version of rlxos
  Mirrors                      list of mirrors to search packages
Exit Status:
  0  if OK
  1  if issue with input data provided.
  2  if build to perform specified task.

Full documentation <https://docs.rlxos.dev/pkgupd>
```