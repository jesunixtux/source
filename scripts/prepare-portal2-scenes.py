#!/usr/bin/env python3
"""Convert VSIF 3 summaries to VSIF 2 in the isolated overlay; VCD bytes stay intact."""
import argparse
from pathlib import Path
import struct
import lzma

def convert(source):
    data = bytearray(source)
    if len(data)<20: raise ValueError('Truncated scene image')
    magic,version,count,strings,offset=struct.unpack_from('<5I',data)
    if magic!=0x46495356 or version!=3: raise ValueError('Expected VSIF version 3')
    if offset+count*16>len(data) or 20+strings*4>len(data): raise ValueError('Invalid scene tables')
    for i in range(count):
        crc,start,size,summary=struct.unpack_from('<4I',source,offset+i*16)
        if start+size>len(data) or summary+12>len(data): raise ValueError('Invalid scene offsets')
        ms,speech,sounds=struct.unpack_from('<3I',source,summary)
        if summary+12+sounds*4>len(data): raise ValueError('Invalid sound table')
        raw=bytes(source[start:start+size])
        if raw[:4]==b'LZMA':
            length,packed=struct.unpack_from('<II',raw,4)
            if packed+17>len(raw): raise ValueError('Truncated LZMA')
            prop=raw[12]; lc=prop%9; prop//=9; lp=prop%5; pb=prop//5
            dictionary=struct.unpack_from('<I',raw,13)[0]
            raw=lzma.LZMADecompressor(format=lzma.FORMAT_RAW,filters=[dict(
                id=lzma.FILTER_LZMA1,dict_size=dictionary,lc=lc,lp=lp,pb=pb)]).decompress(raw[17:17+packed],max_length=length)
            if len(raw)!=length: raise ValueError(f'Incorrect decompressed length in record {i}: {len(raw)} != {length}')
        if raw[:5]!=b'bvcd\x04': raise ValueError('Unsupported binary VCD version')
        # Shrink only the summary payload, preserving all absolute offsets.
        ids=bytes(source[summary+12:summary+12+sounds*4])
        struct.pack_into('<2I',data,summary,ms,sounds)
        data[summary+8:summary+8+len(ids)]=ids
    struct.pack_into('<I',data,4,2)
    return data,count

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('image',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
    data,count=convert(a.image.read_bytes())
    a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_bytes(data)
    print(f'Compatible scene image: {count} verified binary VCD v4 records -> {a.output}')
