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

#include <sstream>
#include "Hash.h"


Hash &Hash::add(const std::string &content) {
    for (auto c: content) {
        sum = (sum * 54059) ^ (c * 76963);
    }
    return *this;
}

std::string Hash::get() {
    std::stringstream ss;
    ss << std::hex << sum;
    return ss.str();
}
