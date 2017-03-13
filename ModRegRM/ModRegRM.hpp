#ifndef MOD_REG_RM_HEADER
#define MOD_REG_RM_HEADER

#include "../DisassemblerTypes.hpp"                           //byte, word, dword, Peek<>(), Select<>()

#include "ModRegRMEnums.h"                                    //Mod, Reg, RM, Size
#include "ModRegRMSchemas.h"                                  //ModRegRMSchema, ModRegRMSchemas
#include "ModRegRMStrings.h"                                  //RegString, RMString

#include "../SIB/SIB.hpp"                                     //SIB

#include "../Instruction/Operator/InstructionOperatorEnums.h" //Prefix
#include "../Instruction/Operand/InstructionOperandStrings.h" //SegmentRegisterString

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

		SetValue(opcode, &startingIndex, index);
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
		return sib;
	}

	std::string GetModRMString(Size size)
	{
		std::stringstream output;

		switch (schema.mod)
		{
			case Mod::NoDisp:
				output << GetSizeString(size);

				if (HasSegmentPrefix())
					output << GetSegmentPrefixString();

				output << "[";

				if (HasRMDisp())
					output << GetDispString();
				else if (HasSIB())
					output << sib.GetString();
				else
					output << GetRMAddressString();

				output << "]";
				break;
			case Mod::Disp:
				output << GetSizeString(size);

				if (HasSegmentPrefix())
					output << GetSegmentPrefixString();

				output << "[";

				if (HasSIB())
					output << sib.GetString();
				else
					output << GetRMAddressString();

				output << " + " << GetDispString() << "]";
				break;
			case Mod::Reg:
				output << GetRMRegString(size);
				break;
		}

		return output.str();
	}
	std::string GetSizeString(Size size)
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
	std::string GetRegString(Size size, bool segment = false)
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
	std::string GetRMAddressString()
	{
		std::stringstream output;
		
		if (HasAddressPrefix())
			output << RMString[SIXTEEN_BIT ((int)schema.rm16 - (int)RM::A)];
		else if (!HasAddressPrefix())
			output << RMString[THIRTYTWO_BIT ((int)schema.rm - (int)RM::A)];

		return output.str();
	}
	std::string GetRMRegString(Size size)
	{
		std::stringstream output;

		switch (size)
		{
			case Size::b:
				output << RegString[EIGHT_BIT ((int)schema.rm - (int)RM::A)];
				break;
			case Size::w:
				output << RegString[SIXTEEN_BIT ((int)schema.rm - (int)RM::A)];
				break;
			case Size::d:
				output << RegString[THIRTYTWO_BIT ((int)schema.rm - (int)RM::A)];
				break;
			case Size::v:
				if (HasOperandPrefix())
					output << RegString[SIXTEEN_BIT ((int)schema.rm - (int)RM::A)];
				else if (!HasOperandPrefix())
					output << RegString[THIRTYTWO_BIT ((int)schema.rm - (int)RM::A)];
				break;
		}

		return output.str();
	}
	std::string GetDispString()
	{
		std::stringstream output;

		switch (dispSize)
		{
			case Size::b:
				output << std::setfill('0') << std::setw(2) << std::uppercase << std::hex << (int)disp8;
				break;
			case Size::w:
				output << std::setfill('0') << std::setw(4) << std::uppercase << std::hex << disp16;
				break;
			case Size::d:
				output << std::setfill('0') << std::setw(8) << std::uppercase << std::hex << disp32;
				break;
		}

		output << "h";

		return output.str();
	}
	std::string GetSegmentPrefixString()
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
	std::vector<byte> value;                     //Contains the ModRegRM opcodes
	SIB sib;                                     //Optional scale index base byte
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
		sib = SIB(opcode, index, schema.mod);
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

	void SetValue(byte * opcode, int * startingIndex, int * endingIndex)
	{
		while (*startingIndex != *endingIndex)
			value.push_back(Select<byte>(opcode, startingIndex));
	}
};

#endif