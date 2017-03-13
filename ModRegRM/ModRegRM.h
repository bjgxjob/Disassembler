#ifndef MOD_REG_RM_HEADER
#define MOD_REG_RM_HEADER

#define _CRT_SECURE_NO_WARNINGS

#include "../DisassemblerTypes.h"                             //byte, word, dword, Peek<>(), Select<>()

#include "ModRegRMEnums.h"                                    //Mod, Reg, RM, Size
#include "ModRegRMSchemas.h"                                  //ModRegRMSchema, ModRegRMSchemas
#include "ModRegRMStrings.h"                                  //RegString, RMString

#include "../SIB/SIB.h"                                       //SIB

#include "../Instruction/Operator/InstructionOperatorEnums.h" //Prefix
#include "../Instruction/Operand/InstructionOperandStrings.h" //SegmentRegisterString

#include <stdlib.h>                                           //malloc(), free()
#include <stdio.h>                                            //sprintf()
#include <string.h>                                           //strlen(), memcpy()

class ModRegRM
{
public:
	ModRegRM() {};
	ModRegRM(byte * opcode, int * index, Prefix prefixes)
	{
		int startingIndex = *index;

		this->prefixes = prefixes;

		SetModRegRMSchema(opcode, index);

		if (HasSIB())
			SetSIB(opcode, index);

		if (HasDisp())
			SetDisp(opcode, index);

		SetValue(opcode, startingIndex, *index);
	}

	~ModRegRM()
	{
		delete sib;
		free(value);
	}

