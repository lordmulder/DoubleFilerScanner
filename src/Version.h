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

#ifdef DBLSCAN_VERSION
#undef DBLSCAN_VERSION

#define DBLSCAN_VER_MAJOR     2
#define DBLSCAN_VER_MINOR_HI  0
#define DBLSCAN_VER_MAJOR_LO  0
#define DBLSCAN_VER_PATCH     7

#define DBLSCAN_VER_STRING_HLP1(X)        #X
#define DBLSCAN_VER_STRING_HLP2(W,X,Y,Z)  DBLSCAN_VER_STRING_HLP1(v##W.X##Y-Z)
#define DBLSCAN_VER_STRING_HLP3(W,X,Y,Z)  DBLSCAN_VER_STRING_HLP2(W,X,Y,Z)
#define DBLSCAN_VER_STRING                DBLSCAN_VER_STRING_HLP3(DBLSCAN_VER_MAJOR,DBLSCAN_VER_MINOR_HI,DBLSCAN_VER_MAJOR_LO,DBLSCAN_VER_PATCH)

#endif //DBLSCAN_VERSION
