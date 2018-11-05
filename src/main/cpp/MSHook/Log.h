/* Cydia Substrate - Powerful Code Insertion Platform
 * Copyright (C) 2008-2011  Jay Freeman (saurik)
*/

/* GNU Lesser General Public License, Version 3 {{{ */
/*
 * Substrate is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * Substrate is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Substrate.  If not, see <http://www.gnu.org/licenses/>.
**/
/* }}} */

#ifndef SUBSTRATE_LOG_HPP
#define SUBSTRATE_LOG_HPP

#include <android/log.h>

#define MSLogLevelNotice ANDROID_LOG_INFO
#define MSLogLevelError ANDROID_LOG_ERROR

#ifndef LOG_TAG
	# define LOG_TAG "NativeBitmap-HOOK"
#endif

#define MSDebug false // control inject log

// MSLog define
/*#define MSLog(level, fmt,...) do { \
	printf("[%12s] " fmt "\n", __FUNCTION__,##__VA_ARGS__); \
    __android_log_print(level, LOG_TAG, "[%s]" fmt, __FUNCTION__,##__VA_ARGS__); \
} while (false)*/

#define MSLog(level, fmt,...) while(0){}

#endif//SUBSTRATE_LOG_HPP
