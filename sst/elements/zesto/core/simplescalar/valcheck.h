#ifndef VALCHECK_H
#define VALCHECK_H

/* macros for checking arguments */

/* true is x is a power of two */
#define IS_POW2(x) (!(((x)-1)&(x)))

/* is power of two? */
#define CHECK_POW2(x) if(!IS_POW2(x)) fatal("%s(%s) %s must be a power of two.",name,COMPONENT_NAME,#x)
/* is positive power of two? */
#define CHECK_PPOW2(x) if(!IS_POW2(x) || ((x) < 1)) fatal("%s(%s) %s must be a positive power of two.",name,COMPONENT_NAME,#x)
/* is not negative? (i.e. >= 0) */
#define CHECK_NNEG(x) if(((x) < 0)) fatal("%s(%s) %s must be non-negative.",name,COMPONENT_NAME,#x)
/* is positive? (i.e. > 0) */
#define CHECK_POS(x) if(((x) <= 0)) fatal("%s(%s) %s must be positive.",name,COMPONENT_NAME,#x)
/* is not negative and <= y? */
#define CHECK_NNEG_LEQ(x,y) if(((x) < 0) || ((x)>(y))) fatal("%s(%s) %s must be non-negative and less than or equal to %lf.",name,COMPONENT_NAME,#x,(double)(y))
/* is positive and <= y? */
#define CHECK_POS_LEQ(x,y) if(((x) <= 0) || ((x)>(y))) fatal("%s(%s) %s must be positive and less than or equal to %lf.",name,COMPONENT_NAME,#x,(double)(y))
/* is not negative and < y? */
#define CHECK_NNEG_LT(x,y) if(((x) < 0) || ((x)>=(y))) fatal("%s(%s) %s must be non-negative and less than or equal to %lf.",name,COMPONENT_NAME,#x,(double)(y))
/* is positive and < y? */
#define CHECK_POS_LT(x,y) if(((x) <= 0) || ((x)>=(y))) fatal("%s(%s) %s must be positive and less than or equal to %lf.",name,COMPONENT_NAME,#x,(double)(y))
/* is positive and > y? */
#define CHECK_POS_GT(x,y) if(((x) <= 0) || ((x)<=(y))) fatal("%s(%s) %s must be positive and greater than %lf.",name,COMPONENT_NAME,#x,(double)(y))
/* is boolean? */
#define CHECK_BOOL(x) if(((x) != 0) && ((x) != 1)) fatal("%s(%s) %s must be boolean (0 or 1).",name,COMPONENT_NAME,#x)


#endif
