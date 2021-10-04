/*
 * (c) Copyright 2021 by Einar Saukas. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * The name of its author may not be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>

#include "zx5.h"

unsigned char* output_data;
int output_index;
int input_index;
int bit_index;
int bit_mask;
int diff;
int skip_next;

void read_bytes(int n, int *delta) {
    input_index += n;
    diff += n;
    if (diff > *delta)
        *delta = diff;
}

void write_byte(int value) {
    output_data[output_index++] = value;
    diff--;
}

void write_bit(int value) {
    if (skip_next) {
        skip_next = FALSE;
    } else {
        if (!bit_mask) {
            bit_mask = 128;
            bit_index = output_index;
            write_byte(0);
        }
        if (value)
            output_data[bit_index] |= bit_mask;
        bit_mask >>= 1;
    }
}

void write_interlaced_elias_gamma(int value, int backwards_mode, int invert_mode) {
    int i;

    for (i = 2; i <= value; i <<= 1)
        ;
    i >>= 1;
    while (i >>= 1) {
        write_bit(backwards_mode);
        write_bit(invert_mode ? !(value & i) : (value & i));
    }
    write_bit(!backwards_mode);
}

unsigned char *compress(BLOCK *optimal, unsigned char *input_data, int input_size, int skip, int backwards_mode, int invert_mode, int *output_size, int *delta) {
    BLOCK *prev;
    BLOCK *next;
    int last_offset1 = INITIAL_OFFSET;
    int last_offset2 = -1;
    int last_offset3 = -1;
    int i;

    /* calculate and allocate output buffer */
    *output_size = (optimal->bits+20+7)/8;
    output_data = (unsigned char *)malloc(*output_size);
    if (!output_data) {
         fprintf(stderr, "Error: Insufficient memory\n");
         exit(1);
    }

    /* initialize delta */
    diff = *output_size-input_size+skip;
    *delta = 0;

    /* un-reverse optimal sequence */
    prev = NULL;
    while (optimal) {
        next = optimal->chain;
        optimal->chain = prev;
        prev = optimal;
        optimal = next;
    }

    input_index = skip;
    output_index = 0;
    bit_mask = 0;
    skip_next = TRUE;

    for (optimal = prev->chain; optimal; optimal = optimal->chain) {
        if (!optimal->offset) {
            /* copy literals indicator */
            write_bit(0);

            /* copy literals length */
            write_interlaced_elias_gamma(optimal->length, backwards_mode, FALSE);

            /* copy literals values */
            for (i = 0; i < optimal->length; i++) {
                write_byte(input_data[input_index]);
                read_bytes(1, delta);
            }
        } else if (optimal->offset == last_offset1) {
            /* copy from last offset indicator */
            write_bit(0);

            /* copy from last offset length */
            write_interlaced_elias_gamma(optimal->length, backwards_mode, FALSE);
            read_bytes(optimal->length, delta);
        } else if (optimal->offset == last_offset2) {
            /* copy from 2nd last offset indicator */
            write_bit(1);
            write_bit(0);
            write_bit(0);

            /* copy from last offset length */
            write_interlaced_elias_gamma(optimal->length, backwards_mode, FALSE);
            read_bytes(optimal->length, delta);

            last_offset2 = last_offset1;
            last_offset1 = optimal->offset;
        } else if (optimal->offset == last_offset3) {
            /* copy from 3rd last offset indicator */
            write_bit(1);
            write_bit(0);
            write_bit(1);

            /* copy from last offset length */
            write_interlaced_elias_gamma(optimal->length, backwards_mode, FALSE);
            read_bytes(optimal->length, delta);

            last_offset3 = last_offset2;
            last_offset2 = last_offset1;
            last_offset1 = optimal->offset;
        } else {
            /* copy from new offset indicator */
            write_bit(1);
            write_bit(1);
            write_bit(optimal->length > 2 ? backwards_mode : !backwards_mode);

            /* copy from new offset MSB */
            write_interlaced_elias_gamma((optimal->offset-1)/256+1, backwards_mode, invert_mode);

            /* copy from new offset LSB */
            if (backwards_mode)
                write_byte((optimal->offset-1)%256);
            else
                write_byte(255-((optimal->offset-1)%256));

            /* copy from new offset length */
            skip_next = TRUE;
            write_interlaced_elias_gamma(optimal->length-1, backwards_mode, FALSE);
            read_bytes(optimal->length, delta);

            last_offset3 = last_offset2;
            last_offset2 = last_offset1;
            last_offset1 = optimal->offset;
        }
    }

    /* end marker */
    write_bit(1);
    write_bit(1);
    write_bit(0);
    write_interlaced_elias_gamma(256, backwards_mode, invert_mode);

    return output_data;
}
