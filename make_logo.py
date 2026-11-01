import struct, zlib

def create_png():
    w, h = 128, 128
    raw_data = bytearray()
    for y in range(h):
        raw_data.append(0)
        for x in range(w):
            if x < 4 or x > w-5 or y < 4 or y > h-5:
                raw_data.extend([0, 220, 255, 255])
            else:
                raw_data.extend([int(30 + x*0.5), int(15 + y*0.4), int(80 + (x+y)*0.6), 255])
    
    def chunk(tag, data):
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xffffffff)

    ihdr = struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0)
    idat = zlib.compress(bytes(raw_data))
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) + chunk(b"IDAT", idat) + chunk(b"IEND", b"")
    with open(r"C:\Users\Maks\.gemini\antigravity\scratch\mega-geode-hack\logo.png", "wb") as f:
        f.write(png)

if __name__ == "__main__":
    create_png()
