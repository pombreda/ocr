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

#include "ocr-macros.h"
#include "ocr-executor.h"
#include "ocr-low-workers.h"
#include "ocr-scheduler.h"
#include "ocr-policy.h"
#include "ocr-runtime-model.h"
#include "ocr-config.h"
#include "hc.h"

/**!
 * Helper function to build a module mapping
 */
static ocr_module_mapping_t build_ocr_module_mapping(ocr_mapping_kind kind, ocr_module_kind from, ocr_module_kind to) {
    ocr_module_mapping_t res;
    res.kind = kind;
    res.from = from;
    res.to = to;
    return res;
}

/**!
 * Function pointer type to define ocr modules mapping functions
 */
typedef void (*ocr_map_module_fct) (ocr_module_kind from, ocr_module_kind to,
        size_t nb_instances_from,
        ocr_module_t ** instances_from,
        size_t nb_instances_to,
        ocr_module_t ** instances_to);

/**!
 * One-to-many mapping function.
 * Maps a single instance of kind 'from' to 'n' instances of kind 'to'.
 * ex: maps a single scheduler to several workers.
 */
static void map_modules_one_to_many(ocr_module_kind from, ocr_module_kind to,
        size_t nb_instances_from,
        ocr_module_t ** instances_from,
        size_t nb_instances_to,
        ocr_module_t ** instances_to) {
    assert(nb_instances_from == 1);
    size_t i;
    for(i=0; i < nb_instances_to; i++) {
        instances_to[i]->map_fct(instances_to[i], from, 1, (void **) &instances_from[0]);
    }
}

/**!
 * One-to-one mapping function.
 * Maps each instance of kind 'from' to each instance of kind 'to' one to one.
 * ex: maps each executor to each worker in a one-one fashion.
 */
static void map_modules_one_to_one(ocr_module_kind from, ocr_module_kind to,
        size_t nb_instances_from,
        ocr_module_t ** instances_from,
        size_t nb_instances_to,
        ocr_module_t ** instances_to) {
    assert(nb_instances_from == nb_instances_to);
    size_t i;
    for(i=0; i < nb_instances_to; i++) {
        instances_to[i]->map_fct(instances_to[i], from, 1, (void **) &instances_from[i]);
    }
}

/**!
 * Many-to-one mapping function.
 * Maps all instance of kind 'from' to each instance of kind 'to'.
 * ex: maps all workpiles to each individual scheduler available.
 */

static void map_modules_many_to_one(ocr_module_kind from, ocr_module_kind to,
        size_t nb_instances_from,
        ocr_module_t ** instances_from,
        size_t nb_instances_to,
        ocr_module_t ** instances_to) {
    size_t i;
    for(i=0; i < nb_instances_to; i++) {
        instances_to[i]->map_fct(instances_to[i], from, nb_instances_from, (void **) instances_from);
    }
}

/**!
 * Given a mapping kind, returns a function pointer
 * to the implementation of the mapping kind.
 */
static ocr_map_module_fct get_module_mapping_function (ocr_mapping_kind mapping_kind) {
    switch(mapping_kind) {
    case MANY_TO_ONE_MAPPING:
        return map_modules_many_to_one;
    case ONE_TO_ONE_MAPPING:
        return map_modules_one_to_one;
    case ONE_TO_MANY_MAPPING:
        return map_modules_one_to_many;
    default:
        assert(false && "Unknown module mapping function");
        break;
    }
    return NULL;
}


/**!
 * Data-structure that stores ocr modules instances of a policy domain.
 * Used as an internal data-structure when building policy domains so
 * that generic code can be written when there's a need to find a particular
 * module's instance backing array.
 */
typedef struct {
    ocr_module_kind kind;
    size_t nb_instances;
    void ** instances;
} ocr_module_instance;

/**!
 * Utility function that goes over modules_kinds and looks for
 * a particular 'kind' of module. Sets pointers to its
 * number of instances and instances backing array.
 */
static void resolve_module_instances(ocr_module_kind kind, size_t nb_module_kind,
        ocr_module_instance * modules_kinds, size_t * nb_instances, void *** instances) {
    size_t i;
    for(i=0; i < nb_module_kind; i++) {
        if (kind == modules_kinds[i].kind) {
            *nb_instances = modules_kinds[i].nb_instances;
            *instances = modules_kinds[i].instances;
            return;
        }
    }
    assert(false && "Cannot resolve modules instances");
}

