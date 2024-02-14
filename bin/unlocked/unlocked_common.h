/*
 * Copyright (c) 2024 Manjeet Singh <itsmanjeet1998@gmail.com>.
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

#ifndef UNLOCKED_COMMON_H
#define UNLOCKED_COMMON_H

#include "../common.h"

#define PKGUPD_UNLOCKED_MODULE(id)                                             \
    extern "C" int PKGUPD_UNLOCKED_##id(std::vector<std::string> const& args,  \
            Engine* engine, Configuration* config)

#define PKGUPD_UNLOCKED_MODULE_HELP(id)                                        \
    extern "C" void PKGUPD_UNLOCKED_help_##id(std::ostream& os, int padding)

#endif // UNLOCKED_COMMON_H
