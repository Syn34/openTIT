#!/usr/bin/env python3
"""Recover C++ class names and virtual tables from Itanium-ABI RTTI in EBOOT.elf.

PS3 PPU code uses 32-bit pointers; function pointers point at .opd descriptors.
vtable layout:  [offset_to_top][typeinfo*][fn0 opd*][fn1 opd*]...
typeinfo:       [vptr][name*][base typeinfo*...]
Output TSV: class<TAB>vtable_addr<TAB>slot<TAB>func_addr
Analysis aid only.
"""
import struct, sys, re, subprocess

def load(path):
    d = open(path, 'rb').read()
    segs = []  # (va, off, size)
    phoff, = struct.unpack_from('>Q', d, 0x20)
    phnum, = struct.unpack_from('>H', d, 0x38)
    for i in range(phnum):
        p = phoff + i * 56
        typ, flags, off, va, pa, fsz, msz = struct.unpack_from('>IIQQQQQ', d, p)
        if typ == 1 and fsz:
            segs.append((va, off, fsz))
    return d, segs

def demangle(n):
    try:
        out = subprocess.run(['c++filt', '-t', n], capture_output=True, text=True).stdout.strip()
        return out or n
    except Exception:
        return n

def main(path, out):
    d, segs = load(path)
    def rd(va):
        for sva, off, sz in segs:
            if sva <= va < sva + sz - 3:
                return struct.unpack_from('>I', d, off + va - sva)[0]
        return None
    def cstr(va):
        for sva, off, sz in segs:
            if sva <= va < sva + sz:
                o = off + va - sva; e = d.find(b'\0', o, o + 256)
                return d[o:e] if e > 0 else None
        return None
    OPD_LO, OPD_HI = 0xf2d6c0, 0xf2d6c0 + 0x1b140
    # index all aligned words in data segments: value -> [addresses]
    words = {}
    for sva, off, sz in segs:
        for i in range(0, sz - 3, 4):
            v, = struct.unpack_from('>I', d, off + i)
            words.setdefault(v, []).append(sva + i)
    name_re = re.compile(rb'^(N?[0-9]+[A-Za-z_][A-Za-z0-9_]*E?|N(?:[0-9]+[A-Za-z_][A-Za-z0-9_]*)+E)$')
    rows = []
    seen_ti = set()
    for v, locs in words.items():
        s = cstr(v)
        if not s or not name_re.match(s):
            continue
        cls = demangle(s.decode())
        for loc in locs:
            ti = loc - 4            # typeinfo object: [vptr][name*]
            if ti in seen_ti: continue
            seen_ti.add(ti)
            for vt_ti_ref in words.get(ti, []):
                vt = vt_ti_ref - 4  # vtable starts at offset_to_top
                if rd(vt) not in (0,) and rd(vt) is not None and rd(vt) < 0x80000000:
                    pass
                slot = 0; p = vt_ti_ref + 4
                while True:
                    fp = rd(p)
                    if fp is None or not (OPD_LO <= fp < OPD_HI or fp == 0): break
                    fa = rd(fp) if fp else 0
                    rows.append((cls, vt, slot, fa, rd(vt) or 0))
                    slot += 1; p += 4
    with open(out, 'w') as f:
        for cls, vt, slot, fa, ott in rows:
            f.write('%s\t%08x\t%d\t%08x\t%d\n' % (cls, vt, slot, fa, ott))
    print('classes:', len(set(r[0] for r in rows)), 'vtable slots:', len(rows))

if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