/**
 * Default policy has one scheduler and a configurable
 * number of workers, executors and workpiles
 */
ocr_model_policy_t * defaultOcrModelPolicy(size_t nb_schedulers, size_t nb_workers,
        size_t nb_executors, size_t nb_workpiles) {

    // Default policy
    ocr_model_policy_t * defaultPolicy =
            checked_malloc(defaultPolicy, sizeof(ocr_model_policy_t));
    defaultPolicy->model.kind = ocr_policy_default_kind;
    defaultPolicy->model.nb_instances = 1;
    defaultPolicy->model.configuration = NULL;
    defaultPolicy->nb_scheduler_types = 1;
    defaultPolicy->nb_worker_types = 1;
    defaultPolicy->nb_executor_types = 1;
    defaultPolicy->nb_workpile_types = 1;
    defaultPolicy->nb_communicator_types = 0;

    // Default scheduler
    ocr_model_t * defaultScheduler =
            checked_malloc(defaultScheduler, sizeof(ocr_model_t));
    defaultScheduler->kind = ocr_scheduler_default_kind;
    defaultScheduler->nb_instances = nb_schedulers;
    defaultScheduler->configuration = NULL;
    defaultPolicy->schedulers = defaultScheduler;

    // Default worker
    ocr_model_t * defaultWorker =
            checked_malloc(defaultWorker, sizeof(ocr_model_t));
    defaultWorker->kind = ocr_worker_default_kind;
    defaultWorker->nb_instances = nb_workers;
    defaultWorker->configuration = NULL;
    defaultPolicy->workers = defaultWorker;

    // Default executor
    ocr_model_t * defaultExecutor =
            checked_malloc(defaultExecutor, sizeof(ocr_model_t));
    defaultExecutor->kind = ocr_executor_default_kind;
    defaultExecutor->nb_instances = nb_executors;
    defaultExecutor->configuration = NULL;
    defaultPolicy->executors = defaultExecutor;

    // Default workpile
    ocr_model_t * defaultWorkpile =
            checked_malloc(defaultWorkpile, sizeof(ocr_model_t));
    defaultWorkpile->kind = ocr_workpile_default_kind;
    defaultWorkpile->nb_instances = nb_workpiles;
    defaultWorkpile->configuration = NULL;
    defaultPolicy->workpiles = defaultWorkpile;

    // Defines how ocr modules are bound together
    size_t nb_module_mappings = 4;
    ocr_module_mapping_t * defaultMapping =
            checked_malloc(defaultMapping, sizeof(ocr_module_mapping_t) * nb_module_mappings);
    // Note: this doesn't bind modules magically. You need to have a mapping function defined
    //       and set in the targeted implementation (see ocr_scheduler_hc implementation for reference).
    //       These just make sure the mapping functions you have defined are called
    defaultMapping[0] = build_ocr_module_mapping(MANY_TO_ONE_MAPPING, OCR_WORKPILE, OCR_SCHEDULER);
    defaultMapping[1] = build_ocr_module_mapping(ONE_TO_ONE_MAPPING, OCR_WORKER, OCR_EXECUTOR);
    defaultMapping[2] = build_ocr_module_mapping(ONE_TO_MANY_MAPPING, OCR_SCHEDULER, OCR_WORKER);
    defaultMapping[3] = build_ocr_module_mapping(MANY_TO_ONE_MAPPING, OCR_MEMORY, OCR_ALLOCATOR);
    defaultPolicy->nb_mappings = nb_module_mappings;
    defaultPolicy->mappings = defaultMapping;

    // Default memory
    ocr_model_t *defaultMemory =
            checked_malloc(defaultMemory, sizeof(ocr_model_t));
    defaultMemory->configuration = NULL;
    defaultMemory->kind = OCR_LOWMEMORY_DEFAULT;
    defaultMemory->nb_instances = 1;
    defaultPolicy->numMemTypes = 1;
    defaultPolicy->memories = defaultMemory;

    // Default allocator
    ocrAllocatorModel_t *defaultAllocator =
            checked_malloc(defaultAllocator, sizeof(ocrAllocatorModel_t));
    defaultAllocator->model.configuration = NULL;
    defaultAllocator->model.kind = OCR_ALLOCATOR_DEFAULT;
    defaultAllocator->model.nb_instances = 1;
    defaultAllocator->sizeManaged = 64*1024*1024; // TODO: FIXME WITH SOME REAL SIZE!!!!

    defaultPolicy->numAllocTypes = 1;
    defaultPolicy->allocators = defaultAllocator;

    return defaultPolicy;
}

