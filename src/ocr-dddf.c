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

#include <stdio.h>
#include <assert.h>
#include <pthread.h>

#include "ocr-macros.h"
#include "ocr-runtime.h"
#include "hc_sysdep.h"
#include "ocr-mpi.h"
#include "ocr-dddf.h"

#define D3F_TERMINATION_TAG 0
#define D3F_AWAIT_TAG       1
#define D3F_UNIQ_TAG_START  2

#define D3F_HASHTABLE_SIZE 1024

volatile int unique_tag; /* unique tag for remote awaits */

typedef struct ocr_d3f_event_struct {
    ocrGuid_t ocr_event;
} ocr_d3f_event_t;

typedef struct d3f_hashtable_entry_struct {
    ocrGuid_t event;
    size_t id;
    volatile struct d3f_hashtable_entry_struct * next;
} d3f_hashtable_entry_t;

typedef struct d3f_hashtable_bucket_struct {
    volatile d3f_hashtable_entry_t * entry;
    pthread_mutex_t lock;
} d3f_hashtable_bucket_t;

d3f_hashtable_bucket_t * d3f_hashtable;

/* quick and dumb hash function */
static u32 getHashBucket(size_t id) {
    return id % D3F_HASHTABLE_SIZE;
}

static d3f_hashtable_entry_t * create_hashtable_entry(ocrGuid_t event, size_t id) {
    d3f_hashtable_entry_t * el = (d3f_hashtable_entry_t *) checked_malloc (el, sizeof(d3f_hashtable_entry_t));
    el->event = event;
    el->id = id;
    el->next = NULL;
    return el;
}

static ocrGuid_t getDDDF(size_t id) {
    u32 bucket = getHashBucket(id);
    volatile d3f_hashtable_entry_t * current = d3f_hashtable[bucket].entry;
    while(current != NULL){
        if (current->id == id) return current->event;
        current = current->next;
    }
    return NULL_GUID;
}

/* Locked implementation */
static ocrGuid_t setDDDF(ocrGuid_t event, size_t id) {
    u32 bucket = getHashBucket(id);

    /* check if already exists */
    volatile d3f_hashtable_entry_t * current = d3f_hashtable[bucket].entry;
    while(current != NULL){
        if (current->id == id) return current->event;
        current = current->next;
    }

    /* set new element */
    pthread_mutex_lock(&(d3f_hashtable[bucket].lock));
    current = d3f_hashtable[bucket].entry;
    if (current == NULL) {
        d3f_hashtable[bucket].entry = (volatile d3f_hashtable_entry_t *) create_hashtable_entry(event, id);
        current = d3f_hashtable[bucket].entry;
    } else {
        while (current->next != NULL) {
            if (current->id == id) {
                pthread_mutex_unlock(&(d3f_hashtable[bucket].lock));
                return current->event;
            }
            current = current->next;
        }
        current->next = (volatile d3f_hashtable_entry_t *) create_hashtable_entry(event, id);
        current = current->next;
    }
    pthread_mutex_unlock(&(d3f_hashtable[bucket].lock));
    return current->event;
}

static u8 ocrD3FCreate(ocrGuid_t *guid, int id) {
    bool isOwner = false;
    ocrGuid_t d3f_guid = getDDDF(id);
    if (d3f_guid == NULL_GUID) {
        ocrEventCreate(&d3f_guid, OCR_EVENT_STICKY_T, true);
        ocrGuid_t tmp_guid = setDDDF(d3f_guid, id);
        if (tmp_guid != d3f_guid) {
            ocrEventDestroy(d3f_guid);
            d3f_guid = tmp_guid;
        } else {
            isOwner = true;
        }
    }
    *guid = d3f_guid;
    return isOwner;
}

static int unique_tag_generator() {
    return hc_fetch_and_add(&unique_tag, 2);
}

static void d3f_await_listener_callback_fn(ocr_mpi_communicator_t * comm, void * buf, size_t sz, int rproc, int tag) {
    int * data = (int*)buf;
    int id = data[0];
    int utag = data[1];
    //printf("Listener on %d received request from %d with id: %d and tag %d\n", comm->rank, rproc, id, utag);
    ocrGuid_t event;
    ocrD3FCreate(&event, id);
    ocrGuid_t task_guid = comm->create_db_push_on_satisfy(comm, rproc, utag, 1);
    ocrAddDependence(event, task_guid, 0);
    ocrEdtSchedule(task_guid);
}

static ocr_mpi_communicator_t * getD3FComm() {
    ocr_mpi_communicator_t * comm = (ocr_mpi_communicator_t *) ocrGetCommunicator(OCR_HC_MPI);
    assert(comm && "MPI communicator not found");
    return comm;
}

u8 ocrD3FModelFinalizeEDT ( u32 paramc, u64 * params, void* paramv[], u32 depc, ocrEdtDep_t depv[]) {
    ocrFinish();
    return 0;
}

/****************************************************/
/* OCR DDDF API                                     */
/****************************************************/

