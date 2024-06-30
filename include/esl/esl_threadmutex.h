/*
 * Cross Platform Thread/Mutex abstraction
 * Copyright(C) 2007 Michael Jerris
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so.
 *
 * This work is provided under this license on an "as is" basis, without warranty of any kind,
 * either expressed or implied, including, without limitation, warranties that the covered code
 * is free of defects, merchantable, fit for a particular purpose or non-infringing. The entire
 * risk as to the quality and performance of the covered code is with you. Should any covered
 * code prove defective in any respect, you (not the initial developer or any other contributor)
 * assume the cost of any necessary servicing, repair or correction. This disclaimer of warranty
 * constitutes an essential part of this license. No use of any covered code is authorized hereunder
 * except under this disclaimer.
 *
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

typedef struct esl_mutex esl_mutex_t;
typedef struct esl_thread esl_thread_t;
typedef void* (*esl_thread_function_t)(esl_thread_t*, void*);

#include "esl/esl.h"

ESL_DECLARE(esl_status_t) esl_thread_create_detached(esl_thread_function_t func, void* data);
esl_status_t esl_thread_create_detached_ex(esl_thread_function_t func, void* data, size_t stack_size);
void esl_thread_override_default_stacksize(size_t size);
ESL_DECLARE(esl_status_t) esl_mutex_create(esl_mutex_t** mutex);
ESL_DECLARE(esl_status_t) esl_mutex_destroy(esl_mutex_t** mutex);
ESL_DECLARE(esl_status_t) esl_mutex_lock(esl_mutex_t* mutex);
ESL_DECLARE(esl_status_t) esl_mutex_trylock(esl_mutex_t* mutex);
ESL_DECLARE(esl_status_t) esl_mutex_unlock(esl_mutex_t* mutex);

#ifdef __cplusplus
}
#endif /* defined(__cplusplus) */
