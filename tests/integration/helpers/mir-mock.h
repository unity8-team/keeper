/*
 * Copyright (C) 2016 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * Authors:
 *     Ted Gould <ted@canonical.com>
 */

#ifndef MIR_MOCK_H
#define MIR_MOCK_H 1

#include <string>
#include <utility>

void mir_mock_connect_return_valid (bool valid);
std::pair<std::string, std::string> mir_mock_connect_last_connect (void);
void mir_mock_set_trusted_fd (int fd);

#endif // MIR_MOCK_H
