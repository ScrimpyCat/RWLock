/*
 *  Copyright (c) 2013, Stefan Johnson
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without modification,
 *  are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice, this list
 *     of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this
 *     list of conditions and the following disclaimer in the documentation and/or other
 *     materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 RWLock rules:
 If lock is marked as read, will block write lock attempts. And allows for
 recursive read locks across multiple threads.
 
 If lock is marked as write, will block read/write lock attempts on other threads.
 And allow for recursive read and write locks on the current thread.
 
 Note: Is susceptible to writer-starvation.
 */

#ifndef RWLock_RWLock_h
#define RWLock_RWLock_h

#include <stdint.h>


#if defined(__unix__) || defined(__posix__) || defined(__APPLE__)
#define RW_LOCK_USES_POSIX_LOCKS 1
#include <pthread.h>
#else
#warning Currently no support for locks.
#endif


typedef struct {
#if RW_LOCK_USES_POSIX_LOCKS
    uint64_t lockCount;
    pthread_t writerThread;
    pthread_mutex_t lock;
    
    pthread_mutex_t monitorLock;
#endif
    
    _Bool destroyed;
} rwlock_t;

typedef int rwlock_error_t;

#define RW_LOCK_SUCCESS 0
#define RW_LOCK_ERROR_INVALID 1 //invalid arguments/input
#define RW_LOCK_ERROR_NO_MEMORY 2 //not enough memory
#define RW_LOCK_ERROR_PERMISSION 3 //invalid permission
#define RW_LOCK_ERROR_BUSY 4 //thread is currently busy
#define RW_LOCK_ERROR_UNKNOWN -1

#define RW_LOCK_UNLOCKED 0
#define RW_LOCK_READ 1
#define RW_LOCK_WRITE 2

rwlock_error_t RWLockInit(rwlock_t *);
rwlock_error_t RWLockDestroy(rwlock_t *);

rwlock_error_t RWLockWrite(rwlock_t *);
rwlock_error_t RWUnlockWrite(rwlock_t *);
rwlock_error_t RWLockRead(rwlock_t *);
rwlock_error_t RWUnlockRead(rwlock_t *);

rwlock_error_t RWTryLockWrite(rwlock_t *);
rwlock_error_t RWTryLockRead(rwlock_t *);

rwlock_error_t RWLockType(rwlock_t *, int *);

#endif
