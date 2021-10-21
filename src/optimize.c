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
#include <limits.h>

#include "zx5.h"

#define MAX_SCALE 55

int offset_ceiling(int index, int offset_limit) {
    return index > offset_limit ? offset_limit : index < INITIAL_OFFSET ? INITIAL_OFFSET : index;
}

int elias_gamma_bits(int value) {
    int bits = 1;
    while (value >>= 1)
        bits += 2;
    return bits;
}

int hash(int offset1, int offset2, int offset3) {
    return (offset1+offset2+offset3) % HASH_SIZE;
}

void erase_table(CELL *cell) {
    int i;

    for (i = 0; i < HASH_SIZE; i++)
        if (cell->table[i]) {
            free_list_entries(cell->table[i]);
            cell->table[i] = NULL;
        }
}

int prepare_cell(CELL *cell, int bits, int index) {
    if (!cell->bits) {
        cell->bits = bits;
        cell->index = index;
        return TRUE;
    }
    if (cell->index != index || cell->bits > bits) {
        cell->bits = bits;
        cell->index = index;
        erase_table(cell);
        return TRUE;
    }
    return cell->bits == bits;
}

ENTRY *create_entry(ENTRY **list, int offset1, int offset2, int offset3) {
    ENTRY *entry = allocate_entry(offset1, offset2, offset3);

    if (*list) {
        entry->next = (*list)->next;
        (*list)->next = entry;
    } else {
        entry->next = entry;
        *list = entry;
    }
    return entry;
}

ENTRY *find_entry(CELL *cell, int offset1, int offset2, int offset3) {
    ENTRY *entry;
    int i = hash(offset1, offset2, offset3);

    for (entry = cell->table[i]; entry; entry = entry->next != cell->table[i] ? entry->next : NULL)
        if (entry->offset1 == offset1 && entry->offset2 == offset2 && entry->offset3 == offset3)
            return entry;
    return create_entry(&cell->table[i], offset1, offset2, offset3);
}

void add_first_block(CELL *dest, int bits, int index, int offset, int length) {
    ENTRY *entry_dest;

    prepare_cell(dest, bits, index);
    entry_dest = find_entry(dest, offset, 0, 0);
    assign_block(&entry_dest->block, allocate_block(bits, offset, length, NULL));
}

void add_literal_block(CELL *dest, int index, CELL *src) {
    ENTRY *entry_src;
    ENTRY *entry_dest;
    int i;
    int length = index-src->index;
    int bits = src->bits + 1 + elias_gamma_bits(length) + length*8;

    prepare_cell(dest, bits, index);
    for (i = 0; i < HASH_SIZE; i++)
        for (entry_src = src->table[i]; entry_src; entry_src = entry_src->next != src->table[i] ? entry_src->next : NULL) {
            entry_dest = create_entry(&dest->table[i], entry_src->offset1, entry_src->offset2, entry_src->offset3);
            assign_block(&entry_dest->block, allocate_block(bits, 0, length, entry_src->block));
        }
}

void add_last_offset_block(CELL *dest, int index, int offset, CELL *src) {
    ENTRY *entry_src;
    ENTRY *entry_dest;
    int i;
    int length = index-src->index;
    int bits = src->bits + 1 + elias_gamma_bits(length);

    prepare_cell(dest, bits, index);
    for (i = 0; i < HASH_SIZE; i++)
        for (entry_src = src->table[i]; entry_src; entry_src = entry_src->next != src->table[i] ? entry_src->next : NULL) {
            entry_dest = create_entry(&dest->table[i], entry_src->offset1, entry_src->offset2, entry_src->offset3);
            assign_block(&entry_dest->block, allocate_block(bits, offset, length, entry_src->block));
        }
}

int add_previous_offset_block(CELL *dest, int index, int offset, CELL *src) {
    ENTRY *entry_src;
    ENTRY *entry_dest;
    int i;
    int length = index-src->index;
    int bits = src->bits + 3 + elias_gamma_bits(length);
    int found = FALSE;

    if (!dest->bits || dest->index != index || dest->bits >= bits)
        for (i = 0; i < HASH_SIZE; i++)
            for (entry_src = src->table[i]; entry_src; entry_src = entry_src->next != src->table[i] ? entry_src->next : NULL)
                if (entry_src->offset2 == offset || entry_src->offset3 == offset) {
                    if (!found) {
                        prepare_cell(dest, bits, index);
                        found = TRUE;
                    }
                    entry_dest = find_entry(dest, offset, entry_src->offset1, entry_src->offset2 != offset ? entry_src->offset2 : entry_src->offset3);
                    if (!entry_dest->block)
                        assign_block(&entry_dest->block, allocate_block(bits, offset, length, entry_src->block));
                }
    return found;
}

int add_new_offset_block(CELL *dest, int index, int offset, CELL *src) {
    ENTRY *entry_src;
    ENTRY *entry_dest;
    int i;
    int length = index-src->index;
    int bits = src->bits + 10 + elias_gamma_bits((offset-1)/256+1) + elias_gamma_bits(length-1);

    if (prepare_cell(dest, bits, index)) {
        for (i = 0; i < HASH_SIZE; i++)
            for (entry_src = src->table[i]; entry_src; entry_src = entry_src->next != src->table[i] ? entry_src->next : NULL) {
                entry_dest = find_entry(dest, offset, entry_src->offset1, entry_src->offset2);
                if (!entry_dest->block)
                    assign_block(&entry_dest->block, allocate_block(bits, offset, length, entry_src->block));
            }
        return TRUE;
    }
    return FALSE;
}

void merge_blocks(CELL *dest, CELL *src) {
    ENTRY *entry_src;
    ENTRY *entry_dest;
    int i;

    dest->bits = src->bits;
    dest->index = src->index;
    for (i = 0; i < HASH_SIZE; i++)
        for (entry_src = src->table[i]; entry_src; entry_src = entry_src->next != src->table[i] ? entry_src->next : NULL) {
            entry_dest = find_entry(dest, entry_src->offset1, entry_src->offset2, entry_src->offset3);
            if (!entry_dest->block)
                assign_block(&entry_dest->block, entry_src->block);
        }
}

BLOCK *find_any_block(CELL *cell) {
    int i;

    for (i = 0; i < HASH_SIZE; i++)
        if (cell->table[i])
            return cell->table[i]->block;
    return NULL;
}

BLOCK* optimize(unsigned char *input_data, int input_size, int skip, int offset_limit) {
    CELL *last_literal;
    CELL *last_match;
    CELL *optimal;
    int index;
    int offset;
    int length;
    int optimal_bits;
    int dots = 2;
    int max_offset = offset_ceiling(input_size-1, offset_limit);

    /* allocate all main data structures at once */
    last_literal = (CELL *)calloc(max_offset+1, sizeof(CELL));
    last_match = (CELL *)calloc(max_offset+1, sizeof(CELL));
    optimal = (CELL *)calloc(input_size, sizeof(CELL));
    if (!last_literal || !last_match || !optimal) {
         fprintf(stderr, "Error: Insufficient memory\n");
         exit(1);
    }

    /* start with fake block */
    add_first_block(&last_match[INITIAL_OFFSET], -1, skip-1, INITIAL_OFFSET, 0);

    printf("[");

    /* process remaining bytes */
    for (index = skip; index < input_size; index++) {
        optimal_bits = INT_MAX;
        max_offset = offset_ceiling(index, offset_limit);
        for (offset = 1; offset <= max_offset; offset++) {
            if (index != skip && index >= offset && input_data[index] == input_data[index-offset]) {
                /* copy from last offset */
                if (last_literal[offset].bits) {
                    add_last_offset_block(&last_match[offset], index, offset, &last_literal[offset]);
                    if (optimal_bits > last_match[offset].bits)
                        optimal_bits = last_match[offset].bits;
                }
                /* copy from another offset */
                for (length = 1; length <= index-offset+1 && input_data[index-length+1] == input_data[index-length-offset+1]; length++)
                    if (add_previous_offset_block(&last_match[offset], index, offset, &optimal[index-length]) ||
                        (length > 1 && add_new_offset_block(&last_match[offset], index, offset, &optimal[index-length]))) 
                        if (optimal_bits > last_match[offset].bits)
                            optimal_bits = last_match[offset].bits;
            } else {
                /* copy literals */
                if (last_match[offset].bits) {
                    add_literal_block(&last_literal[offset], index, &last_match[offset]);
                    if (optimal_bits > last_literal[offset].bits)
                        optimal_bits = last_literal[offset].bits;
                }
            }
        }

        /* identify optimal choice so far */
        for (offset = 1; offset <= max_offset; offset++)
            if (last_match[offset].bits == optimal_bits && last_match[offset].index == index)
                merge_blocks(&optimal[index], &last_match[offset]);
            else if (last_literal[offset].bits == optimal_bits && last_literal[offset].index == index)
                merge_blocks(&optimal[index], &last_literal[offset]);

        /* indicate progress */
        if (index*MAX_SCALE/input_size > dots) {
            printf(".");
            fflush(stdout);
            dots++;
        }
    }

    printf("]\n");

    return find_any_block(&optimal[input_size-1]);
}
