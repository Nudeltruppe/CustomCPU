from dataclasses import dataclass
import enum


class TokenTypes(enum.Enum):
    ID = 0

    BINARY_IMM16      = 1
    DECIMAL_IMM16     = 2
    HEXADECIMAL_IMM16 = 3
    
    COLLON = 4
    COMMA  = 5

ALPHABET = "abcdefghijklmnopqrstuvwxyz"
NUMBERS = "0123456789"
ADDITIONAL_HEX_VALUES = "abcdef"


@dataclass
class Token:
    def __init__(self, value: str, type: int) -> None:
        self.value = value
        self.type = type
    
    def __repr__(self) -> str:
        return "Val: " + str(self.value) + "; Type: " + str(self.type)

class Lexer:
    def __init__(self, text) -> None:
        self.current_char: str = None
        self.text = text
        self.pos = -1
        self.advance()


    def advance(self):
        self.pos += 1
        self.current_char = self.text[self.pos] if self.pos < len(self.text) else None

    def __make_id(self) -> Token:
        ret_string = self.current_char
        self.advance()
        while self.current_char != None and self.current_char in str(ALPHABET) + str(NUMBERS):
            ret_string += self.current_char
            self.advance()
        
        return Token(ret_string, TokenTypes.ID)
    
    def __make_number(self) -> Token:
        ret_string = ""
        if self.current_char == "0":
            self.advance()
            if self.current_char == "x":
                self.advance()
                ret_string = "0x"
                while self.current_char != None and self.current_char in NUMBERS + ADDITIONAL_HEX_VALUES:
                    ret_string += self.current_char
                    self.advance()
                
                return Token(ret_string, TokenTypes.HEXADECIMAL_IMM16)
            elif self.current_char == "b":
                self.advance()
                ret_string = "0b"
                while self.current_char != None and self.current_char in "01":
                    ret_string += self.current_char
                    self.advance()
                
                return Token(ret_string, TokenTypes.BINARY_IMM16)
            else:
                ret_string = "0"

        while self.current_char != None and self.current_char in NUMBERS:
            ret_string += self.current_char
            self.advance()
            
        return Token(ret_string, TokenTypes.DECIMAL_IMM16)
   
    def Lex(self):
        self.text = self.text.lower()
        tokens = []

        while self.current_char != None:
            if self.current_char == " ":
                self.advance()
            elif self.current_char == "\t":
                self.advance()
            elif self.current_char == "\n":
                self.advance()
            elif self.current_char in ALPHABET:
                tokens.append(self.__make_id())
            elif self.current_char in NUMBERS:
                tokens.append(self.__make_number())
            elif self.current_char == ":":
                tokens.append(Token(":", TokenTypes.COLLON))
                self.advance()
            elif self.current_char == ",":
                tokens.append(Token(",", TokenTypes.COMMA))
                self.advance()
            elif self.current_char == ";":
                self.advance()
                
                while self.current_char != "\n" and self.current_char != None:
                    self.advance()
                
            else:
                # unexpected token
                raise Exception("Unexpected token: \"" + self.current_char + "\"")
            
        return tokens
