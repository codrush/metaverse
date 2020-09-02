/**
 * Copyright (c) 2016-2020 metaverse core developers (see MVS-AUTHORS)
 *
 * This file is part of metaverse.
 *
 * metaverse is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef MVS___STRING_VIEW__
#define MVS___STRING_VIEW__

#if __cplusplus <= 201402L || (defined(_MSC_VER) && _MSC_VER < 1910) || (defined(__APPLE__) && __clang_major__ < 10)

#include <experimental/string_view>
namespace std {
using string_view = std::experimental::string_view;
}
#else

#include <string_view>

#endif

using std::string_view;

#endif // MVS___STRING_VIEW__
