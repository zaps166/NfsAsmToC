#include "Instruction.hpp"
#include "Consts.hpp"

#include <QResource>
#include <QDebug>
#include <QFile>
#include <QMap>

#include <iostream>
using namespace std;

int main()
{
	const auto readResource = [](const QString &resPth) {
		QResource res(resPth);
		QByteArray resArr = QByteArray((char *)res.data(), res.size());
		if (res.isCompressed())
			return qUncompress(resArr);
		return resArr;
	};

	AsmFile asmF = readAssemblyFile("NFS2SE.asm");
	const auto &subroutines = asmF.subroutines;
	const auto &jumptables = asmF.jumptables;

	if (asmF.isEmpty())
		throw runtime_error("Cannot open or read assembly file");

	QFile f("NFS2SE.cpp");
	f.open(QFile::WriteOnly);
	f.write(readResource(":/Code1.cpp") + '\n');

	f.write("struct Game : public Application\n{\n");
	for (const auto &s : subroutines)
		f.write("\tVoid _" + s.first.toLatin1() + "();\n");
	f.write("};\n\n");

	for (const auto &s : subroutines)
	{
#if 0
		f.write("static void " + s.first.toLatin1() + "(Game *g){g->_" + s.first.toLatin1() + "();}\n");
#else
		f.write("#define " + s.first.toLatin1() + " ((void(*)(void *))&Game::_" + s.first.toLatin1() + ")\n");
#endif
	}

	f.putChar('\n');
	f.write(getVariables(asmF));
	f.putChar('\n');
	for (auto it = externs.begin(); it != externs.end(); ++it)
	{
		const ExternData &e = it.value();
		QByteArray line = "int32_t " + it.key().toLatin1() + "(";
#if 1
		line.prepend("extern \"C\" ");
#else
		switch (e.callingConvention)
		{
			case ExternData::CDECL:
			case ExternData::CDECL_VARARG:
				line.prepend("CDECL   ");
				break;
			case ExternData::STDCALL:
				line.prepend("STDCALL ");
				break;
			case ExternData::WATCOM_FASTCALL:
				line.prepend("REGPARM ");
				break;
		}
#endif
		for (quint32 i = 0; i < e.numArgs; ++i)
			line += (e.floatArgs.contains(i) ? "float" : "int32_t") + QString(" arg%1, ").arg(i);
		if (e.callingConvention == ExternData::CDECL_VARARG)
			line += "...";
		else if (e.numArgs)
			line.chop(2);

#if 0
		line += "){puts(\"" + it.key().toLatin1() + "\");}\n";
#else
		line += ");\n";
#endif

		f.write(line);
	}
	f.putChar('\n');

	quint32 total = 0, untranslated = 0;

	for (auto it = subroutines.begin(); it != subroutines.end(); ++it)
	{
		static const QMap<quint32, QString> operandSizes = {
			{0, ""},
			{1, "byte"},
			{2, "word"},
			{4, "dword"},
			{8, "qword"}
		};

		static const QVector<QString> registers = {
			"eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
			"ax", "cx", "dx", "bx", "sp", "bp", "si", "di",
			"al", "ah", "cl", "ch", "dl", "dh", "bl", "bh",
			"cs", "ds", "es", "fs", "gs", "ss"
		};
		static const QVector<QString> mnemonicCondJmpToFn = {
			"ja", "jb", "jbe", "jg", "jge", "jl", "jle", "jnb", "jns", "jnz", "js", "jz"
		};
		static const QVector<QString> mnemonic0ArgsToFn = {
			"cpuid", "leave", "pusha", "popa", "pushf", "popf", "movsb", "movsw", "movsd", "stosb", "stosd", "sahf", "lahf"
		};
		static const QVector<QString> mnemonic1ArgsToFn = {
			"inc", "dec", "neg", "setnbe"
		};
		static const QVector<QString> mnemonic2ArgsToFn = {
			"add", "sub", "and", "or", "xor", "test", "adc", "sbb", "cmp", "sar", "shr", "shl", "bsr", "rol", "ror"
		};
		static const QVector<QString> mnemonicMMX = {
			"emms", "movd", "psllq", "por", "movq", "punpcklbw", "pmulhw", "punpckhwd", "punpcklwd", "paddsw", "punpckhbw", "psrlw", "packuswb", "pxor", "psrld", "pmaddwd", "psrlq", "paddw", "psrad", "packssdw", "packsswb", "paddd", "pslld"
		};

		bool clearFunction = false;

		QString funcC = "Void Game::_" + it->first + "()\n{\n";
		const QStringList &code = it->second;
		for (const auto &t : jumptables) //Inserting jumptables into subroutine
			if (t.second == it->first)
				funcC += "\t" + t.first.toLatin1() + '\n';
		const QString subLocLabel = "loc_" + it->first;
		funcC += subLocLabel + ":\n";

		for (int i = 0; i < code.size(); ++i)
		{
			const QString &line = code[i];
			if (line.startsWith('%')) //Macros
			{
				funcC += "#" + line.right(line.length() - 1) + "\n";
			}
			else try
			{
				const Instruction instr(line);
				if (!instr.label.isEmpty())
					funcC += instr.label;
				else
				{
					bool semicolon = true;
					funcC += "\t";
					++total;

					const auto addUntranslatedLine = [&funcC, &instr, &untranslated] {
						funcC += "//" + (instr.rep.isEmpty() ? QString() : instr.rep + " ") + instr.mnemonic;
#if 0
						static QStringList u;
						if (!u.contains(instr.mnemonic))
							cerr << "Untranslated: " << instr.mnemonic.toStdString() << "\n";
						u << instr.mnemonic;
#endif
						for (const Instruction::Operand &o : instr.operands)
						{
							QString operandSizeName = operandSizes[o.size];
							if (!operandSizeName.isEmpty())
								operandSizeName += ' ';
							funcC += " " + operandSizeName + o.name + ',';
						}
						if (!instr.operands.isEmpty())
							funcC.chop(1);
						++untranslated;
					};

					const auto getJump = [&subroutines, &semicolon, &it, &subLocLabel](const QString &label, bool unconditional) -> QString {
						QString jmp;
						bool jumpIsSubroutine = false;
						if (it->first == label)
						{
							jmp = "goto " + subLocLabel;
							jumpIsSubroutine = true;
						}
						else for (const auto &s : subroutines)
						{
							if (s.first == label)
							{
								jmp = "_" + label + "();";
								if (!unconditional)
									jmp.prepend("{");
								if (!label.startsWith("ExitProcess"))
								{
									jmp += " return;";
									if (unconditional)
										jmp += " //jmp";
									else
										jmp += "}";
								}
								jumpIsSubroutine = true;
								semicolon = false;
								break;
							}
						}
						if (!jumpIsSubroutine)
							jmp = "goto " + label;
						return jmp;
					};
					const auto removeFromAddr = [](QString &str) {
						str.remove(0, 6);
						str.chop(1);
					};

					/* REP */
					if (instr.operands.count() == 0 && !instr.rep.isEmpty())
					{
						if (instr.rep == "rep")
							funcC += "while (ecx) //rep\n\t{\n\t\t" + instr.mnemonic + "();\n\t\t--ecx;\n\t}";
						else if (instr.rep == "repe")
							funcC += "while (ecx) //repe\n\t{\n\t\t" + instr.mnemonic + "();\n\t\t--ecx;\n\t\tif (!flags.zf)\n\t\t\tbreak;\n\t}";
						else if (instr.rep == "repne")
							funcC += "while (ecx) //repne\n\t{\n\t\t" + instr.mnemonic + "();\n\t\t--ecx;\n\t\tif (flags.zf)\n\t\t\tbreak;\n\t}";
						else
							throw runtime_error("Unsupported REP: " + instr.rep.toStdString());
					}
					/* Translating instructions */
					else if (instr.mnemonic == "ret" || instr.mnemonic == "retn")
					{
						if (!instr.operands.isEmpty() || i + 1 < code.size())
						{
							if (!instr.operands.isEmpty())
								funcC += "esp += " + instr.operands.first().name + "; ";
							funcC += "return";
						}
					}
					else if (instr.operands.count() == 1 && instr.mnemonic == "jmp")
					{
						QString label = instr.operands.first().name;
						if (!label.contains('['))
							funcC += getJump(label, true);
						else
						{
							if (label.length() == 15)
								label.insert(14, "/4");
							else
							{
								if (label.length() == 17 && label.endsWith("*4]"))
								{
									label.chop(3);
									label += "]";
								}
								else
									throw runtime_error("Unsupported jump");
							}
							funcC += "goto *" + label;
						}
					}
					else if (instr.operands.count() == 1 && instr.mnemonic == "jecxz")
						funcC += "if (!ecx) //jecxz\n\t\tgoto " + instr.operands.at(0).name;
					else if (instr.operands.count() == 1 && (instr.mnemonic == "push" || instr.mnemonic == "pop"))
					{
						const QString bits = instr.operands.first().size == 2 ? "16" : "32";
						QString operand = instr.operands.first().name;
						funcC += instr.mnemonic + bits + "(" + operand.replace("###", bits + "i") + ")";
					}
					else if (instr.operands.count() == 1 && instr.mnemonic == "call")
					{
						QString operand = instr.operands.first().name;
						bool foundSub = false;
						for (const auto &s : subroutines)
						{
							if (s.first == operand)
							{
								funcC += "esp -= 4; _" + operand + "(); esp += 4; //call";
								semicolon = false;
								foundSub = true;
								break;
							}
						}
						if (!foundSub)
						{
							if (!externs.contains(operand))
								funcC += "call(" + operand.replace("###", "32i") + ")";
							else
							{
								const ExternData &e = externs[operand];
								quint32 numArgs = e.numArgs;
								funcC += "eax = " + operand + "(";
								if (!numArgs)
									funcC += ")";
								else
								{
									if (operand == "DispatchMessageA_wrap" || operand == "iSNDdirectserve_" || operand == "DirectInputCreateA_wrap" || operand == "fetchTrackRecords")
									{
										funcC += "this, ";
										--numArgs;
									}
									if (e.callingConvention == ExternData::WATCOM_FASTCALL)
									{
										if (numArgs == 1)
											funcC += "eax";
										else if (numArgs == 2)
											funcC += "eax, edx";
										else
											throw runtime_error("Unsuported argument count for watcom fastcall");
									}
									else
									{
										quint32 moreArgs = operand == "fscanf_wrap" ? 1 : 0; //"fscanf" in this game requires 3 argument (int pointer as third argument)
										for (quint32 i = 0; i < numArgs + moreArgs; ++i)
											funcC += QString("%2(esp + %1), ").arg(i * 4).arg(e.floatArgs.contains(i) ? "*(float *)" : "*(int32_t *)");
										funcC.chop(2);
									}
									funcC += ")";
									if (!operand.startsWith("ExitProcess") && e.callingConvention == ExternData::STDCALL) //STDCALL automatically frees stack, but here we must do it manually - virtual stack is not the same as CPU stack
										funcC += "; esp += " + QString::number(numArgs * 4);
								}
							}
						}
					}
					else if (instr.operands.count() == 2 && (instr.mnemonic == "mov" || instr.mnemonic == "lea"))
					{
						Instruction::Operand operands[2] = { instr.operands[0], instr.operands[1] };
						const bool isLea = instr.mnemonic == "lea";
						if (!isLea)
						{
							Instruction::repairOperandsSizes(operands);
							if (operands[0].name == operands[1].name)
								throw runtime_error("Wasting CPU cycles for MOV");
						}
						else
						{
							const bool hasBrackets = operands[1].name.contains("[") && operands[1].name.contains("]");
							if (!registers.contains(operands[0].name) || (!operands[1].name.startsWith("to###") && !hasBrackets))
								throw runtime_error("Bad LEA usage");
							if (!hasBrackets) //Remove "to###()"
								removeFromAddr(operands[1].name);
							else
								operands[1].name.prepend("&");
							if (operands[0].name == operands[1].name || operands[0].name + "+0" == operands[1].name)
								throw runtime_error("Wasting CPU cycles for LEA");
						}
						funcC += operands[0].name + " = " + operands[1].name + "; //" + (isLea ? "lea" : "mov");
						semicolon = false;
					}
					else if (instr.operands.count() == 2 && instr.mnemonic == "xchg")
					{
						Instruction::Operand operands[2] = {instr.operands.at(0), instr.operands.at(1)};
						Instruction::repairOperandsSizes(operands);
						if (operands[0].size != operands[1].size)
							throw runtime_error("XCHG must have the same operand sizes");
						funcC += "swap(" + operands[0].name + ", " + operands[1].name + ")";
					}
					else if (instr.operands.count() == 2 && (instr.mnemonic == "movsx" || instr.mnemonic == "movzx"))
					{
						Instruction::Operand operands[2] = {instr.operands.at(0), instr.operands.at(1)};
						Instruction::repairOperandsSizes(operands);
						funcC += operands[0].name + " = ";
						if (instr.mnemonic == "movzx")
							funcC += "(uint" + operands[1].getBitsStrFromSize() + "_t)";
						funcC += operands[1].name + "; //" + instr.mnemonic;
						semicolon = false;
					}
					else if (instr.operands.count() == 1 && instr.mnemonic == "bswap")
					{
						const Instruction::Operand &operand = instr.operands.at(0);
						if (operand.size != 4 && !registers.contains(operand.name))
							throw runtime_error("Bad BSWAP usage");
						funcC += operand.name + " = bswap_32(" + operand.name + ")";
					}
					else if (instr.operands.count() == 1 && instr.mnemonic == "not")
					{
						const QString &operand = instr.operands.at(0).name;
						funcC += operand + " = ~" + operand;
					}
					else if (instr.operands.count() == 0 && instr.mnemonic == "cwde")
					{
						//Both must be signed!
						funcC += "eax = ax; //cwde";
						semicolon = false;
					}
					else if (instr.operands.count() == 0 && instr.mnemonic == "cdq")
					{
						//Both must be signed!
						funcC += "edx_eax = eax; //cdq";
						semicolon = false;
					}
					else if (instr.operands.count() == 0 && mnemonic0ArgsToFn.contains(instr.mnemonic))
						funcC += instr.mnemonic + "()";
					/* Modyfikuje flagi */
					else if (instr.operands.count() == 1 && mnemonic1ArgsToFn.contains(instr.mnemonic))
						funcC += instr.mnemonic + "(" + instr.operands.at(0).getName() + ")";
					else if (instr.operands.count() == 2 && mnemonic2ArgsToFn.contains(instr.mnemonic))
					{
						Instruction::Operand operands[2] = {instr.operands.at(0), instr.operands.at(1)};
						Instruction::repairOperandsSizes(operands);

						QString underscore = "";

						if (!registers.contains(operands[1].name) && !operands[1].name.startsWith("to"))
							operands[1].name.prepend("(int" + operands[1].getBitsStrFromSize() + "_t)");

						if (instr.mnemonic == "xor" || instr.mnemonic == "or" || instr.mnemonic == "and")
							underscore = "_";

						funcC += instr.mnemonic + underscore + "(" + operands[0].getName() + ", " + operands[1].getName() + ")";
					}
					else if (instr.mnemonic == "imul")
					{
						funcC += instr.mnemonic + instr.operands.at(0).getBitsStrFromSize() + "(" + instr.operands.at(0).getName();
						if (instr.operands.count() > 1)
						{
							Instruction::Operand operands[2] = {instr.operands.at(0), instr.operands.at(1)};
							Instruction::repairOperandsSizes(operands);
							funcC += ", " + operands[1].getName();
							if (instr.operands.count() == 3)
							{
								operands[0] = operands[1];
								operands[1] = instr.operands.at(2);
								Instruction::repairOperandsSizes(operands);
								funcC += ", " + operands[1].getName();
							}
						}
						funcC += ")";
					}
					else if (instr.operands.count() == 1 && (instr.mnemonic == "div" || instr.mnemonic == "idiv" || instr.mnemonic == "mul"))
					{
						const Instruction::Operand &operand = instr.operands.at(0);
						funcC += instr.mnemonic + operand.getBitsStrFromSize() + "(" + operand.getName() + ")";
					}
					else if (instr.mnemonic == "shrd")
					{
						if (instr.operands.count() != 3)
							throw runtime_error("Incorrect SHRD argument count");

						Instruction::Operand operands[3] = {instr.operands.at(0), instr.operands.at(1), instr.operands.at(2)};
						Instruction::repairOperandsSizes(operands);
						Instruction::repairOperandsSizes(operands + 1);

						funcC += "shrd(";
						for (const auto &operand : operands)
							funcC += operand.getName() + ", ";
						funcC.chop(2);
						funcC += ")";
					}
					/* Czyta flagi (jumps) */
					else if (instr.operands.count() == 1 && mnemonicCondJmpToFn.contains(instr.mnemonic))
						funcC += instr.mnemonic + "(" + getJump(instr.operands.at(0).name, false) + ")";
					/* FPU instructions */
					else if (instr.mnemonic.startsWith('f'))
					{
						switch (instr.operands.count())
						{
							case 0:
								funcC += instr.mnemonic + "()";
								break;
							case 1:
							{
								Instruction::Operand operand = instr.operands.at(0);
								funcC += instr.mnemonic;
								if (operand.name.startsWith("st"))
								{
									if (operand.name.length() != 3)
										throw runtime_error("Incorrect FPU register index");
									funcC += "_st(" + operand.name.mid(2, 1);
								}
								else
								{
									const bool isFloat =
									(
										!instr.mnemonic.startsWith("fild") &&
										!instr.mnemonic.startsWith("fist") &&
										 instr.mnemonic != "fstcw"         &&
										 instr.mnemonic != "fldcw"         &&
										 instr.mnemonic != "fnstsw"
									);
									const bool isStore =
									(
										instr.mnemonic == "fst"  ||
										instr.mnemonic == "fstp"
									);
									funcC += "(";
									if (isFloat && !isStore)
										funcC += operand.name.replace("to###", "get" + bitsStrfromSize(operand.size, "f"));
									else
										funcC += operand.getName();
								}
								funcC += ")";
								break;
							}
							case 2:
							{
								const QString operand0 = instr.operands.at(0).name;
								const QString operand1 = instr.operands.at(1).name;
								if (operand0.startsWith("st") && operand1.startsWith("st") && operand0.length() == 3 && operand1.length() == 3)
									funcC += instr.mnemonic + "_st(" + operand0.mid(2, 1) + ", " + operand1.mid(2, 1) + ")";
								else
									throw runtime_error("Unsupported operands for FPU for 2 operands instructions");
								break;
							}
							default:
								throw runtime_error("Unsupported operand count for FPU: " + to_string(instr.operands.count()));
						}
					}
					/* MMX */
					else if (mnemonicMMX.contains(instr.mnemonic))
					{
						if (instr.mnemonic != "emms")
							clearFunction = true;
					}
					else
						addUntranslatedLine();
					if (semicolon)
						funcC += ';';
				}
				if (funcC.right(2) == "\t;")
					funcC.chop(2);
				else
					funcC += "\n";
			}
			catch (exception &e)
			{
				cout << line.toStdString() << "\t{" << e.what() << "}\n";
//				return -1;
			}
//			break;
		}
#if 1
		if (funcC.endsWith(":\n"))
			funcC += "\treturn;\n";
#else
		funcC += "\treturn;\n";
#endif

		if (clearFunction)
		{
			int idx = funcC.indexOf("\n{");
			if (idx > -1)
			{
				idx += 2;
				funcC.remove(idx, funcC.size() - idx);
				funcC += "\n\t//MMX\n";
			}
		}

		funcC += "}\n";

		if (it->first == "sub_489AE0") //Fix thread creation
		{
			funcC.replace
			(
				"	push32(eax);\n"
				"	push32(StartAddress);",

				"	Game *new_game_ctx = (Game *)malloc(sizeof(Game));\n"
				"	new (new_game_ctx) Game();\n"
				"	push32(new_game_ctx);\n"
				"	push32(eax);"
			);
		}

		if (funcC.count(subLocLabel) == 1)
			funcC.remove(subLocLabel + ":\n");

		f.write(funcC.toUtf8());
	}
	f.write('\n' + readResource(":/Code2.cpp"));
	f.close();

	cout << "Total: " << total << " lines\nUntranslated: " << untranslated << " lines\nProgress: " << (total == 0 ? 0.0 / 0.0 : (10000 - (untranslated * 10000 / total)) / 100.0) << " %\n";

	return 0;
}
