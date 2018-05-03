#ifndef UTILS_HPP
#define UTILS_HPP

#include <QStringList>
#include <QByteArray>

struct AsmFile : public QList<QByteArray>
{
	QList<QPair<QString, QStringList>> subroutines; //label, code
	QList<QPair<QString, QString>> jumptables;
};
AsmFile readAssemblyFile(const QString &fileName);
QByteArray getVariables(const AsmFile &asmF);

QString bitsStrfromSize(quint32 size, const QString &suffix = "i");

#endif // UTILS_HPP
