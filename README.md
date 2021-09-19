# PKGUPD
A better package manager for rlxos

```
Usage: ./build/pkgupd [TASK] [ARGS]... [PKGS]..
PKGUPD is a system package manager for rlxos.
Perfrom system level package transactions like installations, upgradations and removal.

TASK:
  in,  install                 download and install specified package(s) from repository into the system
  rm,  remove                  remove specified package(s) from the system if already installed
  rf,  refresh                 synchronize local data with repositories
  up,  update                  upgarde specified package(s) to their latest avaliable version
  co,  compile                 try to compile specified package(s) from repository recipe files
  deptest                      perform dependencies test for specified package
  info                         print information of specified package

To override default values simply pass argument as VALUE_NAME=VALUE
Avaliable Values:
  config                       override default configuration files path
  download-url                 override primary repository url
  secondary-download-url       override secondary repository url
  sys-db                       override default system database
  repo-db                      override default repository database path

Exit Status:
  0  if OK
  1  if issue with input data provided.

Full documentation <https://docs.rlxos.dev/pkgupd>
or local manual: man pkgupd
```