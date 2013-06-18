/**
 * @brief Simple GUID implementation on distributed systems using 
 * distributed node indexing.
 *
 * @authors Sanjay Chatterjee, Rice University
 * @date 2013-06-15
 * Copyright (c) 2013, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of Intel Corporation nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "dnx.h"
#include "stdlib.h"
#include "stdio.h"
#include "ocr-runtime-model.h"

void dnxCreate(ocrGuidProvider_t* self, void* config) {
	size_t i;
	ocrGuidProviderDnx_t * gp = (ocrGuidProviderDnx_t *)self;
	ocr_model_policy_t * model = (ocr_model_policy_t *)config;
	u32 nWorkers = model->total_nb_workers;
	u64 node_mask = DNX_NODE(model->node_rank);
	gp->workerData = malloc(nWorkers * sizeof(ocrDnxGuidData_t*));
	for (i = 0; i < nWorkers; i++) {
		ocrDnxGuidData_t * gData = malloc(sizeof(ocrDnxGuidData_t));
		gData->metadata = malloc(DNX_NUM_BUCKET * sizeof(u64*));
		gData->metadata[0] = malloc(DNX_NUM_INDEX * sizeof(u64));
		gData->bucket = 0;
		gData->index = (i == 0) ? 1 : 0;
		gData->mask = node_mask | DNX_WORKER(i); // bucket is 0
		gp->workerData[i] = gData;
	}
    return;
}

void dnxDestruct(ocrGuidProvider_t* self) {
    free(self);
    return;
}

void dnxGetWorkerGuid(ocrGuidProvider_t* self, ocrGuid_t* guid, u64 val, ocrGuidKind kind, ocrDnxGuidData_t * gData) {
	if (gData->index >= DNX_NUM_INDEX) {
		gData->bucket++;
		if (gData->bucket >= DNX_NUM_BUCKET)
			ocr_abort(); //TODO: utilize other available worker slots
		gData->metadata[gData->bucket] = malloc(DNX_NUM_INDEX * sizeof(u64));
		gData->index = 0;
		gData->mask &= ~DNX_BUCKET_MASK;
		gData->mask |= DNX_BUCKET(gData->bucket);
	}

	u64 guidInst = gData->mask | DNX_INDEX(gData->index) | DNX_TYPE(kind);
	gData->metadata[gData->bucket][gData->index] = val;
	gData->index++;
    *guid = (u64) guidInst;
	//printf("New GUID DNX %x\n", *guid);
}

u8 dnxGetVal(ocrGuidProvider_t* self, ocrGuid_t guid, u64* val, ocrGuidKind* kind) {
    u64 guidInst = (u64) guid;
	ocrGuidProviderDnx_t * gp = (ocrGuidProviderDnx_t *)self;
	ocrDnxGuidData_t * gData = gp->workerData[DNX_GET_WORKER(guidInst)];
    *val = (u64) gData->metadata[DNX_GET_BUCKET(guidInst)][DNX_GET_INDEX(guidInst)];
    if(kind)
        *kind = DNX_GET_TYPE(guidInst);
    return 0;
}

u8 dnxGetGuid(ocrGuidProvider_t* self, ocrGuid_t* guid, u64 val, ocrGuidKind kind) {
	//Get the current worker id
	ocr_worker_t * worker = ocr_get_current_worker();
	int workerId = get_worker_id(worker);
	ocrGuidProviderDnx_t * gp = (ocrGuidProviderDnx_t *)self;
	ocrDnxGuidData_t * gData = gp->workerData[workerId];
	dnxGetWorkerGuid(self, guid, val, kind, gData);
    return 0;
}

u8 dnxGetRuntimeGuid(ocrGuidProvider_t* self, ocrGuid_t* guid, u64 val, ocrGuidKind kind) {
	// Runtime setup is assumed to be done by worker 0
	ocrGuidProviderDnx_t * gp = (ocrGuidProviderDnx_t *)self;
	dnxGetWorkerGuid(self, guid, val, kind, gp->workerData[0]);
    return 0;
}

u8 dnxGetKind(ocrGuidProvider_t* self, ocrGuid_t guid, ocrGuidKind* kind) {
    u64 guidInst = (u64) guid;
    *kind = DNX_GET_TYPE(guidInst);
    return 0;
}

u8 dnxReleaseGuid(ocrGuidProvider_t *self, ocrGuid_t guid) {
	//TODO
    return 0;
}

void dnxStart(ocrGuidProvider_t* self) {
    self->getGuid = &dnxGetGuid;
    return;
}

ocrGuidProvider_t* newGuidProviderDnx() {
    ocrGuidProviderDnx_t *result = (ocrGuidProviderDnx_t*)malloc(sizeof(ocrGuidProviderDnx_t));
	result->workerData = NULL;
    result->base.create = &dnxCreate;
    result->base.destruct = &dnxDestruct;
    result->base.start = &dnxStart;
    result->base.getGuid = &dnxGetRuntimeGuid;
    result->base.getVal = &dnxGetVal;
    result->base.getKind = &dnxGetKind;
    result->base.releaseGuid = &dnxReleaseGuid;

    return (ocrGuidProvider_t*)result;
}
