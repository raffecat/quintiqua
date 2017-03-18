// global.h: global include.

#ifndef GLOBAL_H
#define GLOBAL_H

#ifdef _DEBUG
#include <assert.h>
#define ASSERT(X) assert(X)
#else
#define ASSERT(X)
#endif

#endif // GLOBAL_H
