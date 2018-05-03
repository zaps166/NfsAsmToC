#include "Instruction.hpp"

#include <QMap>

using namespace std;

QString Instruction::Operand::getBitsStrFromSize() const
{
	return bitsStrfromSize(size, QString());
}
QString Instruction::Operand::getName() const
{
	return QString(name).replace("###", bitsStrfromSize(size));
}

void Instruction::Operand::repairName()
{
	name.replace("###", bitsStrfromSize(size));
}

void Instruction::repairOperandsSizes(Instruction::Operand operands[2])
{
	if (!operands[0].size && !operands[1].size)
		throw runtime_error("One of operands must have a size");
	if (!operands[0].size)
		operands[0].size = operands[1].size;
	else if (!operands[1].size)
		operands[1].size = operands[0].size;
	for (int i = 0; i < 2; ++i)
		operands[i].repairName();
}

Instruction::Instruction(const QString &line)
{
	QList<QStringList> parts;
	for (const QString &part : line.split(','))
		parts += part.split(' ');
	if (!parts.isEmpty())
	{
		if (parts.count() == 1 && parts[0].at(0).endsWith(':'))
			label = parts[0].at(0);
		else
		{
			if (parts[0].at(0).startsWith("rep"))
				rep = parts[0].takeFirst();
			if (parts[0].isEmpty())
				throw runtime_error("Bad rep instruction");
			mnemonic = parts[0].takeFirst();
			if (rep.isEmpty())
			{
				static const QMap<QString, quint32> sizeMap = {
					{"byte", 1},
					{"word", 2},
					{"dword", 4},
					{"qword", 8}
				};
				static const QMap<QString, quint32> regsMap = {
					{"eax", 4},
					{"ecx", 4},
					{"edx", 4},
					{"ebx", 4},
					{"esp", 4},
					{"ebp", 4},
					{"esi", 4},
					{"edi", 4},
					{"ax", 2},
					{"cx", 2},
					{"dx", 2},
					{"bx", 2},
					{"sp", 2},
					{"bp", 2},
					{"si", 2},
					{"di", 2},
					{"al", 1},
					{"ah", 1},
					{"cl", 1},
					{"ch", 1},
					{"dl", 1},
					{"dh", 1},
					{"bl", 1},
					{"bh", 1},
					{"cs", 2},
					{"ds", 2},
					{"es", 2},
					{"fs", 2},
					{"gs", 2},
					{"ss", 2}
				};
				for (int i = 0; i < parts.count(); ++i)
				{
					QStringList &part = parts[i];
					if (part.isEmpty())
						continue;
					Operand operand;
					const auto itSizeMap = sizeMap.find(part.at(0));
					if (itSizeMap != sizeMap.constEnd())
					{
						operand.size = itSizeMap.value();
						part.removeFirst();
					}
					if (part.count() != 1)
						throw runtime_error("Bad part: " + to_string(i + 1));
					operand.name = part.at(0);
					const auto itRegMap = regsMap.find(operand.name);
					if (itRegMap == regsMap.constEnd())
					{
						/* Konwersja hexa */
						int hIdx = operand.name.indexOf('h');
						int n = 0;
						while (hIdx > 0)
						{
							QChar d = operand.name[hIdx - 1].toLower();
							if (d.isDigit() || (d.toLatin1() >= 'a' && d.toLatin1() <= 'f'))
							{
								++n;
								--hIdx;
								continue;
							}
							break;
						}
						if (n && operand.name[hIdx].isDigit())
						{
							bool ok = !hIdx;
							if (!ok)
							{
								const char c = operand.name[hIdx - 1].toLatin1();
								ok = (c == '[' || c == '(' || c == '+' || c == '-' || c == '*' || c == '/');
							}
							if (ok)
							{
								QString number = operand.name.mid(hIdx, n);
								if (number.startsWith("0"))
									number.remove(0, 1);
								operand.name.replace(operand.name.mid(hIdx, n + 1), "0x" + number);
							}
						}

						/* Konwersja adresów i wartości */
						if (operand.name.endsWith("]"))
						{
							int idx = operand.name.indexOf('[');
							if (idx > -1)
							{
								bool ok = true, removeBrackets = true;
								QString valStr;
								if (idx > 0)
								{
									valStr = operand.name.mid(0, idx);
									uint val = valStr.toUInt(&ok, valStr.startsWith("0x") ? 16 : 10);
									if (ok)
										valStr = val >= 10 ? QString("0x%1").arg(val, 0, 16) : QString("%1").arg(val);
									else if (operand.size != 1 && operand.size != 0)
									{
										if (valStr.startsWith("(") && valStr.endsWith(")"))
										{
											valStr.remove(0, 1);
											valStr.remove(valStr.length() - 1, 1);
										}
									}
									else
										removeBrackets = false;
								}
								if (removeBrackets)
								{
									/* Usuwanie nawiasów */
									operand.name.remove(0, idx + 1);
									operand.name.remove(operand.name.length() - 1, 1);

									if (!valStr.isEmpty())
									{
										if (ok)
											operand.name.append("+" + valStr);
										else
											operand.name.prepend(valStr + "+");
									}
									operand.name.prepend("to###(");
									operand.name.append(")");
								}
							}
						}
					}
					else
					{
						const quint32 regSize = itRegMap.value();
						quint32 &size = operand.size;
						if (!size)
							size = regSize;
						else if (size != regSize)
							throw runtime_error("Detected size differs");
					}
					operands += operand;
				}
			}
		}
	}
}

bool Instruction::isOK() const
{
	return !mnemonic.isEmpty() || !rep.isEmpty() || !label.isEmpty();
}
