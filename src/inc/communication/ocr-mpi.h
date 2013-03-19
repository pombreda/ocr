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

#ifndef OCR_MPI_H_
#define OCR_MPI_H_

#include "ocr-communication.h"

/****************************************************/
/* OCR MPI COMMUNICATOR API                         */
/****************************************************/

struct ocr_mpi_communicator_struct;

typedef void (*ocr_mpi_listener_callback_fct) (struct ocr_mpi_communicator_struct * comm, void *buf, size_t sz, int rproc, int tag);

/* MPI task interfaces */
typedef ocrGuid_t (*ocr_mpi_send_fct) (struct ocr_mpi_communicator_struct *communicator, void *buf, size_t size, int rproc, int tag, int depc, ocrGuid_t completionEvt);
typedef ocrGuid_t (*ocr_mpi_recv_fct) (struct ocr_mpi_communicator_struct *communicator, void *buf, size_t size, int rproc, int tag, int depc, ocrGuid_t completionEvt);
typedef ocrGuid_t (*ocr_mpi_any_source_listener_fct) (struct ocr_mpi_communicator_struct *communicator, void *buf, size_t size, int tag, int depc, ocr_mpi_listener_callback_fct callback);
typedef ocrGuid_t (*ocr_mpi_db_push_on_satisfy_fct) (struct ocr_mpi_communicator_struct *communicator, int rproc, int tag, int depc);
typedef ocrGuid_t (*ocr_mpi_db_pull_then_satisfy_fct) (struct ocr_mpi_communicator_struct *communicator, void *buf, size_t size, int rproc, int tag, int depc, ocrGuid_t completionEvt);


typedef struct ocr_mpi_communicator_struct {
    ocr_communicator_t base;

    int rank, size;/* MPI rank and size */

    /* MPI Init arguments */
    int *argc;
    char **argv;

    /* MPI task factory */
    ocr_task_factory *task_factory;

    /* MPI communication task create functions */
    ocr_mpi_send_fct create_send;
    ocr_mpi_send_fct create_recv;
    ocr_mpi_any_source_listener_fct create_any_source_listener;
    ocr_mpi_db_push_on_satisfy_fct create_db_push_on_satisfy;
    ocr_mpi_db_pull_then_satisfy_fct create_db_pull_then_satisfy;
} ocr_mpi_communicator_t;

#endif /* OCR_MPI_H_ */

