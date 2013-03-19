/*
 * This file is part of the HC runtime implementation
 * The Habanero Team (hc-l@rice.edu)
 * Author: Yonghong Yan (yanyh@rice.edu)
 *
 * Architecture-dependent implementation of atomic, barrier and lock functions
 */

#ifndef HC_SYSDEP_H_
#define HC_SYSDEP_H_


/*------------------------
 I386
 ------------------------*/
#ifdef __i386__

static __inline__ void hc_mfence(void) {
	/*
	 * We use an xchg instruction to serialize memory accesses, as can
	 * be done according to the Intel Architecture Software Developer's
	 * Manual, Volume 3: System Programming Guide
	 * (http://www.intel.com/design/pro/manuals/243192.htm), page 7-6,
	 * "For the P6 family processors, locked operations serialize all
	 * outstanding load and store operations (that is, wait for them to
	 * complete)."  The xchg instruction is a locked operation by
	 * default.  Note that the recommended memory barrier is the cpuid
	 * instruction, which is really slow (~70 cycles).  In contrast,
	 * xchg is only about 23 cycles (plus a few per write buffer
	 * entry?).  Still slow, but the best I can find.  -KHR
	 *
	 * Bradley also timed "mfence", and on a Pentium IV xchgl is still quite a bit faster
	 *   mfence appears to take about 125 ns on a 2.5GHZ P4
	 *   xchgl  apears  to take about  90 ns on a 2.5GHZ P4
	 * However on an opteron, the performance of mfence and xchgl are both *MUCH MUCH BETTER*.
	 *   mfence takes 8ns on a 1.5GHZ AMD64 (maybe this is an 801)
	 *   sfence takes 5ns
	 *   lfence takes 3ns
	 *   xchgl  takes 14ns
	 * see mfence-benchmark.c
	 */
	int x = 0, y;
	__asm__ volatile ("xchgl %0,%1" : "=r" (x) : "m" (y), "0" (x) : "memory");
}

#define hc_rmfence() hc_mfence()
#define hc_wmfence() __asm__ __volatile__ ("": : :"memory")

/* atomic set (swap) operation, return the old value of *ptr */
static __inline__ int hc_atomicSet(volatile int *ptr, int x) {
	__asm__("xchgl %0,%1" : "=r" (x) : "m" (*(ptr)), "0" (x) : "memory");
	return x;
}

static __inline__ void hc_atomic_inc(volatile int *ptr) {
	__asm__ __volatile__(
			"lock       ;\n"
			"incl %0;"
			: "=m" (*(ptr))
			  : "m" (*(ptr))
			    : "memory"
	);
}

/* return 1 if the *ptr becomes 0 after decremented, otherwise return 0
 */
static __inline__ int hc_atomic_dec(volatile int *ptr) {
	char rt = (char) 0;
	__asm__ __volatile__(
			"lock;\n"
			"decl %0; sete %1"
			: "=m" (*(ptr)), "=q" (rt)
			  : "m" (*(ptr))
			    : "memory"
	);
	return (short) (rt);

}

/* 
 * if (*ptr == ag) { *ptr = x, return 1 }
 * else return 0;
 */
static __inline__ int hc_cas(volatile int *ptr, int ag, int x) {
	int tmp;
	__asm__ __volatile__("lock;\n"
			"cmpxchg %1,%3"
			: "=a" (tmp) /* %0 EAX, return value */
			  : "r"(x), /* %1 reg, new value */
			    "0" (ag), /* %2 EAX, compare value */
			    "m" (*(ptr)) /* %3 mem, destination operand */
			    : "memory", "cc" /* content changed, memory and cond register */
	);
	return tmp == ag;
}

/*
 * Pointer CAS, this will be different on 64-bit and 32-bit machines
 *
 */
static __inline__ int hc_pointer_cas(void * volatile *ptr, void * ag, void * x) {
	void * tmp;
	__asm__ __volatile__("lock;\n"
			"cmpxchg %1,%3"
			: "=a" (tmp) /* %0 EAX, return value */
			  : "r"(x), /* %1 reg, new value */
			    "0" (ag), /* %2 EAX, compare value */
			    "m" (*(ptr)) /* %3 mem, destination operand */
			    : "memory" /*, "cc" content changed, memory and cond register */
	);
	return tmp == ag;
}

