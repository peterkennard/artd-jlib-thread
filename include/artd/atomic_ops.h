#ifndef __artd_AtomicOps_h
#define __artd_AtomicOps_h

#include "artd/jlib_base.h"
#include "artd/int_types.h"

#ifdef ARTD_WINDOWS

    extern "C" long __cdecl _InterlockedAnd(long volatile *x, long toAnd);
	#pragma intrinsic(_InterlockedAnd)
	__forceinline int ARTD_ATOMIC_FETCH_AND_AND(volatile int *x, int toAnd)
	{
		return(_InterlockedAnd((long volatile *)x, toAnd));
	}
	extern "C" long __cdecl _InterlockedOr(long volatile *x, long toOr);
	#pragma intrinsic(_InterlockedOr)
	__forceinline int ARTD_ATOMIC_FETCH_AND_OR(volatile int *x, int toOr)
	{
		return(_InterlockedOr((long volatile *)x, toOr));
	}
	extern "C" void* __cdecl _InterlockedExchangePointer(void* volatile* x, void* newValue);
	#pragma intrinsic(_InterlockedExchangePointer)
	__forceinline void* ARTD_ATOMIC_SET_PTR(void* volatile* x, void* newValue)
	{
		return(_InterlockedExchangePointer(x, newValue));
	}
	__forceinline void* ARTD_ATOMIC_GET_PTR(void* volatile* x)
	{
		return(*x); // TODO: this needs to have an implementation.
	}

	extern "C" long __cdecl _InterlockedIncrement(long volatile *x);
	#pragma intrinsic(_InterlockedIncrement)
	__forceinline int ARTD_ATOMIC_INCREMENT(volatile int *x)
	{
		return (int) _InterlockedIncrement((long volatile *)x);
	}
	__forceinline unsigned int ARTD_ATOMIC_INCREMENT(volatile unsigned int *x)
	{
		return (unsigned int) _InterlockedIncrement((volatile long *)x);
	}

	extern "C" int64_t __cdecl _InterlockedIncrement64(int64_t volatile *x);
	#pragma intrinsic(_InterlockedIncrement64)
	__forceinline int64_t ARTD_ATOMIC_INCREMENT64(volatile int64_t *x)
	{
		return _InterlockedIncrement64((int64_t volatile *)x);
	}
	__forceinline uint64_t ARTD_ATOMIC_INCREMENT64(volatile uint64_t *x)
	{
		return _InterlockedIncrement64((volatile int64_t *)x);
	}

	extern "C" long __cdecl _InterlockedDecrement(long volatile *x);
	#pragma intrinsic(_InterlockedDecrement)
	__forceinline int ARTD_ATOMIC_DECREMENT(volatile int *x)
	{
		return (int) _InterlockedDecrement((long volatile *)x);
	}
	__forceinline unsigned int ARTD_ATOMIC_DECREMENT(volatile unsigned int *x)
	{
		return (unsigned int) _InterlockedDecrement((volatile long *)x);
	}

	extern "C" int64_t __cdecl _InterlockedDecrement64(int64_t volatile *x);
	#pragma intrinsic(_InterlockedDecrement64)
	__forceinline int64_t ARTD_ATOMIC_DECREMENT64(volatile int *x)
	{
		return _InterlockedDecrement64((int64_t volatile *)x);
	}
	__forceinline uint64_t ARTD_ATOMIC_DECREMENT64(volatile uint64_t *x)
	{
		return (uint64_t) _InterlockedDecrement64((volatile int64_t *)x);
	}

	/*
	extern "C" void _ReadWriteBarrier(void);
	#pragma intrinsic(_ReadWriteBarrier)
	#define ARTD_COMPILER_MEMORY_BARRIER _ReadWriteBarrier
	*/

#else

	#ifdef __GNUC__
		
        
        __attribute__((always_inline)) inline void * ARTD_ATOMIC_GET_PTR(void **x)
		{
            return(*x);
            /*
            if(sizeof(void *) == sizeof(int32_t)) {
			    return((void *)__sync_fetch_and_or((int *)*x, (int32_t)0));
            }
            else if(sizeof(void *) == sizeof(int64_t)) {
			    return((void *)__sync_fetch_and_or((int64_t *)*x, (int64_t)0));
            }
            */
		}
        __attribute__((always_inline)) inline void ARTD_ATOMIC_SET_PTR(void **x, void *val)
		{
            *x = val;
            /*
            if(sizeof(void *) == sizeof(int32_t)) {
			    __sync_lock_test_and_set ((int32_t *)*x, (int32_t)val);
            }
            else if(sizeof(void *) == sizeof(int64_t)) {
			    __sync_lock_test_and_set ((int64_t *)*x, (int64_t)val);
            }
            */
		}
        __attribute__((always_inline)) inline int ARTD_ATOMIC_FETCH_AND_AND(volatile int *x, int toAnd)
		{
			return(__sync_fetch_and_and (x, toAnd));
		}
        __attribute__((always_inline)) inline int ARTD_ATOMIC_FETCH_AND_OR(volatile int *x, int toOr)
		{
			return(__sync_fetch_and_or (x, toOr));
		}
        __attribute__((always_inline)) inline int ARTD_ATOMIC_INCREMENT(volatile int *x)
		{
			int res = __sync_fetch_and_add (x, 1);
			return(res + 1);		
		}
		__attribute__((always_inline)) inline unsigned int ARTD_ATOMIC_INCREMENT(volatile unsigned int *x)
		{
			unsigned int res = __sync_fetch_and_add (x, 1);
			return (unsigned int) (res + 1);
		}

		__attribute__((always_inline)) inline int ARTD_ATOMIC_DECREMENT(volatile int *x)
		{
			int res = __sync_fetch_and_add (x, -1);
			return (res - 1);
		}
		__attribute__((always_inline)) inline unsigned int ARTD_ATOMIC_DECREMENT(volatile unsigned int *x)
		{
			unsigned int res = __sync_fetch_and_add (x, -1);
			return (unsigned int) (res - 1);
		}

	#else // only for x86 architectures
		__attribute__((always_inline)) inline int ARTD_ATOMIC_INCREMENT(volatile int *x)
		{
			int res = 1;
			__asm__ ("lock xaddl %0,%1" : "+r" (res), "+m" (*x));
			return res + 1;
		}
		__attribute__((always_inline)) inline unsigned int ARTD_ATOMIC_INCREMENT(volatile unsigned int *x)
		{
			int res = 1;
			__asm__ ("lock xaddl %0,%1" : "+r" (res), "+m" (*x));
			return (unsigned int) (res + 1);
		}

		__attribute__((always_inline)) inline int ARTD_ATOMIC_DECREMENT(volatile int *x)
		{
			int res = -1;
			__asm__ ("lock xaddl %0,%1" : "+r" (res), "+m" (*x));
			return (res - 1);
		}
		__attribute__((always_inline)) inline unsigned int ARTD_ATOMIC_DECREMENT(volatile unsigned int *x)
		{
			int res = -1;
			__asm__ ("lock xaddl %0,%1" : "+r" (res), "+m" (*x));
			return (unsigned int) (res - 1);
		}

		__attribute__((always_inline)) inline void ARTD_COMPILER_MEMORY_BARRIER()
		{
			__asm__ __volatile__ ("" : : : "memory");
		}
	#endif
#endif

#endif // __artd_AtomicOps_h
