#!/bin/env python

# convert any file to a c header

import sys
import os

argc = len(sys.argv)

if argc == 1:
    print("no input file")
    exit(1)
if argc == 2:
    print("no output file")
    exit(1)

bin = ""
with open(sys.argv[1], "rb") as file:
    bin = file.read()

alt_name = ""
for char in os.path.basename(sys.argv[2]).lower():
    ch = ord(char[0])
    if (ch >= 97 and ch <= 122) or (len(alt_name) > 0 and ch >= 48 and ch <= 57):
        alt_name += chr(ch)
    elif chr(ch) in ['.', ' ', '-']:
        alt_name += '_'

header_content = f"""// generated from {sys.argv[1]}

#pragma once

char {alt_name}_data[] = """

cnt = 0
header_content += "{\n"
for byte in bin:
    header_content += str(byte) + ','
    cnt += 1
    if cnt % 10 == 0:
        header_content += '\n'
header_content = header_content[:-1]
if cnt % 10 == 0:
    header_content = header_content[:-1]
header_content += "\n};\n\n"

header_content += f"int {alt_name}_size = {cnt};"

with open(sys.argv[2], "w") as file:
    file.write(header_content)
