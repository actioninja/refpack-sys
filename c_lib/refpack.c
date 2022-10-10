#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*------------------------------------------------------------------*/
/*                                                                  */
/*               RefPack - Backward Reference Codex                 */
/*                                                                  */
/*                    by FrANK G. Barchard, EAC                     */
/*                                                                  */
/*------------------------------------------------------------------*/
/* Format Notes:                                                    */
/* -------------                                                    */
/* refpack is a sliding window (131k) lzss method, with byte        */
/* oriented coding.                                                 */
/*                                                                  */
/* huff fb5 style header:                                           */
/*      *10fb  fb5      refpack 1.0  reference pack                 */
/*                                                                  */
/*                                                                  */
/* header:                                                          */
/* [10fb] [unpacksize] [totalunpacksize]                            */
/*   2         3                                                    */
/*                                                                  */
/*                                                                  */
/*                                                                  */
/* format is:                                                       */
/* ----------                                                       */
/* 0ffnnndd_ffffffff          short ref, f=0..1023,n=3..10,d=0..3   */
/* 10nnnnnn_ddffffff_ffffffff long ref, f=0..16384,n=4..67,d=0..3   */
/* 110fnndd_f.._f.._nnnnnnnn  very long,f=0..131071,n=5..1028,d=0..3*/
/* 111ddddd                   literal, d=4..112                     */
/* 111111dd                   eof, d=0..3                           */
/*                                                                  */
/*------------------------------------------------------------------*/

static uint32_t crctab[256] = {
        0x0000, 0xc0c1, 0xc181, 0x0140, 0xc301, 0x03c0, 0x0280, 0xc241,
        0xc601, 0x06c0, 0x0780, 0xc741, 0x0500, 0xc5c1, 0xc481, 0x0440,
        0xcc01, 0x0cc0, 0x0d80, 0xcd41, 0x0f00, 0xcfc1, 0xce81, 0x0e40,
        0x0a00, 0xcac1, 0xcb81, 0x0b40, 0xc901, 0x09c0, 0x0880, 0xc841,
        0xd801, 0x18c0, 0x1980, 0xd941, 0x1b00, 0xdbc1, 0xda81, 0x1a40,
        0x1e00, 0xdec1, 0xdf81, 0x1f40, 0xdd01, 0x1dc0, 0x1c80, 0xdc41,
        0x1400, 0xd4c1, 0xd581, 0x1540, 0xd701, 0x17c0, 0x1680, 0xd641,
        0xd201, 0x12c0, 0x1380, 0xd341, 0x1100, 0xd1c1, 0xd081, 0x1040,
        0xf001, 0x30c0, 0x3180, 0xf141, 0x3300, 0xf3c1, 0xf281, 0x3240,
        0x3600, 0xf6c1, 0xf781, 0x3740, 0xf501, 0x35c0, 0x3480, 0xf441,
        0x3c00, 0xfcc1, 0xfd81, 0x3d40, 0xff01, 0x3fc0, 0x3e80, 0xfe41,
        0xfa01, 0x3ac0, 0x3b80, 0xfb41, 0x3900, 0xf9c1, 0xf881, 0x3840,
        0x2800, 0xe8c1, 0xe981, 0x2940, 0xeb01, 0x2bc0, 0x2a80, 0xea41,
        0xee01, 0x2ec0, 0x2f80, 0xef41, 0x2d00, 0xedc1, 0xec81, 0x2c40,
        0xe401, 0x24c0, 0x2580, 0xe541, 0x2700, 0xe7c1, 0xe681, 0x2640,
        0x2200, 0xe2c1, 0xe381, 0x2340, 0xe101, 0x21c0, 0x2080, 0xe041,
        0xa001, 0x60c0, 0x6180, 0xa141, 0x6300, 0xa3c1, 0xa281, 0x6240,
        0x6600, 0xa6c1, 0xa781, 0x6740, 0xa501, 0x65c0, 0x6480, 0xa441,
        0x6c00, 0xacc1, 0xad81, 0x6d40, 0xaf01, 0x6fc0, 0x6e80, 0xae41,
        0xaa01, 0x6ac0, 0x6b80, 0xab41, 0x6900, 0xa9c1, 0xa881, 0x6840,
        0x7800, 0xb8c1, 0xb981, 0x7940, 0xbb01, 0x7bc0, 0x7a80, 0xba41,
        0xbe01, 0x7ec0, 0x7f80, 0xbf41, 0x7d00, 0xbdc1, 0xbc81, 0x7c40,
        0xb401, 0x74c0, 0x7580, 0xb541, 0x7700, 0xb7c1, 0xb681, 0x7640,
        0x7200, 0xb2c1, 0xb381, 0x7340, 0xb101, 0x71c0, 0x7080, 0xb041,
        0x5000, 0x90c1, 0x9181, 0x5140, 0x9301, 0x53c0, 0x5280, 0x9241,
        0x9601, 0x56c0, 0x5780, 0x9741, 0x5500, 0x95c1, 0x9481, 0x5440,
        0x9c01, 0x5cc0, 0x5d80, 0x9d41, 0x5f00, 0x9fc1, 0x9e81, 0x5e40,
        0x5a00, 0x9ac1, 0x9b81, 0x5b40, 0x9901, 0x59c0, 0x5880, 0x9841,
        0x8801, 0x48c0, 0x4980, 0x8941, 0x4b00, 0x8bc1, 0x8a81, 0x4a40,
        0x4e00, 0x8ec1, 0x8f81, 0x4f40, 0x8d01, 0x4dc0, 0x4c80, 0x8c41,
        0x4400, 0x84c1, 0x8581, 0x4540, 0x8701, 0x47c0, 0x4680, 0x8641,
        0x8201, 0x42c0, 0x4380, 0x8341, 0x4100, 0x81c1, 0x8081, 0x4040,
};

