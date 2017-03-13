#ifndef SIB_HEADER
#define SIB_HEADER

#define _CRT_SECURE_NO_WARNINGS

#include "../DisassemblerTypes.h"      //byte, word, dword, Peek<>(), Select<>()

#include "SIBEnums.h"                  //Scale, Index, Base
#include "SIBSchemas.h"                //SIBSchema, SIBSchemas
#include "SIBStrings.h"                //ScaleString, IndexString, BaseString

#include "../ModRegRM/ModRegRMEnums.h" //Mod

#include <stdlib.h>                    //malloc(), free()
#include <stdio.h>                     //sprintf()
#include <string.h>                    //strlen(), memcpy()

class SIB
{
public:
	SIB() {};
	SIB(byte * opcode, int * index, Mod mod)
	{
		int startingIndex = *index;

		SetSIBSchema(opcode, index);

		hasDisp = (mod == Mod::NoDisp) && (schema.base == Base::EBP);

		if (HasDisp())
			SetDisp(opcode, index);

		SetValue(opcode, startingIndex, *index);
	}

	~SIB()
	{
		free(value);
	}

	bool HasIndex()
	{
		return schema.index != Index::_;
	}
	bool HasScale()
	{
		return schema.scale != Scale::ONE;
	}
	bool HasBase()
	{
		return !hasDisp;
	}
	bool HasDisp()
	{
		return hasDisp;
	}

	byte GetScaleValue()
	{
		return (value[0] >> 6) & 0x3;
	}
	byte GetIndexValue()
	{
		return (value[0] >> 3) & 0x7;
	}
	byte GetBaseValue()
	{
		return value[0] & 0x7;
	}
	dword GetDispValue()
	{
		return disp32;
	}

	const char * GetScaleString()
	{
		return ScaleString[(int)schema.scale];
	}
	const char * GetIndexString()
	{
		return IndexString[(int)schema.index];
	}
	const char * GetBaseString()
	{
		return BaseString[(int)schema.base];
	}
	char * GetDispString() //free()
	{
		char * output = (char *)malloc(10 * sizeof(char));

		sprintf(output, "%08Xh", disp32);

		return output;
	}

	char * GetString() //free()
	{
		char * output = NULL;
		
		if (HasDisp())
			Append(&output, GetDispString());
		else if (HasBase())
			Append(&output, GetBaseString());

		if (HasIndex())
		{
			Append(&output, " + ");
			Append(&output, GetIndexString());
		}

		if (HasScale())
		{
			Append(&output, " * ");
			Append(&output, GetScaleString());
		}

		return output;
	}

private:
	SIBSchema schema = EmptySIBSchema; //Contains SIB configuration for the opcode
	byte * value = NULL;               //Contains the SIB opcodes
	dword disp32 = 0;                  //Only used if there is a 32-bit displacement
	bool hasDisp = false;              //Seperate value used to prevent write-access outside of class

	void SetSIBSchema(byte * opcode, int * index)
	{
		schema = SIBSchemas[Select<byte>(opcode, index)];
	}
	void SetDisp(byte * opcode, int * index)
	{
		disp32 = Select<dword>(opcode, index);
	}

	void SetValue(byte * opcode, int startingIndex, int endingIndex)
	{
		value = (byte *)malloc(endingIndex - startingIndex);

		memcpy(value, &opcode[startingIndex], endingIndex - startingIndex);
	}
};

#endif