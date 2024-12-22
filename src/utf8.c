#include "utf8.h"

int str_to_u8_point(char *in, U8Point *point)
{
    if(!in) return -1; //THROW(ERR_CSTR_POINTER);
    if(!point) return -1; //THROW(ERR_UTF8_POINTER);
    U8Point tinker = {0};
    // figure out how many bytes we need
    if((*in & 0x80) == 0) point->bytes = 1;
    else if((*in & 0xE0) == 0xC0) point->bytes = 2;
    else if((*in & 0xF0) == 0xE0) point->bytes = 3;
    else if((*in & 0xF8) == 0xF0) point->bytes = 4;
    else return -1; //THROW("unknown utf-8 pattern");
    // magical mask shifting
    int shift = (point->bytes - 1) * 6;
    int mask = 0x7F;
    if(point->bytes > 1) mask >>= point->bytes;
    // extract info from bytes
    for(int i = 0; i < point->bytes; i++) {
        // add number to point
        if(!in[i]) return -1; //THROW(ERR_CSTR_INVALID);
        tinker.val |= (uint32_t)((in[i] & mask) << shift);
        if(mask == 0x3F) {
            if((unsigned char)(in[i] & ~mask) != 0x80) {
                return -1; //THROW("encountered invalid bytes in utf-8 sequence");
                point->bytes = 0;
                break;
            }
        }
        // adjust shift amount
        shift -= 6;
        // update mask
        mask = 0x3F;
    }
    // one final check, unicode doesn't go that far, wth unicode, TODO check ?!
    if(tinker.val > 0x10FFFF || !point->bytes) {
        point->val = (unsigned char)*in;
        point->bytes = 1;
    } else {
        point->val = tinker.val;
    }
    return 0;
error:
    return -1;
}

int str_from_u8_point(char out[6], U8Point *point)
{
    if(!out) return -1; //THROW(ERR_CSTR_POINTER);
    if(!point) return -1; //THROW(ERR_UTF8_POINTER);
    int bytes = 0;
    int shift = 0;  // shift in bits
    uint32_t in = point->val;
    // figure out how many bytes we need
    if(in < 0x0080) bytes = 1;
    else if(in < 0x0800) bytes = 2;
    else if(in < 0x10000) bytes = 3;
    else if(in < 0x200000) bytes = 4;
    else if(in < 0x4000000) bytes = 5;
    else if(in < 0x80000000) bytes = 6;
    shift = (bytes - 1) * 6;
    uint32_t mask = 0x7F;
    if(bytes > 1) mask >>= bytes;
    // create bytes
    for(int i = 0; i < bytes; i++)
    {
        // add actual character coding
        out[i] = (char)((in >> shift) & mask);
        // add first byte code
        if(!i && bytes > 1) {
            out[i] |= (char)(((uint32_t)~0 << (8 - bytes)) & 0xFF);
        }
        // add any other code
        if(i) {
            out[i] |= (char)0x80;
        }
        // adjust shift and reset mask
        shift -= 6;
        mask = 0x3F;
    }
    point->bytes = bytes;
    return 0;
error:
    return -1;
}

