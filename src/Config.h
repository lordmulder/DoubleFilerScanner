#pragma once

//Version info
extern const unsigned int DOUBLESCANNER_VERSION_MAJOR;
extern const unsigned int DOUBLESCANNER_VERSION_MINOR;
extern const unsigned int DOUBLESCANNER_VERSION_PATCH;

//Build date/time
extern const char* DOUBLESCANNER_BUILD_DATE;
extern const char* DOUBLESCANNER_BUILD_TIME;

//Compiler info
extern const char* DOUBLESCANNER_COMPILER;
extern const char* DOUBLESCANNER_ARCH;

//Check for debug build
#if defined(_DEBUG) && defined(QT_DEBUG) && !defined(NDEBUG) && !defined(QT_NO_DEBUG)
	#define DOUBLESCANNER_DEBUG (1)
#elif defined(NDEBUG) && defined(QT_NO_DEBUG) && !defined(_DEBUG) && !defined(QT_DEBUG)
	#define DOUBLESCANNER_DEBUG (0)
#else
	#error Inconsistent debug defines detected!
#endif