	bool HasAddressPrefix()
	{
		return ((int)prefixes & (int)Prefix::ADDRESS) != 0;
	}
	bool HasOperandPrefix()
	{
		return ((int)prefixes & (int)Prefix::OPERAND) != 0;
	}
	bool HasSIB()
	{
		return (schema.rm == RM::SIB) && !HasAddressPrefix();
	}
	bool HasDisp()
	{
		return (schema.rm16 == RM::Disp && HasAddressPrefix()) || (schema.rm == RM::Disp && !HasAddressPrefix()) || (schema.mod == Mod::Disp);
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

	Size GetDispSize()
	{
		return dispSize;
	}

	byte GetModValue()
	{
		return (value[0] >> 6) & 0x3;
	}
	byte GetRegValue()
	{
		return (value[0] >> 3) & 0x7;
	}
	byte GetRMValue()
	{
		return value[0] & 0x7;
	}
	byte GetModRMValue()
	{
		return ((value[0] >> 3) & 0x18) | (value[0] & 0x7);
	}
	byte GetOpcodeExtensionValue()
	{
		return (value[0] >> 3) & 0x7;
	}
	template<typename T> T GetDispValue()
	{
		switch (dispSize)
		{
			case Size::b:
				return disp8;
			case Size::w:
				return disp16;
			case Size::d:
				return disp32;
		}
	}
	SIB GetSIBValue()
	{
		return *sib;
	}

	char * GetModRMString(Size size) //free()
	{
		char * output = NULL;

		switch (schema.mod)
		{
			case Mod::NoDisp:
				Append(&output, GetSizeString(size));

				if (HasSegmentPrefix())
					Append(&output, GetSegmentPrefixString());

				Append(&output, "[");

				if (HasRMDisp())
					Append(&output, GetDispString());
				else if (HasSIB())
					Append(&output, sib->GetString());
				else
					Append(&output, GetRMAddressString());

				Append(&output, "]");
				break;
			case Mod::Disp:
				Append(&output, GetSizeString(size));

				if (HasSegmentPrefix())
					Append(&output, GetSegmentPrefixString());

				Append(&output, "[");

				if (HasSIB())
					Append(&output, sib->GetString());
				else
					Append(&output, GetRMAddressString());

				Append(&output, " + ");
				Append(&output, GetDispString());
				Append(&output, "]");
				break;
			case Mod::Reg:
				Append(&output, GetRMRegString(size));
				break;
		}

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
			case Size::p:
				if (HasOperandPrefix())
					return "FWORD PTR ";
				else if (!HasOperandPrefix())
					return "DWORD PTR ";
			default:
				return "";
		}
	}
	const char * GetRegString(Size size, bool segment = false)
	{
		if (!segment)
			switch (size)
			{
				case Size::b:
					return RegString[EIGHT_BIT((int)schema.reg - (int)Reg::A)];
				case Size::w:
					return RegString[SIXTEEN_BIT((int)schema.reg - (int)Reg::A)];
				case Size::d:
					return RegString[THIRTYTWO_BIT((int)schema.reg - (int)Reg::A)];
				case Size::v:
					if (HasOperandPrefix())
						return RegString[SIXTEEN_BIT((int)schema.reg - (int)Reg::A)];
					else if (!HasOperandPrefix())
						return RegString[THIRTYTWO_BIT((int)schema.reg - (int)Reg::A)];
			}
		else if (segment)
			return SegmentRegisterString[(int)schema.reg - (int)Reg::A];
	}
	const char * GetRMAddressString()
	{
		if (HasAddressPrefix())
			return RMString[SIXTEEN_BIT((int)schema.rm16 - (int)RM::A)];
		else if (!HasAddressPrefix())
			return RMString[THIRTYTWO_BIT((int)schema.rm - (int)RM::A)];
	}
	const char * GetRMRegString(Size size)
	{
		switch (size)
		{
			case Size::b:
				return RegString[EIGHT_BIT((int)schema.rm - (int)RM::A)];
			case Size::w:
				return RegString[SIXTEEN_BIT((int)schema.rm - (int)RM::A)];
			case Size::d:
				return RegString[THIRTYTWO_BIT((int)schema.rm - (int)RM::A)];
			case Size::v:
				if (HasOperandPrefix())
					return RegString[SIXTEEN_BIT((int)schema.rm - (int)RM::A)];
				else if (!HasOperandPrefix())
					return RegString[THIRTYTWO_BIT((int)schema.rm - (int)RM::A)];
			default:
				return "ERROR";
		}
	}
	char * GetDispString() //free()
	{
		char * output = NULL;

		switch (dispSize)
		{
			case Size::b:
				output = (char *)malloc(4 * sizeof(char));
				sprintf(output, "%02Xh", disp8);
				break;
			case Size::w:
				output = (char *)malloc(6 * sizeof(char));
				sprintf(output, "%04Xh", disp16);
				break;
			case Size::d:
				output = (char *)malloc(10 * sizeof(char));
				sprintf(output, "%08Xh", disp32);
				break;
		}

		return output;
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

private:
	ModRegRMSchema schema = EmptyModRegRMSchema; //Contains the ModRegRM configuration for the opcode
	byte * value = NULL;                         //Contains the ModRegRM opcodes
	SIB * sib = NULL;                            //Optional scale index base byte
	union                                        //Optional displacement
	{
		byte disp8;                              //8-bit version
		word disp16;                             //16-bit version
		dword disp32 = 0;                        //32-bit version
	};
	Size dispSize = Size::_;                     //Represents the size of the displacement stored in the union above
	Prefix prefixes = Prefix::_;                 //Optional instruction prefixes

	void SetModRegRMSchema(byte * opcode, int * index)
	{
		schema = ModRegRMSchemas[Select<byte>(opcode, index)];
	}
	void SetSIB(byte * opcode, int * index)
	{
		sib = new SIB(opcode, index, schema.mod);
	}
	void SetDisp(byte * opcode, int * index)
	{
		if (HasModDisp())
			SetModDisp(opcode, index);
		else if (HasRMDisp())
			SetRMDisp(opcode, index);
	}

	bool HasModDisp()
	{
		return schema.mod == Mod::Disp;
	}
	bool HasRMDisp()
	{
		return (schema.rm16 == RM::Disp && HasAddressPrefix()) || (schema.rm == RM::Disp && !HasAddressPrefix());
	}
	void SetModDisp(byte * opcode, int * index)
	{
		switch (schema.modSize)
		{
			case Size::b:
				disp8 = Select<byte>(opcode, index);
				dispSize = Size::b;
				break;
			case Size::v:
				if (HasAddressPrefix())
				{
					disp16 = Select<word>(opcode, index);
					dispSize = Size::w;
				}

				else if (!HasAddressPrefix())
				{
					disp32 = Select<dword>(opcode, index);
					dispSize = Size::d;
				}
				break;
		}
	}
	void SetRMDisp(byte * opcode, int * index)
	{
		if (HasAddressPrefix())
		{
			disp16 = Select<word>(opcode, index);
			dispSize = Size::w;
		}
		else if (!HasAddressPrefix())
		{
			disp32 = Select<dword>(opcode, index);
			dispSize = Size::d;
		}
	}

	void SetValue(byte * opcode, int startingIndex, int endingIndex)
	{
		value = (byte *)malloc(endingIndex - startingIndex);

		memcpy(value, &opcode[startingIndex], endingIndex - startingIndex);
	}
};

#endif