/**
 * @brief Simple implementation of a malloc wrapper
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#include "debug.h"
#include "external/rangeTracker.h"
#include "mem-platform/malloc/malloc-mem-platform.h"
#include "ocr-mem-platform.h"
#include "ocr-sysboot.h"

#include <stdlib.h>

// Poor man's basic lock
#define INIT_LOCK(addr) do {*addr = 0;} while(0);
#define LOCK(addr) do {while(!__sync_bool_compare_and_swap(addr, 0, 1)) sched_yield(); } while(0);
#define UNLOCK(addr) do {*addr = 0;} while(0);

/******************************************************/
/* OCR MEM PLATFORM MALLOC IMPLEMENTATION             */
/******************************************************/

void mallocDestruct(ocrMemPlatform_t *self) {
    destroyRange(&(((ocrMemPlatformMalloc_t*)self)->rangeTracker));
    runtimeChunkFree((u64)self, NULL);
}

struct _ocrPolicyDomain_t;

static void mallocStart(ocrMemPlatform_t *self, struct _ocrPolicyDomain_t * PD ) {
    self->startAddr = (u64)malloc(self->size);
    ASSERT(self->startAddr);
    self->endAddr = self->startAddr + self->size;

    ocrMemPlatformMalloc_t *rself = (ocrMemPlatformMalloc_t*)self;
    initializeRange(&(rself->rangeTracker), 16, self->startAddr,
                    self->endAddr, USER_FREE);

}

static void mallocStop(ocrMemPlatform_t *self) {
    ocrMemPlatformMalloc_t *rself = (ocrMemPlatformMalloc_t*)self;
    destroyRange(&(rself->rangeTracker));
    free((void*)self->startAddr);
}

static u8 mallocGetThrottle(ocrMemPlatform_t *self, u64 *value) {
    return 1; // Not supported
}

static u8 mallocSetThrottle(ocrMemPlatform_t *self, u64 value) {
    return 1; // Not supported
}

static void mallocGetRange(ocrMemPlatform_t *self, u64* startAddr,
                           u64 *endAddr) {
    if(startAddr) *startAddr = self->startAddr;
    if(endAddr) *endAddr = self->endAddr;
}

static u8 mallocChunkAndTag(ocrMemPlatform_t *self, u64 *startAddr, u64 size,
                            ocrMemoryTag_t oldTag, ocrMemoryTag_t newTag) {

    if(oldTag >= MAX_TAG || newTag >= MAX_TAG)
        return 3;
    
    ocrMemPlatformMalloc_t *rself = (ocrMemPlatformMalloc_t *)self;
    
    u64 iterate = 0;
    u64 startRange, endRange;
    u8 result;
    LOCK(&(rself->lock));
    do {
        result = getRegionWithTag(&(rself->rangeTracker), oldTag, &startRange,
                         &endRange, &iterate);
        if(endRange - startRange >= size) {
            // This is a fit, we do not look for "best" fit for now
            *startAddr = startRange;
            RESULT_ASSERT(splitRange(&(rself->rangeTracker),
                                     startRange, size, newTag), ==, 0);
            break;
        }
    } while(result == 0);

    UNLOCK(&(rself->lock));
    return result;
}

static u8 mallocTag(ocrMemPlatform_t *self, u64 startAddr, u64 endAddr,
                    ocrMemoryTag_t newTag) {

    if(newTag >= MAX_TAG)
        return 3;
    
    ocrMemPlatformMalloc_t *rself = (ocrMemPlatformMalloc_t *)self;

    LOCK(&(rself->lock));
    RESULT_ASSERT(splitRange(&(rself->rangeTracker), startAddr,
                             endAddr - startAddr, newTag), ==, 0);
    UNLOCK(&(rself->lock));
    return 0;
}

static u8 mallocQueryTag(ocrMemPlatform_t *self, u64 *start, u64* end,
                         ocrMemoryTag_t *resultTag, u64 addr) {
    ocrMemPlatformMalloc_t *rself = (ocrMemPlatformMalloc_t *)self;

    RESULT_ASSERT(getTag(&(rself->rangeTracker), addr, start, end, resultTag),
                  ==, 0);
    return 0;
}

ocrMemPlatform_t* newMemPlatformMalloc(ocrMemPlatformFactory_t * factory,
                                       u64 memSize, ocrParamList_t *perInstance) {

    // TODO: This will be replaced by the runtime/GUID meta-data allocator
    // For now, we cheat and use good-old malloc which is kind of counter productive with
    // all the trouble we are going through to *not* use malloc...
    ocrMemPlatform_t *result = (ocrMemPlatform_t*)
        runtimeChunkAlloc(sizeof(ocrMemPlatformMalloc_t), NULL);
        
    result->fctPtrs = &(factory->platformFcts);
    result->size = memSize;
    result->startAddr = result->endAddr = 0ULL;
    ocrMemPlatformMalloc_t *rself = (ocrMemPlatformMalloc_t*)result;
    INIT_LOCK(&(rself->lock));
    return result;
}

/******************************************************/
/* OCR MEM PLATFORM MALLOC FACTORY                    */
/******************************************************/

static void destructMemPlatformFactoryMalloc(ocrMemPlatformFactory_t *factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrMemPlatformFactory_t *newMemPlatformFactoryMalloc(ocrParamList_t *perType) {
    ocrMemPlatformFactory_t *base = (ocrMemPlatformFactory_t*)
        runtimeChunkAlloc(sizeof(ocrMemPlatformFactoryMalloc_t), NULL);
    
    base->instantiate = &newMemPlatformMalloc;
    base->destruct = &destructMemPlatformFactoryMalloc;
    base->platformFcts.destruct = &mallocDestruct;
    base->platformFcts.start = &mallocStart;
    base->platformFcts.stop = &mallocStop;
    base->platformFcts.getThrottle = &mallocGetThrottle;
    base->platformFcts.setThrottle = &mallocSetThrottle;
    base->platformFcts.getRange = &mallocGetRange;
    base->platformFcts.chunkAndTag = &mallocChunkAndTag;
    base->platformFcts.tag = &mallocTag;
    base->platformFcts.queryTag = &mallocQueryTag;
    
    return base;
}
