#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct {
	uint8_t opcode;
	uint8_t reg1: 4;
	uint8_t reg2: 4;
	uint16_t imm16;
} __attribute__((packed)) instruction_t;

char* regs[] = {
	"r0", "r1", "r2", "unk", "unk", "unk", "unk", "unk", "unk", "unk", "unk", "unk", "sp"
};

int main(int argc, char** argv) {
	if (argc < 2) {
		printf("Usage: %s <file>\n", argv[0]);
		return 1;
	}

	FILE* rom = fopen(argv[1], "rb");
	
	fseek(rom, 0, SEEK_END);
	size_t size = ftell(rom);
	fseek(rom, 0, SEEK_SET);

	instruction_t* instructions = malloc(size);
	fread(instructions, size, 1, rom);

	for (int i = 0; i < size / sizeof(instruction_t); i++) {
		instruction_t instruction = instructions[i];

		// instruction.imm16 = ntohl(instruction.imm16);

		// swap endianness if necessary
		if (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__) {
			instruction.imm16 = __builtin_bswap16(instruction.imm16);
		}

		printf("0x%lx:\t\t", i * sizeof(instruction_t));

		switch (instruction.opcode) {
			case 0x00:
				printf("NOP\n");
				break;
			
			case 0x01:
				printf("MOV %s, %s\n", regs[instruction.reg1], regs[instruction.reg2]);
				break;
			
			case 0x02:
				printf("LOD %s, 0x%x\n", regs[instruction.reg1], instruction.imm16);
				break;
			
			case 0x03:
				printf("OUT 0x%x, %s\n", instruction.imm16, regs[instruction.reg1]);
				break;

			case 0x04:
				printf("INP 0x%x, %s\n", instruction.imm16, regs[instruction.reg1]);
				break;
			
			case 0x05:
				printf("JNZ 0x%x\n", instruction.imm16);
				break;

			case 0x06:
				printf("JNZ %s\n", regs[instruction.reg2]);
				break;
			
			case 0x07:
				printf("ADD %s, %s\n", regs[instruction.reg1], regs[instruction.reg2]);
				break;

			case 0x08:
				printf("ADD %s, 0x%x\n", regs[instruction.reg1], instruction.imm16);
				break;

			case 0x09:
				printf("SUB %s, %s\n", regs[instruction.reg1], regs[instruction.reg2]);
				break;
			
			case 0x0a:
				printf("SUB %s, 0x%x\n", regs[instruction.reg1], instruction.imm16);
				break;
			
			case 0x0b:
				printf("NAD %s, %s\n", regs[instruction.reg1], regs[instruction.reg2]);
				break;

			case 0x0c:
				printf("NAD %s, 0x%x\n", regs[instruction.reg1], instruction.imm16);
				break;
			
			case 0x0d:
				printf("NOR %s, %s\n", regs[instruction.reg1], regs[instruction.reg2]);
				break;
			
			case 0x0e:
				printf("NOR %s, 0x%x\n", regs[instruction.reg1], instruction.imm16);
				break;

			case 0x0f:
				printf("CMP %s, %s\n", regs[instruction.reg1], regs[instruction.reg2]);
				break;

			case 0x10:
				printf("CMP %s, 0x%x\n", regs[instruction.reg1], instruction.imm16);
				break;

			case 0x11:
				printf("JZR 0x%x\n", instruction.imm16);
				break;
			
			case 0x12:
				printf("JZR %s\n", regs[instruction.reg2]);
				break;
			
			case 0x13:
				printf("LDR %s, 0x%x\n", regs[instruction.reg1], instruction.imm16);
				break;

			case 0x14:
				printf("LDR %s, %s\n", regs[instruction.reg1], regs[instruction.reg2]);
				break;

			case 0x15:
				printf("WTR %s, 0x%x\n", regs[instruction.reg1], instruction.imm16);
				break;
			
			case 0x16:
				printf("WTR %s, %s\n", regs[instruction.reg1], regs[instruction.reg2]);
				break;

			case 0x17:
				printf("SWP %s\n", regs[instruction.reg1]);
				break;
			
			case 0x18:
				printf("JMP 0x%x\n", instruction.imm16);
				break;

			case 0x19:
				printf("JMP %s\n", regs[instruction.reg2]);
				break;
			
			case 0x20:
				printf("JEQ 0x%x\n", instruction.imm16);
				break;

			case 0x21:
				printf("JEQ %s\n", regs[instruction.reg2]);
				break;

			case 0x22:
				printf("JNQ 0x%x\n", instruction.imm16);
				break;

			case 0x23:
				printf("JNQ %s\n", regs[instruction.reg2]);
				break;

			default:
				printf("UNKNOWN\n");
		}
	}

	fclose(rom);
	free(instructions);
}