/**
 * Habanero communication runtime policy has two schedulers, 
 * one communication worker, one communication workpile and 
 * a configurable number of computation workers, executors and workpiles
 */
ocr_model_policy_t * hcCommOcrModelPolicy(int * argc, char ** argv, size_t nbHardThreads) {

    // policy
    ocr_model_policy_t * hcCommPolicy = 
             checked_malloc(hcCommPolicy, sizeof(ocr_model_policy_t));
    hcCommPolicy->model.kind = OCR_POLICY_HC_COMM;
    hcCommPolicy->model.nb_instances = 1;
    hcCommPolicy->model.configuration = NULL;
    hcCommPolicy->nb_scheduler_types = 1;
    hcCommPolicy->nb_worker_types = 2;
    hcCommPolicy->nb_executor_types = 1;
    hcCommPolicy->nb_workpile_types = 2;
    hcCommPolicy->nb_communicator_types = 1;

    // scheduler
    ocr_model_t * hcCommScheduler =
            checked_malloc(hcCommScheduler, sizeof(ocr_model_t));
    hcCommScheduler->kind = OCR_SCHEDULER_COMM;
    hcCommScheduler->nb_instances = 1;
    hcCommScheduler->configuration = NULL;
    hcCommPolicy->schedulers = hcCommScheduler;

    // worker
    ocr_model_t * hcCommWorker =
            checked_malloc(hcCommWorker, sizeof(ocr_model_t) * hcCommPolicy->nb_worker_types);
    hcCommWorker[0].kind = OCR_WORKER_HC;
    hcCommWorker[0].nb_instances = nbHardThreads-1; /* Computation workers */
    hcCommWorker[0].configuration = NULL;
    hcCommWorker[1].kind = OCR_WORKER_HC_COMM;
    hcCommWorker[1].nb_instances = 1; /* Communication workers */
    hcCommWorker[1].configuration = NULL;
    hcCommPolicy->workers = hcCommWorker;

    // executor
    ocr_model_t * hcCommExecutor =
            checked_malloc(hcCommExecutor, sizeof(ocr_model_t));
    hcCommExecutor->kind = ocr_executor_default_kind;
    hcCommExecutor->nb_instances = nbHardThreads;
    hcCommExecutor->configuration = NULL;
    hcCommPolicy->executors = hcCommExecutor;

    // workpile
    ocr_model_t * hcCommWorkpile =
            checked_malloc(hcCommWorkpile, sizeof(ocr_model_t) * hcCommPolicy->nb_workpile_types);
    hcCommWorkpile[0].kind = OCR_DEQUE;
    hcCommWorkpile[0].nb_instances = nbHardThreads;
    hcCommWorkpile[0].configuration = NULL;
    hcCommWorkpile[1].kind = OCR_LIST;
    hcCommWorkpile[1].nb_instances = 1;
    hcCommWorkpile[1].configuration = NULL;
    hcCommPolicy->workpiles = hcCommWorkpile;

    // communicator
    ocr_model_t * hcCommCommunicator =
            checked_malloc(hcCommCommunicator, sizeof(ocr_model_t) * hcCommPolicy->nb_communicator_types);
    hcCommCommunicator[0].kind = OCR_HC_MPI;
    hcCommCommunicator[0].nb_instances = 1;
    void ** mpi_args = (void **) checked_malloc(mpi_args, sizeof(void*)*2);
    mpi_args[0] = (void*) argc;
    mpi_args[1] = (void*) argv;
    hcCommCommunicator[0].configuration = (void*)mpi_args;
    hcCommPolicy->communicators = hcCommCommunicator;

    // Defines how ocr modules are bound together
    size_t nb_module_mappings = 4;
    ocr_module_mapping_t * hcCommMapping =
            checked_malloc(hcCommMapping, sizeof(ocr_module_mapping_t) * nb_module_mappings);
    // Note: this doesn't bind modules magically. You need to have a mapping function defined
    //       and set in the targeted implementation (see ocr_scheduler_hc implementation for reference).
    //       These just make sure the mapping functions you have defined are called
    hcCommMapping[0] = build_ocr_module_mapping(MANY_TO_ONE_MAPPING, OCR_WORKPILE, OCR_SCHEDULER);
    hcCommMapping[1] = build_ocr_module_mapping(ONE_TO_ONE_MAPPING, OCR_WORKER, OCR_EXECUTOR);
    hcCommMapping[2] = build_ocr_module_mapping(ONE_TO_MANY_MAPPING, OCR_SCHEDULER, OCR_WORKER);
    hcCommMapping[3] = build_ocr_module_mapping(MANY_TO_ONE_MAPPING, OCR_MEMORY, OCR_ALLOCATOR);
    /*TODO Provide mappings from communicators to scheduler */
    hcCommPolicy->nb_mappings = nb_module_mappings;
    hcCommPolicy->mappings = hcCommMapping;

    // memory
    ocr_model_t *hcCommMemory =
            checked_malloc(hcCommMemory, sizeof(ocr_model_t));
    hcCommMemory->configuration = NULL;
    hcCommMemory->kind = OCR_LOWMEMORY_DEFAULT;
    hcCommMemory->nb_instances = 1;
    hcCommPolicy->numMemTypes = 1;
    hcCommPolicy->memories = hcCommMemory;

    // allocator
    ocrAllocatorModel_t *hcCommAllocator =
            checked_malloc(hcCommAllocator, sizeof(ocrAllocatorModel_t));
    hcCommAllocator->model.configuration = NULL;
    hcCommAllocator->model.kind = OCR_ALLOCATOR_DEFAULT;
    hcCommAllocator->model.nb_instances = 1;
    hcCommAllocator->sizeManaged = 64*1024*1024; // TODO: FIXME WITH SOME REAL SIZE!!!!

    hcCommPolicy->numAllocTypes = 1;
    hcCommPolicy->allocators = hcCommAllocator;

    return hcCommPolicy;
}

