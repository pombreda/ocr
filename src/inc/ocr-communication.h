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

#ifndef OCR_COMMUNICATION_H_
#define OCR_COMMUNICATION_H_

#include "ocr-task-event.h"
#include "ocr-runtime-def.h"

/****************************************************/
/* OCR COMMUNICATOR KINDS                           */
/****************************************************/

typedef enum ocr_communicator_kind_enum {
    OCR_HC_MPI = 1,
} ocr_communicator_kind;

/****************************************************/
/* OCR COMMUNICATOR API                             */
/****************************************************/

//Forward declaration
struct ocr_communicator_struct;

typedef void (*communicator_configure_fct) ( struct ocr_communicator_struct* communicator, void * configuration );
typedef void (*communicator_start_fct) ( struct ocr_communicator_struct* communicator);
typedef void (*communicator_stop_fct) ( struct ocr_communicator_struct* communicator);
typedef void (*communicator_destruct_fct)(struct ocr_communicator_struct* communicator);

/*! \brief Abstract class to represent OCR communication modules
 *
 *  This class provides the interface for the underlying implementation to conform.
 */
typedef struct ocr_communicator_struct {
    ocr_module_t module;

    /* Type of communicator */
    ocr_communicator_kind kind;

    /*! \brief Configures a concrete implementation of a communicator */
    communicator_configure_fct configure;
    /*! \brief Starts a configured implementation of a communicator */
    communicator_start_fct start;
    /*! \brief Stops a communicator */
    communicator_stop_fct stop;
    /*! \brief Virtual destructor for the communicator interface */
    communicator_destruct_fct destruct;
} ocr_communicator_t;

/*
 * Runtime API to get a communicator of specified type
 */
ocr_communicator_t * ocrGetCommunicator(ocr_communicator_kind communicatorType);

/****************************************************/
/* OCR COMMUNICATOR CONSTRUCTORS                    */
/****************************************************/

ocr_communicator_t * newCommunicator(ocr_communicator_kind communicatorType);
ocr_communicator_t * ocr_hc_mpi_communicator_constructor(ocr_communicator_kind communicatorType);

/****************************************************/
/* OCR COMMUNICATION TASKS                          */
/****************************************************/

typedef enum ocr_comm_task_status_enum {
    OCR_COMM_TASK_STATUS_UNKNOWN,
    OCR_COMM_TASK_STATUS_PRESCRIBED,
    OCR_COMM_TASK_STATUS_ACTIVE,
    OCR_COMM_TASK_STATUS_CANCELED,
    OCR_COMM_TASK_STATUS_COMPLETE,
} ocr_comm_task_status;

#endif /* OCR_COMMUNICATION_H_ */

