/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "ocr.h"

/**
 * DESC: Add a once event as a dependence to an EDT twice (on different slots)
 */

#define FLAGS 0xdead

ocrGuid_t taskForEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    int* res = (int*)depv[0].ptr;
    printf("In the taskForEdt with value %d\n", (*res));
    assert(*res == 42);
    // This is the last EDT to execute, terminate
    ocrShutdown();
    return NULL_GUID;
}

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    // Current thread is '0' and goes on with user code.
    ocrGuid_t e0;
    ocrEventCreate(&e0, OCR_EVENT_ONCE_T, true);

    // Creates the EDTa
    ocrGuid_t edtGuid;
    ocrGuid_t taskForEdtTemplateGuid;
    ocrEdtTemplateCreate(&taskForEdtTemplateGuid, taskForEdt, 0 /*paramc*/, 2/*depc*/);
    ocrEdtCreate(&edtGuid, taskForEdtTemplateGuid, EDT_PARAM_DEF, /*paramv=*/NULL, EDT_PARAM_DEF, /*depv=*/NULL,
                    /*properties=*/0, NULL_GUID, /*outEvent=*/NULL);

    // Register a dependence between an event and an edt
    ocrAddDependence(e0, edtGuid, 0, DB_MODE_RO);
    ocrAddDependence(e0, edtGuid, 1, DB_MODE_RO);

    int *k;
    ocrGuid_t dbGuid;
    ocrDbCreate(&dbGuid,(void **) &k,
            sizeof(int), /*flags=*/FLAGS,
            /*location=*/NULL_GUID,
            NO_ALLOC);
    *k = 42;

    // Satisfy event's chain head
    ocrEventSatisfy(e0, dbGuid);

    return NULL_GUID;
}
