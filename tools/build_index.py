#!/usr/bin/env python3
"""Build a JSON index of functions: size, callees, referenced strings.
Inputs: eboot.elf, funcs.tsv, Ghidra reference dumps (dc_*.c, analysis aid only).
"""
import re, struct, sys, json, glob
elf, funcs_tsv, dc_glob, out = sys.argv[1:5]
d = open(elf, 'rb').read()
segs = [(0x10000, 0, 0xef5168), (0xf10000, 0xf00000, 0x8472c)]
def cstr(va):
    for sva, off, sz in segs:
        if sva <= va < sva + sz:
            o = off + va - sva; e = d.find(b'\0', o, o + 400)
            if e <= o: return None
            s = d[o:e]
            if len(s) >= 3 and all(32 <= c < 127 or c in (9, 10) for c in s):
                if va == sva or d[o - 1] == 0 or True:
                    return s.decode()
    return None
def rd32(va):
    for sva, off, sz in segs:
        if sva <= va < sva + sz - 3: return struct.unpack_from('>I', d, off + va - sva)[0]
funcs = {}
for line in open(funcs_tsv):
    a, s, src = line.split('\t'); funcs[int(a, 16)] = {'size': int(s), 'src': src.strip()}
hexre = re.compile(r'(?:0x|UNK_|DAT_)([0-9a-f]{5,8})\b')
ramre = re.compile(r'[a-zA-Z]*Ram([0-9a-f]{8})')
callre = re.compile(r'FUN_([0-9a-f]{8})|func_0x([0-9a-f]{8})')
for fn in sorted(glob.glob(dc_glob)):
    txt = open(fn).read()
    for part in txt.split('//==== FUNC ')[1:]:
        a = int(part[:8], 16); body = part[9:]
        f = funcs.setdefault(a, {'size': 0, 'src': 'ghidra'})
        f['callees'] = sorted({int(x or y, 16) for x, y in callre.findall(body)} - {a})
        strs = []
        for h in hexre.findall(body):
            v = int(h, 16)
            if 0xabf500 <= v < 0xf05100:
                s = cstr(v)
                if s: strs.append(s)
        for h in ramre.findall(body):   # global holding a pointer to a string
            p = rd32(int(h, 16))
            if p and 0xabf500 <= p < 0xf05100:
                s = cstr(p)
                if s: strs.append(s)
        f['strings'] = list(dict.fromkeys(strs))[:20]
        f['lines'] = body.count('\n')
callers = {}
for a, f in funcs.items():
    for c in f.get('callees', []): callers.setdefault(c, []).append(a)
for a, f in funcs.items(): f['ncallers'] = len(callers.get(a, []))
json.dump({'%08x' % a: f for a, f in sorted(funcs.items())}, open(out, 'w'))
print(len(funcs), 'with strings:', sum(1 for f in funcs.values() if f.get('strings')))
