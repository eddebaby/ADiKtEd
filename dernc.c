/******************************************************************************/
// dernc.c - RNC compression support.
/******************************************************************************/
// Author:   Jon Skeet
// Created:  14 Oct 1997
// Modified: Tomasz Lis

// Purpose:
//   Compiled normally, this file is a well-behaved, re-entrant code
//   module exporting only `rnc_ulen', `rnc_unpack' and `rnc_error'.
//   Compiled with MAIN_DERNC defined, it's a standalone program which will
//   decompress argv[1] into argv[2].

// Comment:
//   in/out buffers should have 8 redundant "safe bytes" at end.

//Copying and copyrights:
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
/******************************************************************************/

#ifdef MAIN_DERNC
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
#endif

#define INTERNAL
#include "dernc.h"

#ifdef MAIN_DERNC
int main_unpack (char *pname, char *iname, char *oname);
int copy_file (char *iname, char *oname);

int show_usage(char *fname)
{
	printf("usage:\n");
	printf("    %s <files>\n", fname);
    printf(" or\n");
    printf("    %s -o <infile> <outfile>\n", fname);
}

int main(int argc, char **argv) {
    int mode=0;
    int i;

    printf("\nBullfrog RNC files decompressor");
    printf("\n-------------------------------\n");

    if (argc==1)
    {
        show_usage(*argv);
	    return 0;
    }
    for (i=1; i < argc; i++)
	if (!strcmp (argv[i], "-o"))
	    mode=i;
    if (mode && argc != 4)
    {
        show_usage(*argv);
	    return 1;
    }
    switch (mode)
    {
      case 0 :
	for (i=1; i < argc; i++)
	    {
      	printf("Extracting %s...\n",argv[i]);
        if (main_unpack (*argv, argv[i], argv[i]))
            return 1;
        }
	return 0;
      case 1 :
   	printf("Extracting %s to %s...\n",argv[2],argv[3]);
	return main_unpack (*argv, argv[2], argv[3]);
      case 2 :
   	printf("Extracting %s to %s...\n",argv[1],argv[3]);
	return main_unpack (*argv, argv[1], argv[3]);
      case 3 :
   	printf("Extracting %s to %s...\n",argv[1],argv[2]);
	return main_unpack (*argv, argv[1], argv[2]);
      default :
	fprintf (stderr, "Internal fault.\n");
    }
    return 1;
}

int main_unpack (char *pname, char *iname, char *oname)
{
    FILE *ifp, *ofp;
    long plen, ulen;
    void *packed, *unpacked;
    char buffer[4];
    
    ifp = fopen(iname, "rb");
    if (!ifp)
    {
        perror(iname);
        return 1;
    }
    //Checking if the file is RNC
    fseek (ifp, 0L, SEEK_END);
    plen = ftell (ifp);
    rewind (ifp);
    if (plen < 4) // Can't be an RNC file
    {
        if (strcmp (iname, oname))
	        return copy_file (iname, oname);
        return 0;
    }
    fread (buffer, 1, 4, ifp);
    if (strncmp (buffer, "RNC", 3))
    {
	fclose (ifp);
	if (strcmp (iname, oname))
	    return copy_file (iname, oname);
	return 0;
    }
    rewind (ifp);
    //Reading compressed data, 8 bytes in buffer are for safety
    packed = malloc(plen+8);
    if (!packed)
    {
        perror(pname);
        return 1;
    }
    fread (packed, 1, plen, ifp);
    fclose (ifp);
    //Getting unpacked file size & allocating space
    ulen = rnc_ulen (packed);
    if (ulen < 0) 
    {
	free (packed);
	if (ulen == -1) // File wasn't RNC to start with
	    return 0;
	printf("Error: %s\n", rnc_error (ulen));
	return 1;
    }

    //Creating output buffer, 8 bytes are for safety
    unpacked = malloc(ulen+8);
    if (!unpacked)
    {
	    perror(pname);
        return 1;
    }

    //Do the decompression
    ulen = rnc_unpack (packed, unpacked, 0);
    if (ulen < 0)
    {
	printf("%s\n", rnc_error (ulen));
	return 1;
    }

    //Write results to a file
    ofp = fopen(oname, "wb");
    if (!ofp)
    {
        perror(oname);
        return 1;
    }

    fwrite (unpacked, 1, ulen, ofp);
    fclose (ofp);

    free (unpacked);
    free (packed);

    return 0;
}

