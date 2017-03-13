#ifndef INSTRUCTION_HEADER
#define INSTRUCTION_HEADER

#define _CRT_SECURE_NO_WARNINGS

#include "../DisassemblerTypes.h"   //byte, word, dword

#include "InstructionSchemas.h"     //InstructionSchema, InstructionSchemas

#include "../ModRegRM/ModRegRM.h" //ModRegRM

class Instruction
{
public:
	Instruction() {};
	Instruction(byte * opcode, int * index)
	{
		int startingIndex = *index;

		SetInstructionSchema(opcode, index);

		for (int operand = 0; operand < NUMBER_OF_OPERANDS; operand++)
		{
			if (IsModRegRM(operand) && !HasModRegRM())
				SetModRegRM(opcode, index);

			else if (IsImmediate(operand))
				SetImmediate(operand, opcode, index);

			else if (IsRelativeDisplacement(operand))
				SetRelativeDisplacement(operand, opcode, index);

			else if (IsMemoryDisplacement(operand))
				SetMemoryDisplacement(operand, opcode, index);

			else if (IsDirectAddress(operand))
				SetDirectAddress(operand, opcode, index);
		}

		SetValue(opcode, startingIndex, *index);
	}
	~Instruction()
	{
		delete modregrm;
		free(value);
	}

	bool HasPrefix()
	{
		return (Prefix::ES <= schema.operator_.prefix) && (schema.operator_.prefix <= Prefix::REPE);
	}
	bool HasGroup()
	{
		return ((Group::Immediate <= schema.operator_.group) && (schema.operator_.group <= Group::TwoByte)) || hasGroup;
	}
	bool HasModRegRM()
	{
		return hasModregrm;
	}
	bool HasOperandPrefix()
	{
		return ((int)prefixes & (int)Prefix::OPERAND) != 0;
	}
	bool HasAddressPrefix()
	{
		return ((int)prefixes & (int)Prefix::ADDRESS) != 0;
	}
	bool HasSegmentPrefix()
	{
		if ((int)prefixes & (int)Prefix::ES)
			return true;
		if ((int)prefixes & (int)Prefix::CS)
			return true;
		if ((int)prefixes & (int)Prefix::SS)
			return true;
		if ((int)prefixes & (int)Prefix::DS)
			return true;
		if ((int)prefixes & (int)Prefix::FS)
			return true;
		if ((int)prefixes & (int)Prefix::GS)
			return true;

		return false;
	}

	bool IsModRegRM(int operand)
	{
		return IsModRM(operand) || IsGeneralRegister(operand) || IsSegmentRegister(operand);
	}
	bool IsModRM(int operand)
	{
		return (schema.operands[operand].addressingMethod == AddressingMethod::E) || (schema.operands[operand].addressingMethod == AddressingMethod::M);
	}
	bool IsGeneralRegister(int operand)
	{
		return schema.operands[operand].addressingMethod == AddressingMethod::G;
	}
	bool IsSegmentRegister(int operand)
	{
		return schema.operands[operand].addressingMethod == AddressingMethod::S;
	}
	bool IsImmediate(int operand)
	{
		return (schema.operands[operand].addressingMethod == AddressingMethod::I);
	}
	bool IsRelativeDisplacement(int operand)
	{
		return (schema.operands[operand].addressingMethod == AddressingMethod::J);
	}
	bool IsMemoryDisplacement(int operand)
	{
		return (schema.operands[operand].addressingMethod == AddressingMethod::O);
	}
	bool IsDirectAddress(int operand)
	{
		return (schema.operands[operand].addressingMethod == AddressingMethod::A);
	}
	bool IsFixedAddress(int operand)
	{
		return (schema.operands[operand].addressingMethod == AddressingMethod::X) || (schema.operands[operand].addressingMethod == AddressingMethod::Y);
	}
	bool IsFixedGeneralRegister(int operand)
	{
		return (Register::A <= schema.operands[operand].register_) && (schema.operands[operand].register_ <= Register::DI);
	}
	bool IsFixedSegmentRegister(int operand)
	{
		return (SegmentRegister::ES <= schema.operands[operand].segmentRegister) && (schema.operands[operand].segmentRegister <= SegmentRegister::GS);
	}
	bool IsFixedConstant(int operand)
	{
		return (Constant::ONE <= schema.operands[operand].constant) && (schema.operands[operand].constant <= Constant::ONE);
	}