void destructOcrModelPolicy(ocr_model_policy_t * model) {
    if (model->schedulers != NULL) {
        free(model->schedulers);
    }
    if (model->workers != NULL) {
        free(model->workers);
    }
    if (model->executors != NULL) {
        free(model->executors);
    }
    if (model->workpiles != NULL) {
        free(model->workpiles);
    }
    if (model->mappings != NULL) {
        free(model->mappings);
    }
    if(model->memories != NULL) {
        free(model->memories);
    }
    if(model->allocators != NULL) {
        free(model->allocators);
    }
    free(model);
}

/**!
 * Given a policy domain model, go over its modules and instantiate them.
 */
ocr_policy_domain_t * instantiateModel(ocr_model_policy_t * model) {

    // Compute total number of workers, executors and workpiles, allocators and memories
    int total_nb_schedulers = 0;
    int total_nb_workers = 0;
    int total_nb_executors = 0;
    int total_nb_workpiles = 0;
    int total_nb_communicators = 0;
    u64 totalNumMemories = 0;
    u64 totalNumAllocators = 0;
    size_t j = 0;
    for(j=0; j < model->nb_scheduler_types; j++) {
        total_nb_schedulers += model->schedulers[j].nb_instances;
    }
    for(j=0; j < model->nb_worker_types; j++) {
        total_nb_workers += model->workers[j].nb_instances;
    }
    for(j=0; j < model->nb_executor_types; j++) {
        total_nb_executors += model->executors[j].nb_instances;
    }
    for(j=0; j < model->nb_workpile_types; j++) {
        total_nb_workpiles += model->workpiles[j].nb_instances;
    }
    for(j=0; j < model->nb_communicator_types; j++) {
        total_nb_communicators += model->communicators[j].nb_instances;
    }
    for(j=0; j < model->numAllocTypes; ++j) {
        totalNumAllocators += model->allocators[j].model.nb_instances;
    }
    for(j=0; j < model->numMemTypes; ++j) {
        totalNumMemories += model->memories[j].nb_instances;
    }

    // Create an instance of the policy domain
    ocr_policy_domain_t * policyDomain = newPolicy(model->model.kind,
            total_nb_workpiles, total_nb_workers,
            total_nb_executors, total_nb_schedulers, total_nb_communicators);

    // Allocate memory for ocr components
    // Components instances are grouped into one big chunk of memory
    ocr_scheduler_t ** all_schedulers = checked_malloc(all_schedulers, sizeof(ocr_scheduler_t *) * total_nb_schedulers);
    ocr_worker_t ** all_workers = checked_malloc(all_workers, sizeof(ocr_worker_t *) * total_nb_workers);
    ocr_executor_t ** all_executors = checked_malloc(all_executors, sizeof(ocr_executor_t *) * total_nb_executors);
    ocr_workpile_t ** all_workpiles = checked_malloc(all_workpiles, sizeof(ocr_workpile_t *) * total_nb_workpiles);
    ocrAllocator_t ** all_allocators = checked_malloc(all_allocators, sizeof(ocrAllocator_t*) * totalNumAllocators);
    ocrLowMemory_t ** all_memories = checked_malloc(all_memories, sizeof(ocrLowMemory_t*) * totalNumMemories);
    ocr_communicator_t ** all_communicators = checked_malloc(all_communicators, sizeof(ocr_communicator_t *) * total_nb_communicators);

    // This is only needed because we want to be able to
    // write generic code to find instances' backing arrays
    // given a module kind.
    size_t nb_ocr_modules = 6;
    ocr_module_instance modules_kinds[nb_ocr_modules];
    modules_kinds[0].kind = OCR_WORKER;
    modules_kinds[0].nb_instances = total_nb_workers;
    modules_kinds[0].instances = (void **) all_workers;

    modules_kinds[1].kind = OCR_EXECUTOR;
    modules_kinds[1].nb_instances = total_nb_executors;
    modules_kinds[1].instances = (void **) all_executors;

    modules_kinds[2].kind = OCR_WORKPILE;
    modules_kinds[2].nb_instances = total_nb_workpiles;
    modules_kinds[2].instances = (void **) all_workpiles;

    modules_kinds[3].kind = OCR_SCHEDULER;
    modules_kinds[3].nb_instances = total_nb_schedulers;
    modules_kinds[3].instances = (void **) all_schedulers;

    modules_kinds[4].kind = OCR_ALLOCATOR;
    modules_kinds[4].nb_instances = totalNumAllocators;
    modules_kinds[4].instances = (void**)all_allocators;

    modules_kinds[5].kind = OCR_MEMORY;
    modules_kinds[5].nb_instances = totalNumMemories;
    modules_kinds[5].instances = (void**)all_memories;

    int nb_workpile_types = model->nb_workpile_types;
    int nb_worker_types = model->nb_worker_types;
    int nb_executor_types = model->nb_executor_types;
    int nb_scheduler_types = model->nb_scheduler_types;
    int nb_communicator_types = model->nb_communicator_types;
    size_t type = 0;
    size_t instance = 0;
    size_t idx = 0;

    //
    // Build instances of each ocr modules
    //

    //TODO would be nice to make creation more generic
    // Create Workpiles
    // Go over each type of ocr modules
    for(type=0, idx=0; type < nb_workpile_types; type++) {
        // For each type, create the number of instances asked for.
        size_t nb_instances = model->workpiles[type].nb_instances;
        for(instance = 0; instance < nb_instances; instance++) {
            // Call the factory method based on the model's type kind.
            all_workpiles[idx] = newWorkpile(model->workpiles[type].kind);
            idx++;
        }
    }

    // Create Schedulers
    for(type=0, idx=0; type < nb_scheduler_types; type++) {
        size_t nb_instances = model->schedulers[type].nb_instances;
        for(instance = 0; instance < nb_instances; instance++) {
            all_schedulers[idx] = newScheduler(model->schedulers[type].kind);
            idx++;
        }
    }

    // Create Workers
    for(type = 0, idx=0; type < nb_worker_types; type++) {
        size_t nb_instances = model->workers[type].nb_instances;
        model->workers[type].configuration = policyDomain;
        for(instance = 0; instance < nb_instances; instance++) {
            all_workers[idx] = newWorker(model->workers[type].kind);
            idx++;
        }
    }

    // Create Executors
    for(type = 0, idx=0; type < nb_executor_types; type++) {
        size_t nb_instances = model->executors[type].nb_instances;
        for(instance = 0; instance < nb_instances; instance++) {
            all_executors[idx] = newExecutor(model->executors[type].kind);
            idx++;
        }
    }
    // Create allocators
    for(type=0, idx=0 ; type < model->numAllocTypes; ++type) {
        u64 nb_instances = model->allocators[type].model.nb_instances;
        for(instance = 0; instance < nb_instances; ++instance) {
            all_allocators[idx] = newAllocator(model->allocators[type].model.kind);
            ++idx;
        }
    }

    // Create memories
    for(type=0, idx=0 ; type < model->numMemTypes; ++type) {
        u64 nb_instances = model->memories[type].nb_instances;
        for(instance = 0; instance < nb_instances; ++instance) {
            all_memories[idx] = newLowMemory(model->memories[type].kind);
            ++idx;
        }
    }

    // Create Communicators
    for(type=0, idx=0; type < nb_communicator_types; type++) {
        size_t nb_instances = model->communicators[type].nb_instances;
        for(instance = 0; instance < nb_instances; instance++) {
            all_communicators[idx] = newCommunicator(model->communicators[type].kind);
            idx++;
        }
    }

    //
    // Configure instances of each ocr components
    //
    for(type=0, idx=0; type < nb_executor_types; type++) {
        size_t nb_instances = model->executors[type].nb_instances;
        for(instance = 0; instance < nb_instances; instance++) {
            all_executors[idx]->create(all_executors[idx], model->executors[type].configuration);
            idx++;
        }
    }
    for(type=0, idx=0; type < nb_worker_types; type++) {
        size_t nb_instances = model->workers[type].nb_instances;
        for(instance = 0; instance < nb_instances; instance++) {
            //TODO handle worker index
            all_workers[idx]->create(all_workers[idx], model->workers[type].configuration, idx);
            idx++;
        }
    }
    for(type=0, idx=0; type < nb_scheduler_types; type++) {
        size_t nb_instances = model->schedulers[type].nb_instances;
        for(instance = 0; instance < nb_instances; instance++) {
            all_schedulers[idx]->create(all_schedulers[idx], model->schedulers[type].configuration);
            idx++;
        }
    }
    for(type=0, idx=0; type < nb_workpile_types; type++) {
        size_t nb_instances = model->workpiles[type].nb_instances;
        for(instance = 0; instance < nb_instances; instance++) {
            all_workpiles[idx]->create(all_workpiles[idx], model->workpiles[type].configuration);
            idx++;
        }
    }

    for(type=0, idx=0; type < model->numAllocTypes; ++type) {
        u64 nb_instances = model->allocators[type].model.nb_instances;
        for(instance = 0; instance < nb_instances; ++instance) {
            all_allocators[idx]->create(all_allocators[idx], model->allocators[type].sizeManaged,
                                        model->allocators[type].model.configuration);
            ++idx;
        }
    }

    for(type=0, idx=0; type < model->numMemTypes; ++type) {
        u64 nb_instances = model->memories[type].nb_instances;
        for(instance = 0; instance < nb_instances; ++instance) {
            all_memories[idx]->create(all_memories[idx],
                                      model->allocators[type].model.configuration);
            ++idx;
        }
    }

    for(type=0, idx=0; type < nb_communicator_types; type++) {
        size_t nb_instances = model->communicators[type].nb_instances;
        for(instance = 0; instance < nb_instances; instance++) {
            all_communicators[idx]->configure(all_communicators[idx], model->communicators[type].configuration);
            idx++;
        }
    }

    policyDomain->create(policyDomain, model, all_schedulers,
                         all_workers, all_executors, all_workpiles,
                         all_allocators, all_memories, all_communicators);

    //
    // Bind instances of each ocr components through mapping functions
    //   - Binding is the last thing we do as we may need information set
    //     during the 'create' phase
    //
    size_t nb_instances_from, nb_instances_to;
    void ** from_instances;
    void ** to_instances;
    for(instance=0; instance < model->nb_mappings; instance++) {
        ocr_module_mapping_t * mapping = (model->mappings + instance);
        // Resolve modules instances we want to map
        resolve_module_instances(mapping->from, nb_ocr_modules, modules_kinds, &nb_instances_from, &from_instances);
        resolve_module_instances(mapping->to, nb_ocr_modules, modules_kinds, &nb_instances_to, &to_instances);
        // Resolve mapping function to use and call it
        ocr_map_module_fct map_fct = get_module_mapping_function(mapping->kind);

        map_fct(mapping->from, mapping->to,
                nb_instances_from, (ocr_module_t **) from_instances,
                nb_instances_to, (ocr_module_t **) to_instances);
    }

    return policyDomain;
}
