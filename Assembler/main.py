import struct
import sys
from tokenize import Token
import lexer
from lexer import Lexer
from gen import Generator

if __name__=="__main__":
    with open(sys.argv[1], "r") as f:
        tokens = Lexer(f.read()).Lex()
        text = Generator(tokens).Gen()
        
        with open("a.bin", "wb") as f:
            for i in text:
                f.write(struct.pack(">I", i))