struct CompressorInput {
    void *buffer;
    uint32_t lengthInBytes;
};

struct DecompressorInput {
    void *buffer;
    uint32_t lengthInBytes;
};

inline uint32_t HASH(const uint8_t *s) {
    uint32_t crc;

    crc = crctab[(uint8_t) (s[0])];
    crc = crctab[(uint8_t) (crc ^ s[1])] ^ (crc >> 8);
    crc = crctab[(uint8_t) (crc ^ s[2])] ^ (crc >> 8);

    return crc;
}

uint32_t matchlen(const uint8_t *s, const uint8_t *d, uint32_t maxmatch) {
    uint32_t current;

    for (current = 0; current < maxmatch && *s++ == *d++; ++current);

    return current;
}

void compress(struct CompressorInput *input, struct DecompressorInput *output) {
#if 0
    uint32_t quick=0;          // seems to prevent a long compression if set true.  Probably affects compression ratio too.
#endif

    int32_t len;
    uint32_t tlen;
    uint32_t tcost;
    uint32_t run;
    uint32_t toffset;
    uint32_t boffset;
    uint32_t blen;
    uint32_t bcost;
    uint32_t mlen;
    const uint8_t *tptr;
    const uint8_t *cptr;
    const uint8_t *rptr;
    uint8_t *to;

    int countliterals = 0;
    int countshort = 0;
    int countlong = 0;
    int countvlong = 0;
    long hash;
    long hoffset;
    long minhoffset;
    int i;
    int32_t *link;
    int32_t *hashtbl;
    int32_t *hashptr;

    len = input->lengthInBytes;

    output->buffer = malloc((input->lengthInBytes * 2 + 8192) * sizeof(uint8_t));   // same wild guess Frank Barchard makes
    to = (uint8_t *) (output->buffer);

    // write size into the stream
    for (i = 0; i < 4; i++, to++)
        *to = (uint8_t) (input->lengthInBytes >> (i * 8) & 255);

    // write magic in to the stream
    *to++ = (uint8_t) 0x10;
    *to++ = (uint8_t) 0xFB;

    //skip space for the compressed size
    *to++ = 0;
    *to++ = 0;
    *to++ = 0;

    run = 0;
    cptr = rptr = (uint8_t *) (input->buffer);

    hashtbl = malloc(65536 * sizeof(int32_t));
    link = malloc(131072 * sizeof(int32_t));

    hashptr = hashtbl;
    for (i = 0; i < 65536L / 16; ++i) {
        *(hashptr + 0) = *(hashptr + 1) = *(hashptr + 2) = *(hashptr + 3) =
        *(hashptr + 4) = *(hashptr + 5) = *(hashptr + 6) = *(hashptr + 7) =
        *(hashptr + 8) = *(hashptr + 9) = *(hashptr + 10) = *(hashptr + 11) =
        *(hashptr + 12) = hashptr[13] = hashptr[14] = hashptr[15] = -1L;
        hashptr += 16;
    }

    while (len > 0) {
        boffset = 0;
        blen = bcost = 2;
        mlen = min(len, 1028);
        tptr = cptr - 1;
        hash = HASH(cptr);
        hoffset = hashtbl[hash];
        minhoffset = max(cptr - (uint8_t *) (input->buffer) - 131071, 0);


        if (hoffset >= minhoffset) {
            do {
                tptr = (uint8_t *) (input->buffer) + hoffset;
                if (cptr[blen] == tptr[blen]) {
                    tlen = matchlen(cptr, tptr, mlen);
                    if (tlen > blen) {
                        toffset = (cptr - 1) - tptr;
                        if (toffset < 1024 && tlen <= 10)       /* two byte long form */
                            tcost = 2;
                        else if (toffset < 16384 && tlen <= 67) /* three byte long form */
                            tcost = 3;
                        else                                /* four byte very long form */
                            tcost = 4;

                        if (tlen - tcost + 4 > blen - bcost + 4) {
                            blen = tlen;
                            bcost = tcost;
                            boffset = toffset;
                            if (blen >= 1028) break;
                        }
                    }
                }
            } while ((hoffset = link[hoffset & 131071]) >= minhoffset);
        }
        if (bcost >= blen) {
            hoffset = (cptr - (uint8_t *) (input->buffer));
            link[hoffset & 131071] = hashtbl[hash];
            hashtbl[hash] = hoffset;

            ++run;
            ++cptr;
            --len;
        } else {
            while (run > 3)                   /* literal block of data */
            {
                tlen = min((uint32_t) 112, run & ~3);
                run -= tlen;
                *to++ = (unsigned char) (0xe0 + (tlen >> 2) - 1);
                memcpy(to, rptr, tlen);
                rptr += tlen;
                to += tlen;
                ++countliterals;
            }
            if (bcost == 2)                   /* two byte long form */
            {
                *to++ = (unsigned char) (((boffset >> 8) << 5) + ((blen - 3) << 2) + run);
                *to++ = (unsigned char) boffset;
                ++countshort;
            } else if (bcost == 3)              /* three byte long form */
            {
                *to++ = (unsigned char) (0x80 + (blen - 4));
                *to++ = (unsigned char) ((run << 6) + (boffset >> 8));
                *to++ = (unsigned char) boffset;
                ++countlong;
            } else                            /* four byte very long form */
            {
                *to++ = (unsigned char) (0xc0 + ((boffset >> 16) << 4) + (((blen - 5) >> 8) << 2) + run);
                *to++ = (unsigned char) (boffset >> 8);
                *to++ = (unsigned char) (boffset);
                *to++ = (unsigned char) (blen - 5);
                ++countvlong;
            }
            if (run) {
                memcpy(to, rptr, run);
                to += run;
                run = 0;
            }
#if 0
            if (quick)
            {
                hoffset = (cptr-static_cast<uint8_t *>(input.buffer));
                link[hoffset&131071] = hashtbl[hash];
                hashtbl[hash] = hoffset;
                cptr += blen;
            }
            else
#endif
            {
                for (i = 0; i < (int) blen; ++i) {
                    hash = HASH(cptr);
                    hoffset = (cptr - (uint8_t *) (input->buffer));
                    link[hoffset & 131071] = hashtbl[hash];
                    hashtbl[hash] = hoffset;
                    ++cptr;
                }
            }

            rptr = cptr;
            len -= blen;
        }
    }
    while (run > 3)                       /* no match at end, use literal */
    {
        tlen = min((uint32_t) 112, run & ~3);
        run -= tlen;
        *to++ = (unsigned char) (0xe0 + (tlen >> 2) - 1);
        memcpy(to, rptr, tlen);
        rptr += tlen;
        to += tlen;
    }

    *to++ = (unsigned char) (0xfc + run); /* end of stream command + 0..3 literal */
    if (run) {
        memcpy(to, rptr, run);
        to += run;
    }

    free(link);
    free(hashtbl);

    uint32_t length = (uint32_t) (to - (uint8_t *) (output->buffer));

    uint8_t *length_pointer = (input->buffer);

    length_pointer[6] = (unsigned char) (length >> 16) & 255;
    length_pointer[7] = (unsigned char) (length >> 8) & 255;
    length_pointer[8] = (unsigned char) (length & 255);

    output->lengthInBytes = length;
}

