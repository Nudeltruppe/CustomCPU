#include <EEPROM.h>

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

// #define CPU_STATE_CONTAINS_RAM_ROM_IO

uint8_t addr_and_data_pins[] = { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, A0, A1, A2, A3 };
uint8_t addr_hold_pin = A4;
uint8_t rw_pin = A5;
uint8_t en_pin = 1;

#ifdef __GNUC__
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif
#ifdef _MSC_VER
#pragma warning(disable : 4996) //_CRT_SECURE_NO_WARNINGS
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#endif

typedef PACK(struct instruction  {
	uint8_t opcode;
	uint8_t reg1 : 4;
	uint8_t reg2 : 4;
	uint16_t imm16;
}) instruction_t;

#define FG_ZERO (1 << 0)
#define FG_NOT_ZERO (1 << 1)
#define FG_EQ (1 << 2)
#define FG_NOT_EQ (1 << 3)
#define FG_OVERFLOW (1 << 4)

typedef struct cpu_state {
	uint16_t pc;
	uint16_t fg;
	uint16_t regs[16];

#ifdef CPU_STATE_CONTAINS_RAM_ROM_IO
	uint8_t* rom_ram;

	uint16_t* io_in;
	uint16_t* io_out;
#endif
} cpu_state_t;

char* cpu_reg_names[] = {
	"r0", "r1", "r2", "unk", "unk", "unk", "unk", "unk", "unk", "unk", "unk", "unk", "sp"
};

#ifdef CPU_STATE_CONTAINS_RAM_ROM_IO
cpu_state_t init_cpu_state(FILE* rom) {
	cpu_state_t state = {
		.pc = 0,
		.fg = 0,
		.regs = { 0 },

		.rom_ram = malloc(0xff),

		.io_in = malloc(sizeof(uint16_t) * 0xffff),
		.io_out = malloc(sizeof(uint16_t) * 0xff)
	};

	
	assert(state.rom_ram != NULL);

	memset(state.rom_ram, 0, 0xffff);

	fseek(rom, 0, SEEK_END);
	size_t size = ftell(rom);
	fseek(rom, 0, SEEK_SET);

	assert(size <= 0xffff);

	fread(state.rom_ram, size, 1, rom);

	return state;
}
#else
cpu_state_t init_cpu_state() {
	cpu_state_t state = {
		.pc = 0,
		.fg = 0,
		.regs = { 0 },
	};

	return state;
}
#endif

void uninit_cpu_state(cpu_state_t state) {
#ifdef CPU_STATE_CONTAINS_RAM_ROM_IO
	free(state.rom_ram);
	free(state.io_in);
	free(state.io_out);
#endif
}

#define DEBUG

#ifdef DEBUG
#define debugf(...) { char buf[128] = { 0 }; sprintf(buf, __VA_ARGS__); Serial.print(buf); Serial.print('\r'); }
#else
#define debugf(...)
#endif

#ifdef CPU_STATE_CONTAINS_RAM_ROM_IO
uint8_t cpu_fetch_byte(cpu_state_t* state, uint16_t addr) {
	debugf("fetching byte at 0x%x\n", addr);
	return state->rom_ram[addr];
}

void cpu_write_byte(cpu_state_t* state, uint16_t addr, uint8_t val) {
	debugf("writing byte 0x%x to 0x%x\n", val, addr);
	state->rom_ram[addr] = val;
}

uint16_t cpu_read_io(cpu_state_t* state, uint16_t addr) {
	debugf("reading io 0x%x\n", addr);
	return state->io_in[addr];
}

void cpu_write_io(cpu_state_t* state, uint16_t addr, uint16_t val) {
	debugf("writing io 0x%x to 0x%x\n", val, addr);
	state->io_out[addr] = val;
}
#else
void __addr_write(uint16_t addr) {
	for (int i = 0; i < sizeof(addr_and_data_pins); i++) {
		pinMode(addr_and_data_pins[i], OUTPUT);
		digitalWrite(addr_and_data_pins[i], addr & (1 << i) ? HIGH : LOW);
	}

	digitalWrite(addr_hold_pin, HIGH);
}

void __write(uint16_t addr, uint8_t data) {
	if (addr > EEPROM.length()) {
		__addr_write(addr);

		for (int i = 0; i < sizeof(addr_and_data_pins); i++) {
			pinMode(addr_and_data_pins[i], OUTPUT);
			digitalWrite(addr_and_data_pins[i], data & (1 << i) ? HIGH : LOW);
		}

		digitalWrite(rw_pin, HIGH); // set mode to write

		digitalWrite(en_pin, HIGH); // set enable pin
		delay(10);
		digitalWrite(en_pin, LOW);
		digitalWrite(addr_hold_pin, LOW);
	} else {
		EEPROM.write(addr, data);
	}
}

uint8_t __read(uint16_t addr) {
	if (addr > EEPROM.length()) {
		__addr_write(addr);

		digitalWrite(rw_pin, LOW); // set mode to read
		digitalWrite(en_pin, HIGH); // set enable pin

		delay(10);

		uint8_t data = 0;

		for (int i = 0; i < sizeof(uint8_t); i++) {
			pinMode(addr_and_data_pins[i], INPUT);
			data |= digitalRead(addr_and_data_pins[i]) << i;
		}

		digitalWrite(en_pin, LOW);
		digitalWrite(addr_hold_pin, LOW);
		return data;
	} else {
		return EEPROM.read(addr);
	}

}

uint8_t cpu_fetch_byte(cpu_state_t* state, uint16_t addr) {
	debugf("fetching byte at 0x%x\n", addr);
	return __read(addr);
}

void cpu_write_byte(cpu_state_t* state, uint16_t addr, uint8_t val) {
	debugf("writing byte 0x%x to 0x%x\n", val, addr);
	__write(addr, val);
}

uint16_t cpu_read_io(cpu_state_t* state, uint16_t addr) {
	debugf("reading io 0x%x\n", addr);
	return 0x0;
}

void cpu_write_io(cpu_state_t* state, uint16_t addr, uint16_t val) {
	debugf("writing io 0x%x to 0x%x\n", val, addr);
}
#endif