u8 ocrD3FModelInit(void) {
    size_t i;
    d3f_hashtable = (d3f_hashtable_bucket_t *) checked_malloc(d3f_hashtable, sizeof(d3f_hashtable_bucket_t) * D3F_HASHTABLE_SIZE);
    for (i = 0; i < D3F_HASHTABLE_SIZE; i++) {
        d3f_hashtable[i].entry = NULL;
        pthread_mutex_init(&(d3f_hashtable[i].lock), NULL);
    }
    unique_tag = D3F_UNIQ_TAG_START;

    /* setup the await listener on home */
    size_t sz = sizeof(int) * 2;
    void * buf = checked_malloc(buf, sz);
    ocr_mpi_communicator_t * comm = getD3FComm();
    ocrGuid_t task_guid = comm->create_any_source_listener(comm, buf, sz, D3F_AWAIT_TAG, 0, d3f_await_listener_callback_fn);
    ocrEdtSchedule(task_guid);

    return 0;
}

u8 ocrD3FAddDependence(int id, int home, ocrGuid_t edt, u32 slot) {
    ocr_mpi_communicator_t * comm = getD3FComm();
    //printf("Dependence on %d with home %d on rank %d\n", id, home, comm->rank);
    assert((home < comm->size) && "(ocrD3FAddDependence): DDDF home location is out of communicator scope!");

    ocrGuid_t event;
    bool isOwner = ocrD3FCreate(&event, id);
    ocrAddDependence(event, edt, slot);

    if (comm->rank != home && isOwner == true) {
        size_t sz = sizeof(int) * 2;
        int * buf = checked_malloc(buf, sz);
        buf[0] = id;
        buf[1] = unique_tag_generator();
        ocrGuid_t task_guid = comm->create_db_pull_then_satisfy(comm, buf, sz, home, D3F_AWAIT_TAG, 0, event);
        ocrEdtSchedule(task_guid);
    }
    return 0;
}

u8 ocrD3FSatisfy(int id, int home, ocrGuid_t dataGuid) {
    ocr_mpi_communicator_t * comm = getD3FComm();
    //printf("Satisfy %d with home %d on rank %d\n", id, home, comm->rank);
    assert((home < comm->size) && "(ocrD3FSatisfy): DDDF home location is out of communicator scope!");
    if (comm->rank != home) assert(false && "Remote puts not yet supported!");

    ocrGuid_t event;
    ocrD3FCreate(&event, id);
    ocrEventSatisfy(event, dataGuid);
    return 0;
}

u8 ocrD3FModelFinalize(int id, int home) {
    ocr_mpi_communicator_t * comm = getD3FComm();
    //printf("Finalize %d with home %d on rank %d\n", id, home, comm->rank);
    assert((home < comm->size) && "(ocrD3FModelFinalize): DDDF home location is out of communicator scope!");

    ocrGuid_t event;
    bool isOwner = ocrD3FCreate(&event, id);
    if (comm->rank == home) {
        // After termination is signalled at the home location, 
        // the home needs to send out termination messages to other nodes.
        // The termination edt waits for all termination messages sends to complete
        ocrGuid_t term_edt_guid;
        ocrEdtCreate(&term_edt_guid, ocrD3FModelFinalizeEDT, 0, NULL, NULL, 0, comm->size, NULL);
        ocrAddDependence(event, term_edt_guid, 0);
        int i, slot;
        for (i = 0, slot = 1; i < comm->size; i++) {
            if (i != home) {
                ocrGuid_t comm_event;
                ocrEventCreate(&comm_event, OCR_EVENT_STICKY_T, true);
                ocrAddDependence(comm_event, term_edt_guid, slot++);

                ocrGuid_t comm_task = comm->create_send(comm, NULL, 0, i, D3F_TERMINATION_TAG, 1, comm_event);
                ocrAddDependence(event, comm_task, 0);
                ocrEdtSchedule(comm_task);
            }
        }
        ocrEdtSchedule(term_edt_guid);
    } else {
        assert(isOwner && "Finalize should be called only once on unique terminating DDDF id");
        // At remote node, termination edt waits for event being satisfied by recv.
        ocrGuid_t term_edt_guid;
        ocrEdtCreate(&term_edt_guid, ocrD3FModelFinalizeEDT, 0, NULL, NULL, 0, 1, NULL);
        ocrAddDependence(event, term_edt_guid, 0);
        ocrEdtSchedule(term_edt_guid);

        ocrGuid_t comm_task = comm->create_recv(comm, NULL, 0, home, D3F_TERMINATION_TAG, 0, event);
        ocrEdtSchedule(comm_task);
    }
    ocrCleanup();
    return 0;
}

u32 ocrD3FGetRank() {
    ocr_mpi_communicator_t * comm = getD3FComm();
    return comm->rank;
}

u32 ocrD3FGetNumRanks() {
    ocr_mpi_communicator_t * comm = getD3FComm();
    return comm->size;
}
