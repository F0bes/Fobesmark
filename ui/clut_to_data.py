clut_data = [
		0x383838,
		0x0044ff,
		0x0071ff,
		0x00a3ff,
		0x4aa3ff,
		0x4ba4ff,
		0x65b3ff,
		0xffffff,
		0xffffff,
		0xffffff,
		0xffffff,
		0xffffff,
		0xffffff,
		0xffffff,
		0xffffff,
		0xffffff
]

clut_bytes = bytearray()
for value in clut_data:
    # Extract individual bytes (3 bytes per value) and append to the clut_bytes
    clut_bytes.append((value >> 16) & 0xFF)  # Extract the first byte
    clut_bytes.append((value >> 8) & 0xFF)   # Extract the second byte
    clut_bytes.append(value & 0xFF)          # Extract the third byte

with open("clut.bin", "wb") as file:
    file.write(clut_bytes)

