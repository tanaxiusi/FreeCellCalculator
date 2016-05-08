#pragma once
#include <QtGlobal>

#if defined(Q_OS_WIN)
#define U16(str)	QString::fromUtf16(L##str)
#define U8(str)		U16(str).toUtf8()
#elif defined(Q_OS_LINUX)
#define U16(str)	QString::fromUtf8(str);
#define U8(str)		(str)
#endif