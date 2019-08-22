#!/usr/bin/env python3
"""
Produce a single-file bim-baked.c
"""

import glob

def read_file(file):
    with open(file) as f:
        return f.readlines()

bim = read_file('bim.c')

def prune(lines):
    lines_out = []
    for line in lines:
        if line.startswith('#ifndef _BIM_') or line.startswith('#endif /* _BIM_') or line.startswith('#define _BIM_') or line.startswith('#include "bim'):
            pass
        else:
            lines_out.append(line)
    return lines_out

headers = {
    'bim-core.h':   prune(read_file('bim-core.h')),
    'bim-theme.h':  prune(read_file('bim-theme.h')),
    'bim-syntax.h': prune(read_file('bim-syntax.h')),
}

themes = [prune(read_file(x)) for x in glob.glob('themes/*.c')]
syntax = [prune(read_file(x)) for x in glob.glob('syntax/*.c')]

bim_out = [
    '/* This is a baked, single-file version of bim. */',
]

for line in bim:
    if not line.startswith('#include "'):
        bim_out.append(line)
    else:
        # Replace contents with headers
        for header in headers.keys():
            if header in line:
                bim_out.append('/* Included from {} */'.format(header))
                for l in headers[header]:
                    bim_out.append(l)

# Add each pruned theme.
for theme in themes:
    bim_out.extend(theme)

for syn in syntax:
    bim_out.extend(syn)

for line in bim_out:
    print(line.rstrip('\n'))
