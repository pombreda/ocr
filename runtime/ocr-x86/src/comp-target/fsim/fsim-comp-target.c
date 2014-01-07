/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "comp-target/fsim/fsim-comp-target.h"
#include "debug.h"
#include "ocr-comp-platform.h"
#include "ocr-comp-target.h"
#include "ocr-macros.h"
#include "ocr-policy-domain.h"

static void fsimTargetDestruct(ocrCompTarget_t *compTarget) {
    int i = 0;
    while(i < compTarget->platformCount) {
      compTarget->platforms[i]->fctPtrs->destruct(compTarget->platforms[i]);
      i++;
    }
    free(compTarget->platforms);
    free(compTarget);
}

static void fsimTargetStart(ocrCompTarget_t * compTarget, ocrPolicyDomain_t * PD, launchArg_t * launchArg) {
    ASSERT(compTarget->platformCount == 1);
    compTarget->platforms[0]->fctPtrs->start(compTarget->platforms[0], PD, launchArg);
}

static void fsimTargetStop(ocrCompTarget_t * compTarget) {
    ASSERT(compTarget->platformCount == 1);
    compTarget->platforms[0]->fctPtrs->stop(compTarget->platforms[0]);
}

ocrCompTarget_t * newCompTargetFSIM(ocrCompTargetFactory_t * factory, ocrParamList_t* perInstance) {
    ocrCompTargetFSIM_t * compTarget = checkedMalloc(compTarget, sizeof(ocrCompTargetFSIM_t));

    // TODO: Setup GUID
    compTarget->base.platforms = NULL;
    compTarget->base.platformCount = 0;
    compTarget->base.fctPtrs = &(factory->targetFcts);

    // TODO: Setup routine and routineArg. Should be in perInstance misc

    return (ocrCompTarget_t*)compTarget;
}

/******************************************************/
/* OCR COMP TARGET FSIM FACTORY                         */
/******************************************************/
static void destructCompTargetFactoryFSIM(ocrCompTargetFactory_t *factory) {
    free(factory);
}

ocrCompTargetFactory_t *newCompTargetFactoryFSIM(ocrParamList_t *perType) {
    ocrCompTargetFactory_t *base = (ocrCompTargetFactory_t*)
        checkedMalloc(base, sizeof(ocrCompTargetFactoryFSIM_t));
    base->instantiate = &newCompTargetFSIM;
    base->destruct = &destructCompTargetFactoryFSIM;
    base->targetFcts.destruct = &fsimTargetDestruct;
    base->targetFcts.start = &fsimTargetStart;
    base->targetFcts.stop = &fsimTargetStop;

    return base;
}
