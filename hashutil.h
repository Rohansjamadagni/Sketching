/*
 * ============================================================================
 *
 *       Filename:  hashutil.h
 *
 *         Author:  Prashant Pandey (), pandey@cs.utah.edu
 *   Organization:  University of Utah
 *
 * ============================================================================
 */

#ifndef _HASHUTIL_H_
#define _HASHUTIL_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint64_t MurmurHash64A ( const void * key, int len, unsigned int seed );

#ifdef __cplusplus
}
#endif

#endif  // #ifndef _HASHUTIL_H_
