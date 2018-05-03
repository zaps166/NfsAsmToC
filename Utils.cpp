#include "Utils.hpp"

#include <QFile>

#include <iostream>
using namespace std;

AsmFile readAssemblyFile(const QString &fileName)
{
	AsmFile asmF;
	QFile f(fileName);
	if (f.open(QFile::ReadOnly))
	{
		reinterpret_cast<QList<QByteArray> &>(asmF) = f.readAll().split('\n');
		f.close();

//		for (const auto &l : assembly)
//		{
//			if (l.contains("section "))
//				break;
//			else if (l.startsWith("extern "))
//				externs += l.mid(7, l.length() - 7);
//		}

		QPair<QString, int> labelAndLineNumber = {QString(), -1}; //label, line
		for (int i = 0; i < asmF.count(); ++i)
		{
			const QByteArray &s = asmF[i];
			if (s.startsWith(";") /*&& s.count(';') == 1*/ && s.contains(" endp"))
			{
				QStringList lines;
				if (labelAndLineNumber.first.isEmpty())
					throw runtime_error("Zmienna \"labelAndLineNumber\" jest pusta");
				if (!s.contains(labelAndLineNumber.first.toLatin1()))
					throw runtime_error("Zła nazwa zakańczanej funkcji: " + labelAndLineNumber.first.toStdString() + " != " + s.mid(1).left(s.length() - 6).data());
				asmF.removeAt(labelAndLineNumber.second);
				--i;
				for (int j = labelAndLineNumber.second, k = j; j < i; ++j)
				{
					QByteArray line = asmF.takeAt(k);
					int semicolon = line.indexOf(';');
					if (semicolon > -1)
						line.remove(semicolon, line.length() - semicolon);
					line = line.simplified();
					line.replace(", ", ",");
					if (line.isEmpty() || line == "nop" || line == "wait")
						continue;
					lines += line;
				}
				if (!lines.isEmpty())
				{
					const QString &last = lines.constLast();
					if (!last.startsWith("jmp ") && !last.startsWith("ret") && last != "call ExitProcess_wrap" && !last.startsWith('%'))
						cerr << "Incorrect subroutine ending: " + labelAndLineNumber.first.toStdString() << endl;
				}
				asmF.removeAt(labelAndLineNumber.second);
				asmF.subroutines.append({labelAndLineNumber.first, lines});
				i = labelAndLineNumber.second;
				labelAndLineNumber.first.clear();
				labelAndLineNumber.second = -1;
			}
			else if (!s.startsWith(';') && s.endsWith(";SUBROUTINE"))
			{
				if (!labelAndLineNumber.first.isEmpty() || labelAndLineNumber.second > -1)
					throw runtime_error("Zmienna \"labelAndLineNumber\" nie jest pusta");
				const int colon = s.indexOf(':');
				if (colon > -1)
				{
					labelAndLineNumber.first = s.left(colon);
					labelAndLineNumber.second = i;
				}
			}
		}

		labelAndLineNumber = {QString(), -1};
		for (int i = 0; i < asmF.count(); ++i) //Wyrzucanie jumpów do switchy i wczytywanie ich do tablicy
		{
			QByteArray s = asmF[i];
			int semicolonIdx = s.indexOf(';');
			if (semicolonIdx > -1)
				s.remove(semicolonIdx, s.length() - semicolonIdx);
			s = s.simplified();
			int colonIdx = s.indexOf(':');
			if (colonIdx > -1)
			{
				if (labelAndLineNumber.second > -1)
				{
					QString jumptable = "static const void *const " + labelAndLineNumber.first + "[] = {\n\t";
					for (int j = labelAndLineNumber.second, k = j; j < i; ++j)
					{
						QByteArray line = asmF.takeAt(k).simplified();
						if (line.startsWith(';') || line.isEmpty())
							continue;
						line.remove(0, 3);
						if (line.startsWith("loc_"))
							line.prepend("&&");
						else if (line == "0")
							line = "NULL";
						jumptable += "\t" + line + ",\n\t";
					}
					asmF.jumptables += {jumptable + "};", QString()};
					i = labelAndLineNumber.second;
				}
				if (s.startsWith("off_") && s.contains("dd loc_"))
				{
					labelAndLineNumber.first = s.mid(0, colonIdx);
					labelAndLineNumber.second = i;
					s.remove(0, colonIdx + 2);
					asmF[i] = s;
				}
				else
					labelAndLineNumber = {QString(), -1};
			}
			else if (labelAndLineNumber.second > -1)
				asmF[i] = s;
		}

		for (const auto &s : asmF.subroutines)
		{
			for (const auto &l : s.second)
			{
				const int colon = l.indexOf(':');
				if (colon > -1)
				{
					const QString label = l.mid(0, colon);
					for (auto &t : asmF.jumptables)
					{
						if (t.second.isEmpty() && t.first.contains(label))
						{
							t.second = s.first;
							break;
						}
					}
				}
			}
		}
	}
	return asmF;
}

