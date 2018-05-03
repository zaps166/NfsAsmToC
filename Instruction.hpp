#ifndef INSTRUCTION_HPP
#define INSTRUCTION_HPP

#include "Utils.hpp"

struct Instruction
{
	struct Operand
	{
		QString getBitsStrFromSize() const;
		QString getName() const;

		void repairName();

		QString name;
		quint32 size = 0;
	};

	static void repairOperandsSizes(Instruction::Operand operands[2]);

	Instruction(const QString &line);
	Instruction()
	{}

	bool isOK() const;

	QString label, rep, mnemonic;
	QList<Operand> operands;
};

#endif // INSTRUCTION_HPP
