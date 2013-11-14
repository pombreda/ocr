/**
 * @brief OCR implementation of statistics gathering
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifdef OCR_ENABLE_STATISTICS

#include "debug.h"
#include "ocr-statistics.h"
#include "ocr-types.h"


#define DEBUG_TYPE STATS
// Forward declaration
extern u8 intProcessMessage(ocrStatsProcess_t *dst);
extern u8 intProcessOutgoingMessage(ocrStatsProcess_t *src, ocrStatsMessage_t* msg);


void ocrStatsAsyncMessage(ocrStatsProcess_t *src, ocrStatsProcess_t *dst,
                          ocrStatsMessage_t *msg) {

    if(!msg) {
        DPRINTF(DEBUG_LVL_VVERB, "Message is NULL, ignoring\n");
        return;
    }
    u64 tickVal = (src->tick += 1);
    DPRINTF(DEBUG_LVL_VERB, "ASYNC Message 0x%lx src:0x%lx dst:0x%lx ts:%ld type:0x%x\n",
            (u64)msg, src->me, dst->me, tickVal, (int)msg->type);
    msg->tick = tickVal;
    msg->state = 0;
    ASSERT(msg->src == src->me);
    ASSERT(msg->dest == dst->me);

    // Notify the sender that a message is being sent
    intProcessOutgoingMessage(src, msg);
    
    // Push the message into the messages queue
    dst->messages->fctPtrs->pushTail(dst->messages, (u64)msg);

    // Now try to get the lock on processing
    if(dst->processing->fctPtrs->trylock(dst->processing)) {
        // We grabbed the lock
        DPRINTF(DEBUG_LVL_VERB, "Message 0x%lx: grabbing processing lock for 0x%lx\n",
                (u64)msg, dst->me);
        u32 count = 5; // TODO: Make configurable
        while(count-- > 0) {
            if(!intProcessMessage(dst))
                break;
        }
        DPRINTF(DEBUG_LVL_VERB, "Finished processing messages for 0x%lx\n", dst->me);
        // Unlock
        dst->processing->fctPtrs->unlock(dst->processing);
    }
}

void ocrStatsSyncMessage(ocrStatsProcess_t *src, ocrStatsProcess_t *dst,
                          ocrStatsMessage_t *msg) {

    if(!msg) {
        DPRINTF(DEBUG_LVL_VVERB, "Message is NULL, ignoring\n");
        return;
    }
    u64 tickVal = (src->tick += 1);
    DPRINTF(DEBUG_LVL_VERB, "SYNC Message 0x%lx src:0x%lx dst:0x%lx ts:%ld type:0x%x\n",
            (u64)msg, src->me, dst->me, tickVal, (int)msg->type);
    msg->tick = tickVal;
    msg->state = 1;
    ASSERT(msg->src == src->me);
    ASSERT(msg->dest == dst->me);

    // Push the message into the messages queues
    dst->messages->fctPtrs->pushTail(dst->messages, (u64)msg);

    // Now try to get the lock on processing
    // Since this is a sync message, we will first send it to ourself
    // and *then* (and this is important) send it to the destination.
    // There are other ways to do this but the ordering ensures that
    // there will be no race condition on msg (could happen if it
    // was in two queues at once). This also ensures that the ticks
    // at the end of it all are properly synced
    while(1) {
        if(dst->processing->fctPtrs->trylock(dst->processing)) {
            // We grabbed the lock
            DPRINTF(DEBUG_LVL_VERB, "Message 0x%lx: grabbing processing lock for 0x%lx\n",
                    (u64)msg, dst->me);
            s32 count = 5; // TODO: Make configurable
            // Process at least count and at least until we get to our message
            // EXTREMELY RARE PROBLEM of count running over for REALLY deep queues
            while(count-- > 0 || msg->state != 2) {
                if(!intProcessMessage(dst) && (msg->state == 2))
                    break;
            }
            DPRINTF(DEBUG_LVL_VERB, "Finished processing messages for 0x%lx\n", dst->me);
            // Unlock
            dst->processing->fctPtrs->unlock(dst->processing);

            break; // Break out of while(1)
        } else {
            // Backoff (TODO, need HW support (abstraction though))
        }
    }

    // At this point, we can sync our own tick
    ASSERT(msg->state == 2);
    // We may have had another message being processed at the same
    // time
    src->tick = msg->tick>src->tick?msg->tick:src->tick;
    DPRINTF(DEBUG_LVL_VVERB, "Message 0x%lx src tick out: %ld\n", (u64)msg, src->tick);
    // Inform the sender of the message
    intProcessOutgoingMessage(src, msg);
    msg->fcts.destruct(msg);
}

#endif /* OCR_ENABLE_STATISTICS */