# -*- coding: utf-8 -*-

import binascii, sys, struct

if len(sys.argv) < 3:
	print("Usage: %s [firmware] [new_fw]" % sys.argv[0])
	sys.exit(1)

tag = sys.argv[3]
data = []
with open(sys.argv[1], 'rb') as infile:
	data = infile.readlines()
with open(sys.argv[2], 'wb') as outfile:
	# write data length
	outfile.write(b'0000')
	# make space for crc, 4 bytes
	outfile.write(b'0000')
	crc = 0
	size = 0
	crc = binascii.crc32(tag, crc) & 0xffffffff
	for line in data:
		crc = binascii.crc32(line, crc) & 0xffffffff
		size += len(line)
		outfile.write(line)
	# write crc32
	outfile.seek(0)
	print "size", size, "crc %08x" % crc
	outfile.write(struct.pack('<I', size))
	outfile.write(struct.pack('<I', crc))

print("Firmware file %s created." % sys.argv[2])
