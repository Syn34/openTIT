#!/usr/bin/env python3
"""Name the PRX import stubs of EBOOT.elf.

The NID of an exported symbol is the first 4 bytes (little endian) of
SHA1(name + fixed 16-byte suffix). Candidate names are harvested from any
text files passed in (e.g. emulator module sources) and matched by NID.
Output TSV: stub_addr<TAB>library<TAB>nid<TAB>name
"""
import hashlib, re, struct, sys, glob
SUFFIX = bytes.fromhex('6759659904250490566427499489741a')
def nid(name): return struct.unpack('<I', hashlib.sha1(name.encode() + SUFFIX).digest()[:4])[0]
imports, out = sys.argv[1], sys.argv[-1]
cands = set()
for g in sys.argv[2:-1]:
    for fn in glob.glob(g):
        cands |= set(re.findall(r'\b([A-Za-z_][A-Za-z0-9_]{2,})\b', open(fn, errors='ignore').read()))
table = {nid(c): c for c in cands}
rows = [l.rstrip('\n').split('\t') for l in open(imports)]
hit = 0
with open(out, 'w') as f:
    for lib, n, stub in rows:
        name = table.get(int(n, 16), '')
        hit += bool(name)
        f.write('%s\t%s\t%s\t%s\n' % (stub, lib, n, name or 'nid_' + n))
print('resolved %d/%d' % (hit, len(rows)))
