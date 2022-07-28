# convert a.bin to a.h

import sys
import struct

with open(sys.argv[1], 'rb') as f:
	data = f.read()
	print('const unsigned char eeprom_content[] = {')
	for i in range(0, len(data), 16):
		print('    ', end='')
		for j in range(i, i+16):
			if j < len(data):
				print('0x{:02x}, '.format(data[j]), end='')
		print()
	print('};')
	print('const unsigned int eeprom_content_len = {};'.format(len(data)))
	print()