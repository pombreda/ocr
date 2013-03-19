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

#ifndef HC_H_
#define HC_H_

#include <stdlib.h>
#include <assert.h>
#include <pthread.h>

#include "ocr-runtime.h"
#include "deque.h"
#include "list.h"
#include "hc_edf.h"

/******************************************************/
/* OCR-HC-COMM POLICY DOMAIN                          */
/******************************************************/

typedef struct hc_comm_policy_domain_struct {
    ocr_policy_domain_t base;
    volatile int isCommunicationReady;
} hc_comm_policy_domain_t;

/******************************************************/
/* OCR-HC WORKER                                      */
/******************************************************/

typedef struct {
    ocr_worker_t worker;
    //TODO this is a convenience to map workers to workpiles
    int id;
    //TODO shall these stay here or go up ?
    bool run;
    // This is cached here to avoid pounding the guid manager
    ocrGuid_t guid;
    // reference to the policy domain instance this worker is in
    ocrGuid_t policy_domain_guid;
    // reference to the EDT this worker is currently executing
    ocrGuid_t currentEDT_guid;
} hc_worker_t;

ocr_worker_t* hc_worker_constructor(ocr_worker_kind workerType);
ocr_worker_t* hc_comm_worker_constructor(ocr_worker_kind workerType);

/******************************************************/
/* OCR-HC WorkPool                                    */
/******************************************************/

typedef struct hc_workpile {
    ocr_workpile_t base;
    deque_t * deque;
} hc_workpile;

ocr_workpile_t * hc_workpile_constructor(ocr_workpile_kind workpileType);

/******************************************************/
/* OCR-HC-COMM List WorkPool                          */
/******************************************************/

typedef ocrGuid_t (*workpile_peek_fct) ( struct ocr_workpile_struct* base );
typedef ocrGuid_t (*workpile_next_fct) ( struct ocr_workpile_struct* base );

typedef struct hc_comm_workpile_struct {
    ocr_workpile_t base;
    list_t * list;
    /*! \brief Interface to peek at next element without advancing
     *  \return GUID of the task from this task pool
     */
    workpile_peek_fct peek;
    /*! \brief Interface to get next element by advancing
     *  \return GUID of the task from this task pool
     */
    workpile_next_fct next;
} hc_comm_workpile_t;

ocr_workpile_t * hc_comm_workpile_constructor(ocr_workpile_kind workpileType);


/******************************************************/
/* OCR-HC SCHEDULER                                   */
/******************************************************/

typedef struct {
    ocr_scheduler_t scheduler;
    size_t n_pools;
    ocr_workpile_t ** pools;
    // Note: cache steal iterators in hc's scheduler
    // Each worker has its own steal iterator instantiated
    // a sheduler's construction time.
    workpile_iterator_t ** steal_iterators;

} hc_scheduler_t;

ocr_scheduler_t * hc_scheduler_constructor(ocr_scheduler_kind schedulerType);

/******************************************************/
/* OCR-HC-COMM SCHEDULER                              */
/******************************************************/

typedef ocr_workpile_t * (*scheduler_comm_push_mapping_fct) (struct ocr_scheduler_struct*, struct ocr_worker_struct*);
typedef ocr_workpile_t * (*scheduler_comm_pop_mapping_fct) (struct ocr_scheduler_struct*, struct ocr_worker_struct*);

typedef struct {
    hc_scheduler_t hc_base;
    scheduler_comm_push_mapping_fct comm_push_mapping;
    scheduler_comm_pop_mapping_fct comm_pop_mapping;
    ocr_workpile_t * comm_pool; /* TODO: Assumes one communication worker. Need to extend it */
} hc_comm_scheduler_t;

ocr_scheduler_t * hc_comm_scheduler_constructor(ocr_scheduler_kind schedulerType);

/**
 * The computation worker routine that asks work to the scheduler
 */
extern void * worker_computation_routine(void * arg);

/**
 * The communication worker routine that asks work to the scheduler
 */
extern void * worker_communication_routine(void * arg);

#endif /* HC_H_ */
