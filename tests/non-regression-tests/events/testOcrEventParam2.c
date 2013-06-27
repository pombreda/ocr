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

#include "ocr.h"

#define FLAGS 0xdead

/**
 * Test NULL_GUID in depv, which are equivalent to an already satisfied slot
 */

ocrGuid_t task_for_edt ( u32 paramc, u64 * params, void* paramv[], u32 depc, ocrEdtDep_t depv[]) {
    printf("In the task_for_edt");
    // This is the last EDT to execute, terminate
    ocrFinish();
    return NULL_GUID;
}

int main (int argc, char ** argv) {
    ocrEdt_t fctPtrArray [1];
    fctPtrArray[0] = &task_for_edt;
    ocrInit(&argc, argv, 1, fctPtrArray);

    // Current thread is '0' and goes on with user code.
    ocrGuid_t depv[1];
    depv[0] = NULL_GUID;

    // Creates the EDT

    ocrGuid_t edt_guid;

    // 'depv' stores dependencies, so no need to call
    // ocrAddDependence later on to register events.
    ocrEdtCreate(&edt_guid, task_for_edt, 0, NULL, NULL, 0, 0, NULL, NULL_GUID);
    ocrEdtSchedule(edt_guid);

    ocrCleanup();

    return 0;
}
