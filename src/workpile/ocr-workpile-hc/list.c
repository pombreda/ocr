/* Copyright (c) 2012, Rice University

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

1.  Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
2.  Redistributions in binary form must reproduce the above
     copyright notice, this list of conditions and the following
     disclaimer in the documentation and/or other materials provided
     with the distribution.
3.  Neither the name of Intel Corporation
     nor the names of its contributors may be used to endorse or
     promote products derived from this software without specific
     prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include <stdlib.h>
#include <assert.h>

#include "ocr-macros.h"
#include "list.h"
#include "hc_sysdep.h"

void list_init(list_t * lst) {
    lst->head = NULL;
    lst->current = NULL;
    lst->last_peeked = NULL;
}

/*
 * concurrent push of entry onto the head of the list
 */
void list_push(list_t* lst, void* entry) {
    list_node_t * n = checked_malloc(n, sizeof(list_node_t));
    n->data = entry;
    list_node_t * head = (list_node_t *) lst->head;
    n->next = head;
    while(!hc_pointer_cas((void * volatile *)(&(lst->head)), head, n)) {
        head = (list_node_t *) lst->head;
        n->next = head;
    }
}

/*
 * removes the node next to current
 */
void * list_pop(list_t * lst) {
    if (lst->last_peeked == NULL) return NULL;
    if (lst->current && lst->current->next == lst->last_peeked) {
        lst->current->next = lst->last_peeked->next;
    } else {
        if (!hc_pointer_cas((void * volatile *)(&(lst->head)), lst->last_peeked, lst->last_peeked->next)) {
            list_node_t * n = (list_node_t *) lst->head;
            while(n && n->next != lst->last_peeked)
                n = n->next;
            assert(n);
            n->next = lst->last_peeked->next;
        }
    }
    void * data = lst->last_peeked->data;
    free(lst->last_peeked);
    lst->last_peeked = NULL;
    return data;
}

/*
 * return the data in next node without advancing current
 */
void * list_peek(list_t * lst) {
    list_node_t * n;
    if (lst->current == NULL) { 
        n = (list_node_t *) lst->head;
    } else {
        n = lst->current->next;
    }
    lst->last_peeked = n;
    if (n == NULL) return NULL;
    return n->data;
}

/*
 * advance current to next node and return the data
 */
void * list_next(list_t * lst) {
    list_node_t * n;
    if (lst->current == NULL) { 
        n = (list_node_t *) lst->head;
    } else {
        n = lst->current->next;
    }
    lst->current = n;
    lst->last_peeked = NULL;
    if (n == NULL) return NULL;
    return n->data;
}

