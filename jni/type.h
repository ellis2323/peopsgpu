#ifndef TYPE_H
#define TYPE_H

// Return values (should be applied to the entire code).
#define FPSE_OK         0
#define FPSE_ERR        -1
#define FPSE_WARN       1

#ifndef EXCLUDE_FPSE_TYPES

typedef signed char        INT8;
typedef signed short int   INT16;
typedef signed long int    INT32;

typedef unsigned char      UINT8;
typedef unsigned short int UINT16;
typedef unsigned long int  UINT32;

#ifdef __GNUC__
#define UINT64  unsigned long long int
#define INT64   signed long long int
#else
#define UINT64  unsigned __int64
#define INT64   signed __int64
#endif

#endif // EXCLUDE_FPSE_TYPES

#if defined INT64 || defined UINT64

#if defined __GNUC__
#define FPSE_K64(_val)  _val##LL
#elif defined _MSC_VER
#define FPSE_K64(_val)  _val##i64
#else
#define FPSE_K64(_val)  _val
#endif

#endif

#endif
