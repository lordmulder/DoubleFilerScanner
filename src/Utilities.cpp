///////////////////////////////////////////////////////////////////////////////
// Double File Scanner
// Copyright (C) 2014 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version, but always including the *additional*
// restrictions defined in the "License.txt" file.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// http://www.gnu.org/licenses/gpl-2.0.txt
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Utilities.h"

QString Utilities::sizeToString(const qint64 &size)
{
	static const struct
	{
		const char *suffix;
		const qint64 size;
	}
	SIZE[] =
	{
		{ "KB", 1024i64             },
		{ "MB", 1048576i64          },
		{ "GB", 1073741824i64       },
		{ "TB", 1099511627776i64    },
		{ "PB", 1125899906842624i64 }
	};
	

	int idx = 0;

	while((size >= SIZE[idx+1].size) && (idx < 3))
	{
		idx++;
	}

	return QString().sprintf("%.2f %s", double(size)/double(SIZE[idx].size), SIZE[idx].suffix);
}
