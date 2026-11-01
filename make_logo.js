const fs = require('fs');
const zlib = require('zlib');

function createPng() {
    const w = 128, h = 128;
    const raw = [];
    for (let y = 0; y < h; y++) {
        raw.push(0); // filter type none
        for (let x = 0; x < w; x++) {
            // Neon cyan border, dark purple background with MH accent
            const isBorder = (x < 3 || x >= w - 3 || y < 3 || y >= h - 3);
            const isM = (x >= 35 && x <= 45 && y >= 35 && y <= 95) ||
                        (x >= 85 && x <= 95 && y >= 35 && y <= 95) ||
                        (x >= 45 && x <= 65 && Math.abs((x - 45) - (y - 35)) <= 4 && y <= 65) ||
                        (x >= 65 && x <= 85 && Math.abs((85 - x) - (y - 35)) <= 4 && y <= 65);
            if (isBorder) {
                raw.push(0, 230, 255, 255);
            } else if (isM) {
                raw.push(255, 215, 0, 255); // Gold 'M'
            } else {
                raw.push(25, 20, 45, 255); // Dark purple
            }
        }
    }

    const crcTable = [];
    for (let n = 0; n < 256; n++) {
        let c = n;
        for (let k = 0; k < 8; k++) {
            c = ((c & 1) ? (0xedb88320 ^ (c >>> 1)) : (c >>> 1));
        }
        crcTable[n] = c;
    }
    function crc32(buf) {
        let crc = 0xffffffff;
        for (let i = 0; i < buf.length; i++) {
            crc = (crc >>> 8) ^ crcTable[(crc ^ buf[i]) & 0xff];
        }
        return (crc ^ 0xffffffff) >>> 0;
    }

    function chunk(type, data) {
        const len = Buffer.alloc(4);
        len.writeUInt32BE(data.length, 0);
        const t = Buffer.from(type, 'ascii');
        const toCrc = Buffer.concat([t, data]);
        const crc = Buffer.alloc(4);
        crc.writeUInt32BE(crc32(toCrc), 0);
        return Buffer.concat([len, t, data, crc]);
    }

    const sig = Buffer.from([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a]);
    const ihdrData = Buffer.alloc(13);
    ihdrData.writeUInt32BE(w, 0);
    ihdrData.writeUInt32BE(h, 4);
    ihdrData[8] = 8;  // bit depth
    ihdrData[9] = 6;  // RGBA
    ihdrData[10] = 0; // compression
    ihdrData[11] = 0; // filter
    ihdrData[12] = 0; // interlace
    const ihdr = chunk('IHDR', ihdrData);

    const compressed = zlib.deflateSync(Buffer.from(raw));
    const idat = chunk('IDAT', compressed);
    const iend = chunk('IEND', Buffer.alloc(0));

    const finalBuf = Buffer.concat([sig, ihdr, idat, iend]);
    fs.writeFileSync('C:/Users/Maks/.gemini/antigravity/scratch/mega-geode-hack/logo.png', finalBuf);
}

createPng();
