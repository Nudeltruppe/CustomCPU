import struct
import sys
from tokenize import Token
import lexer
from lexer import Lexer
from gen import Generator

import argparse

if __name__=="__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--offset", type=str, default="0x0")
    parser.add_argument("--output", type=str, default="a.bin")
    parser.add_argument("--input", type=str, default="tests/test.16bs")

    args = parser.parse_args()

    ep = int(args.offset, base=16)
    with open(args.input, "r") as f:
        tokens = Lexer(f.read()).Lex()
        text = Generator(tokens, ep).Gen()
        
        with open(args.output, "wb") as f:
            for i in text:
                f.write(struct.pack(">I", i))
