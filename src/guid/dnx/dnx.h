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

#ifndef __OCR_GUIDPROVIDER_DNX_H__
#define __OCR_GUIDPROVIDER_DNX_H__

#include "ocr-types.h"
#include "ocr-guid.h"

#define DNX_GUID_BITS   64

#define DNX_NODE_BITS   24 //Most significant
#define DNX_WORKER_BITS 12
#define DNX_BUCKET_BITS 16
#define DNX_INDEX_BITS  8
#define DNX_TYPE_BITS   4  //Least significant

#define DNX_NUM_NODE   (1U << DNX_NODE_BITS)
#define DNX_NUM_WORKER (1U << DNX_WORKER_BITS)
#define DNX_NUM_BUCKET (1U << DNX_BUCKET_BITS)
#define DNX_NUM_INDEX  (1U << DNX_INDEX_BITS)
#define DNX_NUM_TYPE   (1U << DNX_TYPE_BITS)

#define DNX_NODE_BIT_MASK   (DNX_NUM_NODE   - 1)
#define DNX_WORKER_BIT_MASK (DNX_NUM_WORKER - 1)
#define DNX_BUCKET_BIT_MASK (DNX_NUM_BUCKET - 1)
#define DNX_INDEX_BIT_MASK  (DNX_NUM_INDEX  - 1)
#define DNX_TYPE_BIT_MASK   (DNX_NUM_TYPE   - 1)

#define DNX_NODE_BIT_SHIFTS   (DNX_TYPE_BITS + DNX_INDEX_BITS + DNX_BUCKET_BITS + DNX_WORKER_BITS)
#define DNX_WORKER_BIT_SHIFTS (DNX_TYPE_BITS + DNX_INDEX_BITS + DNX_BUCKET_BITS)
#define DNX_BUCKET_BIT_SHIFTS (DNX_TYPE_BITS + DNX_INDEX_BITS)
#define DNX_INDEX_BIT_SHIFTS  (DNX_TYPE_BITS)
#define DNX_TYPE_BIT_SHIFTS   0

#define DNX_NODE_MASK   (DNX_NODE_BIT_MASK << DNX_NODE_BIT_SHIFTS)
#define DNX_WORKER_MASK (DNX_WORKER_BIT_MASK << DNX_WORKER_BIT_SHIFTS)
#define DNX_BUCKET_MASK (DNX_BUCKET_BIT_MASK << DNX_BUCKET_BIT_SHIFTS)
#define DNX_INDEX_MASK  (DNX_INDEX_BIT_MASK << DNX_INDEX_BIT_SHIFTS)
#define DNX_TYPE_MASK   (DNX_TYPE_BIT_MASK << DNX_TYPE_BIT_SHIFTS)

#define DNX_NODE(x)   ((x & DNX_NODE_BIT_MASK)   << DNX_NODE_BIT_SHIFTS)
#define DNX_WORKER(x) ((x & DNX_WORKER_BIT_MASK) << DNX_WORKER_BIT_SHIFTS)
#define DNX_BUCKET(x) ((x & DNX_BUCKET_BIT_MASK) << DNX_BUCKET_BIT_SHIFTS)
#define DNX_INDEX(x)  ((x & DNX_INDEX_BIT_MASK)  << DNX_INDEX_BIT_SHIFTS)
#define DNX_TYPE(x)   ((x & DNX_TYPE_BIT_MASK)   << DNX_TYPE_BIT_SHIFTS)

#define DNX_GET_NODE(guid)   ((guid & DNX_NODE_MASK)   >> DNX_NODE_BIT_SHIFTS)
#define DNX_GET_WORKER(guid) ((guid & DNX_WORKER_MASK) >> DNX_WORKER_BIT_SHIFTS)
#define DNX_GET_BUCKET(guid) ((guid & DNX_BUCKET_MASK) >> DNX_BUCKET_BIT_SHIFTS)
#define DNX_GET_INDEX(guid)  ((guid & DNX_INDEX_MASK)  >> DNX_INDEX_BIT_SHIFTS)
#define DNX_GET_TYPE(guid)   ((guid & DNX_TYPE_MASK)   >> DNX_TYPE_BIT_SHIFTS)

/**
 * Worker specific DNX guid data
 */
typedef struct ocrDnxGuidData {
	u64 ** metadata;
	u32 bucket;
	u32 index;
	u64 mask;
} ocrDnxGuidData_t;

/**
 * DNX guid provider
 */
typedef struct _ocrGuidProviderDnx_t {
    ocrGuidProvider_t base;
	ocrDnxGuidData_t ** workerData;
} ocrGuidProviderDnx_t;

ocrGuidProvider_t* newGuidProviderDnx();

#endif /* __OCR_GUIDPROVIDER_DNX_H__ */
