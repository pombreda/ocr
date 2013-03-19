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
#include <stdlib.h>
#include "ocr-macros.h"
#include "ocr-db.h"
#include "ocr-mpi.h"
#include "hc_edf.h"
#include "hc.h"
#include "mpi.h"

typedef struct hc_mpi_args_struct {
    void * buf;
    size_t sz;
    int rproc; /* Remote process: Source for recv and Dest for send operations */
    int tag;
    MPI_Request req, req2;
    int phase;
    ocr_mpi_listener_callback_fct callback;

    /* This avoids multiple mallocs for task arguments which should be heap allocated. */
    u64 params;
    struct hc_mpi_args_struct * paramv;
    ocrGuid_t event;
    ocrGuid_t db;
} hc_mpi_args_t;

void hc_mpi_configure( ocr_communicator_t * base, void * configuration) {
    ocr_mpi_communicator_t* derived = (ocr_mpi_communicator_t*) base;
    void ** mpi_args = (void**) configuration;
    derived->argc = (int*) mpi_args[0];
    derived->argv = (char**) mpi_args[1];
}

void hc_mpi_init(ocr_communicator_t * base) {
    int rt, thread_level_provided;
    int MPI_THREAD_LEVEL = MPI_THREAD_SINGLE;
    ocr_mpi_communicator_t* derived = (ocr_mpi_communicator_t*) base;

    rt = MPI_Init_thread(derived->argc, &(derived->argv), MPI_THREAD_LEVEL, &thread_level_provided);

    if (rt != MPI_SUCCESS) {
        fprintf(stderr, "MPI_Init_thread failed with rt = %d\n",rt);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    if (thread_level_provided != MPI_THREAD_LEVEL) {
        fprintf(stderr, "%d thread level is needed, current level: %d\n", MPI_THREAD_LEVEL, thread_level_provided);
        MPI_Abort(MPI_COMM_WORLD, 2);
    }

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    derived->rank = rank;
    derived->size = size;
}

void hc_mpi_finalize(ocr_communicator_t * base) {
    MPI_Finalize();
}

void hc_mpi_destruct( ocr_communicator_t * base) {
    free(base);
}

/****************************************************/
/* HC-MPI TASK EXECUTION FUNCTIONS                  */
/****************************************************/

hc_comm_task_t * getCurrentCommTask() {
    hc_worker_t * worker = (hc_worker_t*)deguidify(ocr_get_current_worker_guid());
    return (hc_comm_task_t*)deguidify(worker->currentEDT_guid);
}

u8 hc_mpi_execute_send( u32 paramc, u64 * params, void* paramv[], u32 depc, ocrEdtDep_t depv[]) {
    hc_mpi_args_t * mpi_args = (hc_mpi_args_t *)paramv[0];
    hc_comm_task_t * task = getCurrentCommTask();
    switch(task->status) {
    case OCR_COMM_TASK_STATUS_PRESCRIBED:
        {
            //ocr_mpi_communicator_t * comm = (ocr_mpi_communicator_t*) task->communicator;
            //printf("Send from %d to %d with tag %d\n", comm->rank, mpi_args->rproc, mpi_args->tag);
            MPI_Isend(mpi_args->buf, mpi_args->sz, MPI_BYTE, mpi_args->rproc, mpi_args->tag, MPI_COMM_WORLD, &mpi_args->req);
            task->status = OCR_COMM_TASK_STATUS_ACTIVE;
        }
        break;
    case OCR_COMM_TASK_STATUS_ACTIVE:
        {
            int done;
            MPI_Test(&mpi_args->req, &done, MPI_STATUS_IGNORE);
            if (done) {
                int *t;
                ocrGuid_t db_guid;
                ocrDbCreate(&db_guid, (void **) &t, sizeof(int), 0, NULL, NO_ALLOC);
                ocrEventSatisfy(mpi_args->event, db_guid);
                task->status = OCR_COMM_TASK_STATUS_COMPLETE;
            }
        }
        break;
    default:
        assert(false && "Invalid comm task status");
        break;
    }
    return 0;
}

u8 hc_mpi_execute_recv( u32 paramc, u64 * params, void* paramv[], u32 depc, ocrEdtDep_t depv[]) {
    hc_mpi_args_t * mpi_args = (hc_mpi_args_t *)paramv[0];
    hc_comm_task_t * task = getCurrentCommTask();
    switch(task->status) {
    case OCR_COMM_TASK_STATUS_PRESCRIBED:
        {
            //ocr_mpi_communicator_t * comm = (ocr_mpi_communicator_t*) task->communicator;
            //printf("Recv from %d to %d with tag %d\n", mpi_args->rproc, comm->rank, mpi_args->tag);
            MPI_Irecv(mpi_args->buf, mpi_args->sz, MPI_BYTE, mpi_args->rproc, mpi_args->tag, MPI_COMM_WORLD, &mpi_args->req);
            task->status = OCR_COMM_TASK_STATUS_ACTIVE;
        }
        break;
    case OCR_COMM_TASK_STATUS_ACTIVE:
        {
            int done;
            MPI_Test(&mpi_args->req, &done, MPI_STATUS_IGNORE);
            if (done) {
                int *t;
                ocrGuid_t db_guid;
                ocrDbCreate(&db_guid, (void **) &t, sizeof(int), 0, NULL, NO_ALLOC);
                ocrEventSatisfy(mpi_args->event, db_guid);
                task->status = OCR_COMM_TASK_STATUS_COMPLETE;
            }
        }
        break;
    default:
        assert(false && "Invalid comm task status");
        break;
    }
    return 0;
}

u8 hc_mpi_execute_listener( u32 paramc, u64 * params, void* paramv[], u32 depc, ocrEdtDep_t depv[]) {
    hc_mpi_args_t * mpi_args = (hc_mpi_args_t *)paramv[0];
    hc_comm_task_t * task = getCurrentCommTask();
    //ocr_mpi_communicator_t * comm = (ocr_mpi_communicator_t*) task->communicator;
    switch(task->status) {
    case OCR_COMM_TASK_STATUS_PRESCRIBED:
        {
            //printf("Listener Recv from %d to %d with tag %d\n", mpi_args->rproc, comm->rank, mpi_args->tag);
            MPI_Irecv(mpi_args->buf, mpi_args->sz, MPI_BYTE, mpi_args->rproc, mpi_args->tag, MPI_COMM_WORLD, &mpi_args->req);
            task->status = OCR_COMM_TASK_STATUS_ACTIVE;
        }
        break;
    case OCR_COMM_TASK_STATUS_ACTIVE:
        {
            MPI_Status status;
            int done, count;
            MPI_Test(&mpi_args->req, &done, &status);
            if (done) {
                MPI_Get_count(&status, MPI_BYTE, &count);
                mpi_args->callback((ocr_mpi_communicator_t *)task->communicator, mpi_args->buf, count, status.MPI_SOURCE, status.MPI_TAG);
                //printf("Listener Recv from %d to %d with tag %d\n", mpi_args->rproc, comm->rank, mpi_args->tag);
                MPI_Irecv(mpi_args->buf, mpi_args->sz, MPI_BYTE, mpi_args->rproc, mpi_args->tag, MPI_COMM_WORLD, &mpi_args->req);
            }
        }
        break;
    case OCR_COMM_TASK_STATUS_CANCELED:
        task->status = OCR_COMM_TASK_STATUS_COMPLETE;
        break;
    default:
        assert(false && "Invalid comm task status");
        break;
    }
    return 0;
}

u8 hc_mpi_execute_db_push_on_satisfy( u32 paramc, u64 * params, void* paramv[], u32 depc, ocrEdtDep_t depv[]) {
    hc_mpi_args_t * mpi_args = (hc_mpi_args_t *)paramv[0];
    hc_comm_task_t * task = getCurrentCommTask();
    switch(task->status) {
    case OCR_COMM_TASK_STATUS_PRESCRIBED:
        {
            mpi_args->buf = depv[0].ptr;
            ocrDataBlock_t *dataBlock = (ocrDataBlock_t*)deguidify(depv[0].guid); /* TODO: assert guid type is datablock */
            dataBlock->getSize(dataBlock, (u64*)(&(mpi_args->sz)));
            //ocr_mpi_communicator_t * comm = (ocr_mpi_communicator_t*) task->communicator;
            //printf("DB Push Send from %d to %d with tag %d\n", comm->rank, mpi_args->rproc, mpi_args->tag);
            MPI_Isend(&mpi_args->sz, sizeof(int), MPI_BYTE, mpi_args->rproc, mpi_args->tag, MPI_COMM_WORLD, &mpi_args->req);
            MPI_Isend(mpi_args->buf, mpi_args->sz, MPI_BYTE, mpi_args->rproc, (mpi_args->tag + 1), MPI_COMM_WORLD, &mpi_args->req2);
            task->status = OCR_COMM_TASK_STATUS_ACTIVE;
        }
        break;
    case OCR_COMM_TASK_STATUS_ACTIVE:
        {
            int done;
            MPI_Test(&mpi_args->req2, &done, MPI_STATUS_IGNORE); /* req2 test success implies req is done */
            if (done) task->status = OCR_COMM_TASK_STATUS_COMPLETE;
        }
        break;
    default:
        assert(false && "Invalid comm task status");
        break;
    }
    return 0;
}

u8 hc_mpi_execute_db_pull_then_satisfy( u32 paramc, u64 * params, void* paramv[], u32 depc, ocrEdtDep_t depv[]) {
    hc_mpi_args_t * mpi_args = (hc_mpi_args_t *)paramv[0];
    hc_comm_task_t * task = getCurrentCommTask();
    //ocr_mpi_communicator_t * comm = (ocr_mpi_communicator_t*) task->communicator;
    switch(task->status) {
    case OCR_COMM_TASK_STATUS_PRESCRIBED:
        {
            //printf("DB Pull Send from %d to %d with tag %d\n", comm->rank, mpi_args->rproc, mpi_args->tag);
            MPI_Isend(mpi_args->buf, mpi_args->sz, MPI_BYTE, mpi_args->rproc, mpi_args->tag, MPI_COMM_WORLD, &mpi_args->req);
            task->status = OCR_COMM_TASK_STATUS_ACTIVE;
        }
        break;
    case OCR_COMM_TASK_STATUS_ACTIVE:
        {
            MPI_Status status;
            int done;
            MPI_Test(&mpi_args->req, &done, &status);
            if (done) {
                if (mpi_args->phase == 0) {
                    int * tbuf = (int *)mpi_args->buf;
                    mpi_args->tag = tbuf[1];
                    //printf("DB Pull Recv from %d to %d with tag %d\n", mpi_args->rproc, comm->rank, mpi_args->tag);
                    MPI_Irecv(&mpi_args->sz, sizeof(int), MPI_BYTE, mpi_args->rproc, mpi_args->tag, MPI_COMM_WORLD, &mpi_args->req);
                } else if (mpi_args->phase == 1) {
                    ocrGuid_t db_guid;
                    ocrDbCreate(&db_guid, &mpi_args->buf, mpi_args->sz, 0, NULL, NO_ALLOC);
                    mpi_args->db = db_guid;
                    //printf("DB Pull Recv from %d to %d with tag %d\n", mpi_args->rproc, comm->rank, mpi_args->tag+1);
                    MPI_Irecv(mpi_args->buf, mpi_args->sz, MPI_BYTE, mpi_args->rproc, mpi_args->tag + 1, MPI_COMM_WORLD, &mpi_args->req);
                } else if (mpi_args->phase == 2) {
                    ocrEventSatisfy(mpi_args->event, mpi_args->db);
                    task->status = OCR_COMM_TASK_STATUS_COMPLETE;
                }
                mpi_args->phase++;
            }
        }
        break;
    default:
        assert(false && "Invalid comm task status");
        break;
    }
    return 0;
}

/****************************************************/
/* HC-MPI TASK CONSTRUCTORS                         */
/****************************************************/

ocrGuid_t hc_mpi_create_send(ocr_mpi_communicator_t* communicator, void *buf, size_t size, int rproc, int tag, int depc, ocrGuid_t completionEvt) {
    size_t params = sizeof(hc_mpi_args_t);
    hc_mpi_args_t * mpi_args = (hc_mpi_args_t *) checked_malloc(mpi_args, params);
    mpi_args->buf = buf;
    mpi_args->sz = size;
    mpi_args->rproc = rproc;
    mpi_args->tag = tag;
    mpi_args->callback = NULL;
    mpi_args->params = (u64)params;
    mpi_args->paramv = mpi_args;
    mpi_args->event = completionEvt;
    ocrGuid_t task_guid = communicator->task_factory->create(communicator->task_factory, hc_mpi_execute_send, 1, &(mpi_args->params), (void**)(&(mpi_args->paramv)), depc);
    hc_comm_task_t * task = (hc_comm_task_t *)deguidify(task_guid);
    task->status = OCR_COMM_TASK_STATUS_PRESCRIBED;
    return task_guid;
}

ocrGuid_t hc_mpi_create_recv(ocr_mpi_communicator_t* communicator, void *buf, size_t size, int rproc, int tag, int depc, ocrGuid_t completionEvt) {
    size_t params = sizeof(hc_mpi_args_t);
    hc_mpi_args_t * mpi_args = (hc_mpi_args_t *) checked_malloc(mpi_args, params);
    mpi_args->buf = buf;
    mpi_args->sz = size;
    mpi_args->rproc = rproc;
    mpi_args->tag = tag;
    mpi_args->callback = NULL;
    mpi_args->params = (u64)params;
    mpi_args->paramv = mpi_args;
    mpi_args->event = completionEvt;
    ocrGuid_t task_guid = communicator->task_factory->create(communicator->task_factory, hc_mpi_execute_recv, 1, &(mpi_args->params), (void**)(&(mpi_args->paramv)), depc);
    hc_comm_task_t * task = (hc_comm_task_t *)deguidify(task_guid);
    task->status = OCR_COMM_TASK_STATUS_PRESCRIBED;
    return task_guid;
}

ocrGuid_t hc_mpi_create_any_source_listener(ocr_mpi_communicator_t* communicator, void * buf, size_t size, int tag, int depc, ocr_mpi_listener_callback_fct callback) {
    size_t params = sizeof(hc_mpi_args_t);
    hc_mpi_args_t * mpi_args = (hc_mpi_args_t *) checked_malloc(mpi_args, params);
    mpi_args->buf = buf;
    mpi_args->sz = size;
    mpi_args->rproc = MPI_ANY_SOURCE;
    mpi_args->tag = tag;
    mpi_args->callback = callback;
    mpi_args->params = (u64)params;
    mpi_args->paramv = mpi_args;
    ocrGuid_t task_guid = communicator->task_factory->create(communicator->task_factory, hc_mpi_execute_listener, 1, &(mpi_args->params), (void**)(&(mpi_args->paramv)), depc);
    hc_comm_task_t * task = (hc_comm_task_t *)deguidify(task_guid);
    task->status = OCR_COMM_TASK_STATUS_PRESCRIBED;
    return task_guid;
}

ocrGuid_t hc_mpi_create_db_push_on_satisfy(ocr_mpi_communicator_t* communicator, int rproc, int tag, int depc ) {
    size_t params = sizeof(hc_mpi_args_t);
    hc_mpi_args_t * mpi_args = (hc_mpi_args_t *) checked_malloc(mpi_args, params);
    mpi_args->buf = NULL;
    mpi_args->sz = 0;
    mpi_args->rproc = rproc;
    mpi_args->tag = tag;
    mpi_args->callback = NULL;
    mpi_args->params = (u64)params;
    mpi_args->paramv = mpi_args;
    ocrGuid_t task_guid = communicator->task_factory->create(communicator->task_factory, hc_mpi_execute_db_push_on_satisfy, 1, &(mpi_args->params), (void**)(&(mpi_args->paramv)), depc);
    hc_comm_task_t * task = (hc_comm_task_t *)deguidify(task_guid);
    task->status = OCR_COMM_TASK_STATUS_PRESCRIBED;
    return task_guid;
}

ocrGuid_t hc_mpi_create_db_pull_then_satisfy(ocr_mpi_communicator_t* communicator, void *buf, size_t size, int rproc, int tag, int depc, ocrGuid_t completionEvt) {
    size_t params = sizeof(hc_mpi_args_t);
    hc_mpi_args_t * mpi_args = (hc_mpi_args_t *) checked_malloc(mpi_args, params);
    mpi_args->buf = buf;
    mpi_args->sz = size;
    mpi_args->rproc = rproc;
    mpi_args->tag = tag;
    mpi_args->phase = 0;
    mpi_args->callback = NULL;
    mpi_args->params = (u64)params;
    mpi_args->paramv = mpi_args;
    mpi_args->event = completionEvt;
    ocrGuid_t task_guid = communicator->task_factory->create(communicator->task_factory, hc_mpi_execute_db_pull_then_satisfy, 1, &(mpi_args->params), (void**)(&(mpi_args->paramv)), depc);
    hc_comm_task_t * task = (hc_comm_task_t *)deguidify(task_guid);
    task->status = OCR_COMM_TASK_STATUS_PRESCRIBED;
    return task_guid;
}

/****************************************************/
/* HC-MPI COMMUNICATOR                              */
/****************************************************/

ocr_communicator_t * ocr_hc_mpi_communicator_constructor(ocr_communicator_kind communicatorType) {
    ocr_mpi_communicator_t* derived = (ocr_mpi_communicator_t*) checked_malloc(derived, sizeof(ocr_mpi_communicator_t));
    ocr_communicator_t * base = (ocr_communicator_t *) derived;

    ocr_module_t * module_base = (ocr_module_t *) base;
    module_base->map_fct = NULL;
    base->kind = communicatorType;
    base->configure = hc_mpi_configure;
    base->start = hc_mpi_init;
    base->stop = hc_mpi_finalize;
    base->destruct = hc_mpi_destruct;

    derived->task_factory = hc_comm_task_factory_constructor(base);
    derived->create_send = hc_mpi_create_send;
    derived->create_recv = hc_mpi_create_recv;
    derived->create_any_source_listener = hc_mpi_create_any_source_listener;
    derived->create_db_push_on_satisfy = hc_mpi_create_db_push_on_satisfy;
    derived->create_db_pull_then_satisfy = hc_mpi_create_db_pull_then_satisfy;

    return base;
}