int copy_file (char *iname, char *oname)
{
    char *sysbuf;
    
    sysbuf = malloc (strlen (iname)+strlen(oname)+6);
    if (!sysbuf)
    {
	fprintf (stderr, "Out of memory.\n");
	return 1;
    }
    strcpy (sysbuf, "cp ");
    strcat (sysbuf, iname);
    strcat (sysbuf, " ");
    strcat (sysbuf, oname);
    system (sysbuf);
    free (sysbuf);
    return 0;
}

#endif
    
typedef struct {
    unsigned long bitbuf;	       /* holds between 16 and 32 bits */
    int bitcount;		       /* how many bits does bitbuf hold? */
} bit_stream;

typedef struct {
    int num;			       /* number of nodes in the tree */
    struct {
	unsigned long code;
	int codelen;
	int value;
    } table[32];
} huf_table;

static void read_huftable (huf_table *h, bit_stream *bs,
                   unsigned char **p, unsigned char *pend);
static unsigned long huf_read (huf_table *h, bit_stream *bs,
			       unsigned char **p,unsigned char *pend);

static void bitread_init (bit_stream *bs, unsigned char **p, unsigned char *pend);
static void bitread_fix (bit_stream *bs, unsigned char **p, unsigned char *pend);
static unsigned long bit_peek (bit_stream *bs, unsigned long mask);
static void bit_advance (bit_stream *bs, int n,
                   unsigned char **p, unsigned char *pend);
static unsigned long bit_read (bit_stream *bs, unsigned long mask,
			       int n, unsigned char **p, unsigned char *pend);

static unsigned long blong (unsigned char *p);
static unsigned long llong (unsigned char *p);
static unsigned long bword (unsigned char *p);
static unsigned long lword (unsigned char *p);

static unsigned long mirror (unsigned long x, int n);

/*
 * Return an error string corresponding to an error return code.
 */
char *rnc_error (long errcode) {
    static char *const errors[] = {
	"No error",
	"File is not RNC-1 format",
	"Huffman decode error",
	"File size mismatch",
	"CRC error in packed data",
	"CRC error in unpacked data",
	"Compressed file header invalid",
	"Huffman decode leads outside buffers",
	"Unknown error"
    };

    errcode = -errcode;
    if (errcode < 0)
	errcode = 0;
    if (errcode > sizeof(errors)/sizeof(*errors) - 1)
	errcode = sizeof(errors)/sizeof(*errors) - 1;
    return errors[errcode];
}

// Return the uncompressed length of a packed data block, or a
// negative error code.
long rnc_ulen (void *packed)
{
    unsigned char *p = (unsigned char *)packed;
    if (blong (p) != RNC_SIGNATURE)
	return RNC_FILE_IS_NOT_RNC;
    return blong (p+4);
}

// Decompress a packed data block. Returns the unpacked length if
// successful, or negative error codes if not.