	char * GetModRMString(int operand) //free()
	{
		return modregrm->GetModRMString(schema.operands[operand].operandSize);
	}
	const char * GetGeneralRegisterString(int operand)
	{
		return modregrm->GetRegString(schema.operands[operand].operandSize);
	}
	const char * GetSegmentRegisterString(int operand)
	{
		return modregrm->GetRegString(schema.operands[operand].operandSize, true);
	}
	char * GetImmediateString(int operand) //free()
	{
		char * output = NULL;

		switch (schema.operands[operand].operandSize)
		{
			case Size::b:
				Append(&output, "%02Xh", imm8);
				break;
			case Size::w:
				Append(&output, "%04Xh", imm16);
				break;
			case Size::d:
				Append(&output, "%08Xh", imm32);
				break;
			case Size::v:
				if (HasOperandPrefix()) 
					Append(&output, "%04Xh", imm16);
				else if (!HasOperandPrefix())
					Append(&output, "%08Xh", imm32);
				break;
		}

		return output;
	}
	char * GetRelativeDisplacementString(int operand) //free()
	{
		char * output = NULL;

		switch (schema.operands[operand].operandSize)
		{
			case Size::b:
				Append(&output, "%02Xh", disp8);
				break;
			case Size::w:
				Append(&output, "%04Xh", disp16);
				break;
			case Size::d:
				Append(&output, "%08Xh", disp32);
				break;
			case Size::v:
				if (HasOperandPrefix())
					Append(&output, "%04Xh", disp16);
				else if (!HasOperandPrefix())
					Append(&output, "%08Xh", disp32);
				break;
		}

		return output;
	}
	char * GetMemoryDisplacementString(int operand) //free()
	{
		char * output = NULL;

		if (HasSegmentPrefix())
			Append(&output, GetSegmentPrefixString());
		else if (!HasSegmentPrefix())
			Append(&output, "DS:");

		if (HasAddressPrefix())
			Append(&output, "%04Xh", disp16);
		else if (!HasAddressPrefix())
			Append(&output, "%08Xh", disp32);

		return output;
	}
	char * GetDirectAddressString(int operand) //free()
	{
		char * output = NULL;

		Append(&output, GetSizeString(schema.operands[operand].operandSize));

		Append(&output, "%04Xh:", sel16);

		if (HasOperandPrefix())
			Append(&output, "%04Xh", addr16);
		else if (!HasOperandPrefix())
			Append(&output, "%08Xh", addr32);
		
		return output;
	}
	char * GetFixedAddressString(int operand) //free()
	{
		char * output = NULL;

		Append(&output, GetSizeString(schema.operands[operand].operandSize));

		switch (schema.operands[operand].addressingMethod)
		{
			case AddressingMethod::X:
				if (HasSegmentPrefix())
				{
					Append(&output, GetSegmentPrefixString());
					if (HasOperandPrefix())
						Append(&output, "[SI]");
					else if (!HasOperandPrefix())
						Append(&output, "[ESI]");
				}
				else if (!HasSegmentPrefix())
				{
					if (HasOperandPrefix())
						Append(&output, "DS:[SI]");
					else if (!HasOperandPrefix())
						Append(&output, "DS:[ESI]");
				}
				break;
			case AddressingMethod::Y:
				if (HasOperandPrefix())
					Append(&output, "ES:[DI]");
				else if (!HasOperandPrefix())
					Append(&output, "ES:[EDI]");
				break;
		}

		return output;
	}
	const char * GetFixedGeneralRegisterString(int operand)
	{
		switch (schema.operands[operand].operandSize)
		{
			case Size::b:
				return RegisterString[EIGHT_BIT((int)schema.operands[operand].register_ - (int)Register::A)];
				break;
			case Size::w:
				return RegisterString[SIXTEEN_BIT((int)schema.operands[operand].register_ - (int)Register::A)];
				break;
			case Size::d:
				return RegisterString[THIRTYTWO_BIT((int)schema.operands[operand].register_ - (int)Register::A)];
				break;
			case Size::v:
				if (HasOperandPrefix())
					return RegisterString[SIXTEEN_BIT((int)schema.operands[operand].register_ - (int)Register::A)];
				else if (!HasOperandPrefix())
					return RegisterString[THIRTYTWO_BIT((int)schema.operands[operand].register_ - (int)Register::A)];
				break;
		}
	}
	const char * GetFixedSegmentRegisterString(int operand)
	{
		return SegmentRegisterString[(int)schema.operands[operand].segmentRegister - (int)SegmentRegister::ES];
	}
	const char * GetFixedConstantString(int operand)
	{
		return ConstantString[(int)schema.operands[operand].constant - (int)Constant::ONE];
	}
	const char * GetSegmentPrefixString()
	{
		if ((int)prefixes & (int)Prefix::ES)
			return "ES:";
		else if ((int)prefixes & (int)Prefix::CS)
			return "CS:";
		else if ((int)prefixes & (int)Prefix::SS)
			return "SS:";
		else if ((int)prefixes & (int)Prefix::DS)
			return "DS:";
		else if ((int)prefixes & (int)Prefix::FS)
			return "FS:";
		else if ((int)prefixes & (int)Prefix::GS)
			return "GS:";

		return "";
	}
	const char * GetOperatorString()
	{
		return MnemonicString[(int)schema.operator_.mnemonic];
	}
	char * GetOperandString(int operand, bool * allocated)
	{
		if (IsModRM(operand))
		{
			*allocated = true;
			return GetModRMString(operand);
		}

		else if (IsGeneralRegister(operand))
		{
			*allocated = false;
			return (char *)GetGeneralRegisterString(operand);
		}

		else if (IsSegmentRegister(operand))
		{
			*allocated = false;
			return (char *)GetSegmentRegisterString(operand);
		}

		else if (IsImmediate(operand))
		{
			*allocated = true;
			return GetImmediateString(operand);
		}

		else if (IsRelativeDisplacement(operand))
		{
			*allocated = true;
			return GetRelativeDisplacementString(operand);
		}

		else if (IsMemoryDisplacement(operand))
		{
			*allocated = true;
			return GetMemoryDisplacementString(operand);
		}

		else if (IsDirectAddress(operand))
		{
			*allocated = true;
			return GetDirectAddressString(operand);
		}

		else if (IsFixedAddress(operand))
		{
			*allocated = true;
			return GetFixedAddressString(operand);
		}

		else if (IsFixedGeneralRegister(operand))
		{
			*allocated = false;
			return (char *)GetFixedGeneralRegisterString(operand);
		}

		else if (IsFixedSegmentRegister(operand))
		{
			*allocated = false;
			return (char *)GetFixedSegmentRegisterString(operand);
		}

		else if (IsFixedConstant(operand))
		{
			*allocated = false;
			return (char *)GetFixedConstantString(operand);
		}
	}
	char * GetOperandsString() //free()
	{
		char * output = NULL;

		int operand = 0;

		while (schema.operands[operand].addressingMethod != AddressingMethod::_ && operand < NUMBER_OF_OPERANDS)
		{
			bool allocated;
			char * operandString = GetOperandString(operand++, &allocated);
			Append(&output, (const char *)operandString);
			if (allocated)
				free(operandString);
			Append(&output, ", ");
		}

		RemoveLast(&output, 2);

		return output;
	}
	char * GetValueString()
	{
		char * output = NULL;

		for (int i = 0; i < valueSize && i < 8; i++)
			if (i < 7)
				Append(&output, "%02X ", value[i]);
			else
				Append(&output, "+ ");

		RemoveLast(&output, 1);

		return output;
	}
	const char * GetSizeString(Size size)
	{
		switch (size)
		{
			case Size::b:
				return "BYTE PTR ";
			case Size::w:
				return "WORD PTR ";
			case Size::d:
				return "DWORD PTR ";
			case Size::v:
				if (HasOperandPrefix())
					return "WORD PTR ";
				else if (!HasOperandPrefix())
					return "DWORD PTR ";
			case Size::a:
				if (HasOperandPrefix())
					return "DWORD PTR ";
				else if (!HasOperandPrefix())
					return "QWORD PTR ";
			default:
				return "";
		}
	}
	char * GetString() //free()
	{
		char * output = NULL;

		Append(&output, "%- 23s ", GetValueString());
		
		if (!error)
		{
			Append(&output, "%- 6s ", GetOperatorString());

			Append(&output, GetOperandsString());
		}
		else
			Append(&output, "ERROR");

		return output;
	}

private:
	InstructionSchema schema = EmptyInstructionSchema; //Contains the Instruction configuration for the opcode
	ModRegRM * modregrm = NULL;                        //Optional ModRegRM byte
	bool hasModregrm = false;                          //Flag that remembers if the ModRegRM byte has been set
	Prefix prefixes = Prefix::_;                       //Optional instruction prefixes
	byte * value = NULL;                               //Contains the Instruction opcodes
	int valueSize = 0;                                 //Number of value bytes 
	bool hasGroup = false;                             //Flag that remembers if instruction belongs to a group
	bool error = false;                                //If there was an unexpected value
	union                                              //Optional immediate(s)
	{
		struct                                         //8-bit and 16-bit immediates can be accessed at same time
		{
			byte imm8;                                 //8-bit immediate
			word imm16;                                //16-bit immediate
		};
		dword imm32 = 0;                               //32-bit immediate (can't be accessed with 8-bit and 16-bit immediates)
	};
	union                                              //Optional displacement
	{
		byte disp8;                                    //8-bit displacement
		word disp16;                                   //16-bit displacement
		dword disp32 = 0;                              //32-bit displacement
	};
	struct                                             //Optional direct address
	{
		word sel16 = 0;                                //16-bit direct address segement selector
		union
		{
			byte addr8;                                //8-bit direct address (Not used)
			word addr16;                               //16-bit direct address
			dword addr32 = 0;                          //32-bit direct address
		};
	};