QByteArray getVariables(const AsmFile &asmF)
{
	const auto interpretTimes = [](QByteArray &line) {
		quint32 mul = 1;
		if (line.startsWith("times "))
		{
			line.remove(0, 6);
			int idx = line.indexOf(' ');
			if (idx < 0)
				throw runtime_error("\"times\" value error");
			QByteArray val = line.mid(0, idx);
			int base = 10;
			if (val.endsWith("h"))
			{
				val.chop(1);
				base = 16;
			}
			mul = val.toUInt(nullptr, base);
			line.remove(0, idx + 1);
		}
		return mul;
	};

	QByteArray bssAndDataLabels;

	/* BSS labels */
	bssAndDataLabels += "struct packed {\n\tint8_t\n\t";
	{
		const QByteArray structName = "_bss";
		QByteArray defines;
		quint32 bytes = 0;
		QString label;
		const auto addCVar = [&structName, &bssAndDataLabels, &label, &bytes, &defines] {
			if (!label.isEmpty())
			{
				bssAndDataLabels += label + "[" + QString::number(bytes) + "],\n\t";
				defines += "#define " + label + " ((int8_t *)&" + structName + "." + label + ")\n";
			}
		};
		for (QByteArray line : asmF.mid(asmF.indexOf("section .bss") + 1))
		{
			line = line.simplified();
			if (line.isEmpty())
				continue;
			int colonIdx = line.indexOf(':');
			if (colonIdx > -1)
			{
				addCVar();
				bytes = 0;
				label = line.mid(0, colonIdx);
				line = line.remove(0, colonIdx + 2);
			}
			quint32 mul = interpretTimes(line);
			switch (line[1])
			{
			case 'b':
				bytes += mul;
				break;
			case 'w':
				bytes += mul * 2;
				break;
			case 'd':
				bytes += mul * 4;
				break;
			case 'q':
				bytes += mul * 8;
				break;
			}
		}
		addCVar();
		bssAndDataLabels.chop(3);
		bssAndDataLabels += ";\n\n" + defines + "\n} static " + structName + ";\n\n";
	}

	/* Data labels */
	bssAndDataLabels += "struct packed {\n";
	{
		const auto convertHex = [](QByteArray &v) {
			if (v.endsWith('h'))
			{
				v.chop(1);
				if (v.startsWith('0') && v.length() > 1)
					v.remove(0, 1);
			}
		};

		const QString structName = "_data";
		QString label;
		QList<QPair<QByteArray, char>> values;
		QByteArray initialization, defines;

		const auto addCVar = [&structName, &label, &values, &bssAndDataLabels, &initialization, &defines] {
			if (!values.isEmpty())
			{
				bool hasPointers = false /*, hasStrings = false*/;

				for (auto &v : values)
				{
					if (v.second == 'p')
						hasPointers = true;
//					else if (v.second == 's')
//						hasStrings = true;
				}
				if (hasPointers)
				{
					for (auto &v : values)
					{
						switch (v.second)
						{
							case 'p':
							{
								int plusIdx = v.first.indexOf('+');
								if (plusIdx > -1)
								{
									if (v.first.endsWith('h'))
									{
										v.first.chop(1);
										if (v.first[++plusIdx] == '0')
											v.first.insert(plusIdx + 1, 'x');
										else
											v.first.insert(plusIdx, "0x");
									}
								}
								break;
							}
							case '4':
								if (v.first == "0x0")
								{
									v.first = "NULL";
									v.second = 'p';
								}
								break;
						}
					}
				}

#if TODO_STR
				if (hasStrings)
				{
					QList<QPair<QString, char>> tmpVals;
					for (auto &v : values)
						if (v.second != '1' || tmpVals.isEmpty() || tmpVals.last().second != 's')
						{
							if (v.second == 's')
							{
								v.first.prepend('"');
								v.first.replace("\\", "\\\\");
							}
							tmpVals += v;
						}
						else
						{
							if (!v.first.startsWith("0x"))
								throw runtime_error("Byte must be in HEX");
							bool ok;
							uint val = v.first.toUInt(&ok, 16);
							if (!ok)
								throw runtime_error("Error during parsing string");
							switch (val)
							{
							case '\0':
								tmpVals.last().first += "\\x00";
								break;
							case '\n':
								tmpVals.last().first += "\\n";
								break;
							case '\r':
								tmpVals.last().first += "\\r";
								break;
							case '\'':
								tmpVals.last().first += "'";
								break;
							default:
								throw runtime_error("Error: " + QString::number(val).toStdString());
							}
						}
					values.clear();
					for (auto &v : tmpVals)
					{
						if (v.second == 's')
							v.first.append('"');
						values += v;
					}
				}
#endif

				bssAndDataLabels += "\tstruct packed {";

				quint32 varNum = 0, arrLen = 0;
				char lastType = 0;

				const auto flushField = [&lastType, &varNum, &arrLen, &bssAndDataLabels] {
					if (lastType == 0)
						return;
					switch (lastType)
					{
					case '1':
						bssAndDataLabels += "int8_t ";
						break;
					case '2':
						bssAndDataLabels += "int16_t ";
						break;
					case '4':
						bssAndDataLabels += "int32_t ";
						break;
					case 'f':
						bssAndDataLabels += "float ";
						break;
					case 'd':
						bssAndDataLabels += "double ";
						break;
					case 'p':
						bssAndDataLabels += "void *";
						break;
					case 's':
						bssAndDataLabels += "char ";
						break;
					default:
						throw runtime_error("Unsupported type: " + string(1, lastType));
					}
					bssAndDataLabels += "x" + QString("%1").arg(varNum++, 2, 16, QChar('0')).toUpper() + ((arrLen > 1 || lastType == 's') ? QString("[%1];").arg(arrLen) : ";");
				};

				for (auto &v : values)
				{
					if (v.second == lastType)
						++arrLen;
					else
					{
						flushField();
						lastType = v.second;
						if (lastType == 's')
						{
#ifdef TODO_STR
							arrLen = v.first.length() - v.first.count("\\r") + v.first.count("\\\\r") - v.first.count("\\n") + v.first.count("\\\\n") - v.first.count("\\x00") * 3 - v.first.count("\\\\") - 2;
							if (v.first.endsWith("\\x00\""))
							{
								v.first.chop(5);
								v.first += '"';
							}
#else
							arrLen = v.first.length();
							v.first = '"' + v.first.replace("\\", "\\\\") + '"';
#endif
							flushField();
							lastType = 0;
						}
						arrLen = 1;
					}
				}
				flushField();

				defines += "#define " + label + " ((int8_t *)&" + structName + "." + label + ")\n";
				bssAndDataLabels += "} " + label + ";\n";

				initialization += "\t{";
				for (const auto &v : values)
					initialization += v.first + ",";
				initialization.chop(1);
				initialization += "}, /* " + label + " */" + "\n";
			}
		};

		const int idx = asmF.indexOf("section .text") + 1;
		for (QByteArray line : asmF.mid(idx, asmF.indexOf("section .bss") - idx))
		{
			if (line.startsWith(';'))
				continue;
			if (!line.contains("'") || (line.indexOf(';') > -1 && line.indexOf("'") > line.indexOf(';')))
			{
				int semicolonIdx = line.indexOf(';');
				if (semicolonIdx > -1)
				{
					line.remove(semicolonIdx, line.length() - semicolonIdx);
					while (line.endsWith(' '))
						line.chop(1);
				}
			}
			if (line.isEmpty())
				continue;
			if (line.startsWith('\t'))
				line.remove(0, 1);

			if (line.startsWith("section "))
				continue;

			int colonIdx = line.indexOf(':');
			if (colonIdx > -1 && (line.indexOf('\'') < 0 || line.indexOf('\'') > colonIdx))
			{
				addCVar();
				values.clear();
				label = line.mid(0, colonIdx);
				line = line.remove(0, colonIdx + 2);
			}

			quint32 mul = interpretTimes(line);

			const char s = line[1];
			line.remove(0, 3);
			QList<QByteArray> vals;
			if (s != 'b' || !line.contains("'"))
			{
				vals = line.replace(" ", "").split(',');
				if (mul > 1 && vals.count() > 1)
					throw runtime_error("Bad \"times\" usage");
			}

			switch (s)
			{
			case 'b':
				if (!vals.isEmpty()) //only values
				{
					for (QByteArray v : vals)
					{
						convertHex(v);
						for (quint32 imul = 0; imul < mul; ++imul)
							values += {"0x" + v, '1'};
					}
				}
				else //contains string
				{
					if (mul > 1)
						throw runtime_error("String mustn't use \"times\"");

					if ((line.count('\'') % 2))
						throw runtime_error("Bad string");

					QList<QByteArray> parts = line.split(',');

					bool strIsOpen = false;
					QByteArray str;
					for (QByteArray p : parts)
					{
						bool isString = false;
						if (p.startsWith('\''))
						{
							isString = strIsOpen = true;
							p.remove(0, 1);
						}
						if (p.endsWith('\''))
						{
							isString = true;
							strIsOpen = false;
							p.chop(1);
						}
						if (isString)
						{
							if (!str.isEmpty())
								str += ',';
							str += p;
							if (!strIsOpen)
							{
								values += {str, 's'};
								str.clear();
							}
						}
						else //values
						{
							p = p.replace(" ", "");
							convertHex(p);
							values += {"0x" + p, '1'};
						}
					}
				}
				break;
			case 'w': //only values
				for (QByteArray v : vals)
				{
					convertHex(v);
					for (quint32 imul = 0; imul < mul; ++imul)
						values += {"0x" + v, '2'};
				}
				break;
			case 'd': //value, float, pointer
				for (QByteArray v : vals)
				{
					if (v.contains('.'))
					{
						for (quint32 imul = 0; imul < mul; ++imul)
							values += {v + 'f', 'f'};
					}
					else
					{
						if (!QChar(v[0]).isDigit())
						{
							for (quint32 imul = 0; imul < mul; ++imul)
								values += {v, 'p'};
						}
						else
						{
							convertHex(v);
							for (quint32 imul = 0; imul < mul; ++imul)
								values += {"0x" + v, '4'};
						}
					}
				}
				break;
			case 'q': //only doubles
				for (const QByteArray &v : vals)
				{
					for (quint32 imul = 0; imul < mul; ++imul)
						values += {v, 'd'};
				}
				break;
			}
		}
		addCVar();

		bssAndDataLabels += "\n" + defines + "\n} static " + structName + " = {\n" + initialization + "};\n";
	}
	return bssAndDataLabels;
}

QString bitsStrfromSize(quint32 size, const QString &suffix)
{
	switch (size)
	{
		case 1:
			return "8" + suffix;
		case 2:
			return "16" + suffix;
		case 4:
			return "32" + suffix;
		case 8:
			return "64" + suffix;
		default:
			throw runtime_error("Unknown size: " + to_string(size));
	}
}