static __inline__ int hc_decr_and_check(volatile int *ptr, int i) {
	unsigned char c;
	__asm__ __volatile__("lock;\n"
			"sub %2,%0; sete %1"
			: "=m" (*(ptr)), "=qm" (c)
			  : "ir" (i), "m" (*(ptr))
			    : "memory");
	return (int)c;
}

#define HC_CACHE_LINE 32

#endif /* __i386__ */

/*------------------------
 amd_64
 ------------------------*/
#ifdef __x86_64


#define HC_CACHE_LINE 64

static __inline__ void hc_mfence() {
	__asm__ __volatile__("mfence":: : "memory");
}
#define hc_rmfence() hc_mfence()
#define hc_wmfence() __asm__ __volatile__ ("": : :"memory")

/* atomic set (swap) operation, return the old value of *ptr */
static __inline__ int hc_atomicSet(volatile int *ptr, int x) {
	__asm__ __volatile__("xchgl %0,%1" : "=r" (x) : "m" (*(ptr)), "0" (x) : "memory");
	return x;
}

static __inline__ int hc_atomic_inc(volatile int *ptr) {
	unsigned char c;
	__asm__ __volatile__(

			"lock       ;\n"
			"incl %0; sete %1"
			: "+m" (*(ptr)), "=qm" (c)
			  : : "memory"
	);
	return c!= 0;
}

/* return 1 if the *ptr becomes 0 after decremented, otherwise return 0
 */
static __inline__ int hc_atomic_dec(volatile int *ptr) {
	unsigned char rt;
	__asm__ __volatile__(
			"lock;\n"
			"decl %0; sete %1"
			: "+m" (*(ptr)), "=qm" (rt)
			  : : "memory"
	);
	return rt != 0;
}

/* 
 * if (*ptr == ag) { *ptr = x, return 1 }
 * else return 0;
 */
static __inline__ int hc_cas(volatile int *ptr, int ag, int x) {
	int tmp;
	__asm__ __volatile__("lock;\n"
			"cmpxchgl %1,%3"
			: "=a" (tmp) /* %0 EAX, return value */
			  : "r"(x), /* %1 reg, new value */
			    "0" (ag), /* %2 EAX, compare value */
			    "m" (*(ptr)) /* %3 mem, destination operand */
			    : "memory" /*, "cc" content changed, memory and cond register */
	);
	return tmp == ag;
}

/*
 * Pointer CAS, this will be different on 64-bit and 32-bit machines
 *
 */
static __inline__ int hc_pointer_cas(void * volatile *ptr, void * ag, void * x) {
	void * tmp;
	__asm__ __volatile__("lock;\n"
			"cmpxchg %1,%3"
			: "=a" (tmp) /* %0 EAX, return value */
			  : "r"(x), /* %1 reg, new value */
			    "0" (ag), /* %2 EAX, compare value */
			    "m" (*(ptr)) /* %3 mem, destination operand */
			    : "memory" /*, "cc" content changed, memory and cond register */
	);
	return tmp == ag;
}

/* atomically substract i from *ptr and test whether the result in *ptr is 0 or not,
 * if 0, return true, otherwise, false
 */
static __inline__ int hc_decr_and_check(volatile int *ptr, int i) {
	unsigned char c;
	__asm__ __volatile__("lock;\n"
			"subl %2,%0; sete %1"
			: "=m" (*(ptr)), "=qm" (c)
			  : "ir" (i), "m" (*(ptr))
			    : "memory");
	return (int)c;
}

static inline int hc_fetch_and_add( volatile int * variable, int value ){
        asm volatile( 
                                "lock; xaddl %%eax, %2;"
                                :"=a" (value)                   //Output
                               : "a" (value), "m" (*variable)  //Input
                               :"memory" );
        return value;
 }

#endif /* __x86_64 */

#ifdef __sparc__
#define HC_CACHE_LINE 64
#include <atomic.h>

static __inline__ void hc_mfence() {
	/* 	__asm__ __volatile__ ("membar #StoreLoad" ::: "memory");
	__sync_synchronize();
	 */
	membar_consumer();
	/* 	membar_producer(); */
}

