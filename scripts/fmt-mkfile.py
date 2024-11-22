"""
Convert non-leading tabs to spaces in a makefile.
"""

import os
import sys
from math import floor

def format_line(line: str, tab_width: int = 4) -> str:
    line = line.rstrip()
    if len(line) == 0:
        return ''
    index = 0
    while line[index] == '\t':
        index += 1
    new_line = line[:index]
    width = tab_width * index
    while index < len(line):
        if line[index] == '\t':
            new_width = floor((width + tab_width) / tab_width) * tab_width
            new_line += ' ' * (new_width - width)
            width = new_width
        else:
            new_line += line[index]
            width += 1
        index += 1
    return new_line

if __name__ == '__main__':

    if len(sys.argv) == 1:
        script_path = os.path.relpath(__file__, os.getcwd())
        print(f'Usage: python {script_path} <path-to-mkfile>')
        exit()

    for file in sys.argv[1:]:
        if not os.path.isfile(file):
            continue
        formatted = None
        with open(file, 'r') as f:
            formatted = '\n'.join(map(format_line, f.readlines()))
        with open(file, 'w') as f:
            f.write(formatted + '\n')