void decompress(const struct DecompressorInput *input, struct CompressorInput *output) {
    const uint8_t *index;
    uint8_t *ref;
    uint8_t *destIndex;
    uint8_t first;
    uint8_t second;
    uint8_t third;
    uint8_t forth;
    uint32_t run;

    index = (input->buffer);

    output->lengthInBytes = 0;
    // read size from the stream
    for (int i = 0; i < 4; i++, index++)
        output->lengthInBytes |= (*index) << (i * 8);

    //skip over magic and compressed size
    index += 5;

    output->buffer = malloc(output->lengthInBytes * sizeof(uint8_t));

    destIndex = (output->buffer);

    for (;;) {
        first = *index++;
        if (!(first & 0x80))          /* short form */
        {
            second = *index++;
            run = first & 3;
            while (run--)
                *destIndex++ = *index++;
            ref = destIndex - 1 - (((first & 0x60) << 3) + second);
            run = ((first & 0x1c) >> 2) + 3 - 1;
            do {
                *destIndex++ = *ref++;
            } while (run--);
            continue;
        }
        if (!(first & 0x40))          /* long form */
        {
            second = *index++;
            third = *index++;
            run = second >> 6;
            while (run--)
                *destIndex++ = *index++;

            ref = destIndex - 1 - (((second & 0x3f) << 8) + third);

            run = (first & 0x3f) + 4 - 1;
            do {
                *destIndex++ = *ref++;
            } while (run--);
            continue;
        }
        if (!(first & 0x20))          /* very long form */
        {
            second = *index++;
            third = *index++;
            forth = *index++;
            run = first & 3;
            while (run--)
                *destIndex++ = *index++;

            ref = destIndex - 1 - (((first & 0x10) >> 4 << 16) + (second << 8) + third);

            run = ((first & 0x0c) >> 2 << 8) + forth + 5 - 1;
            do {
                *destIndex++ = *ref++;
            } while (run--);
            continue;
        }
        run = ((first & 0x1f) << 2) + 4;  /* literal */
        if (run <= 112) {
            while (run--)
                *destIndex++ = *index++;
            continue;
        }
        run = first & 3;              /* eof (+0..3 literal) */
        while (run--)
            *destIndex++ = *index++;
        break;
    }

//	if (static_cast<uint32_t>(static_cast<const uint8_t *>(destIndex)-static_cast<uint8_t *>(output->buffer))!=output->lengthInBytes)
//	{
//		Fatal("What happened?");
//	}
}
