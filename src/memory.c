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

#define QTY_BLOCKS  10000
#define QTY_ENTRIES 10000

BLOCK *ghost_root_block = NULL;
BLOCK *dead_array_block = NULL;
int dead_array_block_size = 0;

ENTRY *ghost_root_entry = NULL;
ENTRY *dead_array_entry = NULL;
int dead_array_entry_size = 0;

BLOCK *allocate_block(int bits, int offset, int length, BLOCK *chain) {
    BLOCK *ptr;

    if (ghost_root_block) {
        ptr = ghost_root_block;
        ghost_root_block = ptr->chain;
    } else {
        if (!dead_array_block_size) {
            dead_array_block = (BLOCK *)malloc(QTY_BLOCKS*sizeof(BLOCK));
            if (!dead_array_block) {
                fprintf(stderr, "Error: Insufficient memory\n");
                exit(1);
            }
            dead_array_block_size = QTY_BLOCKS;
        }
        ptr = &dead_array_block[--dead_array_block_size];
    }
    ptr->bits = bits;
    ptr->offset = offset;
    ptr->length = length;
    if (chain)
        chain->references++;
    ptr->chain = chain;
    ptr->references = 0;
    return ptr;
}

void assign_block(BLOCK **ptr, BLOCK *chain) {
    BLOCK *last = *ptr;

    if (chain)
        chain->references++;
    if (last && !--last->references) {
        while (last->chain && !--last->chain->references) {
            last = last->chain;
        }
        last->chain = ghost_root_block;
        ghost_root_block = *ptr;
    }
    *ptr = chain;
}

ENTRY *allocate_entry(int offset1, int offset2, int offset3) {
    ENTRY *ptr;

    if (ghost_root_entry) {
        ptr = ghost_root_entry;
        ghost_root_entry = ptr->next;
        assign_block(&ptr->block, NULL);
    } else {
        if (!dead_array_entry_size) {
            dead_array_entry = (ENTRY *)malloc(QTY_ENTRIES*sizeof(ENTRY));
            if (!dead_array_entry) {
                fprintf(stderr, "Error: Insufficient memory\n");
                exit(1);
            }
            dead_array_entry_size = QTY_ENTRIES;
        }
        ptr = &dead_array_entry[--dead_array_entry_size];
        ptr->block = NULL;
    }
    ptr->offset1 = offset1;
    ptr->offset2 = offset2;
    ptr->offset3 = offset3;
    return ptr;
}

void free_list_entries(ENTRY *list) {
    ENTRY *root = ghost_root_entry;

    ghost_root_entry = list->next;
    list->next = root;
}