	void SetInstructionSchema(byte * opcode, int * index)
	{
		schema = InstructionSchemas[Select<byte>(opcode, index)];

		if (HasPrefix())
		{
			SetPrefix();
			SetInstructionSchema(opcode, index);
		}
		else if (HasGroup())
		{
			SetModRegRM(opcode, index);
			SetGroupSchema();
		}

		if (schema.operator_.mnemonic == Mnemonic::_)
			error = true;
	}
	void SetGroupSchema()
	{
		hasGroup = true;
		schema |= GroupSchemas[(int)schema.operator_.group - (int)Group::Immediate][modregrm->GetOpcodeExtensionValue()];
	}
	void SetPrefix()
	{
		prefixes = (Prefix)((int)prefixes | (int)schema.operator_.prefix);
	}
	void SetModRegRM(byte * opcode, int * index)
	{
		hasModregrm = true;
		modregrm = new ModRegRM(opcode, index, (Prefix)prefixes);
	}

	void SetImmediate(int operand, byte * opcode, int * index)
	{
		switch (schema.operands[operand].operandSize)
		{
			case Size::b:
				imm8 = Select<byte>(opcode, index);
				break;
			case Size::w:
				imm16 = Select<word>(opcode, index);
				break;
			case Size::v:
				if (HasOperandPrefix())
					imm16 = Select<word>(opcode, index);
				else if (!HasOperandPrefix())
					imm32 = Select<dword>(opcode, index);
				break;
		}
	}
	void SetRelativeDisplacement(int operand, byte * opcode, int * index)
	{
		switch (schema.operands[operand].operandSize)
		{
			case Size::b:
				disp8 = Select<byte>(opcode, index);
				break;
			case Size::v:
				if (HasOperandPrefix())
					disp16 = Select<word>(opcode, index);
				else if (!HasOperandPrefix())
					disp32 = Select<dword>(opcode, index);
				break;
		}

		disp32 += *index;
	}
	void SetMemoryDisplacement(int operand, byte * opcode, int * index)
	{
		if (HasAddressPrefix())
			disp16 = Select<word>(opcode, index);
		else if (!HasAddressPrefix())
			disp32 = Select<dword>(opcode, index);
	}
	void SetDirectAddress(int operand, byte * opcode, int * index)
	{
		if (HasOperandPrefix())
			addr16 = Select<word>(opcode, index);
		else if (!HasOperandPrefix())
			addr32 = Select<dword>(opcode, index);

		sel16 = Select<word>(opcode, index);
	}

	void SetValue(byte * opcode, int startingIndex, int endingIndex)
	{
		valueSize = endingIndex - startingIndex;
		value = (byte *)malloc(valueSize);

		memcpy(value, &opcode[startingIndex], endingIndex - startingIndex);
	}
};

#endif