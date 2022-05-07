#include <stdio.h>

#include <stdint.h>

#define REG1_READ                  0b00000001
#define REG2_READ                  0b00000010
#define REG1_TO_REG_WRITE_DATA     0b00000100
#define REG2_TO_REG_WRITE_DATA     0b00001000
#define DATA_BUS_TO_REG_WRITE_DATA 0b00010000
#define IMM16_TO_REG_WRITE_DATA    0b00100000
#define ALU_ADD                    0b01000000
#define ALU_SUB                    0b10000000

#define ALU_NOR                    0b00000001
#define ALU_NAND                   0b00000010
#define IMM16_TO_REG2_DATA         0b00000100
#define REG2_TO_ADDRESS_BUS        0b00001000
#define ALU_COMPARE                0b00010000

#define REG1_WRITE                 0b00000001
#define REG2_WRITE                 0b00000010
#define PC_COUNT_WRITE             0b00000100
#define FLAGS_WRITE                0b00001000



uint8_t microcode[256][3] = {
	{ 0, 0, 0 }, // NOP
	{ REG2_READ | REG2_TO_REG_WRITE_DATA, 0, REG1_WRITE }, // MOV <reg> <reg>
	{ IMM16_TO_REG_WRITE_DATA, 0, REG1_WRITE }, // MOV <reg> <imm16>
	{ 0, 0, 0 }, // OUT <imm16> <reg>
	{ 0, 0, 0 }, // INP <imm16> <reg>
	{ 0, 0, 0 }, // JNZ <imm16>
	{ 0, 0, 0 }, // JNZ <reg>
	{ REG1_READ | REG2_READ | ALU_ADD, 0, REG1_WRITE }, // ADD <reg> <reg>
	{ REG1_READ | ALU_ADD, IMM16_TO_REG2_DATA, REG1_WRITE }, // ADD <reg> <imm16>
	{ REG1_READ | REG2_READ | ALU_SUB, 0, REG1_WRITE }, // SUB <reg> <reg>
	{ REG1_READ | ALU_SUB, IMM16_TO_REG2_DATA, REG1_WRITE }, // SUB <reg> <imm16>
	{ REG1_READ | REG2_READ, ALU_NAND, REG1_WRITE }, // NAD <reg> <reg>
	{ REG1_READ, ALU_NAND | IMM16_TO_REG2_DATA, REG1_WRITE }, // NAD <reg> <imm16>
	{ REG1_READ | REG2_READ, ALU_NOR, REG1_WRITE }, // NOR <reg> <reg>
	{ REG1_READ, ALU_NOR | IMM16_TO_REG2_DATA, REG1_WRITE }, // NOR <reg> <imm16>
	{ 0, 0, 0 }, // CMP <reg> <reg>
	{ 0, 0, 0 }, // CMP <reg> <imm16>
	{ 0, 0, 0 }, // JZR <imm16> 
	{ 0, 0, 0 }, // JZR <reg>
	{ 0, 0, 0 }, // LDR <reg> <imm16>
	{ 0, 0, 0 }, // LDR <reg> <reg>
	{ 0, 0, 0 }, // WTR <reg> <imm16>
	{ 0, 0, 0 }, // WTR <reg> <reg> 
	{ 0, 0, 0 }, // SWP <reg>
	{ 0, IMM16_TO_REG2_DATA | REG2_TO_ADDRESS_BUS, PC_COUNT_WRITE }, // JMP <imm16>
	{ REG2_READ, REG2_TO_ADDRESS_BUS, PC_COUNT_WRITE }, // JMP <reg>
};

int main() {
	FILE* eeprom1 = fopen("eeprom1.bin", "wb");
	FILE* eeprom2 = fopen("eeprom2.bin", "wb");
	FILE* eeprom3 = fopen("eeprom3.bin", "wb");

	for (int i = 0; i < 256; i++) {
		uint8_t microcode_p1 = microcode[i][0];
		uint8_t microcode_p2 = microcode[i][1];
		uint8_t microcode_p3 = microcode[i][2];

		printf("%d: %x %x %x\n", i, microcode_p1, microcode_p2, microcode_p3);

		fseek(eeprom1, i, SEEK_SET);
		fseek(eeprom2, i, SEEK_SET);
		fseek(eeprom3, i, SEEK_SET);

		fwrite(&microcode_p1, 1, 1, eeprom1);
		fwrite(&microcode_p2, 1, 1, eeprom2);
		fwrite(&microcode_p3, 1, 1, eeprom3);
	}

	fclose(eeprom1);
	fclose(eeprom2);
	fclose(eeprom3);
}