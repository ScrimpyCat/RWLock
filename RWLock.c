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


#include "rwlock.h"
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>

#ifndef TRUE
#define TRUE true
#define FALSE false
#endif


#if RW_LOCK_USES_POSIX_LOCKS
static rwlock_error_t MapError(const int Err)
{
    switch (Err)
    {
        case 0: return RW_LOCK_SUCCESS;
        case ENOMEM: return RW_LOCK_ERROR_NO_MEMORY;
        case EPERM: return RW_LOCK_ERROR_PERMISSION;
        case EBUSY: return RW_LOCK_ERROR_BUSY;
    }
    
    return RW_LOCK_ERROR_UNKNOWN;
}
#endif

rwlock_error_t RWLockInit(rwlock_t *RWLock)
{
    if (!RWLock) return RW_LOCK_ERROR_INVALID;
    
    RWLock->destroyed = FALSE;
#if RW_LOCK_USES_POSIX_LOCKS
    int err;
    if ((err = pthread_mutex_init(&RWLock->lock, NULL))) return MapError(err);
    if ((err = pthread_mutex_init(&RWLock->monitorLock, NULL)))
    {
        pthread_mutex_destroy(&RWLock->lock);
        
        return MapError(err);
    }
    
    RWLock->lockCount = 0;
    RWLock->writerThread = NULL;
#endif
    
    return RW_LOCK_SUCCESS; //lies when no locks are supported
}

rwlock_error_t RWLockDestroy(rwlock_t *RWLock)
{
    if (!RWLock) return RW_LOCK_ERROR_INVALID;
    
#if RW_LOCK_USES_POSIX_LOCKS
    //don't care about most fails
    int err;
    if ((err = pthread_mutex_lock(&RWLock->monitorLock)))
    {
        if (RWLock->destroyed) return RW_LOCK_ERROR_INVALID;
        return MapError(err);
    }
    
    RWLock->destroyed = TRUE;
    
    //wait and let any locks get finished up
    pthread_mutex_unlock(&RWLock->monitorLock);
    pthread_mutex_lock(&RWLock->lock);
    pthread_mutex_lock(&RWLock->monitorLock);
    pthread_mutex_unlock(&RWLock->lock);
    
    pthread_mutex_destroy(&RWLock->lock);
    
    pthread_mutex_unlock(&RWLock->monitorLock);
    pthread_mutex_destroy(&RWLock->monitorLock);
#endif
    
    return RW_LOCK_SUCCESS; //lies when no locks are supported
}

rwlock_error_t RWLockWrite(rwlock_t *RWLock)
{
    if (!RWLock) return RW_LOCK_ERROR_INVALID;
    
#if RW_LOCK_USES_POSIX_LOCKS
    int err;
    if ((err = pthread_mutex_lock(&RWLock->monitorLock)))
    {
        if (RWLock->destroyed) return RW_LOCK_ERROR_INVALID;
        return MapError(err);
    }
    
    if (!RWLock->destroyed)
    {
        pthread_t Self = pthread_self();
        if (RWLock->writerThread != Self)
        {
            pthread_mutex_unlock(&RWLock->monitorLock);
            if ((err = pthread_mutex_lock(&RWLock->lock)))
            {
                if (RWLock->destroyed) return RW_LOCK_ERROR_INVALID;
                return MapError(err);
            }
            
            if ((err = pthread_mutex_lock(&RWLock->monitorLock)))
            {
                pthread_mutex_unlock(&RWLock->lock);
                if (RWLock->destroyed) return RW_LOCK_ERROR_INVALID;
                return MapError(err);
            }
            
            RWLock->writerThread = Self;
        }
        
        RWLock->lockCount++;
    }
    
    pthread_mutex_unlock(&RWLock->monitorLock);
#endif
    
    return RW_LOCK_SUCCESS; //lies when no locks are supported
}

rwlock_error_t RWUnlockWrite(rwlock_t *RWLock)
{
    if (!RWLock) return RW_LOCK_ERROR_INVALID;
    
#if RW_LOCK_USES_POSIX_LOCKS
    int err;
    if ((err = pthread_mutex_lock(&RWLock->monitorLock)))
    {
        if (RWLock->destroyed) return RW_LOCK_ERROR_INVALID;
        return MapError(err);
    }
    
    rwlock_error_t Error = RW_LOCK_SUCCESS;
    //if (!RWLock->destroyed)
    {
        if (RWLock->writerThread == pthread_self())
        {
            if (!--RWLock->lockCount)
            {
                RWLock->writerThread = NULL;
                pthread_mutex_unlock(&RWLock->lock);
            }
        }
        
        else Error = RW_LOCK_ERROR_INVALID;
    }
    
    //else Error = RW_LOCK_ERROR_INVALID;
    
    pthread_mutex_unlock(&RWLock->monitorLock);
    return Error;
#endif
    
    return RW_LOCK_SUCCESS; //lies when no locks are supported
}

