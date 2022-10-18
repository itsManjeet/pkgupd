#!/bin/bash -e

docker run                                                          \
    --privileged                                                    \
    -v ${PWD}:/pkgupd                                               \
    -v /var/run/docker.sock:/var/run/docker.sock                    \
    -w /pkgupd                                                       \
    -it itsmanjeet/devel-docker:2200-3                              \
    bash