// If COMPRESSOR is defined, it also returns the leeway number
// (which gets stored at offset 16 into the compressed-file header)
// in `*leeway', if `leeway' isn't NULL.
long rnc_unpack (void *packed, void *unpacked, unsigned int flags
#ifdef COMPRESSOR
		 , long *leeway
#endif
		 )
{
    unsigned char *input = (unsigned char *)packed;
    unsigned char *output = (unsigned char *)unpacked;
    unsigned char *inputend, *outputend;
    bit_stream bs;
    huf_table raw, dist, len;
    unsigned long ch_count;
    unsigned long ret_len, inp_len;
    unsigned out_crc;
#ifdef COMPRESSOR
    long lee = 0;
#endif
    if (blong(input) != RNC_SIGNATURE)
        if (!(flags&RNC_IGNORE_HEADER_VAL_ERROR)) return RNC_HEADER_VAL_ERROR;
    ret_len = blong (input+4);
    inp_len = blong (input+8);
    if ((ret_len>(1<<30))||(inp_len>(1<<30)))
        return RNC_HEADER_VAL_ERROR;

    outputend = output + ret_len;
    inputend = input + 18 + inp_len;

    input += 18;		       // skip header

    // Check the packed-data CRC. Also save the unpacked-data CRC
    // for later.

    if (rnc_crc(input, inputend-input) != bword(input-4))
	    if (!(flags&RNC_IGNORE_PACKED_CRC_ERROR)) return RNC_PACKED_CRC_ERROR;
    out_crc = bword(input-6);

    bitread_init (&bs, &input, inputend);
    bit_advance (&bs, 2, &input, inputend);      // discard first two bits

   // Process chunks.

  while (output < outputend)
  {
#ifdef COMPRESSOR
	  long this_lee;
#endif
      if (inputend-input<6)
      {
          if (!(flags&RNC_IGNORE_HUF_EXCEEDS_RANGE)) 
              return RNC_HUF_EXCEEDS_RANGE;
            else
              {output=outputend;ch_count=0;break;}
      }
	  read_huftable (&raw,  &bs, &input, inputend);
	  read_huftable (&dist, &bs, &input, inputend);
      read_huftable (&len,  &bs, &input, inputend);
      ch_count = bit_read (&bs, 0xFFFF, 16, &input, inputend);

      while (1)
      {
	    long length, posn;

	    length = huf_read (&raw, &bs, &input,inputend);
//	    if (length == -1) Tomasz Lis's modification
        if (length < 0)
            {
            if (!(flags&RNC_IGNORE_HUF_DECODE_ERROR)) 
                return RNC_HUF_DECODE_ERROR;
            else
                {output=outputend;ch_count=0;break;}
            }
	    if (length)
        {
		    while (length--)
            {
                if ((input>=inputend)||(output>=outputend))
                   {
                   if (!(flags&RNC_IGNORE_HUF_EXCEEDS_RANGE)) 
                       return RNC_HUF_EXCEEDS_RANGE;
                   else
                       {output=outputend;ch_count=0;break;}
                   }
		        *output++ = *input++;
            }
            bitread_fix (&bs, &input, inputend);
	    }
	    if (--ch_count <= 0)
		    break;

	    posn = huf_read (&dist, &bs, &input,inputend);
	    if (posn == -1)
        {
            if (!(flags&RNC_IGNORE_HUF_DECODE_ERROR)) 
		        return RNC_HUF_DECODE_ERROR;
            else
                {output=outputend;ch_count=0;break;}
        }
	    length = huf_read (&len, &bs, &input,inputend);
//	    if (length == -1) Tomasz Lis's modification
        if (length < 0)
        {
            if (!(flags&RNC_IGNORE_HUF_DECODE_ERROR)) 
		        return RNC_HUF_DECODE_ERROR;
            else
                {output=outputend;ch_count=0;break;}
        }
	    posn += 1;
	    length += 2;
	    while (length--)
        {
            if (((output-posn)<(unsigned char *)unpacked)||((output-posn)>outputend)||
                (((void *)output)<unpacked)||((output)>outputend))
            {
                   if (!(flags&RNC_IGNORE_HUF_EXCEEDS_RANGE)) 
                       return RNC_HUF_EXCEEDS_RANGE;
                   else
                       {output=outputend-1;ch_count=0;break;}
            }
		    *output = output[-posn];
		    output++;
	    }
#ifdef COMPRESSOR
	    this_lee = (inputend - input) - (outputend - output);
	    if (lee < this_lee)
		    lee = this_lee;
#endif
	  }
  }

    if (outputend != output)
    {
        if (!(flags&RNC_IGNORE_FILE_SIZE_MISMATCH))
	        return RNC_FILE_SIZE_MISMATCH;
    }

#ifdef COMPRESSOR
    if (leeway)
	    *leeway = lee;
#endif

    // Check the unpacked-data CRC.

    if (rnc_crc(outputend-ret_len, ret_len) != out_crc)
    {
        if (!(flags&RNC_IGNORE_UNPACKED_CRC_ERROR))
            return RNC_UNPACKED_CRC_ERROR;
    }

    return ret_len;
}

// Read a Huffman table out of the bit stream and data stream given.
static void read_huftable (huf_table *h, bit_stream *bs,
                          unsigned char **p, unsigned char *pend)
{
    int i, j, k, num;
    int leaflen[32];
    int leafmax;
    unsigned long codeb;	       // big-endian form of code

    num = bit_read (bs, 0x1F, 5, p, pend);
    if (!num)
        return;

    leafmax = 1;
    for (i=0; i<num; i++)
    {
        leaflen[i] = bit_read (bs, 0x0F, 4, p, pend);
        if (leafmax < leaflen[i])
	        leafmax = leaflen[i];
    }

    codeb = 0L;
    k = 0;
    for (i=1; i<=leafmax; i++)
    {
	for (j=0; j<num; j++)
	    if (leaflen[j] == i)
        {
		    h->table[k].code = mirror (codeb, i);
		    h->table[k].codelen = i;
            h->table[k].value = j;
            codeb++;
            k++;
	    }
	codeb <<= 1;
    }

    h->num = k;
}