rwlock_error_t RWLockRead(rwlock_t *RWLock)
{
    if (!RWLock) return RW_LOCK_ERROR_INVALID;
    
#if RW_LOCK_USES_POSIX_LOCKS
    int err;
    if ((err = pthread_mutex_lock(&RWLock->monitorLock)))
    {
        if (RWLock->destroyed) return RW_LOCK_ERROR_INVALID;
        return MapError(err);
    }
    
    rwlock_error_t Error = RW_LOCK_SUCCESS;
    if (!RWLock->destroyed)
    {
        if ((!RWLock->lockCount) || ((RWLock->writerThread) && (RWLock->writerThread != pthread_self())))
        {
            pthread_mutex_unlock(&RWLock->monitorLock);
            if ((err = pthread_mutex_lock(&RWLock->lock)))
            {
                if (RWLock->destroyed) return RW_LOCK_ERROR_INVALID;
                return MapError(err);
            }
            
            if ((err = pthread_mutex_lock(&RWLock->monitorLock)))
            {
                pthread_mutex_unlock(&RWLock->lock);
                if (RWLock->destroyed) return RW_LOCK_ERROR_INVALID;
                return MapError(err);
            }
        }
        
        RWLock->lockCount++;
    }
    
    else Error = RW_LOCK_ERROR_INVALID;
    
    pthread_mutex_unlock(&RWLock->monitorLock);
    return Error;
#endif
    
    return RW_LOCK_SUCCESS; //lies when no locks are supported
}

rwlock_error_t RWUnlockRead(rwlock_t *RWLock)
{
    if (!RWLock) return RW_LOCK_ERROR_INVALID;
    
#if RW_LOCK_USES_POSIX_LOCKS
    int err;
    if ((err = pthread_mutex_lock(&RWLock->monitorLock)))
    {
        if (RWLock->destroyed) return RW_LOCK_ERROR_INVALID;
        return MapError(err);
    }
    
    rwlock_error_t Error = RW_LOCK_SUCCESS;
    //if (!RWLock->destroyed)
    {
        if ((!RWLock->writerThread) || (RWLock->writerThread == pthread_self()))
        {
            if (!--RWLock->lockCount)
            {
                RWLock->writerThread = NULL;
                pthread_mutex_unlock(&RWLock->lock);
            }
        }
        
        else Error = RW_LOCK_ERROR_INVALID;
    }
    
    //else Error = RW_LOCK_ERROR_INVALID;
    
    pthread_mutex_unlock(&RWLock->monitorLock);
    return Error;
#endif
    
    return RW_LOCK_SUCCESS; //lies when no locks are supported
}

rwlock_error_t RWTryLockWrite(rwlock_t *RWLock)
{
    if (!RWLock) return RW_LOCK_ERROR_INVALID;
    
#if RW_LOCK_USES_POSIX_LOCKS
    int err;
    if ((err = pthread_mutex_trylock(&RWLock->monitorLock)))
    {
        if (RWLock->destroyed) return RW_LOCK_ERROR_INVALID;
        return MapError(err);
    }
    
    if (!RWLock->destroyed)
    {
        pthread_t Self = pthread_self();
        if (RWLock->writerThread != Self)
        {
            if ((err = pthread_mutex_trylock(&RWLock->lock)))
            {
                pthread_mutex_unlock(&RWLock->monitorLock);
                if (RWLock->destroyed) return RW_LOCK_ERROR_INVALID;
                return MapError(err);
            }
            
            RWLock->writerThread = Self;
        }
        
        RWLock->lockCount++;
    }
    
    pthread_mutex_unlock(&RWLock->monitorLock);
#endif
    
    return RW_LOCK_SUCCESS; //lies when no locks are supported
}

rwlock_error_t RWTryLockRead(rwlock_t *RWLock)
{
    if (!RWLock) return RW_LOCK_ERROR_INVALID;
    
#if RW_LOCK_USES_POSIX_LOCKS
    int err;
    if ((err = pthread_mutex_trylock(&RWLock->monitorLock)))
    {
        if (RWLock->destroyed) return RW_LOCK_ERROR_INVALID;
        return MapError(err);
    }
    
    rwlock_error_t Error = RW_LOCK_SUCCESS;
    if (!RWLock->destroyed)
    {
        if ((!RWLock->lockCount) || ((RWLock->writerThread) && (RWLock->writerThread != pthread_self())))
        {
            if ((err = pthread_mutex_trylock(&RWLock->lock)))
            {
                pthread_mutex_unlock(&RWLock->monitorLock);
                if (RWLock->destroyed) return RW_LOCK_ERROR_INVALID;
                return MapError(err);
            }
        }
        
        RWLock->lockCount++;
    }
    
    else Error = RW_LOCK_ERROR_INVALID;
    
    pthread_mutex_unlock(&RWLock->monitorLock);
    return Error;
#endif
    
    return RW_LOCK_SUCCESS; //lies when no locks are supported
}

rwlock_error_t RWLockType(rwlock_t *RWLock, int *Type)
{
    if ((!Type) || (!RWLock)) return RW_LOCK_ERROR_INVALID;
    
#if RW_LOCK_USES_POSIX_LOCKS
    int err;
    if ((err = pthread_mutex_lock(&RWLock->monitorLock)))
    {
        if (RWLock->destroyed) return RW_LOCK_ERROR_INVALID;
        return MapError(err);
    }
    
    if (RWLock->writerThread) *Type = RW_LOCK_WRITE;
    else if (RWLock->lockCount) *Type = RW_LOCK_READ;
    else *Type = RW_LOCK_UNLOCKED;
    
    pthread_mutex_unlock(&RWLock->monitorLock);
#else
    *Type = RW_LOCK_UNLOCKED;
#endif
    
    return RW_LOCK_SUCCESS;
}
