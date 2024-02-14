/*
 * Copyright (c) 2023 Manjeet Singh <itsmanjeet1998@gmail.com>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef PKGUPD_IGNITE_COMMON_H
#define PKGUPD_IGNITE_COMMON_H

#include "../../src/Ignite.h"
#include "../common.h"

#define PKGUPD_IGNITE_MODULE(id)                                               \
    extern "C" int PKGUPD_IGNITE_##id(std::vector<std::string> const& args,    \
            Ignite* ignite, Configuration* config)

#define PKGUPD_IGNITE_MODULE_HELP(id)                                          \
    extern "C" void PKGUPD_IGNITE_help_##id(std::ostream& os, int padding)

#endif // PKGUPD_IGNITE_COMMON_H