static __inline__ int hc_decr_and_check(volatile int *ptr, int i) {
	int nv = atomic_add_32_nv(ptr, -i);
	return (nv == 0);
}

static __inline__ int hc_cas(volatile unsigned int *ptr, int ag, int x) {
	int old = atomic_cas_uint(ptr, ag, x);
	return old == ag;
}

static __inline__ void * hc_pointer_cas(void * volatile * ptr, void * ag, void * x) {
	int old = atomic_cas_ptr(ptr, ag, x);
	return old == ag;
}

static __inline__ void hc_atomic_inc(volatile unsigned int *ptr) {
	atomic_inc_uint(ptr);
}
static __inline__ void hc_atomic_dec(volatile unsigned int *ptr) {
	atomic_dec_uint(ptr);
}
#endif /* sparc */

#ifdef __powerpc64__

#define HC_CACHE_LINE 128

static __inline__ void hc_mfence(void) {
        __asm__ __volatile__ ("sync"
	: : : "memory");
}

static __inline__ int hc_atomicSet(volatile int *ptr, int val) {
         int prev;

        __asm__ __volatile__(
	"lwsync \n\
1:      lwarx   %0,0,%2 \n\
        stwcx.  %3,0,%2 \n\
        bne-    1b \n\
	isync"
        : "=&r" (prev), "+m" (*(volatile int *)ptr)
        : "r" (ptr), "r" (val)
        : "cc", "memory");

        return prev;
}

static __inline__ int hc_atomic_inc(volatile int* ptr) {
        int t;

        __asm__ __volatile__(
	"lwsync \n\
1:      lwarx   %0,0,%1         # atomic_inc_return\n\
        addic   %0,%0,1\n\
        stwcx.  %0,0,%1 \n\
        bne-    1b \n\
	isync"
        : "=&r" (t)
        : "r" (ptr)
        : "cc", "xer", "memory");

        return t;
}

static __inline__ int hc_atomic_dec(volatile int* ptr) {
        int t;

        __asm__ __volatile__(
	"lwsync \n\
1:      lwarx   %0,0,%1         # atomic_dec_return\n\
        addic   %0,%0,-1\n\
        stwcx.  %0,0,%1\n\
        bne-    1b \n\
	isync"
        : "=&r" (t)
        : "r" (ptr)
        : "cc", "xer", "memory");

        return (t != 0);
}

static __inline__ int hc_cas(volatile int *ptr, int ag, int x) {
        int prev;

        __asm__ __volatile__ (
	"lwsync \n\
1:      lwarx   %0,0,%2         # __cmpxchg_u32\n\
        cmpw    0,%0,%3\n\
        bne-    2f\n\
        stwcx.  %4,0,%2\n\
        bne-    1b \n\
2:	isync"
        : "=&r" (prev), "+m" (*ptr)
        : "r" (ptr), "r" (ag), "r" (x)
        : "cc", "memory");

        return prev==ag;
}

static __inline__ int hc_pointer_cas(void* volatile *ptr, void *ag, void *x) {
        void* prev;

        __asm__ __volatile__ (
        "lwsync \n\
1:      ldarx   %0,0,%2         # __cmpxchg_u32\n\
        cmpw    0,%0,%3\n\
        bne-    2f\n\
        stdcx.  %4,0,%2\n\
        bne-    1b \n\
2:	isync"
        : "=&r" (prev), "+m" (*ptr)
        : "r" (ptr), "r" (ag), "r" (x)
        : "cc", "memory");

        return prev==ag;
}

static __inline__ int hc_decr_and_check(volatile int *ptr, int i) {
        int t;

        __asm__ __volatile__(
	"lwsync \n\
1:      lwarx   %0,0,%2         # atomic64_sub_return\n\
        subf    %0,%1,%0\n\
        stwcx.  %0,0,%2 \n\
        bne-    1b \n\
	isync"
        : "=&r" (t)
        : "r" (i), "r" (ptr)
        : "cc", "memory");

        return t == 0;
}

#endif /* __powerpc64__ */

#endif /* HC_SYSDEP_H_ */
