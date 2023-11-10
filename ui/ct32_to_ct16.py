# get hex input

import sys

val = int(sys.argv[1], 16)
r = val & 0xff
r = r >> 3
g = (val >> 8) & 0xff
g = g >> 3
b = (val >> 16) & 0xff
b = b >> 3
ct16 = (b << 10) | (g << 5) | (r) | (((val >> 24) & 1) << 15)
print("0x%04X" % (((val >> 3) & 0x1F) | ((val >> 6) & 0x3E0) | ((val >> 9) & 0x7C00) | ((val >> 16) & 0x8000)))
