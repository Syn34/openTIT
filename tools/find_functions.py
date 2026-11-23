#!/usr/bin/env python3
"""Discover function entry points in the Sly 4 PS3 EBOOT.elf (decrypted).

Sources of entry points:
  * the .opd function-descriptor table (32-bit addr + TOC pairs)
  * every direct `bl` target inside the code section
Function extents are approximated as [entry, next_entry).
Output: TSV  address<TAB>size<TAB>source
This is an analysis aid only; nothing here ships in the game build.
"""
import struct, sys
from capstone import Cs, CS_ARCH_PPC, CS_MODE_64, CS_MODE_BIG_ENDIAN

TEXT_VA, TEXT_OFF, TEXT_SZ = 0x10230, 0x230, 0xaad330
OPD_OFF, OPD_N = 0xf1d6c0, 0x1b140 // 8

def main(path, out):
    d = open(path, 'rb').read()
    entries = {}
    for i in range(OPD_N):
        a, toc = struct.unpack_from('>II', d, OPD_OFF + i * 8)
        if TEXT_VA <= a < TEXT_VA + TEXT_SZ:
            entries[a] = 'opd'
    text = d[TEXT_OFF:TEXT_OFF + TEXT_SZ]
    for i in range(0, len(text), 4):
        w, = struct.unpack_from('>I', text, i)
        if (w >> 26) == 18 and (w & 3) == 1:          # bl (relative, link)
            off = w & 0x03fffffc
            if off & 0x02000000: off -= 0x04000000
            t = TEXT_VA + i + off
            if TEXT_VA <= t < TEXT_VA + TEXT_SZ and t not in entries:
                entries[t] = 'bl'
    addrs = sorted(entries)
    with open(out, 'w') as f:
        for k, a in enumerate(addrs):
            end = addrs[k + 1] if k + 1 < len(addrs) else TEXT_VA + TEXT_SZ
            f.write('%08x\t%d\t%s\n' % (a, end - a, entries[a]))
    print('functions:', len(addrs), 'opd:', sum(1 for v in entries.values() if v == 'opd'))

if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
