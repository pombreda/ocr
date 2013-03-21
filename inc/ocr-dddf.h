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

#ifndef __OCR_D3F_H_
#define __OCR_D3F_H_

#include "ocr.h"

/* Initializes the DDDF model. (NOTE: This Has to be called in place of OCR_INIT) */
#define OCR_DDDF_INIT(argc, argv, fnc, funcs) { ocrInit(argc, argv, fnc, funcs); ocrD3FModelInit(); }

/* Adds a task dependence on a distributed DDF referred as (id). */
#define OCR_DDDF_ADD_DEPENDENCE(id, edt, slot) ocrD3FAddDependence(id, DDDF_HOME(id), edt, slot)

/* Satisfy a DDDF referred as (id) */
#define OCR_DDDF_SATISFY(id, guid) ocrD3FSatisfy(id, DDDF_HOME(id), guid)

/* Finishes the DDDF runtime when terminating DDDF (id) is satisfied.
 * There need not be any other ocrCleanup or ocrFinish in the user code. 
 */
#define OCR_DDDF_FINALIZE(id) ocrD3FModelFinalize(id, DDDF_HOME(id))

#define OCR_DDDF_RANK() ocrD3FGetRank()

#define OCR_DDDF_NUM_RANKS() ocrD3FGetNumRanks()


u8 ocrD3FModelInit(void);

u8 ocrD3FAddDependence(int id, int home, ocrGuid_t edt, u32 slot);

u8 ocrD3FSatisfy(int id, int home, ocrGuid_t dataGuid);

u8 ocrD3FModelFinalize(int id, int home);

u32 ocrD3FGetRank();

u32 ocrD3FGetNumRanks();


#endif /* __OCR_D3F_H_ */