void cpu_tick(cpu_state_t* state) {
	instruction_t instruction = { 0 };
	for (int i = 0; i < sizeof(instruction_t); i++) {
		((uint8_t*) &instruction)[i] = cpu_fetch_byte(state, state->pc + i);
	}

#ifdef __GNUC__
	if (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__) {
		instruction.imm16 = __builtin_bswap16(instruction.imm16);
	}
#endif

	debugf("0x%x: 0x%x 0x%x 0x%x 0x%x (0x%x)\n" , state->pc,  instruction.opcode, instruction.reg1, instruction.reg2, instruction.imm16, instruction);

	switch (instruction.opcode)
	{
	case 0x00:
		break;

	case 0x01:
		debugf("mov %s, %s\n", cpu_reg_names[instruction.reg1], cpu_reg_names[instruction.reg2]);
		state->regs[instruction.reg1] = state->regs[instruction.reg2];
		break;

	case 0x02:
		debugf("lod %s, 0x%x\n", cpu_reg_names[instruction.reg1], instruction.imm16);
		state->regs[instruction.reg1] = instruction.imm16;
		break;

	case 0x03:
		debugf("out 0x%x, %s\n", instruction.imm16, cpu_reg_names[instruction.reg2]);
		cpu_write_io(state, instruction.imm16, state->regs[instruction.reg2]);
		break;

	case 0x04:
		debugf("inp 0x%x, %s\n", instruction.imm16, cpu_reg_names[instruction.reg1]);
		state->regs[instruction.reg1] = cpu_read_io(state, instruction.imm16);
		break;

	case 0x05:
		debugf("jnz 0x%x\n", instruction.imm16);
		if ((state->fg & FG_NOT_ZERO) != 0) {
			debugf("exec jmp\n");
			state->pc = instruction.imm16;
			return;
		}
		break;

	case 0x06:
		debugf("jnz %s\n", cpu_reg_names[instruction.reg2]);
		if ((state->fg & FG_NOT_ZERO) != 0) {
			debugf("exec jmp\n");
			state->pc = state->regs[instruction.reg2];
			return;
		}
		break;

	case 0x07:
		debugf("add %s, %s\n", cpu_reg_names[instruction.reg1], cpu_reg_names[instruction.reg2]);
		state->regs[instruction.reg1] += state->regs[instruction.reg2];
		break;

	case 0x08:
		debugf("add %s, 0x%x\n", cpu_reg_names[instruction.reg1], instruction.imm16);
		state->regs[instruction.reg1] += instruction.imm16;
		break;

	case 0x09:
		debugf("sub %s, %s\n", cpu_reg_names[instruction.reg1], cpu_reg_names[instruction.reg2]);
		state->regs[instruction.reg1] -= state->regs[instruction.reg2];
		break;

	case 0x0a:
		debugf("sub %s, 0x%x\n", cpu_reg_names[instruction.reg1], instruction.imm16);
		state->regs[instruction.reg1] -= instruction.imm16;
		break;

	case 0x0b:
		debugf("nad %s, %s\n", cpu_reg_names[instruction.reg1], cpu_reg_names[instruction.reg2]);
		state->regs[instruction.reg1] = ~(state->regs[instruction.reg1] & state->regs[instruction.reg2]);
		break;

	case 0x0c:
		debugf("nad %s, 0x%x\n", cpu_reg_names[instruction.reg1], instruction.imm16);
		state->regs[instruction.reg1] = ~(state->regs[instruction.reg1] & instruction.imm16);
		break;

	case 0x0d:
		debugf("nor %s, %s\n", cpu_reg_names[instruction.reg1], cpu_reg_names[instruction.reg2]);
		state->regs[instruction.reg1] = ~(state->regs[instruction.reg1] | state->regs[instruction.reg2]);
		break;

	case 0x0e:
		debugf("nor %s, 0x%x\n", cpu_reg_names[instruction.reg1], instruction.imm16);
		state->regs[instruction.reg1] = ~(state->regs[instruction.reg1] | instruction.imm16);
		break;

	case 0x0f:
		debugf("cmp %s, %s\n", cpu_reg_names[instruction.reg1], cpu_reg_names[instruction.reg2]);
		state->fg = 0;

		if (state->regs[instruction.reg1] == state->regs[instruction.reg2]) {
			state->fg |= FG_EQ;
			debugf("FG_EQ\n");
		}
		else {
			state->fg |= FG_NOT_EQ;
			debugf("FG_NOT_EQ\n");
		}

		if (state->regs[instruction.reg1] == 0) {
			state->fg |= FG_ZERO;
			debugf("FG_ZERO\n");
		}
		else {
			state->fg |= FG_NOT_ZERO;
			debugf("FG_NOT_ZERO\n");
		}
		break;

	case 0x10:
		debugf("cmp %s, 0x%x\n", cpu_reg_names[instruction.reg1], instruction.imm16);
		state->fg = 0;

		if (state->regs[instruction.reg1] == instruction.imm16) {
			state->fg |= FG_EQ;
			debugf("FG_EQ\n");
		}
		else {
			state->fg |= FG_NOT_EQ;
			debugf("FG_NOT_EQ\n");
		}

		if (state->regs[instruction.reg1] == 0) {
			state->fg |= FG_ZERO;
			debugf("FG_ZERO\n");
		}
		else {
			state->fg |= FG_NOT_ZERO;
			debugf("FG_NOT_ZERO\n");
		}
		break;

	case 0x11:
		debugf("jzr 0x%x\n", instruction.imm16);
		if ((state->fg & FG_ZERO) != 0) {
			debugf("exec jmp\n");
			state->pc = instruction.imm16;
			return;
		}
		break;

	case 0x12:
		debugf("jzr %s\n", cpu_reg_names[instruction.reg2]);
		if ((state->fg & FG_ZERO) != 0) {
			debugf("exec jmp\n");
			state->pc = state->regs[instruction.reg2];
			return;
		}
		break;

	case 0x13:
		debugf("ldr %s, 0x%x\n", cpu_reg_names[instruction.reg1], instruction.imm16);
		state->regs[instruction.reg1] = cpu_fetch_byte(state, instruction.imm16);
		break;

	case 0x14:
		debugf("ldr %s, %s\n", cpu_reg_names[instruction.reg1], cpu_reg_names[instruction.reg2]);
		state->regs[instruction.reg1] = cpu_fetch_byte(state, state->regs[instruction.reg2]);
		break;

	case 0x15:
		debugf("wtr %s, 0x%x\n", cpu_reg_names[instruction.reg1], instruction.imm16);
		cpu_write_byte(state, instruction.imm16, state->regs[instruction.reg1]);
		break;

	case 0x16:
		debugf("wtr %s, %s\n", cpu_reg_names[instruction.reg1], cpu_reg_names[instruction.reg2]);
		cpu_write_byte(state, state->regs[instruction.reg2], state->regs[instruction.reg1]);
		break;

	case 0x17:
		debugf("swp %s\n", cpu_reg_names[instruction.reg1]);
		state->regs[instruction.reg1] = ((state->regs[instruction.reg1] & 0x00FF) << 8) | ((state->regs[instruction.reg1] & 0xFF00) >> 8);
		break;

	case 0x18:
		debugf("jmp 0x%x\n", instruction.imm16);
		state->pc = instruction.imm16;
		return;

	case 0x19:
		debugf("jmp %s\n", cpu_reg_names[instruction.reg2]);
		state->pc = state->regs[instruction.reg2];
		return;

	case 0x1a:
		debugf("jeq 0x%x\n", instruction.imm16);
		if ((state->fg & FG_EQ) != 0) {
			debugf("exec jmp\n");
			state->pc = instruction.imm16;
			return;
		}
		break;

	case 0x1b:
		debugf("jeq %s\n", cpu_reg_names[instruction.reg2]);
		if ((state->fg & FG_EQ) != 0) {
			debugf("exec jmp\n");
			state->pc = state->regs[instruction.reg2];
			return;
		}
		break;

	case 0x1c:
		debugf("jnq 0x%x\n", instruction.imm16);
		if ((state->fg & FG_NOT_EQ) != 0) {
			debugf("exec jmp\n");
			state->pc = instruction.imm16;
			return;
		}
		break;

	case 0x1d:
		debugf("jnq %s\n", cpu_reg_names[instruction.reg2]);
		if ((state->fg & FG_NOT_EQ) != 0) {
			debugf("exec jmp\n");
			state->pc = state->regs[instruction.reg2];
			return;
		}
		break;
	}

	state->pc += sizeof(instruction_t);
}

