#ifndef CONSTS_HPP
#define CONSTS_HPP

#include <QString>
#include <QMap>
#include <QSet>

struct ExternData
{
	quint32 numArgs = 0;
	enum
	{
		CDECL,
		STDCALL,
		WATCOM_FASTCALL,
		CDECL_VARARG,
	} callingConvention = STDCALL;
	QSet<quint32> floatArgs = {};
};
extern const QMap<QString, ExternData> externs;

#endif // CONSTS_HPP