// Read a value out of the bit stream using the given Huffman table.
static unsigned long huf_read (huf_table *h, bit_stream *bs,
			       unsigned char **p,unsigned char *pend)
{
    int i;
    unsigned long val;

    for (i=0; i<h->num; i++)
    {
        unsigned long mask = (1 << h->table[i].codelen) - 1;
        if (bit_peek(bs, mask) == h->table[i].code)
	        break;
    }
    if (i == h->num)
        return (unsigned long)-1;
    bit_advance (bs, h->table[i].codelen, p, pend);

    val = h->table[i].value;

    if (val >= 2)
    {
	    val = 1 << (val-1);
        val |= bit_read (bs, val-1, h->table[i].value - 1, p, pend);
    }
    return val;
}

// Initialises a bit stream with the first two bytes of the packed
// data.
// Checks pend for proper buffer pointers range.
static void bitread_init (bit_stream *bs, unsigned char **p, unsigned char *pend)
{
    if (pend-(*p) >= 0)
        bs->bitbuf = lword (*p);
    else
        bs->bitbuf = 0;
    bs->bitcount = 16;
}

// Fixes up a bit stream after literals have been read out of the
// data stream.
// Checks pend for proper buffer pointers range.
static void bitread_fix (bit_stream *bs, unsigned char **p, unsigned char *pend)
{
    bs->bitcount -= 16;
    bs->bitbuf &= (1<<bs->bitcount)-1; // remove the top 16 bits
    if (pend-(*p) >= 0)
        bs->bitbuf |= (lword(*p)<<bs->bitcount);// replace with what's at *p
    bs->bitcount += 16;
}

// Returns some bits.
static unsigned long bit_peek (bit_stream *bs, unsigned long mask)
{
    return bs->bitbuf & mask;
}

// Advances the bit stream.
// Checks pend for proper buffer pointers range.
static void bit_advance (bit_stream *bs, int n, unsigned char **p, unsigned char *pend)
{
    bs->bitbuf >>= n;
    bs->bitcount -= n;
    if (bs->bitcount < 16)
    {
        (*p) += 2;
        if (pend-(*p) >= 0)
            bs->bitbuf |= (lword(*p)<<bs->bitcount);
        bs->bitcount += 16;
    }
}

// Reads some bits in one go (ie the above two routines combined).
static unsigned long bit_read (bit_stream *bs, unsigned long mask,
			       int n, unsigned char **p, unsigned char *pend)
{
    unsigned long result = bit_peek (bs, mask);
    bit_advance (bs, n, p, pend);
    return result;
}

// Return the big-endian longword at p.
static unsigned long blong (unsigned char *p)
{
    unsigned long n;
    n = p[0];
    n = (n << 8) + p[1];
    n = (n << 8) + p[2];
    n = (n << 8) + p[3];
    return n;
}

// Return the little-endian longword at p.
static unsigned long llong (unsigned char *p)
{
    unsigned long n;
    n = p[3];
    n = (n << 8) + p[2];
    n = (n << 8) + p[1];
    n = (n << 8) + p[0];
    return n;
}

// Return the big-endian word at p.
static unsigned long bword (unsigned char *p)
{
    unsigned long n;
    n = p[0];
    n = (n << 8) + p[1];
    return n;
}

// Return the little-endian word at p.
static unsigned long lword (unsigned char *p)
{
    unsigned long n;
    n = p[1];
    n = (n << 8) + p[0];
    return n;
}

// Mirror the bottom n bits of x.
static unsigned long mirror (unsigned long x, int n) {
    unsigned long top = 1 << (n-1), bottom = 1;
    while (top > bottom)
    {
        unsigned long mask = top | bottom;
        unsigned long masked = x & mask;
        if (masked != 0 && masked != mask)
            x ^= mask;
        top >>= 1;
        bottom <<= 1;
    }
    return x;
}

// Calculate a CRC, the RNC way. It re-computes its CRC table every
// time it's run, but who cares? ;-)
long rnc_crc (void *data, unsigned long len)
{
    unsigned short crctab[256];
    unsigned short val;
    int i, j;
    unsigned char *p = (unsigned char *)data;
//computing CRC table
    for (i=0; i<256; i++)
    {
        val = i;

        for (j=0; j<8; j++)
        {
          if (val & 1)
       	    val = (val >> 1) ^ 0xA001;
          else
            val = (val >> 1);
	    }
	    crctab[i] = val;
    }

    val = 0;
    while (len--)
    {
	    val ^= *p++;
	    val = (val >> 8) ^ crctab[val & 0xFF];
    }

    return val;
}