cpu_state_t cpu_state;

const unsigned char eeprom_content[] = {
    0x02, 0x00, 0x10, 0x00, 0x02, 0x01, 0x10, 0x00, 0x03, 0x10, 0x00, 0x00, 0x08, 0x00, 0x01, 0x00, 
    0x0a, 0x00, 0x01, 0x00, 0x0f, 0x10, 0x00, 0x00, 0x1a, 0x00, 0x20, 0x00, 0x03, 0x00, 0xf0, 0x00, 
    0x02, 0x02, 0xf0, 0xf0, 0x03, 0x20, 0xff, 0x00, 0x18, 0x00, 0xff, 0xff, 
};
const unsigned int eeprom_content_len = 44;

void setup() {
	Serial.begin(9600);

	pinMode(addr_hold_pin, OUTPUT);
	pinMode(rw_pin, OUTPUT);
	pinMode(en_pin, OUTPUT);

	cpu_state = init_cpu_state();

	for (int i = 0; i < sizeof(eeprom_content); i++) {
		cpu_write_byte(&cpu_state, i, eeprom_content[i]);
	}
}

void loop() {
	cpu_tick(&cpu_state);

	if (cpu_state.pc == 0xffff) {
		debugf("halted\n");
		while (1);
	}

	delay(1000);
}
