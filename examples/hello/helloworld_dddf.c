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
#include <assert.h>

#include "ocr-dddf.h"

#define FLAGS 0xdead

int DDDF_HOME(int id) { return id; }

u8 task_for_edt ( u32 paramc, u64 * params, void* paramv[], u32 depc, ocrEdtDep_t depv[]) {
    int* res = (int*)depv[0].ptr;
    int* res2 = (int*)depv[1].ptr;
    printf("In the task_for_edt with value %d and %d\n", (*res), (*res2));

    // This is the last EDT to execute, terminate
    int *k;
    ocrGuid_t db_guid;
    ocrDbCreate(&db_guid, (void **) &k, sizeof(int), FLAGS,
                NULL, NO_ALLOC);
    *k = *res;
    //ocrFinish();
    OCR_DDDF_SATISFY(0, db_guid);
    return 0;
}

int main (int argc, char ** argv) {
    ocrEdt_t fctPtrArray [1];
    fctPtrArray[0] = &task_for_edt;
    OCR_DDDF_INIT(&argc, argv, 1, fctPtrArray);

    int rank = OCR_DDDF_RANK();
    printf("Hello from rank %d\n", rank);

    if (rank == 0) {
        // Current thread is '0' and goes on with user code.
        ocrGuid_t event_guid;
        ocrEventCreate(&event_guid, OCR_EVENT_STICKY_T, true);
    
        // Creates the EDT
        ocrGuid_t edt_guid;
    
        ocrEdtCreate(&edt_guid, task_for_edt, /*paramc=*/0, /*params=*/ NULL,
                     /*paramv=*/NULL, /*properties=*/0,
                     /*depc=*/2, /*depv=*/NULL);
    
        OCR_DDDF_ADD_DEPENDENCE(1, edt_guid, 0);
        // Register a dependence between an event and an edt
        ocrAddDependence(event_guid, edt_guid, 1);
        // Schedule the EDT (will run when dependences satisfied)
        ocrEdtSchedule(edt_guid);
    
        int *k;
        ocrGuid_t db_guid;
        ocrDbCreate(&db_guid, (void **) &k, sizeof(int), /*flags=*/FLAGS,
                    /*location=*/NULL, NO_ALLOC);
        *k = 42;
    
        ocrEventSatisfy(event_guid, db_guid);

    } else if (rank == 1) {
        int *k;
        ocrGuid_t db_guid;
        ocrDbCreate(&db_guid, (void **) &k, sizeof(int), /*flags=*/FLAGS,
                    /*location=*/NULL, NO_ALLOC);
        *k = 52;
        OCR_DDDF_SATISFY(1, db_guid);
    } else {
        assert(false && "Runs with 2 procs");
    }

    //ocrCleanup();
    OCR_DDDF_FINALIZE(0);

    return 0;
}
