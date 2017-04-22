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

#include "Config.h"

#define DBLSCAN_VERSION
#include "Version.h"

//Version info
const unsigned int DOUBLESCANNER_VERSION_MAJOR = DBLSCAN_VER_MAJOR;
const unsigned int DOUBLESCANNER_VERSION_MINOR = (DBLSCAN_VER_MINOR_HI * 10) + (DBLSCAN_VER_MAJOR_LO % 10);
const unsigned int DOUBLESCANNER_VERSION_PATCH = DBLSCAN_VER_PATCH;

//Build date/time
const char* DOUBLESCANNER_BUILD_DATE = __DATE__;
const char* DOUBLESCANNER_BUILD_TIME = __TIME__;

//Compiler detection
#if defined(__INTEL_COMPILER)
	#if (__INTEL_COMPILER >= 1300)
		static char *DOUBLESCANNER_COMPILER = "ICL 13." LAMEXP_MAKE_STR(__INTEL_COMPILER_BUILD_DATE);
	#elif (__INTEL_COMPILER >= 1200)
		static char *DOUBLESCANNER_COMPILER = "ICL 12." LAMEXP_MAKE_STR(__INTEL_COMPILER_BUILD_DATE);
	#elif (__INTEL_COMPILER >= 1100)
		static char *DOUBLESCANNER_COMPILER = "ICL 11.x";
	#elif (__INTEL_COMPILER >= 1000)
		static char *DOUBLESCANNER_COMPILER = "ICL 10.x";
	#else
		#error Compiler is not supported!
	#endif
#elif defined(_MSC_VER)
	#if (_MSC_VER == 1900)
		#if (_MSC_FULL_VER >= 190024210) && (_MSC_FULL_VER <= 190024215)
			const char *DOUBLESCANNER_COMPILER = "MSVC 2015.3";
		#else
			#error Compiler version is not supported yet!
		#endif
	#elif (_MSC_VER == 1800)
		#if (_MSC_FULL_VER < 180021005)
			const char *DOUBLESCANNER_COMPILER = "MSVC 2013-Beta";
		#elif (_MSC_FULL_VER < 180030501)
			const char *DOUBLESCANNER_COMPILER = "MSVC 2013";
		#elif (_MSC_FULL_VER == 180030501)
			const char *DOUBLESCANNER_COMPILER = "MSVC 2013.2";
		#else
			#error Compiler version is not supported yet!
		#endif
	#elif (_MSC_VER == 1700)
		#if (_MSC_FULL_VER < 170050727)
			const char *DOUBLESCANNER_COMPILER = "MSVC 2012-Beta";
		#elif (_MSC_FULL_VER < 170051020)
			const char *DOUBLESCANNER_COMPILER = "MSVC 2012";
		#elif (_MSC_FULL_VER < 170051106)
			const char *DOUBLESCANNER_COMPILER = "MSVC 2012.1-CTP";
		#elif (_MSC_FULL_VER < 170060315)
			const char *DOUBLESCANNER_COMPILER = "MSVC 2012.1";
		#elif (_MSC_FULL_VER < 170060610)
			const char *DOUBLESCANNER_COMPILER = "MSVC 2012.2";
		#elif (_MSC_FULL_VER < 170061030)
			const char *DOUBLESCANNER_COMPILER = "MSVC 2012.3";
		#elif (_MSC_FULL_VER == 170061030)
			const char *DOUBLESCANNER_COMPILER = "MSVC 2012.4";
		#else
			#error Compiler version is not supported yet!
		#endif
	#elif (_MSC_VER == 1600)
		#if (_MSC_FULL_VER < 160040219)
			const char *DOUBLESCANNER_COMPILER = "MSVC 2010";
		#elif (_MSC_FULL_VER == 160040219)
			const char *DOUBLESCANNER_COMPILER = "MSVC 2010-SP1";
		#else
			#error Compiler version is not supported yet!
		#endif
	#elif (_MSC_VER == 1500)
		#if (_MSC_FULL_VER >= 150030729)
			const char *DOUBLESCANNER_COMPILER = "MSVC 2008-SP1";
		#else
			const char *DOUBLESCANNER_COMPILER = "MSVC 2008";
		#endif
	#else
		#error Compiler is not supported!
	#endif

	// Note: /arch:SSE and /arch:SSE2 are only available for the x86 platform
	#if !defined(_M_X64) && defined(_M_IX86_FP)
		#if (_M_IX86_FP == 1)
			#error SSE instruction set is enabled!
		#elif (_M_IX86_FP == 2)
			#error SSE2 (or higher) instruction set is enabled!
		#endif
	#endif
#else
	#error Compiler is not supported!
#endif

//Architecture detection
#if defined(_M_X64)
	const char *DOUBLESCANNER_ARCH = "x64";
#elif defined(_M_IX86)
	const char *DOUBLESCANNER_ARCH = "x86";
#else
	#error Architecture is not supported!
#endif
