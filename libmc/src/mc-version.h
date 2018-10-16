/*
 * Copyright © 2010 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _MC_VERSION_H_
#define _MC_VERSION_H_

/* The major version, (1, if %LIBMC_VERSION is 1.2.3) */
#define LIBMC_VERSION_MAJOR (3)

/* The minor version (2, if %LIBMC_VERSION is 1.2.3) */
#define LIBMC_VERSION_MINOR (0)

/* The micro version (3, if %LIBMC_VERSION is 1.2.3) */
#define LIBMC_VERSION_MICRO (6)

/* The full version, like 1.2.3 */
#define LIBMC_VERSION        3.0.6

/* The full version, in string form (suited for string concatenation)
 */
#define LIBMC_VERSION_STRING "3.0.6"

/* Numerically encoded version, like 0x010203 */
#define LIBMC_VERSION_HEX ((LIBMC_MAJOR_VERSION << 24) |        \
                               (LIBMC_MINOR_VERSION << 16) |        \
                               (LIBMC_MICRO_VERSION << 8))

/* Evaluates to True if the version is greater than @major, @minor and @micro
 */
#define LIBMC_VERSION_CHECK(major,minor,micro)      \
    (LIBMC_VERSION_MAJOR > (major) ||               \
     (LIBMC_VERSION_MAJOR == (major) &&             \
      LIBMC_VERSION_MINOR > (minor)) ||             \
     (LIBMC_VERSION_MAJOR == (major) &&             \
      LIBMC_VERSION_MINOR == (minor) &&             \
      LIBMC_VERSION_MICRO >= (micro)))

#endif /* _MC_VERSION_H_ */
