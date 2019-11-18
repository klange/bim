#!/usr/bin/env python3
"""
Produce a single-file bim-baked.c
"""
import datetime
import glob
import subprocess

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
    while lines_out[0] == '\n':
        lines_out = lines_out[1:]
    return lines_out

headers = {
    'bim-core.h':   prune(read_file('bim-core.h')),
    'bim-syntax.h': prune(read_file('bim-syntax.h')),
}

syntax = [prune(read_file(x)) for x in sorted(glob.glob('syntax/*.c'))]

gitsha = subprocess.check_output(['git','rev-parse','HEAD']).decode('utf-8').strip()
gitsha_short = subprocess.check_output(['git','rev-parse','--short','HEAD']).decode('utf-8').strip()

bim_out = [
    '/**',
    ' * This is a baked, single-file version of bim.',
    f' * It was built {datetime.datetime.now().strftime("%c")}',
    f' * It is based on git commit {gitsha}',
    ' */',
    f'#define GIT_TAG "{gitsha_short}-baked"',
]

for line in bim:
    if not line.startswith('#include "'):
        bim_out.append(line)
    else:
        # Replace contents with headers
        for header in headers.keys():
            if header in line:
                bim_out.append('')
                bim_out.append('/* Included from {} */'.format(header))
                for l in headers[header]:
                    bim_out.append(l)
                bim_out.append('/* End of {} */'.format(header))

for syn in syntax:
    bim_out.extend(syn)

for line in bim_out:
    print(line.rstrip('\n'))
