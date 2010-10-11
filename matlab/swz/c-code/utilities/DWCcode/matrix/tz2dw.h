
#ifndef __TZ2DW__
#define __TZ2DW__

#include "swzmatrix.h"

#include "modify_for_mex.h"

/*  // flags and defines   ansi-c*/
#define NEARINFINITY 1.0E+300
#define M_UNDEF  0         /*  0 or NULL: No attribute will be given when memory is allocated but no values are initialized.   ansi-c*/
#define M_GE     0x0001    /*  1:    A general matrix.   ansi-c*/
#define M_SU     0x0002    /*  2:    A symmetric (must be square) matrix but only the upper triangular part is referenced.   ansi-c*/
#define M_SL     0x0004    /*  4:    A symmetric (must be square) matrix but only the lower triangular part is referenced.   ansi-c*/
#define M_UT     0x0008    /*  8:    A upper triangular (trapezoidal if nrows < ncols) matrix but only the upper triangular part is referenced.   ansi-c*/
#define M_LT     0x0010    /*  16:   A lower triangular (trapezoidal if nrows > ncols) matrix but only the lower triangular part is referenced.   ansi-c*/
#define M_CN     0x0020    /*  32:   A constant (CN) matrix (All elements are the same or no (N) change from one to another).   ansi-c*/
#define V_UNDEF 0                    /*  Zero or NULL: No values have been assigned to the double vector.   ansi-c*/
#define V_DEF   1                    /*  True: Values have been assigned to the double vector.   ansi-c*/
#define square(x)  ((x)*(x))

/*  // matrix and vector structures   ansi-c*/
typedef struct
{
  double *M;
  int nrows, ncols;
  int flag;           /*  flag: Refers to M_GE, M_SU, M_SL, M_UT, and M_LT in tzmatlab.h.   ansi-c*/
} TSdmatrix;
typedef struct
{
  double *v;
  int n;
  int flag;           /*  flag: no legal values are assigned if 0 and legal values are assigned if 1.   ansi-c*/
} TSdvector;



/*  // memory management   ansi-c*/
#define tzMalloc(elt_count,type)  (type *)swzMalloc((elt_count)*sizeof(type))
#define tzDestroy(x)   {if (x) { swzFree((x)); (x) = NULL; }}

/*  // i/o   ansi-c*/
#define tzFclose(x)  {if (x) { fclose(x); (x)=(FILE *)NULL;}}

#endif