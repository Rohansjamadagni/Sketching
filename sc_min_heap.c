/*
 * BSD-3-Clause
 *
 * Copyright 2021 Ozan Tezcan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <assert.h>
#include "sc_min_heap.h"
#include "uthash.h"

#ifndef SC_HEAP_MAX
#define SC_HEAP_MAX (SIZE_MAX / sizeof(struct sc_heap_data))
#endif

bool sc_heap_init(struct sc_heap *h, size_t cap)
{
	struct sc_heap_data *e;
	const size_t sz = cap * sizeof(struct sc_heap_data);

	*h = (struct sc_heap){0};

	if (cap == 0) {
		return true;
	}

	if (cap > SC_HEAP_MAX || (e = (struct sc_heap_data*)sc_heap_malloc(sz)) == NULL) {
		return false;
	}

	h->elems = e;
	h->cap = cap;
  h->ht = NULL;

	return true;
}

void sc_heap_term(struct sc_heap *h)
{
	sc_heap_free(h->elems);

	*h = (struct sc_heap){
		.elems = NULL,
	};
}

size_t sc_heap_size(struct sc_heap *h)
{
	return h->size;
}

void sc_heap_clear(struct sc_heap *h)
{
	h->size = 0;
}

// Helper to add a new hash entry. Returns true on success.
static inline bool _sc_add_hash_entry(struct sc_heap *h, uint64_t data_key, size_t index) {
    struct sc_ht_entry *entry;

    entry = (struct sc_ht_entry *)sc_heap_malloc(sizeof(struct sc_ht_entry));
    if (!entry) {
        return false; // OOM
    }
    entry->data = data_key;
    entry->heap_index = index;
    HASH_ADD(hh, h->ht, data, sizeof(uint64_t), entry);
    return true;
}

static inline bool _sc_delete_hash_entry(struct sc_heap *h, uint64_t data_key) {
    struct sc_ht_entry *entry;
    HASH_FIND(hh, h->ht, &data_key, sizeof(uint64_t), entry);
    if (entry) {
        HASH_DEL(h->ht, entry);
        sc_heap_free(entry);
        return true;
    }
    return false;
}

bool sc_heap_add(struct sc_heap *h, uint64_t key, uint64_t data)
{
  sc_heap_delete_if_present(h, data);
	size_t i, cap, m;
	struct sc_heap_data* exp;

	if (++h->size >= h->cap) {
		cap = h->cap != 0 ? h->cap * 2 : 4;
		m = cap * 2 * sizeof(*h->elems);

		if (h->cap >= SC_HEAP_MAX / 2 ||
		    (exp = (struct sc_heap_data*)sc_heap_realloc(h->elems, m)) == NULL) {
			return false;
		}

		h->elems = exp;
		h->cap = cap;
	}

	i = h->size;
	while (i != 1 && key < h->elems[i / 2].key) {
		h->elems[i] = h->elems[i / 2];
		i /= 2;
	}

	h->elems[i].key = key;
	h->elems[i].data = data;

  _sc_add_hash_entry(h, data, i);

	return true;
}

struct sc_heap_data *sc_heap_peek(struct sc_heap *h)
{
	if (h->size == 0) {
		return NULL;
	}

	// Top element is always at heap->elems[1].
	return &h->elems[1];
}

struct sc_heap_data *sc_heap_pop(struct sc_heap *h)
{
	size_t i = 1, child = 2;
	struct sc_heap_data last;

	if (h->size == 0) {
		return NULL;
	}

	// Top element is always at heap->elems[1].
	h->elems[0] = h->elems[1];

	last = h->elems[h->size--];
	while (child <= h->size) {
		if (child < h->size &&
		    h->elems[child].key > h->elems[child + 1].key) {
			child++;
		}

		if (last.key <= h->elems[child].key) {
			break;
		}

		h->elems[i] = h->elems[child];

		i = child;
		child *= 2;
	}

	h->elems[i] = last;
  _sc_delete_hash_entry(h, h->elems[0].data);

	return &h->elems[0];
}

static inline void swap_heap_data(struct sc_heap_data *a, struct sc_heap_data *b) {
    struct sc_heap_data temp = *a;
    *a = *b;
    *b = temp;
}

static inline size_t parent_idx(size_t i) {
    return i / 2;
}

static inline size_t left_child_idx(size_t i) {
    return 2 * i;
}

static inline size_t right_child_idx(size_t i) {
    return 2 * i + 1;
}

static void heapify_up(struct sc_heap *h, size_t index) {
  assert(index >= 1 && index <= h->size);
    while (index > 1 && h->elems[index].data < h->elems[parent_idx(index)].data) {
        swap_heap_data(&h->elems[index], &h->elems[parent_idx(index)]);
        index = parent_idx(index); // Move up to the parent's index
    }
}

static void heapify_down(struct sc_heap *h, size_t index) {
  assert(index >= 1 && index <= h->size);
    size_t heap_size = h->size;
    size_t current_idx = index;

    while (1) {
        size_t smallest_idx = current_idx;
        size_t left_idx = left_child_idx(current_idx);
        size_t right_idx = right_child_idx(current_idx);

        if (left_idx <= heap_size && h->elems[left_idx].data < h->elems[smallest_idx].data) {
            smallest_idx = left_idx;
        }

        if (right_idx <= heap_size && h->elems[right_idx].data < h->elems[smallest_idx].data) {
            smallest_idx = right_idx;
        }

        if (smallest_idx == current_idx) {
            break;
        }

        swap_heap_data(&h->elems[current_idx], &h->elems[smallest_idx]);
        current_idx = smallest_idx;
    }
}

bool sc_heap_delete(struct sc_heap *h, uint64_t key) {
  if (h == NULL || h->size == 0) {
        fprintf(stderr, "Delete failed because heap was null or size =0\n");
        return false;
    }

    size_t index_to_delete = 0;
    for (size_t i = 1; i <= h->size; ++i) {
        if (h->elems[i].data == key) {
            index_to_delete = i;
            break;
        }
    }

    if (index_to_delete == 0) {
        fprintf(stderr, "Delete failed cuz Couldn't find key\n");
        return false;
    }

    size_t last_index = h->size;

    if (index_to_delete == last_index) {
        h->size--;
        return true;
    }

    swap_heap_data(&h->elems[index_to_delete], &h->elems[last_index]);

    h->size--;


    if (index_to_delete > 1 && h->elems[index_to_delete].data < h->elems[parent_idx(index_to_delete)].data) {
         heapify_up(h, index_to_delete);
    } else {
        heapify_down(h, index_to_delete);
    }

    return true;
}

bool sc_heap_delete_if_present(struct sc_heap *h, uint64_t data) {

  struct sc_ht_entry *entry = NULL;
  HASH_FIND(hh, h->ht, &data, sizeof(uint64_t), entry);

  if (entry) {
    size_t i = entry->heap_index;
    _sc_delete_hash_entry(h, data);
    if (i > h->size) return false;
    if(!sc_heap_delete(h, h->elems[i].data)) {
      fprintf(stderr, "Failed to delete element with key: %ld\n", data);
      return false;
    }
    return true;

  }
  _sc_delete_hash_entry(h, data);

  return false;
}
//
// void sc_heap_copy(struct sc_heap *dest, struct sc_heap *src){
//
//   dest->cap = src->cap;
//   dest->size = src->size;
//   dest->elems = malloc(sizeof(struct sc_heap_data) * src->cap);
//   memcpy(dest->elems, src->elems, sizeof(struct sc_heap_data) * src->size);
//
// }
