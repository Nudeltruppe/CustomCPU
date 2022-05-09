from psutil import swap_memory
from lexer import TokenTypes
from lexer import Token

OPCODE_MAP = {"nop": 0x00, "mov": 0x01, "lod": 0x02, "out": 0x03, "inp": 0x04, 
              "jnz": 0x05, "add": 0x07, "sub": 0x09, "nad": 0x0b, "nor": 0x0d,
              "cmp": 0x0f, "jzr": 0x11, "ldr": 0x13, "wtr": 0x15, "swp": 0x17,
              "jmp": 0x18, "jeq": 0x1a, "jnq": 0x1c}
REGISTERS = {"r0": 0x00, "r1": 0x01, "r2": 0x02, "sp": 0x12}
LABELS = {}


class Generator:
    def __init__(self, tokens, ep=0x0) -> None:
        self.ep = ep
        self.instruction_counter = 0
        self.final_text = []
        
        self.tokens = tokens
        self.pos = -1
        self.current_token: Token = None
        self.in_check_label = False
        self.advance()
        self.check_label()

    def check_label(self):
        self.in_check_label = True
        self.Gen()
        self.pos = -1
        self.current_token = None
        self.in_check_label = False
        self.instruction_counter = 0
        self.final_text = []
        self.advance()

    def get_imm16(self):
        if self.current_token.type in (TokenTypes.BINARY_IMM16, TokenTypes.DECIMAL_IMM16, TokenTypes.HEXADECIMAL_IMM16, TokenTypes.ID):
            if self.current_token.type == TokenTypes.BINARY_IMM16:
                if int(self.current_token.value, base=0) > 65535:
                    raise Exception("Number: " + str(int(self.current_token.value, base=0)) + " is bigger than 2bytes (65535)")
                i = int(self.current_token.value, base=0)
                i = (i & 0xFF) << 8 | (i & 0xFF00) >> 8
                return i
            elif self.current_token.type == TokenTypes.DECIMAL_IMM16:
                if int(self.current_token.value) > 65535:
                    raise Exception("Number: " + str(int(self.current_token.value, base=0)) + " is bigger than 2bytes (65535)")
                i = int(self.current_token.value)
                i = (i & 0xFF) << 8 | (i & 0xFF00) >> 8
                return i
            elif self.current_token.type == TokenTypes.ID:
                if self.in_check_label:
                    return 0
                if self.current_token.value in LABELS.keys():
                    if int(LABELS[self.current_token.value], base=16) > 65535:
                        raise Exception("Number: " + str(int(self.current_token.value, base=0)) + " is bigger than 2bytes (65535)")
                    i = int(LABELS[self.current_token.value], base=16)
                    i = (i & 0xFF) << 8 | (i & 0xFF00) >> 8
                    return i
                else:
                    raise Exception("Invalid label")
            else:
                if int(self.current_token.value, base=16) > 65535:                    
                    raise Exception("Number: " + str(int(self.current_token.value, base=0)) + " is bigger than 2bytes (65535)")
                i = int(self.current_token.value, base=16)
                i = (i & 0xFF) << 8 | (i & 0xFF00) >> 8
                return i
        else:
            raise Exception("Imm16 was expected but not found")

    def get_imm8(self):
        if self.current_token.type in (TokenTypes.BINARY_IMM16, TokenTypes.DECIMAL_IMM16, TokenTypes.HEXADECIMAL_IMM16, TokenTypes.ID):
            if self.current_token.type == TokenTypes.BINARY_IMM16:
                if int(self.current_token.value, base=0) > 255:
                    raise Exception("Number: " + str(int(self.current_token.value, base=0)) + " is bigger than a byte (255)")
                return int(self.current_token.value, base=0)
            elif self.current_token.type == TokenTypes.DECIMAL_IMM16:
                if int(self.current_token.value) > 255:
                    raise Exception("Number: " + str(int(self.current_token.value, base=0)) + " is bigger than a byte (255)")
                return int(self.current_token.value)
            elif self.current_token.type == TokenTypes.ID:
                if self.in_check_label:
                    return 0
                if self.current_token.value in LABELS.keys():
                    if int(LABELS[self.current_token.value], base=16) > 255:
                        raise Exception("Number: " + str(int(self.current_token.value, base=0)) + " is bigger than a byte (255)")
                    return int(LABELS[self.current_token.value], base=16)
                else:
                    raise Exception("Invalid label")
            else:
                if int(self.current_token.value, base=16) > 255:                    
                    raise Exception("Number: " + str(int(self.current_token.value, base=0)) + " is bigger than a byte (255)")
                return int(self.current_token.value, base=16)
        else:
            raise Exception("Imm8 was expected but not found")

    def make_instruction(self, opcode=0, register1="r0", register2="r0", imm16=0):
        i = "0x{:02x} ".format(opcode) + "{:01x} ".format(REGISTERS[register2]) + "{:01x} ".format(REGISTERS[register1]) + "{:04x} ".format(imm16)
        print(i)
        return "0x{:02x}".format(opcode) + "{:01x}".format(REGISTERS[register2]) + "{:01x}".format(REGISTERS[register1]) + "{:04x}".format(imm16)

    def advance(self):
        self.pos += 1
        self.current_token = self.tokens[self.pos] if self.pos < len(self.tokens) else None
    
    def Gen(self):
        # loop through all tokens
        while self.current_token != None:
            if self.current_token.type == TokenTypes.ID:
                if self.current_token.value in OPCODE_MAP.keys():
                    opcode = self.current_token.value
                    opcode_value = OPCODE_MAP[self.current_token.value]
                    self.advance()
                    # check what the args are. If any are expected and either increment in hex or don't depending on the argument. Then make the whole instruction as encoded binary
                    
                    if opcode_value == 0x00:                           # nop
                        self.final_text.append(self.make_instruction(opcode=0))
                    elif opcode_value == 0x01:                         # mov
                        # check for 2 registers as argument
                        regs = []
                        if self.current_token.type == TokenTypes.ID and self.current_token.value in REGISTERS.keys():
                            regs.append(self.current_token.value)
                            self.advance()
                            
                            if self.current_token.type == TokenTypes.COMMA:
                                self.advance()
                                
                                if self.current_token.type == TokenTypes.ID and self.current_token.value in REGISTERS.keys():
                                    regs.append(self.current_token.value)
                                    self.final_text.append(self.make_instruction(opcode_value, regs[0], regs[1]))
                                    self.advance()
                                else:
                                    raise Exception("Expected second register in mov instruction")
                            else:
                                raise Exception("Second argument for mov expected")
                        else:
                            raise Exception("Expected register in mov instruction")
                    elif opcode_value == 0x02:                         # lod
                        # check for reg and imm16
                        register = ""
                        imm16 = ""
                        
                        if self.current_token.type == TokenTypes.ID and self.current_token.value in REGISTERS.keys():
                            register = self.current_token.value
                            self.advance()
                            
                            if self.current_token.type == TokenTypes.COMMA:
                                self.advance()
                                
                                imm16 = self.get_imm16()
                                    
                                self.advance()
                                self.final_text.append(self.make_instruction(opcode=opcode_value, register1=register, imm16=imm16))
                            else:
                                raise Exception("expected second argument")
                        else:
                            raise Exception("Expected arguments for lod")
                    elif opcode_value == 0x03: # out
                        imm16 = self.get_imm16()
                        register = ""
                        self.advance()
                        
                        if self.current_token.type == TokenTypes.COMMA:
                            self.advance()
                            
                            if self.current_token.type == TokenTypes.ID and self.current_token.value in REGISTERS.keys():
                                register = self.current_token.value
                                
                                self.final_text.append(self.make_instruction(opcode=opcode_value, imm16=imm16, register2=register))
                                self.advance()
                            else:
                                raise Exception("Expected register as second argument for out")
                        else:
                            raise Exception("Expected second argument in out")
                    elif opcode_value == 0x04: # inp
                        imm16 = self.get_imm16()
                        register = ""
                        self.advance()
                        
                        if self.current_token.type == TokenTypes.COMMA:
                            self.advance()
                            
                            if self.current_token.type == TokenTypes.ID and self.current_token.value in REGISTERS.keys():
                                register = self.current_token.value
                                
                                self.final_text.append(self.make_instruction(opcode=opcode_value, imm16=imm16, register1=register))
                                self.advance()
                            else:
                                raise Exception("Expected register as second argument for out")
                        else:
                            raise Exception("Expected second argument in out")
                    elif opcode_value == 0x05 or opcode_value == 0x11 or opcode_value == 0x18 or opcode_value == 0x1a or opcode_value == 0x1c:                         # jnz
                        if self.current_token.type == TokenTypes.ID and self.current_token.value in REGISTERS:
                            # is a register. Add 0x01
                            opcode_value += 0x01
                            
                            self.final_text.append(self.make_instruction(opcode_value, register2=self.current_token.value))
                            self.advance()
                        else:
                            imm16 = self.get_imm16()
                            
                            self.final_text.append(self.make_instruction(opcode_value, imm16=imm16))
                            self.advance()
                    elif opcode_value == 0x07 or opcode_value == 0x09 or opcode_value == 0x0b or opcode_value == 0x0d or opcode_value == 0x0f:                         # add
                        register1 = "r0"
                        register2 = "r0"
                        imm16     = 0
                        
                        if self.current_token.value in REGISTERS:
                            register1 = self.current_token.value
                            self.advance()
                            
                            if self.current_token.type == TokenTypes.COMMA:
                                self.advance()
                            else:
                                raise Exception("Comma expected but not found")
                        else:
                            raise Exception("Register expected but not found")
                        
                        if self.current_token.value in REGISTERS:
                            register2 = self.current_token.value
                            self.advance()
                        else:
                            imm16 = self.get_imm16()
                            opcode_value += 0x01
                            self.advance()
                        
                        self.final_text.append(self.make_instruction(opcode_value, register1, register2, imm16))
                    elif opcode_value == 0x17: # swp
                        if self.current_token.type == TokenTypes.ID and self.current_token.value in REGISTERS:
                            self.final_text.append(self.make_instruction(opcode_value, self.current_token.value))
                            self.advance()
                        else:
                            raise Exception("Register expected but not found")
                    elif opcode_value == 0x13 or opcode_value == 0x15:
                        register1 = "r0"
                        register2 = "r0"
                        imm16     = 0
                        
                        if self.current_token.type == TokenTypes.ID and self.current_token.value in REGISTERS:
                            register1 = self.current_token.value
                            self.advance()
                            
                            if self.current_token.type == TokenTypes.COMMA:
                                self.advance()
                            else:
                                raise Exception("Comma expected but not found")
                        else:
                            raise Exception("Register expected but not found")
                        
                        if self.current_token.type == TokenTypes.ID and self.current_token.value in REGISTERS:
                            register2 = self.current_token.value
                            opcode_value += 1
                            self.advance()
                        else:
                            imm16 = self.get_imm16()
                            self.advance()
                        self.final_text.append(self.make_instruction(opcode_value, register1, register2, imm16))
                    
                    self.instruction_counter += 0x04
                elif self.current_token.value == "db": # db
                    self.advance()
                    imm8 = self.get_imm8()
                    self.final_text.append("0x{:02x}".format(imm8))
                    self.advance()
                    self.instruction_counter += 0x1
                    
                    
                    while self.current_token.type == TokenTypes.COMMA and self.current_token != None:
                        self.advance()
                        imm8 = self.get_imm8()
                        self.final_text.append("0x{:02x}".format(imm8))
                        self.instruction_counter += 0x1
                        self.advance()
                elif self.current_token.value == "dw": #dw
                    self.advance()
                    imm16 = self.get_imm16()
                    self.final_text.append("0x{:04x}".format(imm16))
                    self.advance()
                    self.instruction_counter += 0x02
                    
                    while self.current_token.type == TokenTypes.COMMA and self.current_token != None:
                        self.advance()
                        imm16 = self.get_imm16()
                        self.final_text.append("0x{:04x}".format(imm16))
                        self.instruction_counter += 0x02
                        self.advance()
                elif self.current_token.value in REGISTERS.keys():
                    raise Exception("Unexpected register reference")
                else:
                    # check for label
                    # 0x00 + (2 * instruction_index) = memory offset to jump to labels
                    potential_label_name = self.current_token.value
                    self.advance()
                    
                    if self.current_token.type == TokenTypes.COLLON:
                        LABELS.update({potential_label_name: hex(self.ep + int(self.instruction_counter))})
                        self.advance()
                    else:
                        raise Exception("Expected \":\" but got \"" + self.current_token.value + "\"")
            else:
                raise Exception("An assembly statement can never start with a number")
        
        new_final_text = []
        for i in self.final_text:
            new_final_text.append(int(i, base=16))
        
        return new_final_text
