/* Copyright (C) 1997-2006, 2007, 2009, 2010, 2011, 2012 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Andreas Jaeger <aj@suse.de>, 1997.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

/* Part of testsuite for libm.

   This file is processed by a perl script.  The resulting file has to
   be included by a master file that defines:

   Macros:
   FUNC(function): converts general function name (like cos) to
   name with correct suffix (e.g. cosl or cosf)
   MATHCONST(x):   like FUNC but for constants (e.g convert 0.0 to 0.0L)
   FLOAT:	   floating point type to test
   - TEST_MSG:	   informal message to be displayed
   CHOOSE(Clongdouble,Cdouble,Cfloat,Cinlinelongdouble,Cinlinedouble,Cinlinefloat):
   chooses one of the parameters as delta for testing
   equality
   PRINTF_EXPR	   Floating point conversion specification to print a variable
   of type FLOAT with printf.  PRINTF_EXPR just contains
   the specifier, not the percent and width arguments,
   e.g. "f".
   PRINTF_XEXPR	   Like PRINTF_EXPR, but print in hexadecimal format.
   PRINTF_NEXPR Like PRINTF_EXPR, but print nice.  */

/* This testsuite has currently tests for:
   acos, acosh, asin, asinh, atan, atan2, atanh,
   cbrt, ceil, copysign, cos, cosh, erf, erfc, exp, exp10, exp2, expm1,
   fabs, fdim, finite, floor, fma, fmax, fmin, fmod, fpclassify,
   frexp, gamma, hypot,
   ilogb, isfinite, isinf, isnan, isnormal,
   isless, islessequal, isgreater, isgreaterequal, islessgreater, isunordered,
   j0, j1, jn,
   ldexp, lgamma, log, log10, log1p, log2, logb,
   modf, nearbyint, nextafter, nexttoward,
   pow, remainder, remquo, rint, lrint, llrint,
   round, lround, llround,
   scalb, scalbn, scalbln, signbit, sin, sincos, sinh, sqrt, tan, tanh, tgamma, trunc,
   y0, y1, yn, significand

   and for the following complex math functions:
   cabs, cacos, cacosh, carg, casin, casinh, catan, catanh,
   ccos, ccosh, cexp, cimag, clog, clog10, conj, cpow, cproj, creal,
   csin, csinh, csqrt, ctan, ctanh.

   At the moment the following functions and macros aren't tested:
   drem (alias for remainder),
   lgamma_r,
   nan,
   pow10 (alias for exp10).

   Parameter handling is primitive in the moment:
   --verbose=[0..3] for different levels of output:
   0: only error count
   1: basic report on failed tests (default)
   2: full report on all tests
   -v for full output (equals --verbose=3)
   -u for generation of an ULPs file
 */

/* "Philosophy":

   This suite tests some aspects of the correct implementation of
   mathematical functions in libm.  Some simple, specific parameters
   are tested for correctness but there's no exhaustive
   testing.  Handling of specific inputs (e.g. infinity, not-a-number)
   is also tested.  Correct handling of exceptions is checked
   against.  These implemented tests should check all cases that are
   specified in ISO C99.

   Exception testing: At the moment only divide-by-zero, invalid,
   overflow and underflow exceptions are tested.  Inexact exceptions
   aren't checked at the moment.

   NaN values: There exist signalling and quiet NaNs.  This implementation
   only uses quiet NaN as parameter but does not differentiate
   between the two kinds of NaNs as result.  Where the sign of a NaN is
   significant, this is not tested.

   Inline functions: Inlining functions should give an improvement in
   speed - but not in precission.  The inlined functions return
   reasonable values for a reasonable range of input values.  The
   result is not necessarily correct for all values and exceptions are
   not correctly raised in all cases.  Problematic input and return
   values are infinity, not-a-number and minus zero.  This suite
   therefore does not check these specific inputs and the exception
   handling for inlined mathematical functions - just the "reasonable"
   values are checked.

   Beware: The tests might fail for any of the following reasons:
   - Tests are wrong
   - Functions are wrong
   - Floating Point Unit not working properly
   - Compiler has errors

   With e.g. gcc 2.7.2.2 the test for cexp fails because of a compiler error.


   To Do: All parameter should be numbers that can be represented as
   exact floating point values.  Currently some values cannot be
   represented exactly and therefore the result is not the expected
   result.  For this we will use 36 digits so that numbers can be
   represented exactly.  */

#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#include "libm-test-ulps.h"
#include <complex.h>
#include <math.h>
#include <float.h>
#include <fenv.h>
#include <limits.h>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <argp.h>
#include <tininess.h>

/* Allow platforms without all rounding modes to test properly,
   assuming they provide an __FE_UNDEFINED in <bits/fenv.h> which
   causes fesetround() to return failure.  */
#ifndef FE_TONEAREST
# define FE_TONEAREST	__FE_UNDEFINED
#endif
#ifndef FE_TOWARDZERO
# define FE_TOWARDZERO	__FE_UNDEFINED
#endif
#ifndef FE_UPWARD
# define FE_UPWARD	__FE_UNDEFINED
#endif
#ifndef FE_DOWNWARD
# define FE_DOWNWARD	__FE_UNDEFINED
#endif

/* Possible exceptions */
#define NO_EXCEPTION			0x0
#define INVALID_EXCEPTION		0x1
#define DIVIDE_BY_ZERO_EXCEPTION	0x2
#define OVERFLOW_EXCEPTION		0x4
#define UNDERFLOW_EXCEPTION		0x8
/* The next flags signals that those exceptions are allowed but not required.   */
#define INVALID_EXCEPTION_OK		0x10
#define DIVIDE_BY_ZERO_EXCEPTION_OK	0x20
#define OVERFLOW_EXCEPTION_OK		0x40
#define UNDERFLOW_EXCEPTION_OK		0x80
#define EXCEPTIONS_OK INVALID_EXCEPTION_OK+DIVIDE_BY_ZERO_EXCEPTION_OK
/* Some special test flags, passed together with exceptions.  */
#define IGNORE_ZERO_INF_SIGN		0x100

/* Values underflowing only for float.  */
#ifdef TEST_FLOAT
# define UNDERFLOW_EXCEPTION_FLOAT	UNDERFLOW_EXCEPTION
# define UNDERFLOW_EXCEPTION_OK_FLOAT	UNDERFLOW_EXCEPTION_OK
#else
# define UNDERFLOW_EXCEPTION_FLOAT	0
# define UNDERFLOW_EXCEPTION_OK_FLOAT	0
#endif
/* Values underflowing only for double or types with a larger least
   positive normal value.  */
#if defined TEST_FLOAT || defined TEST_DOUBLE \
  || (defined TEST_LDOUBLE && LDBL_MIN_EXP >= DBL_MIN_EXP)
# define UNDERFLOW_EXCEPTION_DOUBLE	UNDERFLOW_EXCEPTION
#else
# define UNDERFLOW_EXCEPTION_DOUBLE	0
#endif
/* Values underflowing only for IBM long double or types with a larger least
   positive normal value.  */
#if defined TEST_FLOAT || (defined TEST_LDOUBLE && LDBL_MIN_EXP > DBL_MIN_EXP)
# define UNDERFLOW_EXCEPTION_LDOUBLE_IBM	UNDERFLOW_EXCEPTION
#else
# define UNDERFLOW_EXCEPTION_LDOUBLE_IBM	0
#endif
/* Values underflowing on architectures detecting tininess before
   rounding, but not on those detecting tininess after rounding.  */
#define UNDERFLOW_EXCEPTION_BEFORE_ROUNDING	(TININESS_AFTER_ROUNDING \
						 ? 0			\
						 : UNDERFLOW_EXCEPTION)

/* Various constants (we must supply them precalculated for accuracy).  */
#define M_PI_6l			.52359877559829887307710723054658383L
#define M_E2l			7.389056098930650227230427460575008L
#define M_E3l			20.085536923187667740928529654581719L
#define M_2_SQRT_PIl		3.5449077018110320545963349666822903L	/* 2 sqrt (M_PIl)  */
#define M_SQRT_PIl		1.7724538509055160272981674833411451L	/* sqrt (M_PIl)  */
#define M_LOG_SQRT_PIl		0.57236494292470008707171367567652933L	/* log(sqrt(M_PIl))  */
#define M_LOG_2_SQRT_PIl	1.265512123484645396488945797134706L	/* log(2*sqrt(M_PIl))  */
#define M_PI_34l		(M_PIl - M_PI_4l)		/* 3*pi/4 */
#define M_PI_34_LOG10El		(M_PIl - M_PI_4l) * M_LOG10El
#define M_PI2_LOG10El		M_PI_2l * M_LOG10El
#define M_PI4_LOG10El		M_PI_4l * M_LOG10El
#define M_PI_LOG10El		M_PIl * M_LOG10El
#define M_SQRT_2_2		0.70710678118654752440084436210484903L /* sqrt (2) / 2 */

static FILE *ulps_file;	/* File to document difference.  */
static int output_ulps;	/* Should ulps printed?  */

static int noErrors;	/* number of errors */
static int noTests;	/* number of tests (without testing exceptions) */
static int noExcTests;	/* number of tests for exception flags */
static int noXFails;	/* number of expected failures.  */
static int noXPasses;	/* number of unexpected passes.  */

static int verbose;
static int output_max_error;	/* Should the maximal errors printed?  */
static int output_points;	/* Should the single function results printed?  */
static int ignore_max_ulp;	/* Should we ignore max_ulp?  */

static FLOAT minus_zero, plus_zero;
static FLOAT plus_infty, minus_infty, nan_value, max_value, min_value;
static FLOAT min_subnorm_value;

static FLOAT max_error, real_max_error, imag_max_error;


#define BUILD_COMPLEX(real, imag) \
  ({ __complex__ FLOAT __retval;					      \
     __real__ __retval = (real);					      \
     __imag__ __retval = (imag);					      \
     __retval; })

#define BUILD_COMPLEX_INT(real, imag) \
  ({ __complex__ int __retval;						      \
     __real__ __retval = (real);					      \
     __imag__ __retval = (imag);					      \
     __retval; })


#define MANT_DIG CHOOSE ((LDBL_MANT_DIG-1), (DBL_MANT_DIG-1), (FLT_MANT_DIG-1),  \
			 (LDBL_MANT_DIG-1), (DBL_MANT_DIG-1), (FLT_MANT_DIG-1))

static void
init_max_error (void)
{
  max_error = 0;
  real_max_error = 0;
  imag_max_error = 0;
  feclearexcept (FE_ALL_EXCEPT);
}

static void
set_max_error (FLOAT current, FLOAT *curr_max_error)
{
  if (current > *curr_max_error)
    *curr_max_error = current;
}


/* Should the message print to screen?  This depends on the verbose flag,
   and the test status.  */
static int
print_screen (int ok, int xfail)
{
  if (output_points
      && (verbose > 1
	  || (verbose == 1 && ok == xfail)))
    return 1;
  return 0;
}


/* Should the message print to screen?  This depends on the verbose flag,
   and the test status.  */
static int
print_screen_max_error (int ok, int xfail)
{
  if (output_max_error
      && (verbose > 1
	  || ((verbose == 1) && (ok == xfail))))
    return 1;
  return 0;
}

/* Update statistic counters.  */
static void
update_stats (int ok, int xfail)
{
  ++noTests;
  if (ok && xfail)
    ++noXPasses;
  else if (!ok && xfail)
    ++noXFails;
  else if (!ok && !xfail)
    ++noErrors;
}

static void
print_ulps (const char *test_name, FLOAT ulp)
{
  if (output_ulps)
    {
      fprintf (ulps_file, "Test \"%s\":\n", test_name);
      fprintf (ulps_file, "%s: %.0" PRINTF_NEXPR "\n",
	       CHOOSE("ldouble", "double", "float",
		      "ildouble", "idouble", "ifloat"),
	       FUNC(ceil) (ulp));
    }
}

static void
print_function_ulps (const char *function_name, FLOAT ulp)
{
  if (output_ulps)
    {
      fprintf (ulps_file, "Function: \"%s\":\n", function_name);
      fprintf (ulps_file, "%s: %.0" PRINTF_NEXPR "\n",
	       CHOOSE("ldouble", "double", "float",
		      "ildouble", "idouble", "ifloat"),
	       FUNC(ceil) (ulp));
    }
}


static void
print_complex_function_ulps (const char *function_name, FLOAT real_ulp,
			     FLOAT imag_ulp)
{
  if (output_ulps)
    {
      if (real_ulp != 0.0)
	{
	  fprintf (ulps_file, "Function: Real part of \"%s\":\n", function_name);
	  fprintf (ulps_file, "%s: %.0" PRINTF_NEXPR "\n",
		   CHOOSE("ldouble", "double", "float",
			  "ildouble", "idouble", "ifloat"),
		   FUNC(ceil) (real_ulp));
	}
      if (imag_ulp != 0.0)
	{
	  fprintf (ulps_file, "Function: Imaginary part of \"%s\":\n", function_name);
	  fprintf (ulps_file, "%s: %.0" PRINTF_NEXPR "\n",
		   CHOOSE("ldouble", "double", "float",
			  "ildouble", "idouble", "ifloat"),
		   FUNC(ceil) (imag_ulp));
	}


    }
}



/* Test if Floating-Point stack hasn't changed */
static void
fpstack_test (const char *test_name)
{
#ifdef i386
  static int old_stack;
  int sw;

  asm ("fnstsw" : "=a" (sw));
  sw >>= 11;
  sw &= 7;

  if (sw != old_stack)
    {
      printf ("FP-Stack wrong after test %s (%d, should be %d)\n",
	      test_name, sw, old_stack);
      ++noErrors;
      old_stack = sw;
    }
#endif
}


static void
print_max_error (const char *func_name, FLOAT allowed, int xfail)
{
  int ok = 0;

  if (max_error == 0.0 || (max_error <= allowed && !ignore_max_ulp))
    {
      ok = 1;
    }

  if (!ok)
    print_function_ulps (func_name, max_error);


  if (print_screen_max_error (ok, xfail))
    {
      printf ("Maximal error of `%s'\n", func_name);
      printf (" is      : %.0" PRINTF_NEXPR " ulp\n", FUNC(ceil) (max_error));
      printf (" accepted: %.0" PRINTF_NEXPR " ulp\n", FUNC(ceil) (allowed));
    }

  update_stats (ok, xfail);
}


static void
print_complex_max_error (const char *func_name, __complex__ FLOAT allowed,
			 __complex__ int xfail)
{
  int ok = 0;

  if ((real_max_error == 0 && imag_max_error == 0)
      || (real_max_error <= __real__ allowed
	  && imag_max_error <= __imag__ allowed
	  && !ignore_max_ulp))
    {
      ok = 1;
    }

  if (!ok)
    print_complex_function_ulps (func_name, real_max_error, imag_max_error);


  if (print_screen_max_error (ok, xfail))
    {
      printf ("Maximal error of real part of: %s\n", func_name);
      printf (" is      : %.0" PRINTF_NEXPR " ulp\n",
	      FUNC(ceil) (real_max_error));
      printf (" accepted: %.0" PRINTF_NEXPR " ulp\n",
	      FUNC(ceil) (__real__ allowed));
      printf ("Maximal error of imaginary part of: %s\n", func_name);
      printf (" is      : %.0" PRINTF_NEXPR " ulp\n",
	      FUNC(ceil) (imag_max_error));
      printf (" accepted: %.0" PRINTF_NEXPR " ulp\n",
	      FUNC(ceil) (__imag__ allowed));
    }

  update_stats (ok, xfail);
}


/* Test whether a given exception was raised.  */
static void
test_single_exception (const char *test_name,
		       int exception,
		       int exc_flag,
		       int fe_flag,
		       const char *flag_name)
{
#ifndef TEST_INLINE
  int ok = 1;
  if (exception & exc_flag)
    {
      if (fetestexcept (fe_flag))
	{
	  if (print_screen (1, 0))
	    printf ("Pass: %s: Exception \"%s\" set\n", test_name, flag_name);
	}
      else
	{
	  ok = 0;
	  if (print_screen (0, 0))
	    printf ("Failure: %s: Exception \"%s\" not set\n",
		    test_name, flag_name);
	}
    }
  else
    {
      if (fetestexcept (fe_flag))
	{
	  ok = 0;
	  if (print_screen (0, 0))
	    printf ("Failure: %s: Exception \"%s\" set\n",
		    test_name, flag_name);
	}
      else
	{
	  if (print_screen (1, 0))
	    printf ("%s: Exception \"%s\" not set\n", test_name,
		    flag_name);
	}
    }
  if (!ok)
    ++noErrors;

#endif
}


/* Test whether exceptions given by EXCEPTION are raised.  Ignore thereby
   allowed but not required exceptions.
*/
static void
test_exceptions (const char *test_name, int exception)
{
  ++noExcTests;
#ifdef FE_DIVBYZERO
  if ((exception & DIVIDE_BY_ZERO_EXCEPTION_OK) == 0)
    test_single_exception (test_name, exception,
			   DIVIDE_BY_ZERO_EXCEPTION, FE_DIVBYZERO,
			   "Divide by zero");
#endif
#ifdef FE_INVALID
  if ((exception & INVALID_EXCEPTION_OK) == 0)
    test_single_exception (test_name, exception, INVALID_EXCEPTION, FE_INVALID,
			 "Invalid operation");
#endif
#ifdef FE_OVERFLOW
  if ((exception & OVERFLOW_EXCEPTION_OK) == 0)
    test_single_exception (test_name, exception, OVERFLOW_EXCEPTION,
			   FE_OVERFLOW, "Overflow");
#endif
#ifdef FE_UNDERFLOW
  if ((exception & UNDERFLOW_EXCEPTION_OK) == 0)
    test_single_exception (test_name, exception, UNDERFLOW_EXCEPTION,
			   FE_UNDERFLOW, "Underflow");
#endif
  feclearexcept (FE_ALL_EXCEPT);
}


static void
check_float_internal (const char *test_name, FLOAT computed, FLOAT expected,
		      FLOAT max_ulp, int xfail, int exceptions,
		      FLOAT *curr_max_error)
{
  int ok = 0;
  int print_diff = 0;
  FLOAT diff = 0;
  FLOAT ulp = 0;

  test_exceptions (test_name, exceptions);
  if (isnan (computed) && isnan (expected))
    ok = 1;
  else if (isinf (computed) && isinf (expected))
    {
      /* Test for sign of infinities.  */
      if ((exceptions & IGNORE_ZERO_INF_SIGN) == 0
	  && signbit (computed) != signbit (expected))
	{
	  ok = 0;
	  printf ("infinity has wrong sign.\n");
	}
      else
	ok = 1;
    }
  /* Don't calc ulp for NaNs or infinities.  */
  else if (isinf (computed) || isnan (computed) || isinf (expected) || isnan (expected))
    ok = 0;
  else
    {
      diff = FUNC(fabs) (computed - expected);
      switch (fpclassify (expected))
	{
	case FP_ZERO:
	  /* ilogb (0) isn't allowed. */
	  ulp = diff / FUNC(ldexp) (1.0, - MANT_DIG);
	  break;
	case FP_NORMAL:
	  ulp = diff / FUNC(ldexp) (1.0, FUNC(ilogb) (expected) - MANT_DIG);
	  break;
	case FP_SUBNORMAL:
	  /* 1ulp for a subnormal value, shifted by MANT_DIG, is the
	     least normal value.  */
	  ulp = (FUNC(ldexp) (diff, MANT_DIG) / min_value);
	  break;
	default:
	  /* It should never happen. */
	  abort ();
	  break;
	}
      set_max_error (ulp, curr_max_error);
      print_diff = 1;
      if ((exceptions & IGNORE_ZERO_INF_SIGN) == 0
	  && computed == 0.0 && expected == 0.0
	  && signbit(computed) != signbit (expected))
	ok = 0;
      else if (ulp <= 0.5 || (ulp <= max_ulp && !ignore_max_ulp))
	ok = 1;
      else
	{
	  ok = 0;
	  print_ulps (test_name, ulp);
	}

    }
  if (print_screen (ok, xfail))
    {
      if (!ok)
	printf ("Failure: ");
      printf ("Test: %s\n", test_name);
      printf ("Result:\n");
      printf (" is:         % .20" PRINTF_EXPR "  % .20" PRINTF_XEXPR "\n",
	      computed, computed);
      printf (" should be:  % .20" PRINTF_EXPR "  % .20" PRINTF_XEXPR "\n",
	      expected, expected);
      if (print_diff)
	{
	  printf (" difference: % .20" PRINTF_EXPR "  % .20" PRINTF_XEXPR
		  "\n", diff, diff);
	  printf (" ulp       : % .4" PRINTF_NEXPR "\n", ulp);
	  printf (" max.ulp   : % .4" PRINTF_NEXPR "\n", max_ulp);
	}
    }
  update_stats (ok, xfail);

  fpstack_test (test_name);
}


static void
check_float (const char *test_name, FLOAT computed, FLOAT expected,
	     FLOAT max_ulp, int xfail, int exceptions)
{
  check_float_internal (test_name, computed, expected, max_ulp, xfail,
			exceptions, &max_error);
}


static void
check_complex (const char *test_name, __complex__ FLOAT computed,
	       __complex__ FLOAT expected,
	       __complex__ FLOAT max_ulp, __complex__ int xfail,
	       int exception)
{
  FLOAT part_comp, part_exp, part_max_ulp;
  int part_xfail;
  char *str;

  if (asprintf (&str, "Real part of: %s", test_name) == -1)
    abort ();

  part_comp = __real__ computed;
  part_exp = __real__ expected;
  part_max_ulp = __real__ max_ulp;
  part_xfail = __real__ xfail;

  check_float_internal (str, part_comp, part_exp, part_max_ulp, part_xfail,
			exception, &real_max_error);
  free (str);

  if (asprintf (&str, "Imaginary part of: %s", test_name) == -1)
    abort ();

  part_comp = __imag__ computed;
  part_exp = __imag__ expected;
  part_max_ulp = __imag__ max_ulp;
  part_xfail = __imag__ xfail;

  /* Don't check again for exceptions, just pass through the
     zero/inf sign test.  */
  check_float_internal (str, part_comp, part_exp, part_max_ulp, part_xfail,
			exception & IGNORE_ZERO_INF_SIGN,
			&imag_max_error);
  free (str);
}


/* Check that computed and expected values are equal (int values).  */
static void
check_int (const char *test_name, int computed, int expected, int max_ulp,
	   int xfail, int exceptions)
{
  int diff = computed - expected;
  int ok = 0;

  test_exceptions (test_name, exceptions);
  noTests++;
  if (abs (diff) <= max_ulp)
    ok = 1;

  if (!ok)
    print_ulps (test_name, diff);

  if (print_screen (ok, xfail))
    {
      if (!ok)
	printf ("Failure: ");
      printf ("Test: %s\n", test_name);
      printf ("Result:\n");
      printf (" is:         %d\n", computed);
      printf (" should be:  %d\n", expected);
    }

  update_stats (ok, xfail);
  fpstack_test (test_name);
}


/* Check that computed and expected values are equal (long int values).  */
static void
check_long (const char *test_name, long int computed, long int expected,
	    long int max_ulp, int xfail, int exceptions)
{
  long int diff = computed - expected;
  int ok = 0;

  test_exceptions (test_name, exceptions);
  noTests++;
  if (labs (diff) <= max_ulp)
    ok = 1;

  if (!ok)
    print_ulps (test_name, diff);

  if (print_screen (ok, xfail))
    {
      if (!ok)
	printf ("Failure: ");
      printf ("Test: %s\n", test_name);
      printf ("Result:\n");
      printf (" is:         %ld\n", computed);
      printf (" should be:  %ld\n", expected);
    }

  update_stats (ok, xfail);
  fpstack_test (test_name);
}


/* Check that computed value is true/false.  */
static void
check_bool (const char *test_name, int computed, int expected,
	    long int max_ulp, int xfail, int exceptions)
{
  int ok = 0;

  test_exceptions (test_name, exceptions);
  noTests++;
  if ((computed == 0) == (expected == 0))
    ok = 1;

  if (print_screen (ok, xfail))
    {
      if (!ok)
	printf ("Failure: ");
      printf ("Test: %s\n", test_name);
      printf ("Result:\n");
      printf (" is:         %d\n", computed);
      printf (" should be:  %d\n", expected);
    }

  update_stats (ok, xfail);
  fpstack_test (test_name);
}


/* check that computed and expected values are equal (long int values) */
static void
check_longlong (const char *test_name, long long int computed,
		long long int expected,
		long long int max_ulp, int xfail,
		int exceptions)
{
  long long int diff = computed - expected;
  int ok = 0;

  test_exceptions (test_name, exceptions);
  noTests++;
  if (llabs (diff) <= max_ulp)
    ok = 1;

  if (!ok)
    print_ulps (test_name, diff);

  if (print_screen (ok, xfail))
    {
      if (!ok)
	printf ("Failure:");
      printf ("Test: %s\n", test_name);
      printf ("Result:\n");
      printf (" is:         %lld\n", computed);
      printf (" should be:  %lld\n", expected);
    }

  update_stats (ok, xfail);
  fpstack_test (test_name);
}



/* This is to prevent messages from the SVID libm emulation.  */
int
matherr (struct exception *x __attribute__ ((unused)))
{
  return 1;
}


/****************************************************************************
  Tests for single functions of libm.
  Please keep them alphabetically sorted!
****************************************************************************/

static void
acos_test (void)
{
  errno = 0;
  FUNC(acos) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_float ("acos (inf) == NaN",  FUNC(acos) (plus_infty), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("acos (-inf) == NaN",  FUNC(acos) (minus_infty), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("acos (NaN) == NaN",  FUNC(acos) (nan_value), nan_value, 0, 0, 0);

  /* |x| > 1: */
  check_float ("acos (1.125) == NaN",  FUNC(acos) (1.125L), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("acos (-1.125) == NaN",  FUNC(acos) (-1.125L), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("acos (max_value) == NaN",  FUNC(acos) (max_value), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("acos (-max_value) == NaN",  FUNC(acos) (-max_value), nan_value, 0, 0, INVALID_EXCEPTION);

  check_float ("acos (0) == pi/2",  FUNC(acos) (0), M_PI_2l, 0, 0, 0);
  check_float ("acos (-0) == pi/2",  FUNC(acos) (minus_zero), M_PI_2l, 0, 0, 0);
  check_float ("acos (1) == 0",  FUNC(acos) (1), 0, 0, 0, 0);
  check_float ("acos (-1) == pi",  FUNC(acos) (-1), M_PIl, 0, 0, 0);
  check_float ("acos (0.5) == M_PI_6l*2.0",  FUNC(acos) (0.5), M_PI_6l*2.0, 0, 0, 0);
  check_float ("acos (-0.5) == M_PI_6l*4.0",  FUNC(acos) (-0.5), M_PI_6l*4.0, 0, 0, 0);
  check_float ("acos (0.75) == 0.722734247813415611178377352641333362",  FUNC(acos) (0.75L), 0.722734247813415611178377352641333362L, DELTA13, 0, 0);
  check_float ("acos (2e-17) == 1.57079632679489659923132169163975144",  FUNC(acos) (2e-17L), 1.57079632679489659923132169163975144L, 0, 0, 0);
  check_float ("acos (0.0625) == 1.50825556499840522843072005474337068",  FUNC(acos) (0.0625L), 1.50825556499840522843072005474337068L, 0, 0, 0);
  check_float ("acos (0x0.ffffffp0) == 3.4526698471620358760324948263873649728491e-4",  FUNC(acos) (0x0.ffffffp0L), 3.4526698471620358760324948263873649728491e-4L, 0, 0, 0);
  check_float ("acos (-0x0.ffffffp0) == 3.1412473866050770348750401337968641476999",  FUNC(acos) (-0x0.ffffffp0L), 3.1412473866050770348750401337968641476999L, 0, 0, 0);
#ifndef TEST_FLOAT
  check_float ("acos (0x0.ffffffff8p0) == 1.5258789062648029736620564947844627548516e-5",  FUNC(acos) (0x0.ffffffff8p0L), 1.5258789062648029736620564947844627548516e-5L, 0, 0, 0);
  check_float ("acos (-0x0.ffffffff8p0) == 3.1415773948007305904329067627145550395696",  FUNC(acos) (-0x0.ffffffff8p0L), 3.1415773948007305904329067627145550395696L, 0, 0, 0);
  check_float ("acos (0x0.ffffffffffffp0) == 8.4293697021788088529885473244391795127130e-8",  FUNC(acos) (0x0.ffffffffffffp0L), 8.4293697021788088529885473244391795127130e-8L, 0, 0, 0);
  check_float ("acos (-0x0.ffffffffffffp0) == 3.1415925692960962166745548533940296398054",  FUNC(acos) (-0x0.ffffffffffffp0L), 3.1415925692960962166745548533940296398054L, 0, 0, 0);
#endif
#if defined TEST_LDOUBLE && LDBL_MANT_DIG >= 64
  check_float ("acos (0x0.ffffffffffffffffp0) == 3.2927225399135962333718255320079907245059e-10",  FUNC(acos) (0x0.ffffffffffffffffp0L), 3.2927225399135962333718255320079907245059e-10L, 0, 0, 0);
  check_float ("acos (-0x0.ffffffffffffffffp0) == 3.1415926532605209844712837599423203309964",  FUNC(acos) (-0x0.ffffffffffffffffp0L), 3.1415926532605209844712837599423203309964L, 0, 0, 0);
#endif
  print_max_error ("acos", DELTAacos, 0);
}


static void
acos_test_tonearest (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(acos) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_TONEAREST))
    {
  check_float ("acos_tonearest (0) == pi/2",  FUNC(acos) (0), M_PI_2l, 0, 0, 0);
  check_float ("acos_tonearest (-0) == pi/2",  FUNC(acos) (minus_zero), M_PI_2l, 0, 0, 0);
  check_float ("acos_tonearest (1) == 0",  FUNC(acos) (1), 0, 0, 0, 0);
  check_float ("acos_tonearest (-1) == pi",  FUNC(acos) (-1), M_PIl, 0, 0, 0);
  check_float ("acos_tonearest (0.5) == M_PI_6l*2.0",  FUNC(acos) (0.5), M_PI_6l*2.0, 0, 0, 0);
  check_float ("acos_tonearest (-0.5) == M_PI_6l*4.0",  FUNC(acos) (-0.5), M_PI_6l*4.0, 0, 0, 0);
    }

  fesetround (save_round_mode);

  print_max_error ("acos_tonearest", 0, 0);
}


static void
acos_test_towardzero (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(acos) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_TOWARDZERO))
    {
  check_float ("acos_towardzero (0) == pi/2",  FUNC(acos) (0), M_PI_2l, DELTA30, 0, 0);
  check_float ("acos_towardzero (-0) == pi/2",  FUNC(acos) (minus_zero), M_PI_2l, DELTA31, 0, 0);
  check_float ("acos_towardzero (1) == 0",  FUNC(acos) (1), 0, 0, 0, 0);
  check_float ("acos_towardzero (-1) == pi",  FUNC(acos) (-1), M_PIl, DELTA33, 0, 0);
  check_float ("acos_towardzero (0.5) == M_PI_6l*2.0",  FUNC(acos) (0.5), M_PI_6l*2.0, DELTA34, 0, 0);
  check_float ("acos_towardzero (-0.5) == M_PI_6l*4.0",  FUNC(acos) (-0.5), M_PI_6l*4.0, DELTA35, 0, 0);
    }

  fesetround (save_round_mode);

  print_max_error ("acos_towardzero", DELTAacos_towardzero, 0);
}


static void
acos_test_downward (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(acos) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_DOWNWARD))
    {
  check_float ("acos_downward (0) == pi/2",  FUNC(acos) (0), M_PI_2l, DELTA36, 0, 0);
  check_float ("acos_downward (-0) == pi/2",  FUNC(acos) (minus_zero), M_PI_2l, DELTA37, 0, 0);
  check_float ("acos_downward (1) == 0",  FUNC(acos) (1), 0, 0, 0, 0);
  check_float ("acos_downward (-1) == pi",  FUNC(acos) (-1), M_PIl, DELTA39, 0, 0);
  check_float ("acos_downward (0.5) == M_PI_6l*2.0",  FUNC(acos) (0.5), M_PI_6l*2.0, DELTA40, 0, 0);
  check_float ("acos_downward (-0.5) == M_PI_6l*4.0",  FUNC(acos) (-0.5), M_PI_6l*4.0, DELTA41, 0, 0);
    }

  fesetround (save_round_mode);

  print_max_error ("acos_downward", DELTAacos_downward, 0);
}


static void
acos_test_upward (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(acos) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_UPWARD))
    {
  check_float ("acos_upward (0) == pi/2",  FUNC(acos) (0), M_PI_2l, 0, 0, 0);
  check_float ("acos_upward (-0) == pi/2",  FUNC(acos) (minus_zero), M_PI_2l, 0, 0, 0);
  check_float ("acos_upward (1) == 0",  FUNC(acos) (1), 0, 0, 0, 0);
  check_float ("acos_upward (-1) == pi",  FUNC(acos) (-1), M_PIl, 0, 0, 0);
  check_float ("acos_upward (0.5) == M_PI_6l*2.0",  FUNC(acos) (0.5), M_PI_6l*2.0, DELTA46, 0, 0);
  check_float ("acos_upward (-0.5) == M_PI_6l*4.0",  FUNC(acos) (-0.5), M_PI_6l*4.0, DELTA47, 0, 0);
    }

  fesetround (save_round_mode);

  print_max_error ("acos_upward", DELTAacos_upward, 0);
}

static void
acosh_test (void)
{
  errno = 0;
  FUNC(acosh) (7);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_float ("acosh (inf) == inf",  FUNC(acosh) (plus_infty), plus_infty, 0, 0, 0);
  check_float ("acosh (-inf) == NaN",  FUNC(acosh) (minus_infty), nan_value, 0, 0, INVALID_EXCEPTION);

  /* x < 1:  */
  check_float ("acosh (-1.125) == NaN",  FUNC(acosh) (-1.125L), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("acosh (-max_value) == NaN",  FUNC(acosh) (-max_value), nan_value, 0, 0, INVALID_EXCEPTION);

  check_float ("acosh (1) == 0",  FUNC(acosh) (1), 0, 0, 0, 0);
  check_float ("acosh (7) == 2.63391579384963341725009269461593689",  FUNC(acosh) (7), 2.63391579384963341725009269461593689L, 0, 0, 0);

  print_max_error ("acosh", 0, 0);
}

static void
asin_test (void)
{
  errno = 0;
  FUNC(asin) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_float ("asin (inf) == NaN",  FUNC(asin) (plus_infty), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("asin (-inf) == NaN",  FUNC(asin) (minus_infty), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("asin (NaN) == NaN",  FUNC(asin) (nan_value), nan_value, 0, 0, 0);

  /* asin x == NaN plus invalid exception for |x| > 1.  */
  check_float ("asin (1.125) == NaN",  FUNC(asin) (1.125L), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("asin (-1.125) == NaN",  FUNC(asin) (-1.125L), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("asin (max_value) == NaN",  FUNC(asin) (max_value), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("asin (-max_value) == NaN",  FUNC(asin) (-max_value), nan_value, 0, 0, INVALID_EXCEPTION);

  check_float ("asin (0) == 0",  FUNC(asin) (0), 0, 0, 0, 0);
  check_float ("asin (-0) == -0",  FUNC(asin) (minus_zero), minus_zero, 0, 0, 0);
  check_float ("asin (0.5) == pi/6",  FUNC(asin) (0.5), M_PI_6l, DELTA63, 0, 0);
  check_float ("asin (-0.5) == -pi/6",  FUNC(asin) (-0.5), -M_PI_6l, DELTA64, 0, 0);
  check_float ("asin (1.0) == pi/2",  FUNC(asin) (1.0), M_PI_2l, DELTA65, 0, 0);
  check_float ("asin (-1.0) == -pi/2",  FUNC(asin) (-1.0), -M_PI_2l, DELTA66, 0, 0);
  check_float ("asin (0.75) == 0.848062078981481008052944338998418080",  FUNC(asin) (0.75L), 0.848062078981481008052944338998418080L, DELTA67, 0, 0);
  check_float ("asin (0x0.ffffffp0) == 1.5704510598101804156437184421571127056013",  FUNC(asin) (0x0.ffffffp0L), 1.5704510598101804156437184421571127056013L, 0, 0, 0);
  check_float ("asin (-0x0.ffffffp0) == -1.5704510598101804156437184421571127056013",  FUNC(asin) (-0x0.ffffffp0L), -1.5704510598101804156437184421571127056013L, 0, 0, 0);
#ifndef TEST_FLOAT
  check_float ("asin (0x0.ffffffff8p0) == 1.5707810680058339712015850710748035974710",  FUNC(asin) (0x0.ffffffff8p0L), 1.5707810680058339712015850710748035974710L, 0, 0, 0);
  check_float ("asin (-0x0.ffffffff8p0) == -1.5707810680058339712015850710748035974710",  FUNC(asin) (-0x0.ffffffff8p0L), -1.5707810680058339712015850710748035974710L, 0, 0, 0);
  check_float ("asin (0x0.ffffffffffffp0) == 1.5707962425011995974432331617542781977068",  FUNC(asin) (0x0.ffffffffffffp0L), 1.5707962425011995974432331617542781977068L, DELTA72, 0, 0);
  check_float ("asin (-0x0.ffffffffffffp0) == -1.5707962425011995974432331617542781977068",  FUNC(asin) (-0x0.ffffffffffffp0L), -1.5707962425011995974432331617542781977068L, DELTA73, 0, 0);
#endif
#if defined TEST_LDOUBLE && LDBL_MANT_DIG >= 64
  check_float ("asin (0x0.ffffffffffffffffp0) == 1.5707963264656243652399620683025688888978",  FUNC(asin) (0x0.ffffffffffffffffp0L), 1.5707963264656243652399620683025688888978L, DELTA74, 0, 0);
  check_float ("asin (-0x0.ffffffffffffffffp0) == -1.5707963264656243652399620683025688888978",  FUNC(asin) (-0x0.ffffffffffffffffp0L), -1.5707963264656243652399620683025688888978L, DELTA75, 0, 0);
#endif

  print_max_error ("asin", DELTAasin, 0);
}


static void
asin_test_tonearest (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(asin) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_TONEAREST))
    {
  check_float ("asin_tonearest (0) == 0",  FUNC(asin) (0), 0, 0, 0, 0);
  check_float ("asin_tonearest (-0) == -0",  FUNC(asin) (minus_zero), minus_zero, 0, 0, 0);
  check_float ("asin_tonearest (0.5) == pi/6",  FUNC(asin) (0.5), M_PI_6l, DELTA78, 0, 0);
  check_float ("asin_tonearest (-0.5) == -pi/6",  FUNC(asin) (-0.5), -M_PI_6l, DELTA79, 0, 0);
  check_float ("asin_tonearest (1.0) == pi/2",  FUNC(asin) (1.0), M_PI_2l, DELTA80, 0, 0);
  check_float ("asin_tonearest (-1.0) == -pi/2",  FUNC(asin) (-1.0), -M_PI_2l, DELTA81, 0, 0);
    }

  fesetround (save_round_mode);

  print_max_error ("asin_tonearest", DELTAasin_tonearest, 0);
}


static void
asin_test_towardzero (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(asin) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_TOWARDZERO))
    {
  check_float ("asin_towardzero (0) == 0",  FUNC(asin) (0), 0, 0, 0, 0);
  check_float ("asin_towardzero (-0) == -0",  FUNC(asin) (minus_zero), minus_zero, 0, 0, 0);
  check_float ("asin_towardzero (0.5) == pi/6",  FUNC(asin) (0.5), M_PI_6l, DELTA84, 0, 0);
  check_float ("asin_towardzero (-0.5) == -pi/6",  FUNC(asin) (-0.5), -M_PI_6l, DELTA85, 0, 0);
  check_float ("asin_towardzero (1.0) == pi/2",  FUNC(asin) (1.0), M_PI_2l, DELTA86, 0, 0);
  check_float ("asin_towardzero (-1.0) == -pi/2",  FUNC(asin) (-1.0), -M_PI_2l, DELTA87, 0, 0);
    }

  fesetround (save_round_mode);

  print_max_error ("asin_towardzero", DELTAasin_towardzero, 0);
}


static void
asin_test_downward (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(asin) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_DOWNWARD))
    {
  check_float ("asin_downward (0) == 0",  FUNC(asin) (0), 0, 0, 0, 0);
  check_float ("asin_downward (-0) == -0",  FUNC(asin) (minus_zero), minus_zero, 0, 0, 0);
  check_float ("asin_downward (0.5) == pi/6",  FUNC(asin) (0.5), M_PI_6l, DELTA90, 0, 0);
  check_float ("asin_downward (-0.5) == -pi/6",  FUNC(asin) (-0.5), -M_PI_6l, DELTA91, 0, 0);
  check_float ("asin_downward (1.0) == pi/2",  FUNC(asin) (1.0), M_PI_2l, DELTA92, 0, 0);
  check_float ("asin_downward (-1.0) == -pi/2",  FUNC(asin) (-1.0), -M_PI_2l, 0, 0, 0);
    }

  fesetround (save_round_mode);

  print_max_error ("asin_downward", DELTAasin_downward, 0);
}


static void
asin_test_upward (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(asin) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_UPWARD))
    {
  check_float ("asin_upward (0) == 0",  FUNC(asin) (0), 0, 0, 0, 0);
  check_float ("asin_upward (-0) == -0",  FUNC(asin) (minus_zero), minus_zero, 0, 0, 0);
  check_float ("asin_upward (0.5) == pi/6",  FUNC(asin) (0.5), M_PI_6l, DELTA96, 0, 0);
  check_float ("asin_upward (-0.5) == -pi/6",  FUNC(asin) (-0.5), -M_PI_6l, DELTA97, 0, 0);
  check_float ("asin_upward (1.0) == pi/2",  FUNC(asin) (1.0), M_PI_2l, 0, 0, 0);
  check_float ("asin_upward (-1.0) == -pi/2",  FUNC(asin) (-1.0), -M_PI_2l, DELTA99, 0, 0);
    }

  fesetround (save_round_mode);

  print_max_error ("asin_upward", DELTAasin_upward, 0);
}

static void
asinh_test (void)
{
  errno = 0;
  FUNC(asinh) (0.7L);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_float ("asinh (0) == 0",  FUNC(asinh) (0), 0, 0, 0, 0);
  check_float ("asinh (-0) == -0",  FUNC(asinh) (minus_zero), minus_zero, 0, 0, 0);
#ifndef TEST_INLINE
  check_float ("asinh (inf) == inf",  FUNC(asinh) (plus_infty), plus_infty, 0, 0, 0);
  check_float ("asinh (-inf) == -inf",  FUNC(asinh) (minus_infty), minus_infty, 0, 0, 0);
#endif
  check_float ("asinh (NaN) == NaN",  FUNC(asinh) (nan_value), nan_value, 0, 0, 0);
  check_float ("asinh (0.75) == 0.693147180559945309417232121458176568",  FUNC(asinh) (0.75L), 0.693147180559945309417232121458176568L, 0, 0, 0);

  print_max_error ("asinh", 0, 0);
}

static void
atan_test (void)
{
  errno = 0;
  FUNC(atan) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_float ("atan (0) == 0",  FUNC(atan) (0), 0, 0, 0, 0);
  check_float ("atan (-0) == -0",  FUNC(atan) (minus_zero), minus_zero, 0, 0, 0);

  check_float ("atan (inf) == pi/2",  FUNC(atan) (plus_infty), M_PI_2l, 0, 0, 0);
  check_float ("atan (-inf) == -pi/2",  FUNC(atan) (minus_infty), -M_PI_2l, 0, 0, 0);
  check_float ("atan (NaN) == NaN",  FUNC(atan) (nan_value), nan_value, 0, 0, 0);
  check_float ("atan (max_value) == pi/2",  FUNC(atan) (max_value), M_PI_2l, 0, 0, 0);
  check_float ("atan (-max_value) == -pi/2",  FUNC(atan) (-max_value), -M_PI_2l, 0, 0, 0);

  check_float ("atan (1) == pi/4",  FUNC(atan) (1), M_PI_4l, 0, 0, 0);
  check_float ("atan (-1) == -pi/4",  FUNC(atan) (-1), -M_PI_4l, 0, 0, 0);

  check_float ("atan (0.75) == 0.643501108793284386802809228717322638",  FUNC(atan) (0.75L), 0.643501108793284386802809228717322638L, 0, 0, 0);

  check_float ("atan (0x1p-100) == 0x1p-100",  FUNC(atan) (0x1p-100L), 0x1p-100L, 0, 0, 0);
#ifndef TEST_FLOAT
  check_float ("atan (0x1p-600) == 0x1p-600",  FUNC(atan) (0x1p-600L), 0x1p-600L, 0, 0, 0);
#endif
#if defined TEST_LDOUBLE && LDBL_MIN_EXP <= -16381
  check_float ("atan (0x1p-10000) == 0x1p-10000",  FUNC(atan) (0x1p-10000L), 0x1p-10000L, 0, 0, 0);
#endif

  print_max_error ("atan", 0, 0);
}



static void
atanh_test (void)
{
  errno = 0;
  FUNC(atanh) (0.7L);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();


  check_float ("atanh (0) == 0",  FUNC(atanh) (0), 0, 0, 0, 0);
  check_float ("atanh (-0) == -0",  FUNC(atanh) (minus_zero), minus_zero, 0, 0, 0);

  check_float ("atanh (1) == inf",  FUNC(atanh) (1), plus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_float ("atanh (-1) == -inf",  FUNC(atanh) (-1), minus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_float ("atanh (NaN) == NaN",  FUNC(atanh) (nan_value), nan_value, 0, 0, 0);

  /* atanh (x) == NaN plus invalid exception if |x| > 1.  */
  check_float ("atanh (1.125) == NaN",  FUNC(atanh) (1.125L), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("atanh (-1.125) == NaN",  FUNC(atanh) (-1.125L), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("atanh (max_value) == NaN",  FUNC(atanh) (max_value), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("atanh (-max_value) == NaN",  FUNC(atanh) (-max_value), nan_value, 0, 0, INVALID_EXCEPTION);

  check_float ("atanh (0.75) == 0.972955074527656652552676371721589865",  FUNC(atanh) (0.75L), 0.972955074527656652552676371721589865L, DELTA128, 0, 0);

  print_max_error ("atanh", DELTAatanh, 0);
}

static void
atan2_test (void)
{
  errno = 0;
  FUNC(atan2) (-0, 1);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  /* atan2 (0,x) == 0 for x > 0.  */
  check_float ("atan2 (0, 1) == 0",  FUNC(atan2) (0, 1), 0, 0, 0, 0);

  /* atan2 (-0,x) == -0 for x > 0.  */
  check_float ("atan2 (-0, 1) == -0",  FUNC(atan2) (minus_zero, 1), minus_zero, 0, 0, 0);

  check_float ("atan2 (0, 0) == 0",  FUNC(atan2) (0, 0), 0, 0, 0, 0);
  check_float ("atan2 (-0, 0) == -0",  FUNC(atan2) (minus_zero, 0), minus_zero, 0, 0, 0);

  /* atan2 (+0,x) == +pi for x < 0.  */
  check_float ("atan2 (0, -1) == pi",  FUNC(atan2) (0, -1), M_PIl, 0, 0, 0);

  /* atan2 (-0,x) == -pi for x < 0.  */
  check_float ("atan2 (-0, -1) == -pi",  FUNC(atan2) (minus_zero, -1), -M_PIl, 0, 0, 0);

  check_float ("atan2 (0, -0) == pi",  FUNC(atan2) (0, minus_zero), M_PIl, 0, 0, 0);
  check_float ("atan2 (-0, -0) == -pi",  FUNC(atan2) (minus_zero, minus_zero), -M_PIl, 0, 0, 0);

  /* atan2 (y,+0) == pi/2 for y > 0.  */
  check_float ("atan2 (1, 0) == pi/2",  FUNC(atan2) (1, 0), M_PI_2l, 0, 0, 0);

  /* atan2 (y,-0) == pi/2 for y > 0.  */
  check_float ("atan2 (1, -0) == pi/2",  FUNC(atan2) (1, minus_zero), M_PI_2l, 0, 0, 0);

  /* atan2 (y,+0) == -pi/2 for y < 0.  */
  check_float ("atan2 (-1, 0) == -pi/2",  FUNC(atan2) (-1, 0), -M_PI_2l, 0, 0, 0);

  /* atan2 (y,-0) == -pi/2 for y < 0.  */
  check_float ("atan2 (-1, -0) == -pi/2",  FUNC(atan2) (-1, minus_zero), -M_PI_2l, 0, 0, 0);

  /* atan2 (y,inf) == +0 for finite y > 0.  */
  check_float ("atan2 (1, inf) == 0",  FUNC(atan2) (1, plus_infty), 0, 0, 0, 0);

  /* atan2 (y,inf) == -0 for finite y < 0.  */
  check_float ("atan2 (-1, inf) == -0",  FUNC(atan2) (-1, plus_infty), minus_zero, 0, 0, 0);

  /* atan2(+inf, x) == pi/2 for finite x.  */
  check_float ("atan2 (inf, -1) == pi/2",  FUNC(atan2) (plus_infty, -1), M_PI_2l, 0, 0, 0);

  /* atan2(-inf, x) == -pi/2 for finite x.  */
  check_float ("atan2 (-inf, 1) == -pi/2",  FUNC(atan2) (minus_infty, 1), -M_PI_2l, 0, 0, 0);

  /* atan2 (y,-inf) == +pi for finite y > 0.  */
  check_float ("atan2 (1, -inf) == pi",  FUNC(atan2) (1, minus_infty), M_PIl, 0, 0, 0);

  /* atan2 (y,-inf) == -pi for finite y < 0.  */
  check_float ("atan2 (-1, -inf) == -pi",  FUNC(atan2) (-1, minus_infty), -M_PIl, 0, 0, 0);

  check_float ("atan2 (inf, inf) == pi/4",  FUNC(atan2) (plus_infty, plus_infty), M_PI_4l, 0, 0, 0);
  check_float ("atan2 (-inf, inf) == -pi/4",  FUNC(atan2) (minus_infty, plus_infty), -M_PI_4l, 0, 0, 0);
  check_float ("atan2 (inf, -inf) == 3/4 pi",  FUNC(atan2) (plus_infty, minus_infty), M_PI_34l, 0, 0, 0);
  check_float ("atan2 (-inf, -inf) == -3/4 pi",  FUNC(atan2) (minus_infty, minus_infty), -M_PI_34l, 0, 0, 0);
  check_float ("atan2 (NaN, NaN) == NaN",  FUNC(atan2) (nan_value, nan_value), nan_value, 0, 0, 0);

  check_float ("atan2 (max_value, max_value) == pi/4",  FUNC(atan2) (max_value, max_value), M_PI_4l, 0, 0, 0);

  check_float ("atan2 (max_value, min_value) == pi/2",  FUNC(atan2) (max_value, min_value), M_PI_2l, 0, 0, 0);
  check_float ("atan2 (-max_value, -min_value) == -pi/2",  FUNC(atan2) (-max_value, -min_value), -M_PI_2l, DELTA154, 0, 0);

  check_float ("atan2 (0.75, 1) == 0.643501108793284386802809228717322638",  FUNC(atan2) (0.75L, 1), 0.643501108793284386802809228717322638L, 0, 0, 0);
  check_float ("atan2 (-0.75, 1.0) == -0.643501108793284386802809228717322638",  FUNC(atan2) (-0.75L, 1.0L), -0.643501108793284386802809228717322638L, 0, 0, 0);
  check_float ("atan2 (0.75, -1.0) == 2.49809154479650885165983415456218025",  FUNC(atan2) (0.75L, -1.0L), 2.49809154479650885165983415456218025L, DELTA157, 0, 0);
  check_float ("atan2 (-0.75, -1.0) == -2.49809154479650885165983415456218025",  FUNC(atan2) (-0.75L, -1.0L), -2.49809154479650885165983415456218025L, DELTA158, 0, 0);
  check_float ("atan2 (0.390625, .00029) == 1.57005392693128974780151246612928941",  FUNC(atan2) (0.390625L, .00029L), 1.57005392693128974780151246612928941L, 0, 0, 0);
  check_float ("atan2 (1.390625, 0.9296875) == 0.981498387184244311516296577615519772",  FUNC(atan2) (1.390625L, 0.9296875L), 0.981498387184244311516296577615519772L, DELTA160, 0, 0);

  check_float ("atan2 (-0.00756827042671106339, -.001792735857538728036) == -1.80338464113663849327153994379639112",  FUNC(atan2) (-0.00756827042671106339L, -.001792735857538728036L), -1.80338464113663849327153994379639112L, 0, 0, 0);
#if defined TEST_LDOUBLE && LDBL_MANT_DIG >= 64
  check_float ("atan2 (0x1.00000000000001p0, 0x1.00000000000001p0) == pi/4",  FUNC(atan2) (0x1.00000000000001p0L, 0x1.00000000000001p0L), M_PI_4l, 0, 0, 0);
#endif

  print_max_error ("atan2", DELTAatan2, 0);
}

static void
cabs_test (void)
{
  errno = 0;
  FUNC(cabs) (BUILD_COMPLEX (0.7L, 12.4L));
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  /* cabs (x + iy) is specified as hypot (x,y) */

  /* cabs (+inf + i x) == +inf.  */
  check_float ("cabs (inf + 1.0 i) == inf",  FUNC(cabs) (BUILD_COMPLEX (plus_infty, 1.0)), plus_infty, 0, 0, 0);
  /* cabs (-inf + i x) == +inf.  */
  check_float ("cabs (-inf + 1.0 i) == inf",  FUNC(cabs) (BUILD_COMPLEX (minus_infty, 1.0)), plus_infty, 0, 0, 0);

  check_float ("cabs (-inf + NaN i) == inf",  FUNC(cabs) (BUILD_COMPLEX (minus_infty, nan_value)), plus_infty, 0, 0, 0);
  check_float ("cabs (-inf + NaN i) == inf",  FUNC(cabs) (BUILD_COMPLEX (minus_infty, nan_value)), plus_infty, 0, 0, 0);

  check_float ("cabs (NaN + NaN i) == NaN",  FUNC(cabs) (BUILD_COMPLEX (nan_value, nan_value)), nan_value, 0, 0, 0);

  /* cabs (x,y) == cabs (y,x).  */
  check_float ("cabs (0.75 + 12.390625 i) == 12.4133028598606664302388810868156657",  FUNC(cabs) (BUILD_COMPLEX (0.75L, 12.390625L)), 12.4133028598606664302388810868156657L, 0, 0, 0);
  /* cabs (x,y) == cabs (-x,y).  */
  check_float ("cabs (-12.390625 + 0.75 i) == 12.4133028598606664302388810868156657",  FUNC(cabs) (BUILD_COMPLEX (-12.390625L, 0.75L)), 12.4133028598606664302388810868156657L, 0, 0, 0);
  /* cabs (x,y) == cabs (-y,x).  */
  check_float ("cabs (-0.75 + 12.390625 i) == 12.4133028598606664302388810868156657",  FUNC(cabs) (BUILD_COMPLEX (-0.75L, 12.390625L)), 12.4133028598606664302388810868156657L, 0, 0, 0);
  /* cabs (x,y) == cabs (-x,-y).  */
  check_float ("cabs (-12.390625 - 0.75 i) == 12.4133028598606664302388810868156657",  FUNC(cabs) (BUILD_COMPLEX (-12.390625L, -0.75L)), 12.4133028598606664302388810868156657L, 0, 0, 0);
  /* cabs (x,y) == cabs (-y,-x).  */
  check_float ("cabs (-0.75 - 12.390625 i) == 12.4133028598606664302388810868156657",  FUNC(cabs) (BUILD_COMPLEX (-0.75L, -12.390625L)), 12.4133028598606664302388810868156657L, 0, 0, 0);
  /* cabs (x,0) == fabs (x).  */
  check_float ("cabs (-0.75 + 0 i) == 0.75",  FUNC(cabs) (BUILD_COMPLEX (-0.75L, 0)), 0.75L, 0, 0, 0);
  check_float ("cabs (0.75 + 0 i) == 0.75",  FUNC(cabs) (BUILD_COMPLEX (0.75L, 0)), 0.75L, 0, 0, 0);
  check_float ("cabs (-1.0 + 0 i) == 1.0",  FUNC(cabs) (BUILD_COMPLEX (-1.0L, 0)), 1.0L, 0, 0, 0);
  check_float ("cabs (1.0 + 0 i) == 1.0",  FUNC(cabs) (BUILD_COMPLEX (1.0L, 0)), 1.0L, 0, 0, 0);
  check_float ("cabs (-5.7e7 + 0 i) == 5.7e7",  FUNC(cabs) (BUILD_COMPLEX (-5.7e7L, 0)), 5.7e7L, 0, 0, 0);
  check_float ("cabs (5.7e7 + 0 i) == 5.7e7",  FUNC(cabs) (BUILD_COMPLEX (5.7e7L, 0)), 5.7e7L, 0, 0, 0);

  check_float ("cabs (0.75 + 1.25 i) == 1.45773797371132511771853821938639577",  FUNC(cabs) (BUILD_COMPLEX (0.75L, 1.25L)), 1.45773797371132511771853821938639577L, 0, 0, 0);

  print_max_error ("cabs", 0, 0);
}


static void
cacos_test (void)
{
  errno = 0;
  FUNC(cacos) (BUILD_COMPLEX (0.7L, 1.2L));
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();


  check_complex ("cacos (0 + 0 i) == pi/2 - 0 i",  FUNC(cacos) (BUILD_COMPLEX (0, 0)), BUILD_COMPLEX (M_PI_2l, minus_zero), 0, 0, 0);
  check_complex ("cacos (-0 + 0 i) == pi/2 - 0 i",  FUNC(cacos) (BUILD_COMPLEX (minus_zero, 0)), BUILD_COMPLEX (M_PI_2l, minus_zero), 0, 0, 0);
  check_complex ("cacos (-0 - 0 i) == pi/2 + 0.0 i",  FUNC(cacos) (BUILD_COMPLEX (minus_zero, minus_zero)), BUILD_COMPLEX (M_PI_2l, 0.0), 0, 0, 0);
  check_complex ("cacos (0 - 0 i) == pi/2 + 0.0 i",  FUNC(cacos) (BUILD_COMPLEX (0, minus_zero)), BUILD_COMPLEX (M_PI_2l, 0.0), 0, 0, 0);

  check_complex ("cacos (-inf + inf i) == 3/4 pi - inf i",  FUNC(cacos) (BUILD_COMPLEX (minus_infty, plus_infty)), BUILD_COMPLEX (M_PI_34l, minus_infty), 0, 0, 0);
  check_complex ("cacos (-inf - inf i) == 3/4 pi + inf i",  FUNC(cacos) (BUILD_COMPLEX (minus_infty, minus_infty)), BUILD_COMPLEX (M_PI_34l, plus_infty), 0, 0, 0);

  check_complex ("cacos (inf + inf i) == pi/4 - inf i",  FUNC(cacos) (BUILD_COMPLEX (plus_infty, plus_infty)), BUILD_COMPLEX (M_PI_4l, minus_infty), 0, 0, 0);
  check_complex ("cacos (inf - inf i) == pi/4 + inf i",  FUNC(cacos) (BUILD_COMPLEX (plus_infty, minus_infty)), BUILD_COMPLEX (M_PI_4l, plus_infty), 0, 0, 0);

  check_complex ("cacos (-10.0 + inf i) == pi/2 - inf i",  FUNC(cacos) (BUILD_COMPLEX (-10.0, plus_infty)), BUILD_COMPLEX (M_PI_2l, minus_infty), 0, 0, 0);
  check_complex ("cacos (-10.0 - inf i) == pi/2 + inf i",  FUNC(cacos) (BUILD_COMPLEX (-10.0, minus_infty)), BUILD_COMPLEX (M_PI_2l, plus_infty), 0, 0, 0);
  check_complex ("cacos (0 + inf i) == pi/2 - inf i",  FUNC(cacos) (BUILD_COMPLEX (0, plus_infty)), BUILD_COMPLEX (M_PI_2l, minus_infty), 0, 0, 0);
  check_complex ("cacos (0 - inf i) == pi/2 + inf i",  FUNC(cacos) (BUILD_COMPLEX (0, minus_infty)), BUILD_COMPLEX (M_PI_2l, plus_infty), 0, 0, 0);
  check_complex ("cacos (0.1 + inf i) == pi/2 - inf i",  FUNC(cacos) (BUILD_COMPLEX (0.1L, plus_infty)), BUILD_COMPLEX (M_PI_2l, minus_infty), 0, 0, 0);
  check_complex ("cacos (0.1 - inf i) == pi/2 + inf i",  FUNC(cacos) (BUILD_COMPLEX (0.1L, minus_infty)), BUILD_COMPLEX (M_PI_2l, plus_infty), 0, 0, 0);

  check_complex ("cacos (-inf + 0 i) == pi - inf i",  FUNC(cacos) (BUILD_COMPLEX (minus_infty, 0)), BUILD_COMPLEX (M_PIl, minus_infty), 0, 0, 0);
  check_complex ("cacos (-inf - 0 i) == pi + inf i",  FUNC(cacos) (BUILD_COMPLEX (minus_infty, minus_zero)), BUILD_COMPLEX (M_PIl, plus_infty), 0, 0, 0);
  check_complex ("cacos (-inf + 100 i) == pi - inf i",  FUNC(cacos) (BUILD_COMPLEX (minus_infty, 100)), BUILD_COMPLEX (M_PIl, minus_infty), 0, 0, 0);
  check_complex ("cacos (-inf - 100 i) == pi + inf i",  FUNC(cacos) (BUILD_COMPLEX (minus_infty, -100)), BUILD_COMPLEX (M_PIl, plus_infty), 0, 0, 0);

  check_complex ("cacos (inf + 0 i) == 0.0 - inf i",  FUNC(cacos) (BUILD_COMPLEX (plus_infty, 0)), BUILD_COMPLEX (0.0, minus_infty), 0, 0, 0);
  check_complex ("cacos (inf - 0 i) == 0.0 + inf i",  FUNC(cacos) (BUILD_COMPLEX (plus_infty, minus_zero)), BUILD_COMPLEX (0.0, plus_infty), 0, 0, 0);
  check_complex ("cacos (inf + 0.5 i) == 0.0 - inf i",  FUNC(cacos) (BUILD_COMPLEX (plus_infty, 0.5)), BUILD_COMPLEX (0.0, minus_infty), 0, 0, 0);
  check_complex ("cacos (inf - 0.5 i) == 0.0 + inf i",  FUNC(cacos) (BUILD_COMPLEX (plus_infty, -0.5)), BUILD_COMPLEX (0.0, plus_infty), 0, 0, 0);

  check_complex ("cacos (inf + NaN i) == NaN + inf i",  FUNC(cacos) (BUILD_COMPLEX (plus_infty, nan_value)), BUILD_COMPLEX (nan_value, plus_infty), 0, 0, IGNORE_ZERO_INF_SIGN);
  check_complex ("cacos (-inf + NaN i) == NaN + inf i",  FUNC(cacos) (BUILD_COMPLEX (minus_infty, nan_value)), BUILD_COMPLEX (nan_value, plus_infty), 0, 0, IGNORE_ZERO_INF_SIGN);

  check_complex ("cacos (0 + NaN i) == pi/2 + NaN i",  FUNC(cacos) (BUILD_COMPLEX (0, nan_value)), BUILD_COMPLEX (M_PI_2l, nan_value), 0, 0, 0);
  check_complex ("cacos (-0 + NaN i) == pi/2 + NaN i",  FUNC(cacos) (BUILD_COMPLEX (minus_zero, nan_value)), BUILD_COMPLEX (M_PI_2l, nan_value), 0, 0, 0);

  check_complex ("cacos (NaN + inf i) == NaN - inf i",  FUNC(cacos) (BUILD_COMPLEX (nan_value, plus_infty)), BUILD_COMPLEX (nan_value, minus_infty), 0, 0, 0);
  check_complex ("cacos (NaN - inf i) == NaN + inf i",  FUNC(cacos) (BUILD_COMPLEX (nan_value, minus_infty)), BUILD_COMPLEX (nan_value, plus_infty), 0, 0, 0);

  check_complex ("cacos (10.5 + NaN i) == NaN + NaN i",  FUNC(cacos) (BUILD_COMPLEX (10.5, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("cacos (-10.5 + NaN i) == NaN + NaN i",  FUNC(cacos) (BUILD_COMPLEX (-10.5, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);

  check_complex ("cacos (NaN + 0.75 i) == NaN + NaN i",  FUNC(cacos) (BUILD_COMPLEX (nan_value, 0.75)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("cacos (NaN - 0.75 i) == NaN + NaN i",  FUNC(cacos) (BUILD_COMPLEX (nan_value, -0.75)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);

  check_complex ("cacos (NaN + NaN i) == NaN + NaN i",  FUNC(cacos) (BUILD_COMPLEX (nan_value, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, 0);

  check_complex ("cacos (+0 - 1.5 i) == pi/2 + 1.194763217287109304111930828519090523536 i",  FUNC(cacos) (BUILD_COMPLEX (plus_zero, -1.5L)), BUILD_COMPLEX (M_PI_2l, 1.194763217287109304111930828519090523536L), DELTA213, 0, 0);
  check_complex ("cacos (-0 - 1.5 i) == pi/2 + 1.194763217287109304111930828519090523536 i",  FUNC(cacos) (BUILD_COMPLEX (minus_zero, -1.5L)), BUILD_COMPLEX (M_PI_2l, 1.194763217287109304111930828519090523536L), DELTA214, 0, 0);
  check_complex ("cacos (+0 - 1.0 i) == pi/2 + 0.8813735870195430252326093249797923090282 i",  FUNC(cacos) (BUILD_COMPLEX (plus_zero, -1.0L)), BUILD_COMPLEX (M_PI_2l, 0.8813735870195430252326093249797923090282L), DELTA215, 0, 0);
  check_complex ("cacos (-0 - 1.0 i) == pi/2 + 0.8813735870195430252326093249797923090282 i",  FUNC(cacos) (BUILD_COMPLEX (minus_zero, -1.0L)), BUILD_COMPLEX (M_PI_2l, 0.8813735870195430252326093249797923090282L), DELTA216, 0, 0);
  check_complex ("cacos (+0 - 0.5 i) == pi/2 + 0.4812118250596034474977589134243684231352 i",  FUNC(cacos) (BUILD_COMPLEX (plus_zero, -0.5L)), BUILD_COMPLEX (M_PI_2l, 0.4812118250596034474977589134243684231352L), DELTA217, 0, 0);
  check_complex ("cacos (-0 - 0.5 i) == pi/2 + 0.4812118250596034474977589134243684231352 i",  FUNC(cacos) (BUILD_COMPLEX (minus_zero, -0.5L)), BUILD_COMPLEX (M_PI_2l, 0.4812118250596034474977589134243684231352L), DELTA218, 0, 0);
  check_complex ("cacos (+0 + 0.5 i) == pi/2 - 0.4812118250596034474977589134243684231352 i",  FUNC(cacos) (BUILD_COMPLEX (plus_zero, 0.5L)), BUILD_COMPLEX (M_PI_2l, -0.4812118250596034474977589134243684231352L), DELTA219, 0, 0);
  check_complex ("cacos (-0 + 0.5 i) == pi/2 - 0.4812118250596034474977589134243684231352 i",  FUNC(cacos) (BUILD_COMPLEX (minus_zero, 0.5L)), BUILD_COMPLEX (M_PI_2l, -0.4812118250596034474977589134243684231352L), DELTA220, 0, 0);
  check_complex ("cacos (+0 + 1.0 i) == pi/2 - 0.8813735870195430252326093249797923090282 i",  FUNC(cacos) (BUILD_COMPLEX (plus_zero, 1.0L)), BUILD_COMPLEX (M_PI_2l, -0.8813735870195430252326093249797923090282L), DELTA221, 0, 0);
  check_complex ("cacos (-0 + 1.0 i) == pi/2 - 0.8813735870195430252326093249797923090282 i",  FUNC(cacos) (BUILD_COMPLEX (minus_zero, 1.0L)), BUILD_COMPLEX (M_PI_2l, -0.8813735870195430252326093249797923090282L), DELTA222, 0, 0);
  check_complex ("cacos (+0 + 1.5 i) == pi/2 - 1.194763217287109304111930828519090523536 i",  FUNC(cacos) (BUILD_COMPLEX (plus_zero, 1.5L)), BUILD_COMPLEX (M_PI_2l, -1.194763217287109304111930828519090523536L), DELTA223, 0, 0);
  check_complex ("cacos (-0 + 1.5 i) == pi/2 - 1.194763217287109304111930828519090523536 i",  FUNC(cacos) (BUILD_COMPLEX (minus_zero, 1.5L)), BUILD_COMPLEX (M_PI_2l, -1.194763217287109304111930828519090523536L), DELTA224, 0, 0);

  check_complex ("cacos (-1.5 + +0 i) == pi - 0.9624236501192068949955178268487368462704 i",  FUNC(cacos) (BUILD_COMPLEX (-1.5L, plus_zero)), BUILD_COMPLEX (M_PIl, -0.9624236501192068949955178268487368462704L), DELTA225, 0, 0);
  check_complex ("cacos (-1.5 - 0 i) == pi + 0.9624236501192068949955178268487368462704 i",  FUNC(cacos) (BUILD_COMPLEX (-1.5L, minus_zero)), BUILD_COMPLEX (M_PIl, 0.9624236501192068949955178268487368462704L), DELTA226, 0, 0);
  check_complex ("cacos (-1.0 + +0 i) == pi - 0 i",  FUNC(cacos) (BUILD_COMPLEX (-1.0L, plus_zero)), BUILD_COMPLEX (M_PIl, minus_zero), 0, 0, 0);
  check_complex ("cacos (-1.0 - 0 i) == pi + +0 i",  FUNC(cacos) (BUILD_COMPLEX (-1.0L, minus_zero)), BUILD_COMPLEX (M_PIl, plus_zero), 0, 0, 0);
  check_complex ("cacos (-0.5 + +0 i) == 2.094395102393195492308428922186335256131 - 0 i",  FUNC(cacos) (BUILD_COMPLEX (-0.5L, plus_zero)), BUILD_COMPLEX (2.094395102393195492308428922186335256131L, minus_zero), 0, 0, 0);
  check_complex ("cacos (-0.5 - 0 i) == 2.094395102393195492308428922186335256131 + +0 i",  FUNC(cacos) (BUILD_COMPLEX (-0.5L, minus_zero)), BUILD_COMPLEX (2.094395102393195492308428922186335256131L, plus_zero), 0, 0, 0);
  check_complex ("cacos (0.5 + +0 i) == 1.047197551196597746154214461093167628066 - 0 i",  FUNC(cacos) (BUILD_COMPLEX (0.5L, plus_zero)), BUILD_COMPLEX (1.047197551196597746154214461093167628066L, minus_zero), DELTA231, 0, 0);
  check_complex ("cacos (0.5 - 0 i) == 1.047197551196597746154214461093167628066 + +0 i",  FUNC(cacos) (BUILD_COMPLEX (0.5L, minus_zero)), BUILD_COMPLEX (1.047197551196597746154214461093167628066L, plus_zero), DELTA232, 0, 0);
  check_complex ("cacos (1.0 + +0 i) == +0 - 0 i",  FUNC(cacos) (BUILD_COMPLEX (1.0L, plus_zero)), BUILD_COMPLEX (plus_zero, minus_zero), 0, 0, 0);
  check_complex ("cacos (1.0 - 0 i) == +0 + +0 i",  FUNC(cacos) (BUILD_COMPLEX (1.0L, minus_zero)), BUILD_COMPLEX (plus_zero, plus_zero), 0, 0, 0);
  check_complex ("cacos (1.5 + +0 i) == +0 - 0.9624236501192068949955178268487368462704 i",  FUNC(cacos) (BUILD_COMPLEX (1.5L, plus_zero)), BUILD_COMPLEX (plus_zero, -0.9624236501192068949955178268487368462704L), DELTA235, 0, 0);
  check_complex ("cacos (1.5 - 0 i) == +0 + 0.9624236501192068949955178268487368462704 i",  FUNC(cacos) (BUILD_COMPLEX (1.5L, minus_zero)), BUILD_COMPLEX (plus_zero, 0.9624236501192068949955178268487368462704L), DELTA236, 0, 0);

  check_complex ("cacos (0.75 + 1.25 i) == 1.11752014915610270578240049553777969 - 1.13239363160530819522266333696834467 i",  FUNC(cacos) (BUILD_COMPLEX (0.75L, 1.25L)), BUILD_COMPLEX (1.11752014915610270578240049553777969L, -1.13239363160530819522266333696834467L), DELTA237, 0, 0);
  check_complex ("cacos (-2 - 3 i) == 2.1414491111159960199416055713254211 + 1.9833870299165354323470769028940395 i",  FUNC(cacos) (BUILD_COMPLEX (-2, -3)), BUILD_COMPLEX (2.1414491111159960199416055713254211L, 1.9833870299165354323470769028940395L), 0, 0, 0);

  print_complex_max_error ("cacos", DELTAcacos, 0);
}

static void
cacosh_test (void)
{
  errno = 0;
  FUNC(cacosh) (BUILD_COMPLEX (0.7L, 1.2L));
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();


  check_complex ("cacosh (0 + 0 i) == 0.0 + pi/2 i",  FUNC(cacosh) (BUILD_COMPLEX (0, 0)), BUILD_COMPLEX (0.0, M_PI_2l), 0, 0, 0);
  check_complex ("cacosh (-0 + 0 i) == 0.0 + pi/2 i",  FUNC(cacosh) (BUILD_COMPLEX (minus_zero, 0)), BUILD_COMPLEX (0.0, M_PI_2l), 0, 0, 0);
  check_complex ("cacosh (0 - 0 i) == 0.0 - pi/2 i",  FUNC(cacosh) (BUILD_COMPLEX (0, minus_zero)), BUILD_COMPLEX (0.0, -M_PI_2l), 0, 0, 0);
  check_complex ("cacosh (-0 - 0 i) == 0.0 - pi/2 i",  FUNC(cacosh) (BUILD_COMPLEX (minus_zero, minus_zero)), BUILD_COMPLEX (0.0, -M_PI_2l), 0, 0, 0);
  check_complex ("cacosh (-inf + inf i) == inf + 3/4 pi i",  FUNC(cacosh) (BUILD_COMPLEX (minus_infty, plus_infty)), BUILD_COMPLEX (plus_infty, M_PI_34l), 0, 0, 0);
  check_complex ("cacosh (-inf - inf i) == inf - 3/4 pi i",  FUNC(cacosh) (BUILD_COMPLEX (minus_infty, minus_infty)), BUILD_COMPLEX (plus_infty, -M_PI_34l), 0, 0, 0);

  check_complex ("cacosh (inf + inf i) == inf + pi/4 i",  FUNC(cacosh) (BUILD_COMPLEX (plus_infty, plus_infty)), BUILD_COMPLEX (plus_infty, M_PI_4l), 0, 0, 0);
  check_complex ("cacosh (inf - inf i) == inf - pi/4 i",  FUNC(cacosh) (BUILD_COMPLEX (plus_infty, minus_infty)), BUILD_COMPLEX (plus_infty, -M_PI_4l), 0, 0, 0);

  check_complex ("cacosh (-10.0 + inf i) == inf + pi/2 i",  FUNC(cacosh) (BUILD_COMPLEX (-10.0, plus_infty)), BUILD_COMPLEX (plus_infty, M_PI_2l), 0, 0, 0);
  check_complex ("cacosh (-10.0 - inf i) == inf - pi/2 i",  FUNC(cacosh) (BUILD_COMPLEX (-10.0, minus_infty)), BUILD_COMPLEX (plus_infty, -M_PI_2l), 0, 0, 0);
  check_complex ("cacosh (0 + inf i) == inf + pi/2 i",  FUNC(cacosh) (BUILD_COMPLEX (0, plus_infty)), BUILD_COMPLEX (plus_infty, M_PI_2l), 0, 0, 0);
  check_complex ("cacosh (0 - inf i) == inf - pi/2 i",  FUNC(cacosh) (BUILD_COMPLEX (0, minus_infty)), BUILD_COMPLEX (plus_infty, -M_PI_2l), 0, 0, 0);
  check_complex ("cacosh (0.1 + inf i) == inf + pi/2 i",  FUNC(cacosh) (BUILD_COMPLEX (0.1L, plus_infty)), BUILD_COMPLEX (plus_infty, M_PI_2l), 0, 0, 0);
  check_complex ("cacosh (0.1 - inf i) == inf - pi/2 i",  FUNC(cacosh) (BUILD_COMPLEX (0.1L, minus_infty)), BUILD_COMPLEX (plus_infty, -M_PI_2l), 0, 0, 0);

  check_complex ("cacosh (-inf + 0 i) == inf + pi i",  FUNC(cacosh) (BUILD_COMPLEX (minus_infty, 0)), BUILD_COMPLEX (plus_infty, M_PIl), 0, 0, 0);
  check_complex ("cacosh (-inf - 0 i) == inf - pi i",  FUNC(cacosh) (BUILD_COMPLEX (minus_infty, minus_zero)), BUILD_COMPLEX (plus_infty, -M_PIl), 0, 0, 0);
  check_complex ("cacosh (-inf + 100 i) == inf + pi i",  FUNC(cacosh) (BUILD_COMPLEX (minus_infty, 100)), BUILD_COMPLEX (plus_infty, M_PIl), 0, 0, 0);
  check_complex ("cacosh (-inf - 100 i) == inf - pi i",  FUNC(cacosh) (BUILD_COMPLEX (minus_infty, -100)), BUILD_COMPLEX (plus_infty, -M_PIl), 0, 0, 0);

  check_complex ("cacosh (inf + 0 i) == inf + 0.0 i",  FUNC(cacosh) (BUILD_COMPLEX (plus_infty, 0)), BUILD_COMPLEX (plus_infty, 0.0), 0, 0, 0);
  check_complex ("cacosh (inf - 0 i) == inf - 0 i",  FUNC(cacosh) (BUILD_COMPLEX (plus_infty, minus_zero)), BUILD_COMPLEX (plus_infty, minus_zero), 0, 0, 0);
  check_complex ("cacosh (inf + 0.5 i) == inf + 0.0 i",  FUNC(cacosh) (BUILD_COMPLEX (plus_infty, 0.5)), BUILD_COMPLEX (plus_infty, 0.0), 0, 0, 0);
  check_complex ("cacosh (inf - 0.5 i) == inf - 0 i",  FUNC(cacosh) (BUILD_COMPLEX (plus_infty, -0.5)), BUILD_COMPLEX (plus_infty, minus_zero), 0, 0, 0);

  check_complex ("cacosh (inf + NaN i) == inf + NaN i",  FUNC(cacosh) (BUILD_COMPLEX (plus_infty, nan_value)), BUILD_COMPLEX (plus_infty, nan_value), 0, 0, 0);
  check_complex ("cacosh (-inf + NaN i) == inf + NaN i",  FUNC(cacosh) (BUILD_COMPLEX (minus_infty, nan_value)), BUILD_COMPLEX (plus_infty, nan_value), 0, 0, 0);

  check_complex ("cacosh (0 + NaN i) == NaN + NaN i",  FUNC(cacosh) (BUILD_COMPLEX (0, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, 0);
  check_complex ("cacosh (-0 + NaN i) == NaN + NaN i",  FUNC(cacosh) (BUILD_COMPLEX (minus_zero, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, 0);

  check_complex ("cacosh (NaN + inf i) == inf + NaN i",  FUNC(cacosh) (BUILD_COMPLEX (nan_value, plus_infty)), BUILD_COMPLEX (plus_infty, nan_value), 0, 0, 0);
  check_complex ("cacosh (NaN - inf i) == inf + NaN i",  FUNC(cacosh) (BUILD_COMPLEX (nan_value, minus_infty)), BUILD_COMPLEX (plus_infty, nan_value), 0, 0, 0);

  check_complex ("cacosh (10.5 + NaN i) == NaN + NaN i",  FUNC(cacosh) (BUILD_COMPLEX (10.5, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("cacosh (-10.5 + NaN i) == NaN + NaN i",  FUNC(cacosh) (BUILD_COMPLEX (-10.5, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);

  check_complex ("cacosh (NaN + 0.75 i) == NaN + NaN i",  FUNC(cacosh) (BUILD_COMPLEX (nan_value, 0.75)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("cacosh (NaN - 0.75 i) == NaN + NaN i",  FUNC(cacosh) (BUILD_COMPLEX (nan_value, -0.75)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);

  check_complex ("cacosh (NaN + NaN i) == NaN + NaN i",  FUNC(cacosh) (BUILD_COMPLEX (nan_value, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, 0);

  check_complex ("cacosh (+0 - 1.5 i) == 1.194763217287109304111930828519090523536 - pi/2 i",  FUNC(cacosh) (BUILD_COMPLEX (plus_zero, -1.5L)), BUILD_COMPLEX (1.194763217287109304111930828519090523536L, -M_PI_2l), DELTA272, 0, 0);
  check_complex ("cacosh (-0 - 1.5 i) == 1.194763217287109304111930828519090523536 - pi/2 i",  FUNC(cacosh) (BUILD_COMPLEX (minus_zero, -1.5L)), BUILD_COMPLEX (1.194763217287109304111930828519090523536L, -M_PI_2l), DELTA273, 0, 0);
  check_complex ("cacosh (+0 - 1.0 i) == 0.8813735870195430252326093249797923090282 - pi/2 i",  FUNC(cacosh) (BUILD_COMPLEX (plus_zero, -1.0L)), BUILD_COMPLEX (0.8813735870195430252326093249797923090282L, -M_PI_2l), DELTA274, 0, 0);
  check_complex ("cacosh (-0 - 1.0 i) == 0.8813735870195430252326093249797923090282 - pi/2 i",  FUNC(cacosh) (BUILD_COMPLEX (minus_zero, -1.0L)), BUILD_COMPLEX (0.8813735870195430252326093249797923090282L, -M_PI_2l), DELTA275, 0, 0);
  check_complex ("cacosh (+0 - 0.5 i) == 0.4812118250596034474977589134243684231352 - pi/2 i",  FUNC(cacosh) (BUILD_COMPLEX (plus_zero, -0.5L)), BUILD_COMPLEX (0.4812118250596034474977589134243684231352L, -M_PI_2l), DELTA276, 0, 0);
  check_complex ("cacosh (-0 - 0.5 i) == 0.4812118250596034474977589134243684231352 - pi/2 i",  FUNC(cacosh) (BUILD_COMPLEX (minus_zero, -0.5L)), BUILD_COMPLEX (0.4812118250596034474977589134243684231352L, -M_PI_2l), DELTA277, 0, 0);
  check_complex ("cacosh (+0 + 0.5 i) == 0.4812118250596034474977589134243684231352 + pi/2 i",  FUNC(cacosh) (BUILD_COMPLEX (plus_zero, 0.5L)), BUILD_COMPLEX (0.4812118250596034474977589134243684231352L, M_PI_2l), DELTA278, 0, 0);
  check_complex ("cacosh (-0 + 0.5 i) == 0.4812118250596034474977589134243684231352 + pi/2 i",  FUNC(cacosh) (BUILD_COMPLEX (minus_zero, 0.5L)), BUILD_COMPLEX (0.4812118250596034474977589134243684231352L, M_PI_2l), DELTA279, 0, 0);
  check_complex ("cacosh (+0 + 1.0 i) == 0.8813735870195430252326093249797923090282 + pi/2 i",  FUNC(cacosh) (BUILD_COMPLEX (plus_zero, 1.0L)), BUILD_COMPLEX (0.8813735870195430252326093249797923090282L, M_PI_2l), DELTA280, 0, 0);
  check_complex ("cacosh (-0 + 1.0 i) == 0.8813735870195430252326093249797923090282 + pi/2 i",  FUNC(cacosh) (BUILD_COMPLEX (minus_zero, 1.0L)), BUILD_COMPLEX (0.8813735870195430252326093249797923090282L, M_PI_2l), DELTA281, 0, 0);
  check_complex ("cacosh (+0 + 1.5 i) == 1.194763217287109304111930828519090523536 + pi/2 i",  FUNC(cacosh) (BUILD_COMPLEX (plus_zero, 1.5L)), BUILD_COMPLEX (1.194763217287109304111930828519090523536L, M_PI_2l), DELTA282, 0, 0);
  check_complex ("cacosh (-0 + 1.5 i) == 1.194763217287109304111930828519090523536 + pi/2 i",  FUNC(cacosh) (BUILD_COMPLEX (minus_zero, 1.5L)), BUILD_COMPLEX (1.194763217287109304111930828519090523536L, M_PI_2l), DELTA283, 0, 0);

  check_complex ("cacosh (-1.5 + +0 i) == 0.9624236501192068949955178268487368462704 + pi i",  FUNC(cacosh) (BUILD_COMPLEX (-1.5L, plus_zero)), BUILD_COMPLEX (0.9624236501192068949955178268487368462704L, M_PIl), DELTA284, 0, 0);
  check_complex ("cacosh (-1.5 - 0 i) == 0.9624236501192068949955178268487368462704 - pi i",  FUNC(cacosh) (BUILD_COMPLEX (-1.5L, minus_zero)), BUILD_COMPLEX (0.9624236501192068949955178268487368462704L, -M_PIl), DELTA285, 0, 0);
  check_complex ("cacosh (-1.0 + +0 i) == +0 + pi i",  FUNC(cacosh) (BUILD_COMPLEX (-1.0L, plus_zero)), BUILD_COMPLEX (plus_zero, M_PIl), 0, 0, 0);
  check_complex ("cacosh (-1.0 - 0 i) == +0 - pi i",  FUNC(cacosh) (BUILD_COMPLEX (-1.0L, minus_zero)), BUILD_COMPLEX (plus_zero, -M_PIl), 0, 0, 0);
  check_complex ("cacosh (-0.5 + +0 i) == +0 + 2.094395102393195492308428922186335256131 i",  FUNC(cacosh) (BUILD_COMPLEX (-0.5L, plus_zero)), BUILD_COMPLEX (plus_zero, 2.094395102393195492308428922186335256131L), DELTA288, 0, 0);
  check_complex ("cacosh (-0.5 - 0 i) == +0 - 2.094395102393195492308428922186335256131 i",  FUNC(cacosh) (BUILD_COMPLEX (-0.5L, minus_zero)), BUILD_COMPLEX (plus_zero, -2.094395102393195492308428922186335256131L), DELTA289, 0, 0);
  check_complex ("cacosh (0.5 + +0 i) == +0 + 1.047197551196597746154214461093167628066 i",  FUNC(cacosh) (BUILD_COMPLEX (0.5L, plus_zero)), BUILD_COMPLEX (plus_zero, 1.047197551196597746154214461093167628066L), 0, 0, 0);
  check_complex ("cacosh (0.5 - 0 i) == +0 - 1.047197551196597746154214461093167628066 i",  FUNC(cacosh) (BUILD_COMPLEX (0.5L, minus_zero)), BUILD_COMPLEX (plus_zero, -1.047197551196597746154214461093167628066L), 0, 0, 0);
  check_complex ("cacosh (1.0 + +0 i) == +0 + +0 i",  FUNC(cacosh) (BUILD_COMPLEX (1.0L, plus_zero)), BUILD_COMPLEX (plus_zero, plus_zero), 0, 0, 0);
  check_complex ("cacosh (1.0 - 0 i) == +0 - 0 i",  FUNC(cacosh) (BUILD_COMPLEX (1.0L, minus_zero)), BUILD_COMPLEX (plus_zero, minus_zero), 0, 0, 0);
  check_complex ("cacosh (1.5 + +0 i) == 0.9624236501192068949955178268487368462704 + +0 i",  FUNC(cacosh) (BUILD_COMPLEX (1.5L, plus_zero)), BUILD_COMPLEX (0.9624236501192068949955178268487368462704L, plus_zero), DELTA294, 0, 0);
  check_complex ("cacosh (1.5 - 0 i) == 0.9624236501192068949955178268487368462704 - 0 i",  FUNC(cacosh) (BUILD_COMPLEX (1.5L, minus_zero)), BUILD_COMPLEX (0.9624236501192068949955178268487368462704L, minus_zero), DELTA295, 0, 0);

  check_complex ("cacosh (0.75 + 1.25 i) == 1.13239363160530819522266333696834467 + 1.11752014915610270578240049553777969 i",  FUNC(cacosh) (BUILD_COMPLEX (0.75L, 1.25L)), BUILD_COMPLEX (1.13239363160530819522266333696834467L, 1.11752014915610270578240049553777969L), DELTA296, 0, 0);
  check_complex ("cacosh (-2 - 3 i) == 1.9833870299165354323470769028940395 - 2.1414491111159960199416055713254211 i",  FUNC(cacosh) (BUILD_COMPLEX (-2, -3)), BUILD_COMPLEX (1.9833870299165354323470769028940395L, -2.1414491111159960199416055713254211L), DELTA297, 0, 0);

  print_complex_max_error ("cacosh", DELTAcacosh, 0);
}


static void
carg_test (void)
{
  init_max_error ();

  /* carg (x + iy) is specified as atan2 (y, x) */

  /* carg (x + i 0) == 0 for x > 0.  */
  check_float ("carg (2.0 + 0 i) == 0",  FUNC(carg) (BUILD_COMPLEX (2.0, 0)), 0, 0, 0, 0);
  /* carg (x - i 0) == -0 for x > 0.  */
  check_float ("carg (2.0 - 0 i) == -0",  FUNC(carg) (BUILD_COMPLEX (2.0, minus_zero)), minus_zero, 0, 0, 0);

  check_float ("carg (0 + 0 i) == 0",  FUNC(carg) (BUILD_COMPLEX (0, 0)), 0, 0, 0, 0);
  check_float ("carg (0 - 0 i) == -0",  FUNC(carg) (BUILD_COMPLEX (0, minus_zero)), minus_zero, 0, 0, 0);

  /* carg (x + i 0) == +pi for x < 0.  */
  check_float ("carg (-2.0 + 0 i) == pi",  FUNC(carg) (BUILD_COMPLEX (-2.0, 0)), M_PIl, 0, 0, 0);

  /* carg (x - i 0) == -pi for x < 0.  */
  check_float ("carg (-2.0 - 0 i) == -pi",  FUNC(carg) (BUILD_COMPLEX (-2.0, minus_zero)), -M_PIl, 0, 0, 0);

  check_float ("carg (-0 + 0 i) == pi",  FUNC(carg) (BUILD_COMPLEX (minus_zero, 0)), M_PIl, 0, 0, 0);
  check_float ("carg (-0 - 0 i) == -pi",  FUNC(carg) (BUILD_COMPLEX (minus_zero, minus_zero)), -M_PIl, 0, 0, 0);

  /* carg (+0 + i y) == pi/2 for y > 0.  */
  check_float ("carg (0 + 2.0 i) == pi/2",  FUNC(carg) (BUILD_COMPLEX (0, 2.0)), M_PI_2l, 0, 0, 0);

  /* carg (-0 + i y) == pi/2 for y > 0.  */
  check_float ("carg (-0 + 2.0 i) == pi/2",  FUNC(carg) (BUILD_COMPLEX (minus_zero, 2.0)), M_PI_2l, 0, 0, 0);

  /* carg (+0 + i y) == -pi/2 for y < 0.  */
  check_float ("carg (0 - 2.0 i) == -pi/2",  FUNC(carg) (BUILD_COMPLEX (0, -2.0)), -M_PI_2l, 0, 0, 0);

  /* carg (-0 + i y) == -pi/2 for y < 0.  */
  check_float ("carg (-0 - 2.0 i) == -pi/2",  FUNC(carg) (BUILD_COMPLEX (minus_zero, -2.0)), -M_PI_2l, 0, 0, 0);

  /* carg (inf + i y) == +0 for finite y > 0.  */
  check_float ("carg (inf + 2.0 i) == 0",  FUNC(carg) (BUILD_COMPLEX (plus_infty, 2.0)), 0, 0, 0, 0);

  /* carg (inf + i y) == -0 for finite y < 0.  */
  check_float ("carg (inf - 2.0 i) == -0",  FUNC(carg) (BUILD_COMPLEX (plus_infty, -2.0)), minus_zero, 0, 0, 0);

  /* carg(x + i inf) == pi/2 for finite x.  */
  check_float ("carg (10.0 + inf i) == pi/2",  FUNC(carg) (BUILD_COMPLEX (10.0, plus_infty)), M_PI_2l, 0, 0, 0);

  /* carg(x - i inf) == -pi/2 for finite x.  */
  check_float ("carg (10.0 - inf i) == -pi/2",  FUNC(carg) (BUILD_COMPLEX (10.0, minus_infty)), -M_PI_2l, 0, 0, 0);

  /* carg (-inf + i y) == +pi for finite y > 0.  */
  check_float ("carg (-inf + 10.0 i) == pi",  FUNC(carg) (BUILD_COMPLEX (minus_infty, 10.0)), M_PIl, 0, 0, 0);

  /* carg (-inf + i y) == -pi for finite y < 0.  */
  check_float ("carg (-inf - 10.0 i) == -pi",  FUNC(carg) (BUILD_COMPLEX (minus_infty, -10.0)), -M_PIl, 0, 0, 0);

  check_float ("carg (inf + inf i) == pi/4",  FUNC(carg) (BUILD_COMPLEX (plus_infty, plus_infty)), M_PI_4l, 0, 0, 0);

  check_float ("carg (inf - inf i) == -pi/4",  FUNC(carg) (BUILD_COMPLEX (plus_infty, minus_infty)), -M_PI_4l, 0, 0, 0);

  check_float ("carg (-inf + inf i) == 3 * M_PI_4l",  FUNC(carg) (BUILD_COMPLEX (minus_infty, plus_infty)), 3 * M_PI_4l, 0, 0, 0);

  check_float ("carg (-inf - inf i) == -3 * M_PI_4l",  FUNC(carg) (BUILD_COMPLEX (minus_infty, minus_infty)), -3 * M_PI_4l, 0, 0, 0);

  check_float ("carg (NaN + NaN i) == NaN",  FUNC(carg) (BUILD_COMPLEX (nan_value, nan_value)), nan_value, 0, 0, 0);

  print_max_error ("carg", 0, 0);
}

static void
casin_test (void)
{
  errno = 0;
  FUNC(casin) (BUILD_COMPLEX (0.7L, 1.2L));
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_complex ("casin (0 + 0 i) == 0.0 + 0.0 i",  FUNC(casin) (BUILD_COMPLEX (0, 0)), BUILD_COMPLEX (0.0, 0.0), 0, 0, 0);
  check_complex ("casin (-0 + 0 i) == -0 + 0.0 i",  FUNC(casin) (BUILD_COMPLEX (minus_zero, 0)), BUILD_COMPLEX (minus_zero, 0.0), 0, 0, 0);
  check_complex ("casin (0 - 0 i) == 0.0 - 0 i",  FUNC(casin) (BUILD_COMPLEX (0, minus_zero)), BUILD_COMPLEX (0.0, minus_zero), 0, 0, 0);
  check_complex ("casin (-0 - 0 i) == -0 - 0 i",  FUNC(casin) (BUILD_COMPLEX (minus_zero, minus_zero)), BUILD_COMPLEX (minus_zero, minus_zero), 0, 0, 0);

  check_complex ("casin (inf + inf i) == pi/4 + inf i",  FUNC(casin) (BUILD_COMPLEX (plus_infty, plus_infty)), BUILD_COMPLEX (M_PI_4l, plus_infty), 0, 0, 0);
  check_complex ("casin (inf - inf i) == pi/4 - inf i",  FUNC(casin) (BUILD_COMPLEX (plus_infty, minus_infty)), BUILD_COMPLEX (M_PI_4l, minus_infty), 0, 0, 0);
  check_complex ("casin (-inf + inf i) == -pi/4 + inf i",  FUNC(casin) (BUILD_COMPLEX (minus_infty, plus_infty)), BUILD_COMPLEX (-M_PI_4l, plus_infty), 0, 0, 0);
  check_complex ("casin (-inf - inf i) == -pi/4 - inf i",  FUNC(casin) (BUILD_COMPLEX (minus_infty, minus_infty)), BUILD_COMPLEX (-M_PI_4l, minus_infty), 0, 0, 0);

  check_complex ("casin (-10.0 + inf i) == -0 + inf i",  FUNC(casin) (BUILD_COMPLEX (-10.0, plus_infty)), BUILD_COMPLEX (minus_zero, plus_infty), 0, 0, 0);
  check_complex ("casin (-10.0 - inf i) == -0 - inf i",  FUNC(casin) (BUILD_COMPLEX (-10.0, minus_infty)), BUILD_COMPLEX (minus_zero, minus_infty), 0, 0, 0);
  check_complex ("casin (0 + inf i) == 0.0 + inf i",  FUNC(casin) (BUILD_COMPLEX (0, plus_infty)), BUILD_COMPLEX (0.0, plus_infty), 0, 0, 0);
  check_complex ("casin (0 - inf i) == 0.0 - inf i",  FUNC(casin) (BUILD_COMPLEX (0, minus_infty)), BUILD_COMPLEX (0.0, minus_infty), 0, 0, 0);
  check_complex ("casin (-0 + inf i) == -0 + inf i",  FUNC(casin) (BUILD_COMPLEX (minus_zero, plus_infty)), BUILD_COMPLEX (minus_zero, plus_infty), 0, 0, 0);
  check_complex ("casin (-0 - inf i) == -0 - inf i",  FUNC(casin) (BUILD_COMPLEX (minus_zero, minus_infty)), BUILD_COMPLEX (minus_zero, minus_infty), 0, 0, 0);
  check_complex ("casin (0.1 + inf i) == 0.0 + inf i",  FUNC(casin) (BUILD_COMPLEX (0.1L, plus_infty)), BUILD_COMPLEX (0.0, plus_infty), 0, 0, 0);
  check_complex ("casin (0.1 - inf i) == 0.0 - inf i",  FUNC(casin) (BUILD_COMPLEX (0.1L, minus_infty)), BUILD_COMPLEX (0.0, minus_infty), 0, 0, 0);

  check_complex ("casin (-inf + 0 i) == -pi/2 + inf i",  FUNC(casin) (BUILD_COMPLEX (minus_infty, 0)), BUILD_COMPLEX (-M_PI_2l, plus_infty), 0, 0, 0);
  check_complex ("casin (-inf - 0 i) == -pi/2 - inf i",  FUNC(casin) (BUILD_COMPLEX (minus_infty, minus_zero)), BUILD_COMPLEX (-M_PI_2l, minus_infty), 0, 0, 0);
  check_complex ("casin (-inf + 100 i) == -pi/2 + inf i",  FUNC(casin) (BUILD_COMPLEX (minus_infty, 100)), BUILD_COMPLEX (-M_PI_2l, plus_infty), 0, 0, 0);
  check_complex ("casin (-inf - 100 i) == -pi/2 - inf i",  FUNC(casin) (BUILD_COMPLEX (minus_infty, -100)), BUILD_COMPLEX (-M_PI_2l, minus_infty), 0, 0, 0);

  check_complex ("casin (inf + 0 i) == pi/2 + inf i",  FUNC(casin) (BUILD_COMPLEX (plus_infty, 0)), BUILD_COMPLEX (M_PI_2l, plus_infty), 0, 0, 0);
  check_complex ("casin (inf - 0 i) == pi/2 - inf i",  FUNC(casin) (BUILD_COMPLEX (plus_infty, minus_zero)), BUILD_COMPLEX (M_PI_2l, minus_infty), 0, 0, 0);
  check_complex ("casin (inf + 0.5 i) == pi/2 + inf i",  FUNC(casin) (BUILD_COMPLEX (plus_infty, 0.5)), BUILD_COMPLEX (M_PI_2l, plus_infty), 0, 0, 0);
  check_complex ("casin (inf - 0.5 i) == pi/2 - inf i",  FUNC(casin) (BUILD_COMPLEX (plus_infty, -0.5)), BUILD_COMPLEX (M_PI_2l, minus_infty), 0, 0, 0);

  check_complex ("casin (NaN + inf i) == NaN + inf i",  FUNC(casin) (BUILD_COMPLEX (nan_value, plus_infty)), BUILD_COMPLEX (nan_value, plus_infty), 0, 0, 0);
  check_complex ("casin (NaN - inf i) == NaN - inf i",  FUNC(casin) (BUILD_COMPLEX (nan_value, minus_infty)), BUILD_COMPLEX (nan_value, minus_infty), 0, 0, 0);

  check_complex ("casin (0.0 + NaN i) == 0.0 + NaN i",  FUNC(casin) (BUILD_COMPLEX (0.0, nan_value)), BUILD_COMPLEX (0.0, nan_value), 0, 0, 0);
  check_complex ("casin (-0 + NaN i) == -0 + NaN i",  FUNC(casin) (BUILD_COMPLEX (minus_zero, nan_value)), BUILD_COMPLEX (minus_zero, nan_value), 0, 0, 0);

  check_complex ("casin (inf + NaN i) == NaN + inf i",  FUNC(casin) (BUILD_COMPLEX (plus_infty, nan_value)), BUILD_COMPLEX (nan_value, plus_infty), 0, 0, IGNORE_ZERO_INF_SIGN);
  check_complex ("casin (-inf + NaN i) == NaN + inf i",  FUNC(casin) (BUILD_COMPLEX (minus_infty, nan_value)), BUILD_COMPLEX (nan_value, plus_infty), 0, 0, IGNORE_ZERO_INF_SIGN);

  check_complex ("casin (NaN + 10.5 i) == NaN + NaN i",  FUNC(casin) (BUILD_COMPLEX (nan_value, 10.5)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("casin (NaN - 10.5 i) == NaN + NaN i",  FUNC(casin) (BUILD_COMPLEX (nan_value, -10.5)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);

  check_complex ("casin (0.75 + NaN i) == NaN + NaN i",  FUNC(casin) (BUILD_COMPLEX (0.75, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("casin (-0.75 + NaN i) == NaN + NaN i",  FUNC(casin) (BUILD_COMPLEX (-0.75, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);

  check_complex ("casin (NaN + NaN i) == NaN + NaN i",  FUNC(casin) (BUILD_COMPLEX (nan_value, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, 0);

  check_complex ("casin (+0 - 1.5 i) == +0 - 1.194763217287109304111930828519090523536 i",  FUNC(casin) (BUILD_COMPLEX (plus_zero, -1.5L)), BUILD_COMPLEX (plus_zero, -1.194763217287109304111930828519090523536L), DELTA356, 0, 0);
  check_complex ("casin (-0 - 1.5 i) == -0 - 1.194763217287109304111930828519090523536 i",  FUNC(casin) (BUILD_COMPLEX (minus_zero, -1.5L)), BUILD_COMPLEX (minus_zero, -1.194763217287109304111930828519090523536L), DELTA357, 0, 0);
  check_complex ("casin (+0 - 1.0 i) == +0 - 0.8813735870195430252326093249797923090282 i",  FUNC(casin) (BUILD_COMPLEX (plus_zero, -1.0L)), BUILD_COMPLEX (plus_zero, -0.8813735870195430252326093249797923090282L), DELTA358, 0, 0);
  check_complex ("casin (-0 - 1.0 i) == -0 - 0.8813735870195430252326093249797923090282 i",  FUNC(casin) (BUILD_COMPLEX (minus_zero, -1.0L)), BUILD_COMPLEX (minus_zero, -0.8813735870195430252326093249797923090282L), DELTA359, 0, 0);
  check_complex ("casin (+0 - 0.5 i) == +0 - 0.4812118250596034474977589134243684231352 i",  FUNC(casin) (BUILD_COMPLEX (plus_zero, -0.5L)), BUILD_COMPLEX (plus_zero, -0.4812118250596034474977589134243684231352L), DELTA360, 0, 0);
  check_complex ("casin (-0 - 0.5 i) == -0 - 0.4812118250596034474977589134243684231352 i",  FUNC(casin) (BUILD_COMPLEX (minus_zero, -0.5L)), BUILD_COMPLEX (minus_zero, -0.4812118250596034474977589134243684231352L), DELTA361, 0, 0);
  check_complex ("casin (+0 + 0.5 i) == +0 + 0.4812118250596034474977589134243684231352 i",  FUNC(casin) (BUILD_COMPLEX (plus_zero, 0.5L)), BUILD_COMPLEX (plus_zero, 0.4812118250596034474977589134243684231352L), DELTA362, 0, 0);
  check_complex ("casin (-0 + 0.5 i) == -0 + 0.4812118250596034474977589134243684231352 i",  FUNC(casin) (BUILD_COMPLEX (minus_zero, 0.5L)), BUILD_COMPLEX (minus_zero, 0.4812118250596034474977589134243684231352L), DELTA363, 0, 0);
  check_complex ("casin (+0 + 1.0 i) == +0 + 0.8813735870195430252326093249797923090282 i",  FUNC(casin) (BUILD_COMPLEX (plus_zero, 1.0L)), BUILD_COMPLEX (plus_zero, 0.8813735870195430252326093249797923090282L), DELTA364, 0, 0);
  check_complex ("casin (-0 + 1.0 i) == -0 + 0.8813735870195430252326093249797923090282 i",  FUNC(casin) (BUILD_COMPLEX (minus_zero, 1.0L)), BUILD_COMPLEX (minus_zero, 0.8813735870195430252326093249797923090282L), DELTA365, 0, 0);
  check_complex ("casin (+0 + 1.5 i) == +0 + 1.194763217287109304111930828519090523536 i",  FUNC(casin) (BUILD_COMPLEX (plus_zero, 1.5L)), BUILD_COMPLEX (plus_zero, 1.194763217287109304111930828519090523536L), DELTA366, 0, 0);
  check_complex ("casin (-0 + 1.5 i) == -0 + 1.194763217287109304111930828519090523536 i",  FUNC(casin) (BUILD_COMPLEX (minus_zero, 1.5L)), BUILD_COMPLEX (minus_zero, 1.194763217287109304111930828519090523536L), DELTA367, 0, 0);

  check_complex ("casin (-1.5 + +0 i) == -pi/2 + 0.9624236501192068949955178268487368462704 i",  FUNC(casin) (BUILD_COMPLEX (-1.5L, plus_zero)), BUILD_COMPLEX (-M_PI_2l, 0.9624236501192068949955178268487368462704L), DELTA368, 0, 0);
  check_complex ("casin (-1.5 - 0 i) == -pi/2 - 0.9624236501192068949955178268487368462704 i",  FUNC(casin) (BUILD_COMPLEX (-1.5L, minus_zero)), BUILD_COMPLEX (-M_PI_2l, -0.9624236501192068949955178268487368462704L), DELTA369, 0, 0);
  check_complex ("casin (-1.0 + +0 i) == -pi/2 + +0 i",  FUNC(casin) (BUILD_COMPLEX (-1.0L, plus_zero)), BUILD_COMPLEX (-M_PI_2l, plus_zero), 0, 0, 0);
  check_complex ("casin (-1.0 - 0 i) == -pi/2 - 0 i",  FUNC(casin) (BUILD_COMPLEX (-1.0L, minus_zero)), BUILD_COMPLEX (-M_PI_2l, minus_zero), 0, 0, 0);
  check_complex ("casin (-0.5 + +0 i) == -0.5235987755982988730771072305465838140329 + +0 i",  FUNC(casin) (BUILD_COMPLEX (-0.5L, plus_zero)), BUILD_COMPLEX (-0.5235987755982988730771072305465838140329L, plus_zero), 0, 0, 0);
  check_complex ("casin (-0.5 - 0 i) == -0.5235987755982988730771072305465838140329 - 0 i",  FUNC(casin) (BUILD_COMPLEX (-0.5L, minus_zero)), BUILD_COMPLEX (-0.5235987755982988730771072305465838140329L, minus_zero), 0, 0, 0);
  check_complex ("casin (0.5 + +0 i) == 0.5235987755982988730771072305465838140329 + +0 i",  FUNC(casin) (BUILD_COMPLEX (0.5L, plus_zero)), BUILD_COMPLEX (0.5235987755982988730771072305465838140329L, plus_zero), 0, 0, 0);
  check_complex ("casin (0.5 - 0 i) == 0.5235987755982988730771072305465838140329 - 0 i",  FUNC(casin) (BUILD_COMPLEX (0.5L, minus_zero)), BUILD_COMPLEX (0.5235987755982988730771072305465838140329L, minus_zero), 0, 0, 0);
  check_complex ("casin (1.0 + +0 i) == pi/2 + +0 i",  FUNC(casin) (BUILD_COMPLEX (1.0L, plus_zero)), BUILD_COMPLEX (M_PI_2l, plus_zero), 0, 0, 0);
  check_complex ("casin (1.0 - 0 i) == pi/2 - 0 i",  FUNC(casin) (BUILD_COMPLEX (1.0L, minus_zero)), BUILD_COMPLEX (M_PI_2l, minus_zero), 0, 0, 0);
  check_complex ("casin (1.5 + +0 i) == pi/2 + 0.9624236501192068949955178268487368462704 i",  FUNC(casin) (BUILD_COMPLEX (1.5L, plus_zero)), BUILD_COMPLEX (M_PI_2l, 0.9624236501192068949955178268487368462704L), DELTA378, 0, 0);
  check_complex ("casin (1.5 - 0 i) == pi/2 - 0.9624236501192068949955178268487368462704 i",  FUNC(casin) (BUILD_COMPLEX (1.5L, minus_zero)), BUILD_COMPLEX (M_PI_2l, -0.9624236501192068949955178268487368462704L), DELTA379, 0, 0);

  check_complex ("casin (0.75 + 1.25 i) == 0.453276177638793913448921196101971749 + 1.13239363160530819522266333696834467 i",  FUNC(casin) (BUILD_COMPLEX (0.75L, 1.25L)), BUILD_COMPLEX (0.453276177638793913448921196101971749L, 1.13239363160530819522266333696834467L), DELTA380, 0, 0);
  check_complex ("casin (-2 - 3 i) == -0.57065278432109940071028387968566963 - 1.9833870299165354323470769028940395 i",  FUNC(casin) (BUILD_COMPLEX (-2, -3)), BUILD_COMPLEX (-0.57065278432109940071028387968566963L, -1.9833870299165354323470769028940395L), 0, 0, 0);

  print_complex_max_error ("casin", DELTAcasin, 0);
}


static void
casinh_test (void)
{
  errno = 0;
  FUNC(casinh) (BUILD_COMPLEX (0.7L, 1.2L));
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_complex ("casinh (0 + 0 i) == 0.0 + 0.0 i",  FUNC(casinh) (BUILD_COMPLEX (0, 0)), BUILD_COMPLEX (0.0, 0.0), 0, 0, 0);
  check_complex ("casinh (-0 + 0 i) == -0 + 0 i",  FUNC(casinh) (BUILD_COMPLEX (minus_zero, 0)), BUILD_COMPLEX (minus_zero, 0), 0, 0, 0);
  check_complex ("casinh (0 - 0 i) == 0.0 - 0 i",  FUNC(casinh) (BUILD_COMPLEX (0, minus_zero)), BUILD_COMPLEX (0.0, minus_zero), 0, 0, 0);
  check_complex ("casinh (-0 - 0 i) == -0 - 0 i",  FUNC(casinh) (BUILD_COMPLEX (minus_zero, minus_zero)), BUILD_COMPLEX (minus_zero, minus_zero), 0, 0, 0);

  check_complex ("casinh (inf + inf i) == inf + pi/4 i",  FUNC(casinh) (BUILD_COMPLEX (plus_infty, plus_infty)), BUILD_COMPLEX (plus_infty, M_PI_4l), 0, 0, 0);
  check_complex ("casinh (inf - inf i) == inf - pi/4 i",  FUNC(casinh) (BUILD_COMPLEX (plus_infty, minus_infty)), BUILD_COMPLEX (plus_infty, -M_PI_4l), 0, 0, 0);
  check_complex ("casinh (-inf + inf i) == -inf + pi/4 i",  FUNC(casinh) (BUILD_COMPLEX (minus_infty, plus_infty)), BUILD_COMPLEX (minus_infty, M_PI_4l), 0, 0, 0);
  check_complex ("casinh (-inf - inf i) == -inf - pi/4 i",  FUNC(casinh) (BUILD_COMPLEX (minus_infty, minus_infty)), BUILD_COMPLEX (minus_infty, -M_PI_4l), 0, 0, 0);

  check_complex ("casinh (-10.0 + inf i) == -inf + pi/2 i",  FUNC(casinh) (BUILD_COMPLEX (-10.0, plus_infty)), BUILD_COMPLEX (minus_infty, M_PI_2l), 0, 0, 0);
  check_complex ("casinh (-10.0 - inf i) == -inf - pi/2 i",  FUNC(casinh) (BUILD_COMPLEX (-10.0, minus_infty)), BUILD_COMPLEX (minus_infty, -M_PI_2l), 0, 0, 0);
  check_complex ("casinh (0 + inf i) == inf + pi/2 i",  FUNC(casinh) (BUILD_COMPLEX (0, plus_infty)), BUILD_COMPLEX (plus_infty, M_PI_2l), 0, 0, 0);
  check_complex ("casinh (0 - inf i) == inf - pi/2 i",  FUNC(casinh) (BUILD_COMPLEX (0, minus_infty)), BUILD_COMPLEX (plus_infty, -M_PI_2l), 0, 0, 0);
  check_complex ("casinh (-0 + inf i) == -inf + pi/2 i",  FUNC(casinh) (BUILD_COMPLEX (minus_zero, plus_infty)), BUILD_COMPLEX (minus_infty, M_PI_2l), 0, 0, 0);
  check_complex ("casinh (-0 - inf i) == -inf - pi/2 i",  FUNC(casinh) (BUILD_COMPLEX (minus_zero, minus_infty)), BUILD_COMPLEX (minus_infty, -M_PI_2l), 0, 0, 0);
  check_complex ("casinh (0.1 + inf i) == inf + pi/2 i",  FUNC(casinh) (BUILD_COMPLEX (0.1L, plus_infty)), BUILD_COMPLEX (plus_infty, M_PI_2l), 0, 0, 0);
  check_complex ("casinh (0.1 - inf i) == inf - pi/2 i",  FUNC(casinh) (BUILD_COMPLEX (0.1L, minus_infty)), BUILD_COMPLEX (plus_infty, -M_PI_2l), 0, 0, 0);

  check_complex ("casinh (-inf + 0 i) == -inf + 0.0 i",  FUNC(casinh) (BUILD_COMPLEX (minus_infty, 0)), BUILD_COMPLEX (minus_infty, 0.0), 0, 0, 0);
  check_complex ("casinh (-inf - 0 i) == -inf - 0 i",  FUNC(casinh) (BUILD_COMPLEX (minus_infty, minus_zero)), BUILD_COMPLEX (minus_infty, minus_zero), 0, 0, 0);
  check_complex ("casinh (-inf + 100 i) == -inf + 0.0 i",  FUNC(casinh) (BUILD_COMPLEX (minus_infty, 100)), BUILD_COMPLEX (minus_infty, 0.0), 0, 0, 0);
  check_complex ("casinh (-inf - 100 i) == -inf - 0 i",  FUNC(casinh) (BUILD_COMPLEX (minus_infty, -100)), BUILD_COMPLEX (minus_infty, minus_zero), 0, 0, 0);

  check_complex ("casinh (inf + 0 i) == inf + 0.0 i",  FUNC(casinh) (BUILD_COMPLEX (plus_infty, 0)), BUILD_COMPLEX (plus_infty, 0.0), 0, 0, 0);
  check_complex ("casinh (inf - 0 i) == inf - 0 i",  FUNC(casinh) (BUILD_COMPLEX (plus_infty, minus_zero)), BUILD_COMPLEX (plus_infty, minus_zero), 0, 0, 0);
  check_complex ("casinh (inf + 0.5 i) == inf + 0.0 i",  FUNC(casinh) (BUILD_COMPLEX (plus_infty, 0.5)), BUILD_COMPLEX (plus_infty, 0.0), 0, 0, 0);
  check_complex ("casinh (inf - 0.5 i) == inf - 0 i",  FUNC(casinh) (BUILD_COMPLEX (plus_infty, -0.5)), BUILD_COMPLEX (plus_infty, minus_zero), 0, 0, 0);

  check_complex ("casinh (inf + NaN i) == inf + NaN i",  FUNC(casinh) (BUILD_COMPLEX (plus_infty, nan_value)), BUILD_COMPLEX (plus_infty, nan_value), 0, 0, 0);
  check_complex ("casinh (-inf + NaN i) == -inf + NaN i",  FUNC(casinh) (BUILD_COMPLEX (minus_infty, nan_value)), BUILD_COMPLEX (minus_infty, nan_value), 0, 0, 0);

  check_complex ("casinh (NaN + 0 i) == NaN + 0.0 i",  FUNC(casinh) (BUILD_COMPLEX (nan_value, 0)), BUILD_COMPLEX (nan_value, 0.0), 0, 0, 0);
  check_complex ("casinh (NaN - 0 i) == NaN - 0 i",  FUNC(casinh) (BUILD_COMPLEX (nan_value, minus_zero)), BUILD_COMPLEX (nan_value, minus_zero), 0, 0, 0);

  check_complex ("casinh (NaN + inf i) == inf + NaN i",  FUNC(casinh) (BUILD_COMPLEX (nan_value, plus_infty)), BUILD_COMPLEX (plus_infty, nan_value), 0, 0, IGNORE_ZERO_INF_SIGN);
  check_complex ("casinh (NaN - inf i) == inf + NaN i",  FUNC(casinh) (BUILD_COMPLEX (nan_value, minus_infty)), BUILD_COMPLEX (plus_infty, nan_value), 0, 0, IGNORE_ZERO_INF_SIGN);

  check_complex ("casinh (10.5 + NaN i) == NaN + NaN i",  FUNC(casinh) (BUILD_COMPLEX (10.5, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("casinh (-10.5 + NaN i) == NaN + NaN i",  FUNC(casinh) (BUILD_COMPLEX (-10.5, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);

  check_complex ("casinh (NaN + 0.75 i) == NaN + NaN i",  FUNC(casinh) (BUILD_COMPLEX (nan_value, 0.75)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("casinh (-0.75 + NaN i) == NaN + NaN i",  FUNC(casinh) (BUILD_COMPLEX (-0.75, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);

  check_complex ("casinh (NaN + NaN i) == NaN + NaN i",  FUNC(casinh) (BUILD_COMPLEX (nan_value, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, 0);

  check_complex ("casinh (+0 - 1.5 i) == 0.9624236501192068949955178268487368462704 - pi/2 i",  FUNC(casinh) (BUILD_COMPLEX (plus_zero, -1.5L)), BUILD_COMPLEX (0.9624236501192068949955178268487368462704L, -M_PI_2l), DELTA417, 0, 0);
  check_complex ("casinh (-0 - 1.5 i) == -0.9624236501192068949955178268487368462704 - pi/2 i",  FUNC(casinh) (BUILD_COMPLEX (minus_zero, -1.5L)), BUILD_COMPLEX (-0.9624236501192068949955178268487368462704L, -M_PI_2l), DELTA418, 0, 0);
  check_complex ("casinh (+0 - 1.0 i) == +0 - pi/2 i",  FUNC(casinh) (BUILD_COMPLEX (plus_zero, -1.0L)), BUILD_COMPLEX (plus_zero, -M_PI_2l), 0, 0, 0);
  check_complex ("casinh (-0 - 1.0 i) == -0 - pi/2 i",  FUNC(casinh) (BUILD_COMPLEX (minus_zero, -1.0L)), BUILD_COMPLEX (minus_zero, -M_PI_2l), 0, 0, 0);
  check_complex ("casinh (+0 - 0.5 i) == +0 - 0.5235987755982988730771072305465838140329 i",  FUNC(casinh) (BUILD_COMPLEX (plus_zero, -0.5L)), BUILD_COMPLEX (plus_zero, -0.5235987755982988730771072305465838140329L), 0, 0, 0);
  check_complex ("casinh (-0 - 0.5 i) == -0 - 0.5235987755982988730771072305465838140329 i",  FUNC(casinh) (BUILD_COMPLEX (minus_zero, -0.5L)), BUILD_COMPLEX (minus_zero, -0.5235987755982988730771072305465838140329L), 0, 0, 0);
  check_complex ("casinh (+0 + 0.5 i) == +0 + 0.5235987755982988730771072305465838140329 i",  FUNC(casinh) (BUILD_COMPLEX (plus_zero, 0.5L)), BUILD_COMPLEX (plus_zero, 0.5235987755982988730771072305465838140329L), 0, 0, 0);
  check_complex ("casinh (-0 + 0.5 i) == -0 + 0.5235987755982988730771072305465838140329 i",  FUNC(casinh) (BUILD_COMPLEX (minus_zero, 0.5L)), BUILD_COMPLEX (minus_zero, 0.5235987755982988730771072305465838140329L), 0, 0, 0);
  check_complex ("casinh (+0 + 1.0 i) == +0 + pi/2 i",  FUNC(casinh) (BUILD_COMPLEX (plus_zero, 1.0L)), BUILD_COMPLEX (plus_zero, M_PI_2l), 0, 0, 0);
  check_complex ("casinh (-0 + 1.0 i) == -0 + pi/2 i",  FUNC(casinh) (BUILD_COMPLEX (minus_zero, 1.0L)), BUILD_COMPLEX (minus_zero, M_PI_2l), 0, 0, 0);
  check_complex ("casinh (+0 + 1.5 i) == 0.9624236501192068949955178268487368462704 + pi/2 i",  FUNC(casinh) (BUILD_COMPLEX (plus_zero, 1.5L)), BUILD_COMPLEX (0.9624236501192068949955178268487368462704L, M_PI_2l), DELTA427, 0, 0);
  check_complex ("casinh (-0 + 1.5 i) == -0.9624236501192068949955178268487368462704 + pi/2 i",  FUNC(casinh) (BUILD_COMPLEX (minus_zero, 1.5L)), BUILD_COMPLEX (-0.9624236501192068949955178268487368462704L, M_PI_2l), DELTA428, 0, 0);

  check_complex ("casinh (-1.5 + +0 i) == -1.194763217287109304111930828519090523536 + +0 i",  FUNC(casinh) (BUILD_COMPLEX (-1.5L, plus_zero)), BUILD_COMPLEX (-1.194763217287109304111930828519090523536L, plus_zero), DELTA429, 0, 0);
  check_complex ("casinh (-1.5 - 0 i) == -1.194763217287109304111930828519090523536 - 0 i",  FUNC(casinh) (BUILD_COMPLEX (-1.5L, minus_zero)), BUILD_COMPLEX (-1.194763217287109304111930828519090523536L, minus_zero), DELTA430, 0, 0);
  check_complex ("casinh (-1.0 + +0 i) == -0.8813735870195430252326093249797923090282 + +0 i",  FUNC(casinh) (BUILD_COMPLEX (-1.0L, plus_zero)), BUILD_COMPLEX (-0.8813735870195430252326093249797923090282L, plus_zero), DELTA431, 0, 0);
  check_complex ("casinh (-1.0 - 0 i) == -0.8813735870195430252326093249797923090282 - 0 i",  FUNC(casinh) (BUILD_COMPLEX (-1.0L, minus_zero)), BUILD_COMPLEX (-0.8813735870195430252326093249797923090282L, minus_zero), DELTA432, 0, 0);
  check_complex ("casinh (-0.5 + +0 i) == -0.4812118250596034474977589134243684231352 + +0 i",  FUNC(casinh) (BUILD_COMPLEX (-0.5L, plus_zero)), BUILD_COMPLEX (-0.4812118250596034474977589134243684231352L, plus_zero), DELTA433, 0, 0);
  check_complex ("casinh (-0.5 - 0 i) == -0.4812118250596034474977589134243684231352 - 0 i",  FUNC(casinh) (BUILD_COMPLEX (-0.5L, minus_zero)), BUILD_COMPLEX (-0.4812118250596034474977589134243684231352L, minus_zero), DELTA434, 0, 0);
  check_complex ("casinh (0.5 + +0 i) == 0.4812118250596034474977589134243684231352 + +0 i",  FUNC(casinh) (BUILD_COMPLEX (0.5L, plus_zero)), BUILD_COMPLEX (0.4812118250596034474977589134243684231352L, plus_zero), DELTA435, 0, 0);
  check_complex ("casinh (0.5 - 0 i) == 0.4812118250596034474977589134243684231352 - 0 i",  FUNC(casinh) (BUILD_COMPLEX (0.5L, minus_zero)), BUILD_COMPLEX (0.4812118250596034474977589134243684231352L, minus_zero), DELTA436, 0, 0);
  check_complex ("casinh (1.0 + +0 i) == 0.8813735870195430252326093249797923090282 + +0 i",  FUNC(casinh) (BUILD_COMPLEX (1.0L, plus_zero)), BUILD_COMPLEX (0.8813735870195430252326093249797923090282L, plus_zero), DELTA437, 0, 0);
  check_complex ("casinh (1.0 - 0 i) == 0.8813735870195430252326093249797923090282 - 0 i",  FUNC(casinh) (BUILD_COMPLEX (1.0L, minus_zero)), BUILD_COMPLEX (0.8813735870195430252326093249797923090282L, minus_zero), DELTA438, 0, 0);
  check_complex ("casinh (1.5 + +0 i) == 1.194763217287109304111930828519090523536 + +0 i",  FUNC(casinh) (BUILD_COMPLEX (1.5L, plus_zero)), BUILD_COMPLEX (1.194763217287109304111930828519090523536L, plus_zero), DELTA439, 0, 0);
  check_complex ("casinh (1.5 - 0 i) == 1.194763217287109304111930828519090523536 - 0 i",  FUNC(casinh) (BUILD_COMPLEX (1.5L, minus_zero)), BUILD_COMPLEX (1.194763217287109304111930828519090523536L, minus_zero), DELTA440, 0, 0);

  check_complex ("casinh (0.75 + 1.25 i) == 1.03171853444778027336364058631006594 + 0.911738290968487636358489564316731207 i",  FUNC(casinh) (BUILD_COMPLEX (0.75L, 1.25L)), BUILD_COMPLEX (1.03171853444778027336364058631006594L, 0.911738290968487636358489564316731207L), DELTA441, 0, 0);
  check_complex ("casinh (-2 - 3 i) == -1.9686379257930962917886650952454982 - 0.96465850440760279204541105949953237 i",  FUNC(casinh) (BUILD_COMPLEX (-2, -3)), BUILD_COMPLEX (-1.9686379257930962917886650952454982L, -0.96465850440760279204541105949953237L), DELTA442, 0, 0);

  print_complex_max_error ("casinh", DELTAcasinh, 0);
}


static void
catan_test (void)
{
  errno = 0;
  FUNC(catan) (BUILD_COMPLEX (0.7L, 1.2L));
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_complex ("catan (0 + 0 i) == 0 + 0 i",  FUNC(catan) (BUILD_COMPLEX (0, 0)), BUILD_COMPLEX (0, 0), 0, 0, 0);
  check_complex ("catan (-0 + 0 i) == -0 + 0 i",  FUNC(catan) (BUILD_COMPLEX (minus_zero, 0)), BUILD_COMPLEX (minus_zero, 0), 0, 0, 0);
  check_complex ("catan (0 - 0 i) == 0 - 0 i",  FUNC(catan) (BUILD_COMPLEX (0, minus_zero)), BUILD_COMPLEX (0, minus_zero), 0, 0, 0);
  check_complex ("catan (-0 - 0 i) == -0 - 0 i",  FUNC(catan) (BUILD_COMPLEX (minus_zero, minus_zero)), BUILD_COMPLEX (minus_zero, minus_zero), 0, 0, 0);

  check_complex ("catan (inf + inf i) == pi/2 + 0 i",  FUNC(catan) (BUILD_COMPLEX (plus_infty, plus_infty)), BUILD_COMPLEX (M_PI_2l, 0), 0, 0, 0);
  check_complex ("catan (inf - inf i) == pi/2 - 0 i",  FUNC(catan) (BUILD_COMPLEX (plus_infty, minus_infty)), BUILD_COMPLEX (M_PI_2l, minus_zero), 0, 0, 0);
  check_complex ("catan (-inf + inf i) == -pi/2 + 0 i",  FUNC(catan) (BUILD_COMPLEX (minus_infty, plus_infty)), BUILD_COMPLEX (-M_PI_2l, 0), 0, 0, 0);
  check_complex ("catan (-inf - inf i) == -pi/2 - 0 i",  FUNC(catan) (BUILD_COMPLEX (minus_infty, minus_infty)), BUILD_COMPLEX (-M_PI_2l, minus_zero), 0, 0, 0);


  check_complex ("catan (inf - 10.0 i) == pi/2 - 0 i",  FUNC(catan) (BUILD_COMPLEX (plus_infty, -10.0)), BUILD_COMPLEX (M_PI_2l, minus_zero), 0, 0, 0);
  check_complex ("catan (-inf - 10.0 i) == -pi/2 - 0 i",  FUNC(catan) (BUILD_COMPLEX (minus_infty, -10.0)), BUILD_COMPLEX (-M_PI_2l, minus_zero), 0, 0, 0);
  check_complex ("catan (inf - 0 i) == pi/2 - 0 i",  FUNC(catan) (BUILD_COMPLEX (plus_infty, minus_zero)), BUILD_COMPLEX (M_PI_2l, minus_zero), 0, 0, 0);
  check_complex ("catan (-inf - 0 i) == -pi/2 - 0 i",  FUNC(catan) (BUILD_COMPLEX (minus_infty, minus_zero)), BUILD_COMPLEX (-M_PI_2l, minus_zero), 0, 0, 0);
  check_complex ("catan (inf + 0.0 i) == pi/2 + 0 i",  FUNC(catan) (BUILD_COMPLEX (plus_infty, 0.0)), BUILD_COMPLEX (M_PI_2l, 0), 0, 0, 0);
  check_complex ("catan (-inf + 0.0 i) == -pi/2 + 0 i",  FUNC(catan) (BUILD_COMPLEX (minus_infty, 0.0)), BUILD_COMPLEX (-M_PI_2l, 0), 0, 0, 0);
  check_complex ("catan (inf + 0.1 i) == pi/2 + 0 i",  FUNC(catan) (BUILD_COMPLEX (plus_infty, 0.1L)), BUILD_COMPLEX (M_PI_2l, 0), 0, 0, 0);
  check_complex ("catan (-inf + 0.1 i) == -pi/2 + 0 i",  FUNC(catan) (BUILD_COMPLEX (minus_infty, 0.1L)), BUILD_COMPLEX (-M_PI_2l, 0), 0, 0, 0);

  check_complex ("catan (0.0 - inf i) == pi/2 - 0 i",  FUNC(catan) (BUILD_COMPLEX (0.0, minus_infty)), BUILD_COMPLEX (M_PI_2l, minus_zero), 0, 0, 0);
  check_complex ("catan (-0 - inf i) == -pi/2 - 0 i",  FUNC(catan) (BUILD_COMPLEX (minus_zero, minus_infty)), BUILD_COMPLEX (-M_PI_2l, minus_zero), 0, 0, 0);
  check_complex ("catan (100.0 - inf i) == pi/2 - 0 i",  FUNC(catan) (BUILD_COMPLEX (100.0, minus_infty)), BUILD_COMPLEX (M_PI_2l, minus_zero), 0, 0, 0);
  check_complex ("catan (-100.0 - inf i) == -pi/2 - 0 i",  FUNC(catan) (BUILD_COMPLEX (-100.0, minus_infty)), BUILD_COMPLEX (-M_PI_2l, minus_zero), 0, 0, 0);

  check_complex ("catan (0.0 + inf i) == pi/2 + 0 i",  FUNC(catan) (BUILD_COMPLEX (0.0, plus_infty)), BUILD_COMPLEX (M_PI_2l, 0), 0, 0, 0);
  check_complex ("catan (-0 + inf i) == -pi/2 + 0 i",  FUNC(catan) (BUILD_COMPLEX (minus_zero, plus_infty)), BUILD_COMPLEX (-M_PI_2l, 0), 0, 0, 0);
  check_complex ("catan (0.5 + inf i) == pi/2 + 0 i",  FUNC(catan) (BUILD_COMPLEX (0.5, plus_infty)), BUILD_COMPLEX (M_PI_2l, 0), 0, 0, 0);
  check_complex ("catan (-0.5 + inf i) == -pi/2 + 0 i",  FUNC(catan) (BUILD_COMPLEX (-0.5, plus_infty)), BUILD_COMPLEX (-M_PI_2l, 0), 0, 0, 0);

  check_complex ("catan (NaN + 0.0 i) == NaN + 0 i",  FUNC(catan) (BUILD_COMPLEX (nan_value, 0.0)), BUILD_COMPLEX (nan_value, 0), 0, 0, 0);
  check_complex ("catan (NaN - 0 i) == NaN - 0 i",  FUNC(catan) (BUILD_COMPLEX (nan_value, minus_zero)), BUILD_COMPLEX (nan_value, minus_zero), 0, 0, 0);

  check_complex ("catan (NaN + inf i) == NaN + 0 i",  FUNC(catan) (BUILD_COMPLEX (nan_value, plus_infty)), BUILD_COMPLEX (nan_value, 0), 0, 0, 0);
  check_complex ("catan (NaN - inf i) == NaN - 0 i",  FUNC(catan) (BUILD_COMPLEX (nan_value, minus_infty)), BUILD_COMPLEX (nan_value, minus_zero), 0, 0, 0);

  check_complex ("catan (0.0 + NaN i) == NaN + NaN i",  FUNC(catan) (BUILD_COMPLEX (0.0, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, 0);
  check_complex ("catan (-0 + NaN i) == NaN + NaN i",  FUNC(catan) (BUILD_COMPLEX (minus_zero, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, 0);

  check_complex ("catan (inf + NaN i) == pi/2 + 0 i",  FUNC(catan) (BUILD_COMPLEX (plus_infty, nan_value)), BUILD_COMPLEX (M_PI_2l, 0), 0, 0, IGNORE_ZERO_INF_SIGN);
  check_complex ("catan (-inf + NaN i) == -pi/2 + 0 i",  FUNC(catan) (BUILD_COMPLEX (minus_infty, nan_value)), BUILD_COMPLEX (-M_PI_2l, 0), 0, 0, IGNORE_ZERO_INF_SIGN);

  check_complex ("catan (NaN + 10.5 i) == NaN + NaN i",  FUNC(catan) (BUILD_COMPLEX (nan_value, 10.5)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("catan (NaN - 10.5 i) == NaN + NaN i",  FUNC(catan) (BUILD_COMPLEX (nan_value, -10.5)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);

  check_complex ("catan (0.75 + NaN i) == NaN + NaN i",  FUNC(catan) (BUILD_COMPLEX (0.75, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("catan (-0.75 + NaN i) == NaN + NaN i",  FUNC(catan) (BUILD_COMPLEX (-0.75, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);

  check_complex ("catan (NaN + NaN i) == NaN + NaN i",  FUNC(catan) (BUILD_COMPLEX (nan_value, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, 0);

  check_complex ("catan (0.75 + 1.25 i) == 1.10714871779409050301706546017853704 + 0.549306144334054845697622618461262852 i",  FUNC(catan) (BUILD_COMPLEX (0.75L, 1.25L)), BUILD_COMPLEX (1.10714871779409050301706546017853704L, 0.549306144334054845697622618461262852L), DELTA480, 0, 0);
  check_complex ("catan (-2 - 3 i) == -1.4099210495965755225306193844604208 - 0.22907268296853876629588180294200276 i",  FUNC(catan) (BUILD_COMPLEX (-2, -3)), BUILD_COMPLEX (-1.4099210495965755225306193844604208L, -0.22907268296853876629588180294200276L), DELTA481, 0, 0);

  print_complex_max_error ("catan", DELTAcatan, 0);
}

static void
catanh_test (void)
{
  errno = 0;
  FUNC(catanh) (BUILD_COMPLEX (0.7L, 1.2L));
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_complex ("catanh (0 + 0 i) == 0.0 + 0.0 i",  FUNC(catanh) (BUILD_COMPLEX (0, 0)), BUILD_COMPLEX (0.0, 0.0), 0, 0, 0);
  check_complex ("catanh (-0 + 0 i) == -0 + 0.0 i",  FUNC(catanh) (BUILD_COMPLEX (minus_zero, 0)), BUILD_COMPLEX (minus_zero, 0.0), 0, 0, 0);
  check_complex ("catanh (0 - 0 i) == 0.0 - 0 i",  FUNC(catanh) (BUILD_COMPLEX (0, minus_zero)), BUILD_COMPLEX (0.0, minus_zero), 0, 0, 0);
  check_complex ("catanh (-0 - 0 i) == -0 - 0 i",  FUNC(catanh) (BUILD_COMPLEX (minus_zero, minus_zero)), BUILD_COMPLEX (minus_zero, minus_zero), 0, 0, 0);

  check_complex ("catanh (inf + inf i) == 0.0 + pi/2 i",  FUNC(catanh) (BUILD_COMPLEX (plus_infty, plus_infty)), BUILD_COMPLEX (0.0, M_PI_2l), 0, 0, 0);
  check_complex ("catanh (inf - inf i) == 0.0 - pi/2 i",  FUNC(catanh) (BUILD_COMPLEX (plus_infty, minus_infty)), BUILD_COMPLEX (0.0, -M_PI_2l), 0, 0, 0);
  check_complex ("catanh (-inf + inf i) == -0 + pi/2 i",  FUNC(catanh) (BUILD_COMPLEX (minus_infty, plus_infty)), BUILD_COMPLEX (minus_zero, M_PI_2l), 0, 0, 0);
  check_complex ("catanh (-inf - inf i) == -0 - pi/2 i",  FUNC(catanh) (BUILD_COMPLEX (minus_infty, minus_infty)), BUILD_COMPLEX (minus_zero, -M_PI_2l), 0, 0, 0);

  check_complex ("catanh (-10.0 + inf i) == -0 + pi/2 i",  FUNC(catanh) (BUILD_COMPLEX (-10.0, plus_infty)), BUILD_COMPLEX (minus_zero, M_PI_2l), 0, 0, 0);
  check_complex ("catanh (-10.0 - inf i) == -0 - pi/2 i",  FUNC(catanh) (BUILD_COMPLEX (-10.0, minus_infty)), BUILD_COMPLEX (minus_zero, -M_PI_2l), 0, 0, 0);
  check_complex ("catanh (-0 + inf i) == -0 + pi/2 i",  FUNC(catanh) (BUILD_COMPLEX (minus_zero, plus_infty)), BUILD_COMPLEX (minus_zero, M_PI_2l), 0, 0, 0);
  check_complex ("catanh (-0 - inf i) == -0 - pi/2 i",  FUNC(catanh) (BUILD_COMPLEX (minus_zero, minus_infty)), BUILD_COMPLEX (minus_zero, -M_PI_2l), 0, 0, 0);
  check_complex ("catanh (0 + inf i) == 0.0 + pi/2 i",  FUNC(catanh) (BUILD_COMPLEX (0, plus_infty)), BUILD_COMPLEX (0.0, M_PI_2l), 0, 0, 0);
  check_complex ("catanh (0 - inf i) == 0.0 - pi/2 i",  FUNC(catanh) (BUILD_COMPLEX (0, minus_infty)), BUILD_COMPLEX (0.0, -M_PI_2l), 0, 0, 0);
  check_complex ("catanh (0.1 + inf i) == 0.0 + pi/2 i",  FUNC(catanh) (BUILD_COMPLEX (0.1L, plus_infty)), BUILD_COMPLEX (0.0, M_PI_2l), 0, 0, 0);
  check_complex ("catanh (0.1 - inf i) == 0.0 - pi/2 i",  FUNC(catanh) (BUILD_COMPLEX (0.1L, minus_infty)), BUILD_COMPLEX (0.0, -M_PI_2l), 0, 0, 0);

  check_complex ("catanh (-inf + 0 i) == -0 + pi/2 i",  FUNC(catanh) (BUILD_COMPLEX (minus_infty, 0)), BUILD_COMPLEX (minus_zero, M_PI_2l), 0, 0, 0);
  check_complex ("catanh (-inf - 0 i) == -0 - pi/2 i",  FUNC(catanh) (BUILD_COMPLEX (minus_infty, minus_zero)), BUILD_COMPLEX (minus_zero, -M_PI_2l), 0, 0, 0);
  check_complex ("catanh (-inf + 100 i) == -0 + pi/2 i",  FUNC(catanh) (BUILD_COMPLEX (minus_infty, 100)), BUILD_COMPLEX (minus_zero, M_PI_2l), 0, 0, 0);
  check_complex ("catanh (-inf - 100 i) == -0 - pi/2 i",  FUNC(catanh) (BUILD_COMPLEX (minus_infty, -100)), BUILD_COMPLEX (minus_zero, -M_PI_2l), 0, 0, 0);

  check_complex ("catanh (inf + 0 i) == 0.0 + pi/2 i",  FUNC(catanh) (BUILD_COMPLEX (plus_infty, 0)), BUILD_COMPLEX (0.0, M_PI_2l), 0, 0, 0);
  check_complex ("catanh (inf - 0 i) == 0.0 - pi/2 i",  FUNC(catanh) (BUILD_COMPLEX (plus_infty, minus_zero)), BUILD_COMPLEX (0.0, -M_PI_2l), 0, 0, 0);
  check_complex ("catanh (inf + 0.5 i) == 0.0 + pi/2 i",  FUNC(catanh) (BUILD_COMPLEX (plus_infty, 0.5)), BUILD_COMPLEX (0.0, M_PI_2l), 0, 0, 0);
  check_complex ("catanh (inf - 0.5 i) == 0.0 - pi/2 i",  FUNC(catanh) (BUILD_COMPLEX (plus_infty, -0.5)), BUILD_COMPLEX (0.0, -M_PI_2l), 0, 0, 0);

  check_complex ("catanh (0 + NaN i) == 0.0 + NaN i",  FUNC(catanh) (BUILD_COMPLEX (0, nan_value)), BUILD_COMPLEX (0.0, nan_value), 0, 0, 0);
  check_complex ("catanh (-0 + NaN i) == -0 + NaN i",  FUNC(catanh) (BUILD_COMPLEX (minus_zero, nan_value)), BUILD_COMPLEX (minus_zero, nan_value), 0, 0, 0);

  check_complex ("catanh (inf + NaN i) == 0.0 + NaN i",  FUNC(catanh) (BUILD_COMPLEX (plus_infty, nan_value)), BUILD_COMPLEX (0.0, nan_value), 0, 0, 0);
  check_complex ("catanh (-inf + NaN i) == -0 + NaN i",  FUNC(catanh) (BUILD_COMPLEX (minus_infty, nan_value)), BUILD_COMPLEX (minus_zero, nan_value), 0, 0, 0);

  check_complex ("catanh (NaN + 0 i) == NaN + NaN i",  FUNC(catanh) (BUILD_COMPLEX (nan_value, 0)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, 0);
  check_complex ("catanh (NaN - 0 i) == NaN + NaN i",  FUNC(catanh) (BUILD_COMPLEX (nan_value, minus_zero)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, 0);

  check_complex ("catanh (NaN + inf i) == 0.0 + pi/2 i",  FUNC(catanh) (BUILD_COMPLEX (nan_value, plus_infty)), BUILD_COMPLEX (0.0, M_PI_2l), 0, 0, IGNORE_ZERO_INF_SIGN);
  check_complex ("catanh (NaN - inf i) == 0.0 - pi/2 i",  FUNC(catanh) (BUILD_COMPLEX (nan_value, minus_infty)), BUILD_COMPLEX (0.0, -M_PI_2l), 0, 0, IGNORE_ZERO_INF_SIGN);

  check_complex ("catanh (10.5 + NaN i) == NaN + NaN i",  FUNC(catanh) (BUILD_COMPLEX (10.5, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("catanh (-10.5 + NaN i) == NaN + NaN i",  FUNC(catanh) (BUILD_COMPLEX (-10.5, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);

  check_complex ("catanh (NaN + 0.75 i) == NaN + NaN i",  FUNC(catanh) (BUILD_COMPLEX (nan_value, 0.75)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("catanh (NaN - 0.75 i) == NaN + NaN i",  FUNC(catanh) (BUILD_COMPLEX (nan_value, -0.75)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);

  check_complex ("catanh (NaN + NaN i) == NaN + NaN i",  FUNC(catanh) (BUILD_COMPLEX (nan_value, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, 0);

  check_complex ("catanh (0.75 + 1.25 i) == 0.261492138795671927078652057366532140 + 0.996825126463918666098902241310446708 i",  FUNC(catanh) (BUILD_COMPLEX (0.75L, 1.25L)), BUILD_COMPLEX (0.261492138795671927078652057366532140L, 0.996825126463918666098902241310446708L), DELTA519, 0, 0);
  check_complex ("catanh (-2 - 3 i) == -0.14694666622552975204743278515471595 - 1.3389725222944935611241935759091443 i",  FUNC(catanh) (BUILD_COMPLEX (-2, -3)), BUILD_COMPLEX (-0.14694666622552975204743278515471595L, -1.3389725222944935611241935759091443L), DELTA520, 0, 0);

  print_complex_max_error ("catanh", DELTAcatanh, 0);
}

static void
cbrt_test (void)
{
  errno = 0;
  FUNC(cbrt) (8);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_float ("cbrt (0.0) == 0.0",  FUNC(cbrt) (0.0), 0.0, 0, 0, 0);
  check_float ("cbrt (-0) == -0",  FUNC(cbrt) (minus_zero), minus_zero, 0, 0, 0);

  check_float ("cbrt (inf) == inf",  FUNC(cbrt) (plus_infty), plus_infty, 0, 0, 0);
  check_float ("cbrt (-inf) == -inf",  FUNC(cbrt) (minus_infty), minus_infty, 0, 0, 0);
  check_float ("cbrt (NaN) == NaN",  FUNC(cbrt) (nan_value), nan_value, 0, 0, 0);

  check_float ("cbrt (-0.001) == -0.1",  FUNC(cbrt) (-0.001L), -0.1L, DELTA526, 0, 0);
  check_float ("cbrt (8) == 2",  FUNC(cbrt) (8), 2, 0, 0, 0);
  check_float ("cbrt (-27.0) == -3.0",  FUNC(cbrt) (-27.0), -3.0, DELTA528, 0, 0);
  check_float ("cbrt (0.9921875) == 0.997389022060725270579075195353955217",  FUNC(cbrt) (0.9921875L), 0.997389022060725270579075195353955217L, DELTA529, 0, 0);
  check_float ("cbrt (0.75) == 0.908560296416069829445605878163630251",  FUNC(cbrt) (0.75L), 0.908560296416069829445605878163630251L, DELTA530, 0, 0);

  print_max_error ("cbrt", DELTAcbrt, 0);
}


static void
ccos_test (void)
{
  errno = 0;
  FUNC(ccos) (BUILD_COMPLEX (0, 0));
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_complex ("ccos (0.0 + 0.0 i) == 1.0 - 0 i",  FUNC(ccos) (BUILD_COMPLEX (0.0, 0.0)), BUILD_COMPLEX (1.0, minus_zero), 0, 0, 0);
  check_complex ("ccos (-0 + 0.0 i) == 1.0 + 0.0 i",  FUNC(ccos) (BUILD_COMPLEX (minus_zero, 0.0)), BUILD_COMPLEX (1.0, 0.0), 0, 0, 0);
  check_complex ("ccos (0.0 - 0 i) == 1.0 + 0.0 i",  FUNC(ccos) (BUILD_COMPLEX (0.0, minus_zero)), BUILD_COMPLEX (1.0, 0.0), 0, 0, 0);
  check_complex ("ccos (-0 - 0 i) == 1.0 - 0 i",  FUNC(ccos) (BUILD_COMPLEX (minus_zero, minus_zero)), BUILD_COMPLEX (1.0, minus_zero), 0, 0, 0);

  check_complex ("ccos (inf + 0.0 i) == NaN + 0.0 i",  FUNC(ccos) (BUILD_COMPLEX (plus_infty, 0.0)), BUILD_COMPLEX (nan_value, 0.0), 0, 0, INVALID_EXCEPTION|IGNORE_ZERO_INF_SIGN);
  check_complex ("ccos (inf - 0 i) == NaN + 0.0 i",  FUNC(ccos) (BUILD_COMPLEX (plus_infty, minus_zero)), BUILD_COMPLEX (nan_value, 0.0), 0, 0, INVALID_EXCEPTION|IGNORE_ZERO_INF_SIGN);
  check_complex ("ccos (-inf + 0.0 i) == NaN + 0.0 i",  FUNC(ccos) (BUILD_COMPLEX (minus_infty, 0.0)), BUILD_COMPLEX (nan_value, 0.0), 0, 0, INVALID_EXCEPTION|IGNORE_ZERO_INF_SIGN);
  check_complex ("ccos (-inf - 0 i) == NaN + 0.0 i",  FUNC(ccos) (BUILD_COMPLEX (minus_infty, minus_zero)), BUILD_COMPLEX (nan_value, 0.0), 0, 0, INVALID_EXCEPTION|IGNORE_ZERO_INF_SIGN);

  check_complex ("ccos (0.0 + inf i) == inf - 0 i",  FUNC(ccos) (BUILD_COMPLEX (0.0, plus_infty)), BUILD_COMPLEX (plus_infty, minus_zero), 0, 0, 0);
  check_complex ("ccos (0.0 - inf i) == inf + 0.0 i",  FUNC(ccos) (BUILD_COMPLEX (0.0, minus_infty)), BUILD_COMPLEX (plus_infty, 0.0), 0, 0, 0);
  check_complex ("ccos (-0 + inf i) == inf + 0.0 i",  FUNC(ccos) (BUILD_COMPLEX (minus_zero, plus_infty)), BUILD_COMPLEX (plus_infty, 0.0), 0, 0, 0);
  check_complex ("ccos (-0 - inf i) == inf - 0 i",  FUNC(ccos) (BUILD_COMPLEX (minus_zero, minus_infty)), BUILD_COMPLEX (plus_infty, minus_zero), 0, 0, 0);

  check_complex ("ccos (inf + inf i) == inf + NaN i",  FUNC(ccos) (BUILD_COMPLEX (plus_infty, plus_infty)), BUILD_COMPLEX (plus_infty, nan_value), 0, 0, INVALID_EXCEPTION);
  check_complex ("ccos (-inf + inf i) == inf + NaN i",  FUNC(ccos) (BUILD_COMPLEX (minus_infty, plus_infty)), BUILD_COMPLEX (plus_infty, nan_value), 0, 0, INVALID_EXCEPTION);
  check_complex ("ccos (inf - inf i) == inf + NaN i",  FUNC(ccos) (BUILD_COMPLEX (plus_infty, minus_infty)), BUILD_COMPLEX (plus_infty, nan_value), 0, 0, INVALID_EXCEPTION);
  check_complex ("ccos (-inf - inf i) == inf + NaN i",  FUNC(ccos) (BUILD_COMPLEX (minus_infty, minus_infty)), BUILD_COMPLEX (plus_infty, nan_value), 0, 0, INVALID_EXCEPTION);

  check_complex ("ccos (4.625 + inf i) == -inf + inf i",  FUNC(ccos) (BUILD_COMPLEX (4.625, plus_infty)), BUILD_COMPLEX (minus_infty, plus_infty), 0, 0, 0);
  check_complex ("ccos (4.625 - inf i) == -inf - inf i",  FUNC(ccos) (BUILD_COMPLEX (4.625, minus_infty)), BUILD_COMPLEX (minus_infty, minus_infty), 0, 0, 0);
  check_complex ("ccos (-4.625 + inf i) == -inf - inf i",  FUNC(ccos) (BUILD_COMPLEX (-4.625, plus_infty)), BUILD_COMPLEX (minus_infty, minus_infty), 0, 0, 0);
  check_complex ("ccos (-4.625 - inf i) == -inf + inf i",  FUNC(ccos) (BUILD_COMPLEX (-4.625, minus_infty)), BUILD_COMPLEX (minus_infty, plus_infty), 0, 0, 0);

  check_complex ("ccos (inf + 6.75 i) == NaN + NaN i",  FUNC(ccos) (BUILD_COMPLEX (plus_infty, 6.75)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);
  check_complex ("ccos (inf - 6.75 i) == NaN + NaN i",  FUNC(ccos) (BUILD_COMPLEX (plus_infty, -6.75)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);
  check_complex ("ccos (-inf + 6.75 i) == NaN + NaN i",  FUNC(ccos) (BUILD_COMPLEX (minus_infty, 6.75)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);
  check_complex ("ccos (-inf - 6.75 i) == NaN + NaN i",  FUNC(ccos) (BUILD_COMPLEX (minus_infty, -6.75)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);

  check_complex ("ccos (NaN + 0.0 i) == NaN + 0.0 i",  FUNC(ccos) (BUILD_COMPLEX (nan_value, 0.0)), BUILD_COMPLEX (nan_value, 0.0), 0, 0, IGNORE_ZERO_INF_SIGN);
  check_complex ("ccos (NaN - 0 i) == NaN + 0.0 i",  FUNC(ccos) (BUILD_COMPLEX (nan_value, minus_zero)), BUILD_COMPLEX (nan_value, 0.0), 0, 0, IGNORE_ZERO_INF_SIGN);

  check_complex ("ccos (NaN + inf i) == inf + NaN i",  FUNC(ccos) (BUILD_COMPLEX (nan_value, plus_infty)), BUILD_COMPLEX (plus_infty, nan_value), 0, 0, 0);
  check_complex ("ccos (NaN - inf i) == inf + NaN i",  FUNC(ccos) (BUILD_COMPLEX (nan_value, minus_infty)), BUILD_COMPLEX (plus_infty, nan_value), 0, 0, 0);

  check_complex ("ccos (NaN + 9.0 i) == NaN + NaN i",  FUNC(ccos) (BUILD_COMPLEX (nan_value, 9.0)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("ccos (NaN - 9.0 i) == NaN + NaN i",  FUNC(ccos) (BUILD_COMPLEX (nan_value, -9.0)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);

  check_complex ("ccos (0.0 + NaN i) == NaN + 0.0 i",  FUNC(ccos) (BUILD_COMPLEX (0.0, nan_value)), BUILD_COMPLEX (nan_value, 0.0), 0, 0, IGNORE_ZERO_INF_SIGN);
  check_complex ("ccos (-0 + NaN i) == NaN + 0.0 i",  FUNC(ccos) (BUILD_COMPLEX (minus_zero, nan_value)), BUILD_COMPLEX (nan_value, 0.0), 0, 0, IGNORE_ZERO_INF_SIGN);

  check_complex ("ccos (10.0 + NaN i) == NaN + NaN i",  FUNC(ccos) (BUILD_COMPLEX (10.0, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("ccos (-10.0 + NaN i) == NaN + NaN i",  FUNC(ccos) (BUILD_COMPLEX (-10.0, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);

  check_complex ("ccos (inf + NaN i) == NaN + NaN i",  FUNC(ccos) (BUILD_COMPLEX (plus_infty, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("ccos (-inf + NaN i) == NaN + NaN i",  FUNC(ccos) (BUILD_COMPLEX (minus_infty, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);

  check_complex ("ccos (NaN + NaN i) == NaN + NaN i",  FUNC(ccos) (BUILD_COMPLEX (nan_value, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, 0);

  check_complex ("ccos (0.75 + 1.25 i) == 1.38173873063425888530729933139078645 - 1.09193013555397466170919531722024128 i",  FUNC(ccos) (BUILD_COMPLEX (0.75L, 1.25L)), BUILD_COMPLEX (1.38173873063425888530729933139078645L, -1.09193013555397466170919531722024128L), DELTA568, 0, 0);
  check_complex ("ccos (-2 - 3 i) == -4.18962569096880723013255501961597373 - 9.10922789375533659797919726277886212 i",  FUNC(ccos) (BUILD_COMPLEX (-2, -3)), BUILD_COMPLEX (-4.18962569096880723013255501961597373L, -9.10922789375533659797919726277886212L), DELTA569, 0, 0);

  check_complex ("ccos (0.75 + 89.5 i) == 2.708024460708609732016532185663087200560e38 - 2.522786001038096774676288412995370563339e38 i",  FUNC(ccos) (BUILD_COMPLEX (0.75, 89.5)), BUILD_COMPLEX (2.708024460708609732016532185663087200560e38L, -2.522786001038096774676288412995370563339e38L), DELTA570, 0, 0);
  check_complex ("ccos (0.75 - 89.5 i) == 2.708024460708609732016532185663087200560e38 + 2.522786001038096774676288412995370563339e38 i",  FUNC(ccos) (BUILD_COMPLEX (0.75, -89.5)), BUILD_COMPLEX (2.708024460708609732016532185663087200560e38L, 2.522786001038096774676288412995370563339e38L), DELTA571, 0, 0);
  check_complex ("ccos (-0.75 + 89.5 i) == 2.708024460708609732016532185663087200560e38 + 2.522786001038096774676288412995370563339e38 i",  FUNC(ccos) (BUILD_COMPLEX (-0.75, 89.5)), BUILD_COMPLEX (2.708024460708609732016532185663087200560e38L, 2.522786001038096774676288412995370563339e38L), DELTA572, 0, 0);
  check_complex ("ccos (-0.75 - 89.5 i) == 2.708024460708609732016532185663087200560e38 - 2.522786001038096774676288412995370563339e38 i",  FUNC(ccos) (BUILD_COMPLEX (-0.75, -89.5)), BUILD_COMPLEX (2.708024460708609732016532185663087200560e38L, -2.522786001038096774676288412995370563339e38L), DELTA573, 0, 0);

#ifndef TEST_FLOAT
  check_complex ("ccos (0.75 + 710.5 i) == 1.347490911916428129246890157395342279438e308 - 1.255317763348154410745082950806112487736e308 i",  FUNC(ccos) (BUILD_COMPLEX (0.75, 710.5)), BUILD_COMPLEX (1.347490911916428129246890157395342279438e308L, -1.255317763348154410745082950806112487736e308L), DELTA574, 0, 0);
  check_complex ("ccos (0.75 - 710.5 i) == 1.347490911916428129246890157395342279438e308 + 1.255317763348154410745082950806112487736e308 i",  FUNC(ccos) (BUILD_COMPLEX (0.75, -710.5)), BUILD_COMPLEX (1.347490911916428129246890157395342279438e308L, 1.255317763348154410745082950806112487736e308L), DELTA575, 0, 0);
  check_complex ("ccos (-0.75 + 710.5 i) == 1.347490911916428129246890157395342279438e308 + 1.255317763348154410745082950806112487736e308 i",  FUNC(ccos) (BUILD_COMPLEX (-0.75, 710.5)), BUILD_COMPLEX (1.347490911916428129246890157395342279438e308L, 1.255317763348154410745082950806112487736e308L), DELTA576, 0, 0);
  check_complex ("ccos (-0.75 - 710.5 i) == 1.347490911916428129246890157395342279438e308 - 1.255317763348154410745082950806112487736e308 i",  FUNC(ccos) (BUILD_COMPLEX (-0.75, -710.5)), BUILD_COMPLEX (1.347490911916428129246890157395342279438e308L, -1.255317763348154410745082950806112487736e308L), DELTA577, 0, 0);
#endif

#if defined TEST_LDOUBLE && LDBL_MAX_EXP >= 16384
  check_complex ("ccos (0.75 + 11357.25 i) == 9.001213196851067077465606717616495588201e4931 - 8.385498349388321535962327491346664141020e4931 i",  FUNC(ccos) (BUILD_COMPLEX (0.75, 11357.25)), BUILD_COMPLEX (9.001213196851067077465606717616495588201e4931L, -8.385498349388321535962327491346664141020e4931L), 0, 0, 0);
  check_complex ("ccos (0.75 - 11357.25 i) == 9.001213196851067077465606717616495588201e4931 + 8.385498349388321535962327491346664141020e4931 i",  FUNC(ccos) (BUILD_COMPLEX (0.75, -11357.25)), BUILD_COMPLEX (9.001213196851067077465606717616495588201e4931L, 8.385498349388321535962327491346664141020e4931L), 0, 0, 0);
  check_complex ("ccos (-0.75 + 11357.25 i) == 9.001213196851067077465606717616495588201e4931 + 8.385498349388321535962327491346664141020e4931 i",  FUNC(ccos) (BUILD_COMPLEX (-0.75, 11357.25)), BUILD_COMPLEX (9.001213196851067077465606717616495588201e4931L, 8.385498349388321535962327491346664141020e4931L), 0, 0, 0);
  check_complex ("ccos (-0.75 - 11357.25 i) == 9.001213196851067077465606717616495588201e4931 - 8.385498349388321535962327491346664141020e4931 i",  FUNC(ccos) (BUILD_COMPLEX (-0.75, -11357.25)), BUILD_COMPLEX (9.001213196851067077465606717616495588201e4931L, -8.385498349388321535962327491346664141020e4931L), 0, 0, 0);
#endif

#ifdef TEST_FLOAT
  check_complex ("ccos (0x1p-149 + 180 i) == inf - 1.043535896672617552965983803453927655332e33 i",  FUNC(ccos) (BUILD_COMPLEX (0x1p-149, 180)), BUILD_COMPLEX (plus_infty, -1.043535896672617552965983803453927655332e33L), 0, 0, OVERFLOW_EXCEPTION);
#endif

#if defined TEST_DOUBLE || (defined TEST_LDOUBLE && LDBL_MAX_EXP == 1024)
  check_complex ("ccos (0x1p-1074 + 1440 i) == inf - 5.981479269486130556466515778180916082415e301 i",  FUNC(ccos) (BUILD_COMPLEX (0x1p-1074, 1440)), BUILD_COMPLEX (plus_infty, -5.981479269486130556466515778180916082415e301L), DELTA583, 0, OVERFLOW_EXCEPTION);
#endif

#if defined TEST_LDOUBLE && LDBL_MAX_EXP >= 16384
  check_complex ("ccos (0x1p-16434 + 22730 i) == inf - 1.217853148905605987081057582351152052687e4924 i",  FUNC(ccos) (BUILD_COMPLEX (0x1p-16434L, 22730)), BUILD_COMPLEX (plus_infty, -1.217853148905605987081057582351152052687e4924L), 0, 0, OVERFLOW_EXCEPTION);
#endif

  print_complex_max_error ("ccos", DELTAccos, 0);
}


static void
ccosh_test (void)
{
  errno = 0;
  FUNC(ccosh) (BUILD_COMPLEX (0.7L, 1.2L));
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_complex ("ccosh (0.0 + 0.0 i) == 1.0 + 0.0 i",  FUNC(ccosh) (BUILD_COMPLEX (0.0, 0.0)), BUILD_COMPLEX (1.0, 0.0), 0, 0, 0);
  check_complex ("ccosh (-0 + 0.0 i) == 1.0 - 0 i",  FUNC(ccosh) (BUILD_COMPLEX (minus_zero, 0.0)), BUILD_COMPLEX (1.0, minus_zero), 0, 0, 0);
  check_complex ("ccosh (0.0 - 0 i) == 1.0 - 0 i",  FUNC(ccosh) (BUILD_COMPLEX (0.0, minus_zero)), BUILD_COMPLEX (1.0, minus_zero), 0, 0, 0);
  check_complex ("ccosh (-0 - 0 i) == 1.0 + 0.0 i",  FUNC(ccosh) (BUILD_COMPLEX (minus_zero, minus_zero)), BUILD_COMPLEX (1.0, 0.0), 0, 0, 0);

  check_complex ("ccosh (0.0 + inf i) == NaN + 0.0 i",  FUNC(ccosh) (BUILD_COMPLEX (0.0, plus_infty)), BUILD_COMPLEX (nan_value, 0.0), 0, 0, INVALID_EXCEPTION|IGNORE_ZERO_INF_SIGN);
  check_complex ("ccosh (-0 + inf i) == NaN + 0.0 i",  FUNC(ccosh) (BUILD_COMPLEX (minus_zero, plus_infty)), BUILD_COMPLEX (nan_value, 0.0), 0, 0, INVALID_EXCEPTION|IGNORE_ZERO_INF_SIGN);
  check_complex ("ccosh (0.0 - inf i) == NaN + 0.0 i",  FUNC(ccosh) (BUILD_COMPLEX (0.0, minus_infty)), BUILD_COMPLEX (nan_value, 0.0), 0, 0, INVALID_EXCEPTION|IGNORE_ZERO_INF_SIGN);
  check_complex ("ccosh (-0 - inf i) == NaN + 0.0 i",  FUNC(ccosh) (BUILD_COMPLEX (minus_zero, minus_infty)), BUILD_COMPLEX (nan_value, 0.0), 0, 0, INVALID_EXCEPTION|IGNORE_ZERO_INF_SIGN);

  check_complex ("ccosh (inf + 0.0 i) == inf + 0.0 i",  FUNC(ccosh) (BUILD_COMPLEX (plus_infty, 0.0)), BUILD_COMPLEX (plus_infty, 0.0), 0, 0, 0);
  check_complex ("ccosh (-inf + 0.0 i) == inf - 0 i",  FUNC(ccosh) (BUILD_COMPLEX (minus_infty, 0.0)), BUILD_COMPLEX (plus_infty, minus_zero), 0, 0, 0);
  check_complex ("ccosh (inf - 0 i) == inf - 0 i",  FUNC(ccosh) (BUILD_COMPLEX (plus_infty, minus_zero)), BUILD_COMPLEX (plus_infty, minus_zero), 0, 0, 0);
  check_complex ("ccosh (-inf - 0 i) == inf + 0.0 i",  FUNC(ccosh) (BUILD_COMPLEX (minus_infty, minus_zero)), BUILD_COMPLEX (plus_infty, 0.0), 0, 0, 0);

  check_complex ("ccosh (inf + inf i) == inf + NaN i",  FUNC(ccosh) (BUILD_COMPLEX (plus_infty, plus_infty)), BUILD_COMPLEX (plus_infty, nan_value), 0, 0, INVALID_EXCEPTION);
  check_complex ("ccosh (-inf + inf i) == inf + NaN i",  FUNC(ccosh) (BUILD_COMPLEX (minus_infty, plus_infty)), BUILD_COMPLEX (plus_infty, nan_value), 0, 0, INVALID_EXCEPTION);
  check_complex ("ccosh (inf - inf i) == inf + NaN i",  FUNC(ccosh) (BUILD_COMPLEX (plus_infty, minus_infty)), BUILD_COMPLEX (plus_infty, nan_value), 0, 0, INVALID_EXCEPTION);
  check_complex ("ccosh (-inf - inf i) == inf + NaN i",  FUNC(ccosh) (BUILD_COMPLEX (minus_infty, minus_infty)), BUILD_COMPLEX (plus_infty, nan_value), 0, 0, INVALID_EXCEPTION);

  check_complex ("ccosh (inf + 4.625 i) == -inf - inf i",  FUNC(ccosh) (BUILD_COMPLEX (plus_infty, 4.625)), BUILD_COMPLEX (minus_infty, minus_infty), 0, 0, 0);
  check_complex ("ccosh (-inf + 4.625 i) == -inf + inf i",  FUNC(ccosh) (BUILD_COMPLEX (minus_infty, 4.625)), BUILD_COMPLEX (minus_infty, plus_infty), 0, 0, 0);
  check_complex ("ccosh (inf - 4.625 i) == -inf + inf i",  FUNC(ccosh) (BUILD_COMPLEX (plus_infty, -4.625)), BUILD_COMPLEX (minus_infty, plus_infty), 0, 0, 0);
  check_complex ("ccosh (-inf - 4.625 i) == -inf - inf i",  FUNC(ccosh) (BUILD_COMPLEX (minus_infty, -4.625)), BUILD_COMPLEX (minus_infty, minus_infty), 0, 0, 0);

  check_complex ("ccosh (6.75 + inf i) == NaN + NaN i",  FUNC(ccosh) (BUILD_COMPLEX (6.75, plus_infty)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);
  check_complex ("ccosh (-6.75 + inf i) == NaN + NaN i",  FUNC(ccosh) (BUILD_COMPLEX (-6.75, plus_infty)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);
  check_complex ("ccosh (6.75 - inf i) == NaN + NaN i",  FUNC(ccosh) (BUILD_COMPLEX (6.75, minus_infty)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);
  check_complex ("ccosh (-6.75 - inf i) == NaN + NaN i",  FUNC(ccosh) (BUILD_COMPLEX (-6.75, minus_infty)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);

  check_complex ("ccosh (0.0 + NaN i) == NaN + 0.0 i",  FUNC(ccosh) (BUILD_COMPLEX (0.0, nan_value)), BUILD_COMPLEX (nan_value, 0.0), 0, 0, IGNORE_ZERO_INF_SIGN);
  check_complex ("ccosh (-0 + NaN i) == NaN + 0.0 i",  FUNC(ccosh) (BUILD_COMPLEX (minus_zero, nan_value)), BUILD_COMPLEX (nan_value, 0.0), 0, 0, IGNORE_ZERO_INF_SIGN);

  check_complex ("ccosh (inf + NaN i) == inf + NaN i",  FUNC(ccosh) (BUILD_COMPLEX (plus_infty, nan_value)), BUILD_COMPLEX (plus_infty, nan_value), 0, 0, 0);
  check_complex ("ccosh (-inf + NaN i) == inf + NaN i",  FUNC(ccosh) (BUILD_COMPLEX (minus_infty, nan_value)), BUILD_COMPLEX (plus_infty, nan_value), 0, 0, 0);

  check_complex ("ccosh (9.0 + NaN i) == NaN + NaN i",  FUNC(ccosh) (BUILD_COMPLEX (9.0, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("ccosh (-9.0 + NaN i) == NaN + NaN i",  FUNC(ccosh) (BUILD_COMPLEX (-9.0, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);

  check_complex ("ccosh (NaN + 0.0 i) == NaN + 0.0 i",  FUNC(ccosh) (BUILD_COMPLEX (nan_value, 0.0)), BUILD_COMPLEX (nan_value, 0.0), 0, 0, IGNORE_ZERO_INF_SIGN);
  check_complex ("ccosh (NaN - 0 i) == NaN + 0.0 i",  FUNC(ccosh) (BUILD_COMPLEX (nan_value, minus_zero)), BUILD_COMPLEX (nan_value, 0.0), 0, 0, IGNORE_ZERO_INF_SIGN);

  check_complex ("ccosh (NaN + 10.0 i) == NaN + NaN i",  FUNC(ccosh) (BUILD_COMPLEX (nan_value, 10.0)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("ccosh (NaN - 10.0 i) == NaN + NaN i",  FUNC(ccosh) (BUILD_COMPLEX (nan_value, -10.0)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);

  check_complex ("ccosh (NaN + inf i) == NaN + NaN i",  FUNC(ccosh) (BUILD_COMPLEX (nan_value, plus_infty)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("ccosh (NaN - inf i) == NaN + NaN i",  FUNC(ccosh) (BUILD_COMPLEX (nan_value, minus_infty)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);

  check_complex ("ccosh (NaN + NaN i) == NaN + NaN i",  FUNC(ccosh) (BUILD_COMPLEX (nan_value, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, 0);

  check_complex ("ccosh (0.75 + 1.25 i) == 0.408242591877968807788852146397499084 + 0.780365930845853240391326216300863152 i",  FUNC(ccosh) (BUILD_COMPLEX (0.75L, 1.25L)), BUILD_COMPLEX (0.408242591877968807788852146397499084L, 0.780365930845853240391326216300863152L), DELTA622, 0, 0);

  check_complex ("ccosh (-2 - 3 i) == -3.72454550491532256547397070325597253 + 0.511822569987384608834463849801875634 i",  FUNC(ccosh) (BUILD_COMPLEX (-2, -3)), BUILD_COMPLEX (-3.72454550491532256547397070325597253L, 0.511822569987384608834463849801875634L), DELTA623, 0, 0);

  check_complex ("ccosh (89.5 + 0.75 i) == 2.708024460708609732016532185663087200560e38 + 2.522786001038096774676288412995370563339e38 i",  FUNC(ccosh) (BUILD_COMPLEX (89.5, 0.75)), BUILD_COMPLEX (2.708024460708609732016532185663087200560e38L, 2.522786001038096774676288412995370563339e38L), DELTA624, 0, 0);
  check_complex ("ccosh (-89.5 + 0.75 i) == 2.708024460708609732016532185663087200560e38 - 2.522786001038096774676288412995370563339e38 i",  FUNC(ccosh) (BUILD_COMPLEX (-89.5, 0.75)), BUILD_COMPLEX (2.708024460708609732016532185663087200560e38L, -2.522786001038096774676288412995370563339e38L), DELTA625, 0, 0);
  check_complex ("ccosh (89.5 - 0.75 i) == 2.708024460708609732016532185663087200560e38 - 2.522786001038096774676288412995370563339e38 i",  FUNC(ccosh) (BUILD_COMPLEX (89.5, -0.75)), BUILD_COMPLEX (2.708024460708609732016532185663087200560e38L, -2.522786001038096774676288412995370563339e38L), DELTA626, 0, 0);
  check_complex ("ccosh (-89.5 - 0.75 i) == 2.708024460708609732016532185663087200560e38 + 2.522786001038096774676288412995370563339e38 i",  FUNC(ccosh) (BUILD_COMPLEX (-89.5, -0.75)), BUILD_COMPLEX (2.708024460708609732016532185663087200560e38L, 2.522786001038096774676288412995370563339e38L), DELTA627, 0, 0);

#ifndef TEST_FLOAT
  check_complex ("ccosh (710.5 + 0.75 i) == 1.347490911916428129246890157395342279438e308 + 1.255317763348154410745082950806112487736e308 i",  FUNC(ccosh) (BUILD_COMPLEX (710.5, 0.75)), BUILD_COMPLEX (1.347490911916428129246890157395342279438e308L, 1.255317763348154410745082950806112487736e308L), DELTA628, 0, 0);
  check_complex ("ccosh (-710.5 + 0.75 i) == 1.347490911916428129246890157395342279438e308 - 1.255317763348154410745082950806112487736e308 i",  FUNC(ccosh) (BUILD_COMPLEX (-710.5, 0.75)), BUILD_COMPLEX (1.347490911916428129246890157395342279438e308L, -1.255317763348154410745082950806112487736e308L), DELTA629, 0, 0);
  check_complex ("ccosh (710.5 - 0.75 i) == 1.347490911916428129246890157395342279438e308 - 1.255317763348154410745082950806112487736e308 i",  FUNC(ccosh) (BUILD_COMPLEX (710.5, -0.75)), BUILD_COMPLEX (1.347490911916428129246890157395342279438e308L, -1.255317763348154410745082950806112487736e308L), DELTA630, 0, 0);
  check_complex ("ccosh (-710.5 - 0.75 i) == 1.347490911916428129246890157395342279438e308 + 1.255317763348154410745082950806112487736e308 i",  FUNC(ccosh) (BUILD_COMPLEX (-710.5, -0.75)), BUILD_COMPLEX (1.347490911916428129246890157395342279438e308L, 1.255317763348154410745082950806112487736e308L), DELTA631, 0, 0);
#endif

#if defined TEST_LDOUBLE && LDBL_MAX_EXP >= 16384
  check_complex ("ccosh (11357.25 + 0.75 i) == 9.001213196851067077465606717616495588201e4931 + 8.385498349388321535962327491346664141020e4931 i",  FUNC(ccosh) (BUILD_COMPLEX (11357.25, 0.75)), BUILD_COMPLEX (9.001213196851067077465606717616495588201e4931L, 8.385498349388321535962327491346664141020e4931L), 0, 0, 0);
  check_complex ("ccosh (-11357.25 + 0.75 i) == 9.001213196851067077465606717616495588201e4931 - 8.385498349388321535962327491346664141020e4931 i",  FUNC(ccosh) (BUILD_COMPLEX (-11357.25, 0.75)), BUILD_COMPLEX (9.001213196851067077465606717616495588201e4931L, -8.385498349388321535962327491346664141020e4931L), 0, 0, 0);
  check_complex ("ccosh (11357.25 - 0.75 i) == 9.001213196851067077465606717616495588201e4931 - 8.385498349388321535962327491346664141020e4931 i",  FUNC(ccosh) (BUILD_COMPLEX (11357.25, -0.75)), BUILD_COMPLEX (9.001213196851067077465606717616495588201e4931L, -8.385498349388321535962327491346664141020e4931L), 0, 0, 0);
  check_complex ("ccosh (-11357.25 - 0.75 i) == 9.001213196851067077465606717616495588201e4931 + 8.385498349388321535962327491346664141020e4931 i",  FUNC(ccosh) (BUILD_COMPLEX (-11357.25, -0.75)), BUILD_COMPLEX (9.001213196851067077465606717616495588201e4931L, 8.385498349388321535962327491346664141020e4931L), 0, 0, 0);
#endif

#ifdef TEST_FLOAT
  check_complex ("ccosh (180 + 0x1p-149 i) == inf + 1.043535896672617552965983803453927655332e33 i",  FUNC(ccosh) (BUILD_COMPLEX (180, 0x1p-149)), BUILD_COMPLEX (plus_infty, 1.043535896672617552965983803453927655332e33L), 0, 0, OVERFLOW_EXCEPTION);
#endif

#if defined TEST_DOUBLE || (defined TEST_LDOUBLE && LDBL_MAX_EXP == 1024)
  check_complex ("ccosh (1440 + 0x1p-1074 i) == inf + 5.981479269486130556466515778180916082415e301 i",  FUNC(ccosh) (BUILD_COMPLEX (1440, 0x1p-1074)), BUILD_COMPLEX (plus_infty, 5.981479269486130556466515778180916082415e301L), DELTA637, 0, OVERFLOW_EXCEPTION);
#endif

#if defined TEST_LDOUBLE && LDBL_MAX_EXP >= 16384
  check_complex ("ccosh (22730 + 0x1p-16434 i) == inf + 1.217853148905605987081057582351152052687e4924 i",  FUNC(ccosh) (BUILD_COMPLEX (22730, 0x1p-16434L)), BUILD_COMPLEX (plus_infty, 1.217853148905605987081057582351152052687e4924L), 0, 0, OVERFLOW_EXCEPTION);
#endif

  print_complex_max_error ("ccosh", DELTAccosh, 0);
}


static void
ceil_test (void)
{
  init_max_error ();

  check_float ("ceil (0.0) == 0.0",  FUNC(ceil) (0.0), 0.0, 0, 0, 0);
  check_float ("ceil (-0) == -0",  FUNC(ceil) (minus_zero), minus_zero, 0, 0, 0);
  check_float ("ceil (inf) == inf",  FUNC(ceil) (plus_infty), plus_infty, 0, 0, 0);
  check_float ("ceil (-inf) == -inf",  FUNC(ceil) (minus_infty), minus_infty, 0, 0, 0);
  check_float ("ceil (NaN) == NaN",  FUNC(ceil) (nan_value), nan_value, 0, 0, 0);

  check_float ("ceil (pi) == 4.0",  FUNC(ceil) (M_PIl), 4.0, 0, 0, 0);
  check_float ("ceil (-pi) == -3.0",  FUNC(ceil) (-M_PIl), -3.0, 0, 0, 0);
  check_float ("ceil (0.1) == 1.0",  FUNC(ceil) (0.1), 1.0, 0, 0, 0);
  check_float ("ceil (0.25) == 1.0",  FUNC(ceil) (0.25), 1.0, 0, 0, 0);
  check_float ("ceil (0.625) == 1.0",  FUNC(ceil) (0.625), 1.0, 0, 0, 0);
  check_float ("ceil (-0.1) == -0",  FUNC(ceil) (-0.1), minus_zero, 0, 0, 0);
  check_float ("ceil (-0.25) == -0",  FUNC(ceil) (-0.25), minus_zero, 0, 0, 0);
  check_float ("ceil (-0.625) == -0",  FUNC(ceil) (-0.625), minus_zero, 0, 0, 0);

#ifdef TEST_LDOUBLE
  /* The result can only be represented in long double.  */
  check_float ("ceil (4503599627370495.5) == 4503599627370496.0",  FUNC(ceil) (4503599627370495.5L), 4503599627370496.0L, 0, 0, 0);
  check_float ("ceil (4503599627370496.25) == 4503599627370497.0",  FUNC(ceil) (4503599627370496.25L), 4503599627370497.0L, 0, 0, 0);
  check_float ("ceil (4503599627370496.5) == 4503599627370497.0",  FUNC(ceil) (4503599627370496.5L), 4503599627370497.0L, 0, 0, 0);
  check_float ("ceil (4503599627370496.75) == 4503599627370497.0",  FUNC(ceil) (4503599627370496.75L), 4503599627370497.0L, 0, 0, 0);
  check_float ("ceil (4503599627370497.5) == 4503599627370498.0",  FUNC(ceil) (4503599627370497.5L), 4503599627370498.0L, 0, 0, 0);

  check_float ("ceil (-4503599627370495.5) == -4503599627370495.0",  FUNC(ceil) (-4503599627370495.5L), -4503599627370495.0L, 0, 0, 0);
  check_float ("ceil (-4503599627370496.25) == -4503599627370496.0",  FUNC(ceil) (-4503599627370496.25L), -4503599627370496.0L, 0, 0, 0);
  check_float ("ceil (-4503599627370496.5) == -4503599627370496.0",  FUNC(ceil) (-4503599627370496.5L), -4503599627370496.0L, 0, 0, 0);
  check_float ("ceil (-4503599627370496.75) == -4503599627370496.0",  FUNC(ceil) (-4503599627370496.75L), -4503599627370496.0L, 0, 0, 0);
  check_float ("ceil (-4503599627370497.5) == -4503599627370497.0",  FUNC(ceil) (-4503599627370497.5L), -4503599627370497.0L, 0, 0, 0);

# if LDBL_MANT_DIG > 100
  check_float ("ceil (4503599627370494.5000000000001) == 4503599627370495.0",  FUNC(ceil) (4503599627370494.5000000000001L), 4503599627370495.0L, 0, 0, 0);
  check_float ("ceil (4503599627370495.5000000000001) == 4503599627370496.0",  FUNC(ceil) (4503599627370495.5000000000001L), 4503599627370496.0L, 0, 0, 0);
  check_float ("ceil (4503599627370496.5000000000001) == 4503599627370497.0",  FUNC(ceil) (4503599627370496.5000000000001L), 4503599627370497.0L, 0, 0, 0);
  check_float ("ceil (-4503599627370494.5000000000001) == -4503599627370494.0",  FUNC(ceil) (-4503599627370494.5000000000001L), -4503599627370494.0L, 0, 0, 0);
  check_float ("ceil (-4503599627370495.5000000000001) == -4503599627370495.0",  FUNC(ceil) (-4503599627370495.5000000000001L), -4503599627370495.0L, 0, 0, 0);
  check_float ("ceil (-4503599627370496.5000000000001) == -4503599627370496.0",  FUNC(ceil) (-4503599627370496.5000000000001L), -4503599627370496.0L, 0, 0, 0);
# endif

  check_float ("ceil (9007199254740991.5) == 9007199254740992.0",  FUNC(ceil) (9007199254740991.5L), 9007199254740992.0L, 0, 0, 0);
  check_float ("ceil (9007199254740992.25) == 9007199254740993.0",  FUNC(ceil) (9007199254740992.25L), 9007199254740993.0L, 0, 0, 0);
  check_float ("ceil (9007199254740992.5) == 9007199254740993.0",  FUNC(ceil) (9007199254740992.5L), 9007199254740993.0L, 0, 0, 0);
  check_float ("ceil (9007199254740992.75) == 9007199254740993.0",  FUNC(ceil) (9007199254740992.75L), 9007199254740993.0L, 0, 0, 0);
  check_float ("ceil (9007199254740993.5) == 9007199254740994.0",  FUNC(ceil) (9007199254740993.5L), 9007199254740994.0L, 0, 0, 0);

  check_float ("ceil (-9007199254740991.5) == -9007199254740991.0",  FUNC(ceil) (-9007199254740991.5L), -9007199254740991.0L, 0, 0, 0);
  check_float ("ceil (-9007199254740992.25) == -9007199254740992.0",  FUNC(ceil) (-9007199254740992.25L), -9007199254740992.0L, 0, 0, 0);
  check_float ("ceil (-9007199254740992.5) == -9007199254740992.0",  FUNC(ceil) (-9007199254740992.5L), -9007199254740992.0L, 0, 0, 0);
  check_float ("ceil (-9007199254740992.75) == -9007199254740992.0",  FUNC(ceil) (-9007199254740992.75L), -9007199254740992.0L, 0, 0, 0);
  check_float ("ceil (-9007199254740993.5) == -9007199254740993.0",  FUNC(ceil) (-9007199254740993.5L), -9007199254740993.0L, 0, 0, 0);

# if LDBL_MANT_DIG > 100
  check_float ("ceil (9007199254740991.0000000000001) == 9007199254740992.0",  FUNC(ceil) (9007199254740991.0000000000001L), 9007199254740992.0L, 0, 0, 0);
  check_float ("ceil (9007199254740992.0000000000001) == 9007199254740993.0",  FUNC(ceil) (9007199254740992.0000000000001L), 9007199254740993.0L, 0, 0, 0);
  check_float ("ceil (9007199254740993.0000000000001) == 9007199254740994.0",  FUNC(ceil) (9007199254740993.0000000000001L), 9007199254740994.0L, 0, 0, 0);
  check_float ("ceil (9007199254740991.5000000000001) == 9007199254740992.0",  FUNC(ceil) (9007199254740991.5000000000001L), 9007199254740992.0L, 0, 0, 0);
  check_float ("ceil (9007199254740992.5000000000001) == 9007199254740993.0",  FUNC(ceil) (9007199254740992.5000000000001L), 9007199254740993.0L, 0, 0, 0);
  check_float ("ceil (9007199254740993.5000000000001) == 9007199254740994.0",  FUNC(ceil) (9007199254740993.5000000000001L), 9007199254740994.0L, 0, 0, 0);

  check_float ("ceil (-9007199254740991.0000000000001) == -9007199254740991.0",  FUNC(ceil) (-9007199254740991.0000000000001L), -9007199254740991.0L, 0, 0, 0);
  check_float ("ceil (-9007199254740992.0000000000001) == -9007199254740992.0",  FUNC(ceil) (-9007199254740992.0000000000001L), -9007199254740992.0L, 0, 0, 0);
  check_float ("ceil (-9007199254740993.0000000000001) == -9007199254740993.0",  FUNC(ceil) (-9007199254740993.0000000000001L), -9007199254740993.0L, 0, 0, 0);
  check_float ("ceil (-9007199254740991.5000000000001) == -9007199254740991.0",  FUNC(ceil) (-9007199254740991.5000000000001L), -9007199254740991.0L, 0, 0, 0);
  check_float ("ceil (-9007199254740992.5000000000001) == -9007199254740992.0",  FUNC(ceil) (-9007199254740992.5000000000001L), -9007199254740992.0L, 0, 0, 0);
  check_float ("ceil (-9007199254740993.5000000000001) == -9007199254740993.0",  FUNC(ceil) (-9007199254740993.5000000000001L), -9007199254740993.0L, 0, 0, 0);
# endif

  check_float ("ceil (72057594037927935.5) == 72057594037927936.0",  FUNC(ceil) (72057594037927935.5L), 72057594037927936.0L, 0, 0, 0);
  check_float ("ceil (72057594037927936.25) == 72057594037927937.0",  FUNC(ceil) (72057594037927936.25L), 72057594037927937.0L, 0, 0, 0);
  check_float ("ceil (72057594037927936.5) == 72057594037927937.0",  FUNC(ceil) (72057594037927936.5L), 72057594037927937.0L, 0, 0, 0);
  check_float ("ceil (72057594037927936.75) == 72057594037927937.0",  FUNC(ceil) (72057594037927936.75L), 72057594037927937.0L, 0, 0, 0);
  check_float ("ceil (72057594037927937.5) == 72057594037927938.0",  FUNC(ceil) (72057594037927937.5L), 72057594037927938.0L, 0, 0, 0);

  check_float ("ceil (-72057594037927935.5) == -72057594037927935.0",  FUNC(ceil) (-72057594037927935.5L), -72057594037927935.0L, 0, 0, 0);
  check_float ("ceil (-72057594037927936.25) == -72057594037927936.0",  FUNC(ceil) (-72057594037927936.25L), -72057594037927936.0L, 0, 0, 0);
  check_float ("ceil (-72057594037927936.5) == -72057594037927936.0",  FUNC(ceil) (-72057594037927936.5L), -72057594037927936.0L, 0, 0, 0);
  check_float ("ceil (-72057594037927936.75) == -72057594037927936.0",  FUNC(ceil) (-72057594037927936.75L), -72057594037927936.0L, 0, 0, 0);
  check_float ("ceil (-72057594037927937.5) == -72057594037927937.0",  FUNC(ceil) (-72057594037927937.5L), -72057594037927937.0L, 0, 0, 0);

  check_float ("ceil (10141204801825835211973625643007.5) == 10141204801825835211973625643008.0",  FUNC(ceil) (10141204801825835211973625643007.5L), 10141204801825835211973625643008.0L, 0, 0, 0);
  check_float ("ceil (10141204801825835211973625643008.25) == 10141204801825835211973625643009.0",  FUNC(ceil) (10141204801825835211973625643008.25L), 10141204801825835211973625643009.0L, 0, 0, 0);
  check_float ("ceil (10141204801825835211973625643008.5) == 10141204801825835211973625643009.0",  FUNC(ceil) (10141204801825835211973625643008.5L), 10141204801825835211973625643009.0L, 0, 0, 0);
  check_float ("ceil (10141204801825835211973625643008.75) == 10141204801825835211973625643009.0",  FUNC(ceil) (10141204801825835211973625643008.75L), 10141204801825835211973625643009.0L, 0, 0, 0);
  check_float ("ceil (10141204801825835211973625643009.5) == 10141204801825835211973625643010.0",  FUNC(ceil) (10141204801825835211973625643009.5L), 10141204801825835211973625643010.0L, 0, 0, 0);
#endif

  print_max_error ("ceil", 0, 0);
}


static void
cexp_test (void)
{
  errno = 0;
  FUNC(cexp) (BUILD_COMPLEX (0, 0));
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_complex ("cexp (+0 + +0 i) == 1 + 0.0 i",  FUNC(cexp) (BUILD_COMPLEX (plus_zero, plus_zero)), BUILD_COMPLEX (1, 0.0), 0, 0, 0);
  check_complex ("cexp (-0 + +0 i) == 1 + 0.0 i",  FUNC(cexp) (BUILD_COMPLEX (minus_zero, plus_zero)), BUILD_COMPLEX (1, 0.0), 0, 0, 0);
  check_complex ("cexp (+0 - 0 i) == 1 - 0 i",  FUNC(cexp) (BUILD_COMPLEX (plus_zero, minus_zero)), BUILD_COMPLEX (1, minus_zero), 0, 0, 0);
  check_complex ("cexp (-0 - 0 i) == 1 - 0 i",  FUNC(cexp) (BUILD_COMPLEX (minus_zero, minus_zero)), BUILD_COMPLEX (1, minus_zero), 0, 0, 0);

  check_complex ("cexp (inf + +0 i) == inf + 0.0 i",  FUNC(cexp) (BUILD_COMPLEX (plus_infty, plus_zero)), BUILD_COMPLEX (plus_infty, 0.0), 0, 0, 0);
  check_complex ("cexp (inf - 0 i) == inf - 0 i",  FUNC(cexp) (BUILD_COMPLEX (plus_infty, minus_zero)), BUILD_COMPLEX (plus_infty, minus_zero), 0, 0, 0);

  check_complex ("cexp (-inf + +0 i) == 0.0 + 0.0 i",  FUNC(cexp) (BUILD_COMPLEX (minus_infty, plus_zero)), BUILD_COMPLEX (0.0, 0.0), 0, 0, 0);
  check_complex ("cexp (-inf - 0 i) == 0.0 - 0 i",  FUNC(cexp) (BUILD_COMPLEX (minus_infty, minus_zero)), BUILD_COMPLEX (0.0, minus_zero), 0, 0, 0);

  check_complex ("cexp (0.0 + inf i) == NaN + NaN i",  FUNC(cexp) (BUILD_COMPLEX (0.0, plus_infty)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);
  check_complex ("cexp (-0 + inf i) == NaN + NaN i",  FUNC(cexp) (BUILD_COMPLEX (minus_zero, plus_infty)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);

  check_complex ("cexp (0.0 - inf i) == NaN + NaN i",  FUNC(cexp) (BUILD_COMPLEX (0.0, minus_infty)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);
  check_complex ("cexp (-0 - inf i) == NaN + NaN i",  FUNC(cexp) (BUILD_COMPLEX (minus_zero, minus_infty)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);

  check_complex ("cexp (100.0 + inf i) == NaN + NaN i",  FUNC(cexp) (BUILD_COMPLEX (100.0, plus_infty)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);
  check_complex ("cexp (-100.0 + inf i) == NaN + NaN i",  FUNC(cexp) (BUILD_COMPLEX (-100.0, plus_infty)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);

  check_complex ("cexp (100.0 - inf i) == NaN + NaN i",  FUNC(cexp) (BUILD_COMPLEX (100.0, minus_infty)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);
  check_complex ("cexp (-100.0 - inf i) == NaN + NaN i",  FUNC(cexp) (BUILD_COMPLEX (-100.0, minus_infty)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);

  check_complex ("cexp (-inf + 2.0 i) == -0 + 0.0 i",  FUNC(cexp) (BUILD_COMPLEX (minus_infty, 2.0)), BUILD_COMPLEX (minus_zero, 0.0), 0, 0, 0);
  check_complex ("cexp (-inf + 4.0 i) == -0 - 0 i",  FUNC(cexp) (BUILD_COMPLEX (minus_infty, 4.0)), BUILD_COMPLEX (minus_zero, minus_zero), 0, 0, 0);
  check_complex ("cexp (inf + 2.0 i) == -inf + inf i",  FUNC(cexp) (BUILD_COMPLEX (plus_infty, 2.0)), BUILD_COMPLEX (minus_infty, plus_infty), 0, 0, 0);
  check_complex ("cexp (inf + 4.0 i) == -inf - inf i",  FUNC(cexp) (BUILD_COMPLEX (plus_infty, 4.0)), BUILD_COMPLEX (minus_infty, minus_infty), 0, 0, 0);

  check_complex ("cexp (inf + inf i) == inf + NaN i",  FUNC(cexp) (BUILD_COMPLEX (plus_infty, plus_infty)), BUILD_COMPLEX (plus_infty, nan_value), 0, 0, INVALID_EXCEPTION|IGNORE_ZERO_INF_SIGN);
  check_complex ("cexp (inf - inf i) == inf + NaN i",  FUNC(cexp) (BUILD_COMPLEX (plus_infty, minus_infty)), BUILD_COMPLEX (plus_infty, nan_value), 0, 0, INVALID_EXCEPTION|IGNORE_ZERO_INF_SIGN);

  check_complex ("cexp (-inf + inf i) == 0.0 + 0.0 i",  FUNC(cexp) (BUILD_COMPLEX (minus_infty, plus_infty)), BUILD_COMPLEX (0.0, 0.0), 0, 0, IGNORE_ZERO_INF_SIGN);
  check_complex ("cexp (-inf - inf i) == 0.0 - 0 i",  FUNC(cexp) (BUILD_COMPLEX (minus_infty, minus_infty)), BUILD_COMPLEX (0.0, minus_zero), 0, 0, IGNORE_ZERO_INF_SIGN);

  check_complex ("cexp (-inf + NaN i) == 0 + 0 i",  FUNC(cexp) (BUILD_COMPLEX (minus_infty, nan_value)), BUILD_COMPLEX (0, 0), 0, 0, IGNORE_ZERO_INF_SIGN);

  check_complex ("cexp (inf + NaN i) == inf + NaN i",  FUNC(cexp) (BUILD_COMPLEX (plus_infty, nan_value)), BUILD_COMPLEX (plus_infty, nan_value), 0, 0, 0);

  check_complex ("cexp (NaN + 0.0 i) == NaN + NaN i",  FUNC(cexp) (BUILD_COMPLEX (nan_value, 0.0)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("cexp (NaN + 1.0 i) == NaN + NaN i",  FUNC(cexp) (BUILD_COMPLEX (nan_value, 1.0)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);

  check_complex ("cexp (NaN + inf i) == NaN + NaN i",  FUNC(cexp) (BUILD_COMPLEX (nan_value, plus_infty)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("cexp (0 + NaN i) == NaN + NaN i",  FUNC(cexp) (BUILD_COMPLEX (0, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("cexp (1 + NaN i) == NaN + NaN i",  FUNC(cexp) (BUILD_COMPLEX (1, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("cexp (NaN + NaN i) == NaN + NaN i",  FUNC(cexp) (BUILD_COMPLEX (nan_value, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, 0);

  check_complex ("cexp (0.75 + 1.25 i) == 0.667537446429131586942201977015932112 + 2.00900045494094876258347228145863909 i",  FUNC(cexp) (BUILD_COMPLEX (0.75L, 1.25L)), BUILD_COMPLEX (0.667537446429131586942201977015932112L, 2.00900045494094876258347228145863909L), DELTA737, 0, 0);
  check_complex ("cexp (-2.0 - 3.0 i) == -0.13398091492954261346140525546115575 - 0.019098516261135196432576240858800925 i",  FUNC(cexp) (BUILD_COMPLEX (-2.0, -3.0)), BUILD_COMPLEX (-0.13398091492954261346140525546115575L, -0.019098516261135196432576240858800925L), DELTA738, 0, 0);

  check_complex ("cexp (0 + 0x1p65 i) == 0.99888622066058013610642172179340364209972 - 0.047183876212354673805106149805700013943218 i",  FUNC(cexp) (BUILD_COMPLEX (0, 0x1p65)), BUILD_COMPLEX (0.99888622066058013610642172179340364209972L, -0.047183876212354673805106149805700013943218L), 0, 0, 0);
  check_complex ("cexp (0 - 0x1p65 i) == 0.99888622066058013610642172179340364209972 + 0.047183876212354673805106149805700013943218 i",  FUNC(cexp) (BUILD_COMPLEX (0, -0x1p65)), BUILD_COMPLEX (0.99888622066058013610642172179340364209972L, 0.047183876212354673805106149805700013943218L), 0, 0, 0);
  check_complex ("cexp (50 + 0x1p127 i) == 4.053997150228616856622417636046265337193e21 + 3.232070315463388524466674772633810238819e21 i",  FUNC(cexp) (BUILD_COMPLEX (50, 0x1p127)), BUILD_COMPLEX (4.053997150228616856622417636046265337193e21L, 3.232070315463388524466674772633810238819e21L), DELTA741, 0, 0);

#ifndef TEST_FLOAT
  check_complex ("cexp (0 + 1e22 i) == 0.5232147853951389454975944733847094921409 - 0.8522008497671888017727058937530293682618 i",  FUNC(cexp) (BUILD_COMPLEX (0, 1e22)), BUILD_COMPLEX (0.5232147853951389454975944733847094921409L, -0.8522008497671888017727058937530293682618L), 0, 0, 0);
  check_complex ("cexp (0 + 0x1p1023 i) == -0.826369834614147994500785680811743734805 + 0.5631277798508840134529434079444683477104 i",  FUNC(cexp) (BUILD_COMPLEX (0, 0x1p1023)), BUILD_COMPLEX (-0.826369834614147994500785680811743734805L, 0.5631277798508840134529434079444683477104L), 0, 0, 0);
  check_complex ("cexp (500 + 0x1p1023 i) == -1.159886268932754433233243794561351783426e217 + 7.904017694554466595359379965081774849708e216 i",  FUNC(cexp) (BUILD_COMPLEX (500, 0x1p1023)), BUILD_COMPLEX (-1.159886268932754433233243794561351783426e217L, 7.904017694554466595359379965081774849708e216L), DELTA744, 0, 0);
#endif

#if defined TEST_LDOUBLE && LDBL_MAX_EXP >= 16384
  check_complex ("cexp (0 + 0x1p16383 i) == 0.9210843909921906206874509522505756251609 + 0.3893629985894208126948115852610595405563 i",  FUNC(cexp) (BUILD_COMPLEX (0, 0x1p16383L)), BUILD_COMPLEX (0.9210843909921906206874509522505756251609L, 0.3893629985894208126948115852610595405563L), 0, 0, 0);
  check_complex ("cexp (-10000 + 0x1p16383 i) == 1.045876464564882298442774542991176546722e-4343 + 4.421154026488516836023811173959413420548e-4344 i",  FUNC(cexp) (BUILD_COMPLEX (-10000, 0x1p16383L)), BUILD_COMPLEX (1.045876464564882298442774542991176546722e-4343L, 4.421154026488516836023811173959413420548e-4344L), DELTA746, 0, 0);
#endif

  check_complex ("cexp (88.75 + 0.75 i) == 2.558360358486542817001900410314204322891e38 + 2.383359453227311447654736314679677655100e38 i",  FUNC(cexp) (BUILD_COMPLEX (88.75, 0.75)), BUILD_COMPLEX (2.558360358486542817001900410314204322891e38L, 2.383359453227311447654736314679677655100e38L), DELTA747, 0, 0);
  check_complex ("cexp (-95 + 0.75 i) == 4.039714446238306526889476684000081624047e-42 + 3.763383677300535390271646960780570275931e-42 i",  FUNC(cexp) (BUILD_COMPLEX (-95, 0.75)), BUILD_COMPLEX (4.039714446238306526889476684000081624047e-42L, 3.763383677300535390271646960780570275931e-42L), DELTA748, 0, UNDERFLOW_EXCEPTION_FLOAT);

#ifndef TEST_FLOAT
  check_complex ("cexp (709.8125 + 0.75 i) == 1.355121963080879535248452862759108365762e308 + 1.262426823598609432507811340856186873507e308 i",  FUNC(cexp) (BUILD_COMPLEX (709.8125, 0.75)), BUILD_COMPLEX (1.355121963080879535248452862759108365762e308L, 1.262426823598609432507811340856186873507e308L), DELTA749, 0, 0);
  check_complex ("cexp (-720 + 0.75 i) == 1.486960657116368433685753325516638551722e-313 + 1.385247284245720590980701226843815229385e-313 i",  FUNC(cexp) (BUILD_COMPLEX (-720, 0.75)), BUILD_COMPLEX (1.486960657116368433685753325516638551722e-313L, 1.385247284245720590980701226843815229385e-313L), 0, 0, UNDERFLOW_EXCEPTION_DOUBLE);
#endif

#if defined TEST_LDOUBLE && LDBL_MAX_EXP >= 16384
  check_complex ("cexp (11356.5625 + 0.75 i) == 9.052188470850960144814815984311663764287e4931 + 8.432986734191301036267148978260970230200e4931 i",  FUNC(cexp) (BUILD_COMPLEX (11356.5625, 0.75)), BUILD_COMPLEX (9.052188470850960144814815984311663764287e4931L, 8.432986734191301036267148978260970230200e4931L), DELTA751, 0, 0);
  check_complex ("cexp (-11370 + 0.75 i) == 8.631121063182211587489310508568170739592e-4939 + 8.040721827809267291427062346918413482824e-4939 i",  FUNC(cexp) (BUILD_COMPLEX (-11370, 0.75)), BUILD_COMPLEX (8.631121063182211587489310508568170739592e-4939L, 8.040721827809267291427062346918413482824e-4939L), 0, 0, UNDERFLOW_EXCEPTION);
#endif

#ifdef TEST_FLOAT
  check_complex ("cexp (180 + 0x1p-149 i) == inf + 2.087071793345235105931967606907855310664e33 i",  FUNC(cexp) (BUILD_COMPLEX (180, 0x1p-149)), BUILD_COMPLEX (plus_infty, 2.087071793345235105931967606907855310664e33L), 0, 0, OVERFLOW_EXCEPTION);
#endif

#if defined TEST_DOUBLE || (defined TEST_LDOUBLE && LDBL_MAX_EXP == 1024)
  check_complex ("cexp (1440 + 0x1p-1074 i) == inf + 1.196295853897226111293303155636183216483e302 i",  FUNC(cexp) (BUILD_COMPLEX (1440, 0x1p-1074)), BUILD_COMPLEX (plus_infty, 1.196295853897226111293303155636183216483e302L), DELTA754, 0, OVERFLOW_EXCEPTION);
#endif

#if defined TEST_LDOUBLE && LDBL_MAX_EXP >= 16384
  check_complex ("cexp (22730 + 0x1p-16434 i) == inf + 2.435706297811211974162115164702304105374e4924 i",  FUNC(cexp) (BUILD_COMPLEX (22730, 0x1p-16434L)), BUILD_COMPLEX (plus_infty, 2.435706297811211974162115164702304105374e4924L), 0, 0, OVERFLOW_EXCEPTION);
#endif

  check_complex ("cexp (1e6 + 0 i) == inf + 0 i",  FUNC(cexp) (BUILD_COMPLEX (1e6, 0)), BUILD_COMPLEX (plus_infty, 0), 0, 0, OVERFLOW_EXCEPTION);
  check_complex ("cexp (1e6 + min_value i) == inf + inf i",  FUNC(cexp) (BUILD_COMPLEX (1e6, min_value)), BUILD_COMPLEX (plus_infty, plus_infty), 0, 0, OVERFLOW_EXCEPTION);
  check_complex ("cexp (1e6 - min_value i) == inf - inf i",  FUNC(cexp) (BUILD_COMPLEX (1e6, -min_value)), BUILD_COMPLEX (plus_infty, minus_infty), 0, 0, OVERFLOW_EXCEPTION);

  print_complex_max_error ("cexp", DELTAcexp, 0);
}


static void
cimag_test (void)
{
  init_max_error ();
  check_float ("cimag (1.0 + 0.0 i) == 0.0",  FUNC(cimag) (BUILD_COMPLEX (1.0, 0.0)), 0.0, 0, 0, 0);
  check_float ("cimag (1.0 - 0 i) == -0",  FUNC(cimag) (BUILD_COMPLEX (1.0, minus_zero)), minus_zero, 0, 0, 0);
  check_float ("cimag (1.0 + NaN i) == NaN",  FUNC(cimag) (BUILD_COMPLEX (1.0, nan_value)), nan_value, 0, 0, 0);
  check_float ("cimag (NaN + NaN i) == NaN",  FUNC(cimag) (BUILD_COMPLEX (nan_value, nan_value)), nan_value, 0, 0, 0);
  check_float ("cimag (1.0 + inf i) == inf",  FUNC(cimag) (BUILD_COMPLEX (1.0, plus_infty)), plus_infty, 0, 0, 0);
  check_float ("cimag (1.0 - inf i) == -inf",  FUNC(cimag) (BUILD_COMPLEX (1.0, minus_infty)), minus_infty, 0, 0, 0);
  check_float ("cimag (2.0 + 3.0 i) == 3.0",  FUNC(cimag) (BUILD_COMPLEX (2.0, 3.0)), 3.0, 0, 0, 0);

  print_max_error ("cimag", 0, 0);
}

static void
clog_test (void)
{
  errno = 0;
  FUNC(clog) (BUILD_COMPLEX (-2, -3));
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_complex ("clog (-0 + 0 i) == -inf + pi i",  FUNC(clog) (BUILD_COMPLEX (minus_zero, 0)), BUILD_COMPLEX (minus_infty, M_PIl), 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_complex ("clog (-0 - 0 i) == -inf - pi i",  FUNC(clog) (BUILD_COMPLEX (minus_zero, minus_zero)), BUILD_COMPLEX (minus_infty, -M_PIl), 0, 0, DIVIDE_BY_ZERO_EXCEPTION);

  check_complex ("clog (0 + 0 i) == -inf + 0.0 i",  FUNC(clog) (BUILD_COMPLEX (0, 0)), BUILD_COMPLEX (minus_infty, 0.0), 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_complex ("clog (0 - 0 i) == -inf - 0 i",  FUNC(clog) (BUILD_COMPLEX (0, minus_zero)), BUILD_COMPLEX (minus_infty, minus_zero), 0, 0, DIVIDE_BY_ZERO_EXCEPTION);

  check_complex ("clog (-inf + inf i) == inf + 3/4 pi i",  FUNC(clog) (BUILD_COMPLEX (minus_infty, plus_infty)), BUILD_COMPLEX (plus_infty, M_PI_34l), 0, 0, 0);
  check_complex ("clog (-inf - inf i) == inf - 3/4 pi i",  FUNC(clog) (BUILD_COMPLEX (minus_infty, minus_infty)), BUILD_COMPLEX (plus_infty, -M_PI_34l), 0, 0, 0);

  check_complex ("clog (inf + inf i) == inf + pi/4 i",  FUNC(clog) (BUILD_COMPLEX (plus_infty, plus_infty)), BUILD_COMPLEX (plus_infty, M_PI_4l), 0, 0, 0);
  check_complex ("clog (inf - inf i) == inf - pi/4 i",  FUNC(clog) (BUILD_COMPLEX (plus_infty, minus_infty)), BUILD_COMPLEX (plus_infty, -M_PI_4l), 0, 0, 0);

  check_complex ("clog (0 + inf i) == inf + pi/2 i",  FUNC(clog) (BUILD_COMPLEX (0, plus_infty)), BUILD_COMPLEX (plus_infty, M_PI_2l), 0, 0, 0);
  check_complex ("clog (3 + inf i) == inf + pi/2 i",  FUNC(clog) (BUILD_COMPLEX (3, plus_infty)), BUILD_COMPLEX (plus_infty, M_PI_2l), 0, 0, 0);
  check_complex ("clog (-0 + inf i) == inf + pi/2 i",  FUNC(clog) (BUILD_COMPLEX (minus_zero, plus_infty)), BUILD_COMPLEX (plus_infty, M_PI_2l), 0, 0, 0);
  check_complex ("clog (-3 + inf i) == inf + pi/2 i",  FUNC(clog) (BUILD_COMPLEX (-3, plus_infty)), BUILD_COMPLEX (plus_infty, M_PI_2l), 0, 0, 0);
  check_complex ("clog (0 - inf i) == inf - pi/2 i",  FUNC(clog) (BUILD_COMPLEX (0, minus_infty)), BUILD_COMPLEX (plus_infty, -M_PI_2l), 0, 0, 0);
  check_complex ("clog (3 - inf i) == inf - pi/2 i",  FUNC(clog) (BUILD_COMPLEX (3, minus_infty)), BUILD_COMPLEX (plus_infty, -M_PI_2l), 0, 0, 0);
  check_complex ("clog (-0 - inf i) == inf - pi/2 i",  FUNC(clog) (BUILD_COMPLEX (minus_zero, minus_infty)), BUILD_COMPLEX (plus_infty, -M_PI_2l), 0, 0, 0);
  check_complex ("clog (-3 - inf i) == inf - pi/2 i",  FUNC(clog) (BUILD_COMPLEX (-3, minus_infty)), BUILD_COMPLEX (plus_infty, -M_PI_2l), 0, 0, 0);

  check_complex ("clog (-inf + 0 i) == inf + pi i",  FUNC(clog) (BUILD_COMPLEX (minus_infty, 0)), BUILD_COMPLEX (plus_infty, M_PIl), 0, 0, 0);
  check_complex ("clog (-inf + 1 i) == inf + pi i",  FUNC(clog) (BUILD_COMPLEX (minus_infty, 1)), BUILD_COMPLEX (plus_infty, M_PIl), 0, 0, 0);
  check_complex ("clog (-inf - 0 i) == inf - pi i",  FUNC(clog) (BUILD_COMPLEX (minus_infty, minus_zero)), BUILD_COMPLEX (plus_infty, -M_PIl), 0, 0, 0);
  check_complex ("clog (-inf - 1 i) == inf - pi i",  FUNC(clog) (BUILD_COMPLEX (minus_infty, -1)), BUILD_COMPLEX (plus_infty, -M_PIl), 0, 0, 0);

  check_complex ("clog (inf + 0 i) == inf + 0.0 i",  FUNC(clog) (BUILD_COMPLEX (plus_infty, 0)), BUILD_COMPLEX (plus_infty, 0.0), 0, 0, 0);
  check_complex ("clog (inf + 1 i) == inf + 0.0 i",  FUNC(clog) (BUILD_COMPLEX (plus_infty, 1)), BUILD_COMPLEX (plus_infty, 0.0), 0, 0, 0);
  check_complex ("clog (inf - 0 i) == inf - 0 i",  FUNC(clog) (BUILD_COMPLEX (plus_infty, minus_zero)), BUILD_COMPLEX (plus_infty, minus_zero), 0, 0, 0);
  check_complex ("clog (inf - 1 i) == inf - 0 i",  FUNC(clog) (BUILD_COMPLEX (plus_infty, -1)), BUILD_COMPLEX (plus_infty, minus_zero), 0, 0, 0);

  check_complex ("clog (inf + NaN i) == inf + NaN i",  FUNC(clog) (BUILD_COMPLEX (plus_infty, nan_value)), BUILD_COMPLEX (plus_infty, nan_value), 0, 0, 0);
  check_complex ("clog (-inf + NaN i) == inf + NaN i",  FUNC(clog) (BUILD_COMPLEX (minus_infty, nan_value)), BUILD_COMPLEX (plus_infty, nan_value), 0, 0, 0);

  check_complex ("clog (NaN + inf i) == inf + NaN i",  FUNC(clog) (BUILD_COMPLEX (nan_value, plus_infty)), BUILD_COMPLEX (plus_infty, nan_value), 0, 0, 0);
  check_complex ("clog (NaN - inf i) == inf + NaN i",  FUNC(clog) (BUILD_COMPLEX (nan_value, minus_infty)), BUILD_COMPLEX (plus_infty, nan_value), 0, 0, 0);

  check_complex ("clog (0 + NaN i) == NaN + NaN i",  FUNC(clog) (BUILD_COMPLEX (0, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("clog (3 + NaN i) == NaN + NaN i",  FUNC(clog) (BUILD_COMPLEX (3, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("clog (-0 + NaN i) == NaN + NaN i",  FUNC(clog) (BUILD_COMPLEX (minus_zero, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("clog (-3 + NaN i) == NaN + NaN i",  FUNC(clog) (BUILD_COMPLEX (-3, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);

  check_complex ("clog (NaN + 0 i) == NaN + NaN i",  FUNC(clog) (BUILD_COMPLEX (nan_value, 0)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("clog (NaN + 5 i) == NaN + NaN i",  FUNC(clog) (BUILD_COMPLEX (nan_value, 5)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("clog (NaN - 0 i) == NaN + NaN i",  FUNC(clog) (BUILD_COMPLEX (nan_value, minus_zero)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("clog (NaN - 5 i) == NaN + NaN i",  FUNC(clog) (BUILD_COMPLEX (nan_value, -5)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);

  check_complex ("clog (NaN + NaN i) == NaN + NaN i",  FUNC(clog) (BUILD_COMPLEX (nan_value, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, 0);

  check_complex ("clog (0.75 + 1.25 i) == 0.376885901188190075998919126749298416 + 1.03037682652431246378774332703115153 i",  FUNC(clog) (BUILD_COMPLEX (0.75L, 1.25L)), BUILD_COMPLEX (0.376885901188190075998919126749298416L, 1.03037682652431246378774332703115153L), DELTA803, 0, 0);
  check_complex ("clog (-2 - 3 i) == 1.2824746787307683680267437207826593 - 2.1587989303424641704769327722648368 i",  FUNC(clog) (BUILD_COMPLEX (-2, -3)), BUILD_COMPLEX (1.2824746787307683680267437207826593L, -2.1587989303424641704769327722648368L), DELTA804, 0, 0);

  check_complex ("clog (0x1.fffffep+127 + 0x1.fffffep+127 i) == 89.06941264234832570836679262104313101776 + pi/4 i",  FUNC(clog) (BUILD_COMPLEX (0x1.fffffep+127L, 0x1.fffffep+127L)), BUILD_COMPLEX (89.06941264234832570836679262104313101776L, M_PI_4l), DELTA805, 0, 0);
  check_complex ("clog (0x1.fffffep+127 + 1.0 i) == 88.72283905206835305365817656031404273372 + 2.938736052218037251011746307725933020145e-39 i",  FUNC(clog) (BUILD_COMPLEX (0x1.fffffep+127L, 1.0L)), BUILD_COMPLEX (88.72283905206835305365817656031404273372L, 2.938736052218037251011746307725933020145e-39L), 0, 0, UNDERFLOW_EXCEPTION_FLOAT);
  check_complex ("clog (0x1p-149 + 0x1p-149 i) == -102.9323563131518784484589700365392203592 + pi/4 i",  FUNC(clog) (BUILD_COMPLEX (0x1p-149L, 0x1p-149L)), BUILD_COMPLEX (-102.9323563131518784484589700365392203592L, M_PI_4l), DELTA807, 0, 0);
  check_complex ("clog (0x1p-147 + 0x1p-147 i) == -101.5460619520319878296245057936228672231 + pi/4 i",  FUNC(clog) (BUILD_COMPLEX (0x1p-147L, 0x1p-147L)), BUILD_COMPLEX (-101.5460619520319878296245057936228672231L, M_PI_4l), DELTA808, 0, 0);

#ifndef TEST_FLOAT
  check_complex ("clog (0x1.fffffffffffffp+1023 + 0x1.fffffffffffffp+1023 i) == 710.1292864836639693869320059713862337880 + pi/4 i",  FUNC(clog) (BUILD_COMPLEX (0x1.fffffffffffffp+1023L, 0x1.fffffffffffffp+1023L)), BUILD_COMPLEX (710.1292864836639693869320059713862337880L, M_PI_4l), 0, 0, 0);
  check_complex ("clog (0x1.fffffffffffffp+1023 + 0x1p+1023 i) == 709.8942846690411016323109979483151967689 + 0.4636476090008061606231772164674799632783 i",  FUNC(clog) (BUILD_COMPLEX (0x1.fffffffffffffp+1023L, 0x1p+1023L)), BUILD_COMPLEX (709.8942846690411016323109979483151967689L, 0.4636476090008061606231772164674799632783L), 0, 0, 0);
  check_complex ("clog (0x1p-1074 + 0x1p-1074 i) == -744.0934983311012896593986823853525458290 + pi/4 i",  FUNC(clog) (BUILD_COMPLEX (0x1p-1074L, 0x1p-1074L)), BUILD_COMPLEX (-744.0934983311012896593986823853525458290L, M_PI_4l), DELTA811, 0, 0);
  check_complex ("clog (0x1p-1073 + 0x1p-1073 i) == -743.4003511505413443499814502638943692610 + pi/4 i",  FUNC(clog) (BUILD_COMPLEX (0x1p-1073L, 0x1p-1073L)), BUILD_COMPLEX (-743.4003511505413443499814502638943692610L, M_PI_4l), 0, 0, 0);
#endif

#if defined TEST_LDOUBLE && LDBL_MAX_EXP >= 16384
  check_complex ("clog (0x1.fp+16383 + 0x1.fp+16383 i) == 11356.83823118610934184548269774874545400 + pi/4 i",  FUNC(clog) (BUILD_COMPLEX (0x1.fp+16383L, 0x1.fp+16383L)), BUILD_COMPLEX (11356.83823118610934184548269774874545400L, M_PI_4l), DELTA813, 0, 0);
  check_complex ("clog (0x1.fp+16383 + 0x1p+16383 i) == 11356.60974243783798653123798337822335902 + 0.4764674194737066993385333770295162295856 i",  FUNC(clog) (BUILD_COMPLEX (0x1.fp+16383L, 0x1p+16383L)), BUILD_COMPLEX (11356.60974243783798653123798337822335902L, 0.4764674194737066993385333770295162295856L), DELTA814, 0, 0);
  check_complex ("clog (0x1p-16440 + 0x1p-16441 i) == -11395.22807662984378194141292922726786191 + 0.4636476090008061162142562314612144020285 i",  FUNC(clog) (BUILD_COMPLEX (0x1p-16440L, 0x1p-16441L)), BUILD_COMPLEX (-11395.22807662984378194141292922726786191L, 0.4636476090008061162142562314612144020285L), 0, 0, 0);
#endif

  check_complex ("clog (0x1p-149 + 0x1.fp+127 i) == 88.69109041335841930424871526389807508374 + pi/2 i",  FUNC(clog) (BUILD_COMPLEX (0x1p-149L, 0x1.fp+127L)), BUILD_COMPLEX (88.69109041335841930424871526389807508374L, M_PI_2l), DELTA816, 0, 0);
  check_complex ("clog (-0x1p-149 + 0x1.fp+127 i) == 88.69109041335841930424871526389807508374 + pi/2 i",  FUNC(clog) (BUILD_COMPLEX (-0x1p-149L, 0x1.fp+127L)), BUILD_COMPLEX (88.69109041335841930424871526389807508374L, M_PI_2l), DELTA817, 0, 0);
  check_complex ("clog (0x1p-149 - 0x1.fp+127 i) == 88.69109041335841930424871526389807508374 - pi/2 i",  FUNC(clog) (BUILD_COMPLEX (0x1p-149L, -0x1.fp+127L)), BUILD_COMPLEX (88.69109041335841930424871526389807508374L, -M_PI_2l), DELTA818, 0, 0);
  check_complex ("clog (-0x1p-149 - 0x1.fp+127 i) == 88.69109041335841930424871526389807508374 - pi/2 i",  FUNC(clog) (BUILD_COMPLEX (-0x1p-149L, -0x1.fp+127L)), BUILD_COMPLEX (88.69109041335841930424871526389807508374L, -M_PI_2l), DELTA819, 0, 0);
  check_complex ("clog (-0x1.fp+127 + 0x1p-149 i) == 88.69109041335841930424871526389807508374 + pi i",  FUNC(clog) (BUILD_COMPLEX (-0x1.fp+127L, 0x1p-149L)), BUILD_COMPLEX (88.69109041335841930424871526389807508374L, M_PIl), DELTA820, 0, 0);
  check_complex ("clog (-0x1.fp+127 - 0x1p-149 i) == 88.69109041335841930424871526389807508374 - pi i",  FUNC(clog) (BUILD_COMPLEX (-0x1.fp+127L, -0x1p-149L)), BUILD_COMPLEX (88.69109041335841930424871526389807508374L, -M_PIl), DELTA821, 0, 0);
#ifdef TEST_FLOAT
  check_complex ("clog (0x1.fp+127 + 0x1p-149 i) == 88.69109041335841930424871526389807508374 + +0 i",  FUNC(clog) (BUILD_COMPLEX (0x1.fp+127L, 0x1p-149L)), BUILD_COMPLEX (88.69109041335841930424871526389807508374L, plus_zero), DELTA822, 0, UNDERFLOW_EXCEPTION);
  check_complex ("clog (0x1.fp+127 - 0x1p-149 i) == 88.69109041335841930424871526389807508374 - 0 i",  FUNC(clog) (BUILD_COMPLEX (0x1.fp+127L, -0x1p-149L)), BUILD_COMPLEX (88.69109041335841930424871526389807508374L, minus_zero), DELTA823, 0, UNDERFLOW_EXCEPTION);
#endif

#ifndef TEST_FLOAT
  check_complex ("clog (0x1p-1074 + 0x1.fp+1023 i) == 709.7509641950694165420886960904242800794 + pi/2 i",  FUNC(clog) (BUILD_COMPLEX (0x1p-1074L, 0x1.fp+1023L)), BUILD_COMPLEX (709.7509641950694165420886960904242800794L, M_PI_2l), 0, 0, 0);
  check_complex ("clog (-0x1p-1074 + 0x1.fp+1023 i) == 709.7509641950694165420886960904242800794 + pi/2 i",  FUNC(clog) (BUILD_COMPLEX (-0x1p-1074L, 0x1.fp+1023L)), BUILD_COMPLEX (709.7509641950694165420886960904242800794L, M_PI_2l), 0, 0, 0);
  check_complex ("clog (0x1p-1074 - 0x1.fp+1023 i) == 709.7509641950694165420886960904242800794 - pi/2 i",  FUNC(clog) (BUILD_COMPLEX (0x1p-1074L, -0x1.fp+1023L)), BUILD_COMPLEX (709.7509641950694165420886960904242800794L, -M_PI_2l), 0, 0, 0);
  check_complex ("clog (-0x1p-1074 - 0x1.fp+1023 i) == 709.7509641950694165420886960904242800794 - pi/2 i",  FUNC(clog) (BUILD_COMPLEX (-0x1p-1074L, -0x1.fp+1023L)), BUILD_COMPLEX (709.7509641950694165420886960904242800794L, -M_PI_2l), 0, 0, 0);
  check_complex ("clog (-0x1.fp+1023 + 0x1p-1074 i) == 709.7509641950694165420886960904242800794 + pi i",  FUNC(clog) (BUILD_COMPLEX (-0x1.fp+1023L, 0x1p-1074L)), BUILD_COMPLEX (709.7509641950694165420886960904242800794L, M_PIl), 0, 0, 0);
  check_complex ("clog (-0x1.fp+1023 - 0x1p-1074 i) == 709.7509641950694165420886960904242800794 - pi i",  FUNC(clog) (BUILD_COMPLEX (-0x1.fp+1023L, -0x1p-1074L)), BUILD_COMPLEX (709.7509641950694165420886960904242800794L, -M_PIl), 0, 0, 0);
#endif
#if defined TEST_DOUBLE || (defined TEST_LDOUBLE && LDBL_MAX_EXP == 1024)
  check_complex ("clog (0x1.fp+1023 + 0x1p-1074 i) == 709.7509641950694165420886960904242800794 + +0 i",  FUNC(clog) (BUILD_COMPLEX (0x1.fp+1023L, 0x1p-1074L)), BUILD_COMPLEX (709.7509641950694165420886960904242800794L, plus_zero), 0, 0, UNDERFLOW_EXCEPTION);
  check_complex ("clog (0x1.fp+1023 - 0x1p-1074 i) == 709.7509641950694165420886960904242800794 - 0 i",  FUNC(clog) (BUILD_COMPLEX (0x1.fp+1023L, -0x1p-1074L)), BUILD_COMPLEX (709.7509641950694165420886960904242800794L, minus_zero), 0, 0, UNDERFLOW_EXCEPTION);
#endif

#if defined TEST_LDOUBLE && LDBL_MAX_EXP >= 16384
  check_complex ("clog (0x1p-16445 + 0x1.fp+16383 i) == 11356.49165759582936919077408168801636572 + pi/2 i",  FUNC(clog) (BUILD_COMPLEX (0x1p-16445L, 0x1.fp+16383L)), BUILD_COMPLEX (11356.49165759582936919077408168801636572L, M_PI_2l), DELTA832, 0, 0);
  check_complex ("clog (-0x1p-16445 + 0x1.fp+16383 i) == 11356.49165759582936919077408168801636572 + pi/2 i",  FUNC(clog) (BUILD_COMPLEX (-0x1p-16445L, 0x1.fp+16383L)), BUILD_COMPLEX (11356.49165759582936919077408168801636572L, M_PI_2l), DELTA833, 0, 0);
  check_complex ("clog (0x1p-16445 - 0x1.fp+16383 i) == 11356.49165759582936919077408168801636572 - pi/2 i",  FUNC(clog) (BUILD_COMPLEX (0x1p-16445L, -0x1.fp+16383L)), BUILD_COMPLEX (11356.49165759582936919077408168801636572L, -M_PI_2l), DELTA834, 0, 0);
  check_complex ("clog (-0x1p-16445 - 0x1.fp+16383 i) == 11356.49165759582936919077408168801636572 - pi/2 i",  FUNC(clog) (BUILD_COMPLEX (-0x1p-16445L, -0x1.fp+16383L)), BUILD_COMPLEX (11356.49165759582936919077408168801636572L, -M_PI_2l), DELTA835, 0, 0);
  check_complex ("clog (-0x1.fp+16383 + 0x1p-16445 i) == 11356.49165759582936919077408168801636572 + pi i",  FUNC(clog) (BUILD_COMPLEX (-0x1.fp+16383L, 0x1p-16445L)), BUILD_COMPLEX (11356.49165759582936919077408168801636572L, M_PIl), DELTA836, 0, 0);
  check_complex ("clog (-0x1.fp+16383 - 0x1p-16445 i) == 11356.49165759582936919077408168801636572 - pi i",  FUNC(clog) (BUILD_COMPLEX (-0x1.fp+16383L, -0x1p-16445L)), BUILD_COMPLEX (11356.49165759582936919077408168801636572L, -M_PIl), DELTA837, 0, 0);
  check_complex ("clog (0x1.fp+16383 + 0x1p-16445 i) == 11356.49165759582936919077408168801636572 + +0 i",  FUNC(clog) (BUILD_COMPLEX (0x1.fp+16383L, 0x1p-16445L)), BUILD_COMPLEX (11356.49165759582936919077408168801636572L, plus_zero), DELTA838, 0, UNDERFLOW_EXCEPTION);
  check_complex ("clog (0x1.fp+16383 - 0x1p-16445 i) == 11356.49165759582936919077408168801636572 - 0 i",  FUNC(clog) (BUILD_COMPLEX (0x1.fp+16383L, -0x1p-16445L)), BUILD_COMPLEX (11356.49165759582936919077408168801636572L, minus_zero), DELTA839, 0, UNDERFLOW_EXCEPTION);
# if LDBL_MANT_DIG >= 113
  check_complex ("clog (0x1p-16494 + 0x1.fp+16383 i) == 11356.49165759582936919077408168801636572 + pi/2 i",  FUNC(clog) (BUILD_COMPLEX (0x1p-16494L, 0x1.fp+16383L)), BUILD_COMPLEX (11356.49165759582936919077408168801636572L, M_PI_2l), 0, 0, 0);
  check_complex ("clog (-0x1p-16494 + 0x1.fp+16383 i) == 11356.49165759582936919077408168801636572 + pi/2 i",  FUNC(clog) (BUILD_COMPLEX (-0x1p-16494L, 0x1.fp+16383L)), BUILD_COMPLEX (11356.49165759582936919077408168801636572L, M_PI_2l), 0, 0, 0);
  check_complex ("clog (0x1p-16494 - 0x1.fp+16383 i) == 11356.49165759582936919077408168801636572 - pi/2 i",  FUNC(clog) (BUILD_COMPLEX (0x1p-16494L, -0x1.fp+16383L)), BUILD_COMPLEX (11356.49165759582936919077408168801636572L, -M_PI_2l), 0, 0, 0);
  check_complex ("clog (-0x1p-16494 - 0x1.fp+16383 i) == 11356.49165759582936919077408168801636572 - pi/2 i",  FUNC(clog) (BUILD_COMPLEX (-0x1p-16494L, -0x1.fp+16383L)), BUILD_COMPLEX (11356.49165759582936919077408168801636572L, -M_PI_2l), 0, 0, 0);
  check_complex ("clog (-0x1.fp+16383 + 0x1p-16494 i) == 11356.49165759582936919077408168801636572 + pi i",  FUNC(clog) (BUILD_COMPLEX (-0x1.fp+16383L, 0x1p-16494L)), BUILD_COMPLEX (11356.49165759582936919077408168801636572L, M_PIl), 0, 0, 0);
  check_complex ("clog (-0x1.fp+16383 - 0x1p-16494 i) == 11356.49165759582936919077408168801636572 - pi i",  FUNC(clog) (BUILD_COMPLEX (-0x1.fp+16383L, -0x1p-16494L)), BUILD_COMPLEX (11356.49165759582936919077408168801636572L, -M_PIl), 0, 0, 0);
  check_complex ("clog (0x1.fp+16383 + 0x1p-16494 i) == 11356.49165759582936919077408168801636572 + +0 i",  FUNC(clog) (BUILD_COMPLEX (0x1.fp+16383L, 0x1p-16494L)), BUILD_COMPLEX (11356.49165759582936919077408168801636572L, plus_zero), 0, 0, UNDERFLOW_EXCEPTION);
  check_complex ("clog (0x1.fp+16383 - 0x1p-16494 i) == 11356.49165759582936919077408168801636572 - 0 i",  FUNC(clog) (BUILD_COMPLEX (0x1.fp+16383L, -0x1p-16494L)), BUILD_COMPLEX (11356.49165759582936919077408168801636572L, minus_zero), 0, 0, UNDERFLOW_EXCEPTION);
# endif
#endif

  check_complex ("clog (1.0 + 0x1.234566p-10 i) == 6.172834701221959432440126967147726538097e-7 + 1.111110564353742042376451655136933182201e-3 i",  FUNC(clog) (BUILD_COMPLEX (1.0L, 0x1.234566p-10L)), BUILD_COMPLEX (6.172834701221959432440126967147726538097e-7L, 1.111110564353742042376451655136933182201e-3L), DELTA848, 0, 0);
  check_complex ("clog (-1.0 + 0x1.234566p-20 i) == 5.886877547844618300918562490463748605537e-13 + 3.141591568520436206990380699322226378452 i",  FUNC(clog) (BUILD_COMPLEX (-1.0L, 0x1.234566p-20L)), BUILD_COMPLEX (5.886877547844618300918562490463748605537e-13L, 3.141591568520436206990380699322226378452L), 0, 0, 0);
  check_complex ("clog (0x1.234566p-30 + 1.0 i) == 5.614163921211322622623353961365728040115e-19 + 1.570796325735258575254858696548386439740 i",  FUNC(clog) (BUILD_COMPLEX (0x1.234566p-30L, 1.0L)), BUILD_COMPLEX (5.614163921211322622623353961365728040115e-19L, 1.570796325735258575254858696548386439740L), DELTA850, 0, 0);
  check_complex ("clog (-0x1.234566p-40 - 1.0 i) == 5.354083939753840089583620652120903838944e-25 - 1.570796326795931422008642456283782656359 i",  FUNC(clog) (BUILD_COMPLEX (-0x1.234566p-40L, -1.0L)), BUILD_COMPLEX (5.354083939753840089583620652120903838944e-25L, -1.570796326795931422008642456283782656359L), DELTA851, 0, 0);
  check_complex ("clog (0x1.234566p-50 + 1.0 i) == 5.106052341226425256332038420428899201070e-31 + 1.570796326794895608681734464330528755366 i",  FUNC(clog) (BUILD_COMPLEX (0x1.234566p-50L, 1.0L)), BUILD_COMPLEX (5.106052341226425256332038420428899201070e-31L, 1.570796326794895608681734464330528755366L), 0, 0, 0);
  check_complex ("clog (0x1.234566p-60 + 1.0 i) == 4.869510976053643471080816669875627875933e-37 + 1.570796326794896618244456860363082279319 i",  FUNC(clog) (BUILD_COMPLEX (0x1.234566p-60L, 1.0L)), BUILD_COMPLEX (4.869510976053643471080816669875627875933e-37L, 1.570796326794896618244456860363082279319L), 0, 0, 0);
  check_complex ("clog (0x1p-62 + 1.0 i) == 2.350988701644575015937473074444491355582e-38 + 1.570796326794896619014481257142650555297 i",  FUNC(clog) (BUILD_COMPLEX (0x1p-62L, 1.0L)), BUILD_COMPLEX (2.350988701644575015937473074444491355582e-38L, 1.570796326794896619014481257142650555297L), 0, 0, 0);
  check_complex ("clog (0x1p-63 + 1.0 i) == 5.877471754111437539843682686111228389059e-39 + 1.570796326794896619122901474391200998698 i",  FUNC(clog) (BUILD_COMPLEX (0x1p-63L, 1.0L)), BUILD_COMPLEX (5.877471754111437539843682686111228389059e-39L, 1.570796326794896619122901474391200998698L), 0, 0, UNDERFLOW_EXCEPTION_FLOAT);
  check_complex ("clog (0x1p-64 + 1.0 i) == 1.469367938527859384960920671527807097271e-39 + 1.570796326794896619177111583015476220398 i",  FUNC(clog) (BUILD_COMPLEX (0x1p-64L, 1.0L)), BUILD_COMPLEX (1.469367938527859384960920671527807097271e-39L, 1.570796326794896619177111583015476220398L), 0, 0, UNDERFLOW_EXCEPTION_FLOAT);
#ifndef TEST_FLOAT
  check_complex ("clog (0x1p-510 + 1.0 i) == 4.450147717014402766180465434664808128438e-308 + 1.570796326794896619231321691639751442099 i",  FUNC(clog) (BUILD_COMPLEX (0x1p-510L, 1.0L)), BUILD_COMPLEX (4.450147717014402766180465434664808128438e-308L, 1.570796326794896619231321691639751442099L), 0, 0, UNDERFLOW_EXCEPTION_LDOUBLE_IBM);
  check_complex ("clog (0x1p-511 + 1.0 i) == 1.112536929253600691545116358666202032110e-308 + 1.570796326794896619231321691639751442099 i",  FUNC(clog) (BUILD_COMPLEX (0x1p-511L, 1.0L)), BUILD_COMPLEX (1.112536929253600691545116358666202032110e-308L, 1.570796326794896619231321691639751442099L), 0, 0, UNDERFLOW_EXCEPTION_DOUBLE);
  check_complex ("clog (0x1p-512 + 1.0 i) == 2.781342323134001728862790896665505080274e-309 + 1.570796326794896619231321691639751442099 i",  FUNC(clog) (BUILD_COMPLEX (0x1p-512L, 1.0L)), BUILD_COMPLEX (2.781342323134001728862790896665505080274e-309L, 1.570796326794896619231321691639751442099L), 0, 0, UNDERFLOW_EXCEPTION_DOUBLE);
#endif
#if defined TEST_LDOUBLE && LDBL_MAX_EXP >= 16384
  check_complex ("clog (0x1p-8190 + 1.0 i) == 6.724206286224187012525355634643505205196e-4932 + 1.570796326794896619231321691639751442099 i",  FUNC(clog) (BUILD_COMPLEX (0x1p-8190L, 1.0L)), BUILD_COMPLEX (6.724206286224187012525355634643505205196e-4932L, 1.570796326794896619231321691639751442099L), 0, 0, 0);
  check_complex ("clog (0x1p-8191 + 1.0 i) == 1.681051571556046753131338908660876301299e-4932 + 1.570796326794896619231321691639751442099 i",  FUNC(clog) (BUILD_COMPLEX (0x1p-8191L, 1.0L)), BUILD_COMPLEX (1.681051571556046753131338908660876301299e-4932L, 1.570796326794896619231321691639751442099L), 0, 0, UNDERFLOW_EXCEPTION);
  check_complex ("clog (0x1p-8192 + 1.0 i) == 4.202628928890116882828347271652190753248e-4933 + 1.570796326794896619231321691639751442099 i",  FUNC(clog) (BUILD_COMPLEX (0x1p-8192L, 1.0L)), BUILD_COMPLEX (4.202628928890116882828347271652190753248e-4933L, 1.570796326794896619231321691639751442099L), 0, 0, UNDERFLOW_EXCEPTION);
#endif

  check_complex ("clog (0x1.000566p0 + 0x1.234p-10 i) == 8.298731898331237038231468223024422855654e-5 + 1.110938609507128729312743251313024793990e-3 i",  FUNC(clog) (BUILD_COMPLEX (0x1.000566p0L, 0x1.234p-10L)), BUILD_COMPLEX (8.298731898331237038231468223024422855654e-5L, 1.110938609507128729312743251313024793990e-3L), DELTA863, 0, 0);
  check_complex ("clog (0x1.000566p0 + 0x1.234p-100 i) == 8.237022655933121125560939513260027133767e-5 + 8.974094312218060110948251664314290484113e-31 i",  FUNC(clog) (BUILD_COMPLEX (0x1.000566p0L, 0x1.234p-100L)), BUILD_COMPLEX (8.237022655933121125560939513260027133767e-5L, 8.974094312218060110948251664314290484113e-31L), 0, 0, 0);
#ifndef TEST_FLOAT
  check_complex ("clog (-0x1.0000000123456p0 + 0x1.2345678p-30 i) == 2.649094282537168795982991778475646793277e-10 + 3.141592652530155111500161671113150737892 i",  FUNC(clog) (BUILD_COMPLEX (-0x1.0000000123456p0L, 0x1.2345678p-30L)), BUILD_COMPLEX (2.649094282537168795982991778475646793277e-10L, 3.141592652530155111500161671113150737892L), DELTA865, 0, 0);
  check_complex ("clog (-0x1.0000000123456p0 + 0x1.2345678p-1000 i) == 2.649094276923003995420209214900915462737e-10 + 3.141592653589793238462643383279502884197 i",  FUNC(clog) (BUILD_COMPLEX (-0x1.0000000123456p0L, 0x1.2345678p-1000L)), BUILD_COMPLEX (2.649094276923003995420209214900915462737e-10L, 3.141592653589793238462643383279502884197L), DELTA866, 0, 0);
#endif
#if defined TEST_LDOUBLE && LDBL_MANT_DIG >= 106
  check_complex ("clog (0x1.00000000000000123456789abcp0 + 0x1.23456789p-60 i) == 9.868649107778739757272772275265050767867e-19 + 9.868649106423871142816660980898339912137e-19 i",  FUNC(clog) (BUILD_COMPLEX (0x1.00000000000000123456789abcp0L, 0x1.23456789p-60L)), BUILD_COMPLEX (9.868649107778739757272772275265050767867e-19L, 9.868649106423871142816660980898339912137e-19L), 0, 0, 0);
  check_complex ("clog (0x1.00000000000000123456789abcp0 + 0x1.23456789p-1000 i) == 9.868649107778739752403260515979017248596e-19 + 1.061846605795612822522063052130030717368e-301 i",  FUNC(clog) (BUILD_COMPLEX (0x1.00000000000000123456789abcp0L, 0x1.23456789p-1000L)), BUILD_COMPLEX (9.868649107778739752403260515979017248596e-19L, 1.061846605795612822522063052130030717368e-301L), 0, 0, UNDERFLOW_EXCEPTION_LDOUBLE_IBM);
#endif

  check_complex ("clog (0x0.ffffffp0 + 0x0.ffffffp-100 i) == -5.960464655174753498633255797994360530379e-8 + 7.888609052210118054117285652827862296732e-31 i",  FUNC(clog) (BUILD_COMPLEX (0x0.ffffffp0L, 0x0.ffffffp-100L)), BUILD_COMPLEX (-5.960464655174753498633255797994360530379e-8L, 7.888609052210118054117285652827862296732e-31L), DELTA869, 0, 0);
#ifndef TEST_FLOAT
  check_complex ("clog (0x0.fffffffffffff8p0 + 0x0.fffffffffffff8p-1000 i) == -1.110223024625156602053389888482372171810e-16 + 9.332636185032188789900895447238171696171e-302 i",  FUNC(clog) (BUILD_COMPLEX (0x0.fffffffffffff8p0L, 0x0.fffffffffffff8p-1000L)), BUILD_COMPLEX (-1.110223024625156602053389888482372171810e-16L, 9.332636185032188789900895447238171696171e-302L), 0, 0, UNDERFLOW_EXCEPTION_LDOUBLE_IBM);
#endif
#if defined TEST_LDOUBLE && LDBL_MIN_EXP <= -16381
  check_complex ("clog (0x0.ffffffffffffffffp0 + 0x0.ffffffffffffffffp-15000 i) == -5.421010862427522170184200798202494495630e-20 + 3.548665303440282824232502561095699343814e-4516 i",  FUNC(clog) (BUILD_COMPLEX (0x0.ffffffffffffffffp0L, 0x0.ffffffffffffffffp-15000L)), BUILD_COMPLEX (-5.421010862427522170184200798202494495630e-20L, 3.548665303440282824232502561095699343814e-4516L), DELTA871, 0, 0);
#endif

  check_complex ("clog (0x1a6p-10 + 0x3a5p-10 i) == -1.4305135209763571252847059962654228661815e-06 + 1.1460277178115757370775644871674016684074 i",  FUNC(clog) (BUILD_COMPLEX (0x1a6p-10L, 0x3a5p-10L)), BUILD_COMPLEX (-1.4305135209763571252847059962654228661815e-06L, 1.1460277178115757370775644871674016684074L), 0, 0, 0);
  check_complex ("clog (0xf2p-10 + 0x3e3p-10 i) == 6.1988446308070710970664736815277450078106e-06 + 1.3322126499153926210226335249558203898460 i",  FUNC(clog) (BUILD_COMPLEX (0xf2p-10L, 0x3e3p-10L)), BUILD_COMPLEX (6.1988446308070710970664736815277450078106e-06L, 1.3322126499153926210226335249558203898460L), 0, 0, 0);
  check_complex ("clog (0x4d4ep-15 + 0x6605p-15 i) == -1.6298145321400412054744424587143483169412e-08 + 0.9223574537155056772124552172295398141249 i",  FUNC(clog) (BUILD_COMPLEX (0x4d4ep-15L, 0x6605p-15L)), BUILD_COMPLEX (-1.6298145321400412054744424587143483169412e-08L, 0.9223574537155056772124552172295398141249L), 0, 0, 0);
  check_complex ("clog (0x2818p-15 + 0x798fp-15 i) == 1.5366822245016167178749091974664853785194e-08 + 1.2522014929038946066987318471922169174157 i",  FUNC(clog) (BUILD_COMPLEX (0x2818p-15L, 0x798fp-15L)), BUILD_COMPLEX (1.5366822245016167178749091974664853785194e-08L, 1.2522014929038946066987318471922169174157L), DELTA875, 0, 0);
  check_complex ("clog (0x9b57bp-20 + 0xcb7b4p-20 i) == -3.9563019528687610863490232935890272740908e-11 + 0.9187593477446338910857133065497364950682 i",  FUNC(clog) (BUILD_COMPLEX (0x9b57bp-20L, 0xcb7b4p-20L)), BUILD_COMPLEX (-3.9563019528687610863490232935890272740908e-11L, 0.9187593477446338910857133065497364950682L), 0, 0, 0);
  check_complex ("clog (0x2731p-20 + 0xfffd0p-20 i) == 4.4110493034041283943115971658295280288115e-11 + 1.5612279663766352262688735061954290528838 i",  FUNC(clog) (BUILD_COMPLEX (0x2731p-20L, 0xfffd0p-20L)), BUILD_COMPLEX (4.4110493034041283943115971658295280288115e-11L, 1.5612279663766352262688735061954290528838L), 0, 0, 0);
  check_complex ("clog (0x2ede88p-23 + 0x771c3fp-23 i) == -4.4764192352906350039050902870893173560494e-13 + 1.1959106857549200806818600493552847793381 i",  FUNC(clog) (BUILD_COMPLEX (0x2ede88p-23L, 0x771c3fp-23L)), BUILD_COMPLEX (-4.4764192352906350039050902870893173560494e-13L, 1.1959106857549200806818600493552847793381L), 0, 0, 0);
  check_complex ("clog (0x11682p-23 + 0x7ffed1p-23 i) == 1.1723955140027907954461000991619077811832e-12 + 1.5622968405332756349813737986164832897108 i",  FUNC(clog) (BUILD_COMPLEX (0x11682p-23L, 0x7ffed1p-23L)), BUILD_COMPLEX (1.1723955140027907954461000991619077811832e-12L, 1.5622968405332756349813737986164832897108L), 0, 0, 0);
  check_complex ("clog (0xa1f2c1p-24 + 0xc643aep-24 i) == -1.0480505352462576151523512837107080269981e-13 + 0.8858771987699967480545613322309315260313 i",  FUNC(clog) (BUILD_COMPLEX (0xa1f2c1p-24L, 0xc643aep-24L)), BUILD_COMPLEX (-1.0480505352462576151523512837107080269981e-13L, 0.8858771987699967480545613322309315260313L), 0, 0, 0);
  check_complex ("clog (0x659feap-24 + 0xeaf6f9p-24 i) == 3.7303493627403868207597214252239749960738e-14 + 1.1625816408046866464773042283673653469061 i",  FUNC(clog) (BUILD_COMPLEX (0x659feap-24L, 0xeaf6f9p-24L)), BUILD_COMPLEX (3.7303493627403868207597214252239749960738e-14L, 1.1625816408046866464773042283673653469061L), 0, 0, 0);
#ifndef TEST_FLOAT
  check_complex ("clog (0x4447d7175p-35 + 0x6c445e00ap-35 i) == -1.4823076576950255933915367361099865652625e-20 + 1.0081311552703893116404606212158840190615 i",  FUNC(clog) (BUILD_COMPLEX (0x4447d7175p-35L, 0x6c445e00ap-35L)), BUILD_COMPLEX (-1.4823076576950255933915367361099865652625e-20L, 1.0081311552703893116404606212158840190615L), 0, 0, 0);
  check_complex ("clog (0x2dd46725bp-35 + 0x7783a1284p-35 i) == 4.4469229730850767799109418892826021157328e-20 + 1.2046235979300843056806465045930070146351 i",  FUNC(clog) (BUILD_COMPLEX (0x2dd46725bp-35L, 0x7783a1284p-35L)), BUILD_COMPLEX (4.4469229730850767799109418892826021157328e-20L, 1.2046235979300843056806465045930070146351L), DELTA883, 0, 0);
  check_complex ("clog (0x164c74eea876p-45 + 0x16f393482f77p-45 i) == -3.0292258760486853327810377824479932031744e-26 + 0.7998237934177411746093524982030330293980 i",  FUNC(clog) (BUILD_COMPLEX (0x164c74eea876p-45L, 0x16f393482f77p-45L)), BUILD_COMPLEX (-3.0292258760486853327810377824479932031744e-26L, 0.7998237934177411746093524982030330293980L), 0, 0, 0);
  check_complex ("clog (0xfe961079616p-45 + 0x1bc37e09e6d1p-45 i) == 5.3718272201930019901317065495843842735179e-26 + 1.0503831592447830576186444373011142397404 i",  FUNC(clog) (BUILD_COMPLEX (0xfe961079616p-45L, 0x1bc37e09e6d1p-45L)), BUILD_COMPLEX (5.3718272201930019901317065495843842735179e-26L, 1.0503831592447830576186444373011142397404L), 0, 0, 0);
  check_complex ("clog (0xa4722f19346cp-51 + 0x7f9631c5e7f07p-51 i) == -6.2122796286154679676173624516405339768606e-30 + 1.4904138780720095276446375492434049214172 i",  FUNC(clog) (BUILD_COMPLEX (0xa4722f19346cp-51L, 0x7f9631c5e7f07p-51L)), BUILD_COMPLEX (-6.2122796286154679676173624516405339768606e-30L, 1.4904138780720095276446375492434049214172L), 0, 0, 0);
  check_complex ("clog (0x10673dd0f2481p-51 + 0x7ef1d17cefbd2p-51 i) == 3.2047474274603604594851472963586149973093e-29 + 1.4422922682185099608731642353544207976604 i",  FUNC(clog) (BUILD_COMPLEX (0x10673dd0f2481p-51L, 0x7ef1d17cefbd2p-51L)), BUILD_COMPLEX (3.2047474274603604594851472963586149973093e-29L, 1.4422922682185099608731642353544207976604L), 0, 0, 0);
  check_complex ("clog (0x8ecbf810c4ae6p-52 + 0xd479468b09a37p-52 i) == -9.7375017988218644730510244778042114638107e-30 + 0.9790637929494922564724108399524154766631 i",  FUNC(clog) (BUILD_COMPLEX (0x8ecbf810c4ae6p-52L, 0xd479468b09a37p-52L)), BUILD_COMPLEX (-9.7375017988218644730510244778042114638107e-30L, 0.9790637929494922564724108399524154766631L), 0, 0, 0);
  check_complex ("clog (0x5b06b680ea2ccp-52 + 0xef452b965da9fp-52 i) == 8.3076914081087805757422664530653247447136e-30 + 1.2072712126771536614482822173033535043206 i",  FUNC(clog) (BUILD_COMPLEX (0x5b06b680ea2ccp-52L, 0xef452b965da9fp-52L)), BUILD_COMPLEX (8.3076914081087805757422664530653247447136e-30L, 1.2072712126771536614482822173033535043206L), 0, 0, 0);
  check_complex ("clog (0x659b70ab7971bp-53 + 0x1f5d111e08abecp-53 i) == -2.5083311595699359750201056724289010648701e-30 + 1.3710185432462268491534742969536240564640 i",  FUNC(clog) (BUILD_COMPLEX (0x659b70ab7971bp-53L, 0x1f5d111e08abecp-53L)), BUILD_COMPLEX (-2.5083311595699359750201056724289010648701e-30L, 1.3710185432462268491534742969536240564640L), 0, 0, 0);
  check_complex ("clog (0x15cfbd1990d1ffp-53 + 0x176a3973e09a9ap-53 i) == 1.0168910106364605304135563536838075568606e-30 + 0.8208373755522359859870890246475340086663 i",  FUNC(clog) (BUILD_COMPLEX (0x15cfbd1990d1ffp-53L, 0x176a3973e09a9ap-53L)), BUILD_COMPLEX (1.0168910106364605304135563536838075568606e-30L, 0.8208373755522359859870890246475340086663L), 0, 0, 0);
  check_complex ("clog (0x1367a310575591p-54 + 0x3cfcc0a0541f60p-54 i) == 5.0844550531823026520677817684239496041087e-32 + 1.2627468605458094918919206628466016525397 i",  FUNC(clog) (BUILD_COMPLEX (0x1367a310575591p-54L, 0x3cfcc0a0541f60p-54L)), BUILD_COMPLEX (5.0844550531823026520677817684239496041087e-32L, 1.2627468605458094918919206628466016525397L), DELTA892, 0, 0);
  check_complex ("clog (0x55cb6d0c83af5p-55 + 0x7fe33c0c7c4e90p-55 i) == -5.2000108498455368032511404449795741611813e-32 + 1.5288921536982513453421343495466824420259 i",  FUNC(clog) (BUILD_COMPLEX (0x55cb6d0c83af5p-55L, 0x7fe33c0c7c4e90p-55L)), BUILD_COMPLEX (-5.2000108498455368032511404449795741611813e-32L, 1.5288921536982513453421343495466824420259L), 0, 0, 0);
#endif
#if defined TEST_LDOUBLE && LDBL_MANT_DIG >= 64
  check_complex ("clog (0x298c62cb546588a7p-63 + 0x7911b1dfcc4ecdaep-63 i) == -1.1931267660846218205882675852805793644095e-36 + 1.2402109774337032400594953899784058127412 i",  FUNC(clog) (BUILD_COMPLEX (0x298c62cb546588a7p-63L, 0x7911b1dfcc4ecdaep-63L)), BUILD_COMPLEX (-1.1931267660846218205882675852805793644095e-36L, 1.2402109774337032400594953899784058127412L), 0, 0, 0);
  check_complex ("clog (0x4d9c37e2b5cb4533p-63 + 0x65c98be2385a042ep-63 i) == 6.4064442119814669184296141278612389400075e-37 + 0.9193591364645830864185131402313014890145 i",  FUNC(clog) (BUILD_COMPLEX (0x4d9c37e2b5cb4533p-63L, 0x65c98be2385a042ep-63L)), BUILD_COMPLEX (6.4064442119814669184296141278612389400075e-37L, 0.9193591364645830864185131402313014890145L), 0, 0, 0);
  check_complex ("clog (0x602fd5037c4792efp-64 + 0xed3e2086dcca80b8p-64 i) == -2.3362950222592964220878638677292132852104e-37 + 1.1856121127236268105413184264288408265852 i",  FUNC(clog) (BUILD_COMPLEX (0x602fd5037c4792efp-64L, 0xed3e2086dcca80b8p-64L)), BUILD_COMPLEX (-2.3362950222592964220878638677292132852104e-37L, 1.1856121127236268105413184264288408265852L), 0, 0, 0);
  check_complex ("clog (0x6b10b4f3520217b6p-64 + 0xe8893cbb449253a1p-64 i) == 2.4244570985709679851855191080208817099132e-37 + 1.1393074519572050614551047548718495655972 i",  FUNC(clog) (BUILD_COMPLEX (0x6b10b4f3520217b6p-64L, 0xe8893cbb449253a1p-64L)), BUILD_COMPLEX (2.4244570985709679851855191080208817099132e-37L, 1.1393074519572050614551047548718495655972L), 0, 0, 0);
  check_complex ("clog (0x81b7efa81fc35ad1p-65 + 0x1ef4b835f1c79d812p-65 i) == -9.9182335850630508484862145328126979066934e-39 + 1.3146479888794807046338799047003947008804 i",  FUNC(clog) (BUILD_COMPLEX (0x81b7efa81fc35ad1p-65L, 0x1ef4b835f1c79d812p-65L)), BUILD_COMPLEX (-9.9182335850630508484862145328126979066934e-39L, 1.3146479888794807046338799047003947008804L), 0, 0, 0);
#endif
#if defined TEST_LDOUBLE && LDBL_MANT_DIG >= 106
  check_complex ("clog (0x3f96469050f650869c2p-75 + 0x6f16b2c9c8b05988335p-75 i) == -1.0509738482436128031927971874674370984602e-45 + 1.0509191467640012308402149909370784281448 i",  FUNC(clog) (BUILD_COMPLEX (0x3f96469050f650869c2p-75L, 0x6f16b2c9c8b05988335p-75L)), BUILD_COMPLEX (-1.0509738482436128031927971874674370984602e-45L, 1.0509191467640012308402149909370784281448L), 0, 0, 0);
  check_complex ("clog (0x3157fc1d73233e580c8p-75 + 0x761b52ccd435d7c7f5fp-75 i) == 1.3487497719126364307640897239165442763573e-43 + 1.1750493008528425228929764149024375035382 i",  FUNC(clog) (BUILD_COMPLEX (0x3157fc1d73233e580c8p-75L, 0x761b52ccd435d7c7f5fp-75L)), BUILD_COMPLEX (1.3487497719126364307640897239165442763573e-43L, 1.1750493008528425228929764149024375035382L), 0, 0, 0);
  check_complex ("clog (0x155f8afc4c48685bf63610p-85 + 0x17d0cf2652cdbeb1294e19p-85 i) == -4.7775669192897997174762089350332738583822e-50 + 0.8393953487996880419413728440067635213372 i",  FUNC(clog) (BUILD_COMPLEX (0x155f8afc4c48685bf63610p-85L, 0x17d0cf2652cdbeb1294e19p-85L)), BUILD_COMPLEX (-4.7775669192897997174762089350332738583822e-50L, 0.8393953487996880419413728440067635213372L), 0, 0, 0);
  check_complex ("clog (0x13836d58a13448d750b4b9p-85 + 0x195ca7bc3ab4f9161edbe6p-85 i) == 2.8398125044729578740243199963484494962411e-50 + 0.9149964976334130461795060758257083099706 i",  FUNC(clog) (BUILD_COMPLEX (0x13836d58a13448d750b4b9p-85L, 0x195ca7bc3ab4f9161edbe6p-85L)), BUILD_COMPLEX (2.8398125044729578740243199963484494962411e-50L, 0.9149964976334130461795060758257083099706L), 0, 0, 0);
  check_complex ("clog (0x1df515eb171a808b9e400266p-95 + 0x7c71eb0cd4688dfe98581c77p-95 i) == -3.5048022044913950094635368750889659723004e-57 + 1.3345633256521815205858155673950177421079 i",  FUNC(clog) (BUILD_COMPLEX (0x1df515eb171a808b9e400266p-95L, 0x7c71eb0cd4688dfe98581c77p-95L)), BUILD_COMPLEX (-3.5048022044913950094635368750889659723004e-57L, 1.3345633256521815205858155673950177421079L), 0, 0, 0);
  check_complex ("clog (0xe33f66c9542ca25cc43c867p-95 + 0x7f35a68ebd3704a43c465864p-95 i) == 4.1101771307217268747345114262406964584250e-56 + 1.4596065864518742494094402406719567059585 i",  FUNC(clog) (BUILD_COMPLEX (0xe33f66c9542ca25cc43c867p-95L, 0x7f35a68ebd3704a43c465864p-95L)), BUILD_COMPLEX (4.1101771307217268747345114262406964584250e-56L, 1.4596065864518742494094402406719567059585L), 0, 0, 0);
  check_complex ("clog (0x6771f22c64ed551b857c128b4cp-105 + 0x1f570e7a13cc3cf2f44fd793ea1p-105 i) == -1.4281333889622737316199756373421183559948e-62 + 1.3673546561165378090903506783353927980633 i",  FUNC(clog) (BUILD_COMPLEX (0x6771f22c64ed551b857c128b4cp-105L, 0x1f570e7a13cc3cf2f44fd793ea1p-105L)), BUILD_COMPLEX (-1.4281333889622737316199756373421183559948e-62L, 1.3673546561165378090903506783353927980633L), 0, 0, 0);
  check_complex ("clog (0x15d8ab6ed05ca514086ac3a1e84p-105 + 0x1761e480aa094c0b10b34b09ce9p-105 i) == 1.0027319539522347477331743836657426754857e-62 + 0.8193464073721167323313606647411269414759 i",  FUNC(clog) (BUILD_COMPLEX (0x15d8ab6ed05ca514086ac3a1e84p-105L, 0x1761e480aa094c0b10b34b09ce9p-105L)), BUILD_COMPLEX (1.0027319539522347477331743836657426754857e-62L, 0.8193464073721167323313606647411269414759L), 0, 0, 0);
  check_complex ("clog (0x187190c1a334497bdbde5a95f48p-106 + 0x3b25f08062d0a095c4cfbbc338dp-106 i) == -1.7471844652198029695350765775994001163767e-63 + 1.1789110097072986038243729592318526094314 i",  FUNC(clog) (BUILD_COMPLEX (0x187190c1a334497bdbde5a95f48p-106L, 0x3b25f08062d0a095c4cfbbc338dp-106L)), BUILD_COMPLEX (-1.7471844652198029695350765775994001163767e-63L, 1.1789110097072986038243729592318526094314L), 0, 0, 0);
  check_complex ("clog (0x6241ef0da53f539f02fad67dabp-106 + 0x3fb46641182f7efd9caa769dac0p-106 i) == 4.3299788920664682288477984749202524623248e-63 + 1.4746938237585656250866370987773473745867 i",  FUNC(clog) (BUILD_COMPLEX (0x6241ef0da53f539f02fad67dabp-106L, 0x3fb46641182f7efd9caa769dac0p-106L)), BUILD_COMPLEX (4.3299788920664682288477984749202524623248e-63L, 1.4746938237585656250866370987773473745867L), 0, 0, 0);
#endif
#if defined TEST_LDOUBLE && LDBL_MANT_DIG >= 113
  check_complex ("clog (0x3e1d0a105ac4ebeacd9c6952d34cp-112 + 0xf859b3d1b06d005dcbb5516d5479p-112 i) == -1.1683999374665377365054966073875064467108e-66 + 1.3257197596350832748781065387304444940172 i",  FUNC(clog) (BUILD_COMPLEX (0x3e1d0a105ac4ebeacd9c6952d34cp-112L, 0xf859b3d1b06d005dcbb5516d5479p-112L)), BUILD_COMPLEX (-1.1683999374665377365054966073875064467108e-66L, 1.3257197596350832748781065387304444940172L), 0, 0, 0);
  check_complex ("clog (0x47017a2e36807acb1e5214b209dep-112 + 0xf5f4a550c9d75e3bb1839d865f0dp-112 i) == 1.5077923002544367932999503838191154621839e-65 + 1.2897445708311412721399861948957141824914 i",  FUNC(clog) (BUILD_COMPLEX (0x47017a2e36807acb1e5214b209dep-112L, 0xf5f4a550c9d75e3bb1839d865f0dp-112L)), BUILD_COMPLEX (1.5077923002544367932999503838191154621839e-65L, 1.2897445708311412721399861948957141824914L), 0, 0, 0);
  check_complex ("clog (0x148f818cb7a9258fca942ade2a0cap-113 + 0x18854a34780b8333ec53310ad7001p-113 i) == -7.1865869169568789348552370692485515571497e-67 + 0.8730167479365994646287897223471819363668 i",  FUNC(clog) (BUILD_COMPLEX (0x148f818cb7a9258fca942ade2a0cap-113L, 0x18854a34780b8333ec53310ad7001p-113L)), BUILD_COMPLEX (-7.1865869169568789348552370692485515571497e-67L, 0.8730167479365994646287897223471819363668L), 0, 0, 0);
  check_complex ("clog (0xfd95243681c055c2632286921092p-113 + 0x1bccabcd29ca2152860ec29e34ef7p-113 i) == 6.6255694866654064502633121109394710807528e-66 + 1.0526409614996288387567810726095850312049 i",  FUNC(clog) (BUILD_COMPLEX (0xfd95243681c055c2632286921092p-113L, 0x1bccabcd29ca2152860ec29e34ef7p-113L)), BUILD_COMPLEX (6.6255694866654064502633121109394710807528e-66L, 1.0526409614996288387567810726095850312049L), 0, 0, 0);
  check_complex ("clog (0xdb85c467ee2aadd5f425fe0f4b8dp-114 + 0x3e83162a0f95f1dcbf97dddf410eap-114 i) == 4.6017338806965821566734340588575402712716e-67 + 1.3547418904611758959096647942223384691728 i",  FUNC(clog) (BUILD_COMPLEX (0xdb85c467ee2aadd5f425fe0f4b8dp-114L, 0x3e83162a0f95f1dcbf97dddf410eap-114L)), BUILD_COMPLEX (4.6017338806965821566734340588575402712716e-67L, 1.3547418904611758959096647942223384691728L), 0, 0, 0);
  check_complex ("clog (0x1415bcaf2105940d49a636e98ae59p-115 + 0x7e6a150adfcd1b0921d44b31f40f4p-115 i) == 2.5993421227864195179698176012564317527271e-67 + 1.4132318089683022770487383611430906982461 i",  FUNC(clog) (BUILD_COMPLEX (0x1415bcaf2105940d49a636e98ae59p-115L, 0x7e6a150adfcd1b0921d44b31f40f4p-115L)), BUILD_COMPLEX (2.5993421227864195179698176012564317527271e-67L, 1.4132318089683022770487383611430906982461L), 0, 0, 0);
#endif

  print_complex_max_error ("clog", DELTAclog, 0);
}


static void
clog10_test (void)
{
  errno = 0;
  FUNC(clog10) (BUILD_COMPLEX (0.7L, 1.2L));
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_complex ("clog10 (-0 + 0 i) == -inf + pi i",  FUNC(clog10) (BUILD_COMPLEX (minus_zero, 0)), BUILD_COMPLEX (minus_infty, M_PIl), 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_complex ("clog10 (-0 - 0 i) == -inf - pi i",  FUNC(clog10) (BUILD_COMPLEX (minus_zero, minus_zero)), BUILD_COMPLEX (minus_infty, -M_PIl), 0, 0, DIVIDE_BY_ZERO_EXCEPTION);

  check_complex ("clog10 (0 + 0 i) == -inf + 0.0 i",  FUNC(clog10) (BUILD_COMPLEX (0, 0)), BUILD_COMPLEX (minus_infty, 0.0), 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_complex ("clog10 (0 - 0 i) == -inf - 0 i",  FUNC(clog10) (BUILD_COMPLEX (0, minus_zero)), BUILD_COMPLEX (minus_infty, minus_zero), 0, 0, DIVIDE_BY_ZERO_EXCEPTION);

  check_complex ("clog10 (-inf + inf i) == inf + 3/4 pi*log10(e) i",  FUNC(clog10) (BUILD_COMPLEX (minus_infty, plus_infty)), BUILD_COMPLEX (plus_infty, M_PI_34_LOG10El), DELTA919, 0, 0);

  check_complex ("clog10 (inf + inf i) == inf + pi/4*log10(e) i",  FUNC(clog10) (BUILD_COMPLEX (plus_infty, plus_infty)), BUILD_COMPLEX (plus_infty, M_PI4_LOG10El), DELTA920, 0, 0);
  check_complex ("clog10 (inf - inf i) == inf - pi/4*log10(e) i",  FUNC(clog10) (BUILD_COMPLEX (plus_infty, minus_infty)), BUILD_COMPLEX (plus_infty, -M_PI4_LOG10El), DELTA921, 0, 0);

  check_complex ("clog10 (0 + inf i) == inf + pi/2*log10(e) i",  FUNC(clog10) (BUILD_COMPLEX (0, plus_infty)), BUILD_COMPLEX (plus_infty, M_PI2_LOG10El), DELTA922, 0, 0);
  check_complex ("clog10 (3 + inf i) == inf + pi/2*log10(e) i",  FUNC(clog10) (BUILD_COMPLEX (3, plus_infty)), BUILD_COMPLEX (plus_infty, M_PI2_LOG10El), DELTA923, 0, 0);
  check_complex ("clog10 (-0 + inf i) == inf + pi/2*log10(e) i",  FUNC(clog10) (BUILD_COMPLEX (minus_zero, plus_infty)), BUILD_COMPLEX (plus_infty, M_PI2_LOG10El), DELTA924, 0, 0);
  check_complex ("clog10 (-3 + inf i) == inf + pi/2*log10(e) i",  FUNC(clog10) (BUILD_COMPLEX (-3, plus_infty)), BUILD_COMPLEX (plus_infty, M_PI2_LOG10El), DELTA925, 0, 0);
  check_complex ("clog10 (0 - inf i) == inf - pi/2*log10(e) i",  FUNC(clog10) (BUILD_COMPLEX (0, minus_infty)), BUILD_COMPLEX (plus_infty, -M_PI2_LOG10El), DELTA926, 0, 0);
  check_complex ("clog10 (3 - inf i) == inf - pi/2*log10(e) i",  FUNC(clog10) (BUILD_COMPLEX (3, minus_infty)), BUILD_COMPLEX (plus_infty, -M_PI2_LOG10El), DELTA927, 0, 0);
  check_complex ("clog10 (-0 - inf i) == inf - pi/2*log10(e) i",  FUNC(clog10) (BUILD_COMPLEX (minus_zero, minus_infty)), BUILD_COMPLEX (plus_infty, -M_PI2_LOG10El), DELTA928, 0, 0);
  check_complex ("clog10 (-3 - inf i) == inf - pi/2*log10(e) i",  FUNC(clog10) (BUILD_COMPLEX (-3, minus_infty)), BUILD_COMPLEX (plus_infty, -M_PI2_LOG10El), DELTA929, 0, 0);

  check_complex ("clog10 (-inf + 0 i) == inf + pi*log10(e) i",  FUNC(clog10) (BUILD_COMPLEX (minus_infty, 0)), BUILD_COMPLEX (plus_infty, M_PI_LOG10El), DELTA930, 0, 0);
  check_complex ("clog10 (-inf + 1 i) == inf + pi*log10(e) i",  FUNC(clog10) (BUILD_COMPLEX (minus_infty, 1)), BUILD_COMPLEX (plus_infty, M_PI_LOG10El), DELTA931, 0, 0);
  check_complex ("clog10 (-inf - 0 i) == inf - pi*log10(e) i",  FUNC(clog10) (BUILD_COMPLEX (minus_infty, minus_zero)), BUILD_COMPLEX (plus_infty, -M_PI_LOG10El), DELTA932, 0, 0);
  check_complex ("clog10 (-inf - 1 i) == inf - pi*log10(e) i",  FUNC(clog10) (BUILD_COMPLEX (minus_infty, -1)), BUILD_COMPLEX (plus_infty, -M_PI_LOG10El), DELTA933, 0, 0);

  check_complex ("clog10 (inf + 0 i) == inf + 0.0 i",  FUNC(clog10) (BUILD_COMPLEX (plus_infty, 0)), BUILD_COMPLEX (plus_infty, 0.0), 0, 0, 0);
  check_complex ("clog10 (inf + 1 i) == inf + 0.0 i",  FUNC(clog10) (BUILD_COMPLEX (plus_infty, 1)), BUILD_COMPLEX (plus_infty, 0.0), 0, 0, 0);
  check_complex ("clog10 (inf - 0 i) == inf - 0 i",  FUNC(clog10) (BUILD_COMPLEX (plus_infty, minus_zero)), BUILD_COMPLEX (plus_infty, minus_zero), 0, 0, 0);
  check_complex ("clog10 (inf - 1 i) == inf - 0 i",  FUNC(clog10) (BUILD_COMPLEX (plus_infty, -1)), BUILD_COMPLEX (plus_infty, minus_zero), 0, 0, 0);

  check_complex ("clog10 (inf + NaN i) == inf + NaN i",  FUNC(clog10) (BUILD_COMPLEX (plus_infty, nan_value)), BUILD_COMPLEX (plus_infty, nan_value), 0, 0, 0);
  check_complex ("clog10 (-inf + NaN i) == inf + NaN i",  FUNC(clog10) (BUILD_COMPLEX (minus_infty, nan_value)), BUILD_COMPLEX (plus_infty, nan_value), 0, 0, 0);

  check_complex ("clog10 (NaN + inf i) == inf + NaN i",  FUNC(clog10) (BUILD_COMPLEX (nan_value, plus_infty)), BUILD_COMPLEX (plus_infty, nan_value), 0, 0, 0);
  check_complex ("clog10 (NaN - inf i) == inf + NaN i",  FUNC(clog10) (BUILD_COMPLEX (nan_value, minus_infty)), BUILD_COMPLEX (plus_infty, nan_value), 0, 0, 0);

  check_complex ("clog10 (0 + NaN i) == NaN + NaN i",  FUNC(clog10) (BUILD_COMPLEX (0, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("clog10 (3 + NaN i) == NaN + NaN i",  FUNC(clog10) (BUILD_COMPLEX (3, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("clog10 (-0 + NaN i) == NaN + NaN i",  FUNC(clog10) (BUILD_COMPLEX (minus_zero, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("clog10 (-3 + NaN i) == NaN + NaN i",  FUNC(clog10) (BUILD_COMPLEX (-3, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);

  check_complex ("clog10 (NaN + 0 i) == NaN + NaN i",  FUNC(clog10) (BUILD_COMPLEX (nan_value, 0)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("clog10 (NaN + 5 i) == NaN + NaN i",  FUNC(clog10) (BUILD_COMPLEX (nan_value, 5)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("clog10 (NaN - 0 i) == NaN + NaN i",  FUNC(clog10) (BUILD_COMPLEX (nan_value, minus_zero)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("clog10 (NaN - 5 i) == NaN + NaN i",  FUNC(clog10) (BUILD_COMPLEX (nan_value, -5)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);

  check_complex ("clog10 (NaN + NaN i) == NaN + NaN i",  FUNC(clog10) (BUILD_COMPLEX (nan_value, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, 0);

  check_complex ("clog10 (0.75 + 1.25 i) == 0.163679467193165171449476605077428975 + 0.447486970040493067069984724340855636 i",  FUNC(clog10) (BUILD_COMPLEX (0.75L, 1.25L)), BUILD_COMPLEX (0.163679467193165171449476605077428975L, 0.447486970040493067069984724340855636L), DELTA951, 0, 0);
  check_complex ("clog10 (-2 - 3 i) == 0.556971676153418384603252578971164214 - 0.937554462986374708541507952140189646 i",  FUNC(clog10) (BUILD_COMPLEX (-2, -3)), BUILD_COMPLEX (0.556971676153418384603252578971164214L, -0.937554462986374708541507952140189646L), DELTA952, 0, 0);

  check_complex ("clog10 (0x1.fffffep+127 + 0x1.fffffep+127 i) == 38.68235441693561449174780668781319348761 + pi/4*log10(e) i",  FUNC(clog10) (BUILD_COMPLEX (0x1.fffffep+127L, 0x1.fffffep+127L)), BUILD_COMPLEX (38.68235441693561449174780668781319348761L, M_PI4_LOG10El), DELTA953, 0, 0);
  check_complex ("clog10 (0x1.fffffep+127 + 1.0 i) == 38.53183941910362389414093724045094697423 + 1.276276851248440096917018665609900318458e-39 i",  FUNC(clog10) (BUILD_COMPLEX (0x1.fffffep+127L, 1.0L)), BUILD_COMPLEX (38.53183941910362389414093724045094697423L, 1.276276851248440096917018665609900318458e-39L), DELTA954, 0, UNDERFLOW_EXCEPTION_FLOAT);
  check_complex ("clog10 (0x1p-149 + 0x1p-149 i) == -44.70295435610120748924022586658721447508 + pi/4*log10(e) i",  FUNC(clog10) (BUILD_COMPLEX (0x1p-149L, 0x1p-149L)), BUILD_COMPLEX (-44.70295435610120748924022586658721447508L, M_PI4_LOG10El), DELTA955, 0, 0);
  check_complex ("clog10 (0x1p-147 + 0x1p-147 i) == -44.10089436477324509881274807713822842154 + pi/4*log10(e) i",  FUNC(clog10) (BUILD_COMPLEX (0x1p-147L, 0x1p-147L)), BUILD_COMPLEX (-44.10089436477324509881274807713822842154L, M_PI4_LOG10El), DELTA956, 0, 0);

#ifndef TEST_FLOAT
  check_complex ("clog10 (0x1.fffffffffffffp+1023 + 0x1.fffffffffffffp+1023 i) == 308.4052305577487344482591243175787477115 + pi/4*log10(e) i",  FUNC(clog10) (BUILD_COMPLEX (0x1.fffffffffffffp+1023L, 0x1.fffffffffffffp+1023L)), BUILD_COMPLEX (308.4052305577487344482591243175787477115L, M_PI4_LOG10El), DELTA957, 0, 0);
  check_complex ("clog10 (0x1.fffffffffffffp+1023 + 0x1p+1023 i) == 308.3031705664207720674749211936626341569 + 0.2013595981366865903254995612594728746470 i",  FUNC(clog10) (BUILD_COMPLEX (0x1.fffffffffffffp+1023L, 0x1p+1023L)), BUILD_COMPLEX (308.3031705664207720674749211936626341569L, 0.2013595981366865903254995612594728746470L), DELTA958, 0, 0);
  check_complex ("clog10 (0x1p-1074 + 0x1p-1074 i) == -323.1557003452838130619487034867432642357 + pi/4*log10(e) i",  FUNC(clog10) (BUILD_COMPLEX (0x1p-1074L, 0x1p-1074L)), BUILD_COMPLEX (-323.1557003452838130619487034867432642357L, M_PI4_LOG10El), DELTA959, 0, 0);
  check_complex ("clog10 (0x1p-1073 + 0x1p-1073 i) == -322.8546703496198318667349645920187712089 + pi/4*log10(e) i",  FUNC(clog10) (BUILD_COMPLEX (0x1p-1073L, 0x1p-1073L)), BUILD_COMPLEX (-322.8546703496198318667349645920187712089L, M_PI4_LOG10El), DELTA960, 0, 0);
#endif

#if defined TEST_LDOUBLE && LDBL_MAX_EXP >= 16384
  check_complex ("clog10 (0x1.fp+16383 + 0x1.fp+16383 i) == 4932.212175672014259683102930239951947672 + pi/4*log10(e) i",  FUNC(clog10) (BUILD_COMPLEX (0x1.fp+16383L, 0x1.fp+16383L)), BUILD_COMPLEX (4932.212175672014259683102930239951947672L, M_PI4_LOG10El), DELTA961, 0, 0);
  check_complex ("clog10 (0x1.fp+16383 + 0x1p+16383 i) == 4932.112944269463028900262609694408579449 + 0.2069271710841128115912940666587802677383 i",  FUNC(clog10) (BUILD_COMPLEX (0x1.fp+16383L, 0x1p+16383L)), BUILD_COMPLEX (4932.112944269463028900262609694408579449L, 0.2069271710841128115912940666587802677383L), DELTA962, 0, 0);
  check_complex ("clog10 (0x1p-16440 + 0x1p-16441 i) == -4948.884673709346821106688037612752099609 + 0.2013595981366865710389502301937289472543 i",  FUNC(clog10) (BUILD_COMPLEX (0x1p-16440L, 0x1p-16441L)), BUILD_COMPLEX (-4948.884673709346821106688037612752099609L, 0.2013595981366865710389502301937289472543L), DELTA963, 0, 0);
#endif

  check_complex ("clog10 (0x1p-149 + 0x1.fp+127 i) == 38.51805116050395969095658815123105801479 + 0.6821881769209206737428918127156778851051 i",  FUNC(clog10) (BUILD_COMPLEX (0x1p-149L, 0x1.fp+127L)), BUILD_COMPLEX (38.51805116050395969095658815123105801479L, 0.6821881769209206737428918127156778851051L), DELTA964, 0, 0);
  check_complex ("clog10 (-0x1p-149 + 0x1.fp+127 i) == 38.51805116050395969095658815123105801479 + 0.6821881769209206737428918127156778851051 i",  FUNC(clog10) (BUILD_COMPLEX (-0x1p-149L, 0x1.fp+127L)), BUILD_COMPLEX (38.51805116050395969095658815123105801479L, 0.6821881769209206737428918127156778851051L), DELTA965, 0, 0);
  check_complex ("clog10 (0x1p-149 - 0x1.fp+127 i) == 38.51805116050395969095658815123105801479 - 0.6821881769209206737428918127156778851051 i",  FUNC(clog10) (BUILD_COMPLEX (0x1p-149L, -0x1.fp+127L)), BUILD_COMPLEX (38.51805116050395969095658815123105801479L, -0.6821881769209206737428918127156778851051L), DELTA966, 0, 0);
  check_complex ("clog10 (-0x1p-149 - 0x1.fp+127 i) == 38.51805116050395969095658815123105801479 - 0.6821881769209206737428918127156778851051 i",  FUNC(clog10) (BUILD_COMPLEX (-0x1p-149L, -0x1.fp+127L)), BUILD_COMPLEX (38.51805116050395969095658815123105801479L, -0.6821881769209206737428918127156778851051L), DELTA967, 0, 0);
  check_complex ("clog10 (-0x1.fp+127 + 0x1p-149 i) == 38.51805116050395969095658815123105801479 + 1.364376353841841347485783625431355770210 i",  FUNC(clog10) (BUILD_COMPLEX (-0x1.fp+127L, 0x1p-149L)), BUILD_COMPLEX (38.51805116050395969095658815123105801479L, 1.364376353841841347485783625431355770210L), DELTA968, 0, 0);
  check_complex ("clog10 (-0x1.fp+127 - 0x1p-149 i) == 38.51805116050395969095658815123105801479 - 1.364376353841841347485783625431355770210 i",  FUNC(clog10) (BUILD_COMPLEX (-0x1.fp+127L, -0x1p-149L)), BUILD_COMPLEX (38.51805116050395969095658815123105801479L, -1.364376353841841347485783625431355770210L), DELTA969, 0, 0);
#ifdef TEST_FLOAT
  check_complex ("clog10 (0x1.fp+127 + 0x1p-149 i) == 38.51805116050395969095658815123105801479 + +0 i",  FUNC(clog10) (BUILD_COMPLEX (0x1.fp+127L, 0x1p-149L)), BUILD_COMPLEX (38.51805116050395969095658815123105801479L, plus_zero), 0, 0, UNDERFLOW_EXCEPTION);
  check_complex ("clog10 (0x1.fp+127 - 0x1p-149 i) == 38.51805116050395969095658815123105801479 - 0 i",  FUNC(clog10) (BUILD_COMPLEX (0x1.fp+127L, -0x1p-149L)), BUILD_COMPLEX (38.51805116050395969095658815123105801479L, minus_zero), 0, 0, UNDERFLOW_EXCEPTION);
#endif

#ifndef TEST_FLOAT
  check_complex ("clog10 (0x1p-1074 + 0x1.fp+1023 i) == 308.2409272754311106024666378243768099991 + 0.6821881769209206737428918127156778851051 i",  FUNC(clog10) (BUILD_COMPLEX (0x1p-1074L, 0x1.fp+1023L)), BUILD_COMPLEX (308.2409272754311106024666378243768099991L, 0.6821881769209206737428918127156778851051L), DELTA972, 0, 0);
  check_complex ("clog10 (-0x1p-1074 + 0x1.fp+1023 i) == 308.2409272754311106024666378243768099991 + 0.6821881769209206737428918127156778851051 i",  FUNC(clog10) (BUILD_COMPLEX (-0x1p-1074L, 0x1.fp+1023L)), BUILD_COMPLEX (308.2409272754311106024666378243768099991L, 0.6821881769209206737428918127156778851051L), DELTA973, 0, 0);
  check_complex ("clog10 (0x1p-1074 - 0x1.fp+1023 i) == 308.2409272754311106024666378243768099991 - 0.6821881769209206737428918127156778851051 i",  FUNC(clog10) (BUILD_COMPLEX (0x1p-1074L, -0x1.fp+1023L)), BUILD_COMPLEX (308.2409272754311106024666378243768099991L, -0.6821881769209206737428918127156778851051L), DELTA974, 0, 0);
  check_complex ("clog10 (-0x1p-1074 - 0x1.fp+1023 i) == 308.2409272754311106024666378243768099991 - 0.6821881769209206737428918127156778851051 i",  FUNC(clog10) (BUILD_COMPLEX (-0x1p-1074L, -0x1.fp+1023L)), BUILD_COMPLEX (308.2409272754311106024666378243768099991L, -0.6821881769209206737428918127156778851051L), DELTA975, 0, 0);
  check_complex ("clog10 (-0x1.fp+1023 + 0x1p-1074 i) == 308.2409272754311106024666378243768099991 + 1.364376353841841347485783625431355770210 i",  FUNC(clog10) (BUILD_COMPLEX (-0x1.fp+1023L, 0x1p-1074L)), BUILD_COMPLEX (308.2409272754311106024666378243768099991L, 1.364376353841841347485783625431355770210L), DELTA976, 0, 0);
  check_complex ("clog10 (-0x1.fp+1023 - 0x1p-1074 i) == 308.2409272754311106024666378243768099991 - 1.364376353841841347485783625431355770210 i",  FUNC(clog10) (BUILD_COMPLEX (-0x1.fp+1023L, -0x1p-1074L)), BUILD_COMPLEX (308.2409272754311106024666378243768099991L, -1.364376353841841347485783625431355770210L), DELTA977, 0, 0);
#endif
#if defined TEST_DOUBLE || (defined TEST_LDOUBLE && LDBL_MAX_EXP == 1024)
  check_complex ("clog10 (0x1.fp+1023 + 0x1p-1074 i) == 308.2409272754311106024666378243768099991 + +0 i",  FUNC(clog10) (BUILD_COMPLEX (0x1.fp+1023L, 0x1p-1074L)), BUILD_COMPLEX (308.2409272754311106024666378243768099991L, plus_zero), 0, 0, UNDERFLOW_EXCEPTION);
  check_complex ("clog10 (0x1.fp+1023 - 0x1p-1074 i) == 308.2409272754311106024666378243768099991 - 0 i",  FUNC(clog10) (BUILD_COMPLEX (0x1.fp+1023L, -0x1p-1074L)), BUILD_COMPLEX (308.2409272754311106024666378243768099991L, minus_zero), 0, 0, UNDERFLOW_EXCEPTION);
#endif

#if defined TEST_LDOUBLE && LDBL_MAX_EXP >= 16384
  check_complex ("clog10 (0x1p-16445 + 0x1.fp+16383 i) == 4932.061660674182269085496060792589701158 + 0.6821881769209206737428918127156778851051 i",  FUNC(clog10) (BUILD_COMPLEX (0x1p-16445L, 0x1.fp+16383L)), BUILD_COMPLEX (4932.061660674182269085496060792589701158L, 0.6821881769209206737428918127156778851051L), DELTA980, 0, 0);
  check_complex ("clog10 (-0x1p-16445 + 0x1.fp+16383 i) == 4932.061660674182269085496060792589701158 + 0.6821881769209206737428918127156778851051 i",  FUNC(clog10) (BUILD_COMPLEX (-0x1p-16445L, 0x1.fp+16383L)), BUILD_COMPLEX (4932.061660674182269085496060792589701158L, 0.6821881769209206737428918127156778851051L), DELTA981, 0, 0);
  check_complex ("clog10 (0x1p-16445 - 0x1.fp+16383 i) == 4932.061660674182269085496060792589701158 - 0.6821881769209206737428918127156778851051 i",  FUNC(clog10) (BUILD_COMPLEX (0x1p-16445L, -0x1.fp+16383L)), BUILD_COMPLEX (4932.061660674182269085496060792589701158L, -0.6821881769209206737428918127156778851051L), DELTA982, 0, 0);
  check_complex ("clog10 (-0x1p-16445 - 0x1.fp+16383 i) == 4932.061660674182269085496060792589701158 - 0.6821881769209206737428918127156778851051 i",  FUNC(clog10) (BUILD_COMPLEX (-0x1p-16445L, -0x1.fp+16383L)), BUILD_COMPLEX (4932.061660674182269085496060792589701158L, -0.6821881769209206737428918127156778851051L), DELTA983, 0, 0);
  check_complex ("clog10 (-0x1.fp+16383 + 0x1p-16445 i) == 4932.061660674182269085496060792589701158 + 1.364376353841841347485783625431355770210 i",  FUNC(clog10) (BUILD_COMPLEX (-0x1.fp+16383L, 0x1p-16445L)), BUILD_COMPLEX (4932.061660674182269085496060792589701158L, 1.364376353841841347485783625431355770210L), DELTA984, 0, 0);
  check_complex ("clog10 (-0x1.fp+16383 - 0x1p-16445 i) == 4932.061660674182269085496060792589701158 - 1.364376353841841347485783625431355770210 i",  FUNC(clog10) (BUILD_COMPLEX (-0x1.fp+16383L, -0x1p-16445L)), BUILD_COMPLEX (4932.061660674182269085496060792589701158L, -1.364376353841841347485783625431355770210L), DELTA985, 0, 0);
  check_complex ("clog10 (0x1.fp+16383 + 0x1p-16445 i) == 4932.061660674182269085496060792589701158 + +0 i",  FUNC(clog10) (BUILD_COMPLEX (0x1.fp+16383L, 0x1p-16445L)), BUILD_COMPLEX (4932.061660674182269085496060792589701158L, plus_zero), DELTA986, 0, UNDERFLOW_EXCEPTION);
  check_complex ("clog10 (0x1.fp+16383 - 0x1p-16445 i) == 4932.061660674182269085496060792589701158 - 0 i",  FUNC(clog10) (BUILD_COMPLEX (0x1.fp+16383L, -0x1p-16445L)), BUILD_COMPLEX (4932.061660674182269085496060792589701158L, minus_zero), DELTA987, 0, UNDERFLOW_EXCEPTION);
# if LDBL_MANT_DIG >= 113
  check_complex ("clog10 (0x1p-16494 + 0x1.fp+16383 i) == 4932.061660674182269085496060792589701158 + 0.6821881769209206737428918127156778851051 i",  FUNC(clog10) (BUILD_COMPLEX (0x1p-16494L, 0x1.fp+16383L)), BUILD_COMPLEX (4932.061660674182269085496060792589701158L, 0.6821881769209206737428918127156778851051L), 0, 0, 0);
  check_complex ("clog10 (-0x1p-16494 + 0x1.fp+16383 i) == 4932.061660674182269085496060792589701158 + 0.6821881769209206737428918127156778851051 i",  FUNC(clog10) (BUILD_COMPLEX (-0x1p-16494L, 0x1.fp+16383L)), BUILD_COMPLEX (4932.061660674182269085496060792589701158L, 0.6821881769209206737428918127156778851051L), 0, 0, 0);
  check_complex ("clog10 (0x1p-16494 - 0x1.fp+16383 i) == 4932.061660674182269085496060792589701158 - 0.6821881769209206737428918127156778851051 i",  FUNC(clog10) (BUILD_COMPLEX (0x1p-16494L, -0x1.fp+16383L)), BUILD_COMPLEX (4932.061660674182269085496060792589701158L, -0.6821881769209206737428918127156778851051L), 0, 0, 0);
  check_complex ("clog10 (-0x1p-16494 - 0x1.fp+16383 i) == 4932.061660674182269085496060792589701158 - 0.6821881769209206737428918127156778851051 i",  FUNC(clog10) (BUILD_COMPLEX (-0x1p-16494L, -0x1.fp+16383L)), BUILD_COMPLEX (4932.061660674182269085496060792589701158L, -0.6821881769209206737428918127156778851051L), 0, 0, 0);
  check_complex ("clog10 (-0x1.fp+16383 + 0x1p-16494 i) == 4932.061660674182269085496060792589701158 + 1.364376353841841347485783625431355770210 i",  FUNC(clog10) (BUILD_COMPLEX (-0x1.fp+16383L, 0x1p-16494L)), BUILD_COMPLEX (4932.061660674182269085496060792589701158L, 1.364376353841841347485783625431355770210L), 0, 0, 0);
  check_complex ("clog10 (-0x1.fp+16383 - 0x1p-16494 i) == 4932.061660674182269085496060792589701158 - 1.364376353841841347485783625431355770210 i",  FUNC(clog10) (BUILD_COMPLEX (-0x1.fp+16383L, -0x1p-16494L)), BUILD_COMPLEX (4932.061660674182269085496060792589701158L, -1.364376353841841347485783625431355770210L), 0, 0, 0);
  check_complex ("clog10 (0x1.fp+16383 + 0x1p-16494 i) == 4932.061660674182269085496060792589701158 + +0 i",  FUNC(clog10) (BUILD_COMPLEX (0x1.fp+16383L, 0x1p-16494L)), BUILD_COMPLEX (4932.061660674182269085496060792589701158L, plus_zero), 0, 0, UNDERFLOW_EXCEPTION);
  check_complex ("clog10 (0x1.fp+16383 - 0x1p-16494 i) == 4932.061660674182269085496060792589701158 - 0 i",  FUNC(clog10) (BUILD_COMPLEX (0x1.fp+16383L, -0x1p-16494L)), BUILD_COMPLEX (4932.061660674182269085496060792589701158L, minus_zero), 0, 0, UNDERFLOW_EXCEPTION);
# endif
#endif

  check_complex ("clog10 (1.0 + 0x1.234566p-10 i) == 2.680828048441605163181684680300513080769e-7 + 4.825491868832381486767558728169977751564e-4 i",  FUNC(clog10) (BUILD_COMPLEX (1.0L, 0x1.234566p-10L)), BUILD_COMPLEX (2.680828048441605163181684680300513080769e-7L, 4.825491868832381486767558728169977751564e-4L), DELTA996, 0, 0);
  check_complex ("clog10 (-1.0 + 0x1.234566p-20 i) == 2.556638434669064077889576526006849923281e-13 + 1.364375882602207106407956770293808181427 i",  FUNC(clog10) (BUILD_COMPLEX (-1.0L, 0x1.234566p-20L)), BUILD_COMPLEX (2.556638434669064077889576526006849923281e-13L, 1.364375882602207106407956770293808181427L), DELTA997, 0, 0);
  check_complex ("clog10 (0x1.234566p-30 + 1.0 i) == 2.438200411482400072282924063740535840474e-19 + 6.821881764607257184291586401763604544928e-1 i",  FUNC(clog10) (BUILD_COMPLEX (0x1.234566p-30L, 1.0L)), BUILD_COMPLEX (2.438200411482400072282924063740535840474e-19L, 6.821881764607257184291586401763604544928e-1L), DELTA998, 0, 0);
  check_complex ("clog10 (-0x1.234566p-40 - 1.0 i) == 2.325249110681915353442924915876654139373e-25 - 6.821881769213700828789403802671540158935e-1 i",  FUNC(clog10) (BUILD_COMPLEX (-0x1.234566p-40L, -1.0L)), BUILD_COMPLEX (2.325249110681915353442924915876654139373e-25L, -6.821881769213700828789403802671540158935e-1L), 0, 0, 0);
  check_complex ("clog10 (0x1.234566p-50 + 1.0 i) == 2.217530356103816369479108963807448194409e-31 + 6.821881769209202348667823902864283966959e-1 i",  FUNC(clog10) (BUILD_COMPLEX (0x1.234566p-50L, 1.0L)), BUILD_COMPLEX (2.217530356103816369479108963807448194409e-31L, 6.821881769209202348667823902864283966959e-1L), DELTA1000, 0, 0);
  check_complex ("clog10 (0x1.234566p-60 + 1.0 i) == 2.114801746467415208319767917450504756866e-37 + 6.821881769209206733143018621078368211515e-1 i",  FUNC(clog10) (BUILD_COMPLEX (0x1.234566p-60L, 1.0L)), BUILD_COMPLEX (2.114801746467415208319767917450504756866e-37L, 6.821881769209206733143018621078368211515e-1L), DELTA1001, 0, 0);
  check_complex ("clog10 (0x1p-61 + 1.0 i) == 4.084085680564517578238994467153626207224e-38 + 6.821881769209206735545466044044889962925e-1 i",  FUNC(clog10) (BUILD_COMPLEX (0x1p-61L, 1.0L)), BUILD_COMPLEX (4.084085680564517578238994467153626207224e-38L, 6.821881769209206735545466044044889962925e-1L), DELTA1002, 0, 0);
  check_complex ("clog10 (0x1p-62 + 1.0 i) == 1.021021420141129394559748616788406551878e-38 + 6.821881769209206736487192085600834406988e-1 i",  FUNC(clog10) (BUILD_COMPLEX (0x1p-62L, 1.0L)), BUILD_COMPLEX (1.021021420141129394559748616788406551878e-38L, 6.821881769209206736487192085600834406988e-1L), DELTA1003, 0, UNDERFLOW_EXCEPTION_FLOAT);
  check_complex ("clog10 (0x1p-63 + 1.0 i) == 2.552553550352823486399371541971016379740e-39 + 6.821881769209206736958055106378806629019e-1 i",  FUNC(clog10) (BUILD_COMPLEX (0x1p-63L, 1.0L)), BUILD_COMPLEX (2.552553550352823486399371541971016379740e-39L, 6.821881769209206736958055106378806629019e-1L), DELTA1004, 0, UNDERFLOW_EXCEPTION_FLOAT);
#ifndef TEST_FLOAT
  check_complex ("clog10 (0x1p-509 + 1.0 i) == 7.730698388614835910296270976605350994446e-308 + 6.821881769209206737428918127156778851051e-1 i",  FUNC(clog10) (BUILD_COMPLEX (0x1p-509L, 1.0L)), BUILD_COMPLEX (7.730698388614835910296270976605350994446e-308L, 6.821881769209206737428918127156778851051e-1L), DELTA1005, 0, UNDERFLOW_EXCEPTION_LDOUBLE_IBM);
  check_complex ("clog10 (0x1p-510 + 1.0 i) == 1.932674597153708977574067744151337748612e-308 + 6.821881769209206737428918127156778851051e-1 i",  FUNC(clog10) (BUILD_COMPLEX (0x1p-510L, 1.0L)), BUILD_COMPLEX (1.932674597153708977574067744151337748612e-308L, 6.821881769209206737428918127156778851051e-1L), DELTA1006, 0, UNDERFLOW_EXCEPTION_DOUBLE);
  check_complex ("clog10 (0x1p-511 + 1.0 i) == 4.831686492884272443935169360378344371529e-309 + 6.821881769209206737428918127156778851051e-1 i",  FUNC(clog10) (BUILD_COMPLEX (0x1p-511L, 1.0L)), BUILD_COMPLEX (4.831686492884272443935169360378344371529e-309L, 6.821881769209206737428918127156778851051e-1L), DELTA1007, 0, UNDERFLOW_EXCEPTION_DOUBLE);
#endif
#if defined TEST_LDOUBLE && LDBL_MAX_EXP >= 16384
  check_complex ("clog10 (0x1p-8189 + 1.0 i) == 1.168114274114528946314738738025008370069e-4931 + 6.821881769209206737428918127156778851051e-1 i",  FUNC(clog10) (BUILD_COMPLEX (0x1p-8189L, 1.0L)), BUILD_COMPLEX (1.168114274114528946314738738025008370069e-4931L, 6.821881769209206737428918127156778851051e-1L), 0, 0, 0);
  check_complex ("clog10 (0x1p-8190 + 1.0 i) == 2.920285685286322365786846845062520925172e-4932 + 6.821881769209206737428918127156778851051e-1 i",  FUNC(clog10) (BUILD_COMPLEX (0x1p-8190L, 1.0L)), BUILD_COMPLEX (2.920285685286322365786846845062520925172e-4932L, 6.821881769209206737428918127156778851051e-1L), DELTA1009, 0, UNDERFLOW_EXCEPTION);
  check_complex ("clog10 (0x1p-8191 + 1.0 i) == 7.300714213215805914467117112656302312931e-4933 + 6.821881769209206737428918127156778851051e-1 i",  FUNC(clog10) (BUILD_COMPLEX (0x1p-8191L, 1.0L)), BUILD_COMPLEX (7.300714213215805914467117112656302312931e-4933L, 6.821881769209206737428918127156778851051e-1L), 0, 0, UNDERFLOW_EXCEPTION);
#endif

  check_complex ("clog10 (0x1.000566p0 + 0x1.234p-10 i) == 3.604093470239754109961125085078190708674e-5 + 4.824745078422174667425851670822596859720e-4 i",  FUNC(clog10) (BUILD_COMPLEX (0x1.000566p0L, 0x1.234p-10L)), BUILD_COMPLEX (3.604093470239754109961125085078190708674e-5L, 4.824745078422174667425851670822596859720e-4L), DELTA1011, 0, 0);
  check_complex ("clog10 (0x1.000566p0 + 0x1.234p-100 i) == 3.577293486783822178310971763308187385546e-5 + 3.897399639875661463735636919790792140598e-31 i",  FUNC(clog10) (BUILD_COMPLEX (0x1.000566p0L, 0x1.234p-100L)), BUILD_COMPLEX (3.577293486783822178310971763308187385546e-5L, 3.897399639875661463735636919790792140598e-31L), DELTA1012, 0, 0);
#ifndef TEST_FLOAT
  check_complex ("clog10 (-0x1.0000000123456p0 + 0x1.2345678p-30 i) == 1.150487028947346337782682105935961875822e-10 + 1.364376353381646356131680448946397884147 i",  FUNC(clog10) (BUILD_COMPLEX (-0x1.0000000123456p0L, 0x1.2345678p-30L)), BUILD_COMPLEX (1.150487028947346337782682105935961875822e-10L, 1.364376353381646356131680448946397884147L), DELTA1013, 0, 0);
  check_complex ("clog10 (-0x1.0000000123456p0 + 0x1.2345678p-1000 i) == 1.150487026509145544402795327729455391948e-10 + 1.364376353841841347485783625431355770210 i",  FUNC(clog10) (BUILD_COMPLEX (-0x1.0000000123456p0L, 0x1.2345678p-1000L)), BUILD_COMPLEX (1.150487026509145544402795327729455391948e-10L, 1.364376353841841347485783625431355770210L), DELTA1014, 0, 0);
#endif
#if defined TEST_LDOUBLE && LDBL_MANT_DIG >= 106
  check_complex ("clog10 (0x1.00000000000000123456789abcp0 + 0x1.23456789p-60 i) == 4.285899851347756188767674032946882584784e-19 + 4.285899850759344225805480528847018395861e-19 i",  FUNC(clog10) (BUILD_COMPLEX (0x1.00000000000000123456789abcp0L, 0x1.23456789p-60L)), BUILD_COMPLEX (4.285899851347756188767674032946882584784e-19L, 4.285899850759344225805480528847018395861e-19L), 0, 0, 0);
  check_complex ("clog10 (0x1.00000000000000123456789abcp0 + 0x1.23456789p-1000 i) == 4.285899851347756186652871946325962330640e-19 + 4.611541215247321502041995872887317363241e-302 i",  FUNC(clog10) (BUILD_COMPLEX (0x1.00000000000000123456789abcp0L, 0x1.23456789p-1000L)), BUILD_COMPLEX (4.285899851347756186652871946325962330640e-19L, 4.611541215247321502041995872887317363241e-302L), 0, 0, UNDERFLOW_EXCEPTION_LDOUBLE_IBM);
#endif

  check_complex ("clog10 (0x0.ffffffp0 + 0x0.ffffffp-100 i) == -2.588596909321764128428416045209904492216e-8 + 3.425979381266895667295625489912064603415e-31 i",  FUNC(clog10) (BUILD_COMPLEX (0x0.ffffffp0L, 0x0.ffffffp-100L)), BUILD_COMPLEX (-2.588596909321764128428416045209904492216e-8L, 3.425979381266895667295625489912064603415e-31L), DELTA1017, 0, 0);
#ifndef TEST_FLOAT
  check_complex ("clog10 (0x0.fffffffffffff8p0 + 0x0.fffffffffffff8p-1000 i) == -4.821637332766435821255375046554377090472e-17 + 4.053112396770095089737411317782466262176e-302 i",  FUNC(clog10) (BUILD_COMPLEX (0x0.fffffffffffff8p0L, 0x0.fffffffffffff8p-1000L)), BUILD_COMPLEX (-4.821637332766435821255375046554377090472e-17L, 4.053112396770095089737411317782466262176e-302L), DELTA1018, 0, UNDERFLOW_EXCEPTION_LDOUBLE_IBM);
#endif
#if defined TEST_LDOUBLE && LDBL_MIN_EXP <= -16381
  check_complex ("clog10 (0x0.ffffffffffffffffp0 + 0x0.ffffffffffffffffp-15000 i) == -2.354315103889861110220423157644627849164e-20 + 1.541165759405643564697852372112893034397e-4516 i",  FUNC(clog10) (BUILD_COMPLEX (0x0.ffffffffffffffffp0L, 0x0.ffffffffffffffffp-15000L)), BUILD_COMPLEX (-2.354315103889861110220423157644627849164e-20L, 1.541165759405643564697852372112893034397e-4516L), DELTA1019, 0, 0);
#endif

  check_complex ("clog10 (0x1a6p-10 + 0x3a5p-10 i) == -6.2126412844802358329771948751248003038444e-07 + 0.4977135139537443711784513409096950995985 i",  FUNC(clog10) (BUILD_COMPLEX (0x1a6p-10L, 0x3a5p-10L)), BUILD_COMPLEX (-6.2126412844802358329771948751248003038444e-07L, 0.4977135139537443711784513409096950995985L), DELTA1020, 0, 0);
  check_complex ("clog10 (0xf2p-10 + 0x3e3p-10 i) == 2.6921240173351112953324592659528481616879e-06 + 0.5785726025799636431142862788413361783862 i",  FUNC(clog10) (BUILD_COMPLEX (0xf2p-10L, 0x3e3p-10L)), BUILD_COMPLEX (2.6921240173351112953324592659528481616879e-06L, 0.5785726025799636431142862788413361783862L), DELTA1021, 0, 0);
  check_complex ("clog10 (0x4d4ep-15 + 0x6605p-15 i) == -7.0781945783414996953799915941870192015212e-09 + 0.4005747524909781155537088181659175147564 i",  FUNC(clog10) (BUILD_COMPLEX (0x4d4ep-15L, 0x6605p-15L)), BUILD_COMPLEX (-7.0781945783414996953799915941870192015212e-09L, 0.4005747524909781155537088181659175147564L), DELTA1022, 0, 0);
  check_complex ("clog10 (0x2818p-15 + 0x798fp-15 i) == 6.6737261053986614395049481326819059203910e-09 + 0.5438241985991753781478398141908629586460 i",  FUNC(clog10) (BUILD_COMPLEX (0x2818p-15L, 0x798fp-15L)), BUILD_COMPLEX (6.6737261053986614395049481326819059203910e-09L, 0.5438241985991753781478398141908629586460L), DELTA1023, 0, 0);
  check_complex ("clog10 (0x9b57bp-20 + 0xcb7b4p-20 i) == -1.7182001068739620267773842120965071561416e-11 + 0.3990121149225253562859800593935899629087 i",  FUNC(clog10) (BUILD_COMPLEX (0x9b57bp-20L, 0xcb7b4p-20L)), BUILD_COMPLEX (-1.7182001068739620267773842120965071561416e-11L, 0.3990121149225253562859800593935899629087L), DELTA1024, 0, 0);
  check_complex ("clog10 (0x2731p-20 + 0xfffd0p-20 i) == 1.9156943718715958194239364991329064049438e-11 + 0.6780326907904082601285090019969008967595 i",  FUNC(clog10) (BUILD_COMPLEX (0x2731p-20L, 0xfffd0p-20L)), BUILD_COMPLEX (1.9156943718715958194239364991329064049438e-11L, 0.6780326907904082601285090019969008967595L), 0, 0, 0);
  check_complex ("clog10 (0x2ede88p-23 + 0x771c3fp-23 i) == -1.9440841725722970687903291200493082253766e-13 + 0.5193774116724956222518530053006822210323 i",  FUNC(clog10) (BUILD_COMPLEX (0x2ede88p-23L, 0x771c3fp-23L)), BUILD_COMPLEX (-1.9440841725722970687903291200493082253766e-13L, 0.5193774116724956222518530053006822210323L), DELTA1026, 0, 0);
  check_complex ("clog10 (0x11682p-23 + 0x7ffed1p-23 i) == 5.0916490233953865181284669870035717560498e-13 + 0.6784968969384861816694467029319146542069 i",  FUNC(clog10) (BUILD_COMPLEX (0x11682p-23L, 0x7ffed1p-23L)), BUILD_COMPLEX (5.0916490233953865181284669870035717560498e-13L, 0.6784968969384861816694467029319146542069L), DELTA1027, 0, 0);
  check_complex ("clog10 (0xa1f2c1p-24 + 0xc643aep-24 i) == -4.5516256421319921959681423447271490869664e-14 + 0.3847315790697197749315054516562206543710 i",  FUNC(clog10) (BUILD_COMPLEX (0xa1f2c1p-24L, 0xc643aep-24L)), BUILD_COMPLEX (-4.5516256421319921959681423447271490869664e-14L, 0.3847315790697197749315054516562206543710L), 0, 0, 0);
  check_complex ("clog10 (0x659feap-24 + 0xeaf6f9p-24 i) == 1.6200701438094619117335617123525612051457e-14 + 0.5049027913635038013499728086604870749732 i",  FUNC(clog10) (BUILD_COMPLEX (0x659feap-24L, 0xeaf6f9p-24L)), BUILD_COMPLEX (1.6200701438094619117335617123525612051457e-14L, 0.5049027913635038013499728086604870749732L), DELTA1029, 0, 0);
#ifndef TEST_FLOAT
  check_complex ("clog10 (0x4447d7175p-35 + 0x6c445e00ap-35 i) == -6.4375803621988389731799033530075237868110e-21 + 0.4378257977686804492768642780897650927167 i",  FUNC(clog10) (BUILD_COMPLEX (0x4447d7175p-35L, 0x6c445e00ap-35L)), BUILD_COMPLEX (-6.4375803621988389731799033530075237868110e-21L, 0.4378257977686804492768642780897650927167L), DELTA1030, 0, 0);
  check_complex ("clog10 (0x2dd46725bp-35 + 0x7783a1284p-35 i) == 1.9312741086596516918394613098872836703188e-20 + 0.5231613813514771042838490538484014771862 i",  FUNC(clog10) (BUILD_COMPLEX (0x2dd46725bp-35L, 0x7783a1284p-35L)), BUILD_COMPLEX (1.9312741086596516918394613098872836703188e-20L, 0.5231613813514771042838490538484014771862L), DELTA1031, 0, 0);
  check_complex ("clog10 (0x164c74eea876p-45 + 0x16f393482f77p-45 i) == -1.3155760824064879362415202279780039150764e-26 + 0.3473590599762514228227328130640352044313 i",  FUNC(clog10) (BUILD_COMPLEX (0x164c74eea876p-45L, 0x16f393482f77p-45L)), BUILD_COMPLEX (-1.3155760824064879362415202279780039150764e-26L, 0.3473590599762514228227328130640352044313L), DELTA1032, 0, 0);
  check_complex ("clog10 (0xfe961079616p-45 + 0x1bc37e09e6d1p-45 i) == 2.3329549194675052736016290082882121135546e-26 + 0.4561756099441139182878993697611751382976 i",  FUNC(clog10) (BUILD_COMPLEX (0xfe961079616p-45L, 0x1bc37e09e6d1p-45L)), BUILD_COMPLEX (2.3329549194675052736016290082882121135546e-26L, 0.4561756099441139182878993697611751382976L), DELTA1033, 0, 0);
  check_complex ("clog10 (0xa4722f19346cp-51 + 0x7f9631c5e7f07p-51 i) == -2.6979587627476803379953050733225113494503e-30 + 0.6472785229986997177606324374555347813105 i",  FUNC(clog10) (BUILD_COMPLEX (0xa4722f19346cp-51L, 0x7f9631c5e7f07p-51L)), BUILD_COMPLEX (-2.6979587627476803379953050733225113494503e-30L, 0.6472785229986997177606324374555347813105L), 0, 0, 0);
  check_complex ("clog10 (0x10673dd0f2481p-51 + 0x7ef1d17cefbd2p-51 i) == 1.3918041236396763648388478552321724382899e-29 + 0.6263795733790237053262025311642907438291 i",  FUNC(clog10) (BUILD_COMPLEX (0x10673dd0f2481p-51L, 0x7ef1d17cefbd2p-51L)), BUILD_COMPLEX (1.3918041236396763648388478552321724382899e-29L, 0.6263795733790237053262025311642907438291L), DELTA1035, 0, 0);
  check_complex ("clog10 (0x8ecbf810c4ae6p-52 + 0xd479468b09a37p-52 i) == -4.2289432987513243393180377141513840878196e-30 + 0.4252020027092323591068799049905597805296 i",  FUNC(clog10) (BUILD_COMPLEX (0x8ecbf810c4ae6p-52L, 0xd479468b09a37p-52L)), BUILD_COMPLEX (-4.2289432987513243393180377141513840878196e-30L, 0.4252020027092323591068799049905597805296L), DELTA1036, 0, 0);
  check_complex ("clog10 (0x5b06b680ea2ccp-52 + 0xef452b965da9fp-52 i) == 3.6079845358966994996207055940336690133424e-30 + 0.5243112258263349992771652393178033846555 i",  FUNC(clog10) (BUILD_COMPLEX (0x5b06b680ea2ccp-52L, 0xef452b965da9fp-52L)), BUILD_COMPLEX (3.6079845358966994996207055940336690133424e-30L, 0.5243112258263349992771652393178033846555L), DELTA1037, 0, 0);
  check_complex ("clog10 (0x659b70ab7971bp-53 + 0x1f5d111e08abecp-53 i) == -1.0893543813872082317104059174982092534059e-30 + 0.5954257879188711495921161433751775633232 i",  FUNC(clog10) (BUILD_COMPLEX (0x659b70ab7971bp-53L, 0x1f5d111e08abecp-53L)), BUILD_COMPLEX (-1.0893543813872082317104059174982092534059e-30L, 0.5954257879188711495921161433751775633232L), DELTA1038, 0, 0);
  check_complex ("clog10 (0x15cfbd1990d1ffp-53 + 0x176a3973e09a9ap-53 i) == 4.4163015461643576961232672330852798804976e-31 + 0.3564851427422832755956993418877523303529 i",  FUNC(clog10) (BUILD_COMPLEX (0x15cfbd1990d1ffp-53L, 0x176a3973e09a9ap-53L)), BUILD_COMPLEX (4.4163015461643576961232672330852798804976e-31L, 0.3564851427422832755956993418877523303529L), 0, 0, 0);
  check_complex ("clog10 (0x1367a310575591p-54 + 0x3cfcc0a0541f60p-54 i) == 2.2081507730821788480616336165447731164865e-32 + 0.5484039935757001196548030312819898864760 i",  FUNC(clog10) (BUILD_COMPLEX (0x1367a310575591p-54L, 0x3cfcc0a0541f60p-54L)), BUILD_COMPLEX (2.2081507730821788480616336165447731164865e-32L, 0.5484039935757001196548030312819898864760L), DELTA1040, 0, 0);
  check_complex ("clog10 (0x55cb6d0c83af5p-55 + 0x7fe33c0c7c4e90p-55 i) == -2.2583360179249556400630343805573865814771e-32 + 0.6639894257763289307423302343317622430835 i",  FUNC(clog10) (BUILD_COMPLEX (0x55cb6d0c83af5p-55L, 0x7fe33c0c7c4e90p-55L)), BUILD_COMPLEX (-2.2583360179249556400630343805573865814771e-32L, 0.6639894257763289307423302343317622430835L), 0, 0, 0);
#endif
#if defined TEST_LDOUBLE && LDBL_MANT_DIG >= 64
  check_complex ("clog10 (0x298c62cb546588a7p-63 + 0x7911b1dfcc4ecdaep-63 i) == -5.1816837072162316773907242302011632570857e-37 + 0.5386167838952956925896424154370364458140 i",  FUNC(clog10) (BUILD_COMPLEX (0x298c62cb546588a7p-63L, 0x7911b1dfcc4ecdaep-63L)), BUILD_COMPLEX (-5.1816837072162316773907242302011632570857e-37L, 0.5386167838952956925896424154370364458140L), 0, 0, 0);
  check_complex ("clog10 (0x4d9c37e2b5cb4533p-63 + 0x65c98be2385a042ep-63 i) == 2.7822833698845776001753149807484078521508e-37 + 0.3992725998539071066769046272515417679815 i",  FUNC(clog10) (BUILD_COMPLEX (0x4d9c37e2b5cb4533p-63L, 0x65c98be2385a042ep-63L)), BUILD_COMPLEX (2.7822833698845776001753149807484078521508e-37L, 0.3992725998539071066769046272515417679815L), 0, 0, 0);
  check_complex ("clog10 (0x602fd5037c4792efp-64 + 0xed3e2086dcca80b8p-64 i) == -1.0146400362652473358437501879334790111898e-37 + 0.5149047982335273098246594109614460842099 i",  FUNC(clog10) (BUILD_COMPLEX (0x602fd5037c4792efp-64L, 0xed3e2086dcca80b8p-64L)), BUILD_COMPLEX (-1.0146400362652473358437501879334790111898e-37L, 0.5149047982335273098246594109614460842099L), 0, 0, 0);
  check_complex ("clog10 (0x6b10b4f3520217b6p-64 + 0xe8893cbb449253a1p-64 i) == 1.0529283395205396881397407610630442563938e-37 + 0.4947949395762683446121140513971996916447 i",  FUNC(clog10) (BUILD_COMPLEX (0x6b10b4f3520217b6p-64L, 0xe8893cbb449253a1p-64L)), BUILD_COMPLEX (1.0529283395205396881397407610630442563938e-37L, 0.4947949395762683446121140513971996916447L), DELTA1045, 0, 0);
  check_complex ("clog10 (0x81b7efa81fc35ad1p-65 + 0x1ef4b835f1c79d812p-65 i) == -4.3074341162203896332989394770760901408798e-39 + 0.5709443672155660428417571212549720987784 i",  FUNC(clog10) (BUILD_COMPLEX (0x81b7efa81fc35ad1p-65L, 0x1ef4b835f1c79d812p-65L)), BUILD_COMPLEX (-4.3074341162203896332989394770760901408798e-39L, 0.5709443672155660428417571212549720987784L), DELTA1046, 0, 0);
#endif
#if defined TEST_LDOUBLE && LDBL_MANT_DIG >= 106
  check_complex ("clog10 (0x3f96469050f650869c2p-75 + 0x6f16b2c9c8b05988335p-75 i) == -4.5643214291682663316715446865040356750881e-46 + 0.4564083863660793840592614609053162690362 i",  FUNC(clog10) (BUILD_COMPLEX (0x3f96469050f650869c2p-75L, 0x6f16b2c9c8b05988335p-75L)), BUILD_COMPLEX (-4.5643214291682663316715446865040356750881e-46L, 0.4564083863660793840592614609053162690362L), 0, 0, 0);
  check_complex ("clog10 (0x3157fc1d73233e580c8p-75 + 0x761b52ccd435d7c7f5fp-75 i) == 5.8575458340992751256451490143468457830297e-44 + 0.5103174273246635294300470585396890237265 i",  FUNC(clog10) (BUILD_COMPLEX (0x3157fc1d73233e580c8p-75L, 0x761b52ccd435d7c7f5fp-75L)), BUILD_COMPLEX (5.8575458340992751256451490143468457830297e-44L, 0.5103174273246635294300470585396890237265L), 0, 0, 0);
  check_complex ("clog10 (0x155f8afc4c48685bf63610p-85 + 0x17d0cf2652cdbeb1294e19p-85 i) == -2.0748709499710785084693619097712106753591e-50 + 0.3645447681189598740620098186365764884771 i",  FUNC(clog10) (BUILD_COMPLEX (0x155f8afc4c48685bf63610p-85L, 0x17d0cf2652cdbeb1294e19p-85L)), BUILD_COMPLEX (-2.0748709499710785084693619097712106753591e-50L, 0.3645447681189598740620098186365764884771L), 0, 0, 0);
  check_complex ("clog10 (0x13836d58a13448d750b4b9p-85 + 0x195ca7bc3ab4f9161edbe6p-85 i) == 1.2333149003324592532859843519619084433953e-50 + 0.3973779298829931059309198145608711073016 i",  FUNC(clog10) (BUILD_COMPLEX (0x13836d58a13448d750b4b9p-85L, 0x195ca7bc3ab4f9161edbe6p-85L)), BUILD_COMPLEX (1.2333149003324592532859843519619084433953e-50L, 0.3973779298829931059309198145608711073016L), 0, 0, 0);
  check_complex ("clog10 (0x1df515eb171a808b9e400266p-95 + 0x7c71eb0cd4688dfe98581c77p-95 i) == -1.5221162575729652613635150540947625639689e-57 + 0.5795934880811949230121092882659698986043 i",  FUNC(clog10) (BUILD_COMPLEX (0x1df515eb171a808b9e400266p-95L, 0x7c71eb0cd4688dfe98581c77p-95L)), BUILD_COMPLEX (-1.5221162575729652613635150540947625639689e-57L, 0.5795934880811949230121092882659698986043L), 0, 0, 0);
  check_complex ("clog10 (0xe33f66c9542ca25cc43c867p-95 + 0x7f35a68ebd3704a43c465864p-95 i) == 1.7850272475173865337808494725293124613817e-56 + 0.6338990862456906754888183278564382516852 i",  FUNC(clog10) (BUILD_COMPLEX (0xe33f66c9542ca25cc43c867p-95L, 0x7f35a68ebd3704a43c465864p-95L)), BUILD_COMPLEX (1.7850272475173865337808494725293124613817e-56L, 0.6338990862456906754888183278564382516852L), 0, 0, 0);
  check_complex ("clog10 (0x6771f22c64ed551b857c128b4cp-105 + 0x1f570e7a13cc3cf2f44fd793ea1p-105 i) == -6.2023045024810589256360494043570293518879e-63 + 0.5938345819561308555003145899438513900776 i",  FUNC(clog10) (BUILD_COMPLEX (0x6771f22c64ed551b857c128b4cp-105L, 0x1f570e7a13cc3cf2f44fd793ea1p-105L)), BUILD_COMPLEX (-6.2023045024810589256360494043570293518879e-63L, 0.5938345819561308555003145899438513900776L), 0, 0, 0);
  check_complex ("clog10 (0x15d8ab6ed05ca514086ac3a1e84p-105 + 0x1761e480aa094c0b10b34b09ce9p-105 i) == 4.3548095442952115860848857519953610343042e-63 + 0.3558376234889641500775150477035448866763 i",  FUNC(clog10) (BUILD_COMPLEX (0x15d8ab6ed05ca514086ac3a1e84p-105L, 0x1761e480aa094c0b10b34b09ce9p-105L)), BUILD_COMPLEX (4.3548095442952115860848857519953610343042e-63L, 0.3558376234889641500775150477035448866763L), 0, 0, 0);
  check_complex ("clog10 (0x187190c1a334497bdbde5a95f48p-106 + 0x3b25f08062d0a095c4cfbbc338dp-106 i) == -7.5879257211204444302994221436282805900756e-64 + 0.5119945461708707332160859198685423099187 i",  FUNC(clog10) (BUILD_COMPLEX (0x187190c1a334497bdbde5a95f48p-106L, 0x3b25f08062d0a095c4cfbbc338dp-106L)), BUILD_COMPLEX (-7.5879257211204444302994221436282805900756e-64L, 0.5119945461708707332160859198685423099187L), 0, 0, 0);
  check_complex ("clog10 (0x6241ef0da53f539f02fad67dabp-106 + 0x3fb46641182f7efd9caa769dac0p-106 i) == 1.8804859395820231849002915747252695375405e-63 + 0.6404513901551516189871978418046651877394 i",  FUNC(clog10) (BUILD_COMPLEX (0x6241ef0da53f539f02fad67dabp-106L, 0x3fb46641182f7efd9caa769dac0p-106L)), BUILD_COMPLEX (1.8804859395820231849002915747252695375405e-63L, 0.6404513901551516189871978418046651877394L), 0, 0, 0);
#endif
#if defined TEST_LDOUBLE && LDBL_MANT_DIG >= 113
  check_complex ("clog10 (0x3e1d0a105ac4ebeacd9c6952d34cp-112 + 0xf859b3d1b06d005dcbb5516d5479p-112 i) == -5.0742964549782184008668435276046798273476e-67 + 0.5757527761596220360985719127090110408283 i",  FUNC(clog10) (BUILD_COMPLEX (0x3e1d0a105ac4ebeacd9c6952d34cp-112L, 0xf859b3d1b06d005dcbb5516d5479p-112L)), BUILD_COMPLEX (-5.0742964549782184008668435276046798273476e-67L, 0.5757527761596220360985719127090110408283L), 0, 0, 0);
  check_complex ("clog10 (0x47017a2e36807acb1e5214b209dep-112 + 0xf5f4a550c9d75e3bb1839d865f0dp-112 i) == 6.5482587585671294601662599808612773010057e-66 + 0.5601289501766423782280643144987875760229 i",  FUNC(clog10) (BUILD_COMPLEX (0x47017a2e36807acb1e5214b209dep-112L, 0xf5f4a550c9d75e3bb1839d865f0dp-112L)), BUILD_COMPLEX (6.5482587585671294601662599808612773010057e-66L, 0.5601289501766423782280643144987875760229L), 0, 0, 0);
  check_complex ("clog10 (0x148f818cb7a9258fca942ade2a0cap-113 + 0x18854a34780b8333ec53310ad7001p-113 i) == -3.1210950417524756037077807411854181477733e-67 + 0.3791463562379872585396164879981280044658 i",  FUNC(clog10) (BUILD_COMPLEX (0x148f818cb7a9258fca942ade2a0cap-113L, 0x18854a34780b8333ec53310ad7001p-113L)), BUILD_COMPLEX (-3.1210950417524756037077807411854181477733e-67L, 0.3791463562379872585396164879981280044658L), 0, 0, 0);
  check_complex ("clog10 (0xfd95243681c055c2632286921092p-113 + 0x1bccabcd29ca2152860ec29e34ef7p-113 i) == 2.8774482675253468630312378575186855052697e-66 + 0.4571561610046221605554903008571429975493 i",  FUNC(clog10) (BUILD_COMPLEX (0xfd95243681c055c2632286921092p-113L, 0x1bccabcd29ca2152860ec29e34ef7p-113L)), BUILD_COMPLEX (2.8774482675253468630312378575186855052697e-66L, 0.4571561610046221605554903008571429975493L), 0, 0, 0);
  check_complex ("clog10 (0xdb85c467ee2aadd5f425fe0f4b8dp-114 + 0x3e83162a0f95f1dcbf97dddf410eap-114 i) == 1.9985076315737626043096596036300177494613e-67 + 0.5883569274304683249184005177865521205198 i",  FUNC(clog10) (BUILD_COMPLEX (0xdb85c467ee2aadd5f425fe0f4b8dp-114L, 0x3e83162a0f95f1dcbf97dddf410eap-114L)), BUILD_COMPLEX (1.9985076315737626043096596036300177494613e-67L, 0.5883569274304683249184005177865521205198L), 0, 0, 0);
  check_complex ("clog10 (0x1415bcaf2105940d49a636e98ae59p-115 + 0x7e6a150adfcd1b0921d44b31f40f4p-115 i) == 1.1288799405048268615023706955013387413519e-67 + 0.6137587762850841972073301550420510507903 i",  FUNC(clog10) (BUILD_COMPLEX (0x1415bcaf2105940d49a636e98ae59p-115L, 0x7e6a150adfcd1b0921d44b31f40f4p-115L)), BUILD_COMPLEX (1.1288799405048268615023706955013387413519e-67L, 0.6137587762850841972073301550420510507903L), 0, 0, 0);
#endif

  print_complex_max_error ("clog10", DELTAclog10, 0);
}


static void
conj_test (void)
{
  init_max_error ();
  check_complex ("conj (0.0 + 0.0 i) == 0.0 - 0 i",  FUNC(conj) (BUILD_COMPLEX (0.0, 0.0)), BUILD_COMPLEX (0.0, minus_zero), 0, 0, 0);
  check_complex ("conj (0.0 - 0 i) == 0.0 + 0.0 i",  FUNC(conj) (BUILD_COMPLEX (0.0, minus_zero)), BUILD_COMPLEX (0.0, 0.0), 0, 0, 0);
  check_complex ("conj (NaN + NaN i) == NaN + NaN i",  FUNC(conj) (BUILD_COMPLEX (nan_value, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, 0);
  check_complex ("conj (inf - inf i) == inf + inf i",  FUNC(conj) (BUILD_COMPLEX (plus_infty, minus_infty)), BUILD_COMPLEX (plus_infty, plus_infty), 0, 0, 0);
  check_complex ("conj (inf + inf i) == inf - inf i",  FUNC(conj) (BUILD_COMPLEX (plus_infty, plus_infty)), BUILD_COMPLEX (plus_infty, minus_infty), 0, 0, 0);
  check_complex ("conj (1.0 + 2.0 i) == 1.0 - 2.0 i",  FUNC(conj) (BUILD_COMPLEX (1.0, 2.0)), BUILD_COMPLEX (1.0, -2.0), 0, 0, 0);
  check_complex ("conj (3.0 - 4.0 i) == 3.0 + 4.0 i",  FUNC(conj) (BUILD_COMPLEX (3.0, -4.0)), BUILD_COMPLEX (3.0, 4.0), 0, 0, 0);

  print_complex_max_error ("conj", 0, 0);
}


static void
copysign_test (void)
{
  init_max_error ();

  check_float ("copysign (0, 4) == 0",  FUNC(copysign) (0, 4), 0, 0, 0, 0);
  check_float ("copysign (0, -4) == -0",  FUNC(copysign) (0, -4), minus_zero, 0, 0, 0);
  check_float ("copysign (-0, 4) == 0",  FUNC(copysign) (minus_zero, 4), 0, 0, 0, 0);
  check_float ("copysign (-0, -4) == -0",  FUNC(copysign) (minus_zero, -4), minus_zero, 0, 0, 0);

  check_float ("copysign (inf, 0) == inf",  FUNC(copysign) (plus_infty, 0), plus_infty, 0, 0, 0);
  check_float ("copysign (inf, -0) == -inf",  FUNC(copysign) (plus_infty, minus_zero), minus_infty, 0, 0, 0);
  check_float ("copysign (-inf, 0) == inf",  FUNC(copysign) (minus_infty, 0), plus_infty, 0, 0, 0);
  check_float ("copysign (-inf, -0) == -inf",  FUNC(copysign) (minus_infty, minus_zero), minus_infty, 0, 0, 0);

  check_float ("copysign (0, inf) == 0",  FUNC(copysign) (0, plus_infty), 0, 0, 0, 0);
  check_float ("copysign (0, -0) == -0",  FUNC(copysign) (0, minus_zero), minus_zero, 0, 0, 0);
  check_float ("copysign (-0, inf) == 0",  FUNC(copysign) (minus_zero, plus_infty), 0, 0, 0, 0);
  check_float ("copysign (-0, -0) == -0",  FUNC(copysign) (minus_zero, minus_zero), minus_zero, 0, 0, 0);

  /* XXX More correctly we would have to check the sign of the NaN.  */
  check_float ("copysign (NaN, 0) == NaN",  FUNC(copysign) (nan_value, 0), nan_value, 0, 0, 0);
  check_float ("copysign (NaN, -0) == NaN",  FUNC(copysign) (nan_value, minus_zero), nan_value, 0, 0, 0);
  check_float ("copysign (-NaN, 0) == NaN",  FUNC(copysign) (-nan_value, 0), nan_value, 0, 0, 0);
  check_float ("copysign (-NaN, -0) == NaN",  FUNC(copysign) (-nan_value, minus_zero), nan_value, 0, 0, 0);

  print_max_error ("copysign", 0, 0);
}


static void
cos_test (void)
{
  errno = 0;
  FUNC(cos) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_float ("cos (0) == 1",  FUNC(cos) (0), 1, 0, 0, 0);
  check_float ("cos (-0) == 1",  FUNC(cos) (minus_zero), 1, 0, 0, 0);
  errno = 0;
  check_float ("cos (inf) == NaN",  FUNC(cos) (plus_infty), nan_value, 0, 0, INVALID_EXCEPTION);
  check_int ("errno for cos(+inf) == EDOM", errno, EDOM, 0, 0, 0);
  errno = 0;
  check_float ("cos (-inf) == NaN",  FUNC(cos) (minus_infty), nan_value, 0, 0, INVALID_EXCEPTION);
  check_int ("errno for cos(-inf) == EDOM", errno, EDOM, 0, 0, 0);
  errno = 0;
  check_float ("cos (NaN) == NaN",  FUNC(cos) (nan_value), nan_value, 0, 0, 0);
  check_int ("errno for cos(NaN) unchanged", errno, 0, 0, 0, 0);

  check_float ("cos (M_PI_6l * 2.0) == 0.5",  FUNC(cos) (M_PI_6l * 2.0), 0.5, DELTA1091, 0, 0);
  check_float ("cos (M_PI_6l * 4.0) == -0.5",  FUNC(cos) (M_PI_6l * 4.0), -0.5, DELTA1092, 0, 0);
  check_float ("cos (pi/2) == 0",  FUNC(cos) (M_PI_2l), 0, DELTA1093, 0, 0);

  check_float ("cos (0.75) == 0.731688868873820886311838753000084544",  FUNC(cos) (0.75L), 0.731688868873820886311838753000084544L, 0, 0, 0);

  check_float ("cos (0x1p65) == 0.99888622066058013610642172179340364209972",  FUNC(cos) (0x1p65), 0.99888622066058013610642172179340364209972L, 0, 0, 0);
  check_float ("cos (-0x1p65) == 0.99888622066058013610642172179340364209972",  FUNC(cos) (-0x1p65), 0.99888622066058013610642172179340364209972L, 0, 0, 0);

#ifdef TEST_DOUBLE
  check_float ("cos (0.80190127184058835) == 0.69534156199418473",  FUNC(cos) (0.80190127184058835), 0.69534156199418473, DELTA1097, 0, 0);
#endif

  check_float ("cos (0x1.442f74p+15) == 2.4407839902314016628485779006274989801517e-06",  FUNC(cos) (0x1.442f74p+15), 2.4407839902314016628485779006274989801517e-06L, 0, 0, 0);

#ifndef TEST_FLOAT
  check_float ("cos (1e22) == 0.5232147853951389454975944733847094921409",  FUNC(cos) (1e22), 0.5232147853951389454975944733847094921409L, 0, 0, 0);
  check_float ("cos (0x1p1023) == -0.826369834614147994500785680811743734805",  FUNC(cos) (0x1p1023), -0.826369834614147994500785680811743734805L, 0, 0, 0);
#endif

#if defined TEST_LDOUBLE && LDBL_MAX_EXP >= 16384
  check_float ("cos (0x1p16383) == 0.9210843909921906206874509522505756251609",  FUNC(cos) (0x1p16383L), 0.9210843909921906206874509522505756251609L, 0, 0, 0);
#endif

  check_float ("cos (0x1p+120) == -9.25879022854837867303861764107414946730833e-01",  FUNC(cos) (0x1p+120), -9.25879022854837867303861764107414946730833e-01L, 0, 0, 0);
  check_float ("cos (0x1p+127) == 7.81914638714960072263910298466369236613162e-01",  FUNC(cos) (0x1p+127), 7.81914638714960072263910298466369236613162e-01L, 0, 0, 0);
  check_float ("cos (0x1.fffff8p+127) == 9.98819362551949040703862043664101081064641e-01",  FUNC(cos) (0x1.fffff8p+127), 9.98819362551949040703862043664101081064641e-01L, 0, 0, 0);
  check_float ("cos (0x1.fffffep+127) == 8.53021039830304158051791467692161107353094e-01",  FUNC(cos) (0x1.fffffep+127), 8.53021039830304158051791467692161107353094e-01L, 0, 0, 0);
  check_float ("cos (0x1p+50) == 8.68095904660550604334592502063501320395739e-01",  FUNC(cos) (0x1p+50), 8.68095904660550604334592502063501320395739e-01L, 0, 0, 0);
  check_float ("cos (0x1p+28) == -1.65568979490578758865468278195361551113358e-01",  FUNC(cos) (0x1p+28), -1.65568979490578758865468278195361551113358e-01L, 0, 0, 0);

  print_max_error ("cos", DELTAcos, 0);
}


static void
cos_test_tonearest (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(cos) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_TONEAREST))
    {
  check_float ("cos_tonearest (1) == 0.5403023058681397174009366074429766037323",  FUNC(cos) (1), 0.5403023058681397174009366074429766037323L, 0, 0, 0);
  check_float ("cos_tonearest (2) == -0.4161468365471423869975682295007621897660",  FUNC(cos) (2), -0.4161468365471423869975682295007621897660L, 0, 0, 0);
  check_float ("cos_tonearest (3) == -0.9899924966004454572715727947312613023937",  FUNC(cos) (3), -0.9899924966004454572715727947312613023937L, 0, 0, 0);
  check_float ("cos_tonearest (4) == -0.6536436208636119146391681830977503814241",  FUNC(cos) (4), -0.6536436208636119146391681830977503814241L, 0, 0, 0);
  check_float ("cos_tonearest (5) == 0.2836621854632262644666391715135573083344",  FUNC(cos) (5), 0.2836621854632262644666391715135573083344L, 0, 0, 0);
  check_float ("cos_tonearest (6) == 0.9601702866503660205456522979229244054519",  FUNC(cos) (6), 0.9601702866503660205456522979229244054519L, 0, 0, 0);
  check_float ("cos_tonearest (7) == 0.7539022543433046381411975217191820122183",  FUNC(cos) (7), 0.7539022543433046381411975217191820122183L, DELTA1114, 0, 0);
  check_float ("cos_tonearest (8) == -0.1455000338086135258688413818311946826093",  FUNC(cos) (8), -0.1455000338086135258688413818311946826093L, DELTA1115, 0, 0);
  check_float ("cos_tonearest (9) == -0.9111302618846769883682947111811653112463",  FUNC(cos) (9), -0.9111302618846769883682947111811653112463L, DELTA1116, 0, 0);
  check_float ("cos_tonearest (10) == -0.8390715290764524522588639478240648345199",  FUNC(cos) (10), -0.8390715290764524522588639478240648345199L, 0, 0, 0);
    }

  fesetround (save_round_mode);

  print_max_error ("cos_tonearest", DELTAcos_tonearest, 0);
}


static void
cos_test_towardzero (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(cos) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_TOWARDZERO))
    {
  check_float ("cos_towardzero (1) == 0.5403023058681397174009366074429766037323",  FUNC(cos) (1), 0.5403023058681397174009366074429766037323L, DELTA1118, 0, 0);
  check_float ("cos_towardzero (2) == -0.4161468365471423869975682295007621897660",  FUNC(cos) (2), -0.4161468365471423869975682295007621897660L, DELTA1119, 0, 0);
  check_float ("cos_towardzero (3) == -0.9899924966004454572715727947312613023937",  FUNC(cos) (3), -0.9899924966004454572715727947312613023937L, DELTA1120, 0, 0);
  check_float ("cos_towardzero (4) == -0.6536436208636119146391681830977503814241",  FUNC(cos) (4), -0.6536436208636119146391681830977503814241L, 0, 0, 0);
  check_float ("cos_towardzero (5) == 0.2836621854632262644666391715135573083344",  FUNC(cos) (5), 0.2836621854632262644666391715135573083344L, DELTA1122, 0, 0);
  check_float ("cos_towardzero (6) == 0.9601702866503660205456522979229244054519",  FUNC(cos) (6), 0.9601702866503660205456522979229244054519L, 0, 0, 0);
  check_float ("cos_towardzero (7) == 0.7539022543433046381411975217191820122183",  FUNC(cos) (7), 0.7539022543433046381411975217191820122183L, DELTA1124, 0, 0);
  check_float ("cos_towardzero (8) == -0.1455000338086135258688413818311946826093",  FUNC(cos) (8), -0.1455000338086135258688413818311946826093L, DELTA1125, 0, 0);
  check_float ("cos_towardzero (9) == -0.9111302618846769883682947111811653112463",  FUNC(cos) (9), -0.9111302618846769883682947111811653112463L, 0, 0, 0);
  check_float ("cos_towardzero (10) == -0.8390715290764524522588639478240648345199",  FUNC(cos) (10), -0.8390715290764524522588639478240648345199L, DELTA1127, 0, 0);
    }

  fesetround (save_round_mode);

  print_max_error ("cos_towardzero", DELTAcos_towardzero, 0);
}


static void
cos_test_downward (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(cos) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_DOWNWARD))
    {
  check_float ("cos_downward (1) == 0.5403023058681397174009366074429766037323",  FUNC(cos) (1), 0.5403023058681397174009366074429766037323L, DELTA1128, 0, 0);
  check_float ("cos_downward (2) == -0.4161468365471423869975682295007621897660",  FUNC(cos) (2), -0.4161468365471423869975682295007621897660L, DELTA1129, 0, 0);
  check_float ("cos_downward (3) == -0.9899924966004454572715727947312613023937",  FUNC(cos) (3), -0.9899924966004454572715727947312613023937L, DELTA1130, 0, 0);
  check_float ("cos_downward (4) == -0.6536436208636119146391681830977503814241",  FUNC(cos) (4), -0.6536436208636119146391681830977503814241L, DELTA1131, 0, 0);
  check_float ("cos_downward (5) == 0.2836621854632262644666391715135573083344",  FUNC(cos) (5), 0.2836621854632262644666391715135573083344L, DELTA1132, 0, 0);
  check_float ("cos_downward (6) == 0.9601702866503660205456522979229244054519",  FUNC(cos) (6), 0.9601702866503660205456522979229244054519L, 0, 0, 0);
  check_float ("cos_downward (7) == 0.7539022543433046381411975217191820122183",  FUNC(cos) (7), 0.7539022543433046381411975217191820122183L, DELTA1134, 0, 0);
  check_float ("cos_downward (8) == -0.1455000338086135258688413818311946826093",  FUNC(cos) (8), -0.1455000338086135258688413818311946826093L, DELTA1135, 0, 0);
  check_float ("cos_downward (9) == -0.9111302618846769883682947111811653112463",  FUNC(cos) (9), -0.9111302618846769883682947111811653112463L, DELTA1136, 0, 0);
  check_float ("cos_downward (10) == -0.8390715290764524522588639478240648345199",  FUNC(cos) (10), -0.8390715290764524522588639478240648345199L, DELTA1137, 0, 0);
    }

  fesetround (save_round_mode);

  print_max_error ("cos_downward", DELTAcos_downward, 0);
}


static void
cos_test_upward (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(cos) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_UPWARD))
    {
  check_float ("cos_upward (1) == 0.5403023058681397174009366074429766037323",  FUNC(cos) (1), 0.5403023058681397174009366074429766037323L, DELTA1138, 0, 0);
  check_float ("cos_upward (2) == -0.4161468365471423869975682295007621897660",  FUNC(cos) (2), -0.4161468365471423869975682295007621897660L, DELTA1139, 0, 0);
  check_float ("cos_upward (3) == -0.9899924966004454572715727947312613023937",  FUNC(cos) (3), -0.9899924966004454572715727947312613023937L, DELTA1140, 0, 0);
  check_float ("cos_upward (4) == -0.6536436208636119146391681830977503814241",  FUNC(cos) (4), -0.6536436208636119146391681830977503814241L, DELTA1141, 0, 0);
  check_float ("cos_upward (5) == 0.2836621854632262644666391715135573083344",  FUNC(cos) (5), 0.2836621854632262644666391715135573083344L, DELTA1142, 0, 0);
  check_float ("cos_upward (6) == 0.9601702866503660205456522979229244054519",  FUNC(cos) (6), 0.9601702866503660205456522979229244054519L, DELTA1143, 0, 0);
  check_float ("cos_upward (7) == 0.7539022543433046381411975217191820122183",  FUNC(cos) (7), 0.7539022543433046381411975217191820122183L, DELTA1144, 0, 0);
  check_float ("cos_upward (8) == -0.1455000338086135258688413818311946826093",  FUNC(cos) (8), -0.1455000338086135258688413818311946826093L, DELTA1145, 0, 0);
  check_float ("cos_upward (9) == -0.9111302618846769883682947111811653112463",  FUNC(cos) (9), -0.9111302618846769883682947111811653112463L, DELTA1146, 0, 0);
  check_float ("cos_upward (10) == -0.8390715290764524522588639478240648345199",  FUNC(cos) (10), -0.8390715290764524522588639478240648345199L, DELTA1147, 0, 0);
    }

  fesetround (save_round_mode);

  print_max_error ("cos_upward", DELTAcos_upward, 0);
}


static void
cosh_test (void)
{
  errno = 0;
  FUNC(cosh) (0.7L);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();
  check_float ("cosh (0) == 1",  FUNC(cosh) (0), 1, 0, 0, 0);
  check_float ("cosh (-0) == 1",  FUNC(cosh) (minus_zero), 1, 0, 0, 0);

#ifndef TEST_INLINE
  check_float ("cosh (inf) == inf",  FUNC(cosh) (plus_infty), plus_infty, 0, 0, 0);
  check_float ("cosh (-inf) == inf",  FUNC(cosh) (minus_infty), plus_infty, 0, 0, 0);
#endif
  check_float ("cosh (NaN) == NaN",  FUNC(cosh) (nan_value), nan_value, 0, 0, 0);

  check_float ("cosh (0.75) == 1.29468328467684468784170818539018176",  FUNC(cosh) (0.75L), 1.29468328467684468784170818539018176L, 0, 0, 0);

#ifndef TEST_FLOAT
  check_float ("cosh (709.8893558127259666434838436543941497802734375) == 9.9999998999995070652573675944761818416035e+307",  FUNC(cosh) (709.8893558127259666434838436543941497802734375L), 9.9999998999995070652573675944761818416035e+307L, 0, 0, 0);
  check_float ("cosh (-709.8893558127259666434838436543941497802734375) == 9.9999998999995070652573675944761818416035e+307",  FUNC(cosh) (-709.8893558127259666434838436543941497802734375L), 9.9999998999995070652573675944761818416035e+307L, 0, 0, 0);
#endif

  print_max_error ("cosh", 0, 0);
}


static void
cosh_test_tonearest (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(cosh) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_TONEAREST))
    {
  check_float ("cosh_tonearest (22) == 1792456423.065795780980053377632656584997",  FUNC(cosh) (22), 1792456423.065795780980053377632656584997L, DELTA1156, 0, 0);
  check_float ("cosh_tonearest (23) == 4872401723.124451300068625740569997090344",  FUNC(cosh) (23), 4872401723.124451300068625740569997090344L, 0, 0, 0);
  check_float ("cosh_tonearest (24) == 13244561064.92173614708845674912733665919",  FUNC(cosh) (24), 13244561064.92173614708845674912733665919L, 0, 0, 0);
    }

  fesetround (save_round_mode);

  print_max_error ("cosh_tonearest", DELTAcosh_tonearest, 0);
}


static void
cosh_test_towardzero (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(cosh) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_TOWARDZERO))
    {
  check_float ("cosh_towardzero (22) == 1792456423.065795780980053377632656584997",  FUNC(cosh) (22), 1792456423.065795780980053377632656584997L, DELTA1159, 0, 0);
  check_float ("cosh_towardzero (23) == 4872401723.124451300068625740569997090344",  FUNC(cosh) (23), 4872401723.124451300068625740569997090344L, DELTA1160, 0, 0);
  check_float ("cosh_towardzero (24) == 13244561064.92173614708845674912733665919",  FUNC(cosh) (24), 13244561064.92173614708845674912733665919L, DELTA1161, 0, 0);
    }

  fesetround (save_round_mode);

  print_max_error ("cosh_towardzero", DELTAcosh_towardzero, 0);
}


static void
cosh_test_downward (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(cosh) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_DOWNWARD))
    {
  check_float ("cosh_downward (22) == 1792456423.065795780980053377632656584997",  FUNC(cosh) (22), 1792456423.065795780980053377632656584997L, DELTA1162, 0, 0);
  check_float ("cosh_downward (23) == 4872401723.124451300068625740569997090344",  FUNC(cosh) (23), 4872401723.124451300068625740569997090344L, DELTA1163, 0, 0);
  check_float ("cosh_downward (24) == 13244561064.92173614708845674912733665919",  FUNC(cosh) (24), 13244561064.92173614708845674912733665919L, DELTA1164, 0, 0);
    }

  fesetround (save_round_mode);

  print_max_error ("cosh_downward", DELTAcosh_downward, 0);
}


static void
cosh_test_upward (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(cosh) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_UPWARD))
    {
  check_float ("cosh_upward (22) == 1792456423.065795780980053377632656584997",  FUNC(cosh) (22), 1792456423.065795780980053377632656584997L, 0, 0, 0);
  check_float ("cosh_upward (23) == 4872401723.124451300068625740569997090344",  FUNC(cosh) (23), 4872401723.124451300068625740569997090344L, DELTA1166, 0, 0);
  check_float ("cosh_upward (24) == 13244561064.92173614708845674912733665919",  FUNC(cosh) (24), 13244561064.92173614708845674912733665919L, 0, 0, 0);
    }

  fesetround (save_round_mode);

  print_max_error ("cosh_upward", DELTAcosh_upward, 0);
}


static void
cpow_test (void)
{
  errno = 0;
  FUNC(cpow) (BUILD_COMPLEX (1, 0), BUILD_COMPLEX (0, 0));
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_complex ("cpow (1 + 0 i, 0 + 0 i) == 1.0 + 0.0 i",  FUNC(cpow) (BUILD_COMPLEX (1, 0), BUILD_COMPLEX (0, 0)), BUILD_COMPLEX (1.0, 0.0), 0, 0, 0);
  check_complex ("cpow (2 + 0 i, 10 + 0 i) == 1024.0 + 0.0 i",  FUNC(cpow) (BUILD_COMPLEX (2, 0), BUILD_COMPLEX (10, 0)), BUILD_COMPLEX (1024.0, 0.0), DELTA1169, 0, 0);

  check_complex ("cpow (e + 0 i, 0 + 2 * M_PIl i) == 1.0 + 0.0 i",  FUNC(cpow) (BUILD_COMPLEX (M_El, 0), BUILD_COMPLEX (0, 2 * M_PIl)), BUILD_COMPLEX (1.0, 0.0), DELTA1170, 0, 0);
  check_complex ("cpow (2 + 3 i, 4 + 0 i) == -119.0 - 120.0 i",  FUNC(cpow) (BUILD_COMPLEX (2, 3), BUILD_COMPLEX (4, 0)), BUILD_COMPLEX (-119.0, -120.0), DELTA1171, 0, 0);

  check_complex ("cpow (NaN + NaN i, NaN + NaN i) == NaN + NaN i",  FUNC(cpow) (BUILD_COMPLEX (nan_value, nan_value), BUILD_COMPLEX (nan_value, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, 0);

  check_complex ("cpow (0.75 + 1.25 i, 0.75 + 1.25 i) == 0.117506293914473555420279832210420483 + 0.346552747708338676483025352060418001 i",  FUNC(cpow) (BUILD_COMPLEX (0.75L, 1.25L), BUILD_COMPLEX (0.75L, 1.25L)), BUILD_COMPLEX (0.117506293914473555420279832210420483L, 0.346552747708338676483025352060418001L), DELTA1173, 0, 0);
  check_complex ("cpow (0.75 + 1.25 i, 1.0 + 1.0 i) == 0.0846958290317209430433805274189191353 + 0.513285749182902449043287190519090481 i",  FUNC(cpow) (BUILD_COMPLEX (0.75L, 1.25L), BUILD_COMPLEX (1.0L, 1.0L)), BUILD_COMPLEX (0.0846958290317209430433805274189191353L, 0.513285749182902449043287190519090481L), DELTA1174, 0, 0);
  check_complex ("cpow (0.75 + 1.25 i, 1.0 + 0.0 i) == 0.75 + 1.25 i",  FUNC(cpow) (BUILD_COMPLEX (0.75L, 1.25L), BUILD_COMPLEX (1.0L, 0.0L)), BUILD_COMPLEX (0.75L, 1.25L), DELTA1175, 0, 0);
  check_complex ("cpow (0.75 + 1.25 i, 0.0 + 1.0 i) == 0.331825439177608832276067945276730566 + 0.131338600281188544930936345230903032 i",  FUNC(cpow) (BUILD_COMPLEX (0.75L, 1.25L), BUILD_COMPLEX (0.0L, 1.0L)), BUILD_COMPLEX (0.331825439177608832276067945276730566L, 0.131338600281188544930936345230903032L), DELTA1176, 0, 0);

  print_complex_max_error ("cpow", DELTAcpow, 0);
}


static void
cproj_test (void)
{
  init_max_error ();
  check_complex ("cproj (0.0 + 0.0 i) == 0.0 + 0.0 i",  FUNC(cproj) (BUILD_COMPLEX (0.0, 0.0)), BUILD_COMPLEX (0.0, 0.0), 0, 0, 0);
  check_complex ("cproj (-0 - 0 i) == -0 - 0 i",  FUNC(cproj) (BUILD_COMPLEX (minus_zero, minus_zero)), BUILD_COMPLEX (minus_zero, minus_zero), 0, 0, 0);
  check_complex ("cproj (0.0 - 0 i) == 0.0 - 0 i",  FUNC(cproj) (BUILD_COMPLEX (0.0, minus_zero)), BUILD_COMPLEX (0.0, minus_zero), 0, 0, 0);
  check_complex ("cproj (-0 + 0.0 i) == -0 + 0.0 i",  FUNC(cproj) (BUILD_COMPLEX (minus_zero, 0.0)), BUILD_COMPLEX (minus_zero, 0.0), 0, 0, 0);

  check_complex ("cproj (NaN + NaN i) == NaN + NaN i",  FUNC(cproj) (BUILD_COMPLEX (nan_value, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, 0);

  check_complex ("cproj (inf + inf i) == inf + 0.0 i",  FUNC(cproj) (BUILD_COMPLEX (plus_infty, plus_infty)), BUILD_COMPLEX (plus_infty, 0.0), 0, 0, 0);
  check_complex ("cproj (inf - inf i) == inf - 0 i",  FUNC(cproj) (BUILD_COMPLEX (plus_infty, minus_infty)), BUILD_COMPLEX (plus_infty, minus_zero), 0, 0, 0);
  check_complex ("cproj (-inf + inf i) == inf + 0.0 i",  FUNC(cproj) (BUILD_COMPLEX (minus_infty, plus_infty)), BUILD_COMPLEX (plus_infty, 0.0), 0, 0, 0);
  check_complex ("cproj (-inf - inf i) == inf - 0 i",  FUNC(cproj) (BUILD_COMPLEX (minus_infty, minus_infty)), BUILD_COMPLEX (plus_infty, minus_zero), 0, 0, 0);

  check_complex ("cproj (1.0 + 0.0 i) == 1.0 + 0.0 i",  FUNC(cproj) (BUILD_COMPLEX (1.0, 0.0)), BUILD_COMPLEX (1.0, 0.0), 0, 0, 0);
  check_complex ("cproj (2.0 + 3.0 i) == 2.0 + 3.0 i",  FUNC(cproj) (BUILD_COMPLEX (2.0, 3.0)), BUILD_COMPLEX (2.0, 3.0), 0, 0, 0);

  print_complex_max_error ("cproj", 0, 0);
}


static void
creal_test (void)
{
  init_max_error ();
  check_float ("creal (0.0 + 1.0 i) == 0.0",  FUNC(creal) (BUILD_COMPLEX (0.0, 1.0)), 0.0, 0, 0, 0);
  check_float ("creal (-0 + 1.0 i) == -0",  FUNC(creal) (BUILD_COMPLEX (minus_zero, 1.0)), minus_zero, 0, 0, 0);
  check_float ("creal (NaN + 1.0 i) == NaN",  FUNC(creal) (BUILD_COMPLEX (nan_value, 1.0)), nan_value, 0, 0, 0);
  check_float ("creal (NaN + NaN i) == NaN",  FUNC(creal) (BUILD_COMPLEX (nan_value, nan_value)), nan_value, 0, 0, 0);
  check_float ("creal (inf + 1.0 i) == inf",  FUNC(creal) (BUILD_COMPLEX (plus_infty, 1.0)), plus_infty, 0, 0, 0);
  check_float ("creal (-inf + 1.0 i) == -inf",  FUNC(creal) (BUILD_COMPLEX (minus_infty, 1.0)), minus_infty, 0, 0, 0);
  check_float ("creal (2.0 + 3.0 i) == 2.0",  FUNC(creal) (BUILD_COMPLEX (2.0, 3.0)), 2.0, 0, 0, 0);

  print_max_error ("creal", 0, 0);
}

static void
csin_test (void)
{
  errno = 0;
  FUNC(csin) (BUILD_COMPLEX (0.7L, 1.2L));
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_complex ("csin (0.0 + 0.0 i) == 0.0 + 0.0 i",  FUNC(csin) (BUILD_COMPLEX (0.0, 0.0)), BUILD_COMPLEX (0.0, 0.0), 0, 0, 0);
  check_complex ("csin (-0 + 0.0 i) == -0 + 0.0 i",  FUNC(csin) (BUILD_COMPLEX (minus_zero, 0.0)), BUILD_COMPLEX (minus_zero, 0.0), 0, 0, 0);
  check_complex ("csin (0.0 - 0 i) == 0 - 0 i",  FUNC(csin) (BUILD_COMPLEX (0.0, minus_zero)), BUILD_COMPLEX (0, minus_zero), 0, 0, 0);
  check_complex ("csin (-0 - 0 i) == -0 - 0 i",  FUNC(csin) (BUILD_COMPLEX (minus_zero, minus_zero)), BUILD_COMPLEX (minus_zero, minus_zero), 0, 0, 0);

  check_complex ("csin (0.0 + inf i) == 0.0 + inf i",  FUNC(csin) (BUILD_COMPLEX (0.0, plus_infty)), BUILD_COMPLEX (0.0, plus_infty), 0, 0, 0);
  check_complex ("csin (-0 + inf i) == -0 + inf i",  FUNC(csin) (BUILD_COMPLEX (minus_zero, plus_infty)), BUILD_COMPLEX (minus_zero, plus_infty), 0, 0, 0);
  check_complex ("csin (0.0 - inf i) == 0.0 - inf i",  FUNC(csin) (BUILD_COMPLEX (0.0, minus_infty)), BUILD_COMPLEX (0.0, minus_infty), 0, 0, 0);
  check_complex ("csin (-0 - inf i) == -0 - inf i",  FUNC(csin) (BUILD_COMPLEX (minus_zero, minus_infty)), BUILD_COMPLEX (minus_zero, minus_infty), 0, 0, 0);

  check_complex ("csin (inf + 0.0 i) == NaN + 0.0 i",  FUNC(csin) (BUILD_COMPLEX (plus_infty, 0.0)), BUILD_COMPLEX (nan_value, 0.0), 0, 0, INVALID_EXCEPTION|IGNORE_ZERO_INF_SIGN);
  check_complex ("csin (-inf + 0.0 i) == NaN + 0.0 i",  FUNC(csin) (BUILD_COMPLEX (minus_infty, 0.0)), BUILD_COMPLEX (nan_value, 0.0), 0, 0, INVALID_EXCEPTION|IGNORE_ZERO_INF_SIGN);
  check_complex ("csin (inf - 0 i) == NaN + 0.0 i",  FUNC(csin) (BUILD_COMPLEX (plus_infty, minus_zero)), BUILD_COMPLEX (nan_value, 0.0), 0, 0, INVALID_EXCEPTION|IGNORE_ZERO_INF_SIGN);
  check_complex ("csin (-inf - 0 i) == NaN + 0.0 i",  FUNC(csin) (BUILD_COMPLEX (minus_infty, minus_zero)), BUILD_COMPLEX (nan_value, 0.0), 0, 0, INVALID_EXCEPTION|IGNORE_ZERO_INF_SIGN);

  check_complex ("csin (inf + inf i) == NaN + inf i",  FUNC(csin) (BUILD_COMPLEX (plus_infty, plus_infty)), BUILD_COMPLEX (nan_value, plus_infty), 0, 0, INVALID_EXCEPTION|IGNORE_ZERO_INF_SIGN);
  check_complex ("csin (-inf + inf i) == NaN + inf i",  FUNC(csin) (BUILD_COMPLEX (minus_infty, plus_infty)), BUILD_COMPLEX (nan_value, plus_infty), 0, 0, INVALID_EXCEPTION|IGNORE_ZERO_INF_SIGN);
  check_complex ("csin (inf - inf i) == NaN + inf i",  FUNC(csin) (BUILD_COMPLEX (plus_infty, minus_infty)), BUILD_COMPLEX (nan_value, plus_infty), 0, 0, INVALID_EXCEPTION|IGNORE_ZERO_INF_SIGN);
  check_complex ("csin (-inf - inf i) == NaN + inf i",  FUNC(csin) (BUILD_COMPLEX (minus_infty, minus_infty)), BUILD_COMPLEX (nan_value, plus_infty), 0, 0, INVALID_EXCEPTION|IGNORE_ZERO_INF_SIGN);

  check_complex ("csin (inf + 6.75 i) == NaN + NaN i",  FUNC(csin) (BUILD_COMPLEX (plus_infty, 6.75)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);
  check_complex ("csin (inf - 6.75 i) == NaN + NaN i",  FUNC(csin) (BUILD_COMPLEX (plus_infty, -6.75)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);
  check_complex ("csin (-inf + 6.75 i) == NaN + NaN i",  FUNC(csin) (BUILD_COMPLEX (minus_infty, 6.75)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);
  check_complex ("csin (-inf - 6.75 i) == NaN + NaN i",  FUNC(csin) (BUILD_COMPLEX (minus_infty, -6.75)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);

  check_complex ("csin (4.625 + inf i) == -inf - inf i",  FUNC(csin) (BUILD_COMPLEX (4.625, plus_infty)), BUILD_COMPLEX (minus_infty, minus_infty), 0, 0, 0);
  check_complex ("csin (4.625 - inf i) == -inf + inf i",  FUNC(csin) (BUILD_COMPLEX (4.625, minus_infty)), BUILD_COMPLEX (minus_infty, plus_infty), 0, 0, 0);
  check_complex ("csin (-4.625 + inf i) == inf - inf i",  FUNC(csin) (BUILD_COMPLEX (-4.625, plus_infty)), BUILD_COMPLEX (plus_infty, minus_infty), 0, 0, 0);
  check_complex ("csin (-4.625 - inf i) == inf + inf i",  FUNC(csin) (BUILD_COMPLEX (-4.625, minus_infty)), BUILD_COMPLEX (plus_infty, plus_infty), 0, 0, 0);

  check_complex ("csin (NaN + 0.0 i) == NaN + 0.0 i",  FUNC(csin) (BUILD_COMPLEX (nan_value, 0.0)), BUILD_COMPLEX (nan_value, 0.0), 0, 0, IGNORE_ZERO_INF_SIGN);
  check_complex ("csin (NaN - 0 i) == NaN + 0.0 i",  FUNC(csin) (BUILD_COMPLEX (nan_value, minus_zero)), BUILD_COMPLEX (nan_value, 0.0), 0, 0, IGNORE_ZERO_INF_SIGN);

  check_complex ("csin (NaN + inf i) == NaN + inf i",  FUNC(csin) (BUILD_COMPLEX (nan_value, plus_infty)), BUILD_COMPLEX (nan_value, plus_infty), 0, 0, IGNORE_ZERO_INF_SIGN);
  check_complex ("csin (NaN - inf i) == NaN + inf i",  FUNC(csin) (BUILD_COMPLEX (nan_value, minus_infty)), BUILD_COMPLEX (nan_value, plus_infty), 0, 0, IGNORE_ZERO_INF_SIGN);

  check_complex ("csin (NaN + 9.0 i) == NaN + NaN i",  FUNC(csin) (BUILD_COMPLEX (nan_value, 9.0)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("csin (NaN - 9.0 i) == NaN + NaN i",  FUNC(csin) (BUILD_COMPLEX (nan_value, -9.0)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);

  check_complex ("csin (0.0 + NaN i) == 0.0 + NaN i",  FUNC(csin) (BUILD_COMPLEX (0.0, nan_value)), BUILD_COMPLEX (0.0, nan_value), 0, 0, 0);
  check_complex ("csin (-0 + NaN i) == -0 + NaN i",  FUNC(csin) (BUILD_COMPLEX (minus_zero, nan_value)), BUILD_COMPLEX (minus_zero, nan_value), 0, 0, 0);

  check_complex ("csin (10.0 + NaN i) == NaN + NaN i",  FUNC(csin) (BUILD_COMPLEX (10.0, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("csin (NaN - 10.0 i) == NaN + NaN i",  FUNC(csin) (BUILD_COMPLEX (nan_value, -10.0)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);

  check_complex ("csin (inf + NaN i) == NaN + NaN i",  FUNC(csin) (BUILD_COMPLEX (plus_infty, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("csin (-inf + NaN i) == NaN + NaN i",  FUNC(csin) (BUILD_COMPLEX (minus_infty, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);

  check_complex ("csin (NaN + NaN i) == NaN + NaN i",  FUNC(csin) (BUILD_COMPLEX (nan_value, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, 0);

  check_complex ("csin (0.75 + 1.25 i) == 1.28722291002649188575873510790565441 + 1.17210635989270256101081285116138863 i",  FUNC(csin) (BUILD_COMPLEX (0.75L, 1.25L)), BUILD_COMPLEX (1.28722291002649188575873510790565441L, 1.17210635989270256101081285116138863L), DELTA1232, 0, 0);
  check_complex ("csin (-2 - 3 i) == -9.15449914691142957346729954460983256 + 4.16890695996656435075481305885375484 i",  FUNC(csin) (BUILD_COMPLEX (-2, -3)), BUILD_COMPLEX (-9.15449914691142957346729954460983256L, 4.16890695996656435075481305885375484L), DELTA1233, 0, 0);

  check_complex ("csin (0.75 + 89.5 i) == 2.522786001038096774676288412995370563339e38 + 2.708024460708609732016532185663087200560e38 i",  FUNC(csin) (BUILD_COMPLEX (0.75, 89.5)), BUILD_COMPLEX (2.522786001038096774676288412995370563339e38L, 2.708024460708609732016532185663087200560e38L), DELTA1234, 0, 0);
  check_complex ("csin (0.75 - 89.5 i) == 2.522786001038096774676288412995370563339e38 - 2.708024460708609732016532185663087200560e38 i",  FUNC(csin) (BUILD_COMPLEX (0.75, -89.5)), BUILD_COMPLEX (2.522786001038096774676288412995370563339e38L, -2.708024460708609732016532185663087200560e38L), DELTA1235, 0, 0);
  check_complex ("csin (-0.75 + 89.5 i) == -2.522786001038096774676288412995370563339e38 + 2.708024460708609732016532185663087200560e38 i",  FUNC(csin) (BUILD_COMPLEX (-0.75, 89.5)), BUILD_COMPLEX (-2.522786001038096774676288412995370563339e38L, 2.708024460708609732016532185663087200560e38L), DELTA1236, 0, 0);
  check_complex ("csin (-0.75 - 89.5 i) == -2.522786001038096774676288412995370563339e38 - 2.708024460708609732016532185663087200560e38 i",  FUNC(csin) (BUILD_COMPLEX (-0.75, -89.5)), BUILD_COMPLEX (-2.522786001038096774676288412995370563339e38L, -2.708024460708609732016532185663087200560e38L), DELTA1237, 0, 0);

#ifndef TEST_FLOAT
  check_complex ("csin (0.75 + 710.5 i) == 1.255317763348154410745082950806112487736e308 + 1.347490911916428129246890157395342279438e308 i",  FUNC(csin) (BUILD_COMPLEX (0.75, 710.5)), BUILD_COMPLEX (1.255317763348154410745082950806112487736e308L, 1.347490911916428129246890157395342279438e308L), DELTA1238, 0, 0);
  check_complex ("csin (0.75 - 710.5 i) == 1.255317763348154410745082950806112487736e308 - 1.347490911916428129246890157395342279438e308 i",  FUNC(csin) (BUILD_COMPLEX (0.75, -710.5)), BUILD_COMPLEX (1.255317763348154410745082950806112487736e308L, -1.347490911916428129246890157395342279438e308L), DELTA1239, 0, 0);
  check_complex ("csin (-0.75 + 710.5 i) == -1.255317763348154410745082950806112487736e308 + 1.347490911916428129246890157395342279438e308 i",  FUNC(csin) (BUILD_COMPLEX (-0.75, 710.5)), BUILD_COMPLEX (-1.255317763348154410745082950806112487736e308L, 1.347490911916428129246890157395342279438e308L), DELTA1240, 0, 0);
  check_complex ("csin (-0.75 - 710.5 i) == -1.255317763348154410745082950806112487736e308 - 1.347490911916428129246890157395342279438e308 i",  FUNC(csin) (BUILD_COMPLEX (-0.75, -710.5)), BUILD_COMPLEX (-1.255317763348154410745082950806112487736e308L, -1.347490911916428129246890157395342279438e308L), DELTA1241, 0, 0);
#endif

#if defined TEST_LDOUBLE && LDBL_MAX_EXP >= 16384
  check_complex ("csin (0.75 + 11357.25 i) == 8.385498349388321535962327491346664141020e4931 + 9.001213196851067077465606717616495588201e4931 i",  FUNC(csin) (BUILD_COMPLEX (0.75, 11357.25)), BUILD_COMPLEX (8.385498349388321535962327491346664141020e4931L, 9.001213196851067077465606717616495588201e4931L), 0, 0, 0);
  check_complex ("csin (0.75 - 11357.25 i) == 8.385498349388321535962327491346664141020e4931 - 9.001213196851067077465606717616495588201e4931 i",  FUNC(csin) (BUILD_COMPLEX (0.75, -11357.25)), BUILD_COMPLEX (8.385498349388321535962327491346664141020e4931L, -9.001213196851067077465606717616495588201e4931L), 0, 0, 0);
  check_complex ("csin (-0.75 + 11357.25 i) == -8.385498349388321535962327491346664141020e4931 + 9.001213196851067077465606717616495588201e4931 i",  FUNC(csin) (BUILD_COMPLEX (-0.75, 11357.25)), BUILD_COMPLEX (-8.385498349388321535962327491346664141020e4931L, 9.001213196851067077465606717616495588201e4931L), 0, 0, 0);
  check_complex ("csin (-0.75 - 11357.25 i) == -8.385498349388321535962327491346664141020e4931 - 9.001213196851067077465606717616495588201e4931 i",  FUNC(csin) (BUILD_COMPLEX (-0.75, -11357.25)), BUILD_COMPLEX (-8.385498349388321535962327491346664141020e4931L, -9.001213196851067077465606717616495588201e4931L), 0, 0, 0);
#endif

#ifdef TEST_FLOAT
  check_complex ("csin (0x1p-149 + 180 i) == 1.043535896672617552965983803453927655332e33 + inf i",  FUNC(csin) (BUILD_COMPLEX (0x1p-149, 180)), BUILD_COMPLEX (1.043535896672617552965983803453927655332e33L, plus_infty), 0, 0, OVERFLOW_EXCEPTION);
#endif

#if defined TEST_DOUBLE || (defined TEST_LDOUBLE && LDBL_MAX_EXP == 1024)
  check_complex ("csin (0x1p-1074 + 1440 i) == 5.981479269486130556466515778180916082415e301 + inf i",  FUNC(csin) (BUILD_COMPLEX (0x1p-1074, 1440)), BUILD_COMPLEX (5.981479269486130556466515778180916082415e301L, plus_infty), DELTA1247, 0, OVERFLOW_EXCEPTION);
#endif

#if defined TEST_LDOUBLE && LDBL_MAX_EXP >= 16384
  check_complex ("csin (0x1p-16434 + 22730 i) == 1.217853148905605987081057582351152052687e4924 + inf i",  FUNC(csin) (BUILD_COMPLEX (0x1p-16434L, 22730)), BUILD_COMPLEX (1.217853148905605987081057582351152052687e4924L, plus_infty), 0, 0, OVERFLOW_EXCEPTION);
#endif

  print_complex_max_error ("csin", DELTAcsin, 0);
}


static void
csinh_test (void)
{
  errno = 0;
  FUNC(csinh) (BUILD_COMPLEX (0.7L, 1.2L));
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_complex ("csinh (0.0 + 0.0 i) == 0.0 + 0.0 i",  FUNC(csinh) (BUILD_COMPLEX (0.0, 0.0)), BUILD_COMPLEX (0.0, 0.0), 0, 0, 0);
  check_complex ("csinh (-0 + 0.0 i) == -0 + 0.0 i",  FUNC(csinh) (BUILD_COMPLEX (minus_zero, 0.0)), BUILD_COMPLEX (minus_zero, 0.0), 0, 0, 0);
  check_complex ("csinh (0.0 - 0 i) == 0.0 - 0 i",  FUNC(csinh) (BUILD_COMPLEX (0.0, minus_zero)), BUILD_COMPLEX (0.0, minus_zero), 0, 0, 0);
  check_complex ("csinh (-0 - 0 i) == -0 - 0 i",  FUNC(csinh) (BUILD_COMPLEX (minus_zero, minus_zero)), BUILD_COMPLEX (minus_zero, minus_zero), 0, 0, 0);

  check_complex ("csinh (0.0 + inf i) == 0.0 + NaN i",  FUNC(csinh) (BUILD_COMPLEX (0.0, plus_infty)), BUILD_COMPLEX (0.0, nan_value), 0, 0, INVALID_EXCEPTION|IGNORE_ZERO_INF_SIGN);
  check_complex ("csinh (-0 + inf i) == 0.0 + NaN i",  FUNC(csinh) (BUILD_COMPLEX (minus_zero, plus_infty)), BUILD_COMPLEX (0.0, nan_value), 0, 0, INVALID_EXCEPTION|IGNORE_ZERO_INF_SIGN);
  check_complex ("csinh (0.0 - inf i) == 0.0 + NaN i",  FUNC(csinh) (BUILD_COMPLEX (0.0, minus_infty)), BUILD_COMPLEX (0.0, nan_value), 0, 0, INVALID_EXCEPTION|IGNORE_ZERO_INF_SIGN);
  check_complex ("csinh (-0 - inf i) == 0.0 + NaN i",  FUNC(csinh) (BUILD_COMPLEX (minus_zero, minus_infty)), BUILD_COMPLEX (0.0, nan_value), 0, 0, INVALID_EXCEPTION|IGNORE_ZERO_INF_SIGN);

  check_complex ("csinh (inf + 0.0 i) == inf + 0.0 i",  FUNC(csinh) (BUILD_COMPLEX (plus_infty, 0.0)), BUILD_COMPLEX (plus_infty, 0.0), 0, 0, 0);
  check_complex ("csinh (-inf + 0.0 i) == -inf + 0.0 i",  FUNC(csinh) (BUILD_COMPLEX (minus_infty, 0.0)), BUILD_COMPLEX (minus_infty, 0.0), 0, 0, 0);
  check_complex ("csinh (inf - 0 i) == inf - 0 i",  FUNC(csinh) (BUILD_COMPLEX (plus_infty, minus_zero)), BUILD_COMPLEX (plus_infty, minus_zero), 0, 0, 0);
  check_complex ("csinh (-inf - 0 i) == -inf - 0 i",  FUNC(csinh) (BUILD_COMPLEX (minus_infty, minus_zero)), BUILD_COMPLEX (minus_infty, minus_zero), 0, 0, 0);

  check_complex ("csinh (inf + inf i) == inf + NaN i",  FUNC(csinh) (BUILD_COMPLEX (plus_infty, plus_infty)), BUILD_COMPLEX (plus_infty, nan_value), 0, 0, INVALID_EXCEPTION|IGNORE_ZERO_INF_SIGN);
  check_complex ("csinh (-inf + inf i) == inf + NaN i",  FUNC(csinh) (BUILD_COMPLEX (minus_infty, plus_infty)), BUILD_COMPLEX (plus_infty, nan_value), 0, 0, INVALID_EXCEPTION|IGNORE_ZERO_INF_SIGN);
  check_complex ("csinh (inf - inf i) == inf + NaN i",  FUNC(csinh) (BUILD_COMPLEX (plus_infty, minus_infty)), BUILD_COMPLEX (plus_infty, nan_value), 0, 0, INVALID_EXCEPTION|IGNORE_ZERO_INF_SIGN);
  check_complex ("csinh (-inf - inf i) == inf + NaN i",  FUNC(csinh) (BUILD_COMPLEX (minus_infty, minus_infty)), BUILD_COMPLEX (plus_infty, nan_value), 0, 0, INVALID_EXCEPTION|IGNORE_ZERO_INF_SIGN);

  check_complex ("csinh (inf + 4.625 i) == -inf - inf i",  FUNC(csinh) (BUILD_COMPLEX (plus_infty, 4.625)), BUILD_COMPLEX (minus_infty, minus_infty), 0, 0, 0);
  check_complex ("csinh (-inf + 4.625 i) == inf - inf i",  FUNC(csinh) (BUILD_COMPLEX (minus_infty, 4.625)), BUILD_COMPLEX (plus_infty, minus_infty), 0, 0, 0);
  check_complex ("csinh (inf - 4.625 i) == -inf + inf i",  FUNC(csinh) (BUILD_COMPLEX (plus_infty, -4.625)), BUILD_COMPLEX (minus_infty, plus_infty), 0, 0, 0);
  check_complex ("csinh (-inf - 4.625 i) == inf + inf i",  FUNC(csinh) (BUILD_COMPLEX (minus_infty, -4.625)), BUILD_COMPLEX (plus_infty, plus_infty), 0, 0, 0);

  check_complex ("csinh (6.75 + inf i) == NaN + NaN i",  FUNC(csinh) (BUILD_COMPLEX (6.75, plus_infty)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);
  check_complex ("csinh (-6.75 + inf i) == NaN + NaN i",  FUNC(csinh) (BUILD_COMPLEX (-6.75, plus_infty)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);
  check_complex ("csinh (6.75 - inf i) == NaN + NaN i",  FUNC(csinh) (BUILD_COMPLEX (6.75, minus_infty)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);
  check_complex ("csinh (-6.75 - inf i) == NaN + NaN i",  FUNC(csinh) (BUILD_COMPLEX (-6.75, minus_infty)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);

  check_complex ("csinh (0.0 + NaN i) == 0.0 + NaN i",  FUNC(csinh) (BUILD_COMPLEX (0.0, nan_value)), BUILD_COMPLEX (0.0, nan_value), 0, 0, IGNORE_ZERO_INF_SIGN);
  check_complex ("csinh (-0 + NaN i) == 0.0 + NaN i",  FUNC(csinh) (BUILD_COMPLEX (minus_zero, nan_value)), BUILD_COMPLEX (0.0, nan_value), 0, 0, IGNORE_ZERO_INF_SIGN);

  check_complex ("csinh (inf + NaN i) == inf + NaN i",  FUNC(csinh) (BUILD_COMPLEX (plus_infty, nan_value)), BUILD_COMPLEX (plus_infty, nan_value), 0, 0, IGNORE_ZERO_INF_SIGN);
  check_complex ("csinh (-inf + NaN i) == inf + NaN i",  FUNC(csinh) (BUILD_COMPLEX (minus_infty, nan_value)), BUILD_COMPLEX (plus_infty, nan_value), 0, 0, IGNORE_ZERO_INF_SIGN);

  check_complex ("csinh (9.0 + NaN i) == NaN + NaN i",  FUNC(csinh) (BUILD_COMPLEX (9.0, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("csinh (-9.0 + NaN i) == NaN + NaN i",  FUNC(csinh) (BUILD_COMPLEX (-9.0, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);

  check_complex ("csinh (NaN + 0.0 i) == NaN + 0.0 i",  FUNC(csinh) (BUILD_COMPLEX (nan_value, 0.0)), BUILD_COMPLEX (nan_value, 0.0), 0, 0, 0);
  check_complex ("csinh (NaN - 0 i) == NaN - 0 i",  FUNC(csinh) (BUILD_COMPLEX (nan_value, minus_zero)), BUILD_COMPLEX (nan_value, minus_zero), 0, 0, 0);

  check_complex ("csinh (NaN + 10.0 i) == NaN + NaN i",  FUNC(csinh) (BUILD_COMPLEX (nan_value, 10.0)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("csinh (NaN - 10.0 i) == NaN + NaN i",  FUNC(csinh) (BUILD_COMPLEX (nan_value, -10.0)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);

  check_complex ("csinh (NaN + inf i) == NaN + NaN i",  FUNC(csinh) (BUILD_COMPLEX (nan_value, plus_infty)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("csinh (NaN - inf i) == NaN + NaN i",  FUNC(csinh) (BUILD_COMPLEX (nan_value, minus_infty)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);

  check_complex ("csinh (NaN + NaN i) == NaN + NaN i",  FUNC(csinh) (BUILD_COMPLEX (nan_value, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, 0);

  check_complex ("csinh (0.75 + 1.25 i) == 0.259294854551162779153349830618433028 + 1.22863452409509552219214606515777594 i",  FUNC(csinh) (BUILD_COMPLEX (0.75L, 1.25L)), BUILD_COMPLEX (0.259294854551162779153349830618433028L, 1.22863452409509552219214606515777594L), DELTA1286, 0, 0);
  check_complex ("csinh (-2 - 3 i) == 3.59056458998577995201256544779481679 - 0.530921086248519805267040090660676560 i",  FUNC(csinh) (BUILD_COMPLEX (-2, -3)), BUILD_COMPLEX (3.59056458998577995201256544779481679L, -0.530921086248519805267040090660676560L), DELTA1287, 0, 0);

  check_complex ("csinh (89.5 + 0.75 i) == 2.708024460708609732016532185663087200560e38 + 2.522786001038096774676288412995370563339e38 i",  FUNC(csinh) (BUILD_COMPLEX (89.5, 0.75)), BUILD_COMPLEX (2.708024460708609732016532185663087200560e38L, 2.522786001038096774676288412995370563339e38L), DELTA1288, 0, 0);
  check_complex ("csinh (-89.5 + 0.75 i) == -2.708024460708609732016532185663087200560e38 + 2.522786001038096774676288412995370563339e38 i",  FUNC(csinh) (BUILD_COMPLEX (-89.5, 0.75)), BUILD_COMPLEX (-2.708024460708609732016532185663087200560e38L, 2.522786001038096774676288412995370563339e38L), DELTA1289, 0, 0);
  check_complex ("csinh (89.5 - 0.75 i) == 2.708024460708609732016532185663087200560e38 - 2.522786001038096774676288412995370563339e38 i",  FUNC(csinh) (BUILD_COMPLEX (89.5, -0.75)), BUILD_COMPLEX (2.708024460708609732016532185663087200560e38L, -2.522786001038096774676288412995370563339e38L), DELTA1290, 0, 0);
  check_complex ("csinh (-89.5 - 0.75 i) == -2.708024460708609732016532185663087200560e38 - 2.522786001038096774676288412995370563339e38 i",  FUNC(csinh) (BUILD_COMPLEX (-89.5, -0.75)), BUILD_COMPLEX (-2.708024460708609732016532185663087200560e38L, -2.522786001038096774676288412995370563339e38L), DELTA1291, 0, 0);

#ifndef TEST_FLOAT
  check_complex ("csinh (710.5 + 0.75 i) == 1.347490911916428129246890157395342279438e308 + 1.255317763348154410745082950806112487736e308 i",  FUNC(csinh) (BUILD_COMPLEX (710.5, 0.75)), BUILD_COMPLEX (1.347490911916428129246890157395342279438e308L, 1.255317763348154410745082950806112487736e308L), DELTA1292, 0, 0);
  check_complex ("csinh (-710.5 + 0.75 i) == -1.347490911916428129246890157395342279438e308 + 1.255317763348154410745082950806112487736e308 i",  FUNC(csinh) (BUILD_COMPLEX (-710.5, 0.75)), BUILD_COMPLEX (-1.347490911916428129246890157395342279438e308L, 1.255317763348154410745082950806112487736e308L), DELTA1293, 0, 0);
  check_complex ("csinh (710.5 - 0.75 i) == 1.347490911916428129246890157395342279438e308 - 1.255317763348154410745082950806112487736e308 i",  FUNC(csinh) (BUILD_COMPLEX (710.5, -0.75)), BUILD_COMPLEX (1.347490911916428129246890157395342279438e308L, -1.255317763348154410745082950806112487736e308L), DELTA1294, 0, 0);
  check_complex ("csinh (-710.5 - 0.75 i) == -1.347490911916428129246890157395342279438e308 - 1.255317763348154410745082950806112487736e308 i",  FUNC(csinh) (BUILD_COMPLEX (-710.5, -0.75)), BUILD_COMPLEX (-1.347490911916428129246890157395342279438e308L, -1.255317763348154410745082950806112487736e308L), DELTA1295, 0, 0);
#endif

#if defined TEST_LDOUBLE && LDBL_MAX_EXP >= 16384
  check_complex ("csinh (11357.25 + 0.75 i) == 9.001213196851067077465606717616495588201e4931 + 8.385498349388321535962327491346664141020e4931 i",  FUNC(csinh) (BUILD_COMPLEX (11357.25, 0.75)), BUILD_COMPLEX (9.001213196851067077465606717616495588201e4931L, 8.385498349388321535962327491346664141020e4931L), 0, 0, 0);
  check_complex ("csinh (-11357.25 + 0.75 i) == -9.001213196851067077465606717616495588201e4931 + 8.385498349388321535962327491346664141020e4931 i",  FUNC(csinh) (BUILD_COMPLEX (-11357.25, 0.75)), BUILD_COMPLEX (-9.001213196851067077465606717616495588201e4931L, 8.385498349388321535962327491346664141020e4931L), 0, 0, 0);
  check_complex ("csinh (11357.25 - 0.75 i) == 9.001213196851067077465606717616495588201e4931 - 8.385498349388321535962327491346664141020e4931 i",  FUNC(csinh) (BUILD_COMPLEX (11357.25, -0.75)), BUILD_COMPLEX (9.001213196851067077465606717616495588201e4931L, -8.385498349388321535962327491346664141020e4931L), 0, 0, 0);
  check_complex ("csinh (-11357.25 - 0.75 i) == -9.001213196851067077465606717616495588201e4931 - 8.385498349388321535962327491346664141020e4931 i",  FUNC(csinh) (BUILD_COMPLEX (-11357.25, -0.75)), BUILD_COMPLEX (-9.001213196851067077465606717616495588201e4931L, -8.385498349388321535962327491346664141020e4931L), 0, 0, 0);
#endif

#ifdef TEST_FLOAT
  check_complex ("csinh (180 + 0x1p-149 i) == inf + 1.043535896672617552965983803453927655332e33 i",  FUNC(csinh) (BUILD_COMPLEX (180, 0x1p-149)), BUILD_COMPLEX (plus_infty, 1.043535896672617552965983803453927655332e33L), 0, 0, OVERFLOW_EXCEPTION);
#endif

#if defined TEST_DOUBLE || (defined TEST_LDOUBLE && LDBL_MAX_EXP == 1024)
  check_complex ("csinh (1440 + 0x1p-1074 i) == inf + 5.981479269486130556466515778180916082415e301 i",  FUNC(csinh) (BUILD_COMPLEX (1440, 0x1p-1074)), BUILD_COMPLEX (plus_infty, 5.981479269486130556466515778180916082415e301L), DELTA1301, 0, OVERFLOW_EXCEPTION);
#endif

#if defined TEST_LDOUBLE && LDBL_MAX_EXP >= 16384
  check_complex ("csinh (22730 + 0x1p-16434 i) == inf + 1.217853148905605987081057582351152052687e4924 i",  FUNC(csinh) (BUILD_COMPLEX (22730, 0x1p-16434L)), BUILD_COMPLEX (plus_infty, 1.217853148905605987081057582351152052687e4924L), 0, 0, OVERFLOW_EXCEPTION);
#endif

  print_complex_max_error ("csinh", DELTAcsinh, 0);
}


static void
csqrt_test (void)
{
  errno = 0;
  FUNC(csqrt) (BUILD_COMPLEX (-1, 0));
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_complex ("csqrt (0 + 0 i) == 0.0 + 0.0 i",  FUNC(csqrt) (BUILD_COMPLEX (0, 0)), BUILD_COMPLEX (0.0, 0.0), 0, 0, 0);
  check_complex ("csqrt (0 - 0 i) == 0 - 0 i",  FUNC(csqrt) (BUILD_COMPLEX (0, minus_zero)), BUILD_COMPLEX (0, minus_zero), 0, 0, 0);
  check_complex ("csqrt (-0 + 0 i) == 0.0 + 0.0 i",  FUNC(csqrt) (BUILD_COMPLEX (minus_zero, 0)), BUILD_COMPLEX (0.0, 0.0), 0, 0, 0);
  check_complex ("csqrt (-0 - 0 i) == 0.0 - 0 i",  FUNC(csqrt) (BUILD_COMPLEX (minus_zero, minus_zero)), BUILD_COMPLEX (0.0, minus_zero), 0, 0, 0);

  check_complex ("csqrt (-inf + 0 i) == 0.0 + inf i",  FUNC(csqrt) (BUILD_COMPLEX (minus_infty, 0)), BUILD_COMPLEX (0.0, plus_infty), 0, 0, 0);
  check_complex ("csqrt (-inf + 6 i) == 0.0 + inf i",  FUNC(csqrt) (BUILD_COMPLEX (minus_infty, 6)), BUILD_COMPLEX (0.0, plus_infty), 0, 0, 0);
  check_complex ("csqrt (-inf - 0 i) == 0.0 - inf i",  FUNC(csqrt) (BUILD_COMPLEX (minus_infty, minus_zero)), BUILD_COMPLEX (0.0, minus_infty), 0, 0, 0);
  check_complex ("csqrt (-inf - 6 i) == 0.0 - inf i",  FUNC(csqrt) (BUILD_COMPLEX (minus_infty, -6)), BUILD_COMPLEX (0.0, minus_infty), 0, 0, 0);

  check_complex ("csqrt (inf + 0 i) == inf + 0.0 i",  FUNC(csqrt) (BUILD_COMPLEX (plus_infty, 0)), BUILD_COMPLEX (plus_infty, 0.0), 0, 0, 0);
  check_complex ("csqrt (inf + 6 i) == inf + 0.0 i",  FUNC(csqrt) (BUILD_COMPLEX (plus_infty, 6)), BUILD_COMPLEX (plus_infty, 0.0), 0, 0, 0);
  check_complex ("csqrt (inf - 0 i) == inf - 0 i",  FUNC(csqrt) (BUILD_COMPLEX (plus_infty, minus_zero)), BUILD_COMPLEX (plus_infty, minus_zero), 0, 0, 0);
  check_complex ("csqrt (inf - 6 i) == inf - 0 i",  FUNC(csqrt) (BUILD_COMPLEX (plus_infty, -6)), BUILD_COMPLEX (plus_infty, minus_zero), 0, 0, 0);

  check_complex ("csqrt (0 + inf i) == inf + inf i",  FUNC(csqrt) (BUILD_COMPLEX (0, plus_infty)), BUILD_COMPLEX (plus_infty, plus_infty), 0, 0, 0);
  check_complex ("csqrt (4 + inf i) == inf + inf i",  FUNC(csqrt) (BUILD_COMPLEX (4, plus_infty)), BUILD_COMPLEX (plus_infty, plus_infty), 0, 0, 0);
  check_complex ("csqrt (inf + inf i) == inf + inf i",  FUNC(csqrt) (BUILD_COMPLEX (plus_infty, plus_infty)), BUILD_COMPLEX (plus_infty, plus_infty), 0, 0, 0);
  check_complex ("csqrt (-0 + inf i) == inf + inf i",  FUNC(csqrt) (BUILD_COMPLEX (minus_zero, plus_infty)), BUILD_COMPLEX (plus_infty, plus_infty), 0, 0, 0);
  check_complex ("csqrt (-4 + inf i) == inf + inf i",  FUNC(csqrt) (BUILD_COMPLEX (-4, plus_infty)), BUILD_COMPLEX (plus_infty, plus_infty), 0, 0, 0);
  check_complex ("csqrt (-inf + inf i) == inf + inf i",  FUNC(csqrt) (BUILD_COMPLEX (minus_infty, plus_infty)), BUILD_COMPLEX (plus_infty, plus_infty), 0, 0, 0);
  check_complex ("csqrt (0 - inf i) == inf - inf i",  FUNC(csqrt) (BUILD_COMPLEX (0, minus_infty)), BUILD_COMPLEX (plus_infty, minus_infty), 0, 0, 0);
  check_complex ("csqrt (4 - inf i) == inf - inf i",  FUNC(csqrt) (BUILD_COMPLEX (4, minus_infty)), BUILD_COMPLEX (plus_infty, minus_infty), 0, 0, 0);
  check_complex ("csqrt (inf - inf i) == inf - inf i",  FUNC(csqrt) (BUILD_COMPLEX (plus_infty, minus_infty)), BUILD_COMPLEX (plus_infty, minus_infty), 0, 0, 0);
  check_complex ("csqrt (-0 - inf i) == inf - inf i",  FUNC(csqrt) (BUILD_COMPLEX (minus_zero, minus_infty)), BUILD_COMPLEX (plus_infty, minus_infty), 0, 0, 0);
  check_complex ("csqrt (-4 - inf i) == inf - inf i",  FUNC(csqrt) (BUILD_COMPLEX (-4, minus_infty)), BUILD_COMPLEX (plus_infty, minus_infty), 0, 0, 0);
  check_complex ("csqrt (-inf - inf i) == inf - inf i",  FUNC(csqrt) (BUILD_COMPLEX (minus_infty, minus_infty)), BUILD_COMPLEX (plus_infty, minus_infty), 0, 0, 0);

  check_complex ("csqrt (-inf + NaN i) == NaN + inf i",  FUNC(csqrt) (BUILD_COMPLEX (minus_infty, nan_value)), BUILD_COMPLEX (nan_value, plus_infty), 0, 0, IGNORE_ZERO_INF_SIGN);

  check_complex ("csqrt (inf + NaN i) == inf + NaN i",  FUNC(csqrt) (BUILD_COMPLEX (plus_infty, nan_value)), BUILD_COMPLEX (plus_infty, nan_value), 0, 0, 0);

  check_complex ("csqrt (0 + NaN i) == NaN + NaN i",  FUNC(csqrt) (BUILD_COMPLEX (0, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("csqrt (1 + NaN i) == NaN + NaN i",  FUNC(csqrt) (BUILD_COMPLEX (1, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("csqrt (-0 + NaN i) == NaN + NaN i",  FUNC(csqrt) (BUILD_COMPLEX (minus_zero, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("csqrt (-1 + NaN i) == NaN + NaN i",  FUNC(csqrt) (BUILD_COMPLEX (-1, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);

  check_complex ("csqrt (NaN + 0 i) == NaN + NaN i",  FUNC(csqrt) (BUILD_COMPLEX (nan_value, 0)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("csqrt (NaN + 8 i) == NaN + NaN i",  FUNC(csqrt) (BUILD_COMPLEX (nan_value, 8)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("csqrt (NaN - 0 i) == NaN + NaN i",  FUNC(csqrt) (BUILD_COMPLEX (nan_value, minus_zero)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("csqrt (NaN - 8 i) == NaN + NaN i",  FUNC(csqrt) (BUILD_COMPLEX (nan_value, -8)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);

  check_complex ("csqrt (NaN + NaN i) == NaN + NaN i",  FUNC(csqrt) (BUILD_COMPLEX (nan_value, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, 0);

  check_complex ("csqrt (16.0 - 30.0 i) == 5.0 - 3.0 i",  FUNC(csqrt) (BUILD_COMPLEX (16.0, -30.0)), BUILD_COMPLEX (5.0, -3.0), 0, 0, 0);
  check_complex ("csqrt (-1 + 0 i) == 0.0 + 1.0 i",  FUNC(csqrt) (BUILD_COMPLEX (-1, 0)), BUILD_COMPLEX (0.0, 1.0), 0, 0, 0);
  check_complex ("csqrt (0 + 2 i) == 1.0 + 1.0 i",  FUNC(csqrt) (BUILD_COMPLEX (0, 2)), BUILD_COMPLEX (1.0, 1.0), 0, 0, 0);
  check_complex ("csqrt (119 + 120 i) == 12.0 + 5.0 i",  FUNC(csqrt) (BUILD_COMPLEX (119, 120)), BUILD_COMPLEX (12.0, 5.0), 0, 0, 0);
  check_complex ("csqrt (0.75 + 1.25 i) == 1.05065169626078392338656675760808326 + 0.594868882070379067881984030639932657 i",  FUNC(csqrt) (BUILD_COMPLEX (0.75L, 1.25L)), BUILD_COMPLEX (1.05065169626078392338656675760808326L, 0.594868882070379067881984030639932657L), 0, 0, 0);
  check_complex ("csqrt (-2 - 3 i) == 0.89597747612983812471573375529004348 - 1.6741492280355400404480393008490519 i",  FUNC(csqrt) (BUILD_COMPLEX (-2, -3)), BUILD_COMPLEX (0.89597747612983812471573375529004348L, -1.6741492280355400404480393008490519L), DELTA1343, 0, 0);
  check_complex ("csqrt (-2 + 3 i) == 0.89597747612983812471573375529004348 + 1.6741492280355400404480393008490519 i",  FUNC(csqrt) (BUILD_COMPLEX (-2, 3)), BUILD_COMPLEX (0.89597747612983812471573375529004348L, 1.6741492280355400404480393008490519L), DELTA1344, 0, 0);
  /* Principal square root should be returned (i.e., non-negative real
     part).  */
  check_complex ("csqrt (0 - 1 i) == M_SQRT_2_2 - M_SQRT_2_2 i",  FUNC(csqrt) (BUILD_COMPLEX (0, -1)), BUILD_COMPLEX (M_SQRT_2_2, -M_SQRT_2_2), 0, 0, 0);

  check_complex ("csqrt (0x1.fffffep+127 + 0x1.fffffep+127 i) == 2.026714405498316804978751017492482558075e+19 + 8.394925938143272988211878516208015586281e+18 i",  FUNC(csqrt) (BUILD_COMPLEX (0x1.fffffep+127L, 0x1.fffffep+127L)), BUILD_COMPLEX (2.026714405498316804978751017492482558075e+19L, 8.394925938143272988211878516208015586281e+18L), 0, 0, 0);
  check_complex ("csqrt (0x1.fffffep+127 + 1.0 i) == 1.844674352395372953599975585936590505260e+19 + 2.710505511993121390769065968615872097053e-20 i",  FUNC(csqrt) (BUILD_COMPLEX (0x1.fffffep+127L, 1.0L)), BUILD_COMPLEX (1.844674352395372953599975585936590505260e+19L, 2.710505511993121390769065968615872097053e-20L), DELTA1347, 0, 0);
  check_complex ("csqrt (0x1p-149 + 0x1p-149 i) == 4.112805464342778798097003462770175200803e-23 + 1.703579802732953750368659735601389709551e-23 i",  FUNC(csqrt) (BUILD_COMPLEX (0x1p-149L, 0x1p-149L)), BUILD_COMPLEX (4.112805464342778798097003462770175200803e-23L, 1.703579802732953750368659735601389709551e-23L), DELTA1348, 0, 0);
  check_complex ("csqrt (0x1p-147 + 0x1p-147 i) == 8.225610928685557596194006925540350401606e-23 + 3.407159605465907500737319471202779419102e-23 i",  FUNC(csqrt) (BUILD_COMPLEX (0x1p-147L, 0x1p-147L)), BUILD_COMPLEX (8.225610928685557596194006925540350401606e-23L, 3.407159605465907500737319471202779419102e-23L), DELTA1349, 0, 0);

  check_complex ("csqrt (+0 + 0x1p-149 i) == 2.646977960169688559588507814623881131411e-23 + 2.646977960169688559588507814623881131411e-23 i",  FUNC(csqrt) (BUILD_COMPLEX (plus_zero, 0x1p-149L)), BUILD_COMPLEX (2.646977960169688559588507814623881131411e-23L, 2.646977960169688559588507814623881131411e-23L), 0, 0, 0);
  check_complex ("csqrt (0x1p-50 + 0x1p-149 i) == 2.980232238769531250000000000000000000000e-8 + 2.350988701644575015937473074444491355637e-38 i",  FUNC(csqrt) (BUILD_COMPLEX (0x1p-50L, 0x1p-149L)), BUILD_COMPLEX (2.980232238769531250000000000000000000000e-8L, 2.350988701644575015937473074444491355637e-38L), 0, 0, 0);
#ifdef TEST_FLOAT
  check_complex ("csqrt (0x1p+127 + 0x1p-149 i) == 1.304381782533278221234957180625250836888e19 + +0 i",  FUNC(csqrt) (BUILD_COMPLEX (0x1p+127L, 0x1p-149L)), BUILD_COMPLEX (1.304381782533278221234957180625250836888e19L, plus_zero), 0, 0, UNDERFLOW_EXCEPTION);
#endif
  check_complex ("csqrt (0x1p-149 + 0x1p+127 i) == 9.223372036854775808000000000000000000000e18 + 9.223372036854775808000000000000000000000e18 i",  FUNC(csqrt) (BUILD_COMPLEX (0x1p-149L, 0x1p+127L)), BUILD_COMPLEX (9.223372036854775808000000000000000000000e18L, 9.223372036854775808000000000000000000000e18L), 0, 0, 0);
  check_complex ("csqrt (0x1.000002p-126 + 0x1.000002p-126 i) == 1.191195773697904627170323731331667740087e-19 + 4.934094449071842328766868579214125217132e-20 i",  FUNC(csqrt) (BUILD_COMPLEX (0x1.000002p-126L, 0x1.000002p-126L)), BUILD_COMPLEX (1.191195773697904627170323731331667740087e-19L, 4.934094449071842328766868579214125217132e-20L), DELTA1354, 0, 0);
  check_complex ("csqrt (-0x1.000002p-126 - 0x1.000002p-126 i) == 4.934094449071842328766868579214125217132e-20 - 1.191195773697904627170323731331667740087e-19 i",  FUNC(csqrt) (BUILD_COMPLEX (-0x1.000002p-126L, -0x1.000002p-126L)), BUILD_COMPLEX (4.934094449071842328766868579214125217132e-20L, -1.191195773697904627170323731331667740087e-19L), DELTA1355, 0, 0);

#ifndef TEST_FLOAT
  check_complex ("csqrt (0x1.fffffffffffffp+1023 + 0x1.fffffffffffffp+1023 i) == 1.473094556905565378990473658199034571917e+154 + 6.101757441282702188537080005372547713595e+153 i",  FUNC(csqrt) (BUILD_COMPLEX (0x1.fffffffffffffp+1023L, 0x1.fffffffffffffp+1023L)), BUILD_COMPLEX (1.473094556905565378990473658199034571917e+154L, 6.101757441282702188537080005372547713595e+153L), DELTA1356, 0, 0);
  check_complex ("csqrt (0x1.fffffffffffffp+1023 + 0x1p+1023 i) == 1.379778091031440685006200821918878702861e+154 + 3.257214233483129514781233066898042490248e+153 i",  FUNC(csqrt) (BUILD_COMPLEX (0x1.fffffffffffffp+1023L, 0x1p+1023L)), BUILD_COMPLEX (1.379778091031440685006200821918878702861e+154L, 3.257214233483129514781233066898042490248e+153L), DELTA1357, 0, 0);
  check_complex ("csqrt (0x1p-1074 + 0x1p-1074 i) == 2.442109726130830256743814843868934877597e-162 + 1.011554969366634726113090867589031782487e-162 i",  FUNC(csqrt) (BUILD_COMPLEX (0x1p-1074L, 0x1p-1074L)), BUILD_COMPLEX (2.442109726130830256743814843868934877597e-162L, 1.011554969366634726113090867589031782487e-162L), DELTA1358, 0, 0);
  check_complex ("csqrt (0x1p-1073 + 0x1p-1073 i) == 3.453664695497464982856905711457966660085e-162 + 1.430554756764195530630723976279903095110e-162 i",  FUNC(csqrt) (BUILD_COMPLEX (0x1p-1073L, 0x1p-1073L)), BUILD_COMPLEX (3.453664695497464982856905711457966660085e-162L, 1.430554756764195530630723976279903095110e-162L), DELTA1359, 0, 0);

  check_complex ("csqrt (+0 + 0x1p-1074 i) == 1.571727784702628688909515672805082228285e-162 + 1.571727784702628688909515672805082228285e-162 i",  FUNC(csqrt) (BUILD_COMPLEX (plus_zero, 0x1p-1074L)), BUILD_COMPLEX (1.571727784702628688909515672805082228285e-162L, 1.571727784702628688909515672805082228285e-162L), 0, 0, 0);
  check_complex ("csqrt (0x1p-500 + 0x1p-1074 i) == 5.527147875260444560247265192192255725514e-76 + 4.469444793151709302716387622440056066334e-249 i",  FUNC(csqrt) (BUILD_COMPLEX (0x1p-500L, 0x1p-1074L)), BUILD_COMPLEX (5.527147875260444560247265192192255725514e-76L, 4.469444793151709302716387622440056066334e-249L), 0, 0, 0);
#if defined TEST_DOUBLE || (defined TEST_LDOUBLE && LDBL_MAX_EXP == 1024)
  check_complex ("csqrt (0x1p+1023 + 0x1p-1074 i) == 9.480751908109176726832526455652159260085e153 + +0 i",  FUNC(csqrt) (BUILD_COMPLEX (0x1p+1023L, 0x1p-1074L)), BUILD_COMPLEX (9.480751908109176726832526455652159260085e153L, plus_zero), 0, 0, UNDERFLOW_EXCEPTION);
#endif
  check_complex ("csqrt (0x1p-1074 + 0x1p+1023 i) == 6.703903964971298549787012499102923063740e153 + 6.703903964971298549787012499102923063740e153 i",  FUNC(csqrt) (BUILD_COMPLEX (0x1p-1074L, 0x1p+1023L)), BUILD_COMPLEX (6.703903964971298549787012499102923063740e153L, 6.703903964971298549787012499102923063740e153L), 0, 0, 0);
  check_complex ("csqrt (0x1.0000000000001p-1022 + 0x1.0000000000001p-1022 i) == 1.638872094839911521020410942677082920935e-154 + 6.788430486774966350907249113759995429568e-155 i",  FUNC(csqrt) (BUILD_COMPLEX (0x1.0000000000001p-1022L, 0x1.0000000000001p-1022L)), BUILD_COMPLEX (1.638872094839911521020410942677082920935e-154L, 6.788430486774966350907249113759995429568e-155L), DELTA1364, 0, 0);
  check_complex ("csqrt (-0x1.0000000000001p-1022 - 0x1.0000000000001p-1022 i) == 6.788430486774966350907249113759995429568e-155 - 1.638872094839911521020410942677082920935e-154 i",  FUNC(csqrt) (BUILD_COMPLEX (-0x1.0000000000001p-1022L, -0x1.0000000000001p-1022L)), BUILD_COMPLEX (6.788430486774966350907249113759995429568e-155L, -1.638872094839911521020410942677082920935e-154L), DELTA1365, 0, 0);
#endif

#if defined TEST_LDOUBLE && LDBL_MAX_EXP >= 16384
  check_complex ("csqrt (0x1.fp+16383 + 0x1.fp+16383 i) == 1.179514222452201722651836720466795901016e+2466 + 4.885707879516577666702435054303191575148e+2465 i",  FUNC(csqrt) (BUILD_COMPLEX (0x1.fp+16383L, 0x1.fp+16383L)), BUILD_COMPLEX (1.179514222452201722651836720466795901016e+2466L, 4.885707879516577666702435054303191575148e+2465L), DELTA1366, 0, 0);
  check_complex ("csqrt (0x1.fp+16383 + 0x1p+16383 i) == 1.106698967236475180613254276996359485630e+2466 + 2.687568007603946993388538156299100955642e+2465 i",  FUNC(csqrt) (BUILD_COMPLEX (0x1.fp+16383L, 0x1p+16383L)), BUILD_COMPLEX (1.106698967236475180613254276996359485630e+2466L, 2.687568007603946993388538156299100955642e+2465L), 0, 0, 0);
  check_complex ("csqrt (0x1p-16440 + 0x1p-16441 i) == 3.514690655930285351254618340783294558136e-2475 + 8.297059146828716918029689466551384219370e-2476 i",  FUNC(csqrt) (BUILD_COMPLEX (0x1p-16440L, 0x1p-16441L)), BUILD_COMPLEX (3.514690655930285351254618340783294558136e-2475L, 8.297059146828716918029689466551384219370e-2476L), 0, 0, 0);

  check_complex ("csqrt (+0 + 0x1p-16445 i) == 4.269191686890197837775136325621239761720e-2476 + 4.269191686890197837775136325621239761720e-2476 i",  FUNC(csqrt) (BUILD_COMPLEX (plus_zero, 0x1p-16445L)), BUILD_COMPLEX (4.269191686890197837775136325621239761720e-2476L, 4.269191686890197837775136325621239761720e-2476L), 0, 0, 0);
  check_complex ("csqrt (0x1p-5000 + 0x1p-16445 i) == 2.660791472672778409283210520357607795518e-753 + 6.849840675828785164910701384823702064234e-4199 i",  FUNC(csqrt) (BUILD_COMPLEX (0x1p-5000L, 0x1p-16445L)), BUILD_COMPLEX (2.660791472672778409283210520357607795518e-753L, 6.849840675828785164910701384823702064234e-4199L), 0, 0, 0);
  check_complex ("csqrt (0x1p+16383 + 0x1p-16445 i) == 7.712754032630730034273323365543179095045e2465 + +0 i",  FUNC(csqrt) (BUILD_COMPLEX (0x1p+16383L, 0x1p-16445L)), BUILD_COMPLEX (7.712754032630730034273323365543179095045e2465L, plus_zero), 0, 0, UNDERFLOW_EXCEPTION);
  check_complex ("csqrt (0x1p-16445 + 0x1p+16383 i) == 5.453740678097079647314921223668914312241e2465 + 5.453740678097079647314921223668914312241e2465 i",  FUNC(csqrt) (BUILD_COMPLEX (0x1p-16445L, 0x1p+16383L)), BUILD_COMPLEX (5.453740678097079647314921223668914312241e2465L, 5.453740678097079647314921223668914312241e2465L), 0, 0, 0);
  check_complex ("csqrt (0x1.0000000000000002p-16382 + 0x1.0000000000000002p-16382 i) == 2.014551439675644900131815801350165472778e-2466 + 8.344545284118961664300307045791497724440e-2467 i",  FUNC(csqrt) (BUILD_COMPLEX (0x1.0000000000000002p-16382L, 0x1.0000000000000002p-16382L)), BUILD_COMPLEX (2.014551439675644900131815801350165472778e-2466L, 8.344545284118961664300307045791497724440e-2467L), DELTA1373, 0, 0);
  check_complex ("csqrt (-0x1.0000000000000002p-16382 - 0x1.0000000000000002p-16382 i) == 8.344545284118961664300307045791497724440e-2467 - 2.014551439675644900131815801350165472778e-2466 i",  FUNC(csqrt) (BUILD_COMPLEX (-0x1.0000000000000002p-16382L, -0x1.0000000000000002p-16382L)), BUILD_COMPLEX (8.344545284118961664300307045791497724440e-2467L, -2.014551439675644900131815801350165472778e-2466L), DELTA1374, 0, 0);

# if LDBL_MANT_DIG >= 113
  check_complex ("csqrt (+0 + 0x1p-16494 i) == 1.799329752913293143453817328207572571442e-2483 + 1.799329752913293143453817328207572571442e-2483 i",  FUNC(csqrt) (BUILD_COMPLEX (plus_zero, 0x1p-16494L)), BUILD_COMPLEX (1.799329752913293143453817328207572571442e-2483L, 1.799329752913293143453817328207572571442e-2483L), 0, 0, 0);
  check_complex ("csqrt (0x1p-5000 + 0x1p-16494 i) == 2.660791472672778409283210520357607795518e-753 + 1.216776133331049643422030716668249905907e-4213 i",  FUNC(csqrt) (BUILD_COMPLEX (0x1p-5000L, 0x1p-16494L)), BUILD_COMPLEX (2.660791472672778409283210520357607795518e-753L, 1.216776133331049643422030716668249905907e-4213L), 0, 0, 0);
  check_complex ("csqrt (0x1p+16383 + 0x1p-16494 i) == 7.712754032630730034273323365543179095045e2465 + +0 i",  FUNC(csqrt) (BUILD_COMPLEX (0x1p+16383L, 0x1p-16494L)), BUILD_COMPLEX (7.712754032630730034273323365543179095045e2465L, plus_zero), 0, 0, UNDERFLOW_EXCEPTION);
  check_complex ("csqrt (0x1p-16494 + 0x1p+16383 i) == 5.453740678097079647314921223668914312241e2465 + 5.453740678097079647314921223668914312241e2465 i",  FUNC(csqrt) (BUILD_COMPLEX (0x1p-16494L, 0x1p+16383L)), BUILD_COMPLEX (5.453740678097079647314921223668914312241e2465L, 5.453740678097079647314921223668914312241e2465L), 0, 0, 0);
  check_complex ("csqrt (0x1.0000000000000000000000000001p-16382 + 0x1.0000000000000000000000000001p-16382 i) == 2.014551439675644900022606748976158925145e-2466 + 8.344545284118961663847948339519226074126e-2467 i",  FUNC(csqrt) (BUILD_COMPLEX (0x1.0000000000000000000000000001p-16382L, 0x1.0000000000000000000000000001p-16382L)), BUILD_COMPLEX (2.014551439675644900022606748976158925145e-2466L, 8.344545284118961663847948339519226074126e-2467L), 0, 0, 0);
  check_complex ("csqrt (-0x1.0000000000000000000000000001p-16382 - 0x1.0000000000000000000000000001p-16382 i) == 8.344545284118961663847948339519226074126e-2467 - 2.014551439675644900022606748976158925145e-2466 i",  FUNC(csqrt) (BUILD_COMPLEX (-0x1.0000000000000000000000000001p-16382L, -0x1.0000000000000000000000000001p-16382L)), BUILD_COMPLEX (8.344545284118961663847948339519226074126e-2467L, -2.014551439675644900022606748976158925145e-2466L), 0, 0, 0);
# endif
#endif

  print_complex_max_error ("csqrt", DELTAcsqrt, 0);
}

static void
ctan_test (void)
{
  errno = 0;
  FUNC(ctan) (BUILD_COMPLEX (0.7L, 1.2L));
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_complex ("ctan (0 + 0 i) == 0.0 + 0.0 i",  FUNC(ctan) (BUILD_COMPLEX (0, 0)), BUILD_COMPLEX (0.0, 0.0), 0, 0, 0);
  check_complex ("ctan (0 - 0 i) == 0.0 - 0 i",  FUNC(ctan) (BUILD_COMPLEX (0, minus_zero)), BUILD_COMPLEX (0.0, minus_zero), 0, 0, 0);
  check_complex ("ctan (-0 + 0 i) == -0 + 0.0 i",  FUNC(ctan) (BUILD_COMPLEX (minus_zero, 0)), BUILD_COMPLEX (minus_zero, 0.0), 0, 0, 0);
  check_complex ("ctan (-0 - 0 i) == -0 - 0 i",  FUNC(ctan) (BUILD_COMPLEX (minus_zero, minus_zero)), BUILD_COMPLEX (minus_zero, minus_zero), 0, 0, 0);

  check_complex ("ctan (0 + inf i) == 0.0 + 1.0 i",  FUNC(ctan) (BUILD_COMPLEX (0, plus_infty)), BUILD_COMPLEX (0.0, 1.0), 0, 0, 0);
  check_complex ("ctan (1 + inf i) == 0.0 + 1.0 i",  FUNC(ctan) (BUILD_COMPLEX (1, plus_infty)), BUILD_COMPLEX (0.0, 1.0), 0, 0, 0);
  check_complex ("ctan (-0 + inf i) == -0 + 1.0 i",  FUNC(ctan) (BUILD_COMPLEX (minus_zero, plus_infty)), BUILD_COMPLEX (minus_zero, 1.0), 0, 0, 0);
  check_complex ("ctan (-1 + inf i) == -0 + 1.0 i",  FUNC(ctan) (BUILD_COMPLEX (-1, plus_infty)), BUILD_COMPLEX (minus_zero, 1.0), 0, 0, 0);

  check_complex ("ctan (0 - inf i) == 0.0 - 1.0 i",  FUNC(ctan) (BUILD_COMPLEX (0, minus_infty)), BUILD_COMPLEX (0.0, -1.0), 0, 0, 0);
  check_complex ("ctan (1 - inf i) == 0.0 - 1.0 i",  FUNC(ctan) (BUILD_COMPLEX (1, minus_infty)), BUILD_COMPLEX (0.0, -1.0), 0, 0, 0);
  check_complex ("ctan (-0 - inf i) == -0 - 1.0 i",  FUNC(ctan) (BUILD_COMPLEX (minus_zero, minus_infty)), BUILD_COMPLEX (minus_zero, -1.0), 0, 0, 0);
  check_complex ("ctan (-1 - inf i) == -0 - 1.0 i",  FUNC(ctan) (BUILD_COMPLEX (-1, minus_infty)), BUILD_COMPLEX (minus_zero, -1.0), 0, 0, 0);

  check_complex ("ctan (inf + 0 i) == NaN + NaN i",  FUNC(ctan) (BUILD_COMPLEX (plus_infty, 0)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);
  check_complex ("ctan (inf + 2 i) == NaN + NaN i",  FUNC(ctan) (BUILD_COMPLEX (plus_infty, 2)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);
  check_complex ("ctan (-inf + 0 i) == NaN + NaN i",  FUNC(ctan) (BUILD_COMPLEX (minus_infty, 0)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);
  check_complex ("ctan (-inf + 2 i) == NaN + NaN i",  FUNC(ctan) (BUILD_COMPLEX (minus_infty, 2)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);
  check_complex ("ctan (inf - 0 i) == NaN + NaN i",  FUNC(ctan) (BUILD_COMPLEX (plus_infty, minus_zero)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);
  check_complex ("ctan (inf - 2 i) == NaN + NaN i",  FUNC(ctan) (BUILD_COMPLEX (plus_infty, -2)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);
  check_complex ("ctan (-inf - 0 i) == NaN + NaN i",  FUNC(ctan) (BUILD_COMPLEX (minus_infty, minus_zero)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);
  check_complex ("ctan (-inf - 2 i) == NaN + NaN i",  FUNC(ctan) (BUILD_COMPLEX (minus_infty, -2)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);

  check_complex ("ctan (NaN + inf i) == 0.0 + 1.0 i",  FUNC(ctan) (BUILD_COMPLEX (nan_value, plus_infty)), BUILD_COMPLEX (0.0, 1.0), 0, 0, IGNORE_ZERO_INF_SIGN);
  check_complex ("ctan (NaN - inf i) == 0.0 - 1.0 i",  FUNC(ctan) (BUILD_COMPLEX (nan_value, minus_infty)), BUILD_COMPLEX (0.0, -1.0), 0, 0, IGNORE_ZERO_INF_SIGN);

  check_complex ("ctan (0 + NaN i) == 0.0 + NaN i",  FUNC(ctan) (BUILD_COMPLEX (0, nan_value)), BUILD_COMPLEX (0.0, nan_value), 0, 0, 0);
  check_complex ("ctan (-0 + NaN i) == -0 + NaN i",  FUNC(ctan) (BUILD_COMPLEX (minus_zero, nan_value)), BUILD_COMPLEX (minus_zero, nan_value), 0, 0, 0);

  check_complex ("ctan (0.5 + NaN i) == NaN + NaN i",  FUNC(ctan) (BUILD_COMPLEX (0.5, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("ctan (-4.5 + NaN i) == NaN + NaN i",  FUNC(ctan) (BUILD_COMPLEX (-4.5, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);

  check_complex ("ctan (NaN + 0 i) == NaN + NaN i",  FUNC(ctan) (BUILD_COMPLEX (nan_value, 0)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("ctan (NaN + 5 i) == NaN + NaN i",  FUNC(ctan) (BUILD_COMPLEX (nan_value, 5)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("ctan (NaN - 0 i) == NaN + NaN i",  FUNC(ctan) (BUILD_COMPLEX (nan_value, minus_zero)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("ctan (NaN - 0.25 i) == NaN + NaN i",  FUNC(ctan) (BUILD_COMPLEX (nan_value, -0.25)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);

  check_complex ("ctan (NaN + NaN i) == NaN + NaN i",  FUNC(ctan) (BUILD_COMPLEX (nan_value, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, 0);

  check_complex ("ctan (0.75 + 1.25 i) == 0.160807785916206426725166058173438663 + 0.975363285031235646193581759755216379 i",  FUNC(ctan) (BUILD_COMPLEX (0.75L, 1.25L)), BUILD_COMPLEX (0.160807785916206426725166058173438663L, 0.975363285031235646193581759755216379L), DELTA1412, 0, 0);
  check_complex ("ctan (-2 - 3 i) == 0.376402564150424829275122113032269084e-2 - 1.00323862735360980144635859782192726 i",  FUNC(ctan) (BUILD_COMPLEX (-2, -3)), BUILD_COMPLEX (0.376402564150424829275122113032269084e-2L, -1.00323862735360980144635859782192726L), DELTA1413, 0, 0);

  check_complex ("ctan (1 + 45 i) == 1.490158918874345552942703234806348520895e-39 + 1.000000000000000000000000000000000000001 i",  FUNC(ctan) (BUILD_COMPLEX (1, 45)), BUILD_COMPLEX (1.490158918874345552942703234806348520895e-39L, 1.000000000000000000000000000000000000001L), DELTA1414, 0, UNDERFLOW_EXCEPTION_FLOAT);
  check_complex ("ctan (1 + 47 i) == 2.729321264492904590777293425576722354636e-41 + 1.0 i",  FUNC(ctan) (BUILD_COMPLEX (1, 47)), BUILD_COMPLEX (2.729321264492904590777293425576722354636e-41L, 1.0), DELTA1415, 0, UNDERFLOW_EXCEPTION_FLOAT);

#ifndef TEST_FLOAT
  check_complex ("ctan (1 + 355 i) == 8.140551093483276762350406321792653551513e-309 + 1.0 i",  FUNC(ctan) (BUILD_COMPLEX (1, 355)), BUILD_COMPLEX (8.140551093483276762350406321792653551513e-309L, 1.0), 0, 0, UNDERFLOW_EXCEPTION_DOUBLE);
  check_complex ("ctan (1 + 365 i) == 1.677892637497921890115075995898773550884e-317 + 1.0 i",  FUNC(ctan) (BUILD_COMPLEX (1, 365)), BUILD_COMPLEX (1.677892637497921890115075995898773550884e-317L, 1.0), 0, 0, UNDERFLOW_EXCEPTION_DOUBLE);
#endif

#if defined TEST_LDOUBLE && LDBL_MAX_EXP >= 16384
  check_complex ("ctan (1 + 5680 i) == 4.725214596136812019616700920476949798307e-4934 + 1.0 i",  FUNC(ctan) (BUILD_COMPLEX (1, 5680)), BUILD_COMPLEX (4.725214596136812019616700920476949798307e-4934L, 1.0), 0, 0, UNDERFLOW_EXCEPTION);
  check_complex ("ctan (1 + 5690 i) == 9.739393181626937151720816611272607059057e-4943 + 1.0 i",  FUNC(ctan) (BUILD_COMPLEX (1, 5690)), BUILD_COMPLEX (9.739393181626937151720816611272607059057e-4943L, 1.0), 0, 0, UNDERFLOW_EXCEPTION);
#endif

  check_complex ("ctan (0x3.243f6cp-1 + 0 i) == -2.287733242885645987394874673945769518150e7 + 0.0 i",  FUNC(ctan) (BUILD_COMPLEX (0x3.243f6cp-1, 0)), BUILD_COMPLEX (-2.287733242885645987394874673945769518150e7L, 0.0), DELTA1420, 0, 0);

  check_complex ("ctan (0x1p127 + 1 i) == 0.2446359391192790896381501310437708987204 + 0.9101334047676183761532873794426475906201 i",  FUNC(ctan) (BUILD_COMPLEX (0x1p127, 1)), BUILD_COMPLEX (0.2446359391192790896381501310437708987204L, 0.9101334047676183761532873794426475906201L), DELTA1421, 0, 0);

#ifndef TEST_FLOAT
  check_complex ("ctan (0x1p1023 + 1 i) == -0.2254627924997545057926782581695274244229 + 0.8786063118883068695462540226219865087189 i",  FUNC(ctan) (BUILD_COMPLEX (0x1p1023, 1)), BUILD_COMPLEX (-0.2254627924997545057926782581695274244229L, 0.8786063118883068695462540226219865087189L), DELTA1422, 0, 0);
#endif

#if defined TEST_LDOUBLE && LDBL_MAX_EXP >= 16384
  check_complex ("ctan (0x1p16383 + 1 i) == 0.1608598776370396607204448234354670036772 + 0.8133818522051542536316746743877629761488 i",  FUNC(ctan) (BUILD_COMPLEX (0x1p16383L, 1)), BUILD_COMPLEX (0.1608598776370396607204448234354670036772L, 0.8133818522051542536316746743877629761488L), 0, 0, 0);
#endif

  check_complex ("ctan (50000 + 50000 i) == +0 + 1.0 i",  FUNC(ctan) (BUILD_COMPLEX (50000, 50000)), BUILD_COMPLEX (plus_zero, 1.0), 0, 0, UNDERFLOW_EXCEPTION);
  check_complex ("ctan (50000 - 50000 i) == +0 - 1.0 i",  FUNC(ctan) (BUILD_COMPLEX (50000, -50000)), BUILD_COMPLEX (plus_zero, -1.0), 0, 0, UNDERFLOW_EXCEPTION);
  check_complex ("ctan (-50000 + 50000 i) == -0 + 1.0 i",  FUNC(ctan) (BUILD_COMPLEX (-50000, 50000)), BUILD_COMPLEX (minus_zero, 1.0), 0, 0, UNDERFLOW_EXCEPTION);
  check_complex ("ctan (-50000 - 50000 i) == -0 - 1.0 i",  FUNC(ctan) (BUILD_COMPLEX (-50000, -50000)), BUILD_COMPLEX (minus_zero, -1.0), 0, 0, UNDERFLOW_EXCEPTION);

  print_complex_max_error ("ctan", DELTActan, 0);
}


static void
ctan_test_tonearest (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(ctan) (BUILD_COMPLEX (0.7L, 1.2L));
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_TONEAREST))
    {
  check_complex ("ctan_tonearest (0x1.921fb6p+0 + 0x1p-149 i) == -2.287733242885645987394874673945769518150e7 + 7.334008549954377778731880988481078535821e-31 i",  FUNC(ctan) (BUILD_COMPLEX (0x1.921fb6p+0, 0x1p-149)), BUILD_COMPLEX (-2.287733242885645987394874673945769518150e7L, 7.334008549954377778731880988481078535821e-31L), DELTA1428, 0, 0);

#ifndef TEST_FLOAT
  check_complex ("ctan_tonearest (0x1.921fb54442d18p+0 + 0x1p-1074 i) == 1.633123935319536975596773704152891653086e16 + 1.317719414943508315995636961402669067843e-291 i",  FUNC(ctan) (BUILD_COMPLEX (0x1.921fb54442d18p+0, 0x1p-1074)), BUILD_COMPLEX (1.633123935319536975596773704152891653086e16L, 1.317719414943508315995636961402669067843e-291L), DELTA1429, 0, 0);
#endif

#if defined TEST_LDOUBLE && LDBL_MIN_EXP <= -16381
  check_complex ("ctan_tonearest (0x1.921fb54442d1846ap+0 + 0x1p-16445 i) == -3.986797629811710706723242948653362815645e19 + 5.793882568875674066286163141055208625180e-4912 i",  FUNC(ctan) (BUILD_COMPLEX (0x1.921fb54442d1846ap+0L, 0x1p-16445L)), BUILD_COMPLEX (-3.986797629811710706723242948653362815645e19L, 5.793882568875674066286163141055208625180e-4912L), DELTA1430, 0, 0);
#endif
    }

  fesetround (save_round_mode);

  print_complex_max_error ("ctan_tonearest", DELTActan_tonearest, 0);
}


static void
ctan_test_towardzero (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(ctan) (BUILD_COMPLEX (0.7L, 1.2L));
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_TOWARDZERO))
    {
  check_complex ("ctan_towardzero (0x1.921fb6p+0 + 0x1p-149 i) == -2.287733242885645987394874673945769518150e7 + 7.334008549954377778731880988481078535821e-31 i",  FUNC(ctan) (BUILD_COMPLEX (0x1.921fb6p+0, 0x1p-149)), BUILD_COMPLEX (-2.287733242885645987394874673945769518150e7L, 7.334008549954377778731880988481078535821e-31L), DELTA1431, 0, 0);

#ifndef TEST_FLOAT
  check_complex ("ctan_towardzero (0x1.921fb54442d18p+0 + 0x1p-1074 i) == 1.633123935319536975596773704152891653086e16 + 1.317719414943508315995636961402669067843e-291 i",  FUNC(ctan) (BUILD_COMPLEX (0x1.921fb54442d18p+0, 0x1p-1074)), BUILD_COMPLEX (1.633123935319536975596773704152891653086e16L, 1.317719414943508315995636961402669067843e-291L), DELTA1432, 0, 0);
#endif

#if defined TEST_LDOUBLE && LDBL_MIN_EXP <= -16381
  check_complex ("ctan_towardzero (0x1.921fb54442d1846ap+0 + 0x1p-16445 i) == -3.986797629811710706723242948653362815645e19 + 5.793882568875674066286163141055208625180e-4912 i",  FUNC(ctan) (BUILD_COMPLEX (0x1.921fb54442d1846ap+0L, 0x1p-16445L)), BUILD_COMPLEX (-3.986797629811710706723242948653362815645e19L, 5.793882568875674066286163141055208625180e-4912L), 0, 0, 0);
#endif
    }

  fesetround (save_round_mode);

  print_complex_max_error ("ctan_towardzero", DELTActan_towardzero, 0);
}


static void
ctan_test_downward (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(ctan) (BUILD_COMPLEX (0.7L, 1.2L));
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_DOWNWARD))
    {
  check_complex ("ctan_downward (0x1.921fb6p+0 + 0x1p-149 i) == -2.287733242885645987394874673945769518150e7 + 7.334008549954377778731880988481078535821e-31 i",  FUNC(ctan) (BUILD_COMPLEX (0x1.921fb6p+0, 0x1p-149)), BUILD_COMPLEX (-2.287733242885645987394874673945769518150e7L, 7.334008549954377778731880988481078535821e-31L), DELTA1434, 0, 0);

#ifndef TEST_FLOAT
  check_complex ("ctan_downward (0x1.921fb54442d18p+0 + 0x1p-1074 i) == 1.633123935319536975596773704152891653086e16 + 1.317719414943508315995636961402669067843e-291 i",  FUNC(ctan) (BUILD_COMPLEX (0x1.921fb54442d18p+0, 0x1p-1074)), BUILD_COMPLEX (1.633123935319536975596773704152891653086e16L, 1.317719414943508315995636961402669067843e-291L), DELTA1435, 0, 0);
#endif

#if defined TEST_LDOUBLE && LDBL_MIN_EXP <= -16381
  check_complex ("ctan_downward (0x1.921fb54442d1846ap+0 + 0x1p-16445 i) == -3.986797629811710706723242948653362815645e19 + 5.793882568875674066286163141055208625180e-4912 i",  FUNC(ctan) (BUILD_COMPLEX (0x1.921fb54442d1846ap+0L, 0x1p-16445L)), BUILD_COMPLEX (-3.986797629811710706723242948653362815645e19L, 5.793882568875674066286163141055208625180e-4912L), DELTA1436, 0, 0);
#endif
    }

  fesetround (save_round_mode);

  print_complex_max_error ("ctan_downward", DELTActan_downward, 0);
}


static void
ctan_test_upward (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(ctan) (BUILD_COMPLEX (0.7L, 1.2L));
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_UPWARD))
    {
  check_complex ("ctan_upward (0x1.921fb6p+0 + 0x1p-149 i) == -2.287733242885645987394874673945769518150e7 + 7.334008549954377778731880988481078535821e-31 i",  FUNC(ctan) (BUILD_COMPLEX (0x1.921fb6p+0, 0x1p-149)), BUILD_COMPLEX (-2.287733242885645987394874673945769518150e7L, 7.334008549954377778731880988481078535821e-31L), DELTA1437, 0, 0);

#ifndef TEST_FLOAT
  check_complex ("ctan_upward (0x1.921fb54442d18p+0 + 0x1p-1074 i) == 1.633123935319536975596773704152891653086e16 + 1.317719414943508315995636961402669067843e-291 i",  FUNC(ctan) (BUILD_COMPLEX (0x1.921fb54442d18p+0, 0x1p-1074)), BUILD_COMPLEX (1.633123935319536975596773704152891653086e16L, 1.317719414943508315995636961402669067843e-291L), DELTA1438, 0, 0);
#endif

#if defined TEST_LDOUBLE && LDBL_MIN_EXP <= -16381
  check_complex ("ctan_upward (0x1.921fb54442d1846ap+0 + 0x1p-16445 i) == -3.986797629811710706723242948653362815645e19 + 5.793882568875674066286163141055208625180e-4912 i",  FUNC(ctan) (BUILD_COMPLEX (0x1.921fb54442d1846ap+0L, 0x1p-16445L)), BUILD_COMPLEX (-3.986797629811710706723242948653362815645e19L, 5.793882568875674066286163141055208625180e-4912L), DELTA1439, 0, 0);
#endif
    }

  fesetround (save_round_mode);

  print_complex_max_error ("ctan_upward", DELTActan_upward, 0);
}


static void
ctanh_test (void)
{
  errno = 0;
  FUNC(ctanh) (BUILD_COMPLEX (0, 0));
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_complex ("ctanh (0 + 0 i) == 0.0 + 0.0 i",  FUNC(ctanh) (BUILD_COMPLEX (0, 0)), BUILD_COMPLEX (0.0, 0.0), 0, 0, 0);
  check_complex ("ctanh (0 - 0 i) == 0.0 - 0 i",  FUNC(ctanh) (BUILD_COMPLEX (0, minus_zero)), BUILD_COMPLEX (0.0, minus_zero), 0, 0, 0);
  check_complex ("ctanh (-0 + 0 i) == -0 + 0.0 i",  FUNC(ctanh) (BUILD_COMPLEX (minus_zero, 0)), BUILD_COMPLEX (minus_zero, 0.0), 0, 0, 0);
  check_complex ("ctanh (-0 - 0 i) == -0 - 0 i",  FUNC(ctanh) (BUILD_COMPLEX (minus_zero, minus_zero)), BUILD_COMPLEX (minus_zero, minus_zero), 0, 0, 0);

  check_complex ("ctanh (inf + 0 i) == 1.0 + 0.0 i",  FUNC(ctanh) (BUILD_COMPLEX (plus_infty, 0)), BUILD_COMPLEX (1.0, 0.0), 0, 0, 0);
  check_complex ("ctanh (inf + 1 i) == 1.0 + 0.0 i",  FUNC(ctanh) (BUILD_COMPLEX (plus_infty, 1)), BUILD_COMPLEX (1.0, 0.0), 0, 0, 0);
  check_complex ("ctanh (inf - 0 i) == 1.0 - 0 i",  FUNC(ctanh) (BUILD_COMPLEX (plus_infty, minus_zero)), BUILD_COMPLEX (1.0, minus_zero), 0, 0, 0);
  check_complex ("ctanh (inf - 1 i) == 1.0 - 0 i",  FUNC(ctanh) (BUILD_COMPLEX (plus_infty, -1)), BUILD_COMPLEX (1.0, minus_zero), 0, 0, 0);
  check_complex ("ctanh (-inf + 0 i) == -1.0 + 0.0 i",  FUNC(ctanh) (BUILD_COMPLEX (minus_infty, 0)), BUILD_COMPLEX (-1.0, 0.0), 0, 0, 0);
  check_complex ("ctanh (-inf + 1 i) == -1.0 + 0.0 i",  FUNC(ctanh) (BUILD_COMPLEX (minus_infty, 1)), BUILD_COMPLEX (-1.0, 0.0), 0, 0, 0);
  check_complex ("ctanh (-inf - 0 i) == -1.0 - 0 i",  FUNC(ctanh) (BUILD_COMPLEX (minus_infty, minus_zero)), BUILD_COMPLEX (-1.0, minus_zero), 0, 0, 0);
  check_complex ("ctanh (-inf - 1 i) == -1.0 - 0 i",  FUNC(ctanh) (BUILD_COMPLEX (minus_infty, -1)), BUILD_COMPLEX (-1.0, minus_zero), 0, 0, 0);

  check_complex ("ctanh (0 + inf i) == NaN + NaN i",  FUNC(ctanh) (BUILD_COMPLEX (0, plus_infty)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);
  check_complex ("ctanh (2 + inf i) == NaN + NaN i",  FUNC(ctanh) (BUILD_COMPLEX (2, plus_infty)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);
  check_complex ("ctanh (0 - inf i) == NaN + NaN i",  FUNC(ctanh) (BUILD_COMPLEX (0, minus_infty)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);
  check_complex ("ctanh (2 - inf i) == NaN + NaN i",  FUNC(ctanh) (BUILD_COMPLEX (2, minus_infty)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);
  check_complex ("ctanh (-0 + inf i) == NaN + NaN i",  FUNC(ctanh) (BUILD_COMPLEX (minus_zero, plus_infty)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);
  check_complex ("ctanh (-2 + inf i) == NaN + NaN i",  FUNC(ctanh) (BUILD_COMPLEX (-2, plus_infty)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);
  check_complex ("ctanh (-0 - inf i) == NaN + NaN i",  FUNC(ctanh) (BUILD_COMPLEX (minus_zero, minus_infty)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);
  check_complex ("ctanh (-2 - inf i) == NaN + NaN i",  FUNC(ctanh) (BUILD_COMPLEX (-2, minus_infty)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION);

  check_complex ("ctanh (inf + NaN i) == 1.0 + 0.0 i",  FUNC(ctanh) (BUILD_COMPLEX (plus_infty, nan_value)), BUILD_COMPLEX (1.0, 0.0), 0, 0, IGNORE_ZERO_INF_SIGN);
  check_complex ("ctanh (-inf + NaN i) == -1.0 + 0.0 i",  FUNC(ctanh) (BUILD_COMPLEX (minus_infty, nan_value)), BUILD_COMPLEX (-1.0, 0.0), 0, 0, IGNORE_ZERO_INF_SIGN);

  check_complex ("ctanh (NaN + 0 i) == NaN + 0.0 i",  FUNC(ctanh) (BUILD_COMPLEX (nan_value, 0)), BUILD_COMPLEX (nan_value, 0.0), 0, 0, 0);
  check_complex ("ctanh (NaN - 0 i) == NaN - 0 i",  FUNC(ctanh) (BUILD_COMPLEX (nan_value, minus_zero)), BUILD_COMPLEX (nan_value, minus_zero), 0, 0, 0);

  check_complex ("ctanh (NaN + 0.5 i) == NaN + NaN i",  FUNC(ctanh) (BUILD_COMPLEX (nan_value, 0.5)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("ctanh (NaN - 4.5 i) == NaN + NaN i",  FUNC(ctanh) (BUILD_COMPLEX (nan_value, -4.5)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);

  check_complex ("ctanh (0 + NaN i) == NaN + NaN i",  FUNC(ctanh) (BUILD_COMPLEX (0, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("ctanh (5 + NaN i) == NaN + NaN i",  FUNC(ctanh) (BUILD_COMPLEX (5, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("ctanh (-0 + NaN i) == NaN + NaN i",  FUNC(ctanh) (BUILD_COMPLEX (minus_zero, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);
  check_complex ("ctanh (-0.25 + NaN i) == NaN + NaN i",  FUNC(ctanh) (BUILD_COMPLEX (-0.25, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, INVALID_EXCEPTION_OK);

  check_complex ("ctanh (NaN + NaN i) == NaN + NaN i",  FUNC(ctanh) (BUILD_COMPLEX (nan_value, nan_value)), BUILD_COMPLEX (nan_value, nan_value), 0, 0, 0);

  check_complex ("ctanh (0 + pi/4 i) == 0.0 + 1.0 i",  FUNC(ctanh) (BUILD_COMPLEX (0, M_PI_4l)), BUILD_COMPLEX (0.0, 1.0), DELTA1471, 0, 0);

  check_complex ("ctanh (0.75 + 1.25 i) == 1.37260757053378320258048606571226857 + 0.385795952609750664177596760720790220 i",  FUNC(ctanh) (BUILD_COMPLEX (0.75L, 1.25L)), BUILD_COMPLEX (1.37260757053378320258048606571226857L, 0.385795952609750664177596760720790220L), DELTA1472, 0, 0);
  check_complex ("ctanh (-2 - 3 i) == -0.965385879022133124278480269394560686 + 0.988437503832249372031403430350121098e-2 i",  FUNC(ctanh) (BUILD_COMPLEX (-2, -3)), BUILD_COMPLEX (-0.965385879022133124278480269394560686L, 0.988437503832249372031403430350121098e-2L), DELTA1473, 0, 0);

  check_complex ("ctanh (45 + 1 i) == 1.000000000000000000000000000000000000001 + 1.490158918874345552942703234806348520895e-39 i",  FUNC(ctanh) (BUILD_COMPLEX (45, 1)), BUILD_COMPLEX (1.000000000000000000000000000000000000001L, 1.490158918874345552942703234806348520895e-39L), DELTA1474, 0, UNDERFLOW_EXCEPTION_FLOAT);
  check_complex ("ctanh (47 + 1 i) == 1.0 + 2.729321264492904590777293425576722354636e-41 i",  FUNC(ctanh) (BUILD_COMPLEX (47, 1)), BUILD_COMPLEX (1.0, 2.729321264492904590777293425576722354636e-41L), DELTA1475, 0, UNDERFLOW_EXCEPTION_FLOAT);

#ifndef TEST_FLOAT
  check_complex ("ctanh (355 + 1 i) == 1.0 + 8.140551093483276762350406321792653551513e-309 i",  FUNC(ctanh) (BUILD_COMPLEX (355, 1)), BUILD_COMPLEX (1.0, 8.140551093483276762350406321792653551513e-309L), 0, 0, UNDERFLOW_EXCEPTION_DOUBLE);
  check_complex ("ctanh (365 + 1 i) == 1.0 + 1.677892637497921890115075995898773550884e-317 i",  FUNC(ctanh) (BUILD_COMPLEX (365, 1)), BUILD_COMPLEX (1.0, 1.677892637497921890115075995898773550884e-317L), 0, 0, UNDERFLOW_EXCEPTION_DOUBLE);
#endif

#if defined TEST_LDOUBLE && LDBL_MAX_EXP >= 16384
  check_complex ("ctanh (5680 + 1 i) == 1.0 + 4.725214596136812019616700920476949798307e-4934 i",  FUNC(ctanh) (BUILD_COMPLEX (5680, 1)), BUILD_COMPLEX (1.0, 4.725214596136812019616700920476949798307e-4934L), 0, 0, UNDERFLOW_EXCEPTION);
  check_complex ("ctanh (5690 + 1 i) == 1.0 + 9.739393181626937151720816611272607059057e-4943 i",  FUNC(ctanh) (BUILD_COMPLEX (5690, 1)), BUILD_COMPLEX (1.0, 9.739393181626937151720816611272607059057e-4943L), 0, 0, UNDERFLOW_EXCEPTION);
#endif

  check_complex ("ctanh (0 + 0x3.243f6cp-1 i) == 0.0 - 2.287733242885645987394874673945769518150e7 i",  FUNC(ctanh) (BUILD_COMPLEX (0, 0x3.243f6cp-1)), BUILD_COMPLEX (0.0, -2.287733242885645987394874673945769518150e7L), DELTA1480, 0, 0);

  check_complex ("ctanh (1 + 0x1p127 i) == 0.9101334047676183761532873794426475906201 + 0.2446359391192790896381501310437708987204 i",  FUNC(ctanh) (BUILD_COMPLEX (1, 0x1p127)), BUILD_COMPLEX (0.9101334047676183761532873794426475906201L, 0.2446359391192790896381501310437708987204L), DELTA1481, 0, 0);

#ifndef TEST_FLOAT
  check_complex ("ctanh (1 + 0x1p1023 i) == 0.8786063118883068695462540226219865087189 - 0.2254627924997545057926782581695274244229 i",  FUNC(ctanh) (BUILD_COMPLEX (1, 0x1p1023)), BUILD_COMPLEX (0.8786063118883068695462540226219865087189L, -0.2254627924997545057926782581695274244229L), DELTA1482, 0, 0);
#endif

#if defined TEST_LDOUBLE && LDBL_MAX_EXP >= 16384
  check_complex ("ctanh (1 + 0x1p16383 i) == 0.8133818522051542536316746743877629761488 + 0.1608598776370396607204448234354670036772 i",  FUNC(ctanh) (BUILD_COMPLEX (1, 0x1p16383L)), BUILD_COMPLEX (0.8133818522051542536316746743877629761488L, 0.1608598776370396607204448234354670036772L), 0, 0, 0);
#endif

  check_complex ("ctanh (50000 + 50000 i) == 1.0 + +0 i",  FUNC(ctanh) (BUILD_COMPLEX (50000, 50000)), BUILD_COMPLEX (1.0, plus_zero), 0, 0, UNDERFLOW_EXCEPTION);
  check_complex ("ctanh (50000 - 50000 i) == 1.0 - 0 i",  FUNC(ctanh) (BUILD_COMPLEX (50000, -50000)), BUILD_COMPLEX (1.0, minus_zero), 0, 0, UNDERFLOW_EXCEPTION);
  check_complex ("ctanh (-50000 + 50000 i) == -1.0 + +0 i",  FUNC(ctanh) (BUILD_COMPLEX (-50000, 50000)), BUILD_COMPLEX (-1.0, plus_zero), 0, 0, UNDERFLOW_EXCEPTION);
  check_complex ("ctanh (-50000 - 50000 i) == -1.0 - 0 i",  FUNC(ctanh) (BUILD_COMPLEX (-50000, -50000)), BUILD_COMPLEX (-1.0, minus_zero), 0, 0, UNDERFLOW_EXCEPTION);

  print_complex_max_error ("ctanh", DELTActanh, 0);
}


static void
ctanh_test_tonearest (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(ctanh) (BUILD_COMPLEX (0.7L, 1.2L));
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_TONEAREST))
    {
  check_complex ("ctanh_tonearest (0x1p-149 + 0x1.921fb6p+0 i) == 7.334008549954377778731880988481078535821e-31 - 2.287733242885645987394874673945769518150e7 i",  FUNC(ctanh) (BUILD_COMPLEX (0x1p-149, 0x1.921fb6p+0)), BUILD_COMPLEX (7.334008549954377778731880988481078535821e-31L, -2.287733242885645987394874673945769518150e7L), DELTA1488, 0, 0);

#ifndef TEST_FLOAT
  check_complex ("ctanh_tonearest (0x1p-1074 + 0x1.921fb54442d18p+0 i) == 1.317719414943508315995636961402669067843e-291 + 1.633123935319536975596773704152891653086e16 i",  FUNC(ctanh) (BUILD_COMPLEX (0x1p-1074, 0x1.921fb54442d18p+0)), BUILD_COMPLEX (1.317719414943508315995636961402669067843e-291L, 1.633123935319536975596773704152891653086e16L), DELTA1489, 0, 0);
#endif

#if defined TEST_LDOUBLE && LDBL_MIN_EXP <= -16381
  check_complex ("ctanh_tonearest (0x1p-16445 + 0x1.921fb54442d1846ap+0 i) == 5.793882568875674066286163141055208625180e-4912 - 3.986797629811710706723242948653362815645e19 i",  FUNC(ctanh) (BUILD_COMPLEX (0x1p-16445L, 0x1.921fb54442d1846ap+0L)), BUILD_COMPLEX (5.793882568875674066286163141055208625180e-4912L, -3.986797629811710706723242948653362815645e19L), DELTA1490, 0, 0);
#endif
    }

  fesetround (save_round_mode);

  print_complex_max_error ("ctanh_tonearest", DELTActanh_tonearest, 0);
}


static void
ctanh_test_towardzero (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(ctanh) (BUILD_COMPLEX (0.7L, 1.2L));
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_TOWARDZERO))
    {
  check_complex ("ctanh_towardzero (0x1p-149 + 0x1.921fb6p+0 i) == 7.334008549954377778731880988481078535821e-31 - 2.287733242885645987394874673945769518150e7 i",  FUNC(ctanh) (BUILD_COMPLEX (0x1p-149, 0x1.921fb6p+0)), BUILD_COMPLEX (7.334008549954377778731880988481078535821e-31L, -2.287733242885645987394874673945769518150e7L), DELTA1491, 0, 0);

#ifndef TEST_FLOAT
  check_complex ("ctanh_towardzero (0x1p-1074 + 0x1.921fb54442d18p+0 i) == 1.317719414943508315995636961402669067843e-291 + 1.633123935319536975596773704152891653086e16 i",  FUNC(ctanh) (BUILD_COMPLEX (0x1p-1074, 0x1.921fb54442d18p+0)), BUILD_COMPLEX (1.317719414943508315995636961402669067843e-291L, 1.633123935319536975596773704152891653086e16L), DELTA1492, 0, 0);
#endif

#if defined TEST_LDOUBLE && LDBL_MIN_EXP <= -16381
  check_complex ("ctanh_towardzero (0x1p-16445 + 0x1.921fb54442d1846ap+0 i) == 5.793882568875674066286163141055208625180e-4912 - 3.986797629811710706723242948653362815645e19 i",  FUNC(ctanh) (BUILD_COMPLEX (0x1p-16445L, 0x1.921fb54442d1846ap+0L)), BUILD_COMPLEX (5.793882568875674066286163141055208625180e-4912L, -3.986797629811710706723242948653362815645e19L), 0, 0, 0);
#endif
    }

  fesetround (save_round_mode);

  print_complex_max_error ("ctanh_towardzero", DELTActanh_towardzero, 0);
}


static void
ctanh_test_downward (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(ctanh) (BUILD_COMPLEX (0.7L, 1.2L));
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_DOWNWARD))
    {
  check_complex ("ctanh_downward (0x1p-149 + 0x1.921fb6p+0 i) == 7.334008549954377778731880988481078535821e-31 - 2.287733242885645987394874673945769518150e7 i",  FUNC(ctanh) (BUILD_COMPLEX (0x1p-149, 0x1.921fb6p+0)), BUILD_COMPLEX (7.334008549954377778731880988481078535821e-31L, -2.287733242885645987394874673945769518150e7L), DELTA1494, 0, 0);

#ifndef TEST_FLOAT
  check_complex ("ctanh_downward (0x1p-1074 + 0x1.921fb54442d18p+0 i) == 1.317719414943508315995636961402669067843e-291 + 1.633123935319536975596773704152891653086e16 i",  FUNC(ctanh) (BUILD_COMPLEX (0x1p-1074, 0x1.921fb54442d18p+0)), BUILD_COMPLEX (1.317719414943508315995636961402669067843e-291L, 1.633123935319536975596773704152891653086e16L), DELTA1495, 0, 0);
#endif

#if defined TEST_LDOUBLE && LDBL_MIN_EXP <= -16381
  check_complex ("ctanh_downward (0x1p-16445 + 0x1.921fb54442d1846ap+0 i) == 5.793882568875674066286163141055208625180e-4912 - 3.986797629811710706723242948653362815645e19 i",  FUNC(ctanh) (BUILD_COMPLEX (0x1p-16445L, 0x1.921fb54442d1846ap+0L)), BUILD_COMPLEX (5.793882568875674066286163141055208625180e-4912L, -3.986797629811710706723242948653362815645e19L), DELTA1496, 0, 0);
#endif
    }

  fesetround (save_round_mode);

  print_complex_max_error ("ctanh_downward", DELTActanh_downward, 0);
}


static void
ctanh_test_upward (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(ctanh) (BUILD_COMPLEX (0.7L, 1.2L));
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_UPWARD))
    {
  check_complex ("ctanh_upward (0x1p-149 + 0x1.921fb6p+0 i) == 7.334008549954377778731880988481078535821e-31 - 2.287733242885645987394874673945769518150e7 i",  FUNC(ctanh) (BUILD_COMPLEX (0x1p-149, 0x1.921fb6p+0)), BUILD_COMPLEX (7.334008549954377778731880988481078535821e-31L, -2.287733242885645987394874673945769518150e7L), DELTA1497, 0, 0);

#ifndef TEST_FLOAT
  check_complex ("ctanh_upward (0x1p-1074 + 0x1.921fb54442d18p+0 i) == 1.317719414943508315995636961402669067843e-291 + 1.633123935319536975596773704152891653086e16 i",  FUNC(ctanh) (BUILD_COMPLEX (0x1p-1074, 0x1.921fb54442d18p+0)), BUILD_COMPLEX (1.317719414943508315995636961402669067843e-291L, 1.633123935319536975596773704152891653086e16L), DELTA1498, 0, 0);
#endif

#if defined TEST_LDOUBLE && LDBL_MIN_EXP <= -16381
  check_complex ("ctanh_upward (0x1p-16445 + 0x1.921fb54442d1846ap+0 i) == 5.793882568875674066286163141055208625180e-4912 - 3.986797629811710706723242948653362815645e19 i",  FUNC(ctanh) (BUILD_COMPLEX (0x1p-16445L, 0x1.921fb54442d1846ap+0L)), BUILD_COMPLEX (5.793882568875674066286163141055208625180e-4912L, -3.986797629811710706723242948653362815645e19L), DELTA1499, 0, 0);
#endif
    }

  fesetround (save_round_mode);

  print_complex_max_error ("ctanh_upward", DELTActanh_upward, 0);
}


static void
erf_test (void)
{
  errno = 0;
  FUNC(erf) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_float ("erf (0) == 0",  FUNC(erf) (0), 0, 0, 0, 0);
  check_float ("erf (-0) == -0",  FUNC(erf) (minus_zero), minus_zero, 0, 0, 0);
  check_float ("erf (inf) == 1",  FUNC(erf) (plus_infty), 1, 0, 0, 0);
  check_float ("erf (-inf) == -1",  FUNC(erf) (minus_infty), -1, 0, 0, 0);
  check_float ("erf (NaN) == NaN",  FUNC(erf) (nan_value), nan_value, 0, 0, 0);

  check_float ("erf (0.125) == 0.140316204801333817393029446521623398",  FUNC(erf) (0.125L), 0.140316204801333817393029446521623398L, 0, 0, 0);
  check_float ("erf (0.75) == 0.711155633653515131598937834591410777",  FUNC(erf) (0.75L), 0.711155633653515131598937834591410777L, 0, 0, 0);
  check_float ("erf (1.25) == 0.922900128256458230136523481197281140",  FUNC(erf) (1.25L), 0.922900128256458230136523481197281140L, DELTA1507, 0, 0);
  check_float ("erf (2.0) == 0.995322265018952734162069256367252929",  FUNC(erf) (2.0L), 0.995322265018952734162069256367252929L, 0, 0, 0);
  check_float ("erf (4.125) == 0.999999994576599200434933994687765914",  FUNC(erf) (4.125L), 0.999999994576599200434933994687765914L, 0, 0, 0);
  check_float ("erf (27.0) == 1.0",  FUNC(erf) (27.0L), 1.0L, 0, 0, 0);

  print_max_error ("erf", DELTAerf, 0);
}


static void
erfc_test (void)
{
  errno = 0;
  FUNC(erfc) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_float ("erfc (inf) == 0.0",  FUNC(erfc) (plus_infty), 0.0, 0, 0, 0);
  check_float ("erfc (-inf) == 2.0",  FUNC(erfc) (minus_infty), 2.0, 0, 0, 0);
  check_float ("erfc (0.0) == 1.0",  FUNC(erfc) (0.0), 1.0, 0, 0, 0);
  check_float ("erfc (-0) == 1.0",  FUNC(erfc) (minus_zero), 1.0, 0, 0, 0);
  check_float ("erfc (NaN) == NaN",  FUNC(erfc) (nan_value), nan_value, 0, 0, 0);

  check_float ("erfc (0.125) == 0.859683795198666182606970553478376602",  FUNC(erfc) (0.125L), 0.859683795198666182606970553478376602L, 0, 0, 0);
  check_float ("erfc (0.75) == 0.288844366346484868401062165408589223",  FUNC(erfc) (0.75L), 0.288844366346484868401062165408589223L, 0, 0, 0);
  check_float ("erfc (1.25) == 0.0770998717435417698634765188027188596",  FUNC(erfc) (1.25L), 0.0770998717435417698634765188027188596L, DELTA1518, 0, 0);
  check_float ("erfc (2.0) == 0.00467773498104726583793074363274707139",  FUNC(erfc) (2.0L), 0.00467773498104726583793074363274707139L, DELTA1519, 0, 0);
  check_float ("erfc (0x1.f7303cp+1) == 2.705500297238986897105236321218861842255e-8",  FUNC(erfc) (0x1.f7303cp+1L), 2.705500297238986897105236321218861842255e-8L, DELTA1520, 0, 0);
  check_float ("erfc (4.125) == 0.542340079956506600531223408575531062e-8",  FUNC(erfc) (4.125L), 0.542340079956506600531223408575531062e-8L, DELTA1521, 0, 0);
  check_float ("erfc (0x1.ffa002p+2) == 1.233585992097580296336099501489175967033e-29",  FUNC(erfc) (0x1.ffa002p+2L), 1.233585992097580296336099501489175967033e-29L, DELTA1522, 0, 0);
  check_float ("erfc (0x1.ffffc8p+2) == 1.122671365033056305522366683719541099329e-29",  FUNC(erfc) (0x1.ffffc8p+2L), 1.122671365033056305522366683719541099329e-29L, 0, 0, 0);
#ifdef TEST_LDOUBLE
  /* The result can only be represented in long double.  */
# if LDBL_MIN_10_EXP < -319
  check_float ("erfc (27.0) == 0.523704892378925568501606768284954709e-318",  FUNC(erfc) (27.0L), 0.523704892378925568501606768284954709e-318L, 0, 0, 0);
# endif
# if LDBL_MANT_DIG >= 106
  check_float ("erfc (0x1.ffff56789abcdef0123456789a8p+2) == 1.123161416304655390092138725253789378459e-29",  FUNC(erfc) (0x1.ffff56789abcdef0123456789a8p+2L), 1.123161416304655390092138725253789378459e-29L, 0, 0, 0);
# endif
#endif

  print_max_error ("erfc", DELTAerfc, 0);
}


static void
exp_test (void)
{
  errno = 0;
  FUNC(exp) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_float ("exp (0) == 1",  FUNC(exp) (0), 1, 0, 0, 0);
  check_float ("exp (-0) == 1",  FUNC(exp) (minus_zero), 1, 0, 0, 0);

#ifndef TEST_INLINE
  check_float ("exp (inf) == inf",  FUNC(exp) (plus_infty), plus_infty, 0, 0, 0);
  check_float ("exp (-inf) == 0",  FUNC(exp) (minus_infty), 0, 0, 0, 0);
#endif
  check_float ("exp (NaN) == NaN",  FUNC(exp) (nan_value), nan_value, 0, 0, 0);
  check_float ("exp (1) == e",  FUNC(exp) (1), M_El, 0, 0, 0);

  check_float ("exp (2) == e^2",  FUNC(exp) (2), M_E2l, 0, 0, 0);
  check_float ("exp (3) == e^3",  FUNC(exp) (3), M_E3l, 0, 0, 0);
  check_float ("exp (0.75) == 2.11700001661267466854536981983709561",  FUNC(exp) (0.75L), 2.11700001661267466854536981983709561L, 0, 0, 0);
  check_float ("exp (50.0) == 5184705528587072464087.45332293348538",  FUNC(exp) (50.0L), 5184705528587072464087.45332293348538L, 0, 0, 0);
  check_float ("exp (88.72269439697265625) == 3.40233126623160774937554134772290447915e38",  FUNC(exp) (88.72269439697265625L), 3.40233126623160774937554134772290447915e38L, 0, 0, 0);
#if defined TEST_LDOUBLE && __LDBL_MAX_EXP__ > 1024
  /* The result can only be represented in sane long double.  */
  check_float ("exp (1000.0) == 0.197007111401704699388887935224332313e435",  FUNC(exp) (1000.0L), 0.197007111401704699388887935224332313e435L, 0, 0, 0);
#endif

#if !(defined TEST_LDOUBLE && LDBL_MAX_EXP > 1024)
  check_float ("exp (710) == inf",  FUNC(exp) (710), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("exp (-1234) == +0",  FUNC(exp) (-1234), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
#endif
  check_float ("exp (1e5) == inf",  FUNC(exp) (1e5), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("exp (max_value) == inf",  FUNC(exp) (max_value), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("exp (-max_value) == 0",  FUNC(exp) (-max_value), 0, 0, 0, UNDERFLOW_EXCEPTION);

  print_max_error ("exp", 0, 0);
}


static void
exp_test_tonearest (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(exp) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_TONEAREST))
    {
  check_float ("exp_tonearest (1) == e",  FUNC(exp) (1), M_El, 0, 0, 0);
  check_float ("exp_tonearest (2) == e^2",  FUNC(exp) (2), M_E2l, 0, 0, 0);
  check_float ("exp_tonearest (3) == e^3",  FUNC(exp) (3), M_E3l, 0, 0, 0);
    }

  fesetround (save_round_mode);

  print_max_error ("exp_tonearest", 0, 0);
}


static void
exp_test_towardzero (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(exp) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_TOWARDZERO))
    {
  check_float ("exp_towardzero (1) == e",  FUNC(exp) (1), M_El, DELTA1546, 0, 0);
  check_float ("exp_towardzero (2) == e^2",  FUNC(exp) (2), M_E2l, DELTA1547, 0, 0);
  check_float ("exp_towardzero (3) == e^3",  FUNC(exp) (3), M_E3l, DELTA1548, 0, 0);
    }

  fesetround (save_round_mode);

  print_max_error ("exp_towardzero", DELTAexp_towardzero, 0);
}


static void
exp_test_downward (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(exp) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_DOWNWARD))
    {
  check_float ("exp_downward (1) == e",  FUNC(exp) (1), M_El, DELTA1549, 0, 0);
  check_float ("exp_downward (2) == e^2",  FUNC(exp) (2), M_E2l, DELTA1550, 0, 0);
  check_float ("exp_downward (3) == e^3",  FUNC(exp) (3), M_E3l, DELTA1551, 0, 0);
    }

  fesetround (save_round_mode);

  print_max_error ("exp_downward", DELTAexp_downward, 0);
}


static void
exp_test_upward (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(exp) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_UPWARD))
    {
  check_float ("exp_upward (1) == e",  FUNC(exp) (1), M_El, DELTA1552, 0, 0);
  check_float ("exp_upward (2) == e^2",  FUNC(exp) (2), M_E2l, 0, 0, 0);
  check_float ("exp_upward (3) == e^3",  FUNC(exp) (3), M_E3l, 0, 0, 0);
    }

  fesetround (save_round_mode);

  print_max_error ("exp_upward", DELTAexp_upward, 0);
}


static void
exp10_test (void)
{
  errno = 0;
  FUNC(exp10) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_float ("exp10 (0) == 1",  FUNC(exp10) (0), 1, 0, 0, 0);
  check_float ("exp10 (-0) == 1",  FUNC(exp10) (minus_zero), 1, 0, 0, 0);

  check_float ("exp10 (inf) == inf",  FUNC(exp10) (plus_infty), plus_infty, 0, 0, 0);
  check_float ("exp10 (-inf) == 0",  FUNC(exp10) (minus_infty), 0, 0, 0, 0);
  check_float ("exp10 (NaN) == NaN",  FUNC(exp10) (nan_value), nan_value, 0, 0, 0);
  check_float ("exp10 (3) == 1000",  FUNC(exp10) (3), 1000, DELTA1560, 0, 0);
  check_float ("exp10 (-1) == 0.1",  FUNC(exp10) (-1), 0.1L, DELTA1561, 0, 0);
  check_float ("exp10 (36) == 1.0e36",  FUNC(exp10) (36), 1.0e36L, DELTA1562, 0, 0);
  check_float ("exp10 (-36) == 1.0e-36",  FUNC(exp10) (-36), 1.0e-36L, DELTA1563, 0, 0);
#ifndef TEST_FLOAT
  check_float ("exp10 (305) == 1.0e305",  FUNC(exp10) (305), 1.0e305L, 0, 0, 0);
  check_float ("exp10 (-305) == 1.0e-305",  FUNC(exp10) (-305), 1.0e-305L, DELTA1565, 0, UNDERFLOW_EXCEPTION_LDOUBLE_IBM);
#endif
#if defined TEST_LDOUBLE && LDBL_MAX_10_EXP >= 4932
  check_float ("exp10 (4932) == 1.0e4932",  FUNC(exp10) (4932), 1.0e4932L, 0, 0, 0);
  check_float ("exp10 (-4932) == 1.0e-4932",  FUNC(exp10) (-4932), 1.0e-4932L, 0, 0, UNDERFLOW_EXCEPTION);
#endif
  check_float ("exp10 (1e6) == inf",  FUNC(exp10) (1e6), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("exp10 (-1e6) == 0",  FUNC(exp10) (-1e6), 0, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("exp10 (max_value) == inf",  FUNC(exp10) (max_value), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("exp10 (-max_value) == 0",  FUNC(exp10) (-max_value), 0, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("exp10 (0.75) == 5.62341325190349080394951039776481231",  FUNC(exp10) (0.75L), 5.62341325190349080394951039776481231L, DELTA1572, 0, 0);

  print_max_error ("exp10", DELTAexp10, 0);
}


static void
exp2_test (void)
{
  errno = 0;
  FUNC(exp2) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_float ("exp2 (0) == 1",  FUNC(exp2) (0), 1, 0, 0, 0);
  check_float ("exp2 (-0) == 1",  FUNC(exp2) (minus_zero), 1, 0, 0, 0);
  check_float ("exp2 (inf) == inf",  FUNC(exp2) (plus_infty), plus_infty, 0, 0, 0);
  check_float ("exp2 (-inf) == 0",  FUNC(exp2) (minus_infty), 0, 0, 0, 0);
  check_float ("exp2 (NaN) == NaN",  FUNC(exp2) (nan_value), nan_value, 0, 0, 0);

  check_float ("exp2 (10) == 1024",  FUNC(exp2) (10), 1024, 0, 0, 0);
  check_float ("exp2 (-1) == 0.5",  FUNC(exp2) (-1), 0.5, 0, 0, 0);
  check_float ("exp2 (1e6) == inf",  FUNC(exp2) (1e6), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("exp2 (-1e6) == 0",  FUNC(exp2) (-1e6), 0, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("exp2 (max_value) == inf",  FUNC(exp2) (max_value), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("exp2 (-max_value) == 0",  FUNC(exp2) (-max_value), 0, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("exp2 (0.75) == 1.68179283050742908606225095246642979",  FUNC(exp2) (0.75L), 1.68179283050742908606225095246642979L, 0, 0, 0);

  check_float ("exp2 (100.5) == 1.792728671193156477399422023278661496394e+30",  FUNC(exp2) (100.5), 1.792728671193156477399422023278661496394e+30L, 0, 0, 0);
  check_float ("exp2 (127) == 0x1p127",  FUNC(exp2) (127), 0x1p127, 0, 0, 0);
  check_float ("exp2 (-149) == 0x1p-149",  FUNC(exp2) (-149), 0x1p-149, 0, 0, 0);

#ifndef TEST_FLOAT
  check_float ("exp2 (1000.25) == 1.274245659452564874772384918171765416737e+301",  FUNC(exp2) (1000.25), 1.274245659452564874772384918171765416737e+301L, 0, 0, 0);
  check_float ("exp2 (1023) == 0x1p1023",  FUNC(exp2) (1023), 0x1p1023, 0, 0, 0);
  check_float ("exp2 (-1074) == 0x1p-1074",  FUNC(exp2) (-1074), 0x1p-1074, 0, 0, 0);
#endif

#if defined TEST_LDOUBLE && LDBL_MAX_EXP >= 16384
  check_float ("exp2 (16383) == 0x1p16383",  FUNC(exp2) (16383), 0x1p16383L, 0, 0, 0);
  check_float ("exp2 (-16400) == 0x1p-16400",  FUNC(exp2) (-16400), 0x1p-16400L, 0, 0, 0);
#endif

  print_max_error ("exp2", 0, 0);
}


static void
expm1_test (void)
{
  errno = 0;
  FUNC(expm1) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_float ("expm1 (0) == 0",  FUNC(expm1) (0), 0, 0, 0, 0);
  check_float ("expm1 (-0) == -0",  FUNC(expm1) (minus_zero), minus_zero, 0, 0, 0);

#ifndef TEST_INLINE
  check_float ("expm1 (inf) == inf",  FUNC(expm1) (plus_infty), plus_infty, 0, 0, 0);
  check_float ("expm1 (-inf) == -1",  FUNC(expm1) (minus_infty), -1, 0, 0, 0);
#endif
  check_float ("expm1 (NaN) == NaN",  FUNC(expm1) (nan_value), nan_value, 0, 0, 0);

  check_float ("expm1 (1) == M_El - 1.0",  FUNC(expm1) (1), M_El - 1.0, DELTA1598, 0, 0);
  check_float ("expm1 (0.75) == 1.11700001661267466854536981983709561",  FUNC(expm1) (0.75L), 1.11700001661267466854536981983709561L, DELTA1599, 0, 0);

  check_float ("expm1 (50.0) == 5.1847055285870724640864533229334853848275e+21",  FUNC(expm1) (50.0L), 5.1847055285870724640864533229334853848275e+21L, 0, 0, 0);

#ifndef TEST_FLOAT
  check_float ("expm1 (127.0) == 1.4302079958348104463583671072905261080748e+55",  FUNC(expm1) (127.0L), 1.4302079958348104463583671072905261080748e+55L, 0, 0, 0);
  check_float ("expm1 (500.0) == 1.4035922178528374107397703328409120821806e+217",  FUNC(expm1) (500.0L), 1.4035922178528374107397703328409120821806e+217L, DELTA1602, 0, 0);
#endif

#if defined TEST_LDOUBLE && LDBL_MAX_EXP >= 16384
  check_float ("expm1 (11356.25) == 9.05128237311923300051376115753226014206e+4931",  FUNC(expm1) (11356.25L), 9.05128237311923300051376115753226014206e+4931L, DELTA1603, 0, 0);
#endif

  check_float ("expm1 (-10.0) == -0.9999546000702375151484644084844394493898",  FUNC(expm1) (-10.0), -0.9999546000702375151484644084844394493898L, 0, 0, 0);
  check_float ("expm1 (-16.0) == -0.9999998874648252807408854862248209398728",  FUNC(expm1) (-16.0), -0.9999998874648252807408854862248209398728L, 0, 0, 0);
  check_float ("expm1 (-17.0) == -0.9999999586006228121483334034897228104472",  FUNC(expm1) (-17.0), -0.9999999586006228121483334034897228104472L, 0, 0, 0);
  check_float ("expm1 (-18.0) == -0.9999999847700202552873715638633707664826",  FUNC(expm1) (-18.0), -0.9999999847700202552873715638633707664826L, 0, 0, 0);
  check_float ("expm1 (-36.0) == -0.9999999999999997680477169756430611687736",  FUNC(expm1) (-36.0), -0.9999999999999997680477169756430611687736L, 0, 0, 0);
  check_float ("expm1 (-37.0) == -0.9999999999999999146695237425593420572195",  FUNC(expm1) (-37.0), -0.9999999999999999146695237425593420572195L, 0, 0, 0);
  check_float ("expm1 (-38.0) == -0.9999999999999999686086720795197037129104",  FUNC(expm1) (-38.0), -0.9999999999999999686086720795197037129104L, 0, 0, 0);
  check_float ("expm1 (-44.0) == -0.9999999999999999999221886775886620348429",  FUNC(expm1) (-44.0), -0.9999999999999999999221886775886620348429L, 0, 0, 0);
  check_float ("expm1 (-45.0) == -0.9999999999999999999713748141945060635553",  FUNC(expm1) (-45.0), -0.9999999999999999999713748141945060635553L, DELTA1612, 0, 0);
  check_float ("expm1 (-46.0) == -0.9999999999999999999894693826424461876212",  FUNC(expm1) (-46.0), -0.9999999999999999999894693826424461876212L, 0, 0, 0);
  check_float ("expm1 (-73.0) == -0.9999999999999999999999999999999802074012",  FUNC(expm1) (-73.0), -0.9999999999999999999999999999999802074012L, 0, 0, 0);
  check_float ("expm1 (-74.0) == -0.9999999999999999999999999999999927187098",  FUNC(expm1) (-74.0), -0.9999999999999999999999999999999927187098L, 0, 0, 0);
  check_float ("expm1 (-75.0) == -0.9999999999999999999999999999999973213630",  FUNC(expm1) (-75.0), -0.9999999999999999999999999999999973213630L, 0, 0, 0);
  check_float ("expm1 (-78.0) == -0.9999999999999999999999999999999998666385",  FUNC(expm1) (-78.0), -0.9999999999999999999999999999999998666385L, 0, 0, 0);
  check_float ("expm1 (-79.0) == -0.9999999999999999999999999999999999509391",  FUNC(expm1) (-79.0), -0.9999999999999999999999999999999999509391L, 0, 0, 0);
  check_float ("expm1 (-80.0) == -0.9999999999999999999999999999999999819515",  FUNC(expm1) (-80.0), -0.9999999999999999999999999999999999819515L, 0, 0, 0);
  check_float ("expm1 (-100.0) == -1.0",  FUNC(expm1) (-100.0), -1.0, 0, 0, 0);
  check_float ("expm1 (-1000.0) == -1.0",  FUNC(expm1) (-1000.0), -1.0, 0, 0, 0);
  check_float ("expm1 (-10000.0) == -1.0",  FUNC(expm1) (-10000.0), -1.0, 0, 0, 0);
  check_float ("expm1 (-100000.0) == -1.0",  FUNC(expm1) (-100000.0), -1.0, 0, 0, 0);

  errno = 0;
  check_float ("expm1 (100000.0) == inf",  FUNC(expm1) (100000.0), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_int ("errno for expm1(large) == ERANGE", errno, ERANGE, 0, 0, 0);
  check_float ("expm1 (max_value) == inf",  FUNC(expm1) (max_value), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("expm1 (-max_value) == -1",  FUNC(expm1) (-max_value), -1, 0, 0, 0);

  print_max_error ("expm1", DELTAexpm1, 0);
}


static void
fabs_test (void)
{
  init_max_error ();

  check_float ("fabs (0) == 0",  FUNC(fabs) (0), 0, 0, 0, 0);
  check_float ("fabs (-0) == 0",  FUNC(fabs) (minus_zero), 0, 0, 0, 0);

  check_float ("fabs (inf) == inf",  FUNC(fabs) (plus_infty), plus_infty, 0, 0, 0);
  check_float ("fabs (-inf) == inf",  FUNC(fabs) (minus_infty), plus_infty, 0, 0, 0);
  check_float ("fabs (NaN) == NaN",  FUNC(fabs) (nan_value), nan_value, 0, 0, 0);

  check_float ("fabs (38.0) == 38.0",  FUNC(fabs) (38.0), 38.0, 0, 0, 0);
  check_float ("fabs (-e) == e",  FUNC(fabs) (-M_El), M_El, 0, 0, 0);

  print_max_error ("fabs", 0, 0);
}


static void
fdim_test (void)
{
  init_max_error ();

  check_float ("fdim (0, 0) == 0",  FUNC(fdim) (0, 0), 0, 0, 0, 0);
  check_float ("fdim (9, 0) == 9",  FUNC(fdim) (9, 0), 9, 0, 0, 0);
  check_float ("fdim (0, 9) == 0",  FUNC(fdim) (0, 9), 0, 0, 0, 0);
  check_float ("fdim (-9, 0) == 0",  FUNC(fdim) (-9, 0), 0, 0, 0, 0);
  check_float ("fdim (0, -9) == 9",  FUNC(fdim) (0, -9), 9, 0, 0, 0);

  check_float ("fdim (inf, 9) == inf",  FUNC(fdim) (plus_infty, 9), plus_infty, 0, 0, 0);
  check_float ("fdim (inf, -9) == inf",  FUNC(fdim) (plus_infty, -9), plus_infty, 0, 0, 0);
  check_float ("fdim (-inf, 9) == 0",  FUNC(fdim) (minus_infty, 9), 0, 0, 0, 0);
  check_float ("fdim (-inf, -9) == 0",  FUNC(fdim) (minus_infty, -9), 0, 0, 0, 0);
  check_float ("fdim (9, -inf) == inf",  FUNC(fdim) (9, minus_infty), plus_infty, 0, 0, 0);
  check_float ("fdim (-9, -inf) == inf",  FUNC(fdim) (-9, minus_infty), plus_infty, 0, 0, 0);
  check_float ("fdim (9, inf) == 0",  FUNC(fdim) (9, plus_infty), 0, 0, 0, 0);
  check_float ("fdim (-9, inf) == 0",  FUNC(fdim) (-9, plus_infty), 0, 0, 0, 0);

  check_float ("fdim (0, NaN) == NaN",  FUNC(fdim) (0, nan_value), nan_value, 0, 0, 0);
  check_float ("fdim (9, NaN) == NaN",  FUNC(fdim) (9, nan_value), nan_value, 0, 0, 0);
  check_float ("fdim (-9, NaN) == NaN",  FUNC(fdim) (-9, nan_value), nan_value, 0, 0, 0);
  check_float ("fdim (NaN, 9) == NaN",  FUNC(fdim) (nan_value, 9), nan_value, 0, 0, 0);
  check_float ("fdim (NaN, -9) == NaN",  FUNC(fdim) (nan_value, -9), nan_value, 0, 0, 0);
  check_float ("fdim (inf, NaN) == NaN",  FUNC(fdim) (plus_infty, nan_value), nan_value, 0, 0, 0);
  check_float ("fdim (-inf, NaN) == NaN",  FUNC(fdim) (minus_infty, nan_value), nan_value, 0, 0, 0);
  check_float ("fdim (NaN, inf) == NaN",  FUNC(fdim) (nan_value, plus_infty), nan_value, 0, 0, 0);
  check_float ("fdim (NaN, -inf) == NaN",  FUNC(fdim) (nan_value, minus_infty), nan_value, 0, 0, 0);
  check_float ("fdim (NaN, NaN) == NaN",  FUNC(fdim) (nan_value, nan_value), nan_value, 0, 0, 0);

  check_float ("fdim (inf, inf) == 0",  FUNC(fdim) (plus_infty, plus_infty), 0, 0, 0, 0);

  print_max_error ("fdim", 0, 0);
}


static void
finite_test (void)
{
  init_max_error ();

  check_bool ("finite (0) == true",  FUNC(finite) (0), 1, 0, 0, 0);
  check_bool ("finite (-0) == true",  FUNC(finite) (minus_zero), 1, 0, 0, 0);
  check_bool ("finite (10) == true",  FUNC(finite) (10), 1, 0, 0, 0);
  check_bool ("finite (min_subnorm_value) == true",  FUNC(finite) (min_subnorm_value), 1, 0, 0, 0);
  check_bool ("finite (inf) == false",  FUNC(finite) (plus_infty), 0, 0, 0, 0);
  check_bool ("finite (-inf) == false",  FUNC(finite) (minus_infty), 0, 0, 0, 0);
  check_bool ("finite (NaN) == false",  FUNC(finite) (nan_value), 0, 0, 0, 0);

  print_max_error ("finite", 0, 0);
}


static void
floor_test (void)
{
  init_max_error ();

  check_float ("floor (0.0) == 0.0",  FUNC(floor) (0.0), 0.0, 0, 0, 0);
  check_float ("floor (-0) == -0",  FUNC(floor) (minus_zero), minus_zero, 0, 0, 0);
  check_float ("floor (inf) == inf",  FUNC(floor) (plus_infty), plus_infty, 0, 0, 0);
  check_float ("floor (-inf) == -inf",  FUNC(floor) (minus_infty), minus_infty, 0, 0, 0);
  check_float ("floor (NaN) == NaN",  FUNC(floor) (nan_value), nan_value, 0, 0, 0);

  check_float ("floor (pi) == 3.0",  FUNC(floor) (M_PIl), 3.0, 0, 0, 0);
  check_float ("floor (-pi) == -4.0",  FUNC(floor) (-M_PIl), -4.0, 0, 0, 0);

  check_float ("floor (0.1) == 0.0",  FUNC(floor) (0.1), 0.0, 0, 0, 0);
  check_float ("floor (0.25) == 0.0",  FUNC(floor) (0.25), 0.0, 0, 0, 0);
  check_float ("floor (0.625) == 0.0",  FUNC(floor) (0.625), 0.0, 0, 0, 0);
  check_float ("floor (-0.1) == -1.0",  FUNC(floor) (-0.1), -1.0, 0, 0, 0);
  check_float ("floor (-0.25) == -1.0",  FUNC(floor) (-0.25), -1.0, 0, 0, 0);
  check_float ("floor (-0.625) == -1.0",  FUNC(floor) (-0.625), -1.0, 0, 0, 0);

#ifdef TEST_LDOUBLE
  /* The result can only be represented in long double.  */
  check_float ("floor (4503599627370495.5) == 4503599627370495.0",  FUNC(floor) (4503599627370495.5L), 4503599627370495.0L, 0, 0, 0);
  check_float ("floor (4503599627370496.25) == 4503599627370496.0",  FUNC(floor) (4503599627370496.25L), 4503599627370496.0L, 0, 0, 0);
  check_float ("floor (4503599627370496.5) == 4503599627370496.0",  FUNC(floor) (4503599627370496.5L), 4503599627370496.0L, 0, 0, 0);
  check_float ("floor (4503599627370496.75) == 4503599627370496.0",  FUNC(floor) (4503599627370496.75L), 4503599627370496.0L, 0, 0, 0);
  check_float ("floor (4503599627370497.5) == 4503599627370497.0",  FUNC(floor) (4503599627370497.5L), 4503599627370497.0L, 0, 0, 0);
# if LDBL_MANT_DIG > 100
  check_float ("floor (4503599627370494.5000000000001) == 4503599627370494.0",  FUNC(floor) (4503599627370494.5000000000001L), 4503599627370494.0L, 0, 0, 0);
  check_float ("floor (4503599627370495.5000000000001) == 4503599627370495.0",  FUNC(floor) (4503599627370495.5000000000001L), 4503599627370495.0L, 0, 0, 0);
  check_float ("floor (4503599627370496.5000000000001) == 4503599627370496.0",  FUNC(floor) (4503599627370496.5000000000001L), 4503599627370496.0L, 0, 0, 0);
# endif

  check_float ("floor (-4503599627370495.5) == -4503599627370496.0",  FUNC(floor) (-4503599627370495.5L), -4503599627370496.0L, 0, 0, 0);
  check_float ("floor (-4503599627370496.25) == -4503599627370497.0",  FUNC(floor) (-4503599627370496.25L), -4503599627370497.0L, 0, 0, 0);
  check_float ("floor (-4503599627370496.5) == -4503599627370497.0",  FUNC(floor) (-4503599627370496.5L), -4503599627370497.0L, 0, 0, 0);
  check_float ("floor (-4503599627370496.75) == -4503599627370497.0",  FUNC(floor) (-4503599627370496.75L), -4503599627370497.0L, 0, 0, 0);
  check_float ("floor (-4503599627370497.5) == -4503599627370498.0",  FUNC(floor) (-4503599627370497.5L), -4503599627370498.0L, 0, 0, 0);
# if LDBL_MANT_DIG > 100
  check_float ("floor (-4503599627370494.5000000000001) == -4503599627370495.0",  FUNC(floor) (-4503599627370494.5000000000001L), -4503599627370495.0L, 0, 0, 0);
  check_float ("floor (-4503599627370495.5000000000001) == -4503599627370496.0",  FUNC(floor) (-4503599627370495.5000000000001L), -4503599627370496.0L, 0, 0, 0);
  check_float ("floor (-4503599627370496.5000000000001) == -4503599627370497.0",  FUNC(floor) (-4503599627370496.5000000000001L), -4503599627370497.0L, 0, 0, 0);
# endif

  check_float ("floor (9007199254740991.5) == 9007199254740991.0",  FUNC(floor) (9007199254740991.5L), 9007199254740991.0L, 0, 0, 0);
  check_float ("floor (9007199254740992.25) == 9007199254740992.0",  FUNC(floor) (9007199254740992.25L), 9007199254740992.0L, 0, 0, 0);
  check_float ("floor (9007199254740992.5) == 9007199254740992.0",  FUNC(floor) (9007199254740992.5L), 9007199254740992.0L, 0, 0, 0);
  check_float ("floor (9007199254740992.75) == 9007199254740992.0",  FUNC(floor) (9007199254740992.75L), 9007199254740992.0L, 0, 0, 0);
  check_float ("floor (9007199254740993.5) == 9007199254740993.0",  FUNC(floor) (9007199254740993.5L), 9007199254740993.0L, 0, 0, 0);

# if LDBL_MANT_DIG > 100
  check_float ("floor (9007199254740991.0000000000001) == 9007199254740991.0",  FUNC(floor) (9007199254740991.0000000000001L), 9007199254740991.0L, 0, 0, 0);
  check_float ("floor (9007199254740992.0000000000001) == 9007199254740992.0",  FUNC(floor) (9007199254740992.0000000000001L), 9007199254740992.0L, 0, 0, 0);
  check_float ("floor (9007199254740993.0000000000001) == 9007199254740993.0",  FUNC(floor) (9007199254740993.0000000000001L), 9007199254740993.0L, 0, 0, 0);
  check_float ("floor (9007199254740991.5000000000001) == 9007199254740991.0",  FUNC(floor) (9007199254740991.5000000000001L), 9007199254740991.0L, 0, 0, 0);
  check_float ("floor (9007199254740992.5000000000001) == 9007199254740992.0",  FUNC(floor) (9007199254740992.5000000000001L), 9007199254740992.0L, 0, 0, 0);
  check_float ("floor (9007199254740993.5000000000001) == 9007199254740993.0",  FUNC(floor) (9007199254740993.5000000000001L), 9007199254740993.0L, 0, 0, 0);
# endif

  check_float ("floor (-9007199254740991.5) == -9007199254740992.0",  FUNC(floor) (-9007199254740991.5L), -9007199254740992.0L, 0, 0, 0);
  check_float ("floor (-9007199254740992.25) == -9007199254740993.0",  FUNC(floor) (-9007199254740992.25L), -9007199254740993.0L, 0, 0, 0);
  check_float ("floor (-9007199254740992.5) == -9007199254740993.0",  FUNC(floor) (-9007199254740992.5L), -9007199254740993.0L, 0, 0, 0);
  check_float ("floor (-9007199254740992.75) == -9007199254740993.0",  FUNC(floor) (-9007199254740992.75L), -9007199254740993.0L, 0, 0, 0);
  check_float ("floor (-9007199254740993.5) == -9007199254740994.0",  FUNC(floor) (-9007199254740993.5L), -9007199254740994.0L, 0, 0, 0);

# if LDBL_MANT_DIG > 100
  check_float ("floor (-9007199254740991.0000000000001) == -9007199254740992.0",  FUNC(floor) (-9007199254740991.0000000000001L), -9007199254740992.0L, 0, 0, 0);
  check_float ("floor (-9007199254740992.0000000000001) == -9007199254740993.0",  FUNC(floor) (-9007199254740992.0000000000001L), -9007199254740993.0L, 0, 0, 0);
  check_float ("floor (-9007199254740993.0000000000001) == -9007199254740994.0",  FUNC(floor) (-9007199254740993.0000000000001L), -9007199254740994.0L, 0, 0, 0);
  check_float ("floor (-9007199254740991.5000000000001) == -9007199254740992.0",  FUNC(floor) (-9007199254740991.5000000000001L), -9007199254740992.0L, 0, 0, 0);
  check_float ("floor (-9007199254740992.5000000000001) == -9007199254740993.0",  FUNC(floor) (-9007199254740992.5000000000001L), -9007199254740993.0L, 0, 0, 0);
  check_float ("floor (-9007199254740993.5000000000001) == -9007199254740994.0",  FUNC(floor) (-9007199254740993.5000000000001L), -9007199254740994.0L, 0, 0, 0);
# endif

  check_float ("floor (72057594037927935.5) == 72057594037927935.0",  FUNC(floor) (72057594037927935.5L), 72057594037927935.0L, 0, 0, 0);
  check_float ("floor (72057594037927936.25) == 72057594037927936.0",  FUNC(floor) (72057594037927936.25L), 72057594037927936.0L, 0, 0, 0);
  check_float ("floor (72057594037927936.5) == 72057594037927936.0",  FUNC(floor) (72057594037927936.5L), 72057594037927936.0L, 0, 0, 0);
  check_float ("floor (72057594037927936.75) == 72057594037927936.0",  FUNC(floor) (72057594037927936.75L), 72057594037927936.0L, 0, 0, 0);
  check_float ("floor (72057594037927937.5) == 72057594037927937.0",  FUNC(floor) (72057594037927937.5L), 72057594037927937.0L, 0, 0, 0);

  check_float ("floor (-72057594037927935.5) == -72057594037927936.0",  FUNC(floor) (-72057594037927935.5L), -72057594037927936.0L, 0, 0, 0);
  check_float ("floor (-72057594037927936.25) == -72057594037927937.0",  FUNC(floor) (-72057594037927936.25L), -72057594037927937.0L, 0, 0, 0);
  check_float ("floor (-72057594037927936.5) == -72057594037927937.0",  FUNC(floor) (-72057594037927936.5L), -72057594037927937.0L, 0, 0, 0);
  check_float ("floor (-72057594037927936.75) == -72057594037927937.0",  FUNC(floor) (-72057594037927936.75L), -72057594037927937.0L, 0, 0, 0);
  check_float ("floor (-72057594037927937.5) == -72057594037927938.0",  FUNC(floor) (-72057594037927937.5L), -72057594037927938.0L, 0, 0, 0);

  check_float ("floor (10141204801825835211973625643007.5) == 10141204801825835211973625643007.0",  FUNC(floor) (10141204801825835211973625643007.5L), 10141204801825835211973625643007.0L, 0, 0, 0);
  check_float ("floor (10141204801825835211973625643008.25) == 10141204801825835211973625643008.0",  FUNC(floor) (10141204801825835211973625643008.25L), 10141204801825835211973625643008.0L, 0, 0, 0);
  check_float ("floor (10141204801825835211973625643008.5) == 10141204801825835211973625643008.0",  FUNC(floor) (10141204801825835211973625643008.5L), 10141204801825835211973625643008.0L, 0, 0, 0);
  check_float ("floor (10141204801825835211973625643008.75) == 10141204801825835211973625643008.0",  FUNC(floor) (10141204801825835211973625643008.75L), 10141204801825835211973625643008.0L, 0, 0, 0);
  check_float ("floor (10141204801825835211973625643009.5) == 10141204801825835211973625643009.0",  FUNC(floor) (10141204801825835211973625643009.5L), 10141204801825835211973625643009.0L, 0, 0, 0);

  check_float ("floor (0xf.ffffffffffffff8p+47) == 0xf.fffffffffffep+47",  FUNC(floor) (0xf.ffffffffffffff8p+47L), 0xf.fffffffffffep+47L, 0, 0, 0);
  check_float ("floor (-0x8.000000000000004p+48) == -0x8.000000000001p+48",  FUNC(floor) (-0x8.000000000000004p+48L), -0x8.000000000001p+48L, 0, 0, 0);
#endif

  print_max_error ("floor", 0, 0);
}


static void
fma_test (void)
{
  init_max_error ();

  check_float ("fma (1.0, 2.0, 3.0) == 5.0",  FUNC(fma) (1.0, 2.0, 3.0), 5.0, 0, 0, 0);
  check_float ("fma (NaN, 2.0, 3.0) == NaN",  FUNC(fma) (nan_value, 2.0, 3.0), nan_value, 0, 0, 0);
  check_float ("fma (1.0, NaN, 3.0) == NaN",  FUNC(fma) (1.0, nan_value, 3.0), nan_value, 0, 0, 0);
  check_float ("fma (1.0, 2.0, NaN) == NaN",  FUNC(fma) (1.0, 2.0, nan_value), nan_value, 0, 0, INVALID_EXCEPTION_OK);
  check_float ("fma (inf, 0.0, NaN) == NaN",  FUNC(fma) (plus_infty, 0.0, nan_value), nan_value, 0, 0, INVALID_EXCEPTION_OK);
  check_float ("fma (-inf, 0.0, NaN) == NaN",  FUNC(fma) (minus_infty, 0.0, nan_value), nan_value, 0, 0, INVALID_EXCEPTION_OK);
  check_float ("fma (0.0, inf, NaN) == NaN",  FUNC(fma) (0.0, plus_infty, nan_value), nan_value, 0, 0, INVALID_EXCEPTION_OK);
  check_float ("fma (0.0, -inf, NaN) == NaN",  FUNC(fma) (0.0, minus_infty, nan_value), nan_value, 0, 0, INVALID_EXCEPTION_OK);
  check_float ("fma (inf, 0.0, 1.0) == NaN",  FUNC(fma) (plus_infty, 0.0, 1.0), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("fma (-inf, 0.0, 1.0) == NaN",  FUNC(fma) (minus_infty, 0.0, 1.0), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("fma (0.0, inf, 1.0) == NaN",  FUNC(fma) (0.0, plus_infty, 1.0), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("fma (0.0, -inf, 1.0) == NaN",  FUNC(fma) (0.0, minus_infty, 1.0), nan_value, 0, 0, INVALID_EXCEPTION);

  check_float ("fma (inf, inf, -inf) == NaN",  FUNC(fma) (plus_infty, plus_infty, minus_infty), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("fma (-inf, inf, inf) == NaN",  FUNC(fma) (minus_infty, plus_infty, plus_infty), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("fma (inf, -inf, inf) == NaN",  FUNC(fma) (plus_infty, minus_infty, plus_infty), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("fma (-inf, -inf, -inf) == NaN",  FUNC(fma) (minus_infty, minus_infty, minus_infty), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("fma (inf, 3.5, -inf) == NaN",  FUNC(fma) (plus_infty, 3.5L, minus_infty), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("fma (-inf, -7.5, -inf) == NaN",  FUNC(fma) (minus_infty, -7.5L, minus_infty), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("fma (-13.5, inf, inf) == NaN",  FUNC(fma) (-13.5L, plus_infty, plus_infty), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("fma (-inf, 7.5, inf) == NaN",  FUNC(fma) (minus_infty, 7.5L, plus_infty), nan_value, 0, 0, INVALID_EXCEPTION);

  check_float ("fma (1.25, 0.75, 0.0625) == 1.0",  FUNC(fma) (1.25L, 0.75L, 0.0625L), 1.0L, 0, 0, 0);

  FLOAT fltmax = CHOOSE (LDBL_MAX, DBL_MAX, FLT_MAX,
			 LDBL_MAX, DBL_MAX, FLT_MAX);
  check_float ("fma (-fltmax, -fltmax, -inf) == -inf",  FUNC(fma) (-fltmax, -fltmax, minus_infty), minus_infty, 0, 0, 0);
  check_float ("fma (fltmax / 2, fltmax / 2, -inf) == -inf",  FUNC(fma) (fltmax / 2, fltmax / 2, minus_infty), minus_infty, 0, 0, 0);
  check_float ("fma (-fltmax, fltmax, inf) == inf",  FUNC(fma) (-fltmax, fltmax, plus_infty), plus_infty, 0, 0, 0);
  check_float ("fma (fltmax / 2, -fltmax / 4, inf) == inf",  FUNC(fma) (fltmax / 2, -fltmax / 4, plus_infty), plus_infty, 0, 0, 0);
  check_float ("fma (inf, 4, inf) == inf",  FUNC(fma) (plus_infty, 4, plus_infty), plus_infty, 0, 0, 0);
  check_float ("fma (2, -inf, -inf) == -inf",  FUNC(fma) (2, minus_infty, minus_infty), minus_infty, 0, 0, 0);
  check_float ("fma (-inf, -inf, inf) == inf",  FUNC(fma) (minus_infty, minus_infty, plus_infty), plus_infty, 0, 0, 0);
  check_float ("fma (inf, -inf, -inf) == -inf",  FUNC(fma) (plus_infty, minus_infty, minus_infty), minus_infty, 0, 0, 0);

  check_float ("fma (+0, +0, +0) == +0",  FUNC(fma) (plus_zero, plus_zero, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma (+0, +0, -0) == +0",  FUNC(fma) (plus_zero, plus_zero, minus_zero), plus_zero, 0, 0, 0);
  check_float ("fma (+0, -0, +0) == +0",  FUNC(fma) (plus_zero, minus_zero, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma (+0, -0, -0) == -0",  FUNC(fma) (plus_zero, minus_zero, minus_zero), minus_zero, 0, 0, 0);
  check_float ("fma (-0, +0, +0) == +0",  FUNC(fma) (minus_zero, plus_zero, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma (-0, +0, -0) == -0",  FUNC(fma) (minus_zero, plus_zero, minus_zero), minus_zero, 0, 0, 0);
  check_float ("fma (-0, -0, +0) == +0",  FUNC(fma) (minus_zero, minus_zero, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma (-0, -0, -0) == +0",  FUNC(fma) (minus_zero, minus_zero, minus_zero), plus_zero, 0, 0, 0);
  check_float ("fma (1.0, +0, +0) == +0",  FUNC(fma) (1.0, plus_zero, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma (1.0, +0, -0) == +0",  FUNC(fma) (1.0, plus_zero, minus_zero), plus_zero, 0, 0, 0);
  check_float ("fma (1.0, -0, +0) == +0",  FUNC(fma) (1.0, minus_zero, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma (1.0, -0, -0) == -0",  FUNC(fma) (1.0, minus_zero, minus_zero), minus_zero, 0, 0, 0);
  check_float ("fma (-1.0, +0, +0) == +0",  FUNC(fma) (-1.0, plus_zero, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma (-1.0, +0, -0) == -0",  FUNC(fma) (-1.0, plus_zero, minus_zero), minus_zero, 0, 0, 0);
  check_float ("fma (-1.0, -0, +0) == +0",  FUNC(fma) (-1.0, minus_zero, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma (-1.0, -0, -0) == +0",  FUNC(fma) (-1.0, minus_zero, minus_zero), plus_zero, 0, 0, 0);
  check_float ("fma (+0, 1.0, +0) == +0",  FUNC(fma) (plus_zero, 1.0, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma (+0, 1.0, -0) == +0",  FUNC(fma) (plus_zero, 1.0, minus_zero), plus_zero, 0, 0, 0);
  check_float ("fma (+0, -1.0, +0) == +0",  FUNC(fma) (plus_zero, -1.0, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma (+0, -1.0, -0) == -0",  FUNC(fma) (plus_zero, -1.0, minus_zero), minus_zero, 0, 0, 0);
  check_float ("fma (-0, 1.0, +0) == +0",  FUNC(fma) (minus_zero, 1.0, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma (-0, 1.0, -0) == -0",  FUNC(fma) (minus_zero, 1.0, minus_zero), minus_zero, 0, 0, 0);
  check_float ("fma (-0, -1.0, +0) == +0",  FUNC(fma) (minus_zero, -1.0, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma (-0, -1.0, -0) == +0",  FUNC(fma) (minus_zero, -1.0, minus_zero), plus_zero, 0, 0, 0);

  check_float ("fma (1.0, 1.0, -1.0) == +0",  FUNC(fma) (1.0, 1.0, -1.0), plus_zero, 0, 0, 0);
  check_float ("fma (1.0, -1.0, 1.0) == +0",  FUNC(fma) (1.0, -1.0, 1.0), plus_zero, 0, 0, 0);
  check_float ("fma (-1.0, 1.0, 1.0) == +0",  FUNC(fma) (-1.0, 1.0, 1.0), plus_zero, 0, 0, 0);
  check_float ("fma (-1.0, -1.0, -1.0) == +0",  FUNC(fma) (-1.0, -1.0, -1.0), plus_zero, 0, 0, 0);

  check_float ("fma (min_value, min_value, +0) == +0",  FUNC(fma) (min_value, min_value, plus_zero), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (min_value, min_value, -0) == +0",  FUNC(fma) (min_value, min_value, minus_zero), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (min_value, -min_value, +0) == -0",  FUNC(fma) (min_value, -min_value, plus_zero), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (min_value, -min_value, -0) == -0",  FUNC(fma) (min_value, -min_value, minus_zero), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (-min_value, min_value, +0) == -0",  FUNC(fma) (-min_value, min_value, plus_zero), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (-min_value, min_value, -0) == -0",  FUNC(fma) (-min_value, min_value, minus_zero), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (-min_value, -min_value, +0) == +0",  FUNC(fma) (-min_value, -min_value, plus_zero), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (-min_value, -min_value, -0) == +0",  FUNC(fma) (-min_value, -min_value, minus_zero), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);

  check_float ("fma (max_value, max_value, min_value) == inf",  FUNC(fma) (max_value, max_value, min_value), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("fma (max_value, max_value, -min_value) == inf",  FUNC(fma) (max_value, max_value, -min_value), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("fma (max_value, -max_value, min_value) == -inf",  FUNC(fma) (max_value, -max_value, min_value), minus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("fma (max_value, -max_value, -min_value) == -inf",  FUNC(fma) (max_value, -max_value, -min_value), minus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("fma (-max_value, max_value, min_value) == -inf",  FUNC(fma) (-max_value, max_value, min_value), minus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("fma (-max_value, max_value, -min_value) == -inf",  FUNC(fma) (-max_value, max_value, -min_value), minus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("fma (-max_value, -max_value, min_value) == inf",  FUNC(fma) (-max_value, -max_value, min_value), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("fma (-max_value, -max_value, -min_value) == inf",  FUNC(fma) (-max_value, -max_value, -min_value), plus_infty, 0, 0, OVERFLOW_EXCEPTION);

#if defined (TEST_FLOAT) && FLT_MANT_DIG == 24
  check_float ("fma (0x1.7ff8p+13, 0x1.000002p+0, 0x1.ffffp-24) == 0x1.7ff802p+13",  FUNC(fma) (0x1.7ff8p+13, 0x1.000002p+0, 0x1.ffffp-24), 0x1.7ff802p+13, 0, 0, 0);
  check_float ("fma (0x1.fffp+0, 0x1.00001p+0, -0x1.fffp+0) == 0x1.fffp-20",  FUNC(fma) (0x1.fffp+0, 0x1.00001p+0, -0x1.fffp+0), 0x1.fffp-20, 0, 0, 0);
  check_float ("fma (0x1.9abcdep+127, 0x0.9abcdep-126, -0x1.f08948p+0) == 0x1.bb421p-25",  FUNC(fma) (0x1.9abcdep+127, 0x0.9abcdep-126, -0x1.f08948p+0), 0x1.bb421p-25, 0, 0, 0);
  check_float ("fma (0x1.9abcdep+100, 0x0.9abcdep-126, -0x1.f08948p-27) == 0x1.bb421p-52",  FUNC(fma) (0x1.9abcdep+100, 0x0.9abcdep-126, -0x1.f08948p-27), 0x1.bb421p-52, 0, 0, 0);
  check_float ("fma (0x1.fffffep+127, 0x1.001p+0, -0x1.fffffep+127) == 0x1.fffffep+115",  FUNC(fma) (0x1.fffffep+127, 0x1.001p+0, -0x1.fffffep+127), 0x1.fffffep+115, 0, 0, 0);
  check_float ("fma (-0x1.fffffep+127, 0x1.fffffep+0, 0x1.fffffep+127) == -0x1.fffffap+127",  FUNC(fma) (-0x1.fffffep+127, 0x1.fffffep+0, 0x1.fffffep+127), -0x1.fffffap+127, 0, 0, 0);
  check_float ("fma (0x1.fffffep+127, 2.0, -0x1.fffffep+127) == 0x1.fffffep+127",  FUNC(fma) (0x1.fffffep+127, 2.0, -0x1.fffffep+127), 0x1.fffffep+127, 0, 0, 0);
  check_float ("fma (0x1.4p-126, 0x1.000004p-1, 0x1p-128) == 0x1.c00004p-127",  FUNC(fma) (0x1.4p-126, 0x1.000004p-1, 0x1p-128), 0x1.c00004p-127, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (-0x1.4p-126, 0x1.000004p-1, -0x1p-128) == -0x1.c00004p-127",  FUNC(fma) (-0x1.4p-126, 0x1.000004p-1, -0x1p-128), -0x1.c00004p-127, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1.fffff8p-126, 0x1.000002p-1, 0x1p-149) == 0x1p-126",  FUNC(fma) (0x1.fffff8p-126, 0x1.000002p-1, 0x1p-149), 0x1p-126, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma (-0x1.fffff8p-126, 0x1.000002p-1, -0x1p-149) == -0x1p-126",  FUNC(fma) (-0x1.fffff8p-126, 0x1.000002p-1, -0x1p-149), -0x1p-126, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma (0x1p-149, 0x1p-1, 0x0.fffffep-126) == 0x1p-126",  FUNC(fma) (0x1p-149, 0x1p-1, 0x0.fffffep-126), 0x1p-126, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (-0x1p-149, 0x1p-1, -0x0.fffffep-126) == -0x1p-126",  FUNC(fma) (-0x1p-149, 0x1p-1, -0x0.fffffep-126), -0x1p-126, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1p-149, 0x1.1p-1, 0x0.fffffep-126) == 0x1p-126",  FUNC(fma) (0x1p-149, 0x1.1p-1, 0x0.fffffep-126), 0x1p-126, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (-0x1p-149, 0x1.1p-1, -0x0.fffffep-126) == -0x1p-126",  FUNC(fma) (-0x1p-149, 0x1.1p-1, -0x0.fffffep-126), -0x1p-126, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1p-149, 0x1p-149, 0x1p127) == 0x1p127",  FUNC(fma) (0x1p-149, 0x1p-149, 0x1p127), 0x1p127, 0, 0, 0);
  check_float ("fma (0x1p-149, -0x1p-149, 0x1p127) == 0x1p127",  FUNC(fma) (0x1p-149, -0x1p-149, 0x1p127), 0x1p127, 0, 0, 0);
  check_float ("fma (0x1p-149, 0x1p-149, -0x1p127) == -0x1p127",  FUNC(fma) (0x1p-149, 0x1p-149, -0x1p127), -0x1p127, 0, 0, 0);
  check_float ("fma (0x1p-149, -0x1p-149, -0x1p127) == -0x1p127",  FUNC(fma) (0x1p-149, -0x1p-149, -0x1p127), -0x1p127, 0, 0, 0);
  check_float ("fma (0x1p-149, 0x1p-149, 0x1p-126) == 0x1p-126",  FUNC(fma) (0x1p-149, 0x1p-149, 0x1p-126), 0x1p-126, 0, 0, 0);
  check_float ("fma (0x1p-149, -0x1p-149, 0x1p-126) == 0x1p-126",  FUNC(fma) (0x1p-149, -0x1p-149, 0x1p-126), 0x1p-126, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma (0x1p-149, 0x1p-149, -0x1p-126) == -0x1p-126",  FUNC(fma) (0x1p-149, 0x1p-149, -0x1p-126), -0x1p-126, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma (0x1p-149, -0x1p-149, -0x1p-126) == -0x1p-126",  FUNC(fma) (0x1p-149, -0x1p-149, -0x1p-126), -0x1p-126, 0, 0, 0);
  check_float ("fma (0x1p-149, 0x1p-149, 0x0.fffffep-126) == 0x0.fffffep-126",  FUNC(fma) (0x1p-149, 0x1p-149, 0x0.fffffep-126), 0x0.fffffep-126, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1p-149, -0x1p-149, 0x0.fffffep-126) == 0x0.fffffep-126",  FUNC(fma) (0x1p-149, -0x1p-149, 0x0.fffffep-126), 0x0.fffffep-126, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1p-149, 0x1p-149, -0x0.fffffep-126) == -0x0.fffffep-126",  FUNC(fma) (0x1p-149, 0x1p-149, -0x0.fffffep-126), -0x0.fffffep-126, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1p-149, -0x1p-149, -0x0.fffffep-126) == -0x0.fffffep-126",  FUNC(fma) (0x1p-149, -0x1p-149, -0x0.fffffep-126), -0x0.fffffep-126, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1p-149, 0x1p-149, 0x1p-149) == 0x1p-149",  FUNC(fma) (0x1p-149, 0x1p-149, 0x1p-149), 0x1p-149, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1p-149, -0x1p-149, 0x1p-149) == 0x1p-149",  FUNC(fma) (0x1p-149, -0x1p-149, 0x1p-149), 0x1p-149, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1p-149, 0x1p-149, -0x1p-149) == -0x1p-149",  FUNC(fma) (0x1p-149, 0x1p-149, -0x1p-149), -0x1p-149, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1p-149, -0x1p-149, -0x1p-149) == -0x1p-149",  FUNC(fma) (0x1p-149, -0x1p-149, -0x1p-149), -0x1p-149, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x0.fffp0, 0x0.fffp0, -0x0.ffep0) == 0x1p-24",  FUNC(fma) (0x0.fffp0, 0x0.fffp0, -0x0.ffep0), 0x1p-24, 0, 0, 0);
  check_float ("fma (0x0.fffp0, -0x0.fffp0, 0x0.ffep0) == -0x1p-24",  FUNC(fma) (0x0.fffp0, -0x0.fffp0, 0x0.ffep0), -0x1p-24, 0, 0, 0);
  check_float ("fma (-0x0.fffp0, 0x0.fffp0, 0x0.ffep0) == -0x1p-24",  FUNC(fma) (-0x0.fffp0, 0x0.fffp0, 0x0.ffep0), -0x1p-24, 0, 0, 0);
  check_float ("fma (-0x0.fffp0, -0x0.fffp0, -0x0.ffep0) == 0x1p-24",  FUNC(fma) (-0x0.fffp0, -0x0.fffp0, -0x0.ffep0), 0x1p-24, 0, 0, 0);
  check_float ("fma (0x1.000002p-126, 0x1.000002p-26, 0x1p127) == 0x1p127",  FUNC(fma) (0x1.000002p-126, 0x1.000002p-26, 0x1p127), 0x1p127, 0, 0, 0);
  check_float ("fma (0x1.000002p-126, -0x1.000002p-26, 0x1p127) == 0x1p127",  FUNC(fma) (0x1.000002p-126, -0x1.000002p-26, 0x1p127), 0x1p127, 0, 0, 0);
  check_float ("fma (0x1.000002p-126, 0x1.000002p-26, -0x1p127) == -0x1p127",  FUNC(fma) (0x1.000002p-126, 0x1.000002p-26, -0x1p127), -0x1p127, 0, 0, 0);
  check_float ("fma (0x1.000002p-126, -0x1.000002p-26, -0x1p127) == -0x1p127",  FUNC(fma) (0x1.000002p-126, -0x1.000002p-26, -0x1p127), -0x1p127, 0, 0, 0);
  check_float ("fma (0x1.000002p-126, 0x1.000002p-26, 0x1p103) == 0x1p103",  FUNC(fma) (0x1.000002p-126, 0x1.000002p-26, 0x1p103), 0x1p103, 0, 0, 0);
  check_float ("fma (0x1.000002p-126, -0x1.000002p-26, 0x1p103) == 0x1p103",  FUNC(fma) (0x1.000002p-126, -0x1.000002p-26, 0x1p103), 0x1p103, 0, 0, 0);
  check_float ("fma (0x1.000002p-126, 0x1.000002p-26, -0x1p103) == -0x1p103",  FUNC(fma) (0x1.000002p-126, 0x1.000002p-26, -0x1p103), -0x1p103, 0, 0, 0);
  check_float ("fma (0x1.000002p-126, -0x1.000002p-26, -0x1p103) == -0x1p103",  FUNC(fma) (0x1.000002p-126, -0x1.000002p-26, -0x1p103), -0x1p103, 0, 0, 0);
#endif
#if defined (TEST_DOUBLE) && DBL_MANT_DIG == 53
  check_float ("fma (0x1.7fp+13, 0x1.0000000000001p+0, 0x1.ffep-48) == 0x1.7f00000000001p+13",  FUNC(fma) (0x1.7fp+13, 0x1.0000000000001p+0, 0x1.ffep-48), 0x1.7f00000000001p+13, 0, 0, 0);
  check_float ("fma (0x1.fffp+0, 0x1.0000000000001p+0, -0x1.fffp+0) == 0x1.fffp-52",  FUNC(fma) (0x1.fffp+0, 0x1.0000000000001p+0, -0x1.fffp+0), 0x1.fffp-52, 0, 0, 0);
  check_float ("fma (0x1.0000002p+0, 0x1.ffffffcp-1, 0x1p-300) == 1.0",  FUNC(fma) (0x1.0000002p+0, 0x1.ffffffcp-1, 0x1p-300), 1.0, 0, 0, 0);
  check_float ("fma (0x1.0000002p+0, 0x1.ffffffcp-1, -0x1p-300) == 0x1.fffffffffffffp-1",  FUNC(fma) (0x1.0000002p+0, 0x1.ffffffcp-1, -0x1p-300), 0x1.fffffffffffffp-1, 0, 0, 0);
  check_float ("fma (0x1.deadbeef2feedp+1023, 0x0.deadbeef2feedp-1022, -0x1.a05f8c01a4bfbp+1) == 0x1.0989687bc9da4p-53",  FUNC(fma) (0x1.deadbeef2feedp+1023, 0x0.deadbeef2feedp-1022, -0x1.a05f8c01a4bfbp+1), 0x1.0989687bc9da4p-53, 0, 0, 0);
  check_float ("fma (0x1.deadbeef2feedp+900, 0x0.deadbeef2feedp-1022, -0x1.a05f8c01a4bfbp-122) == 0x1.0989687bc9da4p-176",  FUNC(fma) (0x1.deadbeef2feedp+900, 0x0.deadbeef2feedp-1022, -0x1.a05f8c01a4bfbp-122), 0x1.0989687bc9da4p-176, 0, 0, 0);
  check_float ("fma (0x1.fffffffffffffp+1023, 0x1.001p+0, -0x1.fffffffffffffp+1023) == 0x1.fffffffffffffp+1011",  FUNC(fma) (0x1.fffffffffffffp+1023, 0x1.001p+0, -0x1.fffffffffffffp+1023), 0x1.fffffffffffffp+1011, 0, 0, 0);
  check_float ("fma (-0x1.fffffffffffffp+1023, 0x1.fffffffffffffp+0, 0x1.fffffffffffffp+1023) == -0x1.ffffffffffffdp+1023",  FUNC(fma) (-0x1.fffffffffffffp+1023, 0x1.fffffffffffffp+0, 0x1.fffffffffffffp+1023), -0x1.ffffffffffffdp+1023, 0, 0, 0);
  check_float ("fma (0x1.fffffffffffffp+1023, 2.0, -0x1.fffffffffffffp+1023) == 0x1.fffffffffffffp+1023",  FUNC(fma) (0x1.fffffffffffffp+1023, 2.0, -0x1.fffffffffffffp+1023), 0x1.fffffffffffffp+1023, 0, 0, 0);
  check_float ("fma (0x1.6a09e667f3bccp-538, 0x1.6a09e667f3bccp-538, 0.0) == 0.0",  FUNC(fma) (0x1.6a09e667f3bccp-538, 0x1.6a09e667f3bccp-538, 0.0), 0.0, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1.deadbeef2feedp-495, 0x1.deadbeef2feedp-495, -0x1.bf86a5786a574p-989) == 0x0.0000042625a1fp-1022",  FUNC(fma) (0x1.deadbeef2feedp-495, 0x1.deadbeef2feedp-495, -0x1.bf86a5786a574p-989), 0x0.0000042625a1fp-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1.deadbeef2feedp-503, 0x1.deadbeef2feedp-503, -0x1.bf86a5786a574p-1005) == 0x0.0000000004262p-1022",  FUNC(fma) (0x1.deadbeef2feedp-503, 0x1.deadbeef2feedp-503, -0x1.bf86a5786a574p-1005), 0x0.0000000004262p-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1p-537, 0x1p-538, 0x1p-1074) == 0x0.0000000000002p-1022",  FUNC(fma) (0x1p-537, 0x1p-538, 0x1p-1074), 0x0.0000000000002p-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1.7fffff8p-968, 0x1p-106, 0x0.000001p-1022) == 0x0.0000010000001p-1022",  FUNC(fma) (0x1.7fffff8p-968, 0x1p-106, 0x0.000001p-1022), 0x0.0000010000001p-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1.4000004p-967, 0x1p-106, 0x0.000001p-1022) == 0x0.0000010000003p-1022",  FUNC(fma) (0x1.4000004p-967, 0x1p-106, 0x0.000001p-1022), 0x0.0000010000003p-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1.4p-967, -0x1p-106, -0x0.000001p-1022) == -0x0.0000010000002p-1022",  FUNC(fma) (0x1.4p-967, -0x1p-106, -0x0.000001p-1022), -0x0.0000010000002p-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (-0x1.19cab66d73e17p-959, 0x1.c7108a8c5ff51p-107, -0x0.80b0ad65d9b64p-1022) == -0x0.80b0ad65d9d59p-1022",  FUNC(fma) (-0x1.19cab66d73e17p-959, 0x1.c7108a8c5ff51p-107, -0x0.80b0ad65d9b64p-1022), -0x0.80b0ad65d9d59p-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (-0x1.d2eaed6e8e9d3p-979, -0x1.4e066c62ac9ddp-63, -0x0.9245e6b003454p-1022) == -0x0.9245c09c5fb5dp-1022",  FUNC(fma) (-0x1.d2eaed6e8e9d3p-979, -0x1.4e066c62ac9ddp-63, -0x0.9245e6b003454p-1022), -0x0.9245c09c5fb5dp-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1.153d650bb9f06p-907, 0x1.2d01230d48407p-125, -0x0.b278d5acfc3cp-1022) == -0x0.b22757123bbe9p-1022",  FUNC(fma) (0x1.153d650bb9f06p-907, 0x1.2d01230d48407p-125, -0x0.b278d5acfc3cp-1022), -0x0.b22757123bbe9p-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (-0x1.fffffffffffffp-711, 0x1.fffffffffffffp-275, 0x1.fffffe00007ffp-983) == 0x1.7ffffe00007ffp-983",  FUNC(fma) (-0x1.fffffffffffffp-711, 0x1.fffffffffffffp-275, 0x1.fffffe00007ffp-983), 0x1.7ffffe00007ffp-983, 0, 0, 0);
  check_float ("fma (0x1.4p-1022, 0x1.0000000000002p-1, 0x1p-1024) == 0x1.c000000000002p-1023",  FUNC(fma) (0x1.4p-1022, 0x1.0000000000002p-1, 0x1p-1024), 0x1.c000000000002p-1023, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (-0x1.4p-1022, 0x1.0000000000002p-1, -0x1p-1024) == -0x1.c000000000002p-1023",  FUNC(fma) (-0x1.4p-1022, 0x1.0000000000002p-1, -0x1p-1024), -0x1.c000000000002p-1023, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1.ffffffffffffcp-1022, 0x1.0000000000001p-1, 0x1p-1074) == 0x1p-1022",  FUNC(fma) (0x1.ffffffffffffcp-1022, 0x1.0000000000001p-1, 0x1p-1074), 0x1p-1022, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma (-0x1.ffffffffffffcp-1022, 0x1.0000000000001p-1, -0x1p-1074) == -0x1p-1022",  FUNC(fma) (-0x1.ffffffffffffcp-1022, 0x1.0000000000001p-1, -0x1p-1074), -0x1p-1022, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma (0x1p-1074, 0x1p-1, 0x0.fffffffffffffp-1022) == 0x1p-1022",  FUNC(fma) (0x1p-1074, 0x1p-1, 0x0.fffffffffffffp-1022), 0x1p-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (-0x1p-1074, 0x1p-1, -0x0.fffffffffffffp-1022) == -0x1p-1022",  FUNC(fma) (-0x1p-1074, 0x1p-1, -0x0.fffffffffffffp-1022), -0x1p-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1p-1074, 0x1.1p-1, 0x0.fffffffffffffp-1022) == 0x1p-1022",  FUNC(fma) (0x1p-1074, 0x1.1p-1, 0x0.fffffffffffffp-1022), 0x1p-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (-0x1p-1074, 0x1.1p-1, -0x0.fffffffffffffp-1022) == -0x1p-1022",  FUNC(fma) (-0x1p-1074, 0x1.1p-1, -0x0.fffffffffffffp-1022), -0x1p-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1p-1074, 0x1p-1074, 0x1p1023) == 0x1p1023",  FUNC(fma) (0x1p-1074, 0x1p-1074, 0x1p1023), 0x1p1023, 0, 0, 0);
  check_float ("fma (0x1p-1074, -0x1p-1074, 0x1p1023) == 0x1p1023",  FUNC(fma) (0x1p-1074, -0x1p-1074, 0x1p1023), 0x1p1023, 0, 0, 0);
  check_float ("fma (0x1p-1074, 0x1p-1074, -0x1p1023) == -0x1p1023",  FUNC(fma) (0x1p-1074, 0x1p-1074, -0x1p1023), -0x1p1023, 0, 0, 0);
  check_float ("fma (0x1p-1074, -0x1p-1074, -0x1p1023) == -0x1p1023",  FUNC(fma) (0x1p-1074, -0x1p-1074, -0x1p1023), -0x1p1023, 0, 0, 0);
  check_float ("fma (0x1p-1074, 0x1p-1074, 0x1p-1022) == 0x1p-1022",  FUNC(fma) (0x1p-1074, 0x1p-1074, 0x1p-1022), 0x1p-1022, 0, 0, 0);
  check_float ("fma (0x1p-1074, -0x1p-1074, 0x1p-1022) == 0x1p-1022",  FUNC(fma) (0x1p-1074, -0x1p-1074, 0x1p-1022), 0x1p-1022, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma (0x1p-1074, 0x1p-1074, -0x1p-1022) == -0x1p-1022",  FUNC(fma) (0x1p-1074, 0x1p-1074, -0x1p-1022), -0x1p-1022, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma (0x1p-1074, -0x1p-1074, -0x1p-1022) == -0x1p-1022",  FUNC(fma) (0x1p-1074, -0x1p-1074, -0x1p-1022), -0x1p-1022, 0, 0, 0);
  check_float ("fma (0x1p-1074, 0x1p-1074, 0x0.fffffffffffffp-1022) == 0x0.fffffffffffffp-1022",  FUNC(fma) (0x1p-1074, 0x1p-1074, 0x0.fffffffffffffp-1022), 0x0.fffffffffffffp-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1p-1074, -0x1p-1074, 0x0.fffffffffffffp-1022) == 0x0.fffffffffffffp-1022",  FUNC(fma) (0x1p-1074, -0x1p-1074, 0x0.fffffffffffffp-1022), 0x0.fffffffffffffp-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1p-1074, 0x1p-1074, -0x0.fffffffffffffp-1022) == -0x0.fffffffffffffp-1022",  FUNC(fma) (0x1p-1074, 0x1p-1074, -0x0.fffffffffffffp-1022), -0x0.fffffffffffffp-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1p-1074, -0x1p-1074, -0x0.fffffffffffffp-1022) == -0x0.fffffffffffffp-1022",  FUNC(fma) (0x1p-1074, -0x1p-1074, -0x0.fffffffffffffp-1022), -0x0.fffffffffffffp-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1p-1074, 0x1p-1074, 0x1p-1074) == 0x1p-1074",  FUNC(fma) (0x1p-1074, 0x1p-1074, 0x1p-1074), 0x1p-1074, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1p-1074, -0x1p-1074, 0x1p-1074) == 0x1p-1074",  FUNC(fma) (0x1p-1074, -0x1p-1074, 0x1p-1074), 0x1p-1074, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1p-1074, 0x1p-1074, -0x1p-1074) == -0x1p-1074",  FUNC(fma) (0x1p-1074, 0x1p-1074, -0x1p-1074), -0x1p-1074, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1p-1074, -0x1p-1074, -0x1p-1074) == -0x1p-1074",  FUNC(fma) (0x1p-1074, -0x1p-1074, -0x1p-1074), -0x1p-1074, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x0.fffffffffffff8p0, 0x0.fffffffffffff8p0, -0x0.fffffffffffffp0) == 0x1p-106",  FUNC(fma) (0x0.fffffffffffff8p0, 0x0.fffffffffffff8p0, -0x0.fffffffffffffp0), 0x1p-106, 0, 0, 0);
  check_float ("fma (0x0.fffffffffffff8p0, -0x0.fffffffffffff8p0, 0x0.fffffffffffffp0) == -0x1p-106",  FUNC(fma) (0x0.fffffffffffff8p0, -0x0.fffffffffffff8p0, 0x0.fffffffffffffp0), -0x1p-106, 0, 0, 0);
  check_float ("fma (-0x0.fffffffffffff8p0, 0x0.fffffffffffff8p0, 0x0.fffffffffffffp0) == -0x1p-106",  FUNC(fma) (-0x0.fffffffffffff8p0, 0x0.fffffffffffff8p0, 0x0.fffffffffffffp0), -0x1p-106, 0, 0, 0);
  check_float ("fma (-0x0.fffffffffffff8p0, -0x0.fffffffffffff8p0, -0x0.fffffffffffffp0) == 0x1p-106",  FUNC(fma) (-0x0.fffffffffffff8p0, -0x0.fffffffffffff8p0, -0x0.fffffffffffffp0), 0x1p-106, 0, 0, 0);
  check_float ("fma (0x1.0000000000001p-1022, 0x1.0000000000001p-55, 0x1p1023) == 0x1p1023",  FUNC(fma) (0x1.0000000000001p-1022, 0x1.0000000000001p-55, 0x1p1023), 0x1p1023, 0, 0, 0);
  check_float ("fma (0x1.0000000000001p-1022, -0x1.0000000000001p-55, 0x1p1023) == 0x1p1023",  FUNC(fma) (0x1.0000000000001p-1022, -0x1.0000000000001p-55, 0x1p1023), 0x1p1023, 0, 0, 0);
  check_float ("fma (0x1.0000000000001p-1022, 0x1.0000000000001p-55, -0x1p1023) == -0x1p1023",  FUNC(fma) (0x1.0000000000001p-1022, 0x1.0000000000001p-55, -0x1p1023), -0x1p1023, 0, 0, 0);
  check_float ("fma (0x1.0000000000001p-1022, -0x1.0000000000001p-55, -0x1p1023) == -0x1p1023",  FUNC(fma) (0x1.0000000000001p-1022, -0x1.0000000000001p-55, -0x1p1023), -0x1p1023, 0, 0, 0);
  check_float ("fma (0x1.0000000000001p-1022, 0x1.0000000000001p-55, 0x1p970) == 0x1p970",  FUNC(fma) (0x1.0000000000001p-1022, 0x1.0000000000001p-55, 0x1p970), 0x1p970, 0, 0, 0);
  check_float ("fma (0x1.0000000000001p-1022, -0x1.0000000000001p-55, 0x1p970) == 0x1p970",  FUNC(fma) (0x1.0000000000001p-1022, -0x1.0000000000001p-55, 0x1p970), 0x1p970, 0, 0, 0);
  check_float ("fma (0x1.0000000000001p-1022, 0x1.0000000000001p-55, -0x1p970) == -0x1p970",  FUNC(fma) (0x1.0000000000001p-1022, 0x1.0000000000001p-55, -0x1p970), -0x1p970, 0, 0, 0);
  check_float ("fma (0x1.0000000000001p-1022, -0x1.0000000000001p-55, -0x1p970) == -0x1p970",  FUNC(fma) (0x1.0000000000001p-1022, -0x1.0000000000001p-55, -0x1p970), -0x1p970, 0, 0, 0);
#endif
#if defined (TEST_LDOUBLE) && LDBL_MANT_DIG == 64
  check_float ("fma (-0x8.03fcp+3696, 0xf.fffffffffffffffp-6140, 0x8.3ffffffffffffffp-2450) == -0x8.01ecp-2440",  FUNC(fma) (-0x8.03fcp+3696L, 0xf.fffffffffffffffp-6140L, 0x8.3ffffffffffffffp-2450L), -0x8.01ecp-2440L, 0, 0, 0);
  check_float ("fma (0x9.fcp+2033, -0x8.000e1f000ff800fp-3613, -0xf.fffffffffffc0ffp-1579) == -0xd.fc119fb093ed092p-1577",  FUNC(fma) (0x9.fcp+2033L, -0x8.000e1f000ff800fp-3613L, -0xf.fffffffffffc0ffp-1579L), -0xd.fc119fb093ed092p-1577L, 0, 0, 0);
  check_float ("fma (0xc.7fc000003ffffffp-1194, 0x8.1e0003fffffffffp+15327, -0x8.fffep+14072) == 0xc.ae9f164020effffp+14136",  FUNC(fma) (0xc.7fc000003ffffffp-1194L, 0x8.1e0003fffffffffp+15327L, -0x8.fffep+14072L), 0xc.ae9f164020effffp+14136L, 0, 0, 0);
  check_float ("fma (-0x8.0001fc000000003p+1798, 0xcp-2230, 0x8.f7e000000000007p-468) == -0xc.0002f9ffee10404p-429",  FUNC(fma) (-0x8.0001fc000000003p+1798L, 0xcp-2230L, 0x8.f7e000000000007p-468L), -0xc.0002f9ffee10404p-429L, 0, 0, 0);
  check_float ("fma (0xc.0000000000007ffp+10130, -0x8.000000000000001p+4430, 0xc.07000000001ffffp+14513) == -0xb.fffffffffffd7e4p+14563",  FUNC(fma) (0xc.0000000000007ffp+10130L, -0x8.000000000000001p+4430L, 0xc.07000000001ffffp+14513L), -0xb.fffffffffffd7e4p+14563L, 0, 0, 0);
  check_float ("fma (0xb.ffffp-4777, 0x8.000000fffffffffp-11612, -0x0.3800fff8p-16385) == 0x5.c7fe80c7ffeffffp-16385",  FUNC(fma) (0xb.ffffp-4777L, 0x8.000000fffffffffp-11612L, -0x0.3800fff8p-16385L), 0x5.c7fe80c7ffeffffp-16385L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1.4p-16382, 0x1.0000000000000004p-1, 0x1p-16384) == 0x1.c000000000000004p-16383",  FUNC(fma) (0x1.4p-16382L, 0x1.0000000000000004p-1L, 0x1p-16384L), 0x1.c000000000000004p-16383L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (-0x1.4p-16382, 0x1.0000000000000004p-1, -0x1p-16384) == -0x1.c000000000000004p-16383",  FUNC(fma) (-0x1.4p-16382L, 0x1.0000000000000004p-1L, -0x1p-16384L), -0x1.c000000000000004p-16383L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1.fffffffffffffff8p-16382, 0x1.0000000000000002p-1, 0x1p-16445) == 0x1p-16382",  FUNC(fma) (0x1.fffffffffffffff8p-16382L, 0x1.0000000000000002p-1L, 0x1p-16445L), 0x1p-16382L, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma (-0x1.fffffffffffffff8p-16382, 0x1.0000000000000002p-1, -0x1p-16445) == -0x1p-16382",  FUNC(fma) (-0x1.fffffffffffffff8p-16382L, 0x1.0000000000000002p-1L, -0x1p-16445L), -0x1p-16382L, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma (0x1p-16445, 0x1p-1, 0x0.fffffffffffffffep-16382) == 0x1p-16382",  FUNC(fma) (0x1p-16445L, 0x1p-1L, 0x0.fffffffffffffffep-16382L), 0x1p-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (-0x1p-16445, 0x1p-1, -0x0.fffffffffffffffep-16382) == -0x1p-16382",  FUNC(fma) (-0x1p-16445L, 0x1p-1L, -0x0.fffffffffffffffep-16382L), -0x1p-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1p-16445, 0x1.1p-1, 0x0.fffffffffffffffep-16382) == 0x1p-16382",  FUNC(fma) (0x1p-16445L, 0x1.1p-1L, 0x0.fffffffffffffffep-16382L), 0x1p-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (-0x1p-16445, 0x1.1p-1, -0x0.fffffffffffffffep-16382) == -0x1p-16382",  FUNC(fma) (-0x1p-16445L, 0x1.1p-1L, -0x0.fffffffffffffffep-16382L), -0x1p-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1p-16445, 0x1p-16445, 0x1p16383) == 0x1p16383",  FUNC(fma) (0x1p-16445L, 0x1p-16445L, 0x1p16383L), 0x1p16383L, 0, 0, 0);
  check_float ("fma (0x1p-16445, -0x1p-16445, 0x1p16383) == 0x1p16383",  FUNC(fma) (0x1p-16445L, -0x1p-16445L, 0x1p16383L), 0x1p16383L, 0, 0, 0);
  check_float ("fma (0x1p-16445, 0x1p-16445, -0x1p16383) == -0x1p16383",  FUNC(fma) (0x1p-16445L, 0x1p-16445L, -0x1p16383L), -0x1p16383L, 0, 0, 0);
  check_float ("fma (0x1p-16445, -0x1p-16445, -0x1p16383) == -0x1p16383",  FUNC(fma) (0x1p-16445L, -0x1p-16445L, -0x1p16383L), -0x1p16383L, 0, 0, 0);
  check_float ("fma (0x1p-16445, 0x1p-16445, 0x1p-16382) == 0x1p-16382",  FUNC(fma) (0x1p-16445L, 0x1p-16445L, 0x1p-16382L), 0x1p-16382L, 0, 0, 0);
  check_float ("fma (0x1p-16445, -0x1p-16445, 0x1p-16382) == 0x1p-16382",  FUNC(fma) (0x1p-16445L, -0x1p-16445L, 0x1p-16382L), 0x1p-16382L, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma (0x1p-16445, 0x1p-16445, -0x1p-16382) == -0x1p-16382",  FUNC(fma) (0x1p-16445L, 0x1p-16445L, -0x1p-16382L), -0x1p-16382L, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma (0x1p-16445, -0x1p-16445, -0x1p-16382) == -0x1p-16382",  FUNC(fma) (0x1p-16445L, -0x1p-16445L, -0x1p-16382L), -0x1p-16382L, 0, 0, 0);
  check_float ("fma (0x1p-16445, 0x1p-16445, 0x0.fffffffffffffffep-16382) == 0x0.fffffffffffffffep-16382",  FUNC(fma) (0x1p-16445L, 0x1p-16445L, 0x0.fffffffffffffffep-16382L), 0x0.fffffffffffffffep-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1p-16445, -0x1p-16445, 0x0.fffffffffffffffep-16382) == 0x0.fffffffffffffffep-16382",  FUNC(fma) (0x1p-16445L, -0x1p-16445L, 0x0.fffffffffffffffep-16382L), 0x0.fffffffffffffffep-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1p-16445, 0x1p-16445, -0x0.fffffffffffffffep-16382) == -0x0.fffffffffffffffep-16382",  FUNC(fma) (0x1p-16445L, 0x1p-16445L, -0x0.fffffffffffffffep-16382L), -0x0.fffffffffffffffep-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1p-16445, -0x1p-16445, -0x0.fffffffffffffffep-16382) == -0x0.fffffffffffffffep-16382",  FUNC(fma) (0x1p-16445L, -0x1p-16445L, -0x0.fffffffffffffffep-16382L), -0x0.fffffffffffffffep-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1p-16445, 0x1p-16445, 0x1p-16445) == 0x1p-16445",  FUNC(fma) (0x1p-16445L, 0x1p-16445L, 0x1p-16445L), 0x1p-16445L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1p-16445, -0x1p-16445, 0x1p-16445) == 0x1p-16445",  FUNC(fma) (0x1p-16445L, -0x1p-16445L, 0x1p-16445L), 0x1p-16445L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1p-16445, 0x1p-16445, -0x1p-16445) == -0x1p-16445",  FUNC(fma) (0x1p-16445L, 0x1p-16445L, -0x1p-16445L), -0x1p-16445L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1p-16445, -0x1p-16445, -0x1p-16445) == -0x1p-16445",  FUNC(fma) (0x1p-16445L, -0x1p-16445L, -0x1p-16445L), -0x1p-16445L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x0.ffffffffffffffffp0, 0x0.ffffffffffffffffp0, -0x0.fffffffffffffffep0) == 0x1p-128",  FUNC(fma) (0x0.ffffffffffffffffp0L, 0x0.ffffffffffffffffp0L, -0x0.fffffffffffffffep0L), 0x1p-128L, 0, 0, 0);
  check_float ("fma (0x0.ffffffffffffffffp0, -0x0.ffffffffffffffffp0, 0x0.fffffffffffffffep0) == -0x1p-128",  FUNC(fma) (0x0.ffffffffffffffffp0L, -0x0.ffffffffffffffffp0L, 0x0.fffffffffffffffep0L), -0x1p-128L, 0, 0, 0);
  check_float ("fma (-0x0.ffffffffffffffffp0, 0x0.ffffffffffffffffp0, 0x0.fffffffffffffffep0) == -0x1p-128",  FUNC(fma) (-0x0.ffffffffffffffffp0L, 0x0.ffffffffffffffffp0L, 0x0.fffffffffffffffep0L), -0x1p-128L, 0, 0, 0);
  check_float ("fma (-0x0.ffffffffffffffffp0, -0x0.ffffffffffffffffp0, -0x0.fffffffffffffffep0) == 0x1p-128",  FUNC(fma) (-0x0.ffffffffffffffffp0L, -0x0.ffffffffffffffffp0L, -0x0.fffffffffffffffep0L), 0x1p-128L, 0, 0, 0);
  check_float ("fma (0x1.0000000000000002p-16382, 0x1.0000000000000002p-66, 0x1p16383) == 0x1p16383",  FUNC(fma) (0x1.0000000000000002p-16382L, 0x1.0000000000000002p-66L, 0x1p16383L), 0x1p16383L, 0, 0, 0);
  check_float ("fma (0x1.0000000000000002p-16382, -0x1.0000000000000002p-66, 0x1p16383) == 0x1p16383",  FUNC(fma) (0x1.0000000000000002p-16382L, -0x1.0000000000000002p-66L, 0x1p16383L), 0x1p16383L, 0, 0, 0);
  check_float ("fma (0x1.0000000000000002p-16382, 0x1.0000000000000002p-66, -0x1p16383) == -0x1p16383",  FUNC(fma) (0x1.0000000000000002p-16382L, 0x1.0000000000000002p-66L, -0x1p16383L), -0x1p16383L, 0, 0, 0);
  check_float ("fma (0x1.0000000000000002p-16382, -0x1.0000000000000002p-66, -0x1p16383) == -0x1p16383",  FUNC(fma) (0x1.0000000000000002p-16382L, -0x1.0000000000000002p-66L, -0x1p16383L), -0x1p16383L, 0, 0, 0);
  check_float ("fma (0x1.0000000000000002p-16382, 0x1.0000000000000002p-66, 0x1p16319) == 0x1p16319",  FUNC(fma) (0x1.0000000000000002p-16382L, 0x1.0000000000000002p-66L, 0x1p16319L), 0x1p16319L, 0, 0, 0);
  check_float ("fma (0x1.0000000000000002p-16382, -0x1.0000000000000002p-66, 0x1p16319) == 0x1p16319",  FUNC(fma) (0x1.0000000000000002p-16382L, -0x1.0000000000000002p-66L, 0x1p16319L), 0x1p16319L, 0, 0, 0);
  check_float ("fma (0x1.0000000000000002p-16382, 0x1.0000000000000002p-66, -0x1p16319) == -0x1p16319",  FUNC(fma) (0x1.0000000000000002p-16382L, 0x1.0000000000000002p-66L, -0x1p16319L), -0x1p16319L, 0, 0, 0);
  check_float ("fma (0x1.0000000000000002p-16382, -0x1.0000000000000002p-66, -0x1p16319) == -0x1p16319",  FUNC(fma) (0x1.0000000000000002p-16382L, -0x1.0000000000000002p-66L, -0x1p16319L), -0x1p16319L, 0, 0, 0);
#endif
#if defined (TEST_LDOUBLE) && LDBL_MANT_DIG == 113
  check_float ("fma (0x1.bb2de33e02ccbbfa6e245a7c1f71p-2584, -0x1.6b500daf0580d987f1bc0cadfcddp-13777, 0x1.613cd91d9fed34b33820e5ab9d8dp-16378) == -0x1.3a79fb50eb9ce887cffa0f09bd9fp-16360",  FUNC(fma) (0x1.bb2de33e02ccbbfa6e245a7c1f71p-2584L, -0x1.6b500daf0580d987f1bc0cadfcddp-13777L, 0x1.613cd91d9fed34b33820e5ab9d8dp-16378L), -0x1.3a79fb50eb9ce887cffa0f09bd9fp-16360L, 0, 0, 0);
  check_float ("fma (-0x1.f949b880cacb0f0c61540105321dp-5954, -0x1.3876cec84b4140f3bd6198731b7ep-10525, -0x0.a5dc1c6cfbc498c54fb0b504bf19p-16382) == -0x0.a5dc1c6cfbc498c54fb0b5038abbp-16382",  FUNC(fma) (-0x1.f949b880cacb0f0c61540105321dp-5954L, -0x1.3876cec84b4140f3bd6198731b7ep-10525L, -0x0.a5dc1c6cfbc498c54fb0b504bf19p-16382L), -0x0.a5dc1c6cfbc498c54fb0b5038abbp-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (-0x1.0000fffffffffp-16221, 0x1.0000001fffff8007fep-239, 0x0.ff87ffffffffffffe000003fffffp-16382) == 0x0.ff87ffffffffffffdffc003bff7fp-16382",  FUNC(fma) (-0x1.0000fffffffffp-16221L, 0x1.0000001fffff8007fep-239L, 0x0.ff87ffffffffffffe000003fffffp-16382L), 0x0.ff87ffffffffffffdffc003bff7fp-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (-0x1.ac79c9376ef447f3827c9e9de008p-2228, -0x1.5ba830022b6139e21fbe7270cad8p-6314, 0x1.e8282b6a26bb6a9daf5c8e73e9f9p-8616) == 0x1.22f14a0253878a730cd1aee373adp-8541",  FUNC(fma) (-0x1.ac79c9376ef447f3827c9e9de008p-2228L, -0x1.5ba830022b6139e21fbe7270cad8p-6314L, 0x1.e8282b6a26bb6a9daf5c8e73e9f9p-8616L), 0x1.22f14a0253878a730cd1aee373adp-8541L, 0, 0, 0);
  check_float ("fma (-0x1.c69749ec574caaa2ab8e97ddb9f3p+2652, 0x1.f34235ff9d095449c29b4831b62dp+3311, 0x1.fbe4302df23354dbd0c4d3cfe606p+5879) == -0x1.bb473bfdfb7a6e18886ce6e57eafp+5964",  FUNC(fma) (-0x1.c69749ec574caaa2ab8e97ddb9f3p+2652L, 0x1.f34235ff9d095449c29b4831b62dp+3311L, 0x1.fbe4302df23354dbd0c4d3cfe606p+5879L), -0x1.bb473bfdfb7a6e18886ce6e57eafp+5964L, 0, 0, 0);
  check_float ("fma (-0x1.ca8835fc6ecfb5398625fc891be5p-1686, 0x1.621e1972bbe2180e5be9dd7d8df5p-7671, -0x1.7d2d21b73b52cf20dec2a83902a4p-9395) == -0x1.3d2322191c9c88bc68a62ab8042cp-9356",  FUNC(fma) (-0x1.ca8835fc6ecfb5398625fc891be5p-1686L, 0x1.621e1972bbe2180e5be9dd7d8df5p-7671L, -0x1.7d2d21b73b52cf20dec2a83902a4p-9395L), -0x1.3d2322191c9c88bc68a62ab8042cp-9356L, 0, 0, 0);
  check_float ("fma (-0x1.55cff679ec49c2541fab41fc843ep-11819, 0x1.e60e9f464f9e8df0509647c7c971p+12325, 0x1.eaa2a7649d765c2f564f7a5beca7p+454) == -0x1.447e29fa7e406a285f4e350fcf86p+507",  FUNC(fma) (-0x1.55cff679ec49c2541fab41fc843ep-11819L, 0x1.e60e9f464f9e8df0509647c7c971p+12325L, 0x1.eaa2a7649d765c2f564f7a5beca7p+454L), -0x1.447e29fa7e406a285f4e350fcf86p+507L, 0, 0, 0);
  check_float ("fma (0x1.f0e7b1454908576f2537d863cf9bp+11432, 0x1.cdce52f09d4ca76e68706f34b5d5p-1417, -0x1.2e986187c70f146235ea2066e486p+9979) == 0x1.c030dad3cc5643f3dd0f5619f661p+10016",  FUNC(fma) (0x1.f0e7b1454908576f2537d863cf9bp+11432L, 0x1.cdce52f09d4ca76e68706f34b5d5p-1417L, -0x1.2e986187c70f146235ea2066e486p+9979L), 0x1.c030dad3cc5643f3dd0f5619f661p+10016L, 0, 0, 0);
  check_float ("fma (0x1.f102f7da4a57a3a4aab620e29452p-3098, -0x1.cc06a4ff40248f9e2dcc4b6afd84p-11727, 0x1.d512a11126b5ac8ed8973b8580c8p-14849) == -0x1.be8f1cf737ab4d1c31c54f5ec23bp-14824",  FUNC(fma) (0x1.f102f7da4a57a3a4aab620e29452p-3098L, -0x1.cc06a4ff40248f9e2dcc4b6afd84p-11727L, 0x1.d512a11126b5ac8ed8973b8580c8p-14849L), -0x1.be8f1cf737ab4d1c31c54f5ec23bp-14824L, 0, 0, 0);
  check_float ("fma (-0x1.fc47ac7434b993cd8dcb2b431f25p-3816, 0x1.fbc9750da8468852d84558e1db6dp-5773, -0x1.00a98abf783f75c40fe5b7a37d86p-9607) == -0x1.f81917b166f45e763cfcc057e2adp-9588",  FUNC(fma) (-0x1.fc47ac7434b993cd8dcb2b431f25p-3816L, 0x1.fbc9750da8468852d84558e1db6dp-5773L, -0x1.00a98abf783f75c40fe5b7a37d86p-9607L), -0x1.f81917b166f45e763cfcc057e2adp-9588L, 0, 0, 0);
  check_float ("fma (0x1.00000000000007ffffffffffffffp-9045, -0x1.ffffffffffff80000001ffffffffp+4773, -0x1.f8p-4316) == -0x1.00000000000f88000000fffffdffp-4271",  FUNC(fma) (0x1.00000000000007ffffffffffffffp-9045L, -0x1.ffffffffffff80000001ffffffffp+4773L, -0x1.f8p-4316L), -0x1.00000000000f88000000fffffdffp-4271L, 0, 0, 0);
  check_float ("fma (0x1.4e922764c90701d4a2f21d01893dp-8683, -0x1.955a12e2d7c9447c27fa022fc865p+212, -0x1.e9634462eaef96528b90b6944578p-8521) == -0x1.08e1783184a371943d3598e10865p-8470",  FUNC(fma) (0x1.4e922764c90701d4a2f21d01893dp-8683L, -0x1.955a12e2d7c9447c27fa022fc865p+212L, -0x1.e9634462eaef96528b90b6944578p-8521L), -0x1.08e1783184a371943d3598e10865p-8470L, 0, 0, 0);
  check_float ("fma (0x1.801181509c03bdbef10d6165588cp-15131, 0x1.ad86f8e57d3d40bfa8007780af63p-368, -0x1.6e9df0dab1c9f1d7a6043c390741p-15507) == 0x1.417c9b2b15e2ad57dc9e0e920844p-15498",  FUNC(fma) (0x1.801181509c03bdbef10d6165588cp-15131L, 0x1.ad86f8e57d3d40bfa8007780af63p-368L, -0x1.6e9df0dab1c9f1d7a6043c390741p-15507L), 0x1.417c9b2b15e2ad57dc9e0e920844p-15498L, 0, 0, 0);
  check_float ("fma (0x1.4p-16382, 0x1.0000000000000000000000000002p-1, 0x1p-16384) == 0x1.c000000000000000000000000002p-16383",  FUNC(fma) (0x1.4p-16382L, 0x1.0000000000000000000000000002p-1L, 0x1p-16384L), 0x1.c000000000000000000000000002p-16383L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (-0x1.4p-16382, 0x1.0000000000000000000000000002p-1, -0x1p-16384) == -0x1.c000000000000000000000000002p-16383",  FUNC(fma) (-0x1.4p-16382L, 0x1.0000000000000000000000000002p-1L, -0x1p-16384L), -0x1.c000000000000000000000000002p-16383L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1.fffffffffffffffffffffffffffcp-16382, 0x1.0000000000000000000000000001p-1, 0x1p-16494) == 0x1p-16382",  FUNC(fma) (0x1.fffffffffffffffffffffffffffcp-16382L, 0x1.0000000000000000000000000001p-1L, 0x1p-16494L), 0x1p-16382L, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma (-0x1.fffffffffffffffffffffffffffcp-16382, 0x1.0000000000000000000000000001p-1, -0x1p-16494) == -0x1p-16382",  FUNC(fma) (-0x1.fffffffffffffffffffffffffffcp-16382L, 0x1.0000000000000000000000000001p-1L, -0x1p-16494L), -0x1p-16382L, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma (0x1p-16494, 0x1p-1, 0x0.ffffffffffffffffffffffffffffp-16382) == 0x1p-16382",  FUNC(fma) (0x1p-16494L, 0x1p-1L, 0x0.ffffffffffffffffffffffffffffp-16382L), 0x1p-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (-0x1p-16494, 0x1p-1, -0x0.ffffffffffffffffffffffffffffp-16382) == -0x1p-16382",  FUNC(fma) (-0x1p-16494L, 0x1p-1L, -0x0.ffffffffffffffffffffffffffffp-16382L), -0x1p-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1p-16494, 0x1.1p-1, 0x0.ffffffffffffffffffffffffffffp-16382) == 0x1p-16382",  FUNC(fma) (0x1p-16494L, 0x1.1p-1L, 0x0.ffffffffffffffffffffffffffffp-16382L), 0x1p-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (-0x1p-16494, 0x1.1p-1, -0x0.ffffffffffffffffffffffffffffp-16382) == -0x1p-16382",  FUNC(fma) (-0x1p-16494L, 0x1.1p-1L, -0x0.ffffffffffffffffffffffffffffp-16382L), -0x1p-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1p-16494, 0x1p-16494, 0x1p16383) == 0x1p16383",  FUNC(fma) (0x1p-16494L, 0x1p-16494L, 0x1p16383L), 0x1p16383L, 0, 0, 0);
  check_float ("fma (0x1p-16494, -0x1p-16494, 0x1p16383) == 0x1p16383",  FUNC(fma) (0x1p-16494L, -0x1p-16494L, 0x1p16383L), 0x1p16383L, 0, 0, 0);
  check_float ("fma (0x1p-16494, 0x1p-16494, -0x1p16383) == -0x1p16383",  FUNC(fma) (0x1p-16494L, 0x1p-16494L, -0x1p16383L), -0x1p16383L, 0, 0, 0);
  check_float ("fma (0x1p-16494, -0x1p-16494, -0x1p16383) == -0x1p16383",  FUNC(fma) (0x1p-16494L, -0x1p-16494L, -0x1p16383L), -0x1p16383L, 0, 0, 0);
  check_float ("fma (0x1p-16494, 0x1p-16494, 0x1p-16382) == 0x1p-16382",  FUNC(fma) (0x1p-16494L, 0x1p-16494L, 0x1p-16382L), 0x1p-16382L, 0, 0, 0);
  check_float ("fma (0x1p-16494, -0x1p-16494, 0x1p-16382) == 0x1p-16382",  FUNC(fma) (0x1p-16494L, -0x1p-16494L, 0x1p-16382L), 0x1p-16382L, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma (0x1p-16494, 0x1p-16494, -0x1p-16382) == -0x1p-16382",  FUNC(fma) (0x1p-16494L, 0x1p-16494L, -0x1p-16382L), -0x1p-16382L, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma (0x1p-16494, -0x1p-16494, -0x1p-16382) == -0x1p-16382",  FUNC(fma) (0x1p-16494L, -0x1p-16494L, -0x1p-16382L), -0x1p-16382L, 0, 0, 0);
  check_float ("fma (0x1p-16494, 0x1p-16494, 0x0.ffffffffffffffffffffffffffffp-16382) == 0x0.ffffffffffffffffffffffffffffp-16382",  FUNC(fma) (0x1p-16494L, 0x1p-16494L, 0x0.ffffffffffffffffffffffffffffp-16382L), 0x0.ffffffffffffffffffffffffffffp-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1p-16494, -0x1p-16494, 0x0.ffffffffffffffffffffffffffffp-16382) == 0x0.ffffffffffffffffffffffffffffp-16382",  FUNC(fma) (0x1p-16494L, -0x1p-16494L, 0x0.ffffffffffffffffffffffffffffp-16382L), 0x0.ffffffffffffffffffffffffffffp-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1p-16494, 0x1p-16494, -0x0.ffffffffffffffffffffffffffffp-16382) == -0x0.ffffffffffffffffffffffffffffp-16382",  FUNC(fma) (0x1p-16494L, 0x1p-16494L, -0x0.ffffffffffffffffffffffffffffp-16382L), -0x0.ffffffffffffffffffffffffffffp-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1p-16494, -0x1p-16494, -0x0.ffffffffffffffffffffffffffffp-16382) == -0x0.ffffffffffffffffffffffffffffp-16382",  FUNC(fma) (0x1p-16494L, -0x1p-16494L, -0x0.ffffffffffffffffffffffffffffp-16382L), -0x0.ffffffffffffffffffffffffffffp-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1p-16494, 0x1p-16494, 0x1p-16494) == 0x1p-16494",  FUNC(fma) (0x1p-16494L, 0x1p-16494L, 0x1p-16494L), 0x1p-16494L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1p-16494, -0x1p-16494, 0x1p-16494) == 0x1p-16494",  FUNC(fma) (0x1p-16494L, -0x1p-16494L, 0x1p-16494L), 0x1p-16494L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1p-16494, 0x1p-16494, -0x1p-16494) == -0x1p-16494",  FUNC(fma) (0x1p-16494L, 0x1p-16494L, -0x1p-16494L), -0x1p-16494L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x1p-16494, -0x1p-16494, -0x1p-16494) == -0x1p-16494",  FUNC(fma) (0x1p-16494L, -0x1p-16494L, -0x1p-16494L), -0x1p-16494L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma (0x0.ffffffffffffffffffffffffffff8p0, 0x0.ffffffffffffffffffffffffffff8p0, -0x0.ffffffffffffffffffffffffffffp0) == 0x1p-226",  FUNC(fma) (0x0.ffffffffffffffffffffffffffff8p0L, 0x0.ffffffffffffffffffffffffffff8p0L, -0x0.ffffffffffffffffffffffffffffp0L), 0x1p-226L, 0, 0, 0);
  check_float ("fma (0x0.ffffffffffffffffffffffffffff8p0, -0x0.ffffffffffffffffffffffffffff8p0, 0x0.ffffffffffffffffffffffffffffp0) == -0x1p-226",  FUNC(fma) (0x0.ffffffffffffffffffffffffffff8p0L, -0x0.ffffffffffffffffffffffffffff8p0L, 0x0.ffffffffffffffffffffffffffffp0L), -0x1p-226L, 0, 0, 0);
  check_float ("fma (-0x0.ffffffffffffffffffffffffffff8p0, 0x0.ffffffffffffffffffffffffffff8p0, 0x0.ffffffffffffffffffffffffffffp0) == -0x1p-226",  FUNC(fma) (-0x0.ffffffffffffffffffffffffffff8p0L, 0x0.ffffffffffffffffffffffffffff8p0L, 0x0.ffffffffffffffffffffffffffffp0L), -0x1p-226L, 0, 0, 0);
  check_float ("fma (-0x0.ffffffffffffffffffffffffffff8p0, -0x0.ffffffffffffffffffffffffffff8p0, -0x0.ffffffffffffffffffffffffffffp0) == 0x1p-226",  FUNC(fma) (-0x0.ffffffffffffffffffffffffffff8p0L, -0x0.ffffffffffffffffffffffffffff8p0L, -0x0.ffffffffffffffffffffffffffffp0L), 0x1p-226L, 0, 0, 0);
  check_float ("fma (0x1.0000000000000000000000000001p-16382, 0x1.0000000000000000000000000001p-66, 0x1p16383) == 0x1p16383",  FUNC(fma) (0x1.0000000000000000000000000001p-16382L, 0x1.0000000000000000000000000001p-66L, 0x1p16383L), 0x1p16383L, 0, 0, 0);
  check_float ("fma (0x1.0000000000000000000000000001p-16382, -0x1.0000000000000000000000000001p-66, 0x1p16383) == 0x1p16383",  FUNC(fma) (0x1.0000000000000000000000000001p-16382L, -0x1.0000000000000000000000000001p-66L, 0x1p16383L), 0x1p16383L, 0, 0, 0);
  check_float ("fma (0x1.0000000000000000000000000001p-16382, 0x1.0000000000000000000000000001p-66, -0x1p16383) == -0x1p16383",  FUNC(fma) (0x1.0000000000000000000000000001p-16382L, 0x1.0000000000000000000000000001p-66L, -0x1p16383L), -0x1p16383L, 0, 0, 0);
  check_float ("fma (0x1.0000000000000000000000000001p-16382, -0x1.0000000000000000000000000001p-66, -0x1p16383) == -0x1p16383",  FUNC(fma) (0x1.0000000000000000000000000001p-16382L, -0x1.0000000000000000000000000001p-66L, -0x1p16383L), -0x1p16383L, 0, 0, 0);
  check_float ("fma (0x1.0000000000000000000000000001p-16382, 0x1.0000000000000000000000000001p-66, 0x1p16319) == 0x1p16319",  FUNC(fma) (0x1.0000000000000000000000000001p-16382L, 0x1.0000000000000000000000000001p-66L, 0x1p16319L), 0x1p16319L, 0, 0, 0);
  check_float ("fma (0x1.0000000000000000000000000001p-16382, -0x1.0000000000000000000000000001p-66, 0x1p16319) == 0x1p16319",  FUNC(fma) (0x1.0000000000000000000000000001p-16382L, -0x1.0000000000000000000000000001p-66L, 0x1p16319L), 0x1p16319L, 0, 0, 0);
  check_float ("fma (0x1.0000000000000000000000000001p-16382, 0x1.0000000000000000000000000001p-66, -0x1p16319) == -0x1p16319",  FUNC(fma) (0x1.0000000000000000000000000001p-16382L, 0x1.0000000000000000000000000001p-66L, -0x1p16319L), -0x1p16319L, 0, 0, 0);
  check_float ("fma (0x1.0000000000000000000000000001p-16382, -0x1.0000000000000000000000000001p-66, -0x1p16319) == -0x1p16319",  FUNC(fma) (0x1.0000000000000000000000000001p-16382L, -0x1.0000000000000000000000000001p-66L, -0x1p16319L), -0x1p16319L, 0, 0, 0);
#endif

  print_max_error ("fma", 0, 0);
}


static void
fma_test_towardzero (void)
{
  int save_round_mode;
  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_TOWARDZERO))
    {
  check_float ("fma_towardzero (+0, +0, +0) == +0",  FUNC(fma) (plus_zero, plus_zero, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_towardzero (+0, +0, -0) == +0",  FUNC(fma) (plus_zero, plus_zero, minus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_towardzero (+0, -0, +0) == +0",  FUNC(fma) (plus_zero, minus_zero, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_towardzero (+0, -0, -0) == -0",  FUNC(fma) (plus_zero, minus_zero, minus_zero), minus_zero, 0, 0, 0);
  check_float ("fma_towardzero (-0, +0, +0) == +0",  FUNC(fma) (minus_zero, plus_zero, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_towardzero (-0, +0, -0) == -0",  FUNC(fma) (minus_zero, plus_zero, minus_zero), minus_zero, 0, 0, 0);
  check_float ("fma_towardzero (-0, -0, +0) == +0",  FUNC(fma) (minus_zero, minus_zero, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_towardzero (-0, -0, -0) == +0",  FUNC(fma) (minus_zero, minus_zero, minus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_towardzero (1.0, +0, +0) == +0",  FUNC(fma) (1.0, plus_zero, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_towardzero (1.0, +0, -0) == +0",  FUNC(fma) (1.0, plus_zero, minus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_towardzero (1.0, -0, +0) == +0",  FUNC(fma) (1.0, minus_zero, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_towardzero (1.0, -0, -0) == -0",  FUNC(fma) (1.0, minus_zero, minus_zero), minus_zero, 0, 0, 0);
  check_float ("fma_towardzero (-1.0, +0, +0) == +0",  FUNC(fma) (-1.0, plus_zero, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_towardzero (-1.0, +0, -0) == -0",  FUNC(fma) (-1.0, plus_zero, minus_zero), minus_zero, 0, 0, 0);
  check_float ("fma_towardzero (-1.0, -0, +0) == +0",  FUNC(fma) (-1.0, minus_zero, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_towardzero (-1.0, -0, -0) == +0",  FUNC(fma) (-1.0, minus_zero, minus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_towardzero (+0, 1.0, +0) == +0",  FUNC(fma) (plus_zero, 1.0, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_towardzero (+0, 1.0, -0) == +0",  FUNC(fma) (plus_zero, 1.0, minus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_towardzero (+0, -1.0, +0) == +0",  FUNC(fma) (plus_zero, -1.0, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_towardzero (+0, -1.0, -0) == -0",  FUNC(fma) (plus_zero, -1.0, minus_zero), minus_zero, 0, 0, 0);
  check_float ("fma_towardzero (-0, 1.0, +0) == +0",  FUNC(fma) (minus_zero, 1.0, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_towardzero (-0, 1.0, -0) == -0",  FUNC(fma) (minus_zero, 1.0, minus_zero), minus_zero, 0, 0, 0);
  check_float ("fma_towardzero (-0, -1.0, +0) == +0",  FUNC(fma) (minus_zero, -1.0, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_towardzero (-0, -1.0, -0) == +0",  FUNC(fma) (minus_zero, -1.0, minus_zero), plus_zero, 0, 0, 0);

  check_float ("fma_towardzero (1.0, 1.0, -1.0) == +0",  FUNC(fma) (1.0, 1.0, -1.0), plus_zero, 0, 0, 0);
  check_float ("fma_towardzero (1.0, -1.0, 1.0) == +0",  FUNC(fma) (1.0, -1.0, 1.0), plus_zero, 0, 0, 0);
  check_float ("fma_towardzero (-1.0, 1.0, 1.0) == +0",  FUNC(fma) (-1.0, 1.0, 1.0), plus_zero, 0, 0, 0);
  check_float ("fma_towardzero (-1.0, -1.0, -1.0) == +0",  FUNC(fma) (-1.0, -1.0, -1.0), plus_zero, 0, 0, 0);

  check_float ("fma_towardzero (min_value, min_value, +0) == +0",  FUNC(fma) (min_value, min_value, plus_zero), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (min_value, min_value, -0) == +0",  FUNC(fma) (min_value, min_value, minus_zero), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (min_value, -min_value, +0) == -0",  FUNC(fma) (min_value, -min_value, plus_zero), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (min_value, -min_value, -0) == -0",  FUNC(fma) (min_value, -min_value, minus_zero), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (-min_value, min_value, +0) == -0",  FUNC(fma) (-min_value, min_value, plus_zero), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (-min_value, min_value, -0) == -0",  FUNC(fma) (-min_value, min_value, minus_zero), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (-min_value, -min_value, +0) == +0",  FUNC(fma) (-min_value, -min_value, plus_zero), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (-min_value, -min_value, -0) == +0",  FUNC(fma) (-min_value, -min_value, minus_zero), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);

#if !(defined TEST_LDOUBLE && LDBL_MANT_DIG == 106) /* Bug 13304.  */
  check_float ("fma_towardzero (max_value, max_value, min_value) == max_value",  FUNC(fma) (max_value, max_value, min_value), max_value, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("fma_towardzero (max_value, max_value, -min_value) == max_value",  FUNC(fma) (max_value, max_value, -min_value), max_value, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("fma_towardzero (max_value, -max_value, min_value) == -max_value",  FUNC(fma) (max_value, -max_value, min_value), -max_value, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("fma_towardzero (max_value, -max_value, -min_value) == -max_value",  FUNC(fma) (max_value, -max_value, -min_value), -max_value, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("fma_towardzero (-max_value, max_value, min_value) == -max_value",  FUNC(fma) (-max_value, max_value, min_value), -max_value, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("fma_towardzero (-max_value, max_value, -min_value) == -max_value",  FUNC(fma) (-max_value, max_value, -min_value), -max_value, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("fma_towardzero (-max_value, -max_value, min_value) == max_value",  FUNC(fma) (-max_value, -max_value, min_value), max_value, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("fma_towardzero (-max_value, -max_value, -min_value) == max_value",  FUNC(fma) (-max_value, -max_value, -min_value), max_value, 0, 0, OVERFLOW_EXCEPTION);
#endif

#if defined (TEST_FLOAT) && FLT_MANT_DIG == 24
  check_float ("fma_towardzero (0x1.4p-126, 0x1.000004p-1, 0x1p-128) == 0x1.c00004p-127",  FUNC(fma) (0x1.4p-126, 0x1.000004p-1, 0x1p-128), 0x1.c00004p-127, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (-0x1.4p-126, 0x1.000004p-1, -0x1p-128) == -0x1.c00004p-127",  FUNC(fma) (-0x1.4p-126, 0x1.000004p-1, -0x1p-128), -0x1.c00004p-127, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1.fffff8p-126, 0x1.000002p-1, 0x1p-149) == 0x0.fffffep-126",  FUNC(fma) (0x1.fffff8p-126, 0x1.000002p-1, 0x1p-149), 0x0.fffffep-126, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (-0x1.fffff8p-126, 0x1.000002p-1, -0x1p-149) == -0x0.fffffep-126",  FUNC(fma) (-0x1.fffff8p-126, 0x1.000002p-1, -0x1p-149), -0x0.fffffep-126, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-149, 0x1p-1, 0x0.fffffep-126) == 0x0.fffffep-126",  FUNC(fma) (0x1p-149, 0x1p-1, 0x0.fffffep-126), 0x0.fffffep-126, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (-0x1p-149, 0x1p-1, -0x0.fffffep-126) == -0x0.fffffep-126",  FUNC(fma) (-0x1p-149, 0x1p-1, -0x0.fffffep-126), -0x0.fffffep-126, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-149, 0x1.1p-1, 0x0.fffffep-126) == 0x0.fffffep-126",  FUNC(fma) (0x1p-149, 0x1.1p-1, 0x0.fffffep-126), 0x0.fffffep-126, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (-0x1p-149, 0x1.1p-1, -0x0.fffffep-126) == -0x0.fffffep-126",  FUNC(fma) (-0x1p-149, 0x1.1p-1, -0x0.fffffep-126), -0x0.fffffep-126, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-149, 0x1p-149, 0x1p127) == 0x1p127",  FUNC(fma) (0x1p-149, 0x1p-149, 0x1p127), 0x1p127, 0, 0, 0);
  check_float ("fma_towardzero (0x1p-149, -0x1p-149, 0x1p127) == 0x0.ffffffp127",  FUNC(fma) (0x1p-149, -0x1p-149, 0x1p127), 0x0.ffffffp127, 0, 0, 0);
  check_float ("fma_towardzero (0x1p-149, 0x1p-149, -0x1p127) == -0x0.ffffffp127",  FUNC(fma) (0x1p-149, 0x1p-149, -0x1p127), -0x0.ffffffp127, 0, 0, 0);
  check_float ("fma_towardzero (0x1p-149, -0x1p-149, -0x1p127) == -0x1p127",  FUNC(fma) (0x1p-149, -0x1p-149, -0x1p127), -0x1p127, 0, 0, 0);
  check_float ("fma_towardzero (0x1p-149, 0x1p-149, 0x1p-126) == 0x1p-126",  FUNC(fma) (0x1p-149, 0x1p-149, 0x1p-126), 0x1p-126, 0, 0, 0);
  check_float ("fma_towardzero (0x1p-149, -0x1p-149, 0x1p-126) == 0x0.fffffep-126",  FUNC(fma) (0x1p-149, -0x1p-149, 0x1p-126), 0x0.fffffep-126, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-149, 0x1p-149, -0x1p-126) == -0x0.fffffep-126",  FUNC(fma) (0x1p-149, 0x1p-149, -0x1p-126), -0x0.fffffep-126, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-149, -0x1p-149, -0x1p-126) == -0x1p-126",  FUNC(fma) (0x1p-149, -0x1p-149, -0x1p-126), -0x1p-126, 0, 0, 0);
  check_float ("fma_towardzero (0x1p-149, 0x1p-149, 0x0.fffffep-126) == 0x0.fffffep-126",  FUNC(fma) (0x1p-149, 0x1p-149, 0x0.fffffep-126), 0x0.fffffep-126, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-149, -0x1p-149, 0x0.fffffep-126) == 0x0.fffffcp-126",  FUNC(fma) (0x1p-149, -0x1p-149, 0x0.fffffep-126), 0x0.fffffcp-126, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-149, 0x1p-149, -0x0.fffffep-126) == -0x0.fffffcp-126",  FUNC(fma) (0x1p-149, 0x1p-149, -0x0.fffffep-126), -0x0.fffffcp-126, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-149, -0x1p-149, -0x0.fffffep-126) == -0x0.fffffep-126",  FUNC(fma) (0x1p-149, -0x1p-149, -0x0.fffffep-126), -0x0.fffffep-126, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-149, 0x1p-149, 0x1p-149) == 0x1p-149",  FUNC(fma) (0x1p-149, 0x1p-149, 0x1p-149), 0x1p-149, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-149, -0x1p-149, 0x1p-149) == +0",  FUNC(fma) (0x1p-149, -0x1p-149, 0x1p-149), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-149, 0x1p-149, -0x1p-149) == -0",  FUNC(fma) (0x1p-149, 0x1p-149, -0x1p-149), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-149, -0x1p-149, -0x1p-149) == -0x1p-149",  FUNC(fma) (0x1p-149, -0x1p-149, -0x1p-149), -0x1p-149, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x0.fffp0, 0x0.fffp0, -0x0.ffep0) == 0x1p-24",  FUNC(fma) (0x0.fffp0, 0x0.fffp0, -0x0.ffep0), 0x1p-24, 0, 0, 0);
  check_float ("fma_towardzero (0x0.fffp0, -0x0.fffp0, 0x0.ffep0) == -0x1p-24",  FUNC(fma) (0x0.fffp0, -0x0.fffp0, 0x0.ffep0), -0x1p-24, 0, 0, 0);
  check_float ("fma_towardzero (-0x0.fffp0, 0x0.fffp0, 0x0.ffep0) == -0x1p-24",  FUNC(fma) (-0x0.fffp0, 0x0.fffp0, 0x0.ffep0), -0x1p-24, 0, 0, 0);
  check_float ("fma_towardzero (-0x0.fffp0, -0x0.fffp0, -0x0.ffep0) == 0x1p-24",  FUNC(fma) (-0x0.fffp0, -0x0.fffp0, -0x0.ffep0), 0x1p-24, 0, 0, 0);
  check_float ("fma_towardzero (0x1.000002p-126, 0x1.000002p-26, 0x1p127) == 0x1p127",  FUNC(fma) (0x1.000002p-126, 0x1.000002p-26, 0x1p127), 0x1p127, 0, 0, 0);
  check_float ("fma_towardzero (0x1.000002p-126, -0x1.000002p-26, 0x1p127) == 0x0.ffffffp127",  FUNC(fma) (0x1.000002p-126, -0x1.000002p-26, 0x1p127), 0x0.ffffffp127, 0, 0, 0);
  check_float ("fma_towardzero (0x1.000002p-126, 0x1.000002p-26, -0x1p127) == -0x0.ffffffp127",  FUNC(fma) (0x1.000002p-126, 0x1.000002p-26, -0x1p127), -0x0.ffffffp127, 0, 0, 0);
  check_float ("fma_towardzero (0x1.000002p-126, -0x1.000002p-26, -0x1p127) == -0x1p127",  FUNC(fma) (0x1.000002p-126, -0x1.000002p-26, -0x1p127), -0x1p127, 0, 0, 0);
  check_float ("fma_towardzero (0x1.000002p-126, 0x1.000002p-26, 0x1p103) == 0x1p103",  FUNC(fma) (0x1.000002p-126, 0x1.000002p-26, 0x1p103), 0x1p103, 0, 0, 0);
  check_float ("fma_towardzero (0x1.000002p-126, -0x1.000002p-26, 0x1p103) == 0x0.ffffffp103",  FUNC(fma) (0x1.000002p-126, -0x1.000002p-26, 0x1p103), 0x0.ffffffp103, 0, 0, 0);
  check_float ("fma_towardzero (0x1.000002p-126, 0x1.000002p-26, -0x1p103) == -0x0.ffffffp103",  FUNC(fma) (0x1.000002p-126, 0x1.000002p-26, -0x1p103), -0x0.ffffffp103, 0, 0, 0);
  check_float ("fma_towardzero (0x1.000002p-126, -0x1.000002p-26, -0x1p103) == -0x1p103",  FUNC(fma) (0x1.000002p-126, -0x1.000002p-26, -0x1p103), -0x1p103, 0, 0, 0);
#endif
#if defined (TEST_DOUBLE) && DBL_MANT_DIG == 53
  check_float ("fma_towardzero (0x1.4p-1022, 0x1.0000000000002p-1, 0x1p-1024) == 0x1.c000000000002p-1023",  FUNC(fma) (0x1.4p-1022, 0x1.0000000000002p-1, 0x1p-1024), 0x1.c000000000002p-1023, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (-0x1.4p-1022, 0x1.0000000000002p-1, -0x1p-1024) == -0x1.c000000000002p-1023",  FUNC(fma) (-0x1.4p-1022, 0x1.0000000000002p-1, -0x1p-1024), -0x1.c000000000002p-1023, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1.ffffffffffffcp-1022, 0x1.0000000000001p-1, 0x1p-1074) == 0x0.fffffffffffffp-1022",  FUNC(fma) (0x1.ffffffffffffcp-1022, 0x1.0000000000001p-1, 0x1p-1074), 0x0.fffffffffffffp-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (-0x1.ffffffffffffcp-1022, 0x1.0000000000001p-1, -0x1p-1074) == -0x0.fffffffffffffp-1022",  FUNC(fma) (-0x1.ffffffffffffcp-1022, 0x1.0000000000001p-1, -0x1p-1074), -0x0.fffffffffffffp-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-1074, 0x1p-1, 0x0.fffffffffffffp-1022) == 0x0.fffffffffffffp-1022",  FUNC(fma) (0x1p-1074, 0x1p-1, 0x0.fffffffffffffp-1022), 0x0.fffffffffffffp-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (-0x1p-1074, 0x1p-1, -0x0.fffffffffffffp-1022) == -0x0.fffffffffffffp-1022",  FUNC(fma) (-0x1p-1074, 0x1p-1, -0x0.fffffffffffffp-1022), -0x0.fffffffffffffp-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-1074, 0x1.1p-1, 0x0.fffffffffffffp-1022) == 0x0.fffffffffffffp-1022",  FUNC(fma) (0x1p-1074, 0x1.1p-1, 0x0.fffffffffffffp-1022), 0x0.fffffffffffffp-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (-0x1p-1074, 0x1.1p-1, -0x0.fffffffffffffp-1022) == -0x0.fffffffffffffp-1022",  FUNC(fma) (-0x1p-1074, 0x1.1p-1, -0x0.fffffffffffffp-1022), -0x0.fffffffffffffp-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-1074, 0x1p-1074, 0x1p1023) == 0x1p1023",  FUNC(fma) (0x1p-1074, 0x1p-1074, 0x1p1023), 0x1p1023, 0, 0, 0);
  check_float ("fma_towardzero (0x1p-1074, -0x1p-1074, 0x1p1023) == 0x0.fffffffffffff8p1023",  FUNC(fma) (0x1p-1074, -0x1p-1074, 0x1p1023), 0x0.fffffffffffff8p1023, 0, 0, 0);
  check_float ("fma_towardzero (0x1p-1074, 0x1p-1074, -0x1p1023) == -0x0.fffffffffffff8p1023",  FUNC(fma) (0x1p-1074, 0x1p-1074, -0x1p1023), -0x0.fffffffffffff8p1023, 0, 0, 0);
  check_float ("fma_towardzero (0x1p-1074, -0x1p-1074, -0x1p1023) == -0x1p1023",  FUNC(fma) (0x1p-1074, -0x1p-1074, -0x1p1023), -0x1p1023, 0, 0, 0);
  check_float ("fma_towardzero (0x1p-1074, 0x1p-1074, 0x1p-1022) == 0x1p-1022",  FUNC(fma) (0x1p-1074, 0x1p-1074, 0x1p-1022), 0x1p-1022, 0, 0, 0);
  check_float ("fma_towardzero (0x1p-1074, -0x1p-1074, 0x1p-1022) == 0x0.fffffffffffffp-1022",  FUNC(fma) (0x1p-1074, -0x1p-1074, 0x1p-1022), 0x0.fffffffffffffp-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-1074, 0x1p-1074, -0x1p-1022) == -0x0.fffffffffffffp-1022",  FUNC(fma) (0x1p-1074, 0x1p-1074, -0x1p-1022), -0x0.fffffffffffffp-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-1074, -0x1p-1074, -0x1p-1022) == -0x1p-1022",  FUNC(fma) (0x1p-1074, -0x1p-1074, -0x1p-1022), -0x1p-1022, 0, 0, 0);
  check_float ("fma_towardzero (0x1p-1074, 0x1p-1074, 0x0.fffffffffffffp-1022) == 0x0.fffffffffffffp-1022",  FUNC(fma) (0x1p-1074, 0x1p-1074, 0x0.fffffffffffffp-1022), 0x0.fffffffffffffp-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-1074, -0x1p-1074, 0x0.fffffffffffffp-1022) == 0x0.ffffffffffffep-1022",  FUNC(fma) (0x1p-1074, -0x1p-1074, 0x0.fffffffffffffp-1022), 0x0.ffffffffffffep-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-1074, 0x1p-1074, -0x0.fffffffffffffp-1022) == -0x0.ffffffffffffep-1022",  FUNC(fma) (0x1p-1074, 0x1p-1074, -0x0.fffffffffffffp-1022), -0x0.ffffffffffffep-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-1074, -0x1p-1074, -0x0.fffffffffffffp-1022) == -0x0.fffffffffffffp-1022",  FUNC(fma) (0x1p-1074, -0x1p-1074, -0x0.fffffffffffffp-1022), -0x0.fffffffffffffp-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-1074, 0x1p-1074, 0x1p-1074) == 0x1p-1074",  FUNC(fma) (0x1p-1074, 0x1p-1074, 0x1p-1074), 0x1p-1074, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-1074, -0x1p-1074, 0x1p-1074) == +0",  FUNC(fma) (0x1p-1074, -0x1p-1074, 0x1p-1074), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-1074, 0x1p-1074, -0x1p-1074) == -0",  FUNC(fma) (0x1p-1074, 0x1p-1074, -0x1p-1074), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-1074, -0x1p-1074, -0x1p-1074) == -0x1p-1074",  FUNC(fma) (0x1p-1074, -0x1p-1074, -0x1p-1074), -0x1p-1074, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x0.fffffffffffff8p0, 0x0.fffffffffffff8p0, -0x0.fffffffffffffp0) == 0x1p-106",  FUNC(fma) (0x0.fffffffffffff8p0, 0x0.fffffffffffff8p0, -0x0.fffffffffffffp0), 0x1p-106, 0, 0, 0);
  check_float ("fma_towardzero (0x0.fffffffffffff8p0, -0x0.fffffffffffff8p0, 0x0.fffffffffffffp0) == -0x1p-106",  FUNC(fma) (0x0.fffffffffffff8p0, -0x0.fffffffffffff8p0, 0x0.fffffffffffffp0), -0x1p-106, 0, 0, 0);
  check_float ("fma_towardzero (-0x0.fffffffffffff8p0, 0x0.fffffffffffff8p0, 0x0.fffffffffffffp0) == -0x1p-106",  FUNC(fma) (-0x0.fffffffffffff8p0, 0x0.fffffffffffff8p0, 0x0.fffffffffffffp0), -0x1p-106, 0, 0, 0);
  check_float ("fma_towardzero (-0x0.fffffffffffff8p0, -0x0.fffffffffffff8p0, -0x0.fffffffffffffp0) == 0x1p-106",  FUNC(fma) (-0x0.fffffffffffff8p0, -0x0.fffffffffffff8p0, -0x0.fffffffffffffp0), 0x1p-106, 0, 0, 0);
  check_float ("fma_towardzero (0x1.0000000000001p-1022, 0x1.0000000000001p-55, 0x1p1023) == 0x1p1023",  FUNC(fma) (0x1.0000000000001p-1022, 0x1.0000000000001p-55, 0x1p1023), 0x1p1023, 0, 0, 0);
  check_float ("fma_towardzero (0x1.0000000000001p-1022, -0x1.0000000000001p-55, 0x1p1023) == 0x0.fffffffffffff8p1023",  FUNC(fma) (0x1.0000000000001p-1022, -0x1.0000000000001p-55, 0x1p1023), 0x0.fffffffffffff8p1023, 0, 0, 0);
  check_float ("fma_towardzero (0x1.0000000000001p-1022, 0x1.0000000000001p-55, -0x1p1023) == -0x0.fffffffffffff8p1023",  FUNC(fma) (0x1.0000000000001p-1022, 0x1.0000000000001p-55, -0x1p1023), -0x0.fffffffffffff8p1023, 0, 0, 0);
  check_float ("fma_towardzero (0x1.0000000000001p-1022, -0x1.0000000000001p-55, -0x1p1023) == -0x1p1023",  FUNC(fma) (0x1.0000000000001p-1022, -0x1.0000000000001p-55, -0x1p1023), -0x1p1023, 0, 0, 0);
  check_float ("fma_towardzero (0x1.0000000000001p-1022, 0x1.0000000000001p-55, 0x1p970) == 0x1p970",  FUNC(fma) (0x1.0000000000001p-1022, 0x1.0000000000001p-55, 0x1p970), 0x1p970, 0, 0, 0);
  check_float ("fma_towardzero (0x1.0000000000001p-1022, -0x1.0000000000001p-55, 0x1p970) == 0x0.fffffffffffff8p970",  FUNC(fma) (0x1.0000000000001p-1022, -0x1.0000000000001p-55, 0x1p970), 0x0.fffffffffffff8p970, 0, 0, 0);
  check_float ("fma_towardzero (0x1.0000000000001p-1022, 0x1.0000000000001p-55, -0x1p970) == -0x0.fffffffffffff8p970",  FUNC(fma) (0x1.0000000000001p-1022, 0x1.0000000000001p-55, -0x1p970), -0x0.fffffffffffff8p970, 0, 0, 0);
  check_float ("fma_towardzero (0x1.0000000000001p-1022, -0x1.0000000000001p-55, -0x1p970) == -0x1p970",  FUNC(fma) (0x1.0000000000001p-1022, -0x1.0000000000001p-55, -0x1p970), -0x1p970, 0, 0, 0);
#endif
#if defined (TEST_LDOUBLE) && LDBL_MANT_DIG == 64
  check_float ("fma_towardzero (0x1.4p-16382, 0x1.0000000000000004p-1, 0x1p-16384) == 0x1.c000000000000004p-16383",  FUNC(fma) (0x1.4p-16382L, 0x1.0000000000000004p-1L, 0x1p-16384L), 0x1.c000000000000004p-16383L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (-0x1.4p-16382, 0x1.0000000000000004p-1, -0x1p-16384) == -0x1.c000000000000004p-16383",  FUNC(fma) (-0x1.4p-16382L, 0x1.0000000000000004p-1L, -0x1p-16384L), -0x1.c000000000000004p-16383L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1.fffffffffffffff8p-16382, 0x1.0000000000000002p-1, 0x1p-16445) == 0x0.fffffffffffffffep-16382",  FUNC(fma) (0x1.fffffffffffffff8p-16382L, 0x1.0000000000000002p-1L, 0x1p-16445L), 0x0.fffffffffffffffep-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (-0x1.fffffffffffffff8p-16382, 0x1.0000000000000002p-1, -0x1p-16445) == -0x0.fffffffffffffffep-16382",  FUNC(fma) (-0x1.fffffffffffffff8p-16382L, 0x1.0000000000000002p-1L, -0x1p-16445L), -0x0.fffffffffffffffep-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-16445, 0x1p-1, 0x0.fffffffffffffffep-16382) == 0x0.fffffffffffffffep-16382",  FUNC(fma) (0x1p-16445L, 0x1p-1L, 0x0.fffffffffffffffep-16382L), 0x0.fffffffffffffffep-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (-0x1p-16445, 0x1p-1, -0x0.fffffffffffffffep-16382) == -0x0.fffffffffffffffep-16382",  FUNC(fma) (-0x1p-16445L, 0x1p-1L, -0x0.fffffffffffffffep-16382L), -0x0.fffffffffffffffep-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-16445, 0x1.1p-1, 0x0.fffffffffffffffep-16382) == 0x0.fffffffffffffffep-16382",  FUNC(fma) (0x1p-16445L, 0x1.1p-1L, 0x0.fffffffffffffffep-16382L), 0x0.fffffffffffffffep-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (-0x1p-16445, 0x1.1p-1, -0x0.fffffffffffffffep-16382) == -0x0.fffffffffffffffep-16382",  FUNC(fma) (-0x1p-16445L, 0x1.1p-1L, -0x0.fffffffffffffffep-16382L), -0x0.fffffffffffffffep-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-16445, 0x1p-16445, 0x1p16383) == 0x1p16383",  FUNC(fma) (0x1p-16445L, 0x1p-16445L, 0x1p16383L), 0x1p16383L, 0, 0, 0);
  check_float ("fma_towardzero (0x1p-16445, -0x1p-16445, 0x1p16383) == 0x0.ffffffffffffffffp16383",  FUNC(fma) (0x1p-16445L, -0x1p-16445L, 0x1p16383L), 0x0.ffffffffffffffffp16383L, 0, 0, 0);
  check_float ("fma_towardzero (0x1p-16445, 0x1p-16445, -0x1p16383) == -0x0.ffffffffffffffffp16383",  FUNC(fma) (0x1p-16445L, 0x1p-16445L, -0x1p16383L), -0x0.ffffffffffffffffp16383L, 0, 0, 0);
  check_float ("fma_towardzero (0x1p-16445, -0x1p-16445, -0x1p16383) == -0x1p16383",  FUNC(fma) (0x1p-16445L, -0x1p-16445L, -0x1p16383L), -0x1p16383L, 0, 0, 0);
  check_float ("fma_towardzero (0x1p-16445, 0x1p-16445, 0x1p-16382) == 0x1p-16382",  FUNC(fma) (0x1p-16445L, 0x1p-16445L, 0x1p-16382L), 0x1p-16382L, 0, 0, 0);
  check_float ("fma_towardzero (0x1p-16445, -0x1p-16445, 0x1p-16382) == 0x0.fffffffffffffffep-16382",  FUNC(fma) (0x1p-16445L, -0x1p-16445L, 0x1p-16382L), 0x0.fffffffffffffffep-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-16445, 0x1p-16445, -0x1p-16382) == -0x0.fffffffffffffffep-16382",  FUNC(fma) (0x1p-16445L, 0x1p-16445L, -0x1p-16382L), -0x0.fffffffffffffffep-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-16445, -0x1p-16445, -0x1p-16382) == -0x1p-16382",  FUNC(fma) (0x1p-16445L, -0x1p-16445L, -0x1p-16382L), -0x1p-16382L, 0, 0, 0);
  check_float ("fma_towardzero (0x1p-16445, 0x1p-16445, 0x0.fffffffffffffffep-16382) == 0x0.fffffffffffffffep-16382",  FUNC(fma) (0x1p-16445L, 0x1p-16445L, 0x0.fffffffffffffffep-16382L), 0x0.fffffffffffffffep-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-16445, -0x1p-16445, 0x0.fffffffffffffffep-16382) == 0x0.fffffffffffffffcp-16382",  FUNC(fma) (0x1p-16445L, -0x1p-16445L, 0x0.fffffffffffffffep-16382L), 0x0.fffffffffffffffcp-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-16445, 0x1p-16445, -0x0.fffffffffffffffep-16382) == -0x0.fffffffffffffffcp-16382",  FUNC(fma) (0x1p-16445L, 0x1p-16445L, -0x0.fffffffffffffffep-16382L), -0x0.fffffffffffffffcp-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-16445, -0x1p-16445, -0x0.fffffffffffffffep-16382) == -0x0.fffffffffffffffep-16382",  FUNC(fma) (0x1p-16445L, -0x1p-16445L, -0x0.fffffffffffffffep-16382L), -0x0.fffffffffffffffep-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-16445, 0x1p-16445, 0x1p-16445) == 0x1p-16445",  FUNC(fma) (0x1p-16445L, 0x1p-16445L, 0x1p-16445L), 0x1p-16445L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-16445, -0x1p-16445, 0x1p-16445) == +0",  FUNC(fma) (0x1p-16445L, -0x1p-16445L, 0x1p-16445L), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-16445, 0x1p-16445, -0x1p-16445) == -0",  FUNC(fma) (0x1p-16445L, 0x1p-16445L, -0x1p-16445L), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-16445, -0x1p-16445, -0x1p-16445) == -0x1p-16445",  FUNC(fma) (0x1p-16445L, -0x1p-16445L, -0x1p-16445L), -0x1p-16445L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x0.ffffffffffffffffp0, 0x0.ffffffffffffffffp0, -0x0.fffffffffffffffep0) == 0x1p-128",  FUNC(fma) (0x0.ffffffffffffffffp0L, 0x0.ffffffffffffffffp0L, -0x0.fffffffffffffffep0L), 0x1p-128L, 0, 0, 0);
  check_float ("fma_towardzero (0x0.ffffffffffffffffp0, -0x0.ffffffffffffffffp0, 0x0.fffffffffffffffep0) == -0x1p-128",  FUNC(fma) (0x0.ffffffffffffffffp0L, -0x0.ffffffffffffffffp0L, 0x0.fffffffffffffffep0L), -0x1p-128L, 0, 0, 0);
  check_float ("fma_towardzero (-0x0.ffffffffffffffffp0, 0x0.ffffffffffffffffp0, 0x0.fffffffffffffffep0) == -0x1p-128",  FUNC(fma) (-0x0.ffffffffffffffffp0L, 0x0.ffffffffffffffffp0L, 0x0.fffffffffffffffep0L), -0x1p-128L, 0, 0, 0);
  check_float ("fma_towardzero (-0x0.ffffffffffffffffp0, -0x0.ffffffffffffffffp0, -0x0.fffffffffffffffep0) == 0x1p-128",  FUNC(fma) (-0x0.ffffffffffffffffp0L, -0x0.ffffffffffffffffp0L, -0x0.fffffffffffffffep0L), 0x1p-128L, 0, 0, 0);
  check_float ("fma_towardzero (0x1.0000000000000002p-16382, 0x1.0000000000000002p-66, 0x1p16383) == 0x1p16383",  FUNC(fma) (0x1.0000000000000002p-16382L, 0x1.0000000000000002p-66L, 0x1p16383L), 0x1p16383L, 0, 0, 0);
  check_float ("fma_towardzero (0x1.0000000000000002p-16382, -0x1.0000000000000002p-66, 0x1p16383) == 0x0.ffffffffffffffffp16383",  FUNC(fma) (0x1.0000000000000002p-16382L, -0x1.0000000000000002p-66L, 0x1p16383L), 0x0.ffffffffffffffffp16383L, 0, 0, 0);
  check_float ("fma_towardzero (0x1.0000000000000002p-16382, 0x1.0000000000000002p-66, -0x1p16383) == -0x0.ffffffffffffffffp16383",  FUNC(fma) (0x1.0000000000000002p-16382L, 0x1.0000000000000002p-66L, -0x1p16383L), -0x0.ffffffffffffffffp16383L, 0, 0, 0);
  check_float ("fma_towardzero (0x1.0000000000000002p-16382, -0x1.0000000000000002p-66, -0x1p16383) == -0x1p16383",  FUNC(fma) (0x1.0000000000000002p-16382L, -0x1.0000000000000002p-66L, -0x1p16383L), -0x1p16383L, 0, 0, 0);
  check_float ("fma_towardzero (0x1.0000000000000002p-16382, 0x1.0000000000000002p-66, 0x1p16319) == 0x1p16319",  FUNC(fma) (0x1.0000000000000002p-16382L, 0x1.0000000000000002p-66L, 0x1p16319L), 0x1p16319L, 0, 0, 0);
  check_float ("fma_towardzero (0x1.0000000000000002p-16382, -0x1.0000000000000002p-66, 0x1p16319) == 0x0.ffffffffffffffffp16319",  FUNC(fma) (0x1.0000000000000002p-16382L, -0x1.0000000000000002p-66L, 0x1p16319L), 0x0.ffffffffffffffffp16319L, 0, 0, 0);
  check_float ("fma_towardzero (0x1.0000000000000002p-16382, 0x1.0000000000000002p-66, -0x1p16319) == -0x0.ffffffffffffffffp16319",  FUNC(fma) (0x1.0000000000000002p-16382L, 0x1.0000000000000002p-66L, -0x1p16319L), -0x0.ffffffffffffffffp16319L, 0, 0, 0);
  check_float ("fma_towardzero (0x1.0000000000000002p-16382, -0x1.0000000000000002p-66, -0x1p16319) == -0x1p16319",  FUNC(fma) (0x1.0000000000000002p-16382L, -0x1.0000000000000002p-66L, -0x1p16319L), -0x1p16319L, 0, 0, 0);
#endif
#if defined (TEST_LDOUBLE) && LDBL_MANT_DIG == 113
  check_float ("fma_towardzero (0x1.4p-16382, 0x1.0000000000000000000000000002p-1, 0x1p-16384) == 0x1.c000000000000000000000000002p-16383",  FUNC(fma) (0x1.4p-16382L, 0x1.0000000000000000000000000002p-1L, 0x1p-16384L), 0x1.c000000000000000000000000002p-16383L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (-0x1.4p-16382, 0x1.0000000000000000000000000002p-1, -0x1p-16384) == -0x1.c000000000000000000000000002p-16383",  FUNC(fma) (-0x1.4p-16382L, 0x1.0000000000000000000000000002p-1L, -0x1p-16384L), -0x1.c000000000000000000000000002p-16383L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1.fffffffffffffffffffffffffffcp-16382, 0x1.0000000000000000000000000001p-1, 0x1p-16494) == 0x0.ffffffffffffffffffffffffffffp-16382",  FUNC(fma) (0x1.fffffffffffffffffffffffffffcp-16382L, 0x1.0000000000000000000000000001p-1L, 0x1p-16494L), 0x0.ffffffffffffffffffffffffffffp-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (-0x1.fffffffffffffffffffffffffffcp-16382, 0x1.0000000000000000000000000001p-1, -0x1p-16494) == -0x0.ffffffffffffffffffffffffffffp-16382",  FUNC(fma) (-0x1.fffffffffffffffffffffffffffcp-16382L, 0x1.0000000000000000000000000001p-1L, -0x1p-16494L), -0x0.ffffffffffffffffffffffffffffp-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-16494, 0x1p-1, 0x0.ffffffffffffffffffffffffffffp-16382) == 0x0.ffffffffffffffffffffffffffffp-16382",  FUNC(fma) (0x1p-16494L, 0x1p-1L, 0x0.ffffffffffffffffffffffffffffp-16382L), 0x0.ffffffffffffffffffffffffffffp-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (-0x1p-16494, 0x1p-1, -0x0.ffffffffffffffffffffffffffffp-16382) == -0x0.ffffffffffffffffffffffffffffp-16382",  FUNC(fma) (-0x1p-16494L, 0x1p-1L, -0x0.ffffffffffffffffffffffffffffp-16382L), -0x0.ffffffffffffffffffffffffffffp-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-16494, 0x1.1p-1, 0x0.ffffffffffffffffffffffffffffp-16382) == 0x0.ffffffffffffffffffffffffffffp-16382",  FUNC(fma) (0x1p-16494L, 0x1.1p-1L, 0x0.ffffffffffffffffffffffffffffp-16382L), 0x0.ffffffffffffffffffffffffffffp-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (-0x1p-16494, 0x1.1p-1, -0x0.ffffffffffffffffffffffffffffp-16382) == -0x0.ffffffffffffffffffffffffffffp-16382",  FUNC(fma) (-0x1p-16494L, 0x1.1p-1L, -0x0.ffffffffffffffffffffffffffffp-16382L), -0x0.ffffffffffffffffffffffffffffp-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-16494, 0x1p-16494, 0x1p16383) == 0x1p16383",  FUNC(fma) (0x1p-16494L, 0x1p-16494L, 0x1p16383L), 0x1p16383L, 0, 0, 0);
  check_float ("fma_towardzero (0x1p-16494, -0x1p-16494, 0x1p16383) == 0x0.ffffffffffffffffffffffffffff8p16383",  FUNC(fma) (0x1p-16494L, -0x1p-16494L, 0x1p16383L), 0x0.ffffffffffffffffffffffffffff8p16383L, 0, 0, 0);
  check_float ("fma_towardzero (0x1p-16494, 0x1p-16494, -0x1p16383) == -0x0.ffffffffffffffffffffffffffff8p16383",  FUNC(fma) (0x1p-16494L, 0x1p-16494L, -0x1p16383L), -0x0.ffffffffffffffffffffffffffff8p16383L, 0, 0, 0);
  check_float ("fma_towardzero (0x1p-16494, -0x1p-16494, -0x1p16383) == -0x1p16383",  FUNC(fma) (0x1p-16494L, -0x1p-16494L, -0x1p16383L), -0x1p16383L, 0, 0, 0);
  check_float ("fma_towardzero (0x1p-16494, 0x1p-16494, 0x1p-16382) == 0x1p-16382",  FUNC(fma) (0x1p-16494L, 0x1p-16494L, 0x1p-16382L), 0x1p-16382L, 0, 0, 0);
  check_float ("fma_towardzero (0x1p-16494, -0x1p-16494, 0x1p-16382) == 0x0.ffffffffffffffffffffffffffffp-16382",  FUNC(fma) (0x1p-16494L, -0x1p-16494L, 0x1p-16382L), 0x0.ffffffffffffffffffffffffffffp-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-16494, 0x1p-16494, -0x1p-16382) == -0x0.ffffffffffffffffffffffffffffp-16382",  FUNC(fma) (0x1p-16494L, 0x1p-16494L, -0x1p-16382L), -0x0.ffffffffffffffffffffffffffffp-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-16494, -0x1p-16494, -0x1p-16382) == -0x1p-16382",  FUNC(fma) (0x1p-16494L, -0x1p-16494L, -0x1p-16382L), -0x1p-16382L, 0, 0, 0);
  check_float ("fma_towardzero (0x1p-16494, 0x1p-16494, 0x0.ffffffffffffffffffffffffffffp-16382) == 0x0.ffffffffffffffffffffffffffffp-16382",  FUNC(fma) (0x1p-16494L, 0x1p-16494L, 0x0.ffffffffffffffffffffffffffffp-16382L), 0x0.ffffffffffffffffffffffffffffp-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-16494, -0x1p-16494, 0x0.ffffffffffffffffffffffffffffp-16382) == 0x0.fffffffffffffffffffffffffffep-16382",  FUNC(fma) (0x1p-16494L, -0x1p-16494L, 0x0.ffffffffffffffffffffffffffffp-16382L), 0x0.fffffffffffffffffffffffffffep-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-16494, 0x1p-16494, -0x0.ffffffffffffffffffffffffffffp-16382) == -0x0.fffffffffffffffffffffffffffep-16382",  FUNC(fma) (0x1p-16494L, 0x1p-16494L, -0x0.ffffffffffffffffffffffffffffp-16382L), -0x0.fffffffffffffffffffffffffffep-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-16494, -0x1p-16494, -0x0.ffffffffffffffffffffffffffffp-16382) == -0x0.ffffffffffffffffffffffffffffp-16382",  FUNC(fma) (0x1p-16494L, -0x1p-16494L, -0x0.ffffffffffffffffffffffffffffp-16382L), -0x0.ffffffffffffffffffffffffffffp-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-16494, 0x1p-16494, 0x1p-16494) == 0x1p-16494",  FUNC(fma) (0x1p-16494L, 0x1p-16494L, 0x1p-16494L), 0x1p-16494L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-16494, -0x1p-16494, 0x1p-16494) == +0",  FUNC(fma) (0x1p-16494L, -0x1p-16494L, 0x1p-16494L), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-16494, 0x1p-16494, -0x1p-16494) == -0",  FUNC(fma) (0x1p-16494L, 0x1p-16494L, -0x1p-16494L), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x1p-16494, -0x1p-16494, -0x1p-16494) == -0x1p-16494",  FUNC(fma) (0x1p-16494L, -0x1p-16494L, -0x1p-16494L), -0x1p-16494L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_towardzero (0x0.ffffffffffffffffffffffffffff8p0, 0x0.ffffffffffffffffffffffffffff8p0, -0x0.ffffffffffffffffffffffffffffp0) == 0x1p-226",  FUNC(fma) (0x0.ffffffffffffffffffffffffffff8p0L, 0x0.ffffffffffffffffffffffffffff8p0L, -0x0.ffffffffffffffffffffffffffffp0L), 0x1p-226L, 0, 0, 0);
  check_float ("fma_towardzero (0x0.ffffffffffffffffffffffffffff8p0, -0x0.ffffffffffffffffffffffffffff8p0, 0x0.ffffffffffffffffffffffffffffp0) == -0x1p-226",  FUNC(fma) (0x0.ffffffffffffffffffffffffffff8p0L, -0x0.ffffffffffffffffffffffffffff8p0L, 0x0.ffffffffffffffffffffffffffffp0L), -0x1p-226L, 0, 0, 0);
  check_float ("fma_towardzero (-0x0.ffffffffffffffffffffffffffff8p0, 0x0.ffffffffffffffffffffffffffff8p0, 0x0.ffffffffffffffffffffffffffffp0) == -0x1p-226",  FUNC(fma) (-0x0.ffffffffffffffffffffffffffff8p0L, 0x0.ffffffffffffffffffffffffffff8p0L, 0x0.ffffffffffffffffffffffffffffp0L), -0x1p-226L, 0, 0, 0);
  check_float ("fma_towardzero (-0x0.ffffffffffffffffffffffffffff8p0, -0x0.ffffffffffffffffffffffffffff8p0, -0x0.ffffffffffffffffffffffffffffp0) == 0x1p-226",  FUNC(fma) (-0x0.ffffffffffffffffffffffffffff8p0L, -0x0.ffffffffffffffffffffffffffff8p0L, -0x0.ffffffffffffffffffffffffffffp0L), 0x1p-226L, 0, 0, 0);
  check_float ("fma_towardzero (0x1.0000000000000000000000000001p-16382, 0x1.0000000000000000000000000001p-66, 0x1p16383) == 0x1p16383",  FUNC(fma) (0x1.0000000000000000000000000001p-16382L, 0x1.0000000000000000000000000001p-66L, 0x1p16383L), 0x1p16383L, 0, 0, 0);
  check_float ("fma_towardzero (0x1.0000000000000000000000000001p-16382, -0x1.0000000000000000000000000001p-66, 0x1p16383) == 0x0.ffffffffffffffffffffffffffff8p16383",  FUNC(fma) (0x1.0000000000000000000000000001p-16382L, -0x1.0000000000000000000000000001p-66L, 0x1p16383L), 0x0.ffffffffffffffffffffffffffff8p16383L, 0, 0, 0);
  check_float ("fma_towardzero (0x1.0000000000000000000000000001p-16382, 0x1.0000000000000000000000000001p-66, -0x1p16383) == -0x0.ffffffffffffffffffffffffffff8p16383",  FUNC(fma) (0x1.0000000000000000000000000001p-16382L, 0x1.0000000000000000000000000001p-66L, -0x1p16383L), -0x0.ffffffffffffffffffffffffffff8p16383L, 0, 0, 0);
  check_float ("fma_towardzero (0x1.0000000000000000000000000001p-16382, -0x1.0000000000000000000000000001p-66, -0x1p16383) == -0x1p16383",  FUNC(fma) (0x1.0000000000000000000000000001p-16382L, -0x1.0000000000000000000000000001p-66L, -0x1p16383L), -0x1p16383L, 0, 0, 0);
  check_float ("fma_towardzero (0x1.0000000000000000000000000001p-16382, 0x1.0000000000000000000000000001p-66, 0x1p16319) == 0x1p16319",  FUNC(fma) (0x1.0000000000000000000000000001p-16382L, 0x1.0000000000000000000000000001p-66L, 0x1p16319L), 0x1p16319L, 0, 0, 0);
  check_float ("fma_towardzero (0x1.0000000000000000000000000001p-16382, -0x1.0000000000000000000000000001p-66, 0x1p16319) == 0x0.ffffffffffffffffffffffffffff8p16319",  FUNC(fma) (0x1.0000000000000000000000000001p-16382L, -0x1.0000000000000000000000000001p-66L, 0x1p16319L), 0x0.ffffffffffffffffffffffffffff8p16319L, 0, 0, 0);
  check_float ("fma_towardzero (0x1.0000000000000000000000000001p-16382, 0x1.0000000000000000000000000001p-66, -0x1p16319) == -0x0.ffffffffffffffffffffffffffff8p16319",  FUNC(fma) (0x1.0000000000000000000000000001p-16382L, 0x1.0000000000000000000000000001p-66L, -0x1p16319L), -0x0.ffffffffffffffffffffffffffff8p16319L, 0, 0, 0);
  check_float ("fma_towardzero (0x1.0000000000000000000000000001p-16382, -0x1.0000000000000000000000000001p-66, -0x1p16319) == -0x1p16319",  FUNC(fma) (0x1.0000000000000000000000000001p-16382L, -0x1.0000000000000000000000000001p-66L, -0x1p16319L), -0x1p16319L, 0, 0, 0);
#endif
    }

  fesetround (save_round_mode);

  print_max_error ("fma_towardzero", 0, 0);
}


static void
fma_test_downward (void)
{
  int save_round_mode;
  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_DOWNWARD))
    {
  check_float ("fma_downward (+0, +0, +0) == +0",  FUNC(fma) (plus_zero, plus_zero, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_downward (+0, +0, -0) == -0",  FUNC(fma) (plus_zero, plus_zero, minus_zero), minus_zero, 0, 0, 0);
  check_float ("fma_downward (+0, -0, +0) == -0",  FUNC(fma) (plus_zero, minus_zero, plus_zero), minus_zero, 0, 0, 0);
  check_float ("fma_downward (+0, -0, -0) == -0",  FUNC(fma) (plus_zero, minus_zero, minus_zero), minus_zero, 0, 0, 0);
  check_float ("fma_downward (-0, +0, +0) == -0",  FUNC(fma) (minus_zero, plus_zero, plus_zero), minus_zero, 0, 0, 0);
  check_float ("fma_downward (-0, +0, -0) == -0",  FUNC(fma) (minus_zero, plus_zero, minus_zero), minus_zero, 0, 0, 0);
  check_float ("fma_downward (-0, -0, +0) == +0",  FUNC(fma) (minus_zero, minus_zero, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_downward (-0, -0, -0) == -0",  FUNC(fma) (minus_zero, minus_zero, minus_zero), minus_zero, 0, 0, 0);
  check_float ("fma_downward (1.0, +0, +0) == +0",  FUNC(fma) (1.0, plus_zero, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_downward (1.0, +0, -0) == -0",  FUNC(fma) (1.0, plus_zero, minus_zero), minus_zero, 0, 0, 0);
  check_float ("fma_downward (1.0, -0, +0) == -0",  FUNC(fma) (1.0, minus_zero, plus_zero), minus_zero, 0, 0, 0);
  check_float ("fma_downward (1.0, -0, -0) == -0",  FUNC(fma) (1.0, minus_zero, minus_zero), minus_zero, 0, 0, 0);
  check_float ("fma_downward (-1.0, +0, +0) == -0",  FUNC(fma) (-1.0, plus_zero, plus_zero), minus_zero, 0, 0, 0);
  check_float ("fma_downward (-1.0, +0, -0) == -0",  FUNC(fma) (-1.0, plus_zero, minus_zero), minus_zero, 0, 0, 0);
  check_float ("fma_downward (-1.0, -0, +0) == +0",  FUNC(fma) (-1.0, minus_zero, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_downward (-1.0, -0, -0) == -0",  FUNC(fma) (-1.0, minus_zero, minus_zero), minus_zero, 0, 0, 0);
  check_float ("fma_downward (+0, 1.0, +0) == +0",  FUNC(fma) (plus_zero, 1.0, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_downward (+0, 1.0, -0) == -0",  FUNC(fma) (plus_zero, 1.0, minus_zero), minus_zero, 0, 0, 0);
  check_float ("fma_downward (+0, -1.0, +0) == -0",  FUNC(fma) (plus_zero, -1.0, plus_zero), minus_zero, 0, 0, 0);
  check_float ("fma_downward (+0, -1.0, -0) == -0",  FUNC(fma) (plus_zero, -1.0, minus_zero), minus_zero, 0, 0, 0);
  check_float ("fma_downward (-0, 1.0, +0) == -0",  FUNC(fma) (minus_zero, 1.0, plus_zero), minus_zero, 0, 0, 0);
  check_float ("fma_downward (-0, 1.0, -0) == -0",  FUNC(fma) (minus_zero, 1.0, minus_zero), minus_zero, 0, 0, 0);
  check_float ("fma_downward (-0, -1.0, +0) == +0",  FUNC(fma) (minus_zero, -1.0, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_downward (-0, -1.0, -0) == -0",  FUNC(fma) (minus_zero, -1.0, minus_zero), minus_zero, 0, 0, 0);

  check_float ("fma_downward (1.0, 1.0, -1.0) == -0",  FUNC(fma) (1.0, 1.0, -1.0), minus_zero, 0, 0, 0);
  check_float ("fma_downward (1.0, -1.0, 1.0) == -0",  FUNC(fma) (1.0, -1.0, 1.0), minus_zero, 0, 0, 0);
  check_float ("fma_downward (-1.0, 1.0, 1.0) == -0",  FUNC(fma) (-1.0, 1.0, 1.0), minus_zero, 0, 0, 0);
  check_float ("fma_downward (-1.0, -1.0, -1.0) == -0",  FUNC(fma) (-1.0, -1.0, -1.0), minus_zero, 0, 0, 0);

  check_float ("fma_downward (min_value, min_value, +0) == +0",  FUNC(fma) (min_value, min_value, plus_zero), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (min_value, min_value, -0) == +0",  FUNC(fma) (min_value, min_value, minus_zero), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (min_value, -min_value, +0) == -min_subnorm_value",  FUNC(fma) (min_value, -min_value, plus_zero), -min_subnorm_value, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (min_value, -min_value, -0) == -min_subnorm_value",  FUNC(fma) (min_value, -min_value, minus_zero), -min_subnorm_value, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (-min_value, min_value, +0) == -min_subnorm_value",  FUNC(fma) (-min_value, min_value, plus_zero), -min_subnorm_value, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (-min_value, min_value, -0) == -min_subnorm_value",  FUNC(fma) (-min_value, min_value, minus_zero), -min_subnorm_value, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (-min_value, -min_value, +0) == +0",  FUNC(fma) (-min_value, -min_value, plus_zero), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (-min_value, -min_value, -0) == +0",  FUNC(fma) (-min_value, -min_value, minus_zero), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);

#if !(defined TEST_LDOUBLE && LDBL_MANT_DIG == 106) /* Bug 13304.  */
  check_float ("fma_downward (max_value, max_value, min_value) == max_value",  FUNC(fma) (max_value, max_value, min_value), max_value, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("fma_downward (max_value, max_value, -min_value) == max_value",  FUNC(fma) (max_value, max_value, -min_value), max_value, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("fma_downward (max_value, -max_value, min_value) == -inf",  FUNC(fma) (max_value, -max_value, min_value), minus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("fma_downward (max_value, -max_value, -min_value) == -inf",  FUNC(fma) (max_value, -max_value, -min_value), minus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("fma_downward (-max_value, max_value, min_value) == -inf",  FUNC(fma) (-max_value, max_value, min_value), minus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("fma_downward (-max_value, max_value, -min_value) == -inf",  FUNC(fma) (-max_value, max_value, -min_value), minus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("fma_downward (-max_value, -max_value, min_value) == max_value",  FUNC(fma) (-max_value, -max_value, min_value), max_value, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("fma_downward (-max_value, -max_value, -min_value) == max_value",  FUNC(fma) (-max_value, -max_value, -min_value), max_value, 0, 0, OVERFLOW_EXCEPTION);
#endif

#if defined (TEST_FLOAT) && FLT_MANT_DIG == 24
  check_float ("fma_downward (0x1.4p-126, 0x1.000004p-1, 0x1p-128) == 0x1.c00004p-127",  FUNC(fma) (0x1.4p-126, 0x1.000004p-1, 0x1p-128), 0x1.c00004p-127, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (-0x1.4p-126, 0x1.000004p-1, -0x1p-128) == -0x1.c00008p-127",  FUNC(fma) (-0x1.4p-126, 0x1.000004p-1, -0x1p-128), -0x1.c00008p-127, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1.fffff8p-126, 0x1.000002p-1, 0x1p-149) == 0x0.fffffep-126",  FUNC(fma) (0x1.fffff8p-126, 0x1.000002p-1, 0x1p-149), 0x0.fffffep-126, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (-0x1.fffff8p-126, 0x1.000002p-1, -0x1p-149) == -0x1p-126",  FUNC(fma) (-0x1.fffff8p-126, 0x1.000002p-1, -0x1p-149), -0x1p-126, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma_downward (0x1p-149, 0x1p-1, 0x0.fffffep-126) == 0x0.fffffep-126",  FUNC(fma) (0x1p-149, 0x1p-1, 0x0.fffffep-126), 0x0.fffffep-126, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (-0x1p-149, 0x1p-1, -0x0.fffffep-126) == -0x1p-126",  FUNC(fma) (-0x1p-149, 0x1p-1, -0x0.fffffep-126), -0x1p-126, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1p-149, 0x1.1p-1, 0x0.fffffep-126) == 0x0.fffffep-126",  FUNC(fma) (0x1p-149, 0x1.1p-1, 0x0.fffffep-126), 0x0.fffffep-126, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (-0x1p-149, 0x1.1p-1, -0x0.fffffep-126) == -0x1p-126",  FUNC(fma) (-0x1p-149, 0x1.1p-1, -0x0.fffffep-126), -0x1p-126, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma_downward (0x1p-149, 0x1p-149, 0x1p127) == 0x1p127",  FUNC(fma) (0x1p-149, 0x1p-149, 0x1p127), 0x1p127, 0, 0, 0);
  check_float ("fma_downward (0x1p-149, -0x1p-149, 0x1p127) == 0x0.ffffffp127",  FUNC(fma) (0x1p-149, -0x1p-149, 0x1p127), 0x0.ffffffp127, 0, 0, 0);
  check_float ("fma_downward (0x1p-149, 0x1p-149, -0x1p127) == -0x1p127",  FUNC(fma) (0x1p-149, 0x1p-149, -0x1p127), -0x1p127, 0, 0, 0);
  check_float ("fma_downward (0x1p-149, -0x1p-149, -0x1p127) == -0x1.000002p127",  FUNC(fma) (0x1p-149, -0x1p-149, -0x1p127), -0x1.000002p127, 0, 0, 0);
  check_float ("fma_downward (0x1p-149, 0x1p-149, 0x1p-126) == 0x1p-126",  FUNC(fma) (0x1p-149, 0x1p-149, 0x1p-126), 0x1p-126, 0, 0, 0);
  check_float ("fma_downward (0x1p-149, -0x1p-149, 0x1p-126) == 0x0.fffffep-126",  FUNC(fma) (0x1p-149, -0x1p-149, 0x1p-126), 0x0.fffffep-126, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1p-149, 0x1p-149, -0x1p-126) == -0x1p-126",  FUNC(fma) (0x1p-149, 0x1p-149, -0x1p-126), -0x1p-126, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma_downward (0x1p-149, -0x1p-149, -0x1p-126) == -0x1.000002p-126",  FUNC(fma) (0x1p-149, -0x1p-149, -0x1p-126), -0x1.000002p-126, 0, 0, 0);
  check_float ("fma_downward (0x1p-149, 0x1p-149, 0x0.fffffep-126) == 0x0.fffffep-126",  FUNC(fma) (0x1p-149, 0x1p-149, 0x0.fffffep-126), 0x0.fffffep-126, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1p-149, -0x1p-149, 0x0.fffffep-126) == 0x0.fffffcp-126",  FUNC(fma) (0x1p-149, -0x1p-149, 0x0.fffffep-126), 0x0.fffffcp-126, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1p-149, 0x1p-149, -0x0.fffffep-126) == -0x0.fffffep-126",  FUNC(fma) (0x1p-149, 0x1p-149, -0x0.fffffep-126), -0x0.fffffep-126, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1p-149, -0x1p-149, -0x0.fffffep-126) == -0x1p-126",  FUNC(fma) (0x1p-149, -0x1p-149, -0x0.fffffep-126), -0x1p-126, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1p-149, 0x1p-149, 0x1p-149) == 0x1p-149",  FUNC(fma) (0x1p-149, 0x1p-149, 0x1p-149), 0x1p-149, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1p-149, -0x1p-149, 0x1p-149) == +0",  FUNC(fma) (0x1p-149, -0x1p-149, 0x1p-149), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1p-149, 0x1p-149, -0x1p-149) == -0x1p-149",  FUNC(fma) (0x1p-149, 0x1p-149, -0x1p-149), -0x1p-149, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1p-149, -0x1p-149, -0x1p-149) == -0x1p-148",  FUNC(fma) (0x1p-149, -0x1p-149, -0x1p-149), -0x1p-148, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x0.fffp0, 0x0.fffp0, -0x0.ffep0) == 0x1p-24",  FUNC(fma) (0x0.fffp0, 0x0.fffp0, -0x0.ffep0), 0x1p-24, 0, 0, 0);
  check_float ("fma_downward (0x0.fffp0, -0x0.fffp0, 0x0.ffep0) == -0x1p-24",  FUNC(fma) (0x0.fffp0, -0x0.fffp0, 0x0.ffep0), -0x1p-24, 0, 0, 0);
  check_float ("fma_downward (-0x0.fffp0, 0x0.fffp0, 0x0.ffep0) == -0x1p-24",  FUNC(fma) (-0x0.fffp0, 0x0.fffp0, 0x0.ffep0), -0x1p-24, 0, 0, 0);
  check_float ("fma_downward (-0x0.fffp0, -0x0.fffp0, -0x0.ffep0) == 0x1p-24",  FUNC(fma) (-0x0.fffp0, -0x0.fffp0, -0x0.ffep0), 0x1p-24, 0, 0, 0);
  check_float ("fma_downward (0x1.000002p-126, 0x1.000002p-26, 0x1p127) == 0x1p127",  FUNC(fma) (0x1.000002p-126, 0x1.000002p-26, 0x1p127), 0x1p127, 0, 0, 0);
  check_float ("fma_downward (0x1.000002p-126, -0x1.000002p-26, 0x1p127) == 0x0.ffffffp127",  FUNC(fma) (0x1.000002p-126, -0x1.000002p-26, 0x1p127), 0x0.ffffffp127, 0, 0, 0);
  check_float ("fma_downward (0x1.000002p-126, 0x1.000002p-26, -0x1p127) == -0x1p127",  FUNC(fma) (0x1.000002p-126, 0x1.000002p-26, -0x1p127), -0x1p127, 0, 0, 0);
  check_float ("fma_downward (0x1.000002p-126, -0x1.000002p-26, -0x1p127) == -0x1.000002p127",  FUNC(fma) (0x1.000002p-126, -0x1.000002p-26, -0x1p127), -0x1.000002p127, 0, 0, 0);
  check_float ("fma_downward (0x1.000002p-126, 0x1.000002p-26, 0x1p103) == 0x1p103",  FUNC(fma) (0x1.000002p-126, 0x1.000002p-26, 0x1p103), 0x1p103, 0, 0, 0);
  check_float ("fma_downward (0x1.000002p-126, -0x1.000002p-26, 0x1p103) == 0x0.ffffffp103",  FUNC(fma) (0x1.000002p-126, -0x1.000002p-26, 0x1p103), 0x0.ffffffp103, 0, 0, 0);
  check_float ("fma_downward (0x1.000002p-126, 0x1.000002p-26, -0x1p103) == -0x1p103",  FUNC(fma) (0x1.000002p-126, 0x1.000002p-26, -0x1p103), -0x1p103, 0, 0, 0);
  check_float ("fma_downward (0x1.000002p-126, -0x1.000002p-26, -0x1p103) == -0x1.000002p103",  FUNC(fma) (0x1.000002p-126, -0x1.000002p-26, -0x1p103), -0x1.000002p103, 0, 0, 0);
#endif
#if defined (TEST_DOUBLE) && DBL_MANT_DIG == 53
  check_float ("fma_downward (0x1.4p-1022, 0x1.0000000000002p-1, 0x1p-1024) == 0x1.c000000000002p-1023",  FUNC(fma) (0x1.4p-1022, 0x1.0000000000002p-1, 0x1p-1024), 0x1.c000000000002p-1023, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (-0x1.4p-1022, 0x1.0000000000002p-1, -0x1p-1024) == -0x1.c000000000004p-1023",  FUNC(fma) (-0x1.4p-1022, 0x1.0000000000002p-1, -0x1p-1024), -0x1.c000000000004p-1023, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1.ffffffffffffcp-1022, 0x1.0000000000001p-1, 0x1p-1074) == 0x0.fffffffffffffp-1022",  FUNC(fma) (0x1.ffffffffffffcp-1022, 0x1.0000000000001p-1, 0x1p-1074), 0x0.fffffffffffffp-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (-0x1.ffffffffffffcp-1022, 0x1.0000000000001p-1, -0x1p-1074) == -0x1p-1022",  FUNC(fma) (-0x1.ffffffffffffcp-1022, 0x1.0000000000001p-1, -0x1p-1074), -0x1p-1022, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma_downward (0x1p-1074, 0x1p-1, 0x0.fffffffffffffp-1022) == 0x0.fffffffffffffp-1022",  FUNC(fma) (0x1p-1074, 0x1p-1, 0x0.fffffffffffffp-1022), 0x0.fffffffffffffp-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (-0x1p-1074, 0x1p-1, -0x0.fffffffffffffp-1022) == -0x1p-1022",  FUNC(fma) (-0x1p-1074, 0x1p-1, -0x0.fffffffffffffp-1022), -0x1p-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1p-1074, 0x1.1p-1, 0x0.fffffffffffffp-1022) == 0x0.fffffffffffffp-1022",  FUNC(fma) (0x1p-1074, 0x1.1p-1, 0x0.fffffffffffffp-1022), 0x0.fffffffffffffp-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (-0x1p-1074, 0x1.1p-1, -0x0.fffffffffffffp-1022) == -0x1p-1022",  FUNC(fma) (-0x1p-1074, 0x1.1p-1, -0x0.fffffffffffffp-1022), -0x1p-1022, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma_downward (0x1p-1074, 0x1p-1074, 0x1p1023) == 0x1p1023",  FUNC(fma) (0x1p-1074, 0x1p-1074, 0x1p1023), 0x1p1023, 0, 0, 0);
  check_float ("fma_downward (0x1p-1074, -0x1p-1074, 0x1p1023) == 0x0.fffffffffffff8p1023",  FUNC(fma) (0x1p-1074, -0x1p-1074, 0x1p1023), 0x0.fffffffffffff8p1023, 0, 0, 0);
  check_float ("fma_downward (0x1p-1074, 0x1p-1074, -0x1p1023) == -0x1p1023",  FUNC(fma) (0x1p-1074, 0x1p-1074, -0x1p1023), -0x1p1023, 0, 0, 0);
  check_float ("fma_downward (0x1p-1074, -0x1p-1074, -0x1p1023) == -0x1.0000000000001p1023",  FUNC(fma) (0x1p-1074, -0x1p-1074, -0x1p1023), -0x1.0000000000001p1023, 0, 0, 0);
  check_float ("fma_downward (0x1p-1074, 0x1p-1074, 0x1p-1022) == 0x1p-1022",  FUNC(fma) (0x1p-1074, 0x1p-1074, 0x1p-1022), 0x1p-1022, 0, 0, 0);
  check_float ("fma_downward (0x1p-1074, -0x1p-1074, 0x1p-1022) == 0x0.fffffffffffffp-1022",  FUNC(fma) (0x1p-1074, -0x1p-1074, 0x1p-1022), 0x0.fffffffffffffp-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1p-1074, 0x1p-1074, -0x1p-1022) == -0x1p-1022",  FUNC(fma) (0x1p-1074, 0x1p-1074, -0x1p-1022), -0x1p-1022, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma_downward (0x1p-1074, -0x1p-1074, -0x1p-1022) == -0x1.0000000000001p-1022",  FUNC(fma) (0x1p-1074, -0x1p-1074, -0x1p-1022), -0x1.0000000000001p-1022, 0, 0, 0);
  check_float ("fma_downward (0x1p-1074, 0x1p-1074, 0x0.fffffffffffffp-1022) == 0x0.fffffffffffffp-1022",  FUNC(fma) (0x1p-1074, 0x1p-1074, 0x0.fffffffffffffp-1022), 0x0.fffffffffffffp-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1p-1074, -0x1p-1074, 0x0.fffffffffffffp-1022) == 0x0.ffffffffffffep-1022",  FUNC(fma) (0x1p-1074, -0x1p-1074, 0x0.fffffffffffffp-1022), 0x0.ffffffffffffep-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1p-1074, 0x1p-1074, -0x0.fffffffffffffp-1022) == -0x0.fffffffffffffp-1022",  FUNC(fma) (0x1p-1074, 0x1p-1074, -0x0.fffffffffffffp-1022), -0x0.fffffffffffffp-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1p-1074, -0x1p-1074, -0x0.fffffffffffffp-1022) == -0x1p-1022",  FUNC(fma) (0x1p-1074, -0x1p-1074, -0x0.fffffffffffffp-1022), -0x1p-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1p-1074, 0x1p-1074, 0x1p-1074) == 0x1p-1074",  FUNC(fma) (0x1p-1074, 0x1p-1074, 0x1p-1074), 0x1p-1074, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1p-1074, -0x1p-1074, 0x1p-1074) == +0",  FUNC(fma) (0x1p-1074, -0x1p-1074, 0x1p-1074), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1p-1074, 0x1p-1074, -0x1p-1074) == -0x1p-1074",  FUNC(fma) (0x1p-1074, 0x1p-1074, -0x1p-1074), -0x1p-1074, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1p-1074, -0x1p-1074, -0x1p-1074) == -0x1p-1073",  FUNC(fma) (0x1p-1074, -0x1p-1074, -0x1p-1074), -0x1p-1073, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x0.fffffffffffff8p0, 0x0.fffffffffffff8p0, -0x0.fffffffffffffp0) == 0x1p-106",  FUNC(fma) (0x0.fffffffffffff8p0, 0x0.fffffffffffff8p0, -0x0.fffffffffffffp0), 0x1p-106, 0, 0, 0);
  check_float ("fma_downward (0x0.fffffffffffff8p0, -0x0.fffffffffffff8p0, 0x0.fffffffffffffp0) == -0x1p-106",  FUNC(fma) (0x0.fffffffffffff8p0, -0x0.fffffffffffff8p0, 0x0.fffffffffffffp0), -0x1p-106, 0, 0, 0);
  check_float ("fma_downward (-0x0.fffffffffffff8p0, 0x0.fffffffffffff8p0, 0x0.fffffffffffffp0) == -0x1p-106",  FUNC(fma) (-0x0.fffffffffffff8p0, 0x0.fffffffffffff8p0, 0x0.fffffffffffffp0), -0x1p-106, 0, 0, 0);
  check_float ("fma_downward (-0x0.fffffffffffff8p0, -0x0.fffffffffffff8p0, -0x0.fffffffffffffp0) == 0x1p-106",  FUNC(fma) (-0x0.fffffffffffff8p0, -0x0.fffffffffffff8p0, -0x0.fffffffffffffp0), 0x1p-106, 0, 0, 0);
  check_float ("fma_downward (0x1.0000000000001p-1022, 0x1.0000000000001p-55, 0x1p1023) == 0x1p1023",  FUNC(fma) (0x1.0000000000001p-1022, 0x1.0000000000001p-55, 0x1p1023), 0x1p1023, 0, 0, 0);
  check_float ("fma_downward (0x1.0000000000001p-1022, -0x1.0000000000001p-55, 0x1p1023) == 0x0.fffffffffffff8p1023",  FUNC(fma) (0x1.0000000000001p-1022, -0x1.0000000000001p-55, 0x1p1023), 0x0.fffffffffffff8p1023, 0, 0, 0);
  check_float ("fma_downward (0x1.0000000000001p-1022, 0x1.0000000000001p-55, -0x1p1023) == -0x1p1023",  FUNC(fma) (0x1.0000000000001p-1022, 0x1.0000000000001p-55, -0x1p1023), -0x1p1023, 0, 0, 0);
  check_float ("fma_downward (0x1.0000000000001p-1022, -0x1.0000000000001p-55, -0x1p1023) == -0x1.0000000000001p1023",  FUNC(fma) (0x1.0000000000001p-1022, -0x1.0000000000001p-55, -0x1p1023), -0x1.0000000000001p1023, 0, 0, 0);
  check_float ("fma_downward (0x1.0000000000001p-1022, 0x1.0000000000001p-55, 0x1p970) == 0x1p970",  FUNC(fma) (0x1.0000000000001p-1022, 0x1.0000000000001p-55, 0x1p970), 0x1p970, 0, 0, 0);
  check_float ("fma_downward (0x1.0000000000001p-1022, -0x1.0000000000001p-55, 0x1p970) == 0x0.fffffffffffff8p970",  FUNC(fma) (0x1.0000000000001p-1022, -0x1.0000000000001p-55, 0x1p970), 0x0.fffffffffffff8p970, 0, 0, 0);
  check_float ("fma_downward (0x1.0000000000001p-1022, 0x1.0000000000001p-55, -0x1p970) == -0x1p970",  FUNC(fma) (0x1.0000000000001p-1022, 0x1.0000000000001p-55, -0x1p970), -0x1p970, 0, 0, 0);
  check_float ("fma_downward (0x1.0000000000001p-1022, -0x1.0000000000001p-55, -0x1p970) == -0x1.0000000000001p970",  FUNC(fma) (0x1.0000000000001p-1022, -0x1.0000000000001p-55, -0x1p970), -0x1.0000000000001p970, 0, 0, 0);
#endif
#if defined (TEST_LDOUBLE) && LDBL_MANT_DIG == 64
  check_float ("fma_downward (0x1.4p-16382, 0x1.0000000000000004p-1, 0x1p-16384) == 0x1.c000000000000004p-16383",  FUNC(fma) (0x1.4p-16382L, 0x1.0000000000000004p-1L, 0x1p-16384L), 0x1.c000000000000004p-16383L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (-0x1.4p-16382, 0x1.0000000000000004p-1, -0x1p-16384) == -0x1.c000000000000008p-16383",  FUNC(fma) (-0x1.4p-16382L, 0x1.0000000000000004p-1L, -0x1p-16384L), -0x1.c000000000000008p-16383L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1.fffffffffffffff8p-16382, 0x1.0000000000000002p-1, 0x1p-16445) == 0x0.fffffffffffffffep-16382",  FUNC(fma) (0x1.fffffffffffffff8p-16382L, 0x1.0000000000000002p-1L, 0x1p-16445L), 0x0.fffffffffffffffep-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (-0x1.fffffffffffffff8p-16382, 0x1.0000000000000002p-1, -0x1p-16445) == -0x1p-16382",  FUNC(fma) (-0x1.fffffffffffffff8p-16382L, 0x1.0000000000000002p-1L, -0x1p-16445L), -0x1p-16382L, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma_downward (0x1p-16445, 0x1p-1, 0x0.fffffffffffffffep-16382) == 0x0.fffffffffffffffep-16382",  FUNC(fma) (0x1p-16445L, 0x1p-1L, 0x0.fffffffffffffffep-16382L), 0x0.fffffffffffffffep-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (-0x1p-16445, 0x1p-1, -0x0.fffffffffffffffep-16382) == -0x1p-16382",  FUNC(fma) (-0x1p-16445L, 0x1p-1L, -0x0.fffffffffffffffep-16382L), -0x1p-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1p-16445, 0x1.1p-1, 0x0.fffffffffffffffep-16382) == 0x0.fffffffffffffffep-16382",  FUNC(fma) (0x1p-16445L, 0x1.1p-1L, 0x0.fffffffffffffffep-16382L), 0x0.fffffffffffffffep-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (-0x1p-16445, 0x1.1p-1, -0x0.fffffffffffffffep-16382) == -0x1p-16382",  FUNC(fma) (-0x1p-16445L, 0x1.1p-1L, -0x0.fffffffffffffffep-16382L), -0x1p-16382L, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma_downward (0x1p-16445, 0x1p-16445, 0x1p16383) == 0x1p16383",  FUNC(fma) (0x1p-16445L, 0x1p-16445L, 0x1p16383L), 0x1p16383L, 0, 0, 0);
  check_float ("fma_downward (0x1p-16445, -0x1p-16445, 0x1p16383) == 0x0.ffffffffffffffffp16383",  FUNC(fma) (0x1p-16445L, -0x1p-16445L, 0x1p16383L), 0x0.ffffffffffffffffp16383L, 0, 0, 0);
  check_float ("fma_downward (0x1p-16445, 0x1p-16445, -0x1p16383) == -0x1p16383",  FUNC(fma) (0x1p-16445L, 0x1p-16445L, -0x1p16383L), -0x1p16383L, 0, 0, 0);
  check_float ("fma_downward (0x1p-16445, -0x1p-16445, -0x1p16383) == -0x1.0000000000000002p16383",  FUNC(fma) (0x1p-16445L, -0x1p-16445L, -0x1p16383L), -0x1.0000000000000002p16383L, 0, 0, 0);
  check_float ("fma_downward (0x1p-16445, 0x1p-16445, 0x1p-16382) == 0x1p-16382",  FUNC(fma) (0x1p-16445L, 0x1p-16445L, 0x1p-16382L), 0x1p-16382L, 0, 0, 0);
  check_float ("fma_downward (0x1p-16445, -0x1p-16445, 0x1p-16382) == 0x0.fffffffffffffffep-16382",  FUNC(fma) (0x1p-16445L, -0x1p-16445L, 0x1p-16382L), 0x0.fffffffffffffffep-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1p-16445, 0x1p-16445, -0x1p-16382) == -0x1p-16382",  FUNC(fma) (0x1p-16445L, 0x1p-16445L, -0x1p-16382L), -0x1p-16382L, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma_downward (0x1p-16445, -0x1p-16445, -0x1p-16382) == -0x1.0000000000000002p-16382",  FUNC(fma) (0x1p-16445L, -0x1p-16445L, -0x1p-16382L), -0x1.0000000000000002p-16382L, 0, 0, 0);
  check_float ("fma_downward (0x1p-16445, 0x1p-16445, 0x0.fffffffffffffffep-16382) == 0x0.fffffffffffffffep-16382",  FUNC(fma) (0x1p-16445L, 0x1p-16445L, 0x0.fffffffffffffffep-16382L), 0x0.fffffffffffffffep-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1p-16445, -0x1p-16445, 0x0.fffffffffffffffep-16382) == 0x0.fffffffffffffffcp-16382",  FUNC(fma) (0x1p-16445L, -0x1p-16445L, 0x0.fffffffffffffffep-16382L), 0x0.fffffffffffffffcp-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1p-16445, 0x1p-16445, -0x0.fffffffffffffffep-16382) == -0x0.fffffffffffffffep-16382",  FUNC(fma) (0x1p-16445L, 0x1p-16445L, -0x0.fffffffffffffffep-16382L), -0x0.fffffffffffffffep-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1p-16445, -0x1p-16445, -0x0.fffffffffffffffep-16382) == -0x1p-16382",  FUNC(fma) (0x1p-16445L, -0x1p-16445L, -0x0.fffffffffffffffep-16382L), -0x1p-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1p-16445, 0x1p-16445, 0x1p-16445) == 0x1p-16445",  FUNC(fma) (0x1p-16445L, 0x1p-16445L, 0x1p-16445L), 0x1p-16445L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1p-16445, -0x1p-16445, 0x1p-16445) == +0",  FUNC(fma) (0x1p-16445L, -0x1p-16445L, 0x1p-16445L), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1p-16445, 0x1p-16445, -0x1p-16445) == -0x1p-16445",  FUNC(fma) (0x1p-16445L, 0x1p-16445L, -0x1p-16445L), -0x1p-16445L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1p-16445, -0x1p-16445, -0x1p-16445) == -0x1p-16444",  FUNC(fma) (0x1p-16445L, -0x1p-16445L, -0x1p-16445L), -0x1p-16444L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x0.ffffffffffffffffp0, 0x0.ffffffffffffffffp0, -0x0.fffffffffffffffep0) == 0x1p-128",  FUNC(fma) (0x0.ffffffffffffffffp0L, 0x0.ffffffffffffffffp0L, -0x0.fffffffffffffffep0L), 0x1p-128L, 0, 0, 0);
  check_float ("fma_downward (0x0.ffffffffffffffffp0, -0x0.ffffffffffffffffp0, 0x0.fffffffffffffffep0) == -0x1p-128",  FUNC(fma) (0x0.ffffffffffffffffp0L, -0x0.ffffffffffffffffp0L, 0x0.fffffffffffffffep0L), -0x1p-128L, 0, 0, 0);
  check_float ("fma_downward (-0x0.ffffffffffffffffp0, 0x0.ffffffffffffffffp0, 0x0.fffffffffffffffep0) == -0x1p-128",  FUNC(fma) (-0x0.ffffffffffffffffp0L, 0x0.ffffffffffffffffp0L, 0x0.fffffffffffffffep0L), -0x1p-128L, 0, 0, 0);
  check_float ("fma_downward (-0x0.ffffffffffffffffp0, -0x0.ffffffffffffffffp0, -0x0.fffffffffffffffep0) == 0x1p-128",  FUNC(fma) (-0x0.ffffffffffffffffp0L, -0x0.ffffffffffffffffp0L, -0x0.fffffffffffffffep0L), 0x1p-128L, 0, 0, 0);
  check_float ("fma_downward (0x1.0000000000000002p-16382, 0x1.0000000000000002p-66, 0x1p16383) == 0x1p16383",  FUNC(fma) (0x1.0000000000000002p-16382L, 0x1.0000000000000002p-66L, 0x1p16383L), 0x1p16383L, 0, 0, 0);
  check_float ("fma_downward (0x1.0000000000000002p-16382, -0x1.0000000000000002p-66, 0x1p16383) == 0x0.ffffffffffffffffp16383",  FUNC(fma) (0x1.0000000000000002p-16382L, -0x1.0000000000000002p-66L, 0x1p16383L), 0x0.ffffffffffffffffp16383L, 0, 0, 0);
  check_float ("fma_downward (0x1.0000000000000002p-16382, 0x1.0000000000000002p-66, -0x1p16383) == -0x1p16383",  FUNC(fma) (0x1.0000000000000002p-16382L, 0x1.0000000000000002p-66L, -0x1p16383L), -0x1p16383L, 0, 0, 0);
  check_float ("fma_downward (0x1.0000000000000002p-16382, -0x1.0000000000000002p-66, -0x1p16383) == -0x1.0000000000000002p16383",  FUNC(fma) (0x1.0000000000000002p-16382L, -0x1.0000000000000002p-66L, -0x1p16383L), -0x1.0000000000000002p16383L, 0, 0, 0);
  check_float ("fma_downward (0x1.0000000000000002p-16382, 0x1.0000000000000002p-66, 0x1p16319) == 0x1p16319",  FUNC(fma) (0x1.0000000000000002p-16382L, 0x1.0000000000000002p-66L, 0x1p16319L), 0x1p16319L, 0, 0, 0);
  check_float ("fma_downward (0x1.0000000000000002p-16382, -0x1.0000000000000002p-66, 0x1p16319) == 0x0.ffffffffffffffffp16319",  FUNC(fma) (0x1.0000000000000002p-16382L, -0x1.0000000000000002p-66L, 0x1p16319L), 0x0.ffffffffffffffffp16319L, 0, 0, 0);
  check_float ("fma_downward (0x1.0000000000000002p-16382, 0x1.0000000000000002p-66, -0x1p16319) == -0x1p16319",  FUNC(fma) (0x1.0000000000000002p-16382L, 0x1.0000000000000002p-66L, -0x1p16319L), -0x1p16319L, 0, 0, 0);
  check_float ("fma_downward (0x1.0000000000000002p-16382, -0x1.0000000000000002p-66, -0x1p16319) == -0x1.0000000000000002p16319",  FUNC(fma) (0x1.0000000000000002p-16382L, -0x1.0000000000000002p-66L, -0x1p16319L), -0x1.0000000000000002p16319L, 0, 0, 0);
#endif
#if defined (TEST_LDOUBLE) && LDBL_MANT_DIG == 113
  check_float ("fma_downward (0x1.4p-16382, 0x1.0000000000000000000000000002p-1, 0x1p-16384) == 0x1.c000000000000000000000000002p-16383",  FUNC(fma) (0x1.4p-16382L, 0x1.0000000000000000000000000002p-1L, 0x1p-16384L), 0x1.c000000000000000000000000002p-16383L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (-0x1.4p-16382, 0x1.0000000000000000000000000002p-1, -0x1p-16384) == -0x1.c000000000000000000000000004p-16383",  FUNC(fma) (-0x1.4p-16382L, 0x1.0000000000000000000000000002p-1L, -0x1p-16384L), -0x1.c000000000000000000000000004p-16383L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1.fffffffffffffffffffffffffffcp-16382, 0x1.0000000000000000000000000001p-1, 0x1p-16494) == 0x0.ffffffffffffffffffffffffffffp-16382",  FUNC(fma) (0x1.fffffffffffffffffffffffffffcp-16382L, 0x1.0000000000000000000000000001p-1L, 0x1p-16494L), 0x0.ffffffffffffffffffffffffffffp-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (-0x1.fffffffffffffffffffffffffffcp-16382, 0x1.0000000000000000000000000001p-1, -0x1p-16494) == -0x1p-16382",  FUNC(fma) (-0x1.fffffffffffffffffffffffffffcp-16382L, 0x1.0000000000000000000000000001p-1L, -0x1p-16494L), -0x1p-16382L, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma_downward (0x1p-16494, 0x1p-1, 0x0.ffffffffffffffffffffffffffffp-16382) == 0x0.ffffffffffffffffffffffffffffp-16382",  FUNC(fma) (0x1p-16494L, 0x1p-1L, 0x0.ffffffffffffffffffffffffffffp-16382L), 0x0.ffffffffffffffffffffffffffffp-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (-0x1p-16494, 0x1p-1, -0x0.ffffffffffffffffffffffffffffp-16382) == -0x1p-16382",  FUNC(fma) (-0x1p-16494L, 0x1p-1L, -0x0.ffffffffffffffffffffffffffffp-16382L), -0x1p-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1p-16494, 0x1.1p-1, 0x0.ffffffffffffffffffffffffffffp-16382) == 0x0.ffffffffffffffffffffffffffffp-16382",  FUNC(fma) (0x1p-16494L, 0x1.1p-1L, 0x0.ffffffffffffffffffffffffffffp-16382L), 0x0.ffffffffffffffffffffffffffffp-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (-0x1p-16494, 0x1.1p-1, -0x0.ffffffffffffffffffffffffffffp-16382) == -0x1p-16382",  FUNC(fma) (-0x1p-16494L, 0x1.1p-1L, -0x0.ffffffffffffffffffffffffffffp-16382L), -0x1p-16382L, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma_downward (0x1p-16494, 0x1p-16494, 0x1p16383) == 0x1p16383",  FUNC(fma) (0x1p-16494L, 0x1p-16494L, 0x1p16383L), 0x1p16383L, 0, 0, 0);
  check_float ("fma_downward (0x1p-16494, -0x1p-16494, 0x1p16383) == 0x0.ffffffffffffffffffffffffffff8p16383",  FUNC(fma) (0x1p-16494L, -0x1p-16494L, 0x1p16383L), 0x0.ffffffffffffffffffffffffffff8p16383L, 0, 0, 0);
  check_float ("fma_downward (0x1p-16494, 0x1p-16494, -0x1p16383) == -0x1p16383",  FUNC(fma) (0x1p-16494L, 0x1p-16494L, -0x1p16383L), -0x1p16383L, 0, 0, 0);
  check_float ("fma_downward (0x1p-16494, -0x1p-16494, -0x1p16383) == -0x1.0000000000000000000000000001p16383",  FUNC(fma) (0x1p-16494L, -0x1p-16494L, -0x1p16383L), -0x1.0000000000000000000000000001p16383L, 0, 0, 0);
  check_float ("fma_downward (0x1p-16494, 0x1p-16494, 0x1p-16382) == 0x1p-16382",  FUNC(fma) (0x1p-16494L, 0x1p-16494L, 0x1p-16382L), 0x1p-16382L, 0, 0, 0);
  check_float ("fma_downward (0x1p-16494, -0x1p-16494, 0x1p-16382) == 0x0.ffffffffffffffffffffffffffffp-16382",  FUNC(fma) (0x1p-16494L, -0x1p-16494L, 0x1p-16382L), 0x0.ffffffffffffffffffffffffffffp-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1p-16494, 0x1p-16494, -0x1p-16382) == -0x1p-16382",  FUNC(fma) (0x1p-16494L, 0x1p-16494L, -0x1p-16382L), -0x1p-16382L, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma_downward (0x1p-16494, -0x1p-16494, -0x1p-16382) == -0x1.0000000000000000000000000001p-16382",  FUNC(fma) (0x1p-16494L, -0x1p-16494L, -0x1p-16382L), -0x1.0000000000000000000000000001p-16382L, 0, 0, 0);
  check_float ("fma_downward (0x1p-16494, 0x1p-16494, 0x0.ffffffffffffffffffffffffffffp-16382) == 0x0.ffffffffffffffffffffffffffffp-16382",  FUNC(fma) (0x1p-16494L, 0x1p-16494L, 0x0.ffffffffffffffffffffffffffffp-16382L), 0x0.ffffffffffffffffffffffffffffp-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1p-16494, -0x1p-16494, 0x0.ffffffffffffffffffffffffffffp-16382) == 0x0.fffffffffffffffffffffffffffep-16382",  FUNC(fma) (0x1p-16494L, -0x1p-16494L, 0x0.ffffffffffffffffffffffffffffp-16382L), 0x0.fffffffffffffffffffffffffffep-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1p-16494, 0x1p-16494, -0x0.ffffffffffffffffffffffffffffp-16382) == -0x0.ffffffffffffffffffffffffffffp-16382",  FUNC(fma) (0x1p-16494L, 0x1p-16494L, -0x0.ffffffffffffffffffffffffffffp-16382L), -0x0.ffffffffffffffffffffffffffffp-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1p-16494, -0x1p-16494, -0x0.ffffffffffffffffffffffffffffp-16382) == -0x1p-16382",  FUNC(fma) (0x1p-16494L, -0x1p-16494L, -0x0.ffffffffffffffffffffffffffffp-16382L), -0x1p-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1p-16494, 0x1p-16494, 0x1p-16494) == 0x1p-16494",  FUNC(fma) (0x1p-16494L, 0x1p-16494L, 0x1p-16494L), 0x1p-16494L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1p-16494, -0x1p-16494, 0x1p-16494) == +0",  FUNC(fma) (0x1p-16494L, -0x1p-16494L, 0x1p-16494L), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1p-16494, 0x1p-16494, -0x1p-16494) == -0x1p-16494",  FUNC(fma) (0x1p-16494L, 0x1p-16494L, -0x1p-16494L), -0x1p-16494L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x1p-16494, -0x1p-16494, -0x1p-16494) == -0x1p-16493",  FUNC(fma) (0x1p-16494L, -0x1p-16494L, -0x1p-16494L), -0x1p-16493L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_downward (0x0.ffffffffffffffffffffffffffff8p0, 0x0.ffffffffffffffffffffffffffff8p0, -0x0.ffffffffffffffffffffffffffffp0) == 0x1p-226",  FUNC(fma) (0x0.ffffffffffffffffffffffffffff8p0L, 0x0.ffffffffffffffffffffffffffff8p0L, -0x0.ffffffffffffffffffffffffffffp0L), 0x1p-226L, 0, 0, 0);
  check_float ("fma_downward (0x0.ffffffffffffffffffffffffffff8p0, -0x0.ffffffffffffffffffffffffffff8p0, 0x0.ffffffffffffffffffffffffffffp0) == -0x1p-226",  FUNC(fma) (0x0.ffffffffffffffffffffffffffff8p0L, -0x0.ffffffffffffffffffffffffffff8p0L, 0x0.ffffffffffffffffffffffffffffp0L), -0x1p-226L, 0, 0, 0);
  check_float ("fma_downward (-0x0.ffffffffffffffffffffffffffff8p0, 0x0.ffffffffffffffffffffffffffff8p0, 0x0.ffffffffffffffffffffffffffffp0) == -0x1p-226",  FUNC(fma) (-0x0.ffffffffffffffffffffffffffff8p0L, 0x0.ffffffffffffffffffffffffffff8p0L, 0x0.ffffffffffffffffffffffffffffp0L), -0x1p-226L, 0, 0, 0);
  check_float ("fma_downward (-0x0.ffffffffffffffffffffffffffff8p0, -0x0.ffffffffffffffffffffffffffff8p0, -0x0.ffffffffffffffffffffffffffffp0) == 0x1p-226",  FUNC(fma) (-0x0.ffffffffffffffffffffffffffff8p0L, -0x0.ffffffffffffffffffffffffffff8p0L, -0x0.ffffffffffffffffffffffffffffp0L), 0x1p-226L, 0, 0, 0);
  check_float ("fma_downward (0x1.0000000000000000000000000001p-16382, 0x1.0000000000000000000000000001p-66, 0x1p16383) == 0x1p16383",  FUNC(fma) (0x1.0000000000000000000000000001p-16382L, 0x1.0000000000000000000000000001p-66L, 0x1p16383L), 0x1p16383L, 0, 0, 0);
  check_float ("fma_downward (0x1.0000000000000000000000000001p-16382, -0x1.0000000000000000000000000001p-66, 0x1p16383) == 0x0.ffffffffffffffffffffffffffff8p16383",  FUNC(fma) (0x1.0000000000000000000000000001p-16382L, -0x1.0000000000000000000000000001p-66L, 0x1p16383L), 0x0.ffffffffffffffffffffffffffff8p16383L, 0, 0, 0);
  check_float ("fma_downward (0x1.0000000000000000000000000001p-16382, 0x1.0000000000000000000000000001p-66, -0x1p16383) == -0x1p16383",  FUNC(fma) (0x1.0000000000000000000000000001p-16382L, 0x1.0000000000000000000000000001p-66L, -0x1p16383L), -0x1p16383L, 0, 0, 0);
  check_float ("fma_downward (0x1.0000000000000000000000000001p-16382, -0x1.0000000000000000000000000001p-66, -0x1p16383) == -0x1.0000000000000000000000000001p16383",  FUNC(fma) (0x1.0000000000000000000000000001p-16382L, -0x1.0000000000000000000000000001p-66L, -0x1p16383L), -0x1.0000000000000000000000000001p16383L, 0, 0, 0);
  check_float ("fma_downward (0x1.0000000000000000000000000001p-16382, 0x1.0000000000000000000000000001p-66, 0x1p16319) == 0x1p16319",  FUNC(fma) (0x1.0000000000000000000000000001p-16382L, 0x1.0000000000000000000000000001p-66L, 0x1p16319L), 0x1p16319L, 0, 0, 0);
  check_float ("fma_downward (0x1.0000000000000000000000000001p-16382, -0x1.0000000000000000000000000001p-66, 0x1p16319) == 0x0.ffffffffffffffffffffffffffff8p16319",  FUNC(fma) (0x1.0000000000000000000000000001p-16382L, -0x1.0000000000000000000000000001p-66L, 0x1p16319L), 0x0.ffffffffffffffffffffffffffff8p16319L, 0, 0, 0);
  check_float ("fma_downward (0x1.0000000000000000000000000001p-16382, 0x1.0000000000000000000000000001p-66, -0x1p16319) == -0x1p16319",  FUNC(fma) (0x1.0000000000000000000000000001p-16382L, 0x1.0000000000000000000000000001p-66L, -0x1p16319L), -0x1p16319L, 0, 0, 0);
  check_float ("fma_downward (0x1.0000000000000000000000000001p-16382, -0x1.0000000000000000000000000001p-66, -0x1p16319) == -0x1.0000000000000000000000000001p16319",  FUNC(fma) (0x1.0000000000000000000000000001p-16382L, -0x1.0000000000000000000000000001p-66L, -0x1p16319L), -0x1.0000000000000000000000000001p16319L, 0, 0, 0);
#endif
    }

  fesetround (save_round_mode);

  print_max_error ("fma_downward", 0, 0);
}


static void
fma_test_upward (void)
{
  int save_round_mode;
  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_UPWARD))
    {
  check_float ("fma_upward (+0, +0, +0) == +0",  FUNC(fma) (plus_zero, plus_zero, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_upward (+0, +0, -0) == +0",  FUNC(fma) (plus_zero, plus_zero, minus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_upward (+0, -0, +0) == +0",  FUNC(fma) (plus_zero, minus_zero, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_upward (+0, -0, -0) == -0",  FUNC(fma) (plus_zero, minus_zero, minus_zero), minus_zero, 0, 0, 0);
  check_float ("fma_upward (-0, +0, +0) == +0",  FUNC(fma) (minus_zero, plus_zero, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_upward (-0, +0, -0) == -0",  FUNC(fma) (minus_zero, plus_zero, minus_zero), minus_zero, 0, 0, 0);
  check_float ("fma_upward (-0, -0, +0) == +0",  FUNC(fma) (minus_zero, minus_zero, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_upward (-0, -0, -0) == +0",  FUNC(fma) (minus_zero, minus_zero, minus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_upward (1.0, +0, +0) == +0",  FUNC(fma) (1.0, plus_zero, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_upward (1.0, +0, -0) == +0",  FUNC(fma) (1.0, plus_zero, minus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_upward (1.0, -0, +0) == +0",  FUNC(fma) (1.0, minus_zero, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_upward (1.0, -0, -0) == -0",  FUNC(fma) (1.0, minus_zero, minus_zero), minus_zero, 0, 0, 0);
  check_float ("fma_upward (-1.0, +0, +0) == +0",  FUNC(fma) (-1.0, plus_zero, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_upward (-1.0, +0, -0) == -0",  FUNC(fma) (-1.0, plus_zero, minus_zero), minus_zero, 0, 0, 0);
  check_float ("fma_upward (-1.0, -0, +0) == +0",  FUNC(fma) (-1.0, minus_zero, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_upward (-1.0, -0, -0) == +0",  FUNC(fma) (-1.0, minus_zero, minus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_upward (+0, 1.0, +0) == +0",  FUNC(fma) (plus_zero, 1.0, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_upward (+0, 1.0, -0) == +0",  FUNC(fma) (plus_zero, 1.0, minus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_upward (+0, -1.0, +0) == +0",  FUNC(fma) (plus_zero, -1.0, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_upward (+0, -1.0, -0) == -0",  FUNC(fma) (plus_zero, -1.0, minus_zero), minus_zero, 0, 0, 0);
  check_float ("fma_upward (-0, 1.0, +0) == +0",  FUNC(fma) (minus_zero, 1.0, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_upward (-0, 1.0, -0) == -0",  FUNC(fma) (minus_zero, 1.0, minus_zero), minus_zero, 0, 0, 0);
  check_float ("fma_upward (-0, -1.0, +0) == +0",  FUNC(fma) (minus_zero, -1.0, plus_zero), plus_zero, 0, 0, 0);
  check_float ("fma_upward (-0, -1.0, -0) == +0",  FUNC(fma) (minus_zero, -1.0, minus_zero), plus_zero, 0, 0, 0);

  check_float ("fma_upward (1.0, 1.0, -1.0) == +0",  FUNC(fma) (1.0, 1.0, -1.0), plus_zero, 0, 0, 0);
  check_float ("fma_upward (1.0, -1.0, 1.0) == +0",  FUNC(fma) (1.0, -1.0, 1.0), plus_zero, 0, 0, 0);
  check_float ("fma_upward (-1.0, 1.0, 1.0) == +0",  FUNC(fma) (-1.0, 1.0, 1.0), plus_zero, 0, 0, 0);
  check_float ("fma_upward (-1.0, -1.0, -1.0) == +0",  FUNC(fma) (-1.0, -1.0, -1.0), plus_zero, 0, 0, 0);

  check_float ("fma_upward (min_value, min_value, +0) == min_subnorm_value",  FUNC(fma) (min_value, min_value, plus_zero), min_subnorm_value, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (min_value, min_value, -0) == min_subnorm_value",  FUNC(fma) (min_value, min_value, minus_zero), min_subnorm_value, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (min_value, -min_value, +0) == -0",  FUNC(fma) (min_value, -min_value, plus_zero), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (min_value, -min_value, -0) == -0",  FUNC(fma) (min_value, -min_value, minus_zero), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (-min_value, min_value, +0) == -0",  FUNC(fma) (-min_value, min_value, plus_zero), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (-min_value, min_value, -0) == -0",  FUNC(fma) (-min_value, min_value, minus_zero), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (-min_value, -min_value, +0) == min_subnorm_value",  FUNC(fma) (-min_value, -min_value, plus_zero), min_subnorm_value, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (-min_value, -min_value, -0) == min_subnorm_value",  FUNC(fma) (-min_value, -min_value, minus_zero), min_subnorm_value, 0, 0, UNDERFLOW_EXCEPTION);

#if !(defined TEST_LDOUBLE && LDBL_MANT_DIG == 106) /* Bug 13304.  */
  check_float ("fma_upward (max_value, max_value, min_value) == inf",  FUNC(fma) (max_value, max_value, min_value), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("fma_upward (max_value, max_value, -min_value) == inf",  FUNC(fma) (max_value, max_value, -min_value), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("fma_upward (max_value, -max_value, min_value) == -max_value",  FUNC(fma) (max_value, -max_value, min_value), -max_value, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("fma_upward (max_value, -max_value, -min_value) == -max_value",  FUNC(fma) (max_value, -max_value, -min_value), -max_value, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("fma_upward (-max_value, max_value, min_value) == -max_value",  FUNC(fma) (-max_value, max_value, min_value), -max_value, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("fma_upward (-max_value, max_value, -min_value) == -max_value",  FUNC(fma) (-max_value, max_value, -min_value), -max_value, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("fma_upward (-max_value, -max_value, min_value) == inf",  FUNC(fma) (-max_value, -max_value, min_value), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("fma_upward (-max_value, -max_value, -min_value) == inf",  FUNC(fma) (-max_value, -max_value, -min_value), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
#endif

#if defined (TEST_FLOAT) && FLT_MANT_DIG == 24
  check_float ("fma_upward (0x1.4p-126, 0x1.000004p-1, 0x1p-128) == 0x1.c00008p-127",  FUNC(fma) (0x1.4p-126, 0x1.000004p-1, 0x1p-128), 0x1.c00008p-127, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (-0x1.4p-126, 0x1.000004p-1, -0x1p-128) == -0x1.c00004p-127",  FUNC(fma) (-0x1.4p-126, 0x1.000004p-1, -0x1p-128), -0x1.c00004p-127, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1.fffff8p-126, 0x1.000002p-1, 0x1p-149) == 0x1p-126",  FUNC(fma) (0x1.fffff8p-126, 0x1.000002p-1, 0x1p-149), 0x1p-126, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma_upward (-0x1.fffff8p-126, 0x1.000002p-1, -0x1p-149) == -0x0.fffffep-126",  FUNC(fma) (-0x1.fffff8p-126, 0x1.000002p-1, -0x1p-149), -0x0.fffffep-126, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-149, 0x1p-1, 0x0.fffffep-126) == 0x1p-126",  FUNC(fma) (0x1p-149, 0x1p-1, 0x0.fffffep-126), 0x1p-126, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (-0x1p-149, 0x1p-1, -0x0.fffffep-126) == -0x0.fffffep-126",  FUNC(fma) (-0x1p-149, 0x1p-1, -0x0.fffffep-126), -0x0.fffffep-126, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-149, 0x1.1p-1, 0x0.fffffep-126) == 0x1p-126",  FUNC(fma) (0x1p-149, 0x1.1p-1, 0x0.fffffep-126), 0x1p-126, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma_upward (-0x1p-149, 0x1.1p-1, -0x0.fffffep-126) == -0x0.fffffep-126",  FUNC(fma) (-0x1p-149, 0x1.1p-1, -0x0.fffffep-126), -0x0.fffffep-126, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-149, 0x1p-149, 0x1p127) == 0x1.000002p127",  FUNC(fma) (0x1p-149, 0x1p-149, 0x1p127), 0x1.000002p127, 0, 0, 0);
  check_float ("fma_upward (0x1p-149, -0x1p-149, 0x1p127) == 0x1p127",  FUNC(fma) (0x1p-149, -0x1p-149, 0x1p127), 0x1p127, 0, 0, 0);
  check_float ("fma_upward (0x1p-149, 0x1p-149, -0x1p127) == -0x0.ffffffp127",  FUNC(fma) (0x1p-149, 0x1p-149, -0x1p127), -0x0.ffffffp127, 0, 0, 0);
  check_float ("fma_upward (0x1p-149, -0x1p-149, -0x1p127) == -0x1p127",  FUNC(fma) (0x1p-149, -0x1p-149, -0x1p127), -0x1p127, 0, 0, 0);
  check_float ("fma_upward (0x1p-149, 0x1p-149, 0x1p-126) == 0x1.000002p-126",  FUNC(fma) (0x1p-149, 0x1p-149, 0x1p-126), 0x1.000002p-126, 0, 0, 0);
  check_float ("fma_upward (0x1p-149, -0x1p-149, 0x1p-126) == 0x1p-126",  FUNC(fma) (0x1p-149, -0x1p-149, 0x1p-126), 0x1p-126, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma_upward (0x1p-149, 0x1p-149, -0x1p-126) == -0x0.fffffep-126",  FUNC(fma) (0x1p-149, 0x1p-149, -0x1p-126), -0x0.fffffep-126, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-149, -0x1p-149, -0x1p-126) == -0x1p-126",  FUNC(fma) (0x1p-149, -0x1p-149, -0x1p-126), -0x1p-126, 0, 0, 0);
  check_float ("fma_upward (0x1p-149, 0x1p-149, 0x0.fffffep-126) == 0x1p-126",  FUNC(fma) (0x1p-149, 0x1p-149, 0x0.fffffep-126), 0x1p-126, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-149, -0x1p-149, 0x0.fffffep-126) == 0x0.fffffep-126",  FUNC(fma) (0x1p-149, -0x1p-149, 0x0.fffffep-126), 0x0.fffffep-126, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-149, 0x1p-149, -0x0.fffffep-126) == -0x0.fffffcp-126",  FUNC(fma) (0x1p-149, 0x1p-149, -0x0.fffffep-126), -0x0.fffffcp-126, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-149, -0x1p-149, -0x0.fffffep-126) == -0x0.fffffep-126",  FUNC(fma) (0x1p-149, -0x1p-149, -0x0.fffffep-126), -0x0.fffffep-126, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-149, 0x1p-149, 0x1p-149) == 0x1p-148",  FUNC(fma) (0x1p-149, 0x1p-149, 0x1p-149), 0x1p-148, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-149, -0x1p-149, 0x1p-149) == 0x1p-149",  FUNC(fma) (0x1p-149, -0x1p-149, 0x1p-149), 0x1p-149, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-149, 0x1p-149, -0x1p-149) == -0",  FUNC(fma) (0x1p-149, 0x1p-149, -0x1p-149), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-149, -0x1p-149, -0x1p-149) == -0x1p-149",  FUNC(fma) (0x1p-149, -0x1p-149, -0x1p-149), -0x1p-149, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x0.fffp0, 0x0.fffp0, -0x0.ffep0) == 0x1p-24",  FUNC(fma) (0x0.fffp0, 0x0.fffp0, -0x0.ffep0), 0x1p-24, 0, 0, 0);
  check_float ("fma_upward (0x0.fffp0, -0x0.fffp0, 0x0.ffep0) == -0x1p-24",  FUNC(fma) (0x0.fffp0, -0x0.fffp0, 0x0.ffep0), -0x1p-24, 0, 0, 0);
  check_float ("fma_upward (-0x0.fffp0, 0x0.fffp0, 0x0.ffep0) == -0x1p-24",  FUNC(fma) (-0x0.fffp0, 0x0.fffp0, 0x0.ffep0), -0x1p-24, 0, 0, 0);
  check_float ("fma_upward (-0x0.fffp0, -0x0.fffp0, -0x0.ffep0) == 0x1p-24",  FUNC(fma) (-0x0.fffp0, -0x0.fffp0, -0x0.ffep0), 0x1p-24, 0, 0, 0);
  check_float ("fma_upward (0x1.000002p-126, 0x1.000002p-26, 0x1p127) == 0x1.000002p127",  FUNC(fma) (0x1.000002p-126, 0x1.000002p-26, 0x1p127), 0x1.000002p127, 0, 0, 0);
  check_float ("fma_upward (0x1.000002p-126, -0x1.000002p-26, 0x1p127) == 0x1p127",  FUNC(fma) (0x1.000002p-126, -0x1.000002p-26, 0x1p127), 0x1p127, 0, 0, 0);
  check_float ("fma_upward (0x1.000002p-126, 0x1.000002p-26, -0x1p127) == -0x0.ffffffp127",  FUNC(fma) (0x1.000002p-126, 0x1.000002p-26, -0x1p127), -0x0.ffffffp127, 0, 0, 0);
  check_float ("fma_upward (0x1.000002p-126, -0x1.000002p-26, -0x1p127) == -0x1p127",  FUNC(fma) (0x1.000002p-126, -0x1.000002p-26, -0x1p127), -0x1p127, 0, 0, 0);
  check_float ("fma_upward (0x1.000002p-126, 0x1.000002p-26, 0x1p103) == 0x1.000002p103",  FUNC(fma) (0x1.000002p-126, 0x1.000002p-26, 0x1p103), 0x1.000002p103, 0, 0, 0);
  check_float ("fma_upward (0x1.000002p-126, -0x1.000002p-26, 0x1p103) == 0x1p103",  FUNC(fma) (0x1.000002p-126, -0x1.000002p-26, 0x1p103), 0x1p103, 0, 0, 0);
  check_float ("fma_upward (0x1.000002p-126, 0x1.000002p-26, -0x1p103) == -0x0.ffffffp103",  FUNC(fma) (0x1.000002p-126, 0x1.000002p-26, -0x1p103), -0x0.ffffffp103, 0, 0, 0);
  check_float ("fma_upward (0x1.000002p-126, -0x1.000002p-26, -0x1p103) == -0x1p103",  FUNC(fma) (0x1.000002p-126, -0x1.000002p-26, -0x1p103), -0x1p103, 0, 0, 0);
#endif
#if defined (TEST_DOUBLE) && DBL_MANT_DIG == 53
  check_float ("fma_upward (0x1.4p-1022, 0x1.0000000000002p-1, 0x1p-1024) == 0x1.c000000000004p-1023",  FUNC(fma) (0x1.4p-1022, 0x1.0000000000002p-1, 0x1p-1024), 0x1.c000000000004p-1023, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (-0x1.4p-1022, 0x1.0000000000002p-1, -0x1p-1024) == -0x1.c000000000002p-1023",  FUNC(fma) (-0x1.4p-1022, 0x1.0000000000002p-1, -0x1p-1024), -0x1.c000000000002p-1023, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1.ffffffffffffcp-1022, 0x1.0000000000001p-1, 0x1p-1074) == 0x1p-1022",  FUNC(fma) (0x1.ffffffffffffcp-1022, 0x1.0000000000001p-1, 0x1p-1074), 0x1p-1022, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma_upward (-0x1.ffffffffffffcp-1022, 0x1.0000000000001p-1, -0x1p-1074) == -0x0.fffffffffffffp-1022",  FUNC(fma) (-0x1.ffffffffffffcp-1022, 0x1.0000000000001p-1, -0x1p-1074), -0x0.fffffffffffffp-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-1074, 0x1p-1, 0x0.fffffffffffffp-1022) == 0x1p-1022",  FUNC(fma) (0x1p-1074, 0x1p-1, 0x0.fffffffffffffp-1022), 0x1p-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (-0x1p-1074, 0x1p-1, -0x0.fffffffffffffp-1022) == -0x0.fffffffffffffp-1022",  FUNC(fma) (-0x1p-1074, 0x1p-1, -0x0.fffffffffffffp-1022), -0x0.fffffffffffffp-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-1074, 0x1.1p-1, 0x0.fffffffffffffp-1022) == 0x1p-1022",  FUNC(fma) (0x1p-1074, 0x1.1p-1, 0x0.fffffffffffffp-1022), 0x1p-1022, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma_upward (-0x1p-1074, 0x1.1p-1, -0x0.fffffffffffffp-1022) == -0x0.fffffffffffffp-1022",  FUNC(fma) (-0x1p-1074, 0x1.1p-1, -0x0.fffffffffffffp-1022), -0x0.fffffffffffffp-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-1074, 0x1p-1074, 0x1p1023) == 0x1.0000000000001p1023",  FUNC(fma) (0x1p-1074, 0x1p-1074, 0x1p1023), 0x1.0000000000001p1023, 0, 0, 0);
  check_float ("fma_upward (0x1p-1074, -0x1p-1074, 0x1p1023) == 0x1p1023",  FUNC(fma) (0x1p-1074, -0x1p-1074, 0x1p1023), 0x1p1023, 0, 0, 0);
  check_float ("fma_upward (0x1p-1074, 0x1p-1074, -0x1p1023) == -0x0.fffffffffffff8p1023",  FUNC(fma) (0x1p-1074, 0x1p-1074, -0x1p1023), -0x0.fffffffffffff8p1023, 0, 0, 0);
  check_float ("fma_upward (0x1p-1074, -0x1p-1074, -0x1p1023) == -0x1p1023",  FUNC(fma) (0x1p-1074, -0x1p-1074, -0x1p1023), -0x1p1023, 0, 0, 0);
  check_float ("fma_upward (0x1p-1074, 0x1p-1074, 0x1p-1022) == 0x1.0000000000001p-1022",  FUNC(fma) (0x1p-1074, 0x1p-1074, 0x1p-1022), 0x1.0000000000001p-1022, 0, 0, 0);
  check_float ("fma_upward (0x1p-1074, -0x1p-1074, 0x1p-1022) == 0x1p-1022",  FUNC(fma) (0x1p-1074, -0x1p-1074, 0x1p-1022), 0x1p-1022, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma_upward (0x1p-1074, 0x1p-1074, -0x1p-1022) == -0x0.fffffffffffffp-1022",  FUNC(fma) (0x1p-1074, 0x1p-1074, -0x1p-1022), -0x0.fffffffffffffp-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-1074, -0x1p-1074, -0x1p-1022) == -0x1p-1022",  FUNC(fma) (0x1p-1074, -0x1p-1074, -0x1p-1022), -0x1p-1022, 0, 0, 0);
  check_float ("fma_upward (0x1p-1074, 0x1p-1074, 0x0.fffffffffffffp-1022) == 0x1p-1022",  FUNC(fma) (0x1p-1074, 0x1p-1074, 0x0.fffffffffffffp-1022), 0x1p-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-1074, -0x1p-1074, 0x0.fffffffffffffp-1022) == 0x0.fffffffffffffp-1022",  FUNC(fma) (0x1p-1074, -0x1p-1074, 0x0.fffffffffffffp-1022), 0x0.fffffffffffffp-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-1074, 0x1p-1074, -0x0.fffffffffffffp-1022) == -0x0.ffffffffffffep-1022",  FUNC(fma) (0x1p-1074, 0x1p-1074, -0x0.fffffffffffffp-1022), -0x0.ffffffffffffep-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-1074, -0x1p-1074, -0x0.fffffffffffffp-1022) == -0x0.fffffffffffffp-1022",  FUNC(fma) (0x1p-1074, -0x1p-1074, -0x0.fffffffffffffp-1022), -0x0.fffffffffffffp-1022, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-1074, 0x1p-1074, 0x1p-1074) == 0x1p-1073",  FUNC(fma) (0x1p-1074, 0x1p-1074, 0x1p-1074), 0x1p-1073, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-1074, -0x1p-1074, 0x1p-1074) == 0x1p-1074",  FUNC(fma) (0x1p-1074, -0x1p-1074, 0x1p-1074), 0x1p-1074, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-1074, 0x1p-1074, -0x1p-1074) == -0",  FUNC(fma) (0x1p-1074, 0x1p-1074, -0x1p-1074), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-1074, -0x1p-1074, -0x1p-1074) == -0x1p-1074",  FUNC(fma) (0x1p-1074, -0x1p-1074, -0x1p-1074), -0x1p-1074, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x0.fffffffffffff8p0, 0x0.fffffffffffff8p0, -0x0.fffffffffffffp0) == 0x1p-106",  FUNC(fma) (0x0.fffffffffffff8p0, 0x0.fffffffffffff8p0, -0x0.fffffffffffffp0), 0x1p-106, 0, 0, 0);
  check_float ("fma_upward (0x0.fffffffffffff8p0, -0x0.fffffffffffff8p0, 0x0.fffffffffffffp0) == -0x1p-106",  FUNC(fma) (0x0.fffffffffffff8p0, -0x0.fffffffffffff8p0, 0x0.fffffffffffffp0), -0x1p-106, 0, 0, 0);
  check_float ("fma_upward (-0x0.fffffffffffff8p0, 0x0.fffffffffffff8p0, 0x0.fffffffffffffp0) == -0x1p-106",  FUNC(fma) (-0x0.fffffffffffff8p0, 0x0.fffffffffffff8p0, 0x0.fffffffffffffp0), -0x1p-106, 0, 0, 0);
  check_float ("fma_upward (-0x0.fffffffffffff8p0, -0x0.fffffffffffff8p0, -0x0.fffffffffffffp0) == 0x1p-106",  FUNC(fma) (-0x0.fffffffffffff8p0, -0x0.fffffffffffff8p0, -0x0.fffffffffffffp0), 0x1p-106, 0, 0, 0);
  check_float ("fma_upward (0x1.0000000000001p-1022, 0x1.0000000000001p-55, 0x1p1023) == 0x1.0000000000001p1023",  FUNC(fma) (0x1.0000000000001p-1022, 0x1.0000000000001p-55, 0x1p1023), 0x1.0000000000001p1023, 0, 0, 0);
  check_float ("fma_upward (0x1.0000000000001p-1022, -0x1.0000000000001p-55, 0x1p1023) == 0x1p1023",  FUNC(fma) (0x1.0000000000001p-1022, -0x1.0000000000001p-55, 0x1p1023), 0x1p1023, 0, 0, 0);
  check_float ("fma_upward (0x1.0000000000001p-1022, 0x1.0000000000001p-55, -0x1p1023) == -0x0.fffffffffffff8p1023",  FUNC(fma) (0x1.0000000000001p-1022, 0x1.0000000000001p-55, -0x1p1023), -0x0.fffffffffffff8p1023, 0, 0, 0);
  check_float ("fma_upward (0x1.0000000000001p-1022, -0x1.0000000000001p-55, -0x1p1023) == -0x1p1023",  FUNC(fma) (0x1.0000000000001p-1022, -0x1.0000000000001p-55, -0x1p1023), -0x1p1023, 0, 0, 0);
  check_float ("fma_upward (0x1.0000000000001p-1022, 0x1.0000000000001p-55, 0x1p970) == 0x1.0000000000001p970",  FUNC(fma) (0x1.0000000000001p-1022, 0x1.0000000000001p-55, 0x1p970), 0x1.0000000000001p970, 0, 0, 0);
  check_float ("fma_upward (0x1.0000000000001p-1022, -0x1.0000000000001p-55, 0x1p970) == 0x1p970",  FUNC(fma) (0x1.0000000000001p-1022, -0x1.0000000000001p-55, 0x1p970), 0x1p970, 0, 0, 0);
  check_float ("fma_upward (0x1.0000000000001p-1022, 0x1.0000000000001p-55, -0x1p970) == -0x0.fffffffffffff8p970",  FUNC(fma) (0x1.0000000000001p-1022, 0x1.0000000000001p-55, -0x1p970), -0x0.fffffffffffff8p970, 0, 0, 0);
  check_float ("fma_upward (0x1.0000000000001p-1022, -0x1.0000000000001p-55, -0x1p970) == -0x1p970",  FUNC(fma) (0x1.0000000000001p-1022, -0x1.0000000000001p-55, -0x1p970), -0x1p970, 0, 0, 0);
#endif
#if defined (TEST_LDOUBLE) && LDBL_MANT_DIG == 64
  check_float ("fma_upward (0x1.4p-16382, 0x1.0000000000000004p-1, 0x1p-16384) == 0x1.c000000000000008p-16383",  FUNC(fma) (0x1.4p-16382L, 0x1.0000000000000004p-1L, 0x1p-16384L), 0x1.c000000000000008p-16383L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (-0x1.4p-16382, 0x1.0000000000000004p-1, -0x1p-16384) == -0x1.c000000000000004p-16383",  FUNC(fma) (-0x1.4p-16382L, 0x1.0000000000000004p-1L, -0x1p-16384L), -0x1.c000000000000004p-16383L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1.fffffffffffffff8p-16382, 0x1.0000000000000002p-1, 0x1p-16445) == 0x1p-16382",  FUNC(fma) (0x1.fffffffffffffff8p-16382L, 0x1.0000000000000002p-1L, 0x1p-16445L), 0x1p-16382L, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma_upward (-0x1.fffffffffffffff8p-16382, 0x1.0000000000000002p-1, -0x1p-16445) == -0x0.fffffffffffffffep-16382",  FUNC(fma) (-0x1.fffffffffffffff8p-16382L, 0x1.0000000000000002p-1L, -0x1p-16445L), -0x0.fffffffffffffffep-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-16445, 0x1p-1, 0x0.fffffffffffffffep-16382) == 0x1p-16382",  FUNC(fma) (0x1p-16445L, 0x1p-1L, 0x0.fffffffffffffffep-16382L), 0x1p-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (-0x1p-16445, 0x1p-1, -0x0.fffffffffffffffep-16382) == -0x0.fffffffffffffffep-16382",  FUNC(fma) (-0x1p-16445L, 0x1p-1L, -0x0.fffffffffffffffep-16382L), -0x0.fffffffffffffffep-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-16445, 0x1.1p-1, 0x0.fffffffffffffffep-16382) == 0x1p-16382",  FUNC(fma) (0x1p-16445L, 0x1.1p-1L, 0x0.fffffffffffffffep-16382L), 0x1p-16382L, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma_upward (-0x1p-16445, 0x1.1p-1, -0x0.fffffffffffffffep-16382) == -0x0.fffffffffffffffep-16382",  FUNC(fma) (-0x1p-16445L, 0x1.1p-1L, -0x0.fffffffffffffffep-16382L), -0x0.fffffffffffffffep-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-16445, 0x1p-16445, 0x1p16383) == 0x1.0000000000000002p16383",  FUNC(fma) (0x1p-16445L, 0x1p-16445L, 0x1p16383L), 0x1.0000000000000002p16383L, 0, 0, 0);
  check_float ("fma_upward (0x1p-16445, -0x1p-16445, 0x1p16383) == 0x1p16383",  FUNC(fma) (0x1p-16445L, -0x1p-16445L, 0x1p16383L), 0x1p16383L, 0, 0, 0);
  check_float ("fma_upward (0x1p-16445, 0x1p-16445, -0x1p16383) == -0x0.ffffffffffffffffp16383",  FUNC(fma) (0x1p-16445L, 0x1p-16445L, -0x1p16383L), -0x0.ffffffffffffffffp16383L, 0, 0, 0);
  check_float ("fma_upward (0x1p-16445, -0x1p-16445, -0x1p16383) == -0x1p16383",  FUNC(fma) (0x1p-16445L, -0x1p-16445L, -0x1p16383L), -0x1p16383L, 0, 0, 0);
  check_float ("fma_upward (0x1p-16445, 0x1p-16445, 0x1p-16382) == 0x1.0000000000000002p-16382",  FUNC(fma) (0x1p-16445L, 0x1p-16445L, 0x1p-16382L), 0x1.0000000000000002p-16382L, 0, 0, 0);
  check_float ("fma_upward (0x1p-16445, -0x1p-16445, 0x1p-16382) == 0x1p-16382",  FUNC(fma) (0x1p-16445L, -0x1p-16445L, 0x1p-16382L), 0x1p-16382L, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma_upward (0x1p-16445, 0x1p-16445, -0x1p-16382) == -0x0.fffffffffffffffep-16382",  FUNC(fma) (0x1p-16445L, 0x1p-16445L, -0x1p-16382L), -0x0.fffffffffffffffep-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-16445, -0x1p-16445, -0x1p-16382) == -0x1p-16382",  FUNC(fma) (0x1p-16445L, -0x1p-16445L, -0x1p-16382L), -0x1p-16382L, 0, 0, 0);
  check_float ("fma_upward (0x1p-16445, 0x1p-16445, 0x0.fffffffffffffffep-16382) == 0x1p-16382",  FUNC(fma) (0x1p-16445L, 0x1p-16445L, 0x0.fffffffffffffffep-16382L), 0x1p-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-16445, -0x1p-16445, 0x0.fffffffffffffffep-16382) == 0x0.fffffffffffffffep-16382",  FUNC(fma) (0x1p-16445L, -0x1p-16445L, 0x0.fffffffffffffffep-16382L), 0x0.fffffffffffffffep-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-16445, 0x1p-16445, -0x0.fffffffffffffffep-16382) == -0x0.fffffffffffffffcp-16382",  FUNC(fma) (0x1p-16445L, 0x1p-16445L, -0x0.fffffffffffffffep-16382L), -0x0.fffffffffffffffcp-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-16445, -0x1p-16445, -0x0.fffffffffffffffep-16382) == -0x0.fffffffffffffffep-16382",  FUNC(fma) (0x1p-16445L, -0x1p-16445L, -0x0.fffffffffffffffep-16382L), -0x0.fffffffffffffffep-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-16445, 0x1p-16445, 0x1p-16445) == 0x1p-16444",  FUNC(fma) (0x1p-16445L, 0x1p-16445L, 0x1p-16445L), 0x1p-16444L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-16445, -0x1p-16445, 0x1p-16445) == 0x1p-16445",  FUNC(fma) (0x1p-16445L, -0x1p-16445L, 0x1p-16445L), 0x1p-16445L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-16445, 0x1p-16445, -0x1p-16445) == -0",  FUNC(fma) (0x1p-16445L, 0x1p-16445L, -0x1p-16445L), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-16445, -0x1p-16445, -0x1p-16445) == -0x1p-16445",  FUNC(fma) (0x1p-16445L, -0x1p-16445L, -0x1p-16445L), -0x1p-16445L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x0.ffffffffffffffffp0, 0x0.ffffffffffffffffp0, -0x0.fffffffffffffffep0) == 0x1p-128",  FUNC(fma) (0x0.ffffffffffffffffp0L, 0x0.ffffffffffffffffp0L, -0x0.fffffffffffffffep0L), 0x1p-128L, 0, 0, 0);
  check_float ("fma_upward (0x0.ffffffffffffffffp0, -0x0.ffffffffffffffffp0, 0x0.fffffffffffffffep0) == -0x1p-128",  FUNC(fma) (0x0.ffffffffffffffffp0L, -0x0.ffffffffffffffffp0L, 0x0.fffffffffffffffep0L), -0x1p-128L, 0, 0, 0);
  check_float ("fma_upward (-0x0.ffffffffffffffffp0, 0x0.ffffffffffffffffp0, 0x0.fffffffffffffffep0) == -0x1p-128",  FUNC(fma) (-0x0.ffffffffffffffffp0L, 0x0.ffffffffffffffffp0L, 0x0.fffffffffffffffep0L), -0x1p-128L, 0, 0, 0);
  check_float ("fma_upward (-0x0.ffffffffffffffffp0, -0x0.ffffffffffffffffp0, -0x0.fffffffffffffffep0) == 0x1p-128",  FUNC(fma) (-0x0.ffffffffffffffffp0L, -0x0.ffffffffffffffffp0L, -0x0.fffffffffffffffep0L), 0x1p-128L, 0, 0, 0);
  check_float ("fma_upward (0x1.0000000000000002p-16382, 0x1.0000000000000002p-66, 0x1p16383) == 0x1.0000000000000002p16383",  FUNC(fma) (0x1.0000000000000002p-16382L, 0x1.0000000000000002p-66L, 0x1p16383L), 0x1.0000000000000002p16383L, 0, 0, 0);
  check_float ("fma_upward (0x1.0000000000000002p-16382, -0x1.0000000000000002p-66, 0x1p16383) == 0x1p16383",  FUNC(fma) (0x1.0000000000000002p-16382L, -0x1.0000000000000002p-66L, 0x1p16383L), 0x1p16383L, 0, 0, 0);
  check_float ("fma_upward (0x1.0000000000000002p-16382, 0x1.0000000000000002p-66, -0x1p16383) == -0x0.ffffffffffffffffp16383",  FUNC(fma) (0x1.0000000000000002p-16382L, 0x1.0000000000000002p-66L, -0x1p16383L), -0x0.ffffffffffffffffp16383L, 0, 0, 0);
  check_float ("fma_upward (0x1.0000000000000002p-16382, -0x1.0000000000000002p-66, -0x1p16383) == -0x1p16383",  FUNC(fma) (0x1.0000000000000002p-16382L, -0x1.0000000000000002p-66L, -0x1p16383L), -0x1p16383L, 0, 0, 0);
  check_float ("fma_upward (0x1.0000000000000002p-16382, 0x1.0000000000000002p-66, 0x1p16319) == 0x1.0000000000000002p16319",  FUNC(fma) (0x1.0000000000000002p-16382L, 0x1.0000000000000002p-66L, 0x1p16319L), 0x1.0000000000000002p16319L, 0, 0, 0);
  check_float ("fma_upward (0x1.0000000000000002p-16382, -0x1.0000000000000002p-66, 0x1p16319) == 0x1p16319",  FUNC(fma) (0x1.0000000000000002p-16382L, -0x1.0000000000000002p-66L, 0x1p16319L), 0x1p16319L, 0, 0, 0);
  check_float ("fma_upward (0x1.0000000000000002p-16382, 0x1.0000000000000002p-66, -0x1p16319) == -0x0.ffffffffffffffffp16319",  FUNC(fma) (0x1.0000000000000002p-16382L, 0x1.0000000000000002p-66L, -0x1p16319L), -0x0.ffffffffffffffffp16319L, 0, 0, 0);
  check_float ("fma_upward (0x1.0000000000000002p-16382, -0x1.0000000000000002p-66, -0x1p16319) == -0x1p16319",  FUNC(fma) (0x1.0000000000000002p-16382L, -0x1.0000000000000002p-66L, -0x1p16319L), -0x1p16319L, 0, 0, 0);
#endif
#if defined (TEST_LDOUBLE) && LDBL_MANT_DIG == 113
  check_float ("fma_upward (0x1.4p-16382, 0x1.0000000000000000000000000002p-1, 0x1p-16384) == 0x1.c000000000000000000000000004p-16383",  FUNC(fma) (0x1.4p-16382L, 0x1.0000000000000000000000000002p-1L, 0x1p-16384L), 0x1.c000000000000000000000000004p-16383L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (-0x1.4p-16382, 0x1.0000000000000000000000000002p-1, -0x1p-16384) == -0x1.c000000000000000000000000002p-16383",  FUNC(fma) (-0x1.4p-16382L, 0x1.0000000000000000000000000002p-1L, -0x1p-16384L), -0x1.c000000000000000000000000002p-16383L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1.fffffffffffffffffffffffffffcp-16382, 0x1.0000000000000000000000000001p-1, 0x1p-16494) == 0x1p-16382",  FUNC(fma) (0x1.fffffffffffffffffffffffffffcp-16382L, 0x1.0000000000000000000000000001p-1L, 0x1p-16494L), 0x1p-16382L, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma_upward (-0x1.fffffffffffffffffffffffffffcp-16382, 0x1.0000000000000000000000000001p-1, -0x1p-16494) == -0x0.ffffffffffffffffffffffffffffp-16382",  FUNC(fma) (-0x1.fffffffffffffffffffffffffffcp-16382L, 0x1.0000000000000000000000000001p-1L, -0x1p-16494L), -0x0.ffffffffffffffffffffffffffffp-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-16494, 0x1p-1, 0x0.ffffffffffffffffffffffffffffp-16382) == 0x1p-16382",  FUNC(fma) (0x1p-16494L, 0x1p-1L, 0x0.ffffffffffffffffffffffffffffp-16382L), 0x1p-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (-0x1p-16494, 0x1p-1, -0x0.ffffffffffffffffffffffffffffp-16382) == -0x0.ffffffffffffffffffffffffffffp-16382",  FUNC(fma) (-0x1p-16494L, 0x1p-1L, -0x0.ffffffffffffffffffffffffffffp-16382L), -0x0.ffffffffffffffffffffffffffffp-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-16494, 0x1.1p-1, 0x0.ffffffffffffffffffffffffffffp-16382) == 0x1p-16382",  FUNC(fma) (0x1p-16494L, 0x1.1p-1L, 0x0.ffffffffffffffffffffffffffffp-16382L), 0x1p-16382L, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma_upward (-0x1p-16494, 0x1.1p-1, -0x0.ffffffffffffffffffffffffffffp-16382) == -0x0.ffffffffffffffffffffffffffffp-16382",  FUNC(fma) (-0x1p-16494L, 0x1.1p-1L, -0x0.ffffffffffffffffffffffffffffp-16382L), -0x0.ffffffffffffffffffffffffffffp-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-16494, 0x1p-16494, 0x1p16383) == 0x1.0000000000000000000000000001p16383",  FUNC(fma) (0x1p-16494L, 0x1p-16494L, 0x1p16383L), 0x1.0000000000000000000000000001p16383L, 0, 0, 0);
  check_float ("fma_upward (0x1p-16494, -0x1p-16494, 0x1p16383) == 0x1p16383",  FUNC(fma) (0x1p-16494L, -0x1p-16494L, 0x1p16383L), 0x1p16383L, 0, 0, 0);
  check_float ("fma_upward (0x1p-16494, 0x1p-16494, -0x1p16383) == -0x0.ffffffffffffffffffffffffffff8p16383",  FUNC(fma) (0x1p-16494L, 0x1p-16494L, -0x1p16383L), -0x0.ffffffffffffffffffffffffffff8p16383L, 0, 0, 0);
  check_float ("fma_upward (0x1p-16494, -0x1p-16494, -0x1p16383) == -0x1p16383",  FUNC(fma) (0x1p-16494L, -0x1p-16494L, -0x1p16383L), -0x1p16383L, 0, 0, 0);
  check_float ("fma_upward (0x1p-16494, 0x1p-16494, 0x1p-16382) == 0x1.0000000000000000000000000001p-16382",  FUNC(fma) (0x1p-16494L, 0x1p-16494L, 0x1p-16382L), 0x1.0000000000000000000000000001p-16382L, 0, 0, 0);
  check_float ("fma_upward (0x1p-16494, -0x1p-16494, 0x1p-16382) == 0x1p-16382",  FUNC(fma) (0x1p-16494L, -0x1p-16494L, 0x1p-16382L), 0x1p-16382L, 0, 0, UNDERFLOW_EXCEPTION_BEFORE_ROUNDING);
  check_float ("fma_upward (0x1p-16494, 0x1p-16494, -0x1p-16382) == -0x0.ffffffffffffffffffffffffffffp-16382",  FUNC(fma) (0x1p-16494L, 0x1p-16494L, -0x1p-16382L), -0x0.ffffffffffffffffffffffffffffp-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-16494, -0x1p-16494, -0x1p-16382) == -0x1p-16382",  FUNC(fma) (0x1p-16494L, -0x1p-16494L, -0x1p-16382L), -0x1p-16382L, 0, 0, 0);
  check_float ("fma_upward (0x1p-16494, 0x1p-16494, 0x0.ffffffffffffffffffffffffffffp-16382) == 0x1p-16382",  FUNC(fma) (0x1p-16494L, 0x1p-16494L, 0x0.ffffffffffffffffffffffffffffp-16382L), 0x1p-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-16494, -0x1p-16494, 0x0.ffffffffffffffffffffffffffffp-16382) == 0x0.ffffffffffffffffffffffffffffp-16382",  FUNC(fma) (0x1p-16494L, -0x1p-16494L, 0x0.ffffffffffffffffffffffffffffp-16382L), 0x0.ffffffffffffffffffffffffffffp-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-16494, 0x1p-16494, -0x0.ffffffffffffffffffffffffffffp-16382) == -0x0.fffffffffffffffffffffffffffep-16382",  FUNC(fma) (0x1p-16494L, 0x1p-16494L, -0x0.ffffffffffffffffffffffffffffp-16382L), -0x0.fffffffffffffffffffffffffffep-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-16494, -0x1p-16494, -0x0.ffffffffffffffffffffffffffffp-16382) == -0x0.ffffffffffffffffffffffffffffp-16382",  FUNC(fma) (0x1p-16494L, -0x1p-16494L, -0x0.ffffffffffffffffffffffffffffp-16382L), -0x0.ffffffffffffffffffffffffffffp-16382L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-16494, 0x1p-16494, 0x1p-16494) == 0x1p-16493",  FUNC(fma) (0x1p-16494L, 0x1p-16494L, 0x1p-16494L), 0x1p-16493L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-16494, -0x1p-16494, 0x1p-16494) == 0x1p-16494",  FUNC(fma) (0x1p-16494L, -0x1p-16494L, 0x1p-16494L), 0x1p-16494L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-16494, 0x1p-16494, -0x1p-16494) == -0",  FUNC(fma) (0x1p-16494L, 0x1p-16494L, -0x1p-16494L), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x1p-16494, -0x1p-16494, -0x1p-16494) == -0x1p-16494",  FUNC(fma) (0x1p-16494L, -0x1p-16494L, -0x1p-16494L), -0x1p-16494L, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("fma_upward (0x0.ffffffffffffffffffffffffffff8p0, 0x0.ffffffffffffffffffffffffffff8p0, -0x0.ffffffffffffffffffffffffffffp0) == 0x1p-226",  FUNC(fma) (0x0.ffffffffffffffffffffffffffff8p0L, 0x0.ffffffffffffffffffffffffffff8p0L, -0x0.ffffffffffffffffffffffffffffp0L), 0x1p-226L, 0, 0, 0);
  check_float ("fma_upward (0x0.ffffffffffffffffffffffffffff8p0, -0x0.ffffffffffffffffffffffffffff8p0, 0x0.ffffffffffffffffffffffffffffp0) == -0x1p-226",  FUNC(fma) (0x0.ffffffffffffffffffffffffffff8p0L, -0x0.ffffffffffffffffffffffffffff8p0L, 0x0.ffffffffffffffffffffffffffffp0L), -0x1p-226L, 0, 0, 0);
  check_float ("fma_upward (-0x0.ffffffffffffffffffffffffffff8p0, 0x0.ffffffffffffffffffffffffffff8p0, 0x0.ffffffffffffffffffffffffffffp0) == -0x1p-226",  FUNC(fma) (-0x0.ffffffffffffffffffffffffffff8p0L, 0x0.ffffffffffffffffffffffffffff8p0L, 0x0.ffffffffffffffffffffffffffffp0L), -0x1p-226L, 0, 0, 0);
  check_float ("fma_upward (-0x0.ffffffffffffffffffffffffffff8p0, -0x0.ffffffffffffffffffffffffffff8p0, -0x0.ffffffffffffffffffffffffffffp0) == 0x1p-226",  FUNC(fma) (-0x0.ffffffffffffffffffffffffffff8p0L, -0x0.ffffffffffffffffffffffffffff8p0L, -0x0.ffffffffffffffffffffffffffffp0L), 0x1p-226L, 0, 0, 0);
  check_float ("fma_upward (0x1.0000000000000000000000000001p-16382, 0x1.0000000000000000000000000001p-66, 0x1p16383) == 0x1.0000000000000000000000000001p16383",  FUNC(fma) (0x1.0000000000000000000000000001p-16382L, 0x1.0000000000000000000000000001p-66L, 0x1p16383L), 0x1.0000000000000000000000000001p16383L, 0, 0, 0);
  check_float ("fma_upward (0x1.0000000000000000000000000001p-16382, -0x1.0000000000000000000000000001p-66, 0x1p16383) == 0x1p16383",  FUNC(fma) (0x1.0000000000000000000000000001p-16382L, -0x1.0000000000000000000000000001p-66L, 0x1p16383L), 0x1p16383L, 0, 0, 0);
  check_float ("fma_upward (0x1.0000000000000000000000000001p-16382, 0x1.0000000000000000000000000001p-66, -0x1p16383) == -0x0.ffffffffffffffffffffffffffff8p16383",  FUNC(fma) (0x1.0000000000000000000000000001p-16382L, 0x1.0000000000000000000000000001p-66L, -0x1p16383L), -0x0.ffffffffffffffffffffffffffff8p16383L, 0, 0, 0);
  check_float ("fma_upward (0x1.0000000000000000000000000001p-16382, -0x1.0000000000000000000000000001p-66, -0x1p16383) == -0x1p16383",  FUNC(fma) (0x1.0000000000000000000000000001p-16382L, -0x1.0000000000000000000000000001p-66L, -0x1p16383L), -0x1p16383L, 0, 0, 0);
  check_float ("fma_upward (0x1.0000000000000000000000000001p-16382, 0x1.0000000000000000000000000001p-66, 0x1p16319) == 0x1.0000000000000000000000000001p16319",  FUNC(fma) (0x1.0000000000000000000000000001p-16382L, 0x1.0000000000000000000000000001p-66L, 0x1p16319L), 0x1.0000000000000000000000000001p16319L, 0, 0, 0);
  check_float ("fma_upward (0x1.0000000000000000000000000001p-16382, -0x1.0000000000000000000000000001p-66, 0x1p16319) == 0x1p16319",  FUNC(fma) (0x1.0000000000000000000000000001p-16382L, -0x1.0000000000000000000000000001p-66L, 0x1p16319L), 0x1p16319L, 0, 0, 0);
  check_float ("fma_upward (0x1.0000000000000000000000000001p-16382, 0x1.0000000000000000000000000001p-66, -0x1p16319) == -0x0.ffffffffffffffffffffffffffff8p16319",  FUNC(fma) (0x1.0000000000000000000000000001p-16382L, 0x1.0000000000000000000000000001p-66L, -0x1p16319L), -0x0.ffffffffffffffffffffffffffff8p16319L, 0, 0, 0);
  check_float ("fma_upward (0x1.0000000000000000000000000001p-16382, -0x1.0000000000000000000000000001p-66, -0x1p16319) == -0x1p16319",  FUNC(fma) (0x1.0000000000000000000000000001p-16382L, -0x1.0000000000000000000000000001p-66L, -0x1p16319L), -0x1p16319L, 0, 0, 0);
#endif
    }

  fesetround (save_round_mode);

  print_max_error ("fma_upward", 0, 0);
}


static void
fmax_test (void)
{
  init_max_error ();

  check_float ("fmax (0, 0) == 0",  FUNC(fmax) (0, 0), 0, 0, 0, 0);
  check_float ("fmax (-0, -0) == -0",  FUNC(fmax) (minus_zero, minus_zero), minus_zero, 0, 0, 0);
  check_float ("fmax (9, 0) == 9",  FUNC(fmax) (9, 0), 9, 0, 0, 0);
  check_float ("fmax (0, 9) == 9",  FUNC(fmax) (0, 9), 9, 0, 0, 0);
  check_float ("fmax (-9, 0) == 0",  FUNC(fmax) (-9, 0), 0, 0, 0, 0);
  check_float ("fmax (0, -9) == 0",  FUNC(fmax) (0, -9), 0, 0, 0, 0);

  check_float ("fmax (inf, 9) == inf",  FUNC(fmax) (plus_infty, 9), plus_infty, 0, 0, 0);
  check_float ("fmax (0, inf) == inf",  FUNC(fmax) (0, plus_infty), plus_infty, 0, 0, 0);
  check_float ("fmax (-9, inf) == inf",  FUNC(fmax) (-9, plus_infty), plus_infty, 0, 0, 0);
  check_float ("fmax (inf, -9) == inf",  FUNC(fmax) (plus_infty, -9), plus_infty, 0, 0, 0);

  check_float ("fmax (-inf, 9) == 9",  FUNC(fmax) (minus_infty, 9), 9, 0, 0, 0);
  check_float ("fmax (-inf, -9) == -9",  FUNC(fmax) (minus_infty, -9), -9, 0, 0, 0);
  check_float ("fmax (9, -inf) == 9",  FUNC(fmax) (9, minus_infty), 9, 0, 0, 0);
  check_float ("fmax (-9, -inf) == -9",  FUNC(fmax) (-9, minus_infty), -9, 0, 0, 0);

  check_float ("fmax (0, NaN) == 0",  FUNC(fmax) (0, nan_value), 0, 0, 0, 0);
  check_float ("fmax (9, NaN) == 9",  FUNC(fmax) (9, nan_value), 9, 0, 0, 0);
  check_float ("fmax (-9, NaN) == -9",  FUNC(fmax) (-9, nan_value), -9, 0, 0, 0);
  check_float ("fmax (NaN, 0) == 0",  FUNC(fmax) (nan_value, 0), 0, 0, 0, 0);
  check_float ("fmax (NaN, 9) == 9",  FUNC(fmax) (nan_value, 9), 9, 0, 0, 0);
  check_float ("fmax (NaN, -9) == -9",  FUNC(fmax) (nan_value, -9), -9, 0, 0, 0);
  check_float ("fmax (inf, NaN) == inf",  FUNC(fmax) (plus_infty, nan_value), plus_infty, 0, 0, 0);
  check_float ("fmax (-inf, NaN) == -inf",  FUNC(fmax) (minus_infty, nan_value), minus_infty, 0, 0, 0);
  check_float ("fmax (NaN, inf) == inf",  FUNC(fmax) (nan_value, plus_infty), plus_infty, 0, 0, 0);
  check_float ("fmax (NaN, -inf) == -inf",  FUNC(fmax) (nan_value, minus_infty), minus_infty, 0, 0, 0);
  check_float ("fmax (NaN, NaN) == NaN",  FUNC(fmax) (nan_value, nan_value), nan_value, 0, 0, 0);

  print_max_error ("fmax", 0, 0);
}


static void
fmin_test (void)
{
  init_max_error ();

  check_float ("fmin (0, 0) == 0",  FUNC(fmin) (0, 0), 0, 0, 0, 0);
  check_float ("fmin (-0, -0) == -0",  FUNC(fmin) (minus_zero, minus_zero), minus_zero, 0, 0, 0);
  check_float ("fmin (9, 0) == 0",  FUNC(fmin) (9, 0), 0, 0, 0, 0);
  check_float ("fmin (0, 9) == 0",  FUNC(fmin) (0, 9), 0, 0, 0, 0);
  check_float ("fmin (-9, 0) == -9",  FUNC(fmin) (-9, 0), -9, 0, 0, 0);
  check_float ("fmin (0, -9) == -9",  FUNC(fmin) (0, -9), -9, 0, 0, 0);

  check_float ("fmin (inf, 9) == 9",  FUNC(fmin) (plus_infty, 9), 9, 0, 0, 0);
  check_float ("fmin (9, inf) == 9",  FUNC(fmin) (9, plus_infty), 9, 0, 0, 0);
  check_float ("fmin (inf, -9) == -9",  FUNC(fmin) (plus_infty, -9), -9, 0, 0, 0);
  check_float ("fmin (-9, inf) == -9",  FUNC(fmin) (-9, plus_infty), -9, 0, 0, 0);
  check_float ("fmin (-inf, 9) == -inf",  FUNC(fmin) (minus_infty, 9), minus_infty, 0, 0, 0);
  check_float ("fmin (-inf, -9) == -inf",  FUNC(fmin) (minus_infty, -9), minus_infty, 0, 0, 0);
  check_float ("fmin (9, -inf) == -inf",  FUNC(fmin) (9, minus_infty), minus_infty, 0, 0, 0);
  check_float ("fmin (-9, -inf) == -inf",  FUNC(fmin) (-9, minus_infty), minus_infty, 0, 0, 0);

  check_float ("fmin (0, NaN) == 0",  FUNC(fmin) (0, nan_value), 0, 0, 0, 0);
  check_float ("fmin (9, NaN) == 9",  FUNC(fmin) (9, nan_value), 9, 0, 0, 0);
  check_float ("fmin (-9, NaN) == -9",  FUNC(fmin) (-9, nan_value), -9, 0, 0, 0);
  check_float ("fmin (NaN, 0) == 0",  FUNC(fmin) (nan_value, 0), 0, 0, 0, 0);
  check_float ("fmin (NaN, 9) == 9",  FUNC(fmin) (nan_value, 9), 9, 0, 0, 0);
  check_float ("fmin (NaN, -9) == -9",  FUNC(fmin) (nan_value, -9), -9, 0, 0, 0);
  check_float ("fmin (inf, NaN) == inf",  FUNC(fmin) (plus_infty, nan_value), plus_infty, 0, 0, 0);
  check_float ("fmin (-inf, NaN) == -inf",  FUNC(fmin) (minus_infty, nan_value), minus_infty, 0, 0, 0);
  check_float ("fmin (NaN, inf) == inf",  FUNC(fmin) (nan_value, plus_infty), plus_infty, 0, 0, 0);
  check_float ("fmin (NaN, -inf) == -inf",  FUNC(fmin) (nan_value, minus_infty), minus_infty, 0, 0, 0);
  check_float ("fmin (NaN, NaN) == NaN",  FUNC(fmin) (nan_value, nan_value), nan_value, 0, 0, 0);

  print_max_error ("fmin", 0, 0);
}


static void
fmod_test (void)
{
  errno = 0;
  FUNC(fmod) (6.5, 2.3L);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  /* fmod (+0, y) == +0 for y != 0.  */
  check_float ("fmod (0, 3) == 0",  FUNC(fmod) (0, 3), 0, 0, 0, 0);

  /* fmod (-0, y) == -0 for y != 0.  */
  check_float ("fmod (-0, 3) == -0",  FUNC(fmod) (minus_zero, 3), minus_zero, 0, 0, 0);

  /* fmod (+inf, y) == NaN plus invalid exception.  */
  errno = 0;
  check_float ("fmod (inf, 3) == NaN",  FUNC(fmod) (plus_infty, 3), nan_value, 0, 0, INVALID_EXCEPTION);
  check_int ("errno for fmod(Inf,3) unchanged", errno, EDOM, 0, 0, 0);
  /* fmod (-inf, y) == NaN plus invalid exception.  */
  errno = 0;
  check_float ("fmod (-inf, 3) == NaN",  FUNC(fmod) (minus_infty, 3), nan_value, 0, 0, INVALID_EXCEPTION);
  check_int ("errno for fmod(-Inf,3) unchanged", errno, EDOM, 0, 0, 0);
  /* fmod (x, +0) == NaN plus invalid exception.  */
  errno = 0;
  check_float ("fmod (3, 0) == NaN",  FUNC(fmod) (3, 0), nan_value, 0, 0, INVALID_EXCEPTION);
  check_int ("errno for fmod(3,0) unchanged", errno, EDOM, 0, 0, 0);
  /* fmod (x, -0) == NaN plus invalid exception.  */
  check_float ("fmod (3, -0) == NaN",  FUNC(fmod) (3, minus_zero), nan_value, 0, 0, INVALID_EXCEPTION);

  /* fmod (x, +inf) == x for x not infinite.  */
  check_float ("fmod (3.0, inf) == 3.0",  FUNC(fmod) (3.0, plus_infty), 3.0, 0, 0, 0);
  /* fmod (x, -inf) == x for x not infinite.  */
  check_float ("fmod (3.0, -inf) == 3.0",  FUNC(fmod) (3.0, minus_infty), 3.0, 0, 0, 0);

  check_float ("fmod (NaN, NaN) == NaN",  FUNC(fmod) (nan_value, nan_value), nan_value, 0, 0, 0);

  check_float ("fmod (6.5, 2.25) == 2.0",  FUNC(fmod) (6.5, 2.25L), 2.0L, 0, 0, 0);
  check_float ("fmod (-6.5, 2.25) == -2.0",  FUNC(fmod) (-6.5, 2.25L), -2.0L, 0, 0, 0);
  check_float ("fmod (6.5, -2.25) == 2.0",  FUNC(fmod) (6.5, -2.25L), 2.0L, 0, 0, 0);
  check_float ("fmod (-6.5, -2.25) == -2.0",  FUNC(fmod) (-6.5, -2.25L), -2.0L, 0, 0, 0);

  check_float ("fmod (0x0.fffffep-126, 0x1p-149) == +0",  FUNC(fmod) (0x0.fffffep-126L, 0x1p-149L), plus_zero, 0, 0, 0);
#ifndef TEST_FLOAT
  check_float ("fmod (0x0.fffffffffffffp-1022, 0x1p-1074) == +0",  FUNC(fmod) (0x0.fffffffffffffp-1022L, 0x1p-1074L), plus_zero, 0, 0, 0);
#endif
#if defined TEST_LDOUBLE && LDBL_MIN_EXP <= -16381
  check_float ("fmod (0x0.fffffffffffffffep-16382, 0x1p-16445) == +0",  FUNC(fmod) (0x0.fffffffffffffffep-16382L, 0x1p-16445L), plus_zero, 0, 0, 0);
#endif

  print_max_error ("fmod", 0, 0);
}


static void
fpclassify_test (void)
{
  init_max_error ();

  check_int ("fpclassify (NaN) == FP_NAN", fpclassify (nan_value), FP_NAN, 0, 0, 0);
  check_int ("fpclassify (inf) == FP_INFINITE", fpclassify (plus_infty), FP_INFINITE, 0, 0, 0);
  check_int ("fpclassify (-inf) == FP_INFINITE", fpclassify (minus_infty), FP_INFINITE, 0, 0, 0);
  check_int ("fpclassify (+0) == FP_ZERO", fpclassify (plus_zero), FP_ZERO, 0, 0, 0);
  check_int ("fpclassify (-0) == FP_ZERO", fpclassify (minus_zero), FP_ZERO, 0, 0, 0);
  check_int ("fpclassify (1000) == FP_NORMAL", fpclassify (1000), FP_NORMAL, 0, 0, 0);
  check_int ("fpclassify (min_subnorm_value) == FP_SUBNORMAL", fpclassify (min_subnorm_value), FP_SUBNORMAL, 0, 0, 0);

  print_max_error ("fpclassify", 0, 0);
}


static void
frexp_test (void)
{
  int x;

  init_max_error ();

  check_float ("frexp (inf, &x) == inf",  FUNC(frexp) (plus_infty, &x), plus_infty, 0, 0, 0);
  check_float ("frexp (-inf, &x) == -inf",  FUNC(frexp) (minus_infty, &x), minus_infty, 0, 0, 0);
  check_float ("frexp (NaN, &x) == NaN",  FUNC(frexp) (nan_value, &x), nan_value, 0, 0, 0);

  check_float ("frexp (0.0, &x) == 0.0",  FUNC(frexp) (0.0, &x), 0.0, 0, 0, 0);
  check_int ("frexp (0.0, &x) sets x to 0.0", x, 0.0, 0, 0, 0);
  check_float ("frexp (-0, &x) == -0",  FUNC(frexp) (minus_zero, &x), minus_zero, 0, 0, 0);
  check_int ("frexp (-0, &x) sets x to 0.0", x, 0.0, 0, 0, 0);

  check_float ("frexp (12.8, &x) == 0.8",  FUNC(frexp) (12.8L, &x), 0.8L, 0, 0, 0);
  check_int ("frexp (12.8, &x) sets x to 4", x, 4, 0, 0, 0);
  check_float ("frexp (-27.34, &x) == -0.854375",  FUNC(frexp) (-27.34L, &x), -0.854375L, 0, 0, 0);
  check_int ("frexp (-27.34, &x) sets x to 5", x, 5, 0, 0, 0);

  print_max_error ("frexp", 0, 0);
}


static void
gamma_test (void)
{
  errno = 0;
  FUNC(gamma) (1);

  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  signgam = 0;
  check_float ("gamma (inf) == inf",  FUNC(gamma) (plus_infty), plus_infty, 0, 0, 0);
  signgam = 0;
  check_float ("gamma (0) == inf",  FUNC(gamma) (0), plus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  signgam = 0;
  check_float ("gamma (-3) == inf",  FUNC(gamma) (-3), plus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  signgam = 0;
  check_float ("gamma (-inf) == inf",  FUNC(gamma) (minus_infty), plus_infty, 0, 0, 0);
  signgam = 0;
  check_float ("gamma (NaN) == NaN",  FUNC(gamma) (nan_value), nan_value, 0, 0, 0);

  signgam = 0;
  check_float ("gamma (1) == 0",  FUNC(gamma) (1), 0, 0, 0, 0);
  check_int ("gamma (1) sets signgam to 1", signgam, 1, 0, 0, 0);
  signgam = 0;
  check_float ("gamma (3) == M_LN2l",  FUNC(gamma) (3), M_LN2l, 0, 0, 0);
  check_int ("gamma (3) sets signgam to 1", signgam, 1, 0, 0, 0);

  signgam = 0;
  check_float ("gamma (0.5) == log(sqrt(pi))",  FUNC(gamma) (0.5), M_LOG_SQRT_PIl, 0, 0, 0);
  check_int ("gamma (0.5) sets signgam to 1", signgam, 1, 0, 0, 0);
  signgam = 0;
  check_float ("gamma (-0.5) == log(2*sqrt(pi))",  FUNC(gamma) (-0.5), M_LOG_2_SQRT_PIl, DELTA2655, 0, 0);
  check_int ("gamma (-0.5) sets signgam to -1", signgam, -1, 0, 0, 0);

  print_max_error ("gamma", DELTAgamma, 0);
}

static void
hypot_test (void)
{
  errno = 0;
  FUNC(hypot) (0.7L, 12.4L);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_float ("hypot (inf, 1) == inf",  FUNC(hypot) (plus_infty, 1), plus_infty, 0, 0, IGNORE_ZERO_INF_SIGN);
  check_float ("hypot (-inf, 1) == inf",  FUNC(hypot) (minus_infty, 1), plus_infty, 0, 0, IGNORE_ZERO_INF_SIGN);

#ifndef TEST_INLINE
  check_float ("hypot (inf, NaN) == inf",  FUNC(hypot) (plus_infty, nan_value), plus_infty, 0, 0, 0);
  check_float ("hypot (-inf, NaN) == inf",  FUNC(hypot) (minus_infty, nan_value), plus_infty, 0, 0, 0);
  check_float ("hypot (NaN, inf) == inf",  FUNC(hypot) (nan_value, plus_infty), plus_infty, 0, 0, 0);
  check_float ("hypot (NaN, -inf) == inf",  FUNC(hypot) (nan_value, minus_infty), plus_infty, 0, 0, 0);
#endif

  check_float ("hypot (NaN, NaN) == NaN",  FUNC(hypot) (nan_value, nan_value), nan_value, 0, 0, 0);

  /* hypot (x,y) == hypot (+-x, +-y)  */
  check_float ("hypot (0.7, 12.4) == 12.419742348374220601176836866763271",  FUNC(hypot) (0.7L, 12.4L), 12.419742348374220601176836866763271L, DELTA2664, 0, 0);
  check_float ("hypot (-0.7, 12.4) == 12.419742348374220601176836866763271",  FUNC(hypot) (-0.7L, 12.4L), 12.419742348374220601176836866763271L, DELTA2665, 0, 0);
  check_float ("hypot (0.7, -12.4) == 12.419742348374220601176836866763271",  FUNC(hypot) (0.7L, -12.4L), 12.419742348374220601176836866763271L, DELTA2666, 0, 0);
  check_float ("hypot (-0.7, -12.4) == 12.419742348374220601176836866763271",  FUNC(hypot) (-0.7L, -12.4L), 12.419742348374220601176836866763271L, DELTA2667, 0, 0);
  check_float ("hypot (12.4, 0.7) == 12.419742348374220601176836866763271",  FUNC(hypot) (12.4L, 0.7L), 12.419742348374220601176836866763271L, DELTA2668, 0, 0);
  check_float ("hypot (-12.4, 0.7) == 12.419742348374220601176836866763271",  FUNC(hypot) (-12.4L, 0.7L), 12.419742348374220601176836866763271L, DELTA2669, 0, 0);
  check_float ("hypot (12.4, -0.7) == 12.419742348374220601176836866763271",  FUNC(hypot) (12.4L, -0.7L), 12.419742348374220601176836866763271L, DELTA2670, 0, 0);
  check_float ("hypot (-12.4, -0.7) == 12.419742348374220601176836866763271",  FUNC(hypot) (-12.4L, -0.7L), 12.419742348374220601176836866763271L, DELTA2671, 0, 0);

  /*  hypot (x,0) == fabs (x)  */
  check_float ("hypot (0.75, 0) == 0.75",  FUNC(hypot) (0.75L, 0), 0.75L, 0, 0, 0);
  check_float ("hypot (-0.75, 0) == 0.75",  FUNC(hypot) (-0.75L, 0), 0.75L, 0, 0, 0);
  check_float ("hypot (-5.7e7, 0) == 5.7e7",  FUNC(hypot) (-5.7e7, 0), 5.7e7L, 0, 0, 0);

  check_float ("hypot (0.75, 1.25) == 1.45773797371132511771853821938639577",  FUNC(hypot) (0.75L, 1.25L), 1.45773797371132511771853821938639577L, 0, 0, 0);

  check_float ("hypot (1.0, 0x1p-61) == 1.0",  FUNC(hypot) (1.0L, 0x1p-61L), 1.0L, 0, 0, 0);
#if defined TEST_LDOUBLE && LDBL_MANT_DIG >= 106
  check_float ("hypot (0x1.23456789abcdef0123456789ab8p-500, 0x1.23456789abcdef0123456789ab8p-500) == 4.9155782399407039128612180934736799735113e-151",  FUNC(hypot) (0x1.23456789abcdef0123456789ab8p-500L, 0x1.23456789abcdef0123456789ab8p-500L), 4.9155782399407039128612180934736799735113e-151L, 0, 0, 0);
#endif

#if !(defined TEST_FLOAT && defined TEST_INLINE)
  check_float ("hypot (0x3p125, 0x4p125) == 0x5p125",  FUNC(hypot) (0x3p125L, 0x4p125L), 0x5p125L, 0, 0, 0);
  check_float ("hypot (0x1.234566p-126, 0x1.234566p-126) == 1.891441686191081936598531534017449451173e-38",  FUNC(hypot) (0x1.234566p-126L, 0x1.234566p-126L), 1.891441686191081936598531534017449451173e-38L, 0, 0, 0);
#endif

#if !defined TEST_FLOAT && !(defined TEST_DOUBLE && defined TEST_INLINE)
  check_float ("hypot (0x3p1021, 0x4p1021) == 0x5p1021",  FUNC(hypot) (0x3p1021L, 0x4p1021L), 0x5p1021L, 0, 0, 0);
#endif

#if defined TEST_LDOUBLE && LDBL_MAX_EXP >= 16384 && !defined TEST_INLINE
  check_float ("hypot (0x3p16381, 0x4p16381) == 0x5p16381",  FUNC(hypot) (0x3p16381L, 0x4p16381L), 0x5p16381L, 0, 0, 0);
#endif

  print_max_error ("hypot", DELTAhypot, 0);
}


static void
ilogb_test (void)
{
  init_max_error ();

  check_int ("ilogb (1) == 0",  FUNC(ilogb) (1), 0, 0, 0, 0);
  check_int ("ilogb (e) == 1",  FUNC(ilogb) (M_El), 1, 0, 0, 0);
  check_int ("ilogb (1024) == 10",  FUNC(ilogb) (1024), 10, 0, 0, 0);
  check_int ("ilogb (-2000) == 10",  FUNC(ilogb) (-2000), 10, 0, 0, 0);

  /* ilogb (0.0) == FP_ILOGB0 plus invalid exception  */
  errno = 0;
  check_int ("ilogb (0.0) == FP_ILOGB0",  FUNC(ilogb) (0.0), FP_ILOGB0, 0, 0, INVALID_EXCEPTION);
  check_int ("errno for ilogb(0.0) unchanged", errno, EDOM, 0, 0, 0);
  /* ilogb (NaN) == FP_ILOGBNAN plus invalid exception  */
  errno = 0;
  check_int ("ilogb (NaN) == FP_ILOGBNAN",  FUNC(ilogb) (nan_value), FP_ILOGBNAN, 0, 0, INVALID_EXCEPTION);
  check_int ("errno for ilogb(NaN) unchanged", errno, EDOM, 0, 0, 0);
  /* ilogb (inf) == INT_MAX plus invalid exception  */
  errno = 0;
  check_int ("ilogb (inf) == INT_MAX",  FUNC(ilogb) (plus_infty), INT_MAX, 0, 0, INVALID_EXCEPTION);
  check_int ("errno for ilogb(Inf) unchanged", errno, EDOM, 0, 0, 0);
  /* ilogb (-inf) == INT_MAX plus invalid exception  */
  errno = 0;
  check_int ("ilogb (-inf) == INT_MAX",  FUNC(ilogb) (minus_infty), INT_MAX, 0, 0, INVALID_EXCEPTION);
  check_int ("errno for ilogb(-Inf) unchanged", errno, EDOM, 0, 0, 0);

  print_max_error ("ilogb", 0, 0);
}

static void
isfinite_test (void)
{
  init_max_error ();

  check_bool ("isfinite (0) == true", isfinite (0), 1, 0, 0, 0);
  check_bool ("isfinite (-0) == true", isfinite (minus_zero), 1, 0, 0, 0);
  check_bool ("isfinite (10) == true", isfinite (10), 1, 0, 0, 0);
  check_bool ("isfinite (min_subnorm_value) == true", isfinite (min_subnorm_value), 1, 0, 0, 0);
  check_bool ("isfinite (inf) == false", isfinite (plus_infty), 0, 0, 0, 0);
  check_bool ("isfinite (-inf) == false", isfinite (minus_infty), 0, 0, 0, 0);
  check_bool ("isfinite (NaN) == false", isfinite (nan_value), 0, 0, 0, 0);

  print_max_error ("isfinite", 0, 0);
}

static void
isgreater_test (void)
{
  init_max_error ();

  check_int ("isgreater (-0, -0) == 0", isgreater (minus_zero, minus_zero), 0, 0, 0, 0);
  check_int ("isgreater (-0, +0) == 0", isgreater (minus_zero, plus_zero), 0, 0, 0, 0);
  check_int ("isgreater (-0, (FLOAT) 1) == 0", isgreater (minus_zero, (FLOAT) 1), 0, 0, 0, 0);
  check_int ("isgreater (-0, NaN) == 0", isgreater (minus_zero, nan_value), 0, 0, 0, 0);
  check_int ("isgreater (+0, -0) == 0", isgreater (plus_zero, minus_zero), 0, 0, 0, 0);
  check_int ("isgreater (+0, +0) == 0", isgreater (plus_zero, plus_zero), 0, 0, 0, 0);
  check_int ("isgreater (+0, (FLOAT) 1) == 0", isgreater (plus_zero, (FLOAT) 1), 0, 0, 0, 0);
  check_int ("isgreater (+0, NaN) == 0", isgreater (plus_zero, nan_value), 0, 0, 0, 0);
  check_int ("isgreater ((FLOAT) 1, -0) == 1", isgreater ((FLOAT) 1, minus_zero), 1, 0, 0, 0);
  check_int ("isgreater ((FLOAT) 1, +0) == 1", isgreater ((FLOAT) 1, plus_zero), 1, 0, 0, 0);
  check_int ("isgreater ((FLOAT) 1, (FLOAT) 1) == 0", isgreater ((FLOAT) 1, (FLOAT) 1), 0, 0, 0, 0);
  check_int ("isgreater ((FLOAT) 1, NaN) == 0", isgreater ((FLOAT) 1, nan_value), 0, 0, 0, 0);
  check_int ("isgreater (NaN, -0) == 0", isgreater (nan_value, minus_zero), 0, 0, 0, 0);
  check_int ("isgreater (NaN, +0) == 0", isgreater (nan_value, plus_zero), 0, 0, 0, 0);
  check_int ("isgreater (NaN, (FLOAT) 1) == 0", isgreater (nan_value, (FLOAT) 1), 0, 0, 0, 0);
  check_int ("isgreater (NaN, NaN) == 0", isgreater (nan_value, nan_value), 0, 0, 0, 0);

  print_max_error ("isgreater", 0, 0);
}

static void
isgreaterequal_test (void)
{
  init_max_error ();

  check_int ("isgreaterequal (-0, -0) == 1", isgreaterequal (minus_zero, minus_zero), 1, 0, 0, 0);
  check_int ("isgreaterequal (-0, +0) == 1", isgreaterequal (minus_zero, plus_zero), 1, 0, 0, 0);
  check_int ("isgreaterequal (-0, (FLOAT) 1) == 0", isgreaterequal (minus_zero, (FLOAT) 1), 0, 0, 0, 0);
  check_int ("isgreaterequal (-0, NaN) == 0", isgreaterequal (minus_zero, nan_value), 0, 0, 0, 0);
  check_int ("isgreaterequal (+0, -0) == 1", isgreaterequal (plus_zero, minus_zero), 1, 0, 0, 0);
  check_int ("isgreaterequal (+0, +0) == 1", isgreaterequal (plus_zero, plus_zero), 1, 0, 0, 0);
  check_int ("isgreaterequal (+0, (FLOAT) 1) == 0", isgreaterequal (plus_zero, (FLOAT) 1), 0, 0, 0, 0);
  check_int ("isgreaterequal (+0, NaN) == 0", isgreaterequal (plus_zero, nan_value), 0, 0, 0, 0);
  check_int ("isgreaterequal ((FLOAT) 1, -0) == 1", isgreaterequal ((FLOAT) 1, minus_zero), 1, 0, 0, 0);
  check_int ("isgreaterequal ((FLOAT) 1, +0) == 1", isgreaterequal ((FLOAT) 1, plus_zero), 1, 0, 0, 0);
  check_int ("isgreaterequal ((FLOAT) 1, (FLOAT) 1) == 1", isgreaterequal ((FLOAT) 1, (FLOAT) 1), 1, 0, 0, 0);
  check_int ("isgreaterequal ((FLOAT) 1, NaN) == 0", isgreaterequal ((FLOAT) 1, nan_value), 0, 0, 0, 0);
  check_int ("isgreaterequal (NaN, -0) == 0", isgreaterequal (nan_value, minus_zero), 0, 0, 0, 0);
  check_int ("isgreaterequal (NaN, +0) == 0", isgreaterequal (nan_value, plus_zero), 0, 0, 0, 0);
  check_int ("isgreaterequal (NaN, (FLOAT) 1) == 0", isgreaterequal (nan_value, (FLOAT) 1), 0, 0, 0, 0);
  check_int ("isgreaterequal (NaN, NaN) == 0", isgreaterequal (nan_value, nan_value), 0, 0, 0, 0);

  print_max_error ("isgreaterequal", 0, 0);
}

static void
isinf_test (void)
{
  init_max_error ();

  check_bool ("isinf (0) == false", isinf (0), 0, 0, 0, 0);
  check_bool ("isinf (-0) == false", isinf (minus_zero), 0, 0, 0, 0);
  check_bool ("isinf (10) == false", isinf (10), 0, 0, 0, 0);
  check_bool ("isinf (min_subnorm_value) == false", isinf (min_subnorm_value), 0, 0, 0, 0);
  check_bool ("isinf (inf) == true", isinf (plus_infty), 1, 0, 0, 0);
  check_bool ("isinf (-inf) == true", isinf (minus_infty), 1, 0, 0, 0);
  check_bool ("isinf (NaN) == false", isinf (nan_value), 0, 0, 0, 0);

  print_max_error ("isinf", 0, 0);
}

static void
isless_test (void)
{
  init_max_error ();

  check_int ("isless (-0, -0) == 0", isless (minus_zero, minus_zero), 0, 0, 0, 0);
  check_int ("isless (-0, +0) == 0", isless (minus_zero, plus_zero), 0, 0, 0, 0);
  check_int ("isless (-0, (FLOAT) 1) == 1", isless (minus_zero, (FLOAT) 1), 1, 0, 0, 0);
  check_int ("isless (-0, NaN) == 0", isless (minus_zero, nan_value), 0, 0, 0, 0);
  check_int ("isless (+0, -0) == 0", isless (plus_zero, minus_zero), 0, 0, 0, 0);
  check_int ("isless (+0, +0) == 0", isless (plus_zero, plus_zero), 0, 0, 0, 0);
  check_int ("isless (+0, (FLOAT) 1) == 1", isless (plus_zero, (FLOAT) 1), 1, 0, 0, 0);
  check_int ("isless (+0, NaN) == 0", isless (plus_zero, nan_value), 0, 0, 0, 0);
  check_int ("isless ((FLOAT) 1, -0) == 0", isless ((FLOAT) 1, minus_zero), 0, 0, 0, 0);
  check_int ("isless ((FLOAT) 1, +0) == 0", isless ((FLOAT) 1, plus_zero), 0, 0, 0, 0);
  check_int ("isless ((FLOAT) 1, (FLOAT) 1) == 0", isless ((FLOAT) 1, (FLOAT) 1), 0, 0, 0, 0);
  check_int ("isless ((FLOAT) 1, NaN) == 0", isless ((FLOAT) 1, nan_value), 0, 0, 0, 0);
  check_int ("isless (NaN, -0) == 0", isless (nan_value, minus_zero), 0, 0, 0, 0);
  check_int ("isless (NaN, +0) == 0", isless (nan_value, plus_zero), 0, 0, 0, 0);
  check_int ("isless (NaN, (FLOAT) 1) == 0", isless (nan_value, (FLOAT) 1), 0, 0, 0, 0);
  check_int ("isless (NaN, NaN) == 0", isless (nan_value, nan_value), 0, 0, 0, 0);

  print_max_error ("isless", 0, 0);
}

static void
islessequal_test (void)
{
  init_max_error ();

  check_int ("islessequal (-0, -0) == 1", islessequal (minus_zero, minus_zero), 1, 0, 0, 0);
  check_int ("islessequal (-0, +0) == 1", islessequal (minus_zero, plus_zero), 1, 0, 0, 0);
  check_int ("islessequal (-0, (FLOAT) 1) == 1", islessequal (minus_zero, (FLOAT) 1), 1, 0, 0, 0);
  check_int ("islessequal (-0, NaN) == 0", islessequal (minus_zero, nan_value), 0, 0, 0, 0);
  check_int ("islessequal (+0, -0) == 1", islessequal (plus_zero, minus_zero), 1, 0, 0, 0);
  check_int ("islessequal (+0, +0) == 1", islessequal (plus_zero, plus_zero), 1, 0, 0, 0);
  check_int ("islessequal (+0, (FLOAT) 1) == 1", islessequal (plus_zero, (FLOAT) 1), 1, 0, 0, 0);
  check_int ("islessequal (+0, NaN) == 0", islessequal (plus_zero, nan_value), 0, 0, 0, 0);
  check_int ("islessequal ((FLOAT) 1, -0) == 0", islessequal ((FLOAT) 1, minus_zero), 0, 0, 0, 0);
  check_int ("islessequal ((FLOAT) 1, +0) == 0", islessequal ((FLOAT) 1, plus_zero), 0, 0, 0, 0);
  check_int ("islessequal ((FLOAT) 1, (FLOAT) 1) == 1", islessequal ((FLOAT) 1, (FLOAT) 1), 1, 0, 0, 0);
  check_int ("islessequal ((FLOAT) 1, NaN) == 0", islessequal ((FLOAT) 1, nan_value), 0, 0, 0, 0);
  check_int ("islessequal (NaN, -0) == 0", islessequal (nan_value, minus_zero), 0, 0, 0, 0);
  check_int ("islessequal (NaN, +0) == 0", islessequal (nan_value, plus_zero), 0, 0, 0, 0);
  check_int ("islessequal (NaN, (FLOAT) 1) == 0", islessequal (nan_value, (FLOAT) 1), 0, 0, 0, 0);
  check_int ("islessequal (NaN, NaN) == 0", islessequal (nan_value, nan_value), 0, 0, 0, 0);

  print_max_error ("islessequal", 0, 0);
}

static void
islessgreater_test (void)
{
  init_max_error ();

  check_int ("islessgreater (-0, -0) == 0", islessgreater (minus_zero, minus_zero), 0, 0, 0, 0);
  check_int ("islessgreater (-0, +0) == 0", islessgreater (minus_zero, plus_zero), 0, 0, 0, 0);
  check_int ("islessgreater (-0, (FLOAT) 1) == 1", islessgreater (minus_zero, (FLOAT) 1), 1, 0, 0, 0);
  check_int ("islessgreater (-0, NaN) == 0", islessgreater (minus_zero, nan_value), 0, 0, 0, 0);
  check_int ("islessgreater (+0, -0) == 0", islessgreater (plus_zero, minus_zero), 0, 0, 0, 0);
  check_int ("islessgreater (+0, +0) == 0", islessgreater (plus_zero, plus_zero), 0, 0, 0, 0);
  check_int ("islessgreater (+0, (FLOAT) 1) == 1", islessgreater (plus_zero, (FLOAT) 1), 1, 0, 0, 0);
  check_int ("islessgreater (+0, NaN) == 0", islessgreater (plus_zero, nan_value), 0, 0, 0, 0);
  check_int ("islessgreater ((FLOAT) 1, -0) == 1", islessgreater ((FLOAT) 1, minus_zero), 1, 0, 0, 0);
  check_int ("islessgreater ((FLOAT) 1, +0) == 1", islessgreater ((FLOAT) 1, plus_zero), 1, 0, 0, 0);
  check_int ("islessgreater ((FLOAT) 1, (FLOAT) 1) == 0", islessgreater ((FLOAT) 1, (FLOAT) 1), 0, 0, 0, 0);
  check_int ("islessgreater ((FLOAT) 1, NaN) == 0", islessgreater ((FLOAT) 1, nan_value), 0, 0, 0, 0);
  check_int ("islessgreater (NaN, -0) == 0", islessgreater (nan_value, minus_zero), 0, 0, 0, 0);
  check_int ("islessgreater (NaN, +0) == 0", islessgreater (nan_value, plus_zero), 0, 0, 0, 0);
  check_int ("islessgreater (NaN, (FLOAT) 1) == 0", islessgreater (nan_value, (FLOAT) 1), 0, 0, 0, 0);
  check_int ("islessgreater (NaN, NaN) == 0", islessgreater (nan_value, nan_value), 0, 0, 0, 0);

  print_max_error ("islessgreater", 0, 0);
}

static void
isnan_test (void)
{
  init_max_error ();

  check_bool ("isnan (0) == false", isnan (0), 0, 0, 0, 0);
  check_bool ("isnan (-0) == false", isnan (minus_zero), 0, 0, 0, 0);
  check_bool ("isnan (10) == false", isnan (10), 0, 0, 0, 0);
  check_bool ("isnan (min_subnorm_value) == false", isnan (min_subnorm_value), 0, 0, 0, 0);
  check_bool ("isnan (inf) == false", isnan (plus_infty), 0, 0, 0, 0);
  check_bool ("isnan (-inf) == false", isnan (minus_infty), 0, 0, 0, 0);
  check_bool ("isnan (NaN) == true", isnan (nan_value), 1, 0, 0, 0);

  print_max_error ("isnan", 0, 0);
}

static void
isnormal_test (void)
{
  init_max_error ();

  check_bool ("isnormal (0) == false", isnormal (0), 0, 0, 0, 0);
  check_bool ("isnormal (-0) == false", isnormal (minus_zero), 0, 0, 0, 0);
  check_bool ("isnormal (10) == true", isnormal (10), 1, 0, 0, 0);
  check_bool ("isnormal (min_subnorm_value) == false", isnormal (min_subnorm_value), 0, 0, 0, 0);
  check_bool ("isnormal (inf) == false", isnormal (plus_infty), 0, 0, 0, 0);
  check_bool ("isnormal (-inf) == false", isnormal (minus_infty), 0, 0, 0, 0);
  check_bool ("isnormal (NaN) == false", isnormal (nan_value), 0, 0, 0, 0);

  print_max_error ("isnormal", 0, 0);
}

static void
isunordered_test (void)
{
  init_max_error ();

  check_int ("isunordered (-0, -0) == 0", isunordered (minus_zero, minus_zero), 0, 0, 0, 0);
  check_int ("isunordered (-0, +0) == 0", isunordered (minus_zero, plus_zero), 0, 0, 0, 0);
  check_int ("isunordered (-0, (FLOAT) 1) == 0", isunordered (minus_zero, (FLOAT) 1), 0, 0, 0, 0);
  check_int ("isunordered (-0, NaN) == 1", isunordered (minus_zero, nan_value), 1, 0, 0, 0);
  check_int ("isunordered (+0, -0) == 0", isunordered (plus_zero, minus_zero), 0, 0, 0, 0);
  check_int ("isunordered (+0, +0) == 0", isunordered (plus_zero, plus_zero), 0, 0, 0, 0);
  check_int ("isunordered (+0, (FLOAT) 1) == 0", isunordered (plus_zero, (FLOAT) 1), 0, 0, 0, 0);
  check_int ("isunordered (+0, NaN) == 1", isunordered (plus_zero, nan_value), 1, 0, 0, 0);
  check_int ("isunordered ((FLOAT) 1, -0) == 0", isunordered ((FLOAT) 1, minus_zero), 0, 0, 0, 0);
  check_int ("isunordered ((FLOAT) 1, +0) == 0", isunordered ((FLOAT) 1, plus_zero), 0, 0, 0, 0);
  check_int ("isunordered ((FLOAT) 1, (FLOAT) 1) == 0", isunordered ((FLOAT) 1, (FLOAT) 1), 0, 0, 0, 0);
  check_int ("isunordered ((FLOAT) 1, NaN) == 1", isunordered ((FLOAT) 1, nan_value), 1, 0, 0, 0);
  check_int ("isunordered (NaN, -0) == 1", isunordered (nan_value, minus_zero), 1, 0, 0, 0);
  check_int ("isunordered (NaN, +0) == 1", isunordered (nan_value, plus_zero), 1, 0, 0, 0);
  check_int ("isunordered (NaN, (FLOAT) 1) == 1", isunordered (nan_value, (FLOAT) 1), 1, 0, 0, 0);
  check_int ("isunordered (NaN, NaN) == 1", isunordered (nan_value, nan_value), 1, 0, 0, 0);

  print_max_error ("isunordered", 0, 0);
}

static void
j0_test (void)
{
  FLOAT s, c;
  errno = 0;
  FUNC (sincos) (0, &s, &c);
  if (errno == ENOSYS)
    /* Required function not implemented.  */
    return;
  FUNC(j0) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  /* j0 is the Bessel function of the first kind of order 0 */
  check_float ("j0 (NaN) == NaN",  FUNC(j0) (nan_value), nan_value, 0, 0, 0);
  check_float ("j0 (inf) == 0",  FUNC(j0) (plus_infty), 0, 0, 0, 0);
  check_float ("j0 (-1.0) == 0.765197686557966551449717526102663221",  FUNC(j0) (-1.0), 0.765197686557966551449717526102663221L, 0, 0, 0);
  check_float ("j0 (0.0) == 1.0",  FUNC(j0) (0.0), 1.0, 0, 0, 0);
  check_float ("j0 (0.125) == 0.996097563041985204620768999453174712",  FUNC(j0) (0.125L), 0.996097563041985204620768999453174712L, 0, 0, 0);
  check_float ("j0 (0.75) == 0.864242275166648623555731103820923211",  FUNC(j0) (0.75L), 0.864242275166648623555731103820923211L, DELTA2819, 0, 0);
  check_float ("j0 (1.0) == 0.765197686557966551449717526102663221",  FUNC(j0) (1.0), 0.765197686557966551449717526102663221L, 0, 0, 0);
  check_float ("j0 (1.5) == 0.511827671735918128749051744283411720",  FUNC(j0) (1.5), 0.511827671735918128749051744283411720L, 0, 0, 0);
  check_float ("j0 (2.0) == 0.223890779141235668051827454649948626",  FUNC(j0) (2.0), 0.223890779141235668051827454649948626L, DELTA2822, 0, 0);
  check_float ("j0 (8.0) == 0.171650807137553906090869407851972001",  FUNC(j0) (8.0), 0.171650807137553906090869407851972001L, DELTA2823, 0, 0);
  check_float ("j0 (10.0) == -0.245935764451348335197760862485328754",  FUNC(j0) (10.0), -0.245935764451348335197760862485328754L, DELTA2824, 0, 0);
  check_float ("j0 (4.0) == -3.9714980986384737228659076845169804197562E-1",  FUNC(j0) (4.0), -3.9714980986384737228659076845169804197562E-1L, DELTA2825, 0, 0);
  check_float ("j0 (-4.0) == -3.9714980986384737228659076845169804197562E-1",  FUNC(j0) (-4.0), -3.9714980986384737228659076845169804197562E-1L, DELTA2826, 0, 0);

  /* Bug 14155: spurious exception may occur.  */
  check_float ("j0 (0x1.d7ce3ap+107) == 2.775523647291230802651040996274861694514e-17",  FUNC(j0) (0x1.d7ce3ap+107L), 2.775523647291230802651040996274861694514e-17L, DELTA2827, 0, UNDERFLOW_EXCEPTION_OK);

#ifndef TEST_FLOAT
  /* Bug 14155: spurious exception may occur.  */
  check_float ("j0 (-0x1.001000001p+593) == -3.927269966354206207832593635798954916263e-90",  FUNC(j0) (-0x1.001000001p+593L), -3.927269966354206207832593635798954916263e-90L, DELTA2828, 0, UNDERFLOW_EXCEPTION_OK);
#endif

  print_max_error ("j0", DELTAj0, 0);
}


static void
j1_test (void)
{
  FLOAT s, c;
  errno = 0;
  FUNC (sincos) (0, &s, &c);
  if (errno == ENOSYS)
    /* Required function not implemented.  */
    return;
  FUNC(j1) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  /* j1 is the Bessel function of the first kind of order 1 */

  init_max_error ();

  check_float ("j1 (NaN) == NaN",  FUNC(j1) (nan_value), nan_value, 0, 0, 0);
  check_float ("j1 (inf) == 0",  FUNC(j1) (plus_infty), 0, 0, 0, 0);

  check_float ("j1 (-1.0) == -0.440050585744933515959682203718914913",  FUNC(j1) (-1.0), -0.440050585744933515959682203718914913L, 0, 0, 0);
  check_float ("j1 (0.0) == 0.0",  FUNC(j1) (0.0), 0.0, 0, 0, 0);
  check_float ("j1 (0.125) == 0.0623780091344946810942311355879361177",  FUNC(j1) (0.125L), 0.0623780091344946810942311355879361177L, 0, 0, 0);
  check_float ("j1 (0.75) == 0.349243602174862192523281016426251335",  FUNC(j1) (0.75L), 0.349243602174862192523281016426251335L, 0, 0, 0);
  check_float ("j1 (1.0) == 0.440050585744933515959682203718914913",  FUNC(j1) (1.0), 0.440050585744933515959682203718914913L, 0, 0, 0);
  check_float ("j1 (1.5) == 0.557936507910099641990121213156089400",  FUNC(j1) (1.5), 0.557936507910099641990121213156089400L, 0, 0, 0);
  check_float ("j1 (2.0) == 0.576724807756873387202448242269137087",  FUNC(j1) (2.0), 0.576724807756873387202448242269137087L, DELTA2837, 0, 0);
  check_float ("j1 (8.0) == 0.234636346853914624381276651590454612",  FUNC(j1) (8.0), 0.234636346853914624381276651590454612L, DELTA2838, 0, 0);
  check_float ("j1 (10.0) == 0.0434727461688614366697487680258592883",  FUNC(j1) (10.0), 0.0434727461688614366697487680258592883L, DELTA2839, 0, 0);

  check_float ("j1 (0x1.3ffp+74) == 1.818984347516051243459364437186082741567e-12",  FUNC(j1) (0x1.3ffp+74L), 1.818984347516051243459364437186082741567e-12L, DELTA2840, 0, 0);

#ifndef TEST_FLOAT
  /* Bug 14155: spurious exception may occur.  */
  check_float ("j1 (0x1.ff00000000002p+840) == 1.846591691699331493194965158699937660696e-127",  FUNC(j1) (0x1.ff00000000002p+840L), 1.846591691699331493194965158699937660696e-127L, DELTA2841, 0, UNDERFLOW_EXCEPTION_OK);
#endif

  print_max_error ("j1", DELTAj1, 0);
}

static void
jn_test (void)
{
  FLOAT s, c;
  errno = 0;
  FUNC (sincos) (0, &s, &c);
  if (errno == ENOSYS)
    /* Required function not implemented.  */
    return;
  FUNC(jn) (1, 1);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  /* jn is the Bessel function of the first kind of order n.  */
  init_max_error ();

  /* jn (0, x) == j0 (x)  */
  check_float ("jn (0, NaN) == NaN",  FUNC(jn) (0, nan_value), nan_value, 0, 0, 0);
  check_float ("jn (0, inf) == 0",  FUNC(jn) (0, plus_infty), 0, 0, 0, 0);
  check_float ("jn (0, -1.0) == 0.765197686557966551449717526102663221",  FUNC(jn) (0, -1.0), 0.765197686557966551449717526102663221L, 0, 0, 0);
  check_float ("jn (0, 0.0) == 1.0",  FUNC(jn) (0, 0.0), 1.0, 0, 0, 0);
  check_float ("jn (0, 0.125) == 0.996097563041985204620768999453174712",  FUNC(jn) (0, 0.125L), 0.996097563041985204620768999453174712L, 0, 0, 0);
  check_float ("jn (0, 0.75) == 0.864242275166648623555731103820923211",  FUNC(jn) (0, 0.75L), 0.864242275166648623555731103820923211L, DELTA2847, 0, 0);
  check_float ("jn (0, 1.0) == 0.765197686557966551449717526102663221",  FUNC(jn) (0, 1.0), 0.765197686557966551449717526102663221L, 0, 0, 0);
  check_float ("jn (0, 1.5) == 0.511827671735918128749051744283411720",  FUNC(jn) (0, 1.5), 0.511827671735918128749051744283411720L, 0, 0, 0);
  check_float ("jn (0, 2.0) == 0.223890779141235668051827454649948626",  FUNC(jn) (0, 2.0), 0.223890779141235668051827454649948626L, DELTA2850, 0, 0);
  check_float ("jn (0, 8.0) == 0.171650807137553906090869407851972001",  FUNC(jn) (0, 8.0), 0.171650807137553906090869407851972001L, DELTA2851, 0, 0);
  check_float ("jn (0, 10.0) == -0.245935764451348335197760862485328754",  FUNC(jn) (0, 10.0), -0.245935764451348335197760862485328754L, DELTA2852, 0, 0);
  check_float ("jn (0, 4.0) == -3.9714980986384737228659076845169804197562E-1",  FUNC(jn) (0, 4.0), -3.9714980986384737228659076845169804197562E-1L, DELTA2853, 0, 0);
  check_float ("jn (0, -4.0) == -3.9714980986384737228659076845169804197562E-1",  FUNC(jn) (0, -4.0), -3.9714980986384737228659076845169804197562E-1L, DELTA2854, 0, 0);

  /* jn (1, x) == j1 (x)  */
  check_float ("jn (1, NaN) == NaN",  FUNC(jn) (1, nan_value), nan_value, 0, 0, 0);
  check_float ("jn (1, inf) == 0",  FUNC(jn) (1, plus_infty), 0, 0, 0, 0);
  check_float ("jn (1, -1.0) == -0.440050585744933515959682203718914913",  FUNC(jn) (1, -1.0), -0.440050585744933515959682203718914913L, 0, 0, 0);
  check_float ("jn (1, 0.0) == 0.0",  FUNC(jn) (1, 0.0), 0.0, 0, 0, 0);
  check_float ("jn (1, 0.125) == 0.0623780091344946810942311355879361177",  FUNC(jn) (1, 0.125L), 0.0623780091344946810942311355879361177L, 0, 0, 0);
  check_float ("jn (1, 0.75) == 0.349243602174862192523281016426251335",  FUNC(jn) (1, 0.75L), 0.349243602174862192523281016426251335L, 0, 0, 0);
  check_float ("jn (1, 1.0) == 0.440050585744933515959682203718914913",  FUNC(jn) (1, 1.0), 0.440050585744933515959682203718914913L, 0, 0, 0);
  check_float ("jn (1, 1.5) == 0.557936507910099641990121213156089400",  FUNC(jn) (1, 1.5), 0.557936507910099641990121213156089400L, 0, 0, 0);
  check_float ("jn (1, 2.0) == 0.576724807756873387202448242269137087",  FUNC(jn) (1, 2.0), 0.576724807756873387202448242269137087L, DELTA2863, 0, 0);
  check_float ("jn (1, 8.0) == 0.234636346853914624381276651590454612",  FUNC(jn) (1, 8.0), 0.234636346853914624381276651590454612L, DELTA2864, 0, 0);
  check_float ("jn (1, 10.0) == 0.0434727461688614366697487680258592883",  FUNC(jn) (1, 10.0), 0.0434727461688614366697487680258592883L, DELTA2865, 0, 0);

  /* jn (3, x)  */
  check_float ("jn (3, NaN) == NaN",  FUNC(jn) (3, nan_value), nan_value, 0, 0, 0);
  check_float ("jn (3, inf) == 0",  FUNC(jn) (3, plus_infty), 0, 0, 0, 0);

  check_float ("jn (3, -1.0) == -0.0195633539826684059189053216217515083",  FUNC(jn) (3, -1.0), -0.0195633539826684059189053216217515083L, DELTA2868, 0, 0);
  check_float ("jn (3, 0.0) == 0.0",  FUNC(jn) (3, 0.0), 0.0, 0, 0, 0);
  check_float ("jn (3, 0.125) == 0.406503832554912875023029337653442868e-4",  FUNC(jn) (3, 0.125L), 0.406503832554912875023029337653442868e-4L, DELTA2870, 0, 0);
  check_float ("jn (3, 0.75) == 0.848438342327410884392755236884386804e-2",  FUNC(jn) (3, 0.75L), 0.848438342327410884392755236884386804e-2L, DELTA2871, 0, 0);
  check_float ("jn (3, 1.0) == 0.0195633539826684059189053216217515083",  FUNC(jn) (3, 1.0), 0.0195633539826684059189053216217515083L, DELTA2872, 0, 0);
  check_float ("jn (3, 2.0) == 0.128943249474402051098793332969239835",  FUNC(jn) (3, 2.0), 0.128943249474402051098793332969239835L, DELTA2873, 0, 0);
  check_float ("jn (3, 10.0) == 0.0583793793051868123429354784103409563",  FUNC(jn) (3, 10.0), 0.0583793793051868123429354784103409563L, DELTA2874, 0, 0);

  /*  jn (10, x)  */
  check_float ("jn (10, NaN) == NaN",  FUNC(jn) (10, nan_value), nan_value, 0, 0, 0);
  check_float ("jn (10, inf) == 0",  FUNC(jn) (10, plus_infty), 0, 0, 0, 0);

  check_float ("jn (10, -1.0) == 0.263061512368745320699785368779050294e-9",  FUNC(jn) (10, -1.0), 0.263061512368745320699785368779050294e-9L, DELTA2877, 0, 0);
  check_float ("jn (10, 0.0) == 0.0",  FUNC(jn) (10, 0.0), 0.0, 0, 0, 0);
  check_float ("jn (10, 0.125) == 0.250543369809369890173993791865771547e-18",  FUNC(jn) (10, 0.125L), 0.250543369809369890173993791865771547e-18L, DELTA2879, 0, 0);
  check_float ("jn (10, 0.75) == 0.149621713117596814698712483621682835e-10",  FUNC(jn) (10, 0.75L), 0.149621713117596814698712483621682835e-10L, DELTA2880, 0, 0);
  check_float ("jn (10, 1.0) == 0.263061512368745320699785368779050294e-9",  FUNC(jn) (10, 1.0), 0.263061512368745320699785368779050294e-9L, DELTA2881, 0, 0);
  check_float ("jn (10, 2.0) == 0.251538628271673670963516093751820639e-6",  FUNC(jn) (10, 2.0), 0.251538628271673670963516093751820639e-6L, DELTA2882, 0, 0);
  check_float ("jn (10, 10.0) == 0.207486106633358857697278723518753428",  FUNC(jn) (10, 10.0), 0.207486106633358857697278723518753428L, DELTA2883, 0, 0);

  /* BZ #11589 .*/
  check_float ("jn (2, 2.4048255576957729) == 0.43175480701968038399746111312430703",  FUNC(jn) (2, 2.4048255576957729L), 0.43175480701968038399746111312430703L, DELTA2884, 0, 0);
  check_float ("jn (3, 2.4048255576957729) == 0.19899990535769083404042146764530813",  FUNC(jn) (3, 2.4048255576957729L), 0.19899990535769083404042146764530813L, DELTA2885, 0, 0);
  check_float ("jn (4, 2.4048255576957729) == 0.647466661641779720084932282551219891E-1",  FUNC(jn) (4, 2.4048255576957729L), 0.647466661641779720084932282551219891E-1L, DELTA2886, 0, 0);
  check_float ("jn (5, 2.4048255576957729) == 0.163892432048058525099230549946147698E-1",  FUNC(jn) (5, 2.4048255576957729L), 0.163892432048058525099230549946147698E-1L, DELTA2887, 0, 0);
  check_float ("jn (6, 2.4048255576957729) == 0.34048184720278336646673682895929161E-2",  FUNC(jn) (6, 2.4048255576957729L), 0.34048184720278336646673682895929161E-2L, DELTA2888, 0, 0);
  check_float ("jn (7, 2.4048255576957729) == 0.60068836573295394221291569249883076E-3",  FUNC(jn) (7, 2.4048255576957729L), 0.60068836573295394221291569249883076E-3L, DELTA2889, 0, 0);
  check_float ("jn (8, 2.4048255576957729) == 0.92165786705344923232879022467054148E-4",  FUNC(jn) (8, 2.4048255576957729L), 0.92165786705344923232879022467054148E-4L, DELTA2890, 0, 0);
  check_float ("jn (9, 2.4048255576957729) == 0.12517270977961513005428966643852564E-4",  FUNC(jn) (9, 2.4048255576957729L), 0.12517270977961513005428966643852564E-4L, DELTA2891, 0, 0);

  /* Bug 14155: spurious exception may occur.  */
  check_float ("jn (2, 0x1.ffff62p+99) == -4.43860668048170034334926693188979974489e-16",  FUNC(jn) (2, 0x1.ffff62p+99L), -4.43860668048170034334926693188979974489e-16L, DELTA2892, 0, UNDERFLOW_EXCEPTION_OK);

  print_max_error ("jn", DELTAjn, 0);
}


static void
ldexp_test (void)
{
  check_float ("jn (0, 0) == 0",  FUNC(ldexp) (0, 0), 0, 0, 0, 0);
  check_float ("jn (-0, 0) == -0",  FUNC(ldexp) (minus_zero, 0), minus_zero, 0, 0, 0);

  check_float ("jn (inf, 1) == inf",  FUNC(ldexp) (plus_infty, 1), plus_infty, 0, 0, 0);
  check_float ("jn (-inf, 1) == -inf",  FUNC(ldexp) (minus_infty, 1), minus_infty, 0, 0, 0);
  check_float ("jn (NaN, 1) == NaN",  FUNC(ldexp) (nan_value, 1), nan_value, 0, 0, 0);

  check_float ("jn (0.8, 4) == 12.8",  FUNC(ldexp) (0.8L, 4), 12.8L, 0, 0, 0);
  check_float ("jn (-0.854375, 5) == -27.34",  FUNC(ldexp) (-0.854375L, 5), -27.34L, 0, 0, 0);

  /* ldexp (x, 0) == x.  */
  check_float ("jn (1.0, 0) == 1.0",  FUNC(ldexp) (1.0L, 0L), 1.0L, 0, 0, 0);
}


static void
lgamma_test (void)
{
  errno = 0;
  FUNC(lgamma) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  signgam = 0;
  check_float ("lgamma (inf) == inf",  FUNC(lgamma) (plus_infty), plus_infty, 0, 0, 0);
  signgam = 0;
  check_float ("lgamma (0) == inf",  FUNC(lgamma) (0), plus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_int ("signgam for lgamma(0) == 1", signgam, 1, 0, 0, 0);
  signgam = 0;
  check_float ("lgamma (-0) == inf",  FUNC(lgamma) (minus_zero), plus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_int ("signgam for lgamma(-0) == -1", signgam, -1, 0, 0, 0);
  signgam = 0;
  check_float ("lgamma (NaN) == NaN",  FUNC(lgamma) (nan_value), nan_value, 0, 0, 0);

  /* lgamma (x) == +inf plus divide by zero exception for integer x <= 0.  */
  errno = 0;
  signgam = 0;
  check_float ("lgamma (-3) == inf",  FUNC(lgamma) (-3), plus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_int ("errno for lgamma(-integer) == ERANGE", errno, ERANGE, 0, 0, 0);
  signgam = 0;
  check_float ("lgamma (-inf) == inf",  FUNC(lgamma) (minus_infty), plus_infty, 0, 0, 0);
  signgam = 0;
  check_float ("lgamma (-max_value) == inf",  FUNC(lgamma) (-max_value), plus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  signgam = 0;
  check_float ("lgamma (max_value) == inf",  FUNC(lgamma) (max_value), plus_infty, 0, 0, OVERFLOW_EXCEPTION);

  signgam = 0;
  check_float ("lgamma (1) == 0",  FUNC(lgamma) (1), 0, 0, 0, 0);
  check_int ("lgamma (1) sets signgam to 1", signgam, 1, 0, 0, 0);

  signgam = 0;
  check_float ("lgamma (3) == M_LN2l",  FUNC(lgamma) (3), M_LN2l, 0, 0, 0);
  check_int ("lgamma (3) sets signgam to 1", signgam, 1, 0, 0, 0);

  signgam = 0;
  check_float ("lgamma (0.5) == log(sqrt(pi))",  FUNC(lgamma) (0.5), M_LOG_SQRT_PIl, 0, 0, 0);
  check_int ("lgamma (0.5) sets signgam to 1", signgam, 1, 0, 0, 0);
  signgam = 0;
  check_float ("lgamma (-0.5) == log(2*sqrt(pi))",  FUNC(lgamma) (-0.5), M_LOG_2_SQRT_PIl, DELTA2915, 0, 0);
  check_int ("lgamma (-0.5) sets signgam to -1", signgam, -1, 0, 0, 0);
  signgam = 0;
  check_float ("lgamma (0.7) == 0.260867246531666514385732417016759578",  FUNC(lgamma) (0.7L), 0.260867246531666514385732417016759578L, DELTA2917, 0, 0);
  check_int ("lgamma (0.7) sets signgam to 1", signgam, 1, 0, 0, 0);
  signgam = 0;
  check_float ("lgamma (1.2) == -0.853740900033158497197028392998854470e-1",  FUNC(lgamma) (1.2L), -0.853740900033158497197028392998854470e-1L, DELTA2919, 0, 0);
  check_int ("lgamma (1.2) sets signgam to 1", signgam, 1, 0, 0, 0);

  print_max_error ("lgamma", DELTAlgamma, 0);
}


static void
lrint_test (void)
{
  /* XXX this test is incomplete.  We need to have a way to specifiy
     the rounding method and test the critical cases.  So far, only
     unproblematic numbers are tested.  */

  init_max_error ();

  check_long ("lrint (0.0) == 0",  FUNC(lrint) (0.0), 0, 0, 0, 0);
  check_long ("lrint (-0) == 0",  FUNC(lrint) (minus_zero), 0, 0, 0, 0);
  check_long ("lrint (0.2) == 0",  FUNC(lrint) (0.2L), 0, 0, 0, 0);
  check_long ("lrint (-0.2) == 0",  FUNC(lrint) (-0.2L), 0, 0, 0, 0);

  check_long ("lrint (1.4) == 1",  FUNC(lrint) (1.4L), 1, 0, 0, 0);
  check_long ("lrint (-1.4) == -1",  FUNC(lrint) (-1.4L), -1, 0, 0, 0);

  check_long ("lrint (8388600.3) == 8388600",  FUNC(lrint) (8388600.3L), 8388600, 0, 0, 0);
  check_long ("lrint (-8388600.3) == -8388600",  FUNC(lrint) (-8388600.3L), -8388600, 0, 0, 0);

  check_long ("lrint (1071930.0008) == 1071930",  FUNC(lrint) (1071930.0008), 1071930, 0, 0, 0);
#ifndef TEST_FLOAT
  check_long ("lrint (1073741824.01) == 1073741824",  FUNC(lrint) (1073741824.01), 1073741824, 0, 0, 0);
# if LONG_MAX > 281474976710656
  check_long ("lrint (281474976710656.025) == 281474976710656",  FUNC(lrint) (281474976710656.025), 281474976710656, 0, 0, 0);
# endif
#endif

  print_max_error ("lrint", 0, 0);
}


static void
lrint_test_tonearest (void)
{
  int save_round_mode;
  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_TONEAREST))
    {
  check_long ("lrint_tonearest (0.0) == 0",  FUNC(lrint) (0.0), 0, 0, 0, 0);
  check_long ("lrint_tonearest (-0) == 0",  FUNC(lrint) (minus_zero), 0, 0, 0, 0);
  check_long ("lrint_tonearest (0.2) == 0",  FUNC(lrint) (0.2L), 0, 0, 0, 0);
  check_long ("lrint_tonearest (-0.2) == 0",  FUNC(lrint) (-0.2L), 0, 0, 0, 0);
  check_long ("lrint_tonearest (0.5) == 0",  FUNC(lrint) (0.5L), 0, 0, 0, 0);
  check_long ("lrint_tonearest (-0.5) == 0",  FUNC(lrint) (-0.5L), 0, 0, 0, 0);
  check_long ("lrint_tonearest (0.8) == 1",  FUNC(lrint) (0.8L), 1, 0, 0, 0);
  check_long ("lrint_tonearest (-0.8) == -1",  FUNC(lrint) (-0.8L), -1, 0, 0, 0);

  check_long ("lrint_tonearest (1.4) == 1",  FUNC(lrint) (1.4L), 1, 0, 0, 0);
  check_long ("lrint_tonearest (-1.4) == -1",  FUNC(lrint) (-1.4L), -1, 0, 0, 0);

  check_long ("lrint_tonearest (8388600.3) == 8388600",  FUNC(lrint) (8388600.3L), 8388600, 0, 0, 0);
  check_long ("lrint_tonearest (-8388600.3) == -8388600",  FUNC(lrint) (-8388600.3L), -8388600, 0, 0, 0);

  check_long ("lrint_tonearest (1071930.0008) == 1071930",  FUNC(lrint) (1071930.0008), 1071930, 0, 0, 0);
#ifndef TEST_FLOAT
  check_long ("lrint_tonearest (1073741824.01) == 1073741824",  FUNC(lrint) (1073741824.01), 1073741824, 0, 0, 0);
# if LONG_MAX > 281474976710656
  check_long ("lrint_tonearest (281474976710656.025) == 281474976710656",  FUNC(lrint) (281474976710656.025), 281474976710656, 0, 0, 0);
# endif
#endif
    }

  fesetround (save_round_mode);

  print_max_error ("lrint_tonearest", 0, 0);
}


static void
lrint_test_towardzero (void)
{
  int save_round_mode;
  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_TOWARDZERO))
    {
  check_long ("lrint_towardzero (0.0) == 0",  FUNC(lrint) (0.0), 0, 0, 0, 0);
  check_long ("lrint_towardzero (-0) == 0",  FUNC(lrint) (minus_zero), 0, 0, 0, 0);
  check_long ("lrint_towardzero (0.2) == 0",  FUNC(lrint) (0.2L), 0, 0, 0, 0);
  check_long ("lrint_towardzero (-0.2) == 0",  FUNC(lrint) (-0.2L), 0, 0, 0, 0);
  check_long ("lrint_towardzero (0.5) == 0",  FUNC(lrint) (0.5L), 0, 0, 0, 0);
  check_long ("lrint_towardzero (-0.5) == 0",  FUNC(lrint) (-0.5L), 0, 0, 0, 0);
  check_long ("lrint_towardzero (0.8) == 0",  FUNC(lrint) (0.8L), 0, 0, 0, 0);
  check_long ("lrint_towardzero (-0.8) == 0",  FUNC(lrint) (-0.8L), 0, 0, 0, 0);

  check_long ("lrint_towardzero (1.4) == 1",  FUNC(lrint) (1.4L), 1, 0, 0, 0);
  check_long ("lrint_towardzero (-1.4) == -1",  FUNC(lrint) (-1.4L), -1, 0, 0, 0);

  check_long ("lrint_towardzero (8388600.3) == 8388600",  FUNC(lrint) (8388600.3L), 8388600, 0, 0, 0);
  check_long ("lrint_towardzero (-8388600.3) == -8388600",  FUNC(lrint) (-8388600.3L), -8388600, 0, 0, 0);

  check_long ("lrint_towardzero (1071930.0008) == 1071930",  FUNC(lrint) (1071930.0008), 1071930, 0, 0, 0);
#ifndef TEST_FLOAT
  check_long ("lrint_towardzero (1073741824.01) == 1073741824",  FUNC(lrint) (1073741824.01), 1073741824, 0, 0, 0);
# if LONG_MAX > 281474976710656
  check_long ("lrint_towardzero (281474976710656.025) == 281474976710656",  FUNC(lrint) (281474976710656.025), 281474976710656, 0, 0, 0);
# endif
#endif
    }

  fesetround (save_round_mode);

  print_max_error ("lrint_towardzero", 0, 0);
}


static void
lrint_test_downward (void)
{
  int save_round_mode;
  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_DOWNWARD))
    {
  check_long ("lrint_downward (0.0) == 0",  FUNC(lrint) (0.0), 0, 0, 0, 0);
  check_long ("lrint_downward (-0) == 0",  FUNC(lrint) (minus_zero), 0, 0, 0, 0);
  check_long ("lrint_downward (0.2) == 0",  FUNC(lrint) (0.2L), 0, 0, 0, 0);
  check_long ("lrint_downward (-0.2) == -1",  FUNC(lrint) (-0.2L), -1, 0, 0, 0);
  check_long ("lrint_downward (0.5) == 0",  FUNC(lrint) (0.5L), 0, 0, 0, 0);
  check_long ("lrint_downward (-0.5) == -1",  FUNC(lrint) (-0.5L), -1, 0, 0, 0);
  check_long ("lrint_downward (0.8) == 0",  FUNC(lrint) (0.8L), 0, 0, 0, 0);
  check_long ("lrint_downward (-0.8) == -1",  FUNC(lrint) (-0.8L), -1, 0, 0, 0);

  check_long ("lrint_downward (1.4) == 1",  FUNC(lrint) (1.4L), 1, 0, 0, 0);
  check_long ("lrint_downward (-1.4) == -2",  FUNC(lrint) (-1.4L), -2, 0, 0, 0);

  check_long ("lrint_downward (8388600.3) == 8388600",  FUNC(lrint) (8388600.3L), 8388600, 0, 0, 0);
  check_long ("lrint_downward (-8388600.3) == -8388601",  FUNC(lrint) (-8388600.3L), -8388601, 0, 0, 0);

  check_long ("lrint_downward (1071930.0008) == 1071930",  FUNC(lrint) (1071930.0008), 1071930, 0, 0, 0);
#ifndef TEST_FLOAT
  check_long ("lrint_downward (1073741824.01) == 1073741824",  FUNC(lrint) (1073741824.01), 1073741824, 0, 0, 0);
# if LONG_MAX > 281474976710656
  check_long ("lrint_downward (281474976710656.025) == 281474976710656",  FUNC(lrint) (281474976710656.025), 281474976710656, 0, 0, 0);
# endif
#endif
    }

  fesetround (save_round_mode);

  print_max_error ("lrint_downward", 0, 0);
}


static void
lrint_test_upward (void)
{
  int save_round_mode;
  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_UPWARD))
    {
  check_long ("lrint_upward (0.0) == 0",  FUNC(lrint) (0.0), 0, 0, 0, 0);
  check_long ("lrint_upward (-0) == 0",  FUNC(lrint) (minus_zero), 0, 0, 0, 0);
  check_long ("lrint_upward (0.2) == 1",  FUNC(lrint) (0.2L), 1, 0, 0, 0);
  check_long ("lrint_upward (-0.2) == 0",  FUNC(lrint) (-0.2L), 0, 0, 0, 0);
  check_long ("lrint_upward (0.5) == 1",  FUNC(lrint) (0.5L), 1, 0, 0, 0);
  check_long ("lrint_upward (-0.5) == 0",  FUNC(lrint) (-0.5L), 0, 0, 0, 0);
  check_long ("lrint_upward (0.8) == 1",  FUNC(lrint) (0.8L), 1, 0, 0, 0);
  check_long ("lrint_upward (-0.8) == 0",  FUNC(lrint) (-0.8L), 0, 0, 0, 0);

  check_long ("lrint_upward (1.4) == 2",  FUNC(lrint) (1.4L), 2, 0, 0, 0);
  check_long ("lrint_upward (-1.4) == -1",  FUNC(lrint) (-1.4L), -1, 0, 0, 0);

  check_long ("lrint_upward (8388600.3) == 8388601",  FUNC(lrint) (8388600.3L), 8388601, 0, 0, 0);
  check_long ("lrint_upward (-8388600.3) == -8388600",  FUNC(lrint) (-8388600.3L), -8388600, 0, 0, 0);

#ifndef TEST_FLOAT
  check_long ("lrint_upward (1071930.0008) == 1071931",  FUNC(lrint) (1071930.0008), 1071931, 0, 0, 0);
  check_long ("lrint_upward (1073741824.01) == 1073741825",  FUNC(lrint) (1073741824.01), 1073741825, 0, 0, 0);
# if LONG_MAX > 281474976710656 && defined (TEST_LDOUBLE)
  check_long ("lrint_upward (281474976710656.025) == 281474976710656",  FUNC(lrint) (281474976710656.025), 281474976710656, 0, 0, 0);
# endif
#endif
    }

  fesetround (save_round_mode);

  print_max_error ("lrint_upward", 0, 0);
}


static void
llrint_test (void)
{
  /* XXX this test is incomplete.  We need to have a way to specifiy
     the rounding method and test the critical cases.  So far, only
     unproblematic numbers are tested.  */

  init_max_error ();

  check_longlong ("llrint (0.0) == 0",  FUNC(llrint) (0.0), 0, 0, 0, 0);
  check_longlong ("llrint (-0) == 0",  FUNC(llrint) (minus_zero), 0, 0, 0, 0);
  check_longlong ("llrint (0.2) == 0",  FUNC(llrint) (0.2L), 0, 0, 0, 0);
  check_longlong ("llrint (-0.2) == 0",  FUNC(llrint) (-0.2L), 0, 0, 0, 0);

  check_longlong ("llrint (1.4) == 1",  FUNC(llrint) (1.4L), 1, 0, 0, 0);
  check_longlong ("llrint (-1.4) == -1",  FUNC(llrint) (-1.4L), -1, 0, 0, 0);

  check_longlong ("llrint (8388600.3) == 8388600",  FUNC(llrint) (8388600.3L), 8388600, 0, 0, 0);
  check_longlong ("llrint (-8388600.3) == -8388600",  FUNC(llrint) (-8388600.3L), -8388600, 0, 0, 0);

  check_long ("llrint (1071930.0008) == 1071930",  FUNC(llrint) (1071930.0008), 1071930, 0, 0, 0);

  /* Test boundary conditions.  */
  /* 0x1FFFFF */
  check_longlong ("llrint (2097151.0) == 2097151LL",  FUNC(llrint) (2097151.0), 2097151LL, 0, 0, 0);
  /* 0x800000 */
  check_longlong ("llrint (8388608.0) == 8388608LL",  FUNC(llrint) (8388608.0), 8388608LL, 0, 0, 0);
  /* 0x1000000 */
  check_longlong ("llrint (16777216.0) == 16777216LL",  FUNC(llrint) (16777216.0), 16777216LL, 0, 0, 0);
  /* 0x20000000000 */
  check_longlong ("llrint (2199023255552.0) == 2199023255552LL",  FUNC(llrint) (2199023255552.0), 2199023255552LL, 0, 0, 0);
  /* 0x40000000000 */
  check_longlong ("llrint (4398046511104.0) == 4398046511104LL",  FUNC(llrint) (4398046511104.0), 4398046511104LL, 0, 0, 0);
  /* 0x1000000000000 */
  check_longlong ("llrint (281474976710656.0) == 281474976710656LL",  FUNC(llrint) (281474976710656.0), 281474976710656LL, 0, 0, 0);
  /* 0x10000000000000 */
  check_longlong ("llrint (4503599627370496.0) == 4503599627370496LL",  FUNC(llrint) (4503599627370496.0), 4503599627370496LL, 0, 0, 0);
  /* 0x10000080000000 */
  check_longlong ("llrint (4503601774854144.0) == 4503601774854144LL",  FUNC(llrint) (4503601774854144.0), 4503601774854144LL, 0, 0, 0);
  /* 0x20000000000000 */
  check_longlong ("llrint (9007199254740992.0) == 9007199254740992LL",  FUNC(llrint) (9007199254740992.0), 9007199254740992LL, 0, 0, 0);
  /* 0x80000000000000 */
  check_longlong ("llrint (36028797018963968.0) == 36028797018963968LL",  FUNC(llrint) (36028797018963968.0), 36028797018963968LL, 0, 0, 0);
  /* 0x100000000000000 */
  check_longlong ("llrint (72057594037927936.0) == 72057594037927936LL",  FUNC(llrint) (72057594037927936.0), 72057594037927936LL, 0, 0, 0);
#ifdef TEST_LDOUBLE
  /* The input can only be represented in long double.  */
  check_longlong ("llrint (4503599627370495.5) == 4503599627370496LL",  FUNC(llrint) (4503599627370495.5L), 4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint (4503599627370496.25) == 4503599627370496LL",  FUNC(llrint) (4503599627370496.25L), 4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint (4503599627370496.5) == 4503599627370496LL",  FUNC(llrint) (4503599627370496.5L), 4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint (4503599627370496.75) == 4503599627370497LL",  FUNC(llrint) (4503599627370496.75L), 4503599627370497LL, 0, 0, 0);
  check_longlong ("llrint (4503599627370497.5) == 4503599627370498LL",  FUNC(llrint) (4503599627370497.5L), 4503599627370498LL, 0, 0, 0);

  check_longlong ("llrint (-4503599627370495.5) == -4503599627370496LL",  FUNC(llrint) (-4503599627370495.5L), -4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint (-4503599627370496.25) == -4503599627370496LL",  FUNC(llrint) (-4503599627370496.25L), -4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint (-4503599627370496.5) == -4503599627370496LL",  FUNC(llrint) (-4503599627370496.5L), -4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint (-4503599627370496.75) == -4503599627370497LL",  FUNC(llrint) (-4503599627370496.75L), -4503599627370497LL, 0, 0, 0);
  check_longlong ("llrint (-4503599627370497.5) == -4503599627370498LL",  FUNC(llrint) (-4503599627370497.5L), -4503599627370498LL, 0, 0, 0);

# if LDBL_MANT_DIG > 100
  check_longlong ("llrint (4503599627370495.4999999999999) == 4503599627370495LL",  FUNC(llrint) (4503599627370495.4999999999999L), 4503599627370495LL, 0, 0, 0);
  check_longlong ("llrint (4503599627370496.4999999999999) == 4503599627370496LL",  FUNC(llrint) (4503599627370496.4999999999999L), 4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint (4503599627370497.4999999999999) == 4503599627370497LL",  FUNC(llrint) (4503599627370497.4999999999999L), 4503599627370497LL, 0, 0, 0);
  check_longlong ("llrint (4503599627370494.5000000000001) == 4503599627370495LL",  FUNC(llrint) (4503599627370494.5000000000001L), 4503599627370495LL, 0, 0, 0);
  check_longlong ("llrint (4503599627370495.5000000000001) == 4503599627370496LL",  FUNC(llrint) (4503599627370495.5000000000001L), 4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint (4503599627370496.5000000000001) == 4503599627370497LL",  FUNC(llrint) (4503599627370496.5000000000001L), 4503599627370497LL, 0, 0, 0);

  check_longlong ("llrint (-4503599627370495.4999999999999) == -4503599627370495LL",  FUNC(llrint) (-4503599627370495.4999999999999L), -4503599627370495LL, 0, 0, 0);
  check_longlong ("llrint (-4503599627370496.4999999999999) == -4503599627370496LL",  FUNC(llrint) (-4503599627370496.4999999999999L), -4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint (-4503599627370497.4999999999999) == -4503599627370497LL",  FUNC(llrint) (-4503599627370497.4999999999999L), -4503599627370497LL, 0, 0, 0);
  check_longlong ("llrint (-4503599627370494.5000000000001) == -4503599627370495LL",  FUNC(llrint) (-4503599627370494.5000000000001L), -4503599627370495LL, 0, 0, 0);
  check_longlong ("llrint (-4503599627370495.5000000000001) == -4503599627370496LL",  FUNC(llrint) (-4503599627370495.5000000000001L), -4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint (-4503599627370496.5000000000001) == -4503599627370497LL",  FUNC(llrint) (-4503599627370496.5000000000001L), -4503599627370497LL, 0, 0, 0);
#endif

  check_longlong ("llrint (9007199254740991.5) == 9007199254740992LL",  FUNC(llrint) (9007199254740991.5L), 9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint (9007199254740992.25) == 9007199254740992LL",  FUNC(llrint) (9007199254740992.25L), 9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint (9007199254740992.5) == 9007199254740992LL",  FUNC(llrint) (9007199254740992.5L), 9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint (9007199254740992.75) == 9007199254740993LL",  FUNC(llrint) (9007199254740992.75L), 9007199254740993LL, 0, 0, 0);
  check_longlong ("llrint (9007199254740993.5) == 9007199254740994LL",  FUNC(llrint) (9007199254740993.5L), 9007199254740994LL, 0, 0, 0);

  check_longlong ("llrint (-9007199254740991.5) == -9007199254740992LL",  FUNC(llrint) (-9007199254740991.5L), -9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint (-9007199254740992.25) == -9007199254740992LL",  FUNC(llrint) (-9007199254740992.25L), -9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint (-9007199254740992.5) == -9007199254740992LL",  FUNC(llrint) (-9007199254740992.5L), -9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint (-9007199254740992.75) == -9007199254740993LL",  FUNC(llrint) (-9007199254740992.75L), -9007199254740993LL, 0, 0, 0);
  check_longlong ("llrint (-9007199254740993.5) == -9007199254740994LL",  FUNC(llrint) (-9007199254740993.5L), -9007199254740994LL, 0, 0, 0);

# if LDBL_MANT_DIG > 100
  check_longlong ("llrint (9007199254740991.4999999999999) == 9007199254740991LL",  FUNC(llrint) (9007199254740991.4999999999999L), 9007199254740991LL, 0, 0, 0);
  check_longlong ("llrint (9007199254740992.4999999999999) == 9007199254740992LL",  FUNC(llrint) (9007199254740992.4999999999999L), 9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint (9007199254740993.4999999999999) == 9007199254740993LL",  FUNC(llrint) (9007199254740993.4999999999999L), 9007199254740993LL, 0, 0, 0);
  check_longlong ("llrint (9007199254740991.5000000000001) == 9007199254740992LL",  FUNC(llrint) (9007199254740991.5000000000001L), 9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint (9007199254740992.5000000000001) == 9007199254740993LL",  FUNC(llrint) (9007199254740992.5000000000001L), 9007199254740993LL, 0, 0, 0);
  check_longlong ("llrint (9007199254740993.5000000000001) == 9007199254740994LL",  FUNC(llrint) (9007199254740993.5000000000001L), 9007199254740994LL, 0, 0, 0);

  check_longlong ("llrint (-9007199254740991.4999999999999) == -9007199254740991LL",  FUNC(llrint) (-9007199254740991.4999999999999L), -9007199254740991LL, 0, 0, 0);
  check_longlong ("llrint (-9007199254740992.4999999999999) == -9007199254740992LL",  FUNC(llrint) (-9007199254740992.4999999999999L), -9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint (-9007199254740993.4999999999999) == -9007199254740993LL",  FUNC(llrint) (-9007199254740993.4999999999999L), -9007199254740993LL, 0, 0, 0);
  check_longlong ("llrint (-9007199254740991.5000000000001) == -9007199254740992LL",  FUNC(llrint) (-9007199254740991.5000000000001L), -9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint (-9007199254740992.5000000000001) == -9007199254740993LL",  FUNC(llrint) (-9007199254740992.5000000000001L), -9007199254740993LL, 0, 0, 0);
  check_longlong ("llrint (-9007199254740993.5000000000001) == -9007199254740994LL",  FUNC(llrint) (-9007199254740993.5000000000001L), -9007199254740994LL, 0, 0, 0);
#endif

  check_longlong ("llrint (72057594037927935.5) == 72057594037927936LL",  FUNC(llrint) (72057594037927935.5L), 72057594037927936LL, 0, 0, 0);
  check_longlong ("llrint (72057594037927936.25) == 72057594037927936LL",  FUNC(llrint) (72057594037927936.25L), 72057594037927936LL, 0, 0, 0);
  check_longlong ("llrint (72057594037927936.5) == 72057594037927936LL",  FUNC(llrint) (72057594037927936.5L), 72057594037927936LL, 0, 0, 0);
  check_longlong ("llrint (72057594037927936.75) == 72057594037927937LL",  FUNC(llrint) (72057594037927936.75L), 72057594037927937LL, 0, 0, 0);
  check_longlong ("llrint (72057594037927937.5) == 72057594037927938LL",  FUNC(llrint) (72057594037927937.5L), 72057594037927938LL, 0, 0, 0);

  check_longlong ("llrint (-72057594037927935.5) == -72057594037927936LL",  FUNC(llrint) (-72057594037927935.5L), -72057594037927936LL, 0, 0, 0);
  check_longlong ("llrint (-72057594037927936.25) == -72057594037927936LL",  FUNC(llrint) (-72057594037927936.25L), -72057594037927936LL, 0, 0, 0);
  check_longlong ("llrint (-72057594037927936.5) == -72057594037927936LL",  FUNC(llrint) (-72057594037927936.5L), -72057594037927936LL, 0, 0, 0);
  check_longlong ("llrint (-72057594037927936.75) == -72057594037927937LL",  FUNC(llrint) (-72057594037927936.75L), -72057594037927937LL, 0, 0, 0);
  check_longlong ("llrint (-72057594037927937.5) == -72057594037927938LL",  FUNC(llrint) (-72057594037927937.5L), -72057594037927938LL, 0, 0, 0);

# if LDBL_MANT_DIG > 100
  check_longlong ("llrint (9223372036854775805.5) == 9223372036854775806LL",  FUNC(llrint) (9223372036854775805.5L), 9223372036854775806LL, 0, 0, 0);
  check_longlong ("llrint (-9223372036854775805.5) == -9223372036854775806LL",  FUNC(llrint) (-9223372036854775805.5L), -9223372036854775806LL, 0, 0, 0);
  check_longlong ("llrint (9223372036854775806.0) == 9223372036854775806LL",  FUNC(llrint) (9223372036854775806.0L), 9223372036854775806LL, 0, 0, 0);
  check_longlong ("llrint (-9223372036854775806.0) == -9223372036854775806LL",  FUNC(llrint) (-9223372036854775806.0L), -9223372036854775806LL, 0, 0, 0);
  check_longlong ("llrint (9223372036854775806.25) == 9223372036854775806LL",  FUNC(llrint) (9223372036854775806.25L), 9223372036854775806LL, 0, 0, 0);
  check_longlong ("llrint (-9223372036854775806.25) == -9223372036854775806LL",  FUNC(llrint) (-9223372036854775806.25L), -9223372036854775806LL, 0, 0, 0);
  check_longlong ("llrint (9223372036854775806.5) == 9223372036854775806",  FUNC(llrint) (9223372036854775806.5L), 9223372036854775806L, 0, 0, 0);
  check_longlong ("llrint (-9223372036854775806.5) == -9223372036854775806LL",  FUNC(llrint) (-9223372036854775806.5L), -9223372036854775806LL, 0, 0, 0);
  check_longlong ("llrint (9223372036854775806.75) == 9223372036854775807LL",  FUNC(llrint) (9223372036854775806.75L), 9223372036854775807LL, 0, 0, 0);
  check_longlong ("llrint (-9223372036854775806.75) == -9223372036854775807LL",  FUNC(llrint) (-9223372036854775806.75L), -9223372036854775807LL, 0, 0, 0);
  check_longlong ("llrint (9223372036854775807.0) == 9223372036854775807LL",  FUNC(llrint) (9223372036854775807.0L), 9223372036854775807LL, 0, 0, 0);
  check_longlong ("llrint (-9223372036854775807.0) == -9223372036854775807LL",  FUNC(llrint) (-9223372036854775807.0L), -9223372036854775807LL, 0, 0, 0);
# endif
#endif

  print_max_error ("llrint", 0, 0);
}

static void
llrint_test_tonearest (void)
{
  int save_round_mode;
  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_TONEAREST))
    {
  check_longlong ("llrint_tonearest (0.0) == 0",  FUNC(llrint) (0.0), 0, 0, 0, 0);
  check_longlong ("llrint_tonearest (-0) == 0",  FUNC(llrint) (minus_zero), 0, 0, 0, 0);
  check_longlong ("llrint_tonearest (0.2) == 0",  FUNC(llrint) (0.2L), 0, 0, 0, 0);
  check_longlong ("llrint_tonearest (-0.2) == 0",  FUNC(llrint) (-0.2L), 0, 0, 0, 0);

  check_longlong ("llrint_tonearest (1.4) == 1",  FUNC(llrint) (1.4L), 1, 0, 0, 0);
  check_longlong ("llrint_tonearest (-1.4) == -1",  FUNC(llrint) (-1.4L), -1, 0, 0, 0);

  check_longlong ("llrint_tonearest (8388600.3) == 8388600",  FUNC(llrint) (8388600.3L), 8388600, 0, 0, 0);
  check_longlong ("llrint_tonearest (-8388600.3) == -8388600",  FUNC(llrint) (-8388600.3L), -8388600, 0, 0, 0);

  check_long ("llrint_tonearest (1071930.0008) == 1071930",  FUNC(llrint) (1071930.0008), 1071930, 0, 0, 0);

      /* Test boundary conditions.  */
      /* 0x1FFFFF */
  check_longlong ("llrint_tonearest (2097151.0) == 2097151LL",  FUNC(llrint) (2097151.0), 2097151LL, 0, 0, 0);
      /* 0x800000 */
  check_longlong ("llrint_tonearest (8388608.0) == 8388608LL",  FUNC(llrint) (8388608.0), 8388608LL, 0, 0, 0);
      /* 0x1000000 */
  check_longlong ("llrint_tonearest (16777216.0) == 16777216LL",  FUNC(llrint) (16777216.0), 16777216LL, 0, 0, 0);
      /* 0x20000000000 */
  check_longlong ("llrint_tonearest (2199023255552.0) == 2199023255552LL",  FUNC(llrint) (2199023255552.0), 2199023255552LL, 0, 0, 0);
      /* 0x40000000000 */
  check_longlong ("llrint_tonearest (4398046511104.0) == 4398046511104LL",  FUNC(llrint) (4398046511104.0), 4398046511104LL, 0, 0, 0);
      /* 0x1000000000000 */
  check_longlong ("llrint_tonearest (281474976710656.0) == 281474976710656LL",  FUNC(llrint) (281474976710656.0), 281474976710656LL, 0, 0, 0);
      /* 0x10000000000000 */
  check_longlong ("llrint_tonearest (4503599627370496.0) == 4503599627370496LL",  FUNC(llrint) (4503599627370496.0), 4503599627370496LL, 0, 0, 0);
      /* 0x10000080000000 */
  check_longlong ("llrint_tonearest (4503601774854144.0) == 4503601774854144LL",  FUNC(llrint) (4503601774854144.0), 4503601774854144LL, 0, 0, 0);
      /* 0x20000000000000 */
  check_longlong ("llrint_tonearest (9007199254740992.0) == 9007199254740992LL",  FUNC(llrint) (9007199254740992.0), 9007199254740992LL, 0, 0, 0);
      /* 0x80000000000000 */
  check_longlong ("llrint_tonearest (36028797018963968.0) == 36028797018963968LL",  FUNC(llrint) (36028797018963968.0), 36028797018963968LL, 0, 0, 0);
      /* 0x100000000000000 */
  check_longlong ("llrint_tonearest (72057594037927936.0) == 72057594037927936LL",  FUNC(llrint) (72057594037927936.0), 72057594037927936LL, 0, 0, 0);
#ifdef TEST_LDOUBLE
      /* The input can only be represented in long double.  */
  check_longlong ("llrint_tonearest (4503599627370495.5) == 4503599627370496LL",  FUNC(llrint) (4503599627370495.5L), 4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (4503599627370496.25) == 4503599627370496LL",  FUNC(llrint) (4503599627370496.25L), 4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (4503599627370496.5) == 4503599627370496LL",  FUNC(llrint) (4503599627370496.5L), 4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (4503599627370496.75) == 4503599627370497LL",  FUNC(llrint) (4503599627370496.75L), 4503599627370497LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (4503599627370497.5) == 4503599627370498LL",  FUNC(llrint) (4503599627370497.5L), 4503599627370498LL, 0, 0, 0);

  check_longlong ("llrint_tonearest (-4503599627370495.5) == -4503599627370496LL",  FUNC(llrint) (-4503599627370495.5L), -4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (-4503599627370496.25) == -4503599627370496LL",  FUNC(llrint) (-4503599627370496.25L), -4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (-4503599627370496.5) == -4503599627370496LL",  FUNC(llrint) (-4503599627370496.5L), -4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (-4503599627370496.75) == -4503599627370497LL",  FUNC(llrint) (-4503599627370496.75L), -4503599627370497LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (-4503599627370497.5) == -4503599627370498LL",  FUNC(llrint) (-4503599627370497.5L), -4503599627370498LL, 0, 0, 0);

# if LDBL_MANT_DIG > 100
  check_longlong ("llrint_tonearest (4503599627370495.4999999999999) == 4503599627370495LL",  FUNC(llrint) (4503599627370495.4999999999999L), 4503599627370495LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (4503599627370496.4999999999999) == 4503599627370496LL",  FUNC(llrint) (4503599627370496.4999999999999L), 4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (4503599627370497.4999999999999) == 4503599627370497LL",  FUNC(llrint) (4503599627370497.4999999999999L), 4503599627370497LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (4503599627370494.5000000000001) == 4503599627370495LL",  FUNC(llrint) (4503599627370494.5000000000001L), 4503599627370495LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (4503599627370495.5000000000001) == 4503599627370496LL",  FUNC(llrint) (4503599627370495.5000000000001L), 4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (4503599627370496.5000000000001) == 4503599627370497LL",  FUNC(llrint) (4503599627370496.5000000000001L), 4503599627370497LL, 0, 0, 0);

  check_longlong ("llrint_tonearest (-4503599627370495.4999999999999) == -4503599627370495LL",  FUNC(llrint) (-4503599627370495.4999999999999L), -4503599627370495LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (-4503599627370496.4999999999999) == -4503599627370496LL",  FUNC(llrint) (-4503599627370496.4999999999999L), -4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (-4503599627370497.4999999999999) == -4503599627370497LL",  FUNC(llrint) (-4503599627370497.4999999999999L), -4503599627370497LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (-4503599627370494.5000000000001) == -4503599627370495LL",  FUNC(llrint) (-4503599627370494.5000000000001L), -4503599627370495LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (-4503599627370495.5000000000001) == -4503599627370496LL",  FUNC(llrint) (-4503599627370495.5000000000001L), -4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (-4503599627370496.5000000000001) == -4503599627370497LL",  FUNC(llrint) (-4503599627370496.5000000000001L), -4503599627370497LL, 0, 0, 0);
#endif

  check_longlong ("llrint_tonearest (9007199254740991.5) == 9007199254740992LL",  FUNC(llrint) (9007199254740991.5L), 9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (9007199254740992.25) == 9007199254740992LL",  FUNC(llrint) (9007199254740992.25L), 9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (9007199254740992.5) == 9007199254740992LL",  FUNC(llrint) (9007199254740992.5L), 9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (9007199254740992.75) == 9007199254740993LL",  FUNC(llrint) (9007199254740992.75L), 9007199254740993LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (9007199254740993.5) == 9007199254740994LL",  FUNC(llrint) (9007199254740993.5L), 9007199254740994LL, 0, 0, 0);

  check_longlong ("llrint_tonearest (-9007199254740991.5) == -9007199254740992LL",  FUNC(llrint) (-9007199254740991.5L), -9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (-9007199254740992.25) == -9007199254740992LL",  FUNC(llrint) (-9007199254740992.25L), -9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (-9007199254740992.5) == -9007199254740992LL",  FUNC(llrint) (-9007199254740992.5L), -9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (-9007199254740992.75) == -9007199254740993LL",  FUNC(llrint) (-9007199254740992.75L), -9007199254740993LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (-9007199254740993.5) == -9007199254740994LL",  FUNC(llrint) (-9007199254740993.5L), -9007199254740994LL, 0, 0, 0);

# if LDBL_MANT_DIG > 100
  check_longlong ("llrint_tonearest (9007199254740991.4999999999999) == 9007199254740991LL",  FUNC(llrint) (9007199254740991.4999999999999L), 9007199254740991LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (9007199254740992.4999999999999) == 9007199254740992LL",  FUNC(llrint) (9007199254740992.4999999999999L), 9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (9007199254740993.4999999999999) == 9007199254740993LL",  FUNC(llrint) (9007199254740993.4999999999999L), 9007199254740993LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (9007199254740991.5000000000001) == 9007199254740992LL",  FUNC(llrint) (9007199254740991.5000000000001L), 9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (9007199254740992.5000000000001) == 9007199254740993LL",  FUNC(llrint) (9007199254740992.5000000000001L), 9007199254740993LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (9007199254740993.5000000000001) == 9007199254740994LL",  FUNC(llrint) (9007199254740993.5000000000001L), 9007199254740994LL, 0, 0, 0);

  check_longlong ("llrint_tonearest (-9007199254740991.4999999999999) == -9007199254740991LL",  FUNC(llrint) (-9007199254740991.4999999999999L), -9007199254740991LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (-9007199254740992.4999999999999) == -9007199254740992LL",  FUNC(llrint) (-9007199254740992.4999999999999L), -9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (-9007199254740993.4999999999999) == -9007199254740993LL",  FUNC(llrint) (-9007199254740993.4999999999999L), -9007199254740993LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (-9007199254740991.5000000000001) == -9007199254740992LL",  FUNC(llrint) (-9007199254740991.5000000000001L), -9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (-9007199254740992.5000000000001) == -9007199254740993LL",  FUNC(llrint) (-9007199254740992.5000000000001L), -9007199254740993LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (-9007199254740993.5000000000001) == -9007199254740994LL",  FUNC(llrint) (-9007199254740993.5000000000001L), -9007199254740994LL, 0, 0, 0);
#endif

  check_longlong ("llrint_tonearest (72057594037927935.5) == 72057594037927936LL",  FUNC(llrint) (72057594037927935.5L), 72057594037927936LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (72057594037927936.25) == 72057594037927936LL",  FUNC(llrint) (72057594037927936.25L), 72057594037927936LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (72057594037927936.5) == 72057594037927936LL",  FUNC(llrint) (72057594037927936.5L), 72057594037927936LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (72057594037927936.75) == 72057594037927937LL",  FUNC(llrint) (72057594037927936.75L), 72057594037927937LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (72057594037927937.5) == 72057594037927938LL",  FUNC(llrint) (72057594037927937.5L), 72057594037927938LL, 0, 0, 0);

  check_longlong ("llrint_tonearest (-72057594037927935.5) == -72057594037927936LL",  FUNC(llrint) (-72057594037927935.5L), -72057594037927936LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (-72057594037927936.25) == -72057594037927936LL",  FUNC(llrint) (-72057594037927936.25L), -72057594037927936LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (-72057594037927936.5) == -72057594037927936LL",  FUNC(llrint) (-72057594037927936.5L), -72057594037927936LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (-72057594037927936.75) == -72057594037927937LL",  FUNC(llrint) (-72057594037927936.75L), -72057594037927937LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (-72057594037927937.5) == -72057594037927938LL",  FUNC(llrint) (-72057594037927937.5L), -72057594037927938LL, 0, 0, 0);

# if LDBL_MANT_DIG > 100
  check_longlong ("llrint_tonearest (9223372036854775805.5) == 9223372036854775806LL",  FUNC(llrint) (9223372036854775805.5L), 9223372036854775806LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (-9223372036854775805.5) == -9223372036854775806LL",  FUNC(llrint) (-9223372036854775805.5L), -9223372036854775806LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (9223372036854775806.0) == 9223372036854775806LL",  FUNC(llrint) (9223372036854775806.0L), 9223372036854775806LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (-9223372036854775806.0) == -9223372036854775806LL",  FUNC(llrint) (-9223372036854775806.0L), -9223372036854775806LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (9223372036854775806.25) == 9223372036854775806LL",  FUNC(llrint) (9223372036854775806.25L), 9223372036854775806LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (-9223372036854775806.25) == -9223372036854775806LL",  FUNC(llrint) (-9223372036854775806.25L), -9223372036854775806LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (9223372036854775806.5) == 9223372036854775806",  FUNC(llrint) (9223372036854775806.5L), 9223372036854775806L, 0, 0, 0);
  check_longlong ("llrint_tonearest (-9223372036854775806.5) == -9223372036854775806LL",  FUNC(llrint) (-9223372036854775806.5L), -9223372036854775806LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (9223372036854775806.75) == 9223372036854775807LL",  FUNC(llrint) (9223372036854775806.75L), 9223372036854775807LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (-9223372036854775806.75) == -9223372036854775807LL",  FUNC(llrint) (-9223372036854775806.75L), -9223372036854775807LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (9223372036854775807.0) == 9223372036854775807LL",  FUNC(llrint) (9223372036854775807.0L), 9223372036854775807LL, 0, 0, 0);
  check_longlong ("llrint_tonearest (-9223372036854775807.0) == -9223372036854775807LL",  FUNC(llrint) (-9223372036854775807.0L), -9223372036854775807LL, 0, 0, 0);
# endif
#endif
    }

  fesetround (save_round_mode);

  print_max_error ("llrint_tonearest", 0, 0);
}

static void
llrint_test_towardzero (void)
{
  int save_round_mode;
  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_TOWARDZERO))
    {
  check_longlong ("llrint_towardzero (0.0) == 0",  FUNC(llrint) (0.0), 0, 0, 0, 0);
  check_longlong ("llrint_towardzero (-0) == 0",  FUNC(llrint) (minus_zero), 0, 0, 0, 0);
  check_longlong ("llrint_towardzero (0.2) == 0",  FUNC(llrint) (0.2L), 0, 0, 0, 0);
  check_longlong ("llrint_towardzero (-0.2) == 0",  FUNC(llrint) (-0.2L), 0, 0, 0, 0);

  check_longlong ("llrint_towardzero (1.4) == 1",  FUNC(llrint) (1.4L), 1, 0, 0, 0);
  check_longlong ("llrint_towardzero (-1.4) == -1",  FUNC(llrint) (-1.4L), -1, 0, 0, 0);

  check_longlong ("llrint_towardzero (8388600.3) == 8388600",  FUNC(llrint) (8388600.3L), 8388600, 0, 0, 0);
  check_longlong ("llrint_towardzero (-8388600.3) == -8388600",  FUNC(llrint) (-8388600.3L), -8388600, 0, 0, 0);

  check_long ("llrint_towardzero (1071930.0008) == 1071930",  FUNC(llrint) (1071930.0008), 1071930, 0, 0, 0);

      /* Test boundary conditions.  */
      /* 0x1FFFFF */
  check_longlong ("llrint_towardzero (2097151.0) == 2097151LL",  FUNC(llrint) (2097151.0), 2097151LL, 0, 0, 0);
      /* 0x800000 */
  check_longlong ("llrint_towardzero (8388608.0) == 8388608LL",  FUNC(llrint) (8388608.0), 8388608LL, 0, 0, 0);
      /* 0x1000000 */
  check_longlong ("llrint_towardzero (16777216.0) == 16777216LL",  FUNC(llrint) (16777216.0), 16777216LL, 0, 0, 0);
      /* 0x20000000000 */
  check_longlong ("llrint_towardzero (2199023255552.0) == 2199023255552LL",  FUNC(llrint) (2199023255552.0), 2199023255552LL, 0, 0, 0);
      /* 0x40000000000 */
  check_longlong ("llrint_towardzero (4398046511104.0) == 4398046511104LL",  FUNC(llrint) (4398046511104.0), 4398046511104LL, 0, 0, 0);
      /* 0x1000000000000 */
  check_longlong ("llrint_towardzero (281474976710656.0) == 281474976710656LL",  FUNC(llrint) (281474976710656.0), 281474976710656LL, 0, 0, 0);
      /* 0x10000000000000 */
  check_longlong ("llrint_towardzero (4503599627370496.0) == 4503599627370496LL",  FUNC(llrint) (4503599627370496.0), 4503599627370496LL, 0, 0, 0);
      /* 0x10000080000000 */
  check_longlong ("llrint_towardzero (4503601774854144.0) == 4503601774854144LL",  FUNC(llrint) (4503601774854144.0), 4503601774854144LL, 0, 0, 0);
      /* 0x20000000000000 */
  check_longlong ("llrint_towardzero (9007199254740992.0) == 9007199254740992LL",  FUNC(llrint) (9007199254740992.0), 9007199254740992LL, 0, 0, 0);
      /* 0x80000000000000 */
  check_longlong ("llrint_towardzero (36028797018963968.0) == 36028797018963968LL",  FUNC(llrint) (36028797018963968.0), 36028797018963968LL, 0, 0, 0);
      /* 0x100000000000000 */
  check_longlong ("llrint_towardzero (72057594037927936.0) == 72057594037927936LL",  FUNC(llrint) (72057594037927936.0), 72057594037927936LL, 0, 0, 0);
#ifdef TEST_LDOUBLE
      /* The input can only be represented in long double.  */
  check_longlong ("llrint_towardzero (4503599627370495.5) == 4503599627370495LL",  FUNC(llrint) (4503599627370495.5L), 4503599627370495LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (4503599627370496.25) == 4503599627370496LL",  FUNC(llrint) (4503599627370496.25L), 4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (4503599627370496.5) == 4503599627370496LL",  FUNC(llrint) (4503599627370496.5L), 4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (4503599627370496.75) == 4503599627370496LL",  FUNC(llrint) (4503599627370496.75L), 4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (4503599627370497.5) == 4503599627370497LL",  FUNC(llrint) (4503599627370497.5L), 4503599627370497LL, 0, 0, 0);

  check_longlong ("llrint_towardzero (-4503599627370495.5) == -4503599627370495LL",  FUNC(llrint) (-4503599627370495.5L), -4503599627370495LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (-4503599627370496.25) == -4503599627370496LL",  FUNC(llrint) (-4503599627370496.25L), -4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (-4503599627370496.5) == -4503599627370496LL",  FUNC(llrint) (-4503599627370496.5L), -4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (-4503599627370496.75) == -4503599627370496LL",  FUNC(llrint) (-4503599627370496.75L), -4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (-4503599627370497.5) == -4503599627370497LL",  FUNC(llrint) (-4503599627370497.5L), -4503599627370497LL, 0, 0, 0);

# if LDBL_MANT_DIG > 100
  check_longlong ("llrint_towardzero (4503599627370495.4999999999999) == 4503599627370495LL",  FUNC(llrint) (4503599627370495.4999999999999L), 4503599627370495LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (4503599627370496.4999999999999) == 4503599627370496LL",  FUNC(llrint) (4503599627370496.4999999999999L), 4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (4503599627370497.4999999999999) == 4503599627370497LL",  FUNC(llrint) (4503599627370497.4999999999999L), 4503599627370497LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (4503599627370494.5000000000001) == 4503599627370494LL",  FUNC(llrint) (4503599627370494.5000000000001L), 4503599627370494LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (4503599627370495.5000000000001) == 4503599627370495LL",  FUNC(llrint) (4503599627370495.5000000000001L), 4503599627370495LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (4503599627370496.5000000000001) == 4503599627370496LL",  FUNC(llrint) (4503599627370496.5000000000001L), 4503599627370496LL, 0, 0, 0);

  check_longlong ("llrint_towardzero (-4503599627370495.4999999999999) == -4503599627370495LL",  FUNC(llrint) (-4503599627370495.4999999999999L), -4503599627370495LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (-4503599627370496.4999999999999) == -4503599627370496LL",  FUNC(llrint) (-4503599627370496.4999999999999L), -4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (-4503599627370497.4999999999999) == -4503599627370497LL",  FUNC(llrint) (-4503599627370497.4999999999999L), -4503599627370497LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (-4503599627370494.5000000000001) == -4503599627370494LL",  FUNC(llrint) (-4503599627370494.5000000000001L), -4503599627370494LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (-4503599627370495.5000000000001) == -4503599627370495LL",  FUNC(llrint) (-4503599627370495.5000000000001L), -4503599627370495LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (-4503599627370496.5000000000001) == -4503599627370496LL",  FUNC(llrint) (-4503599627370496.5000000000001L), -4503599627370496LL, 0, 0, 0);
#endif

  check_longlong ("llrint_towardzero (9007199254740991.5) == 9007199254740991LL",  FUNC(llrint) (9007199254740991.5L), 9007199254740991LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (9007199254740992.25) == 9007199254740992LL",  FUNC(llrint) (9007199254740992.25L), 9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (9007199254740992.5) == 9007199254740992LL",  FUNC(llrint) (9007199254740992.5L), 9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (9007199254740992.75) == 9007199254740992LL",  FUNC(llrint) (9007199254740992.75L), 9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (9007199254740993.5) == 9007199254740993LL",  FUNC(llrint) (9007199254740993.5L), 9007199254740993LL, 0, 0, 0);

  check_longlong ("llrint_towardzero (-9007199254740991.5) == -9007199254740991LL",  FUNC(llrint) (-9007199254740991.5L), -9007199254740991LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (-9007199254740992.25) == -9007199254740992LL",  FUNC(llrint) (-9007199254740992.25L), -9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (-9007199254740992.5) == -9007199254740992LL",  FUNC(llrint) (-9007199254740992.5L), -9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (-9007199254740992.75) == -9007199254740992LL",  FUNC(llrint) (-9007199254740992.75L), -9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (-9007199254740993.5) == -9007199254740993LL",  FUNC(llrint) (-9007199254740993.5L), -9007199254740993LL, 0, 0, 0);

# if LDBL_MANT_DIG > 100
  check_longlong ("llrint_towardzero (9007199254740991.4999999999999) == 9007199254740991LL",  FUNC(llrint) (9007199254740991.4999999999999L), 9007199254740991LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (9007199254740992.4999999999999) == 9007199254740992LL",  FUNC(llrint) (9007199254740992.4999999999999L), 9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (9007199254740993.4999999999999) == 9007199254740993LL",  FUNC(llrint) (9007199254740993.4999999999999L), 9007199254740993LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (9007199254740991.5000000000001) == 9007199254740991LL",  FUNC(llrint) (9007199254740991.5000000000001L), 9007199254740991LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (9007199254740992.5000000000001) == 9007199254740992LL",  FUNC(llrint) (9007199254740992.5000000000001L), 9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (9007199254740993.5000000000001) == 9007199254740993LL",  FUNC(llrint) (9007199254740993.5000000000001L), 9007199254740993LL, 0, 0, 0);

  check_longlong ("llrint_towardzero (-9007199254740991.4999999999999) == -9007199254740991LL",  FUNC(llrint) (-9007199254740991.4999999999999L), -9007199254740991LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (-9007199254740992.4999999999999) == -9007199254740992LL",  FUNC(llrint) (-9007199254740992.4999999999999L), -9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (-9007199254740993.4999999999999) == -9007199254740993LL",  FUNC(llrint) (-9007199254740993.4999999999999L), -9007199254740993LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (-9007199254740991.5000000000001) == -9007199254740991LL",  FUNC(llrint) (-9007199254740991.5000000000001L), -9007199254740991LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (-9007199254740992.5000000000001) == -9007199254740992LL",  FUNC(llrint) (-9007199254740992.5000000000001L), -9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (-9007199254740993.5000000000001) == -9007199254740993LL",  FUNC(llrint) (-9007199254740993.5000000000001L), -9007199254740993LL, 0, 0, 0);
#endif

  check_longlong ("llrint_towardzero (72057594037927935.5) == 72057594037927935LL",  FUNC(llrint) (72057594037927935.5L), 72057594037927935LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (72057594037927936.25) == 72057594037927936LL",  FUNC(llrint) (72057594037927936.25L), 72057594037927936LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (72057594037927936.5) == 72057594037927936LL",  FUNC(llrint) (72057594037927936.5L), 72057594037927936LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (72057594037927936.75) == 72057594037927936LL",  FUNC(llrint) (72057594037927936.75L), 72057594037927936LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (72057594037927937.5) == 72057594037927937LL",  FUNC(llrint) (72057594037927937.5L), 72057594037927937LL, 0, 0, 0);

  check_longlong ("llrint_towardzero (-72057594037927935.5) == -72057594037927935LL",  FUNC(llrint) (-72057594037927935.5L), -72057594037927935LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (-72057594037927936.25) == -72057594037927936LL",  FUNC(llrint) (-72057594037927936.25L), -72057594037927936LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (-72057594037927936.5) == -72057594037927936LL",  FUNC(llrint) (-72057594037927936.5L), -72057594037927936LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (-72057594037927936.75) == -72057594037927936LL",  FUNC(llrint) (-72057594037927936.75L), -72057594037927936LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (-72057594037927937.5) == -72057594037927937LL",  FUNC(llrint) (-72057594037927937.5L), -72057594037927937LL, 0, 0, 0);

# if LDBL_MANT_DIG > 100
  check_longlong ("llrint_towardzero (9223372036854775805.5) == 9223372036854775805LL",  FUNC(llrint) (9223372036854775805.5L), 9223372036854775805LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (-9223372036854775805.5) == -9223372036854775805LL",  FUNC(llrint) (-9223372036854775805.5L), -9223372036854775805LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (9223372036854775806.0) == 9223372036854775806LL",  FUNC(llrint) (9223372036854775806.0L), 9223372036854775806LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (-9223372036854775806.0) == -9223372036854775806LL",  FUNC(llrint) (-9223372036854775806.0L), -9223372036854775806LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (9223372036854775806.25) == 9223372036854775806LL",  FUNC(llrint) (9223372036854775806.25L), 9223372036854775806LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (-9223372036854775806.25) == -9223372036854775806LL",  FUNC(llrint) (-9223372036854775806.25L), -9223372036854775806LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (9223372036854775806.5) == 9223372036854775806",  FUNC(llrint) (9223372036854775806.5L), 9223372036854775806L, 0, 0, 0);
  check_longlong ("llrint_towardzero (-9223372036854775806.5) == -9223372036854775806LL",  FUNC(llrint) (-9223372036854775806.5L), -9223372036854775806LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (9223372036854775806.75) == 9223372036854775806LL",  FUNC(llrint) (9223372036854775806.75L), 9223372036854775806LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (-9223372036854775806.75) == -9223372036854775806LL",  FUNC(llrint) (-9223372036854775806.75L), -9223372036854775806LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (9223372036854775807.0) == 9223372036854775807LL",  FUNC(llrint) (9223372036854775807.0L), 9223372036854775807LL, 0, 0, 0);
  check_longlong ("llrint_towardzero (-9223372036854775807.0) == -9223372036854775807LL",  FUNC(llrint) (-9223372036854775807.0L), -9223372036854775807LL, 0, 0, 0);
# endif
#endif
    }

  fesetround (save_round_mode);

  print_max_error ("llrint_towardzero", 0, 0);
}

static void
llrint_test_downward (void)
{
  int save_round_mode;
  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_DOWNWARD))
    {
  check_longlong ("llrint_downward (0.0) == 0",  FUNC(llrint) (0.0), 0, 0, 0, 0);
  check_longlong ("llrint_downward (-0) == 0",  FUNC(llrint) (minus_zero), 0, 0, 0, 0);
  check_longlong ("llrint_downward (0.2) == 0",  FUNC(llrint) (0.2L), 0, 0, 0, 0);
  check_longlong ("llrint_downward (-0.2) == -1",  FUNC(llrint) (-0.2L), -1, 0, 0, 0);

  check_longlong ("llrint_downward (1.4) == 1",  FUNC(llrint) (1.4L), 1, 0, 0, 0);
  check_longlong ("llrint_downward (-1.4) == -2",  FUNC(llrint) (-1.4L), -2, 0, 0, 0);

  check_longlong ("llrint_downward (8388600.3) == 8388600",  FUNC(llrint) (8388600.3L), 8388600, 0, 0, 0);
  check_longlong ("llrint_downward (-8388600.3) == -8388601",  FUNC(llrint) (-8388600.3L), -8388601, 0, 0, 0);

  check_long ("llrint_downward (1071930.0008) == 1071930",  FUNC(llrint) (1071930.0008), 1071930, 0, 0, 0);

      /* Test boundary conditions.  */
      /* 0x1FFFFF */
  check_longlong ("llrint_downward (2097151.0) == 2097151LL",  FUNC(llrint) (2097151.0), 2097151LL, 0, 0, 0);
      /* 0x800000 */
  check_longlong ("llrint_downward (8388608.0) == 8388608LL",  FUNC(llrint) (8388608.0), 8388608LL, 0, 0, 0);
      /* 0x1000000 */
  check_longlong ("llrint_downward (16777216.0) == 16777216LL",  FUNC(llrint) (16777216.0), 16777216LL, 0, 0, 0);
      /* 0x20000000000 */
  check_longlong ("llrint_downward (2199023255552.0) == 2199023255552LL",  FUNC(llrint) (2199023255552.0), 2199023255552LL, 0, 0, 0);
      /* 0x40000000000 */
  check_longlong ("llrint_downward (4398046511104.0) == 4398046511104LL",  FUNC(llrint) (4398046511104.0), 4398046511104LL, 0, 0, 0);
      /* 0x1000000000000 */
  check_longlong ("llrint_downward (281474976710656.0) == 281474976710656LL",  FUNC(llrint) (281474976710656.0), 281474976710656LL, 0, 0, 0);
      /* 0x10000000000000 */
  check_longlong ("llrint_downward (4503599627370496.0) == 4503599627370496LL",  FUNC(llrint) (4503599627370496.0), 4503599627370496LL, 0, 0, 0);
      /* 0x10000080000000 */
  check_longlong ("llrint_downward (4503601774854144.0) == 4503601774854144LL",  FUNC(llrint) (4503601774854144.0), 4503601774854144LL, 0, 0, 0);
      /* 0x20000000000000 */
  check_longlong ("llrint_downward (9007199254740992.0) == 9007199254740992LL",  FUNC(llrint) (9007199254740992.0), 9007199254740992LL, 0, 0, 0);
      /* 0x80000000000000 */
  check_longlong ("llrint_downward (36028797018963968.0) == 36028797018963968LL",  FUNC(llrint) (36028797018963968.0), 36028797018963968LL, 0, 0, 0);
      /* 0x100000000000000 */
  check_longlong ("llrint_downward (72057594037927936.0) == 72057594037927936LL",  FUNC(llrint) (72057594037927936.0), 72057594037927936LL, 0, 0, 0);
#ifdef TEST_LDOUBLE
      /* The input can only be represented in long double.  */
  check_longlong ("llrint_downward (4503599627370495.5) == 4503599627370495LL",  FUNC(llrint) (4503599627370495.5L), 4503599627370495LL, 0, 0, 0);
  check_longlong ("llrint_downward (4503599627370496.25) == 4503599627370496LL",  FUNC(llrint) (4503599627370496.25L), 4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint_downward (4503599627370496.5) == 4503599627370496LL",  FUNC(llrint) (4503599627370496.5L), 4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint_downward (4503599627370496.75) == 4503599627370496LL",  FUNC(llrint) (4503599627370496.75L), 4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint_downward (4503599627370497.5) == 4503599627370497LL",  FUNC(llrint) (4503599627370497.5L), 4503599627370497LL, 0, 0, 0);

  check_longlong ("llrint_downward (4503599627370495.4999999999999) == 4503599627370495LL",  FUNC(llrint) (4503599627370495.4999999999999L), 4503599627370495LL, 0, 0, 0);
  check_longlong ("llrint_downward (4503599627370496.4999999999999) == 4503599627370496LL",  FUNC(llrint) (4503599627370496.4999999999999L), 4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint_downward (4503599627370497.4999999999999) == 4503599627370497LL",  FUNC(llrint) (4503599627370497.4999999999999L), 4503599627370497LL, 0, 0, 0);
  check_longlong ("llrint_downward (4503599627370494.5000000000001) == 4503599627370494LL",  FUNC(llrint) (4503599627370494.5000000000001L), 4503599627370494LL, 0, 0, 0);
  check_longlong ("llrint_downward (4503599627370495.5000000000001) == 4503599627370495LL",  FUNC(llrint) (4503599627370495.5000000000001L), 4503599627370495LL, 0, 0, 0);
  check_longlong ("llrint_downward (4503599627370496.5000000000001) == 4503599627370496LL",  FUNC(llrint) (4503599627370496.5000000000001L), 4503599627370496LL, 0, 0, 0);

  check_longlong ("llrint_downward (-4503599627370495.5) == -4503599627370496LL",  FUNC(llrint) (-4503599627370495.5L), -4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint_downward (-4503599627370496.25) == -4503599627370497LL",  FUNC(llrint) (-4503599627370496.25L), -4503599627370497LL, 0, 0, 0);
  check_longlong ("llrint_downward (-4503599627370496.5) == -4503599627370497LL",  FUNC(llrint) (-4503599627370496.5L), -4503599627370497LL, 0, 0, 0);
  check_longlong ("llrint_downward (-4503599627370496.75) == -4503599627370497LL",  FUNC(llrint) (-4503599627370496.75L), -4503599627370497LL, 0, 0, 0);
  check_longlong ("llrint_downward (-4503599627370497.5) == -4503599627370498LL",  FUNC(llrint) (-4503599627370497.5L), -4503599627370498LL, 0, 0, 0);

  check_longlong ("llrint_downward (-4503599627370495.4999999999999) == -4503599627370496LL",  FUNC(llrint) (-4503599627370495.4999999999999L), -4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint_downward (-4503599627370496.4999999999999) == -4503599627370497LL",  FUNC(llrint) (-4503599627370496.4999999999999L), -4503599627370497LL, 0, 0, 0);
  check_longlong ("llrint_downward (-4503599627370497.4999999999999) == -4503599627370498LL",  FUNC(llrint) (-4503599627370497.4999999999999L), -4503599627370498LL, 0, 0, 0);
  check_longlong ("llrint_downward (-4503599627370494.5000000000001) == -4503599627370495LL",  FUNC(llrint) (-4503599627370494.5000000000001L), -4503599627370495LL, 0, 0, 0);
  check_longlong ("llrint_downward (-4503599627370495.5000000000001) == -4503599627370496LL",  FUNC(llrint) (-4503599627370495.5000000000001L), -4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint_downward (-4503599627370496.5000000000001) == -4503599627370497LL",  FUNC(llrint) (-4503599627370496.5000000000001L), -4503599627370497LL, 0, 0, 0);

  check_longlong ("llrint_downward (9007199254740991.5) == 9007199254740991LL",  FUNC(llrint) (9007199254740991.5L), 9007199254740991LL, 0, 0, 0);
  check_longlong ("llrint_downward (9007199254740992.25) == 9007199254740992LL",  FUNC(llrint) (9007199254740992.25L), 9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint_downward (9007199254740992.5) == 9007199254740992LL",  FUNC(llrint) (9007199254740992.5L), 9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint_downward (9007199254740992.75) == 9007199254740992LL",  FUNC(llrint) (9007199254740992.75L), 9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint_downward (9007199254740993.5) == 9007199254740993LL",  FUNC(llrint) (9007199254740993.5L), 9007199254740993LL, 0, 0, 0);

  check_longlong ("llrint_downward (9007199254740991.4999999999999) == 9007199254740991LL",  FUNC(llrint) (9007199254740991.4999999999999L), 9007199254740991LL, 0, 0, 0);
  check_longlong ("llrint_downward (9007199254740992.4999999999999) == 9007199254740992LL",  FUNC(llrint) (9007199254740992.4999999999999L), 9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint_downward (9007199254740993.4999999999999) == 9007199254740993LL",  FUNC(llrint) (9007199254740993.4999999999999L), 9007199254740993LL, 0, 0, 0);
  check_longlong ("llrint_downward (9007199254740991.5000000000001) == 9007199254740991LL",  FUNC(llrint) (9007199254740991.5000000000001L), 9007199254740991LL, 0, 0, 0);
  check_longlong ("llrint_downward (9007199254740992.5000000000001) == 9007199254740992LL",  FUNC(llrint) (9007199254740992.5000000000001L), 9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint_downward (9007199254740993.5000000000001) == 9007199254740993LL",  FUNC(llrint) (9007199254740993.5000000000001L), 9007199254740993LL, 0, 0, 0);

  check_longlong ("llrint_downward (-9007199254740991.5) == -9007199254740992LL",  FUNC(llrint) (-9007199254740991.5L), -9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint_downward (-9007199254740992.25) == -9007199254740993LL",  FUNC(llrint) (-9007199254740992.25L), -9007199254740993LL, 0, 0, 0);
  check_longlong ("llrint_downward (-9007199254740992.5) == -9007199254740993LL",  FUNC(llrint) (-9007199254740992.5L), -9007199254740993LL, 0, 0, 0);
  check_longlong ("llrint_downward (-9007199254740992.75) == -9007199254740993LL",  FUNC(llrint) (-9007199254740992.75L), -9007199254740993LL, 0, 0, 0);
  check_longlong ("llrint_downward (-9007199254740993.5) == -9007199254740994LL",  FUNC(llrint) (-9007199254740993.5L), -9007199254740994LL, 0, 0, 0);

  check_longlong ("llrint_downward (-9007199254740991.4999999999999) == -9007199254740992LL",  FUNC(llrint) (-9007199254740991.4999999999999L), -9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint_downward (-9007199254740992.4999999999999) == -9007199254740993LL",  FUNC(llrint) (-9007199254740992.4999999999999L), -9007199254740993LL, 0, 0, 0);
  check_longlong ("llrint_downward (-9007199254740993.4999999999999) == -9007199254740994LL",  FUNC(llrint) (-9007199254740993.4999999999999L), -9007199254740994LL, 0, 0, 0);
  check_longlong ("llrint_downward (-9007199254740991.5000000000001) == -9007199254740992LL",  FUNC(llrint) (-9007199254740991.5000000000001L), -9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint_downward (-9007199254740992.5000000000001) == -9007199254740993LL",  FUNC(llrint) (-9007199254740992.5000000000001L), -9007199254740993LL, 0, 0, 0);
  check_longlong ("llrint_downward (-9007199254740993.5000000000001) == -9007199254740994LL",  FUNC(llrint) (-9007199254740993.5000000000001L), -9007199254740994LL, 0, 0, 0);

  check_longlong ("llrint_downward (72057594037927935.5) == 72057594037927935LL",  FUNC(llrint) (72057594037927935.5L), 72057594037927935LL, 0, 0, 0);
  check_longlong ("llrint_downward (72057594037927936.25) == 72057594037927936LL",  FUNC(llrint) (72057594037927936.25L), 72057594037927936LL, 0, 0, 0);
  check_longlong ("llrint_downward (72057594037927936.5) == 72057594037927936LL",  FUNC(llrint) (72057594037927936.5L), 72057594037927936LL, 0, 0, 0);
  check_longlong ("llrint_downward (72057594037927936.75) == 72057594037927936LL",  FUNC(llrint) (72057594037927936.75L), 72057594037927936LL, 0, 0, 0);
  check_longlong ("llrint_downward (72057594037927937.5) == 72057594037927937LL",  FUNC(llrint) (72057594037927937.5L), 72057594037927937LL, 0, 0, 0);

  check_longlong ("llrint_downward (-72057594037927935.5) == -72057594037927936LL",  FUNC(llrint) (-72057594037927935.5L), -72057594037927936LL, 0, 0, 0);
  check_longlong ("llrint_downward (-72057594037927936.25) == -72057594037927937LL",  FUNC(llrint) (-72057594037927936.25L), -72057594037927937LL, 0, 0, 0);
  check_longlong ("llrint_downward (-72057594037927936.5) == -72057594037927937LL",  FUNC(llrint) (-72057594037927936.5L), -72057594037927937LL, 0, 0, 0);
  check_longlong ("llrint_downward (-72057594037927936.75) == -72057594037927937LL",  FUNC(llrint) (-72057594037927936.75L), -72057594037927937LL, 0, 0, 0);
  check_longlong ("llrint_downward (-72057594037927937.5) == -72057594037927938LL",  FUNC(llrint) (-72057594037927937.5L), -72057594037927938LL, 0, 0, 0);

# if LDBL_MANT_DIG > 100
  check_longlong ("llrint_downward (9223372036854775805.5) == 9223372036854775805LL",  FUNC(llrint) (9223372036854775805.5L), 9223372036854775805LL, 0, 0, 0);
  check_longlong ("llrint_downward (-9223372036854775805.5) == -9223372036854775806LL",  FUNC(llrint) (-9223372036854775805.5L), -9223372036854775806LL, 0, 0, 0);
  check_longlong ("llrint_downward (9223372036854775806.0) == 9223372036854775806LL",  FUNC(llrint) (9223372036854775806.0L), 9223372036854775806LL, 0, 0, 0);
  check_longlong ("llrint_downward (-9223372036854775806.0) == -9223372036854775806LL",  FUNC(llrint) (-9223372036854775806.0L), -9223372036854775806LL, 0, 0, 0);
  check_longlong ("llrint_downward (9223372036854775806.25) == 9223372036854775806LL",  FUNC(llrint) (9223372036854775806.25L), 9223372036854775806LL, 0, 0, 0);
  check_longlong ("llrint_downward (-9223372036854775806.25) == -9223372036854775807LL",  FUNC(llrint) (-9223372036854775806.25L), -9223372036854775807LL, 0, 0, 0);
  check_longlong ("llrint_downward (9223372036854775806.5) == 9223372036854775806",  FUNC(llrint) (9223372036854775806.5L), 9223372036854775806L, 0, 0, 0);
  check_longlong ("llrint_downward (-9223372036854775806.5) == -9223372036854775807LL",  FUNC(llrint) (-9223372036854775806.5L), -9223372036854775807LL, 0, 0, 0);
  check_longlong ("llrint_downward (9223372036854775806.75) == 9223372036854775806LL",  FUNC(llrint) (9223372036854775806.75L), 9223372036854775806LL, 0, 0, 0);
  check_longlong ("llrint_downward (-9223372036854775806.75) == -9223372036854775807LL",  FUNC(llrint) (-9223372036854775806.75L), -9223372036854775807LL, 0, 0, 0);
  check_longlong ("llrint_downward (9223372036854775807.0) == 9223372036854775807LL",  FUNC(llrint) (9223372036854775807.0L), 9223372036854775807LL, 0, 0, 0);
  check_longlong ("llrint_downward (-9223372036854775807.0) == -9223372036854775807LL",  FUNC(llrint) (-9223372036854775807.0L), -9223372036854775807LL, 0, 0, 0);
# endif
#endif
    }

  fesetround (save_round_mode);

  print_max_error ("llrint_downward", 0, 0);
}

static void
llrint_test_upward (void)
{
  int save_round_mode;
  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_UPWARD))
    {
  check_longlong ("llrint_upward (0.0) == 0",  FUNC(llrint) (0.0), 0, 0, 0, 0);
  check_longlong ("llrint_upward (-0) == 0",  FUNC(llrint) (minus_zero), 0, 0, 0, 0);
  check_longlong ("llrint_upward (0.2) == 1",  FUNC(llrint) (0.2L), 1, 0, 0, 0);
  check_longlong ("llrint_upward (-0.2) == 0",  FUNC(llrint) (-0.2L), 0, 0, 0, 0);

  check_longlong ("llrint_upward (1.4) == 2",  FUNC(llrint) (1.4L), 2, 0, 0, 0);
  check_longlong ("llrint_upward (-1.4) == -1",  FUNC(llrint) (-1.4L), -1, 0, 0, 0);

  check_longlong ("llrint_upward (8388600.3) == 8388601",  FUNC(llrint) (8388600.3L), 8388601, 0, 0, 0);
  check_longlong ("llrint_upward (-8388600.3) == -8388600",  FUNC(llrint) (-8388600.3L), -8388600, 0, 0, 0);
#ifndef TEST_FLOAT
  check_long ("llrint_upward (1071930.0008) == 1071931",  FUNC(llrint) (1071930.0008), 1071931, 0, 0, 0);
#endif
      /* Test boundary conditions.  */
      /* 0x1FFFFF */
  check_longlong ("llrint_upward (2097151.0) == 2097151LL",  FUNC(llrint) (2097151.0), 2097151LL, 0, 0, 0);
      /* 0x800000 */
  check_longlong ("llrint_upward (8388608.0) == 8388608LL",  FUNC(llrint) (8388608.0), 8388608LL, 0, 0, 0);
      /* 0x1000000 */
  check_longlong ("llrint_upward (16777216.0) == 16777216LL",  FUNC(llrint) (16777216.0), 16777216LL, 0, 0, 0);
      /* 0x20000000000 */
  check_longlong ("llrint_upward (2199023255552.0) == 2199023255552LL",  FUNC(llrint) (2199023255552.0), 2199023255552LL, 0, 0, 0);
      /* 0x40000000000 */
  check_longlong ("llrint_upward (4398046511104.0) == 4398046511104LL",  FUNC(llrint) (4398046511104.0), 4398046511104LL, 0, 0, 0);
      /* 0x1000000000000 */
  check_longlong ("llrint_upward (281474976710656.0) == 281474976710656LL",  FUNC(llrint) (281474976710656.0), 281474976710656LL, 0, 0, 0);
      /* 0x10000000000000 */
  check_longlong ("llrint_upward (4503599627370496.0) == 4503599627370496LL",  FUNC(llrint) (4503599627370496.0), 4503599627370496LL, 0, 0, 0);
      /* 0x10000080000000 */
  check_longlong ("llrint_upward (4503601774854144.0) == 4503601774854144LL",  FUNC(llrint) (4503601774854144.0), 4503601774854144LL, 0, 0, 0);
      /* 0x20000000000000 */
  check_longlong ("llrint_upward (9007199254740992.0) == 9007199254740992LL",  FUNC(llrint) (9007199254740992.0), 9007199254740992LL, 0, 0, 0);
      /* 0x80000000000000 */
  check_longlong ("llrint_upward (36028797018963968.0) == 36028797018963968LL",  FUNC(llrint) (36028797018963968.0), 36028797018963968LL, 0, 0, 0);
      /* 0x100000000000000 */
  check_longlong ("llrint_upward (72057594037927936.0) == 72057594037927936LL",  FUNC(llrint) (72057594037927936.0), 72057594037927936LL, 0, 0, 0);
#ifdef TEST_LDOUBLE
      /* The input can only be represented in long double.  */
  check_longlong ("llrint_upward (4503599627370495.5) == 4503599627370496LL",  FUNC(llrint) (4503599627370495.5L), 4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint_upward (4503599627370496.25) == 4503599627370497LL",  FUNC(llrint) (4503599627370496.25L), 4503599627370497LL, 0, 0, 0);
  check_longlong ("llrint_upward (4503599627370496.5) == 4503599627370497LL",  FUNC(llrint) (4503599627370496.5L), 4503599627370497LL, 0, 0, 0);
  check_longlong ("llrint_upward (4503599627370496.75) == 4503599627370497LL",  FUNC(llrint) (4503599627370496.75L), 4503599627370497LL, 0, 0, 0);
  check_longlong ("llrint_upward (4503599627370497.5) == 4503599627370498LL",  FUNC(llrint) (4503599627370497.5L), 4503599627370498LL, 0, 0, 0);

  check_longlong ("llrint_upward (4503599627370495.4999999999999) == 4503599627370496LL",  FUNC(llrint) (4503599627370495.4999999999999L), 4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint_upward (4503599627370496.4999999999999) == 4503599627370497LL",  FUNC(llrint) (4503599627370496.4999999999999L), 4503599627370497LL, 0, 0, 0);
  check_longlong ("llrint_upward (4503599627370497.4999999999999) == 4503599627370498LL",  FUNC(llrint) (4503599627370497.4999999999999L), 4503599627370498LL, 0, 0, 0);
  check_longlong ("llrint_upward (4503599627370494.5000000000001) == 4503599627370495LL",  FUNC(llrint) (4503599627370494.5000000000001L), 4503599627370495LL, 0, 0, 0);
  check_longlong ("llrint_upward (4503599627370495.5000000000001) == 4503599627370496LL",  FUNC(llrint) (4503599627370495.5000000000001L), 4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint_upward (4503599627370496.5000000000001) == 4503599627370497LL",  FUNC(llrint) (4503599627370496.5000000000001L), 4503599627370497LL, 0, 0, 0);

  check_longlong ("llrint_upward (-4503599627370495.5) == -4503599627370495LL",  FUNC(llrint) (-4503599627370495.5L), -4503599627370495LL, 0, 0, 0);
  check_longlong ("llrint_upward (-4503599627370496.25) == -4503599627370496LL",  FUNC(llrint) (-4503599627370496.25L), -4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint_upward (-4503599627370496.5) == -4503599627370496LL",  FUNC(llrint) (-4503599627370496.5L), -4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint_upward (-4503599627370496.75) == -4503599627370496LL",  FUNC(llrint) (-4503599627370496.75L), -4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint_upward (-4503599627370497.5) == -4503599627370497LL",  FUNC(llrint) (-4503599627370497.5L), -4503599627370497LL, 0, 0, 0);

  check_longlong ("llrint_upward (-4503599627370495.4999999999999) == -4503599627370495LL",  FUNC(llrint) (-4503599627370495.4999999999999L), -4503599627370495LL, 0, 0, 0);
  check_longlong ("llrint_upward (-4503599627370496.4999999999999) == -4503599627370496LL",  FUNC(llrint) (-4503599627370496.4999999999999L), -4503599627370496LL, 0, 0, 0);
  check_longlong ("llrint_upward (-4503599627370497.4999999999999) == -4503599627370497LL",  FUNC(llrint) (-4503599627370497.4999999999999L), -4503599627370497LL, 0, 0, 0);
  check_longlong ("llrint_upward (-4503599627370494.5000000000001) == -4503599627370494LL",  FUNC(llrint) (-4503599627370494.5000000000001L), -4503599627370494LL, 0, 0, 0);
  check_longlong ("llrint_upward (-4503599627370495.5000000000001) == -4503599627370495LL",  FUNC(llrint) (-4503599627370495.5000000000001L), -4503599627370495LL, 0, 0, 0);
  check_longlong ("llrint_upward (-4503599627370496.5000000000001) == -4503599627370496LL",  FUNC(llrint) (-4503599627370496.5000000000001L), -4503599627370496LL, 0, 0, 0);

  check_longlong ("llrint_upward (9007199254740991.5) == 9007199254740992LL",  FUNC(llrint) (9007199254740991.5L), 9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint_upward (9007199254740992.25) == 9007199254740993LL",  FUNC(llrint) (9007199254740992.25L), 9007199254740993LL, 0, 0, 0);
  check_longlong ("llrint_upward (9007199254740992.5) == 9007199254740993LL",  FUNC(llrint) (9007199254740992.5L), 9007199254740993LL, 0, 0, 0);
  check_longlong ("llrint_upward (9007199254740992.75) == 9007199254740993LL",  FUNC(llrint) (9007199254740992.75L), 9007199254740993LL, 0, 0, 0);
  check_longlong ("llrint_upward (9007199254740993.5) == 9007199254740994LL",  FUNC(llrint) (9007199254740993.5L), 9007199254740994LL, 0, 0, 0);

  check_longlong ("llrint_upward (9007199254740991.4999999999999) == 9007199254740992LL",  FUNC(llrint) (9007199254740991.4999999999999L), 9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint_upward (9007199254740992.4999999999999) == 9007199254740993LL",  FUNC(llrint) (9007199254740992.4999999999999L), 9007199254740993LL, 0, 0, 0);
  check_longlong ("llrint_upward (9007199254740993.4999999999999) == 9007199254740994LL",  FUNC(llrint) (9007199254740993.4999999999999L), 9007199254740994LL, 0, 0, 0);
  check_longlong ("llrint_upward (9007199254740991.5000000000001) == 9007199254740992LL",  FUNC(llrint) (9007199254740991.5000000000001L), 9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint_upward (9007199254740992.5000000000001) == 9007199254740993LL",  FUNC(llrint) (9007199254740992.5000000000001L), 9007199254740993LL, 0, 0, 0);
  check_longlong ("llrint_upward (9007199254740993.5000000000001) == 9007199254740994LL",  FUNC(llrint) (9007199254740993.5000000000001L), 9007199254740994LL, 0, 0, 0);

  check_longlong ("llrint_upward (-9007199254740991.5) == -9007199254740991LL",  FUNC(llrint) (-9007199254740991.5L), -9007199254740991LL, 0, 0, 0);
  check_longlong ("llrint_upward (-9007199254740992.25) == -9007199254740992LL",  FUNC(llrint) (-9007199254740992.25L), -9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint_upward (-9007199254740992.5) == -9007199254740992LL",  FUNC(llrint) (-9007199254740992.5L), -9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint_upward (-9007199254740992.75) == -9007199254740992LL",  FUNC(llrint) (-9007199254740992.75L), -9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint_upward (-9007199254740993.5) == -9007199254740993LL",  FUNC(llrint) (-9007199254740993.5L), -9007199254740993LL, 0, 0, 0);

  check_longlong ("llrint_upward (-9007199254740991.4999999999999) == -9007199254740991LL",  FUNC(llrint) (-9007199254740991.4999999999999L), -9007199254740991LL, 0, 0, 0);
  check_longlong ("llrint_upward (-9007199254740992.4999999999999) == -9007199254740992LL",  FUNC(llrint) (-9007199254740992.4999999999999L), -9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint_upward (-9007199254740993.4999999999999) == -9007199254740993LL",  FUNC(llrint) (-9007199254740993.4999999999999L), -9007199254740993LL, 0, 0, 0);
  check_longlong ("llrint_upward (-9007199254740991.5000000000001) == -9007199254740991LL",  FUNC(llrint) (-9007199254740991.5000000000001L), -9007199254740991LL, 0, 0, 0);
  check_longlong ("llrint_upward (-9007199254740992.5000000000001) == -9007199254740992LL",  FUNC(llrint) (-9007199254740992.5000000000001L), -9007199254740992LL, 0, 0, 0);
  check_longlong ("llrint_upward (-9007199254740993.5000000000001) == -9007199254740993LL",  FUNC(llrint) (-9007199254740993.5000000000001L), -9007199254740993LL, 0, 0, 0);

  check_longlong ("llrint_upward (72057594037927935.5) == 72057594037927936LL",  FUNC(llrint) (72057594037927935.5L), 72057594037927936LL, 0, 0, 0);
  check_longlong ("llrint_upward (72057594037927936.25) == 72057594037927937LL",  FUNC(llrint) (72057594037927936.25L), 72057594037927937LL, 0, 0, 0);
  check_longlong ("llrint_upward (72057594037927936.5) == 72057594037927937LL",  FUNC(llrint) (72057594037927936.5L), 72057594037927937LL, 0, 0, 0);
  check_longlong ("llrint_upward (72057594037927936.75) == 72057594037927937LL",  FUNC(llrint) (72057594037927936.75L), 72057594037927937LL, 0, 0, 0);
  check_longlong ("llrint_upward (72057594037927937.5) == 72057594037927938LL",  FUNC(llrint) (72057594037927937.5L), 72057594037927938LL, 0, 0, 0);

  check_longlong ("llrint_upward (-72057594037927935.5) == -72057594037927935LL",  FUNC(llrint) (-72057594037927935.5L), -72057594037927935LL, 0, 0, 0);
  check_longlong ("llrint_upward (-72057594037927936.25) == -72057594037927936LL",  FUNC(llrint) (-72057594037927936.25L), -72057594037927936LL, 0, 0, 0);
  check_longlong ("llrint_upward (-72057594037927936.5) == -72057594037927936LL",  FUNC(llrint) (-72057594037927936.5L), -72057594037927936LL, 0, 0, 0);
  check_longlong ("llrint_upward (-72057594037927936.75) == -72057594037927936LL",  FUNC(llrint) (-72057594037927936.75L), -72057594037927936LL, 0, 0, 0);
  check_longlong ("llrint_upward (-72057594037927937.5) == -72057594037927937LL",  FUNC(llrint) (-72057594037927937.5L), -72057594037927937LL, 0, 0, 0);

# if LDBL_MANT_DIG > 100
  check_longlong ("llrint_upward (9223372036854775805.5) == 9223372036854775806LL",  FUNC(llrint) (9223372036854775805.5L), 9223372036854775806LL, 0, 0, 0);
  check_longlong ("llrint_upward (-9223372036854775805.5) == -9223372036854775805LL",  FUNC(llrint) (-9223372036854775805.5L), -9223372036854775805LL, 0, 0, 0);
  check_longlong ("llrint_upward (9223372036854775806.0) == 9223372036854775806LL",  FUNC(llrint) (9223372036854775806.0L), 9223372036854775806LL, 0, 0, 0);
  check_longlong ("llrint_upward (-9223372036854775806.0) == -9223372036854775806LL",  FUNC(llrint) (-9223372036854775806.0L), -9223372036854775806LL, 0, 0, 0);
  check_longlong ("llrint_upward (9223372036854775806.25) == 9223372036854775807LL",  FUNC(llrint) (9223372036854775806.25L), 9223372036854775807LL, 0, 0, 0);
  check_longlong ("llrint_upward (-9223372036854775806.25) == -9223372036854775806LL",  FUNC(llrint) (-9223372036854775806.25L), -9223372036854775806LL, 0, 0, 0);
  check_longlong ("llrint_upward (9223372036854775806.5) == 9223372036854775807",  FUNC(llrint) (9223372036854775806.5L), 9223372036854775807L, 0, 0, 0);
  check_longlong ("llrint_upward (-9223372036854775806.5) == -9223372036854775806LL",  FUNC(llrint) (-9223372036854775806.5L), -9223372036854775806LL, 0, 0, 0);
  check_longlong ("llrint_upward (9223372036854775806.75) == 9223372036854775807LL",  FUNC(llrint) (9223372036854775806.75L), 9223372036854775807LL, 0, 0, 0);
  check_longlong ("llrint_upward (-9223372036854775806.75) == -9223372036854775806LL",  FUNC(llrint) (-9223372036854775806.75L), -9223372036854775806LL, 0, 0, 0);
  check_longlong ("llrint_upward (9223372036854775807.0) == 9223372036854775807LL",  FUNC(llrint) (9223372036854775807.0L), 9223372036854775807LL, 0, 0, 0);
  check_longlong ("llrint_upward (-9223372036854775807.0) == -9223372036854775807LL",  FUNC(llrint) (-9223372036854775807.0L), -9223372036854775807LL, 0, 0, 0);
# endif
#endif
    }

  fesetround (save_round_mode);

  print_max_error ("llrint_upward", 0, 0);
}


static void
log_test (void)
{
  errno = 0;
  FUNC(log) (1);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;
  init_max_error ();

  check_float ("log (0) == -inf",  FUNC(log) (0), minus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_float ("log (-0) == -inf",  FUNC(log) (minus_zero), minus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);

  check_float ("log (1) == 0",  FUNC(log) (1), 0, 0, 0, 0);

  check_float ("log (-1) == NaN",  FUNC(log) (-1), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("log (-max_value) == NaN",  FUNC(log) (-max_value), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("log (-inf) == NaN",  FUNC(log) (minus_infty), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("log (inf) == inf",  FUNC(log) (plus_infty), plus_infty, 0, 0, 0);
  check_float ("log (NaN) == NaN",  FUNC(log) (nan_value), nan_value, 0, 0, 0);

  check_float ("log (e) == 1",  FUNC(log) (M_El), 1, 0, 0, 0);
  check_float ("log (1.0 / M_El) == -1",  FUNC(log) (1.0 / M_El), -1, 0, 0, 0);
  check_float ("log (2) == M_LN2l",  FUNC(log) (2), M_LN2l, 0, 0, 0);
  check_float ("log (10) == M_LN10l",  FUNC(log) (10), M_LN10l, 0, 0, 0);
  check_float ("log (0.75) == -0.287682072451780927439219005993827432",  FUNC(log) (0.75L), -0.287682072451780927439219005993827432L, 0, 0, 0);

  print_max_error ("log", 0, 0);
}


static void
log10_test (void)
{
  errno = 0;
  FUNC(log10) (1);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_float ("log10 (0) == -inf",  FUNC(log10) (0), minus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_float ("log10 (-0) == -inf",  FUNC(log10) (minus_zero), minus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);

  check_float ("log10 (1) == 0",  FUNC(log10) (1), 0, 0, 0, 0);

  /* log10 (x) == NaN plus invalid exception if x < 0.  */
  check_float ("log10 (-1) == NaN",  FUNC(log10) (-1), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("log10 (-max_value) == NaN",  FUNC(log10) (-max_value), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("log10 (-inf) == NaN",  FUNC(log10) (minus_infty), nan_value, 0, 0, INVALID_EXCEPTION);

  check_float ("log10 (inf) == inf",  FUNC(log10) (plus_infty), plus_infty, 0, 0, 0);
  check_float ("log10 (NaN) == NaN",  FUNC(log10) (nan_value), nan_value, 0, 0, 0);

  check_float ("log10 (0.1) == -1",  FUNC(log10) (0.1L), -1, 0, 0, 0);
  check_float ("log10 (10.0) == 1",  FUNC(log10) (10.0), 1, 0, 0, 0);
  check_float ("log10 (100.0) == 2",  FUNC(log10) (100.0), 2, 0, 0, 0);
  check_float ("log10 (10000.0) == 4",  FUNC(log10) (10000.0), 4, 0, 0, 0);
  check_float ("log10 (e) == log10(e)",  FUNC(log10) (M_El), M_LOG10El, DELTA3447, 0, 0);
  check_float ("log10 (0.75) == -0.124938736608299953132449886193870744",  FUNC(log10) (0.75L), -0.124938736608299953132449886193870744L, DELTA3448, 0, 0);

  print_max_error ("log10", DELTAlog10, 0);
}


static void
log1p_test (void)
{
  errno = 0;
  FUNC(log1p) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_float ("log1p (0) == 0",  FUNC(log1p) (0), 0, 0, 0, 0);
  check_float ("log1p (-0) == -0",  FUNC(log1p) (minus_zero), minus_zero, 0, 0, 0);

  check_float ("log1p (-1) == -inf",  FUNC(log1p) (-1), minus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_float ("log1p (-2) == NaN",  FUNC(log1p) (-2), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("log1p (-max_value) == NaN",  FUNC(log1p) (-max_value), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("log1p (-inf) == NaN",  FUNC(log1p) (minus_infty), nan_value, 0, 0, INVALID_EXCEPTION);

  check_float ("log1p (inf) == inf",  FUNC(log1p) (plus_infty), plus_infty, 0, 0, 0);
  check_float ("log1p (NaN) == NaN",  FUNC(log1p) (nan_value), nan_value, 0, 0, 0);

  check_float ("log1p (M_El - 1.0) == 1",  FUNC(log1p) (M_El - 1.0), 1, 0, 0, 0);

  check_float ("log1p (-0.25) == -0.287682072451780927439219005993827432",  FUNC(log1p) (-0.25L), -0.287682072451780927439219005993827432L, DELTA3458, 0, 0);
  check_float ("log1p (-0.875) == -2.07944154167983592825169636437452970",  FUNC(log1p) (-0.875), -2.07944154167983592825169636437452970L, 0, 0, 0);

  print_max_error ("log1p", DELTAlog1p, 0);
}


static void
log2_test (void)
{
  errno = 0;
  FUNC(log2) (1);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_float ("log2 (0) == -inf",  FUNC(log2) (0), minus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_float ("log2 (-0) == -inf",  FUNC(log2) (minus_zero), minus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);

  check_float ("log2 (1) == 0",  FUNC(log2) (1), 0, 0, 0, 0);

  check_float ("log2 (-1) == NaN",  FUNC(log2) (-1), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("log2 (-max_value) == NaN",  FUNC(log2) (-max_value), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("log2 (-inf) == NaN",  FUNC(log2) (minus_infty), nan_value, 0, 0, INVALID_EXCEPTION);

  check_float ("log2 (inf) == inf",  FUNC(log2) (plus_infty), plus_infty, 0, 0, 0);
  check_float ("log2 (NaN) == NaN",  FUNC(log2) (nan_value), nan_value, 0, 0, 0);

  check_float ("log2 (e) == M_LOG2El",  FUNC(log2) (M_El), M_LOG2El, 0, 0, 0);
  check_float ("log2 (2.0) == 1",  FUNC(log2) (2.0), 1, 0, 0, 0);
  check_float ("log2 (16.0) == 4",  FUNC(log2) (16.0), 4, 0, 0, 0);
  check_float ("log2 (256.0) == 8",  FUNC(log2) (256.0), 8, 0, 0, 0);
  check_float ("log2 (0.75) == -.415037499278843818546261056052183492",  FUNC(log2) (0.75L), -.415037499278843818546261056052183492L, 0, 0, 0);

  print_max_error ("log2", 0, 0);
}


static void
logb_test (void)
{
  init_max_error ();

  check_float ("logb (inf) == inf",  FUNC(logb) (plus_infty), plus_infty, 0, 0, 0);
  check_float ("logb (-inf) == inf",  FUNC(logb) (minus_infty), plus_infty, 0, 0, 0);

  check_float ("logb (0) == -inf",  FUNC(logb) (0), minus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);

  check_float ("logb (-0) == -inf",  FUNC(logb) (minus_zero), minus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_float ("logb (NaN) == NaN",  FUNC(logb) (nan_value), nan_value, 0, 0, 0);

  check_float ("logb (1) == 0",  FUNC(logb) (1), 0, 0, 0, 0);
  check_float ("logb (e) == 1",  FUNC(logb) (M_El), 1, 0, 0, 0);
  check_float ("logb (1024) == 10",  FUNC(logb) (1024), 10, 0, 0, 0);
  check_float ("logb (-2000) == 10",  FUNC(logb) (-2000), 10, 0, 0, 0);

  check_float ("logb (0x0.1p-127) == -131",  FUNC(logb) (0x0.1p-127), -131, 0, 0, 0);
  check_float ("logb (0x0.01p-127) == -135",  FUNC(logb) (0x0.01p-127), -135, 0, 0, 0);
  check_float ("logb (0x0.011p-127) == -135",  FUNC(logb) (0x0.011p-127), -135, 0, 0, 0);
#ifndef TEST_FLOAT
  check_float ("logb (0x0.8p-1022) == -1023",  FUNC(logb) (0x0.8p-1022), -1023, 0, 0, 0);
  check_float ("logb (0x0.1p-1022) == -1026",  FUNC(logb) (0x0.1p-1022), -1026, 0, 0, 0);
  check_float ("logb (0x0.00111p-1022) == -1034",  FUNC(logb) (0x0.00111p-1022), -1034, 0, 0, 0);
  check_float ("logb (0x0.00001p-1022) == -1042",  FUNC(logb) (0x0.00001p-1022), -1042, 0, 0, 0);
  check_float ("logb (0x0.000011p-1022) == -1042",  FUNC(logb) (0x0.000011p-1022), -1042, 0, 0, 0);
  check_float ("logb (0x0.0000000000001p-1022) == -1074",  FUNC(logb) (0x0.0000000000001p-1022), -1074, 0, 0, 0);
#endif
#if defined TEST_LDOUBLE && LDBL_MIN_EXP - LDBL_MANT_DIG <= -16400
  check_float ("logb (0x1p-16400) == -16400",  FUNC(logb) (0x1p-16400L), -16400, 0, 0, 0);
  check_float ("logb (0x.00000000001p-16382) == -16426",  FUNC(logb) (0x.00000000001p-16382L), -16426, 0, 0, 0);
#endif

  print_max_error ("logb", 0, 0);
}

static void
logb_test_downward (void)
{
  int save_round_mode;
  errno = 0;

  FUNC(logb) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_DOWNWARD))
    {

      /* IEEE 754-2008 says (section 5.3.3) that "logB(1) is +0.".  Libm
         should not return -0 from logb in any rounding mode.  PowerPC32 has
         failed with this test for power4 logb (and logbl on all PowerPC
         platforms) in the past due to instruction selection.  GCC PR 52775
         provides the availability of the fcfid insn in 32-bit mode which
         eliminates the use of fsub in this instance and prevents the negative
         signed 0.0.  */

      /* BZ #887  */
  check_float ("logb_downward (1.000e+0) == +0",  FUNC(logb) (1.000e+0), plus_zero, 0, 0, 0);
    }

  fesetround (save_round_mode);

  print_max_error ("logb_downward", 0, 0);
}

static void
lround_test (void)
{
  init_max_error ();

  check_long ("lround (0) == 0",  FUNC(lround) (0), 0, 0, 0, 0);
  check_long ("lround (-0) == 0",  FUNC(lround) (minus_zero), 0, 0, 0, 0);
  check_long ("lround (0.2) == 0.0",  FUNC(lround) (0.2L), 0.0, 0, 0, 0);
  check_long ("lround (-0.2) == 0",  FUNC(lround) (-0.2L), 0, 0, 0, 0);
  check_long ("lround (0.5) == 1",  FUNC(lround) (0.5), 1, 0, 0, 0);
  check_long ("lround (-0.5) == -1",  FUNC(lround) (-0.5), -1, 0, 0, 0);
  check_long ("lround (0.8) == 1",  FUNC(lround) (0.8L), 1, 0, 0, 0);
  check_long ("lround (-0.8) == -1",  FUNC(lround) (-0.8L), -1, 0, 0, 0);
  check_long ("lround (1.5) == 2",  FUNC(lround) (1.5), 2, 0, 0, 0);
  check_long ("lround (-1.5) == -2",  FUNC(lround) (-1.5), -2, 0, 0, 0);
  check_long ("lround (22514.5) == 22515",  FUNC(lround) (22514.5), 22515, 0, 0, 0);
  check_long ("lround (-22514.5) == -22515",  FUNC(lround) (-22514.5), -22515, 0, 0, 0);
  check_long ("lround (1071930.0008) == 1071930",  FUNC(lround) (1071930.0008), 1071930, 0, 0, 0);
#ifndef TEST_FLOAT
  check_long ("lround (1073741824.01) == 1073741824",  FUNC(lround) (1073741824.01), 1073741824, 0, 0, 0);
# if LONG_MAX > 281474976710656
  check_long ("lround (281474976710656.025) == 281474976710656",  FUNC(lround) (281474976710656.025), 281474976710656, 0, 0, 0);
  check_long ("lround (18014398509481974) == 18014398509481974",  FUNC(lround) (18014398509481974), 18014398509481974, 0, 0, 0);
# endif
  check_long ("lround (2097152.5) == 2097153",  FUNC(lround) (2097152.5), 2097153, 0, 0, 0);
  check_long ("lround (-2097152.5) == -2097153",  FUNC(lround) (-2097152.5), -2097153, 0, 0, 0);
  /* nextafter(0.5,-1)  */
  check_long ("lround (0x1.fffffffffffffp-2) == 0",  FUNC(lround) (0x1.fffffffffffffp-2), 0, 0, 0, 0);
  /* nextafter(-0.5,1)  */
  check_long ("lround (-0x1.fffffffffffffp-2) == 0",  FUNC(lround) (-0x1.fffffffffffffp-2), 0, 0, 0, 0);
#else
  /* nextafter(0.5,-1)  */
  check_long ("lround (0x1.fffffp-2) == 0",  FUNC(lround) (0x1.fffffp-2), 0, 0, 0, 0);
  /* nextafter(-0.5,1)  */
  check_long ("lround (-0x1.fffffp-2) == 0",  FUNC(lround) (-0x1.fffffp-2), 0, 0, 0, 0);
  check_long ("lround (0x1.fffffep+23) == 16777215",  FUNC(lround) (0x1.fffffep+23), 16777215, 0, 0, 0);
  check_long ("lround (-0x1.fffffep+23) == -16777215",  FUNC(lround) (-0x1.fffffep+23), -16777215, 0, 0, 0);
#endif
  print_max_error ("lround", 0, 0);
}


static void
llround_test (void)
{
  init_max_error ();

  check_longlong ("llround (0) == 0",  FUNC(llround) (0), 0, 0, 0, 0);
  check_longlong ("llround (-0) == 0",  FUNC(llround) (minus_zero), 0, 0, 0, 0);
  check_longlong ("llround (0.2) == 0.0",  FUNC(llround) (0.2L), 0.0, 0, 0, 0);
  check_longlong ("llround (-0.2) == 0",  FUNC(llround) (-0.2L), 0, 0, 0, 0);
  check_longlong ("llround (0.5) == 1",  FUNC(llround) (0.5), 1, 0, 0, 0);
  check_longlong ("llround (-0.5) == -1",  FUNC(llround) (-0.5), -1, 0, 0, 0);
  check_longlong ("llround (0.8) == 1",  FUNC(llround) (0.8L), 1, 0, 0, 0);
  check_longlong ("llround (-0.8) == -1",  FUNC(llround) (-0.8L), -1, 0, 0, 0);
  check_longlong ("llround (1.5) == 2",  FUNC(llround) (1.5), 2, 0, 0, 0);
  check_longlong ("llround (-1.5) == -2",  FUNC(llround) (-1.5), -2, 0, 0, 0);
  check_longlong ("llround (22514.5) == 22515",  FUNC(llround) (22514.5), 22515, 0, 0, 0);
  check_longlong ("llround (-22514.5) == -22515",  FUNC(llround) (-22514.5), -22515, 0, 0, 0);
  check_long ("llround (1071930.0008) == 1071930",  FUNC(llround) (1071930.0008), 1071930, 0, 0, 0);
#ifndef TEST_FLOAT
  check_longlong ("llround (2097152.5) == 2097153",  FUNC(llround) (2097152.5), 2097153, 0, 0, 0);
  check_longlong ("llround (-2097152.5) == -2097153",  FUNC(llround) (-2097152.5), -2097153, 0, 0, 0);
  check_longlong ("llround (34359738368.5) == 34359738369ll",  FUNC(llround) (34359738368.5), 34359738369ll, 0, 0, 0);
  check_longlong ("llround (-34359738368.5) == -34359738369ll",  FUNC(llround) (-34359738368.5), -34359738369ll, 0, 0, 0);
  check_longlong ("llround (-3.65309740835E17) == -365309740835000000LL",  FUNC(llround) (-3.65309740835E17), -365309740835000000LL, 0, 0, 0);
#endif

  /* Test boundary conditions.  */
  /* 0x1FFFFF */
  check_longlong ("llround (2097151.0) == 2097151LL",  FUNC(llround) (2097151.0), 2097151LL, 0, 0, 0);
  /* 0x800000 */
  check_longlong ("llround (8388608.0) == 8388608LL",  FUNC(llround) (8388608.0), 8388608LL, 0, 0, 0);
  /* 0x1000000 */
  check_longlong ("llround (16777216.0) == 16777216LL",  FUNC(llround) (16777216.0), 16777216LL, 0, 0, 0);
  /* 0x20000000000 */
  check_longlong ("llround (2199023255552.0) == 2199023255552LL",  FUNC(llround) (2199023255552.0), 2199023255552LL, 0, 0, 0);
  /* 0x40000000000 */
  check_longlong ("llround (4398046511104.0) == 4398046511104LL",  FUNC(llround) (4398046511104.0), 4398046511104LL, 0, 0, 0);
  /* 0x1000000000000 */
  check_longlong ("llround (281474976710656.0) == 281474976710656LL",  FUNC(llround) (281474976710656.0), 281474976710656LL, 0, 0, 0);
  /* 0x10000000000000 */
  check_longlong ("llround (4503599627370496.0) == 4503599627370496LL",  FUNC(llround) (4503599627370496.0), 4503599627370496LL, 0, 0, 0);
  /* 0x10000080000000 */
  check_longlong ("llround (4503601774854144.0) == 4503601774854144LL",  FUNC(llround) (4503601774854144.0), 4503601774854144LL, 0, 0, 0);
  /* 0x20000000000000 */
  check_longlong ("llround (9007199254740992.0) == 9007199254740992LL",  FUNC(llround) (9007199254740992.0), 9007199254740992LL, 0, 0, 0);
  /* 0x80000000000000 */
  check_longlong ("llround (36028797018963968.0) == 36028797018963968LL",  FUNC(llround) (36028797018963968.0), 36028797018963968LL, 0, 0, 0);
  /* 0x100000000000000 */
  check_longlong ("llround (72057594037927936.0) == 72057594037927936LL",  FUNC(llround) (72057594037927936.0), 72057594037927936LL, 0, 0, 0);

#ifndef TEST_FLOAT
  /* 0x100000000 */
  check_longlong ("llround (4294967295.5) == 4294967296LL",  FUNC(llround) (4294967295.5), 4294967296LL, 0, 0, 0);
  /* 0x200000000 */
  check_longlong ("llround (8589934591.5) == 8589934592LL",  FUNC(llround) (8589934591.5), 8589934592LL, 0, 0, 0);

  /* nextafter(0.5,-1)  */
  check_longlong ("llround (0x1.fffffffffffffp-2) == 0",  FUNC(llround) (0x1.fffffffffffffp-2), 0, 0, 0, 0);
  /* nextafter(-0.5,1)  */
  check_longlong ("llround (-0x1.fffffffffffffp-2) == 0",  FUNC(llround) (-0x1.fffffffffffffp-2), 0, 0, 0, 0);
  /* On PowerPC an exponent of '52' is the largest incrementally
   * representable sequence of whole-numbers in the 'double' range.  We test
   * lround to make sure that a guard bit set during the lround operation
   * hasn't forced an erroneous shift giving us an incorrect result.  The odd
   * numbers between +-(2^52+1 and 2^53-1) are affected since they have the
   * rightmost bit set.  */
  /* +-(2^52+1)  */
  check_longlong ("llround (0x1.0000000000001p+52) == 4503599627370497LL",  FUNC(llround) (0x1.0000000000001p+52), 4503599627370497LL, 0, 0, 0);
  check_longlong ("llround (-0x1.0000000000001p+52) == -4503599627370497LL",  FUNC(llround) (-0x1.0000000000001p+52), -4503599627370497LL, 0, 0, 0);
  /* +-(2^53-1): Input is the last (positive and negative) incrementally
   * representable whole-number in the 'double' range that might round
   * erroneously.  */
  check_longlong ("llround (0x1.fffffffffffffp+52) == 9007199254740991LL",  FUNC(llround) (0x1.fffffffffffffp+52), 9007199254740991LL, 0, 0, 0);
  check_longlong ("llround (-0x1.fffffffffffffp+52) == -9007199254740991LL",  FUNC(llround) (-0x1.fffffffffffffp+52), -9007199254740991LL, 0, 0, 0);
#else
  /* nextafter(0.5,-1)  */
  check_longlong ("llround (0x1.fffffep-2) == 0",  FUNC(llround) (0x1.fffffep-2), 0, 0, 0, 0);
  /* nextafter(-0.5,1)  */
  check_longlong ("llround (-0x1.fffffep-2) == 0",  FUNC(llround) (-0x1.fffffep-2), 0, 0, 0, 0);
  /* As above, on PowerPC an exponent of '23' is the largest incrementally
   * representable sequence of whole-numbers in the 'float' range.
   * Likewise, numbers between +-(2^23+1 and 2^24-1) are affected.  */
  check_longlong ("llround (0x1.000002p+23) == 8388609",  FUNC(llround) (0x1.000002p+23), 8388609, 0, 0, 0);
  check_longlong ("llround (-0x1.000002p+23) == -8388609",  FUNC(llround) (-0x1.000002p+23), -8388609, 0, 0, 0);
  check_longlong ("llround (0x1.fffffep+23) == 16777215",  FUNC(llround) (0x1.fffffep+23), 16777215, 0, 0, 0);
  check_longlong ("llround (-0x1.fffffep+23) == -16777215",  FUNC(llround) (-0x1.fffffep+23), -16777215, 0, 0, 0);
#endif


#ifdef TEST_LDOUBLE
  /* The input can only be represented in long double.  */
  check_longlong ("llround (4503599627370495.5) == 4503599627370496LL",  FUNC(llround) (4503599627370495.5L), 4503599627370496LL, 0, 0, 0);
  check_longlong ("llround (4503599627370496.25) == 4503599627370496LL",  FUNC(llround) (4503599627370496.25L), 4503599627370496LL, 0, 0, 0);
  check_longlong ("llround (4503599627370496.5) == 4503599627370497LL",  FUNC(llround) (4503599627370496.5L), 4503599627370497LL, 0, 0, 0);
  check_longlong ("llround (4503599627370496.75) == 4503599627370497LL",  FUNC(llround) (4503599627370496.75L), 4503599627370497LL, 0, 0, 0);
  check_longlong ("llround (4503599627370497.5) == 4503599627370498LL",  FUNC(llround) (4503599627370497.5L), 4503599627370498LL, 0, 0, 0);

# if LDBL_MANT_DIG > 100
  check_longlong ("llround (4503599627370495.4999999999999) == 4503599627370495LL",  FUNC(llround) (4503599627370495.4999999999999L), 4503599627370495LL, 0, 0, 0);
  check_longlong ("llround (4503599627370496.4999999999999) == 4503599627370496LL",  FUNC(llround) (4503599627370496.4999999999999L), 4503599627370496LL, 0, 0, 0);
  check_longlong ("llround (4503599627370497.4999999999999) == 4503599627370497LL",  FUNC(llround) (4503599627370497.4999999999999L), 4503599627370497LL, 0, 0, 0);
  check_longlong ("llround (4503599627370494.5000000000001) == 4503599627370495LL",  FUNC(llround) (4503599627370494.5000000000001L), 4503599627370495LL, 0, 0, 0);
  check_longlong ("llround (4503599627370495.5000000000001) == 4503599627370496LL",  FUNC(llround) (4503599627370495.5000000000001L), 4503599627370496LL, 0, 0, 0);
  check_longlong ("llround (4503599627370496.5000000000001) == 4503599627370497LL",  FUNC(llround) (4503599627370496.5000000000001L), 4503599627370497LL, 0, 0, 0);

  check_longlong ("llround (-4503599627370495.4999999999999) == -4503599627370495LL",  FUNC(llround) (-4503599627370495.4999999999999L), -4503599627370495LL, 0, 0, 0);
  check_longlong ("llround (-4503599627370496.4999999999999) == -4503599627370496LL",  FUNC(llround) (-4503599627370496.4999999999999L), -4503599627370496LL, 0, 0, 0);
  check_longlong ("llround (-4503599627370497.4999999999999) == -4503599627370497LL",  FUNC(llround) (-4503599627370497.4999999999999L), -4503599627370497LL, 0, 0, 0);
  check_longlong ("llround (-4503599627370494.5000000000001) == -4503599627370495LL",  FUNC(llround) (-4503599627370494.5000000000001L), -4503599627370495LL, 0, 0, 0);
  check_longlong ("llround (-4503599627370495.5000000000001) == -4503599627370496LL",  FUNC(llround) (-4503599627370495.5000000000001L), -4503599627370496LL, 0, 0, 0);
  check_longlong ("llround (-4503599627370496.5000000000001) == -4503599627370497LL",  FUNC(llround) (-4503599627370496.5000000000001L), -4503599627370497LL, 0, 0, 0);
# endif

  check_longlong ("llround (-4503599627370495.5) == -4503599627370496LL",  FUNC(llround) (-4503599627370495.5L), -4503599627370496LL, 0, 0, 0);
  check_longlong ("llround (-4503599627370496.25) == -4503599627370496LL",  FUNC(llround) (-4503599627370496.25L), -4503599627370496LL, 0, 0, 0);
  check_longlong ("llround (-4503599627370496.5) == -4503599627370497LL",  FUNC(llround) (-4503599627370496.5L), -4503599627370497LL, 0, 0, 0);
  check_longlong ("llround (-4503599627370496.75) == -4503599627370497LL",  FUNC(llround) (-4503599627370496.75L), -4503599627370497LL, 0, 0, 0);
  check_longlong ("llround (-4503599627370497.5) == -4503599627370498LL",  FUNC(llround) (-4503599627370497.5L), -4503599627370498LL, 0, 0, 0);

  check_longlong ("llround (9007199254740991.5) == 9007199254740992LL",  FUNC(llround) (9007199254740991.5L), 9007199254740992LL, 0, 0, 0);
  check_longlong ("llround (9007199254740992.25) == 9007199254740992LL",  FUNC(llround) (9007199254740992.25L), 9007199254740992LL, 0, 0, 0);
  check_longlong ("llround (9007199254740992.5) == 9007199254740993LL",  FUNC(llround) (9007199254740992.5L), 9007199254740993LL, 0, 0, 0);
  check_longlong ("llround (9007199254740992.75) == 9007199254740993LL",  FUNC(llround) (9007199254740992.75L), 9007199254740993LL, 0, 0, 0);
  check_longlong ("llround (9007199254740993.5) == 9007199254740994LL",  FUNC(llround) (9007199254740993.5L), 9007199254740994LL, 0, 0, 0);

# if LDBL_MANT_DIG > 100
  check_longlong ("llround (9007199254740991.4999999999999) == 9007199254740991LL",  FUNC(llround) (9007199254740991.4999999999999L), 9007199254740991LL, 0, 0, 0);
  check_longlong ("llround (9007199254740992.4999999999999) == 9007199254740992LL",  FUNC(llround) (9007199254740992.4999999999999L), 9007199254740992LL, 0, 0, 0);
  check_longlong ("llround (9007199254740993.4999999999999) == 9007199254740993LL",  FUNC(llround) (9007199254740993.4999999999999L), 9007199254740993LL, 0, 0, 0);
  check_longlong ("llround (9007199254740991.5000000000001) == 9007199254740992LL",  FUNC(llround) (9007199254740991.5000000000001L), 9007199254740992LL, 0, 0, 0);
  check_longlong ("llround (9007199254740992.5000000000001) == 9007199254740993LL",  FUNC(llround) (9007199254740992.5000000000001L), 9007199254740993LL, 0, 0, 0);
  check_longlong ("llround (9007199254740993.5000000000001) == 9007199254740994LL",  FUNC(llround) (9007199254740993.5000000000001L), 9007199254740994LL, 0, 0, 0);

  check_longlong ("llround (-9007199254740991.4999999999999) == -9007199254740991LL",  FUNC(llround) (-9007199254740991.4999999999999L), -9007199254740991LL, 0, 0, 0);
  check_longlong ("llround (-9007199254740992.4999999999999) == -9007199254740992LL",  FUNC(llround) (-9007199254740992.4999999999999L), -9007199254740992LL, 0, 0, 0);
  check_longlong ("llround (-9007199254740993.4999999999999) == -9007199254740993LL",  FUNC(llround) (-9007199254740993.4999999999999L), -9007199254740993LL, 0, 0, 0);
  check_longlong ("llround (-9007199254740991.5000000000001) == -9007199254740992LL",  FUNC(llround) (-9007199254740991.5000000000001L), -9007199254740992LL, 0, 0, 0);
  check_longlong ("llround (-9007199254740992.5000000000001) == -9007199254740993LL",  FUNC(llround) (-9007199254740992.5000000000001L), -9007199254740993LL, 0, 0, 0);
  check_longlong ("llround (-9007199254740993.5000000000001) == -9007199254740994LL",  FUNC(llround) (-9007199254740993.5000000000001L), -9007199254740994LL, 0, 0, 0);
# endif

  check_longlong ("llround (-9007199254740991.5) == -9007199254740992LL",  FUNC(llround) (-9007199254740991.5L), -9007199254740992LL, 0, 0, 0);
  check_longlong ("llround (-9007199254740992.25) == -9007199254740992LL",  FUNC(llround) (-9007199254740992.25L), -9007199254740992LL, 0, 0, 0);
  check_longlong ("llround (-9007199254740992.5) == -9007199254740993LL",  FUNC(llround) (-9007199254740992.5L), -9007199254740993LL, 0, 0, 0);
  check_longlong ("llround (-9007199254740992.75) == -9007199254740993LL",  FUNC(llround) (-9007199254740992.75L), -9007199254740993LL, 0, 0, 0);
  check_longlong ("llround (-9007199254740993.5) == -9007199254740994LL",  FUNC(llround) (-9007199254740993.5L), -9007199254740994LL, 0, 0, 0);

  check_longlong ("llround (72057594037927935.5) == 72057594037927936LL",  FUNC(llround) (72057594037927935.5L), 72057594037927936LL, 0, 0, 0);
  check_longlong ("llround (72057594037927936.25) == 72057594037927936LL",  FUNC(llround) (72057594037927936.25L), 72057594037927936LL, 0, 0, 0);
  check_longlong ("llround (72057594037927936.5) == 72057594037927937LL",  FUNC(llround) (72057594037927936.5L), 72057594037927937LL, 0, 0, 0);
  check_longlong ("llround (72057594037927936.75) == 72057594037927937LL",  FUNC(llround) (72057594037927936.75L), 72057594037927937LL, 0, 0, 0);
  check_longlong ("llround (72057594037927937.5) == 72057594037927938LL",  FUNC(llround) (72057594037927937.5L), 72057594037927938LL, 0, 0, 0);

  check_longlong ("llround (-72057594037927935.5) == -72057594037927936LL",  FUNC(llround) (-72057594037927935.5L), -72057594037927936LL, 0, 0, 0);
  check_longlong ("llround (-72057594037927936.25) == -72057594037927936LL",  FUNC(llround) (-72057594037927936.25L), -72057594037927936LL, 0, 0, 0);
  check_longlong ("llround (-72057594037927936.5) == -72057594037927937LL",  FUNC(llround) (-72057594037927936.5L), -72057594037927937LL, 0, 0, 0);
  check_longlong ("llround (-72057594037927936.75) == -72057594037927937LL",  FUNC(llround) (-72057594037927936.75L), -72057594037927937LL, 0, 0, 0);
  check_longlong ("llround (-72057594037927937.5) == -72057594037927938LL",  FUNC(llround) (-72057594037927937.5L), -72057594037927938LL, 0, 0, 0);

  check_longlong ("llround (9223372036854775806.25) == 9223372036854775806LL",  FUNC(llround) (9223372036854775806.25L), 9223372036854775806LL, 0, 0, 0);
  check_longlong ("llround (-9223372036854775806.25) == -9223372036854775806LL",  FUNC(llround) (-9223372036854775806.25L), -9223372036854775806LL, 0, 0, 0);
  check_longlong ("llround (9223372036854775806.5) == 9223372036854775807LL",  FUNC(llround) (9223372036854775806.5L), 9223372036854775807LL, 0, 0, 0);
  check_longlong ("llround (-9223372036854775806.5) == -9223372036854775807LL",  FUNC(llround) (-9223372036854775806.5L), -9223372036854775807LL, 0, 0, 0);
  check_longlong ("llround (9223372036854775807.0) == 9223372036854775807LL",  FUNC(llround) (9223372036854775807.0L), 9223372036854775807LL, 0, 0, 0);
  check_longlong ("llround (-9223372036854775807.0) == -9223372036854775807LL",  FUNC(llround) (-9223372036854775807.0L), -9223372036854775807LL, 0, 0, 0);
#endif

  print_max_error ("llround", 0, 0);
}

static void
modf_test (void)
{
  FLOAT x;

  init_max_error ();

  check_float ("modf (inf, &x) == 0",  FUNC(modf) (plus_infty, &x), 0, 0, 0, 0);
  check_float ("modf (inf, &x) sets x to plus_infty", x, plus_infty, 0, 0, 0);
  check_float ("modf (-inf, &x) == -0",  FUNC(modf) (minus_infty, &x), minus_zero, 0, 0, 0);
  check_float ("modf (-inf, &x) sets x to minus_infty", x, minus_infty, 0, 0, 0);
  check_float ("modf (NaN, &x) == NaN",  FUNC(modf) (nan_value, &x), nan_value, 0, 0, 0);
  check_float ("modf (NaN, &x) sets x to nan_value", x, nan_value, 0, 0, 0);
  check_float ("modf (0, &x) == 0",  FUNC(modf) (0, &x), 0, 0, 0, 0);
  check_float ("modf (0, &x) sets x to 0", x, 0, 0, 0, 0);
  check_float ("modf (1.5, &x) == 0.5",  FUNC(modf) (1.5, &x), 0.5, 0, 0, 0);
  check_float ("modf (1.5, &x) sets x to 1", x, 1, 0, 0, 0);
  check_float ("modf (2.5, &x) == 0.5",  FUNC(modf) (2.5, &x), 0.5, 0, 0, 0);
  check_float ("modf (2.5, &x) sets x to 2", x, 2, 0, 0, 0);
  check_float ("modf (-2.5, &x) == -0.5",  FUNC(modf) (-2.5, &x), -0.5, 0, 0, 0);
  check_float ("modf (-2.5, &x) sets x to -2", x, -2, 0, 0, 0);
  check_float ("modf (20, &x) == 0",  FUNC(modf) (20, &x), 0, 0, 0, 0);
  check_float ("modf (20, &x) sets x to 20", x, 20, 0, 0, 0);
  check_float ("modf (21, &x) == 0",  FUNC(modf) (21, &x), 0, 0, 0, 0);
  check_float ("modf (21, &x) sets x to 21", x, 21, 0, 0, 0);
  check_float ("modf (89.5, &x) == 0.5",  FUNC(modf) (89.5, &x), 0.5, 0, 0, 0);
  check_float ("modf (89.5, &x) sets x to 89", x, 89, 0, 0, 0);

  print_max_error ("modf", 0, 0);
}


static void
nearbyint_test (void)
{
  init_max_error ();

  check_float ("nearbyint (0.0) == 0.0",  FUNC(nearbyint) (0.0), 0.0, 0, 0, 0);
  check_float ("nearbyint (-0) == -0",  FUNC(nearbyint) (minus_zero), minus_zero, 0, 0, 0);
  check_float ("nearbyint (inf) == inf",  FUNC(nearbyint) (plus_infty), plus_infty, 0, 0, 0);
  check_float ("nearbyint (-inf) == -inf",  FUNC(nearbyint) (minus_infty), minus_infty, 0, 0, 0);
  check_float ("nearbyint (NaN) == NaN",  FUNC(nearbyint) (nan_value), nan_value, 0, 0, 0);

  /* Subnormal values */
  check_float ("nearbyint (-8.98847e+307) == -8.98847e+307",  FUNC(nearbyint) (-8.98847e+307), -8.98847e+307, 0, 0, 0);
  check_float ("nearbyint (-4.45015e-308) == -0",  FUNC(nearbyint) (-4.45015e-308), minus_zero, 0, 0, 0);

  /* Default rounding mode is round to nearest.  */
  check_float ("nearbyint (0.5) == 0.0",  FUNC(nearbyint) (0.5), 0.0, 0, 0, 0);
  check_float ("nearbyint (1.5) == 2.0",  FUNC(nearbyint) (1.5), 2.0, 0, 0, 0);
  check_float ("nearbyint (-0.5) == -0",  FUNC(nearbyint) (-0.5), minus_zero, 0, 0, 0);
  check_float ("nearbyint (-1.5) == -2.0",  FUNC(nearbyint) (-1.5), -2.0, 0, 0, 0);

  check_float ("nearbyint (262144.75) == 262145.0",  FUNC(nearbyint) (262144.75), 262145.0, 0, 0, 0);
  check_float ("nearbyint (262142.75) == 262143.0",  FUNC(nearbyint) (262142.75), 262143.0, 0, 0, 0);
  check_float ("nearbyint (524286.75) == 524287.0",  FUNC(nearbyint) (524286.75), 524287.0, 0, 0, 0);
  check_float ("nearbyint (524288.75) == 524289.0",  FUNC(nearbyint) (524288.75), 524289.0, 0, 0, 0);

  check_float ("nearbyint (1048576.75) == 1048577.0",  FUNC(nearbyint) (1048576.75), 1048577.0, 0, 0, 0);
  check_float ("nearbyint (2097152.75) == 2097153.0",  FUNC(nearbyint) (2097152.75), 2097153.0, 0, 0, 0);
  check_float ("nearbyint (2492472.75) == 2492473.0",  FUNC(nearbyint) (2492472.75), 2492473.0, 0, 0, 0);
  check_float ("nearbyint (2886220.75) == 2886221.0",  FUNC(nearbyint) (2886220.75), 2886221.0, 0, 0, 0);
  check_float ("nearbyint (3058792.75) == 3058793.0",  FUNC(nearbyint) (3058792.75), 3058793.0, 0, 0, 0);
  check_float ("nearbyint (-1048576.75) == -1048577.0",  FUNC(nearbyint) (-1048576.75), -1048577.0, 0, 0, 0);
  check_float ("nearbyint (-2097152.75) == -2097153.0",  FUNC(nearbyint) (-2097152.75), -2097153.0, 0, 0, 0);
  check_float ("nearbyint (-2492472.75) == -2492473.0",  FUNC(nearbyint) (-2492472.75), -2492473.0, 0, 0, 0);
  check_float ("nearbyint (-2886220.75) == -2886221.0",  FUNC(nearbyint) (-2886220.75), -2886221.0, 0, 0, 0);
  check_float ("nearbyint (-3058792.75) == -3058793.0",  FUNC(nearbyint) (-3058792.75), -3058793.0, 0, 0, 0);
#ifndef TEST_FLOAT
  check_float ("nearbyint (70368744177664.75) == 70368744177665.0",  FUNC(nearbyint) (70368744177664.75), 70368744177665.0, 0, 0, 0);
  check_float ("nearbyint (140737488355328.75) == 140737488355329.0",  FUNC(nearbyint) (140737488355328.75), 140737488355329.0, 0, 0, 0);
  check_float ("nearbyint (281474976710656.75) == 281474976710657.0",  FUNC(nearbyint) (281474976710656.75), 281474976710657.0, 0, 0, 0);
  check_float ("nearbyint (562949953421312.75) == 562949953421313.0",  FUNC(nearbyint) (562949953421312.75), 562949953421313.0, 0, 0, 0);
  check_float ("nearbyint (1125899906842624.75) == 1125899906842625.0",  FUNC(nearbyint) (1125899906842624.75), 1125899906842625.0, 0, 0, 0);
  check_float ("nearbyint (-70368744177664.75) == -70368744177665.0",  FUNC(nearbyint) (-70368744177664.75), -70368744177665.0, 0, 0, 0);
  check_float ("nearbyint (-140737488355328.75) == -140737488355329.0",  FUNC(nearbyint) (-140737488355328.75), -140737488355329.0, 0, 0, 0);
  check_float ("nearbyint (-281474976710656.75) == -281474976710657.0",  FUNC(nearbyint) (-281474976710656.75), -281474976710657.0, 0, 0, 0);
  check_float ("nearbyint (-562949953421312.75) == -562949953421313.0",  FUNC(nearbyint) (-562949953421312.75), -562949953421313.0, 0, 0, 0);
  check_float ("nearbyint (-1125899906842624.75) == -1125899906842625.0",  FUNC(nearbyint) (-1125899906842624.75), -1125899906842625.0, 0, 0, 0);
#endif

  print_max_error ("nearbyint", 0, 0);
}

static void
nextafter_test (void)
{

  init_max_error ();

  check_float ("nextafter (0, 0) == 0",  FUNC(nextafter) (0, 0), 0, 0, 0, 0);
  check_float ("nextafter (-0, 0) == 0",  FUNC(nextafter) (minus_zero, 0), 0, 0, 0, 0);
  check_float ("nextafter (0, -0) == -0",  FUNC(nextafter) (0, minus_zero), minus_zero, 0, 0, 0);
  check_float ("nextafter (-0, -0) == -0",  FUNC(nextafter) (minus_zero, minus_zero), minus_zero, 0, 0, 0);

  check_float ("nextafter (9, 9) == 9",  FUNC(nextafter) (9, 9), 9, 0, 0, 0);
  check_float ("nextafter (-9, -9) == -9",  FUNC(nextafter) (-9, -9), -9, 0, 0, 0);
  check_float ("nextafter (inf, inf) == inf",  FUNC(nextafter) (plus_infty, plus_infty), plus_infty, 0, 0, 0);
  check_float ("nextafter (-inf, -inf) == -inf",  FUNC(nextafter) (minus_infty, minus_infty), minus_infty, 0, 0, 0);

  check_float ("nextafter (NaN, 1.1) == NaN",  FUNC(nextafter) (nan_value, 1.1L), nan_value, 0, 0, 0);
  check_float ("nextafter (1.1, NaN) == NaN",  FUNC(nextafter) (1.1L, nan_value), nan_value, 0, 0, 0);
  check_float ("nextafter (NaN, NaN) == NaN",  FUNC(nextafter) (nan_value, nan_value), nan_value, 0, 0, 0);

  FLOAT fltmax = CHOOSE (LDBL_MAX, DBL_MAX, FLT_MAX,
			 LDBL_MAX, DBL_MAX, FLT_MAX);
  check_float ("nextafter (fltmax, inf) == inf",  FUNC(nextafter) (fltmax, plus_infty), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("nextafter (-fltmax, -inf) == -inf",  FUNC(nextafter) (-fltmax, minus_infty), minus_infty, 0, 0, OVERFLOW_EXCEPTION);

#ifdef TEST_LDOUBLE
  // XXX Enable once gcc is fixed.
  //TEST_ff_f (nextafter, 0x0.00000040000000000000p-16385L, -0.1L, 0x0.0000003ffffffff00000p-16385L);
#endif

  /* XXX We need the hexadecimal FP number representation here for further
     tests.  */

  print_max_error ("nextafter", 0, 0);
}


static void
nexttoward_test (void)
{
  init_max_error ();
  check_float ("nexttoward (0, 0) == 0",  FUNC(nexttoward) (0, 0), 0, 0, 0, 0);
  check_float ("nexttoward (-0, 0) == 0",  FUNC(nexttoward) (minus_zero, 0), 0, 0, 0, 0);
  check_float ("nexttoward (0, -0) == -0",  FUNC(nexttoward) (0, minus_zero), minus_zero, 0, 0, 0);
  check_float ("nexttoward (-0, -0) == -0",  FUNC(nexttoward) (minus_zero, minus_zero), minus_zero, 0, 0, 0);

  check_float ("nexttoward (9, 9) == 9",  FUNC(nexttoward) (9, 9), 9, 0, 0, 0);
  check_float ("nexttoward (-9, -9) == -9",  FUNC(nexttoward) (-9, -9), -9, 0, 0, 0);
  check_float ("nexttoward (inf, inf) == inf",  FUNC(nexttoward) (plus_infty, plus_infty), plus_infty, 0, 0, 0);
  check_float ("nexttoward (-inf, -inf) == -inf",  FUNC(nexttoward) (minus_infty, minus_infty), minus_infty, 0, 0, 0);

  check_float ("nexttoward (NaN, 1.1) == NaN",  FUNC(nexttoward) (nan_value, 1.1L), nan_value, 0, 0, 0);
  check_float ("nexttoward (1.1, NaN) == NaN",  FUNC(nexttoward) (1.1L, nan_value), nan_value, 0, 0, 0);
  check_float ("nexttoward (NaN, NaN) == NaN",  FUNC(nexttoward) (nan_value, nan_value), nan_value, 0, 0, 0);

#ifdef TEST_FLOAT
  check_float ("nexttoward (1.0, 1.1) == 0x1.000002p0",  FUNC(nexttoward) (1.0, 1.1L), 0x1.000002p0, 0, 0, 0);
  check_float ("nexttoward (1.0, LDBL_MAX) == 0x1.000002p0",  FUNC(nexttoward) (1.0, LDBL_MAX), 0x1.000002p0, 0, 0, 0);
  check_float ("nexttoward (1.0, 0x1.0000000000001p0) == 0x1.000002p0",  FUNC(nexttoward) (1.0, 0x1.0000000000001p0), 0x1.000002p0, 0, 0, 0);
  check_float ("nexttoward (1.0, 0.9) == 0x0.ffffffp0",  FUNC(nexttoward) (1.0, 0.9L), 0x0.ffffffp0, 0, 0, 0);
  check_float ("nexttoward (1.0, -LDBL_MAX) == 0x0.ffffffp0",  FUNC(nexttoward) (1.0, -LDBL_MAX), 0x0.ffffffp0, 0, 0, 0);
  check_float ("nexttoward (1.0, 0x0.fffffffffffff8p0) == 0x0.ffffffp0",  FUNC(nexttoward) (1.0, 0x0.fffffffffffff8p0), 0x0.ffffffp0, 0, 0, 0);
  check_float ("nexttoward (-1.0, -1.1) == -0x1.000002p0",  FUNC(nexttoward) (-1.0, -1.1L), -0x1.000002p0, 0, 0, 0);
  check_float ("nexttoward (-1.0, -LDBL_MAX) == -0x1.000002p0",  FUNC(nexttoward) (-1.0, -LDBL_MAX), -0x1.000002p0, 0, 0, 0);
  check_float ("nexttoward (-1.0, -0x1.0000000000001p0) == -0x1.000002p0",  FUNC(nexttoward) (-1.0, -0x1.0000000000001p0), -0x1.000002p0, 0, 0, 0);
  check_float ("nexttoward (-1.0, -0.9) == -0x0.ffffffp0",  FUNC(nexttoward) (-1.0, -0.9L), -0x0.ffffffp0, 0, 0, 0);
  check_float ("nexttoward (-1.0, LDBL_MAX) == -0x0.ffffffp0",  FUNC(nexttoward) (-1.0, LDBL_MAX), -0x0.ffffffp0, 0, 0, 0);
  check_float ("nexttoward (-1.0, -0x0.fffffffffffff8p0) == -0x0.ffffffp0",  FUNC(nexttoward) (-1.0, -0x0.fffffffffffff8p0), -0x0.ffffffp0, 0, 0, 0);
  check_float ("nexttoward (-0x1.3p-145, -0xap-148) == -0x1.4p-145",  FUNC(nexttoward) (-0x1.3p-145, -0xap-148L), -0x1.4p-145, 0, 0, UNDERFLOW_EXCEPTION);
# if LDBL_MANT_DIG >= 64
  check_float ("nexttoward (1.0, 0x1.000000000000002p0) == 0x1.000002p0",  FUNC(nexttoward) (1.0, 0x1.000000000000002p0L), 0x1.000002p0, 0, 0, 0);
  check_float ("nexttoward (1.0, 0x0.ffffffffffffffffp0) == 0x0.ffffffp0",  FUNC(nexttoward) (1.0, 0x0.ffffffffffffffffp0L), 0x0.ffffffp0, 0, 0, 0);
  check_float ("nexttoward (-1.0, -0x1.000000000000002p0) == -0x1.000002p0",  FUNC(nexttoward) (-1.0, -0x1.000000000000002p0L), -0x1.000002p0, 0, 0, 0);
  check_float ("nexttoward (-1.0, -0x0.ffffffffffffffffp0) == -0x0.ffffffp0",  FUNC(nexttoward) (-1.0, -0x0.ffffffffffffffffp0L), -0x0.ffffffp0, 0, 0, 0);
# endif
# if LDBL_MANT_DIG >= 106
  check_float ("nexttoward (1.0, 0x1.000000000000000000000000008p0) == 0x1.000002p0",  FUNC(nexttoward) (1.0, 0x1.000000000000000000000000008p0L), 0x1.000002p0, 0, 0, 0);
  check_float ("nexttoward (1.0, 0x0.ffffffffffffffffffffffffffcp0) == 0x0.ffffffp0",  FUNC(nexttoward) (1.0, 0x0.ffffffffffffffffffffffffffcp0L), 0x0.ffffffp0, 0, 0, 0);
  check_float ("nexttoward (-1.0, -0x1.000000000000000000000000008p0) == -0x1.000002p0",  FUNC(nexttoward) (-1.0, -0x1.000000000000000000000000008p0L), -0x1.000002p0, 0, 0, 0);
  check_float ("nexttoward (-1.0, -0x0.ffffffffffffffffffffffffffcp0) == -0x0.ffffffp0",  FUNC(nexttoward) (-1.0, -0x0.ffffffffffffffffffffffffffcp0L), -0x0.ffffffp0, 0, 0, 0);
# endif
# if LDBL_MANT_DIG >= 113
  check_float ("nexttoward (1.0, 0x1.0000000000000000000000000001p0) == 0x1.000002p0",  FUNC(nexttoward) (1.0, 0x1.0000000000000000000000000001p0L), 0x1.000002p0, 0, 0, 0);
  check_float ("nexttoward (1.0, 0x0.ffffffffffffffffffffffffffff8p0) == 0x0.ffffffp0",  FUNC(nexttoward) (1.0, 0x0.ffffffffffffffffffffffffffff8p0L), 0x0.ffffffp0, 0, 0, 0);
  check_float ("nexttoward (-1.0, -0x1.0000000000000000000000000001p0) == -0x1.000002p0",  FUNC(nexttoward) (-1.0, -0x1.0000000000000000000000000001p0L), -0x1.000002p0, 0, 0, 0);
  check_float ("nexttoward (-1.0, -0x0.ffffffffffffffffffffffffffff8p0) == -0x0.ffffffp0",  FUNC(nexttoward) (-1.0, -0x0.ffffffffffffffffffffffffffff8p0L), -0x0.ffffffp0, 0, 0, 0);
# endif
#endif
#ifdef TEST_DOUBLE
  check_float ("nexttoward (1.0, 1.1) == 0x1.0000000000001p0",  FUNC(nexttoward) (1.0, 1.1L), 0x1.0000000000001p0, 0, 0, 0);
  check_float ("nexttoward (1.0, LDBL_MAX) == 0x1.0000000000001p0",  FUNC(nexttoward) (1.0, LDBL_MAX), 0x1.0000000000001p0, 0, 0, 0);
  check_float ("nexttoward (1.0, 0x1.0000000000001p0) == 0x1.0000000000001p0",  FUNC(nexttoward) (1.0, 0x1.0000000000001p0), 0x1.0000000000001p0, 0, 0, 0);
  check_float ("nexttoward (1.0, 0.9) == 0x0.fffffffffffff8p0",  FUNC(nexttoward) (1.0, 0.9L), 0x0.fffffffffffff8p0, 0, 0, 0);
  check_float ("nexttoward (1.0, -LDBL_MAX) == 0x0.fffffffffffff8p0",  FUNC(nexttoward) (1.0, -LDBL_MAX), 0x0.fffffffffffff8p0, 0, 0, 0);
  check_float ("nexttoward (1.0, 0x0.fffffffffffff8p0) == 0x0.fffffffffffff8p0",  FUNC(nexttoward) (1.0, 0x0.fffffffffffff8p0), 0x0.fffffffffffff8p0, 0, 0, 0);
  check_float ("nexttoward (-1.0, -1.1) == -0x1.0000000000001p0",  FUNC(nexttoward) (-1.0, -1.1L), -0x1.0000000000001p0, 0, 0, 0);
  check_float ("nexttoward (-1.0, -LDBL_MAX) == -0x1.0000000000001p0",  FUNC(nexttoward) (-1.0, -LDBL_MAX), -0x1.0000000000001p0, 0, 0, 0);
  check_float ("nexttoward (-1.0, -0x1.0000000000001p0) == -0x1.0000000000001p0",  FUNC(nexttoward) (-1.0, -0x1.0000000000001p0), -0x1.0000000000001p0, 0, 0, 0);
  check_float ("nexttoward (-1.0, -0.9) == -0x0.fffffffffffff8p0",  FUNC(nexttoward) (-1.0, -0.9L), -0x0.fffffffffffff8p0, 0, 0, 0);
  check_float ("nexttoward (-1.0, LDBL_MAX) == -0x0.fffffffffffff8p0",  FUNC(nexttoward) (-1.0, LDBL_MAX), -0x0.fffffffffffff8p0, 0, 0, 0);
  check_float ("nexttoward (-1.0, -0x0.fffffffffffff8p0) == -0x0.fffffffffffff8p0",  FUNC(nexttoward) (-1.0, -0x0.fffffffffffff8p0), -0x0.fffffffffffff8p0, 0, 0, 0);
  check_float ("nexttoward (-1.0, -0x8.00346dc5d6388p-3) == -0x1.0000000000001p0",  FUNC(nexttoward) (-1.0, -0x8.00346dc5d6388p-3L), -0x1.0000000000001p0, 0, 0, 0);
  check_float ("nexttoward (0x1p-1074, 0x1p-1073) == 0x1p-1073",  FUNC(nexttoward) (0x1p-1074, 0x1p-1073L), 0x1p-1073, 0, 0, UNDERFLOW_EXCEPTION);
# if LDBL_MANT_DIG >= 64
  check_float ("nexttoward (1.0, 0x1.000000000000002p0) == 0x1.0000000000001p0",  FUNC(nexttoward) (1.0, 0x1.000000000000002p0L), 0x1.0000000000001p0, 0, 0, 0);
  check_float ("nexttoward (1.0, 0x0.ffffffffffffffffp0) == 0x0.fffffffffffff8p0",  FUNC(nexttoward) (1.0, 0x0.ffffffffffffffffp0L), 0x0.fffffffffffff8p0, 0, 0, 0);
  check_float ("nexttoward (-1.0, -0x1.000000000000002p0) == -0x1.0000000000001p0",  FUNC(nexttoward) (-1.0, -0x1.000000000000002p0L), -0x1.0000000000001p0, 0, 0, 0);
  check_float ("nexttoward (-1.0, -0x0.ffffffffffffffffp0) == -0x0.fffffffffffff8p0",  FUNC(nexttoward) (-1.0, -0x0.ffffffffffffffffp0L), -0x0.fffffffffffff8p0, 0, 0, 0);
# endif
# if LDBL_MANT_DIG >= 106
  check_float ("nexttoward (1.0, 0x1.000000000000000000000000008p0) == 0x1.0000000000001p0",  FUNC(nexttoward) (1.0, 0x1.000000000000000000000000008p0L), 0x1.0000000000001p0, 0, 0, 0);
  check_float ("nexttoward (1.0, 0x0.ffffffffffffffffffffffffffcp0) == 0x0.fffffffffffff8p0",  FUNC(nexttoward) (1.0, 0x0.ffffffffffffffffffffffffffcp0L), 0x0.fffffffffffff8p0, 0, 0, 0);
  check_float ("nexttoward (-1.0, -0x1.000000000000000000000000008p0) == -0x1.0000000000001p0",  FUNC(nexttoward) (-1.0, -0x1.000000000000000000000000008p0L), -0x1.0000000000001p0, 0, 0, 0);
  check_float ("nexttoward (-1.0, -0x0.ffffffffffffffffffffffffffcp0) == -0x0.fffffffffffff8p0",  FUNC(nexttoward) (-1.0, -0x0.ffffffffffffffffffffffffffcp0L), -0x0.fffffffffffff8p0, 0, 0, 0);
# endif
# if LDBL_MANT_DIG >= 113
  check_float ("nexttoward (1.0, 0x1.0000000000000000000000000001p0) == 0x1.0000000000001p0",  FUNC(nexttoward) (1.0, 0x1.0000000000000000000000000001p0L), 0x1.0000000000001p0, 0, 0, 0);
  check_float ("nexttoward (1.0, 0x0.ffffffffffffffffffffffffffff8p0) == 0x0.fffffffffffff8p0",  FUNC(nexttoward) (1.0, 0x0.ffffffffffffffffffffffffffff8p0L), 0x0.fffffffffffff8p0, 0, 0, 0);
  check_float ("nexttoward (-1.0, -0x1.0000000000000000000000000001p0) == -0x1.0000000000001p0",  FUNC(nexttoward) (-1.0, -0x1.0000000000000000000000000001p0L), -0x1.0000000000001p0, 0, 0, 0);
  check_float ("nexttoward (-1.0, -0x0.ffffffffffffffffffffffffffff8p0) == -0x0.fffffffffffff8p0",  FUNC(nexttoward) (-1.0, -0x0.ffffffffffffffffffffffffffff8p0L), -0x0.fffffffffffff8p0, 0, 0, 0);
# endif
#endif

  print_max_error ("nexttoward", 0, 0);
}


static void
pow_test (void)
{

  errno = 0;
  FUNC(pow) (0, 0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_float ("pow (0, 0) == 1",  FUNC(pow) (0, 0), 1, 0, 0, 0);
  check_float ("pow (0, -0) == 1",  FUNC(pow) (0, minus_zero), 1, 0, 0, 0);
  check_float ("pow (-0, 0) == 1",  FUNC(pow) (minus_zero, 0), 1, 0, 0, 0);
  check_float ("pow (-0, -0) == 1",  FUNC(pow) (minus_zero, minus_zero), 1, 0, 0, 0);

  check_float ("pow (10, 0) == 1",  FUNC(pow) (10, 0), 1, 0, 0, 0);
  check_float ("pow (10, -0) == 1",  FUNC(pow) (10, minus_zero), 1, 0, 0, 0);
  check_float ("pow (-10, 0) == 1",  FUNC(pow) (-10, 0), 1, 0, 0, 0);
  check_float ("pow (-10, -0) == 1",  FUNC(pow) (-10, minus_zero), 1, 0, 0, 0);

  check_float ("pow (NaN, 0) == 1",  FUNC(pow) (nan_value, 0), 1, 0, 0, 0);
  check_float ("pow (NaN, -0) == 1",  FUNC(pow) (nan_value, minus_zero), 1, 0, 0, 0);


#ifndef TEST_INLINE
  check_float ("pow (1.1, inf) == inf",  FUNC(pow) (1.1L, plus_infty), plus_infty, 0, 0, 0);
  check_float ("pow (inf, inf) == inf",  FUNC(pow) (plus_infty, plus_infty), plus_infty, 0, 0, 0);
  check_float ("pow (-1.1, inf) == inf",  FUNC(pow) (-1.1L, plus_infty), plus_infty, 0, 0, 0);
  check_float ("pow (-inf, inf) == inf",  FUNC(pow) (minus_infty, plus_infty), plus_infty, 0, 0, 0);

  check_float ("pow (0.9, inf) == 0",  FUNC(pow) (0.9L, plus_infty), 0, 0, 0, 0);
  check_float ("pow (1e-7, inf) == 0",  FUNC(pow) (1e-7L, plus_infty), 0, 0, 0, 0);
  check_float ("pow (-0.9, inf) == 0",  FUNC(pow) (-0.9L, plus_infty), 0, 0, 0, 0);
  check_float ("pow (-1e-7, inf) == 0",  FUNC(pow) (-1e-7L, plus_infty), 0, 0, 0, 0);

  check_float ("pow (1.1, -inf) == 0",  FUNC(pow) (1.1L, minus_infty), 0, 0, 0, 0);
  check_float ("pow (inf, -inf) == 0",  FUNC(pow) (plus_infty, minus_infty), 0, 0, 0, 0);
  check_float ("pow (-1.1, -inf) == 0",  FUNC(pow) (-1.1L, minus_infty), 0, 0, 0, 0);
  check_float ("pow (-inf, -inf) == 0",  FUNC(pow) (minus_infty, minus_infty), 0, 0, 0, 0);

  check_float ("pow (0.9, -inf) == inf",  FUNC(pow) (0.9L, minus_infty), plus_infty, 0, 0, 0);
  check_float ("pow (1e-7, -inf) == inf",  FUNC(pow) (1e-7L, minus_infty), plus_infty, 0, 0, 0);
  check_float ("pow (-0.9, -inf) == inf",  FUNC(pow) (-0.9L, minus_infty), plus_infty, 0, 0, 0);
  check_float ("pow (-1e-7, -inf) == inf",  FUNC(pow) (-1e-7L, minus_infty), plus_infty, 0, 0, 0);

  check_float ("pow (inf, 1e-7) == inf",  FUNC(pow) (plus_infty, 1e-7L), plus_infty, 0, 0, 0);
  check_float ("pow (inf, 1) == inf",  FUNC(pow) (plus_infty, 1), plus_infty, 0, 0, 0);
  check_float ("pow (inf, 1e7) == inf",  FUNC(pow) (plus_infty, 1e7L), plus_infty, 0, 0, 0);
  check_float ("pow (inf, min_subnorm_value) == inf",  FUNC(pow) (plus_infty, min_subnorm_value), plus_infty, 0, 0, 0);

  check_float ("pow (inf, -1e-7) == 0",  FUNC(pow) (plus_infty, -1e-7L), 0, 0, 0, 0);
  check_float ("pow (inf, -1) == 0",  FUNC(pow) (plus_infty, -1), 0, 0, 0, 0);
  check_float ("pow (inf, -1e7) == 0",  FUNC(pow) (plus_infty, -1e7L), 0, 0, 0, 0);
  check_float ("pow (inf, -min_subnorm_value) == 0",  FUNC(pow) (plus_infty, -min_subnorm_value), 0, 0, 0, 0);

  check_float ("pow (-inf, 1) == -inf",  FUNC(pow) (minus_infty, 1), minus_infty, 0, 0, 0);
  check_float ("pow (-inf, 11) == -inf",  FUNC(pow) (minus_infty, 11), minus_infty, 0, 0, 0);
  check_float ("pow (-inf, 1001) == -inf",  FUNC(pow) (minus_infty, 1001), minus_infty, 0, 0, 0);

  check_float ("pow (-inf, 2) == inf",  FUNC(pow) (minus_infty, 2), plus_infty, 0, 0, 0);
  check_float ("pow (-inf, 12) == inf",  FUNC(pow) (minus_infty, 12), plus_infty, 0, 0, 0);
  check_float ("pow (-inf, 1002) == inf",  FUNC(pow) (minus_infty, 1002), plus_infty, 0, 0, 0);
  check_float ("pow (-inf, 0.1) == inf",  FUNC(pow) (minus_infty, 0.1L), plus_infty, 0, 0, 0);
  check_float ("pow (-inf, 1.1) == inf",  FUNC(pow) (minus_infty, 1.1L), plus_infty, 0, 0, 0);
  check_float ("pow (-inf, 11.1) == inf",  FUNC(pow) (minus_infty, 11.1L), plus_infty, 0, 0, 0);
  check_float ("pow (-inf, 1001.1) == inf",  FUNC(pow) (minus_infty, 1001.1L), plus_infty, 0, 0, 0);
  check_float ("pow (-inf, min_subnorm_value) == inf",  FUNC(pow) (minus_infty, min_subnorm_value), plus_infty, 0, 0, 0);

  check_float ("pow (-inf, -1) == -0",  FUNC(pow) (minus_infty, -1), minus_zero, 0, 0, 0);
  check_float ("pow (-inf, -11) == -0",  FUNC(pow) (minus_infty, -11), minus_zero, 0, 0, 0);
  check_float ("pow (-inf, -1001) == -0",  FUNC(pow) (minus_infty, -1001), minus_zero, 0, 0, 0);

  check_float ("pow (-inf, -2) == 0",  FUNC(pow) (minus_infty, -2), 0, 0, 0, 0);
  check_float ("pow (-inf, -12) == 0",  FUNC(pow) (minus_infty, -12), 0, 0, 0, 0);
  check_float ("pow (-inf, -1002) == 0",  FUNC(pow) (minus_infty, -1002), 0, 0, 0, 0);
  check_float ("pow (-inf, -0.1) == 0",  FUNC(pow) (minus_infty, -0.1L), 0, 0, 0, 0);
  check_float ("pow (-inf, -1.1) == 0",  FUNC(pow) (minus_infty, -1.1L), 0, 0, 0, 0);
  check_float ("pow (-inf, -11.1) == 0",  FUNC(pow) (minus_infty, -11.1L), 0, 0, 0, 0);
  check_float ("pow (-inf, -1001.1) == 0",  FUNC(pow) (minus_infty, -1001.1L), 0, 0, 0, 0);
  check_float ("pow (-inf, -min_subnorm_value) == 0",  FUNC(pow) (minus_infty, -min_subnorm_value), 0, 0, 0, 0);
#endif

  check_float ("pow (NaN, NaN) == NaN",  FUNC(pow) (nan_value, nan_value), nan_value, 0, 0, 0);
  check_float ("pow (0, NaN) == NaN",  FUNC(pow) (0, nan_value), nan_value, 0, 0, 0);
  check_float ("pow (1, NaN) == 1",  FUNC(pow) (1, nan_value), 1, 0, 0, 0);
  check_float ("pow (-1, NaN) == NaN",  FUNC(pow) (-1, nan_value), nan_value, 0, 0, 0);
  check_float ("pow (NaN, 1) == NaN",  FUNC(pow) (nan_value, 1), nan_value, 0, 0, 0);
  check_float ("pow (NaN, -1) == NaN",  FUNC(pow) (nan_value, -1), nan_value, 0, 0, 0);

  /* pow (x, NaN) == NaN.  */
  check_float ("pow (3.0, NaN) == NaN",  FUNC(pow) (3.0, nan_value), nan_value, 0, 0, 0);
  check_float ("pow (-0, NaN) == NaN",  FUNC(pow) (minus_zero, nan_value), nan_value, 0, 0, 0);
  check_float ("pow (inf, NaN) == NaN",  FUNC(pow) (plus_infty, nan_value), nan_value, 0, 0, 0);
  check_float ("pow (-3.0, NaN) == NaN",  FUNC(pow) (-3.0, nan_value), nan_value, 0, 0, 0);
  check_float ("pow (-inf, NaN) == NaN",  FUNC(pow) (minus_infty, nan_value), nan_value, 0, 0, 0);

  check_float ("pow (NaN, 3.0) == NaN",  FUNC(pow) (nan_value, 3.0), nan_value, 0, 0, 0);
  check_float ("pow (NaN, -3.0) == NaN",  FUNC(pow) (nan_value, -3.0), nan_value, 0, 0, 0);
  check_float ("pow (NaN, inf) == NaN",  FUNC(pow) (nan_value, plus_infty), nan_value, 0, 0, 0);
  check_float ("pow (NaN, -inf) == NaN",  FUNC(pow) (nan_value, minus_infty), nan_value, 0, 0, 0);
  check_float ("pow (NaN, 2.5) == NaN",  FUNC(pow) (nan_value, 2.5), nan_value, 0, 0, 0);
  check_float ("pow (NaN, -2.5) == NaN",  FUNC(pow) (nan_value, -2.5), nan_value, 0, 0, 0);
  check_float ("pow (NaN, min_subnorm_value) == NaN",  FUNC(pow) (nan_value, min_subnorm_value), nan_value, 0, 0, 0);
  check_float ("pow (NaN, -min_subnorm_value) == NaN",  FUNC(pow) (nan_value, -min_subnorm_value), nan_value, 0, 0, 0);

  check_float ("pow (1, inf) == 1",  FUNC(pow) (1, plus_infty), 1, 0, 0, 0);
  check_float ("pow (-1, inf) == 1",  FUNC(pow) (-1, plus_infty), 1, 0, 0, 0);
  check_float ("pow (1, -inf) == 1",  FUNC(pow) (1, minus_infty), 1, 0, 0, 0);
  check_float ("pow (-1, -inf) == 1",  FUNC(pow) (-1, minus_infty), 1, 0, 0, 0);
  check_float ("pow (1, 1) == 1",  FUNC(pow) (1, 1), 1, 0, 0, 0);
  check_float ("pow (1, -1) == 1",  FUNC(pow) (1, -1), 1, 0, 0, 0);
  check_float ("pow (1, 1.25) == 1",  FUNC(pow) (1, 1.25), 1, 0, 0, 0);
  check_float ("pow (1, -1.25) == 1",  FUNC(pow) (1, -1.25), 1, 0, 0, 0);
  check_float ("pow (1, 0x1p62) == 1",  FUNC(pow) (1, 0x1p62L), 1, 0, 0, 0);
  check_float ("pow (1, 0x1p63) == 1",  FUNC(pow) (1, 0x1p63L), 1, 0, 0, 0);
  check_float ("pow (1, 0x1p64) == 1",  FUNC(pow) (1, 0x1p64L), 1, 0, 0, 0);
  check_float ("pow (1, 0x1p72) == 1",  FUNC(pow) (1, 0x1p72L), 1, 0, 0, 0);
  check_float ("pow (1, min_subnorm_value) == 1",  FUNC(pow) (1, min_subnorm_value), 1, 0, 0, 0);
  check_float ("pow (1, -min_subnorm_value) == 1",  FUNC(pow) (1, -min_subnorm_value), 1, 0, 0, 0);

  /* pow (x, +-0) == 1.  */
  check_float ("pow (inf, 0) == 1",  FUNC(pow) (plus_infty, 0), 1, 0, 0, 0);
  check_float ("pow (inf, -0) == 1",  FUNC(pow) (plus_infty, minus_zero), 1, 0, 0, 0);
  check_float ("pow (-inf, 0) == 1",  FUNC(pow) (minus_infty, 0), 1, 0, 0, 0);
  check_float ("pow (-inf, -0) == 1",  FUNC(pow) (minus_infty, minus_zero), 1, 0, 0, 0);
  check_float ("pow (32.75, 0) == 1",  FUNC(pow) (32.75L, 0), 1, 0, 0, 0);
  check_float ("pow (32.75, -0) == 1",  FUNC(pow) (32.75L, minus_zero), 1, 0, 0, 0);
  check_float ("pow (-32.75, 0) == 1",  FUNC(pow) (-32.75L, 0), 1, 0, 0, 0);
  check_float ("pow (-32.75, -0) == 1",  FUNC(pow) (-32.75L, minus_zero), 1, 0, 0, 0);
  check_float ("pow (0x1p72, 0) == 1",  FUNC(pow) (0x1p72L, 0), 1, 0, 0, 0);
  check_float ("pow (0x1p72, -0) == 1",  FUNC(pow) (0x1p72L, minus_zero), 1, 0, 0, 0);
  check_float ("pow (0x1p-72, 0) == 1",  FUNC(pow) (0x1p-72L, 0), 1, 0, 0, 0);
  check_float ("pow (0x1p-72, -0) == 1",  FUNC(pow) (0x1p-72L, minus_zero), 1, 0, 0, 0);

  check_float ("pow (-0.1, 1.1) == NaN",  FUNC(pow) (-0.1L, 1.1L), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("pow (-0.1, -1.1) == NaN",  FUNC(pow) (-0.1L, -1.1L), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("pow (-10.1, 1.1) == NaN",  FUNC(pow) (-10.1L, 1.1L), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("pow (-10.1, -1.1) == NaN",  FUNC(pow) (-10.1L, -1.1L), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("pow (-1.01, min_subnorm_value) == NaN",  FUNC(pow) (-1.01L, min_subnorm_value), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("pow (-1.01, -min_subnorm_value) == NaN",  FUNC(pow) (-1.01L, -min_subnorm_value), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("pow (-1.0, min_subnorm_value) == NaN",  FUNC(pow) (-1.0L, min_subnorm_value), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("pow (-1.0, -min_subnorm_value) == NaN",  FUNC(pow) (-1.0L, -min_subnorm_value), nan_value, 0, 0, INVALID_EXCEPTION);

  errno = 0;
  check_float ("pow (0, -1) == inf",  FUNC(pow) (0, -1), plus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_int ("errno for pow(0,-odd) == ERANGE", errno, ERANGE, 0, 0, 0);
  errno = 0;
  check_float ("pow (0, -11) == inf",  FUNC(pow) (0, -11), plus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_int ("errno for pow(0,-odd) == ERANGE", errno, ERANGE, 0, 0, 0);
  errno = 0;
  check_float ("pow (0, -0xffffff) == inf",  FUNC(pow) (0, -0xffffff), plus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_int ("errno for pow(0,-odd) == ERANGE", errno, ERANGE, 0, 0, 0);
#ifndef TEST_FLOAT
  errno = 0;
  check_float ("pow (0, -0x1.fffffffffffffp+52) == inf",  FUNC(pow) (0, -0x1.fffffffffffffp+52L), plus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_int ("errno for pow(0,-odd) == ERANGE", errno, ERANGE, 0, 0, 0);
#endif
#ifdef TEST_LDOUBLE
# if LDBL_MANT_DIG >= 64
  errno = 0;
  check_float ("pow (0, -0x1.fffffffffffffffep+63) == inf",  FUNC(pow) (0, -0x1.fffffffffffffffep+63L), plus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_int ("errno for pow(0,-odd) == ERANGE", errno, ERANGE, 0, 0, 0);
# endif
# if LDBL_MANT_DIG >= 106
  errno = 0;
  check_float ("pow (0, -0x1.ffffffffffffffffffffffffff8p+105) == inf",  FUNC(pow) (0, -0x1.ffffffffffffffffffffffffff8p+105L), plus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_int ("errno for pow(0,-odd) == ERANGE", errno, ERANGE, 0, 0, 0);
# endif
# if LDBL_MANT_DIG >= 113
  errno = 0;
  check_float ("pow (0, -0x1.ffffffffffffffffffffffffffffp+112) == inf",  FUNC(pow) (0, -0x1.ffffffffffffffffffffffffffffp+112L), plus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_int ("errno for pow(0,-odd) == ERANGE", errno, ERANGE, 0, 0, 0);
# endif
#endif
  check_float ("pow (-0, -1) == -inf",  FUNC(pow) (minus_zero, -1), minus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_int ("errno for pow(-0,-odd) == ERANGE", errno, ERANGE, 0, 0, 0);
  errno = 0;
  check_float ("pow (-0, -11) == -inf",  FUNC(pow) (minus_zero, -11L), minus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_int ("errno for pow(-0,-odd) == ERANGE", errno, ERANGE, 0, 0, 0);
  errno = 0;
  check_float ("pow (-0, -0xffffff) == -inf",  FUNC(pow) (minus_zero, -0xffffff), minus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_int ("errno for pow(-0,-odd) == ERANGE", errno, ERANGE, 0, 0, 0);
  errno = 0;
  check_float ("pow (-0, -0x1fffffe) == inf",  FUNC(pow) (minus_zero, -0x1fffffe), plus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_int ("errno for pow(-0,-even) == ERANGE", errno, ERANGE, 0, 0, 0);
#ifndef TEST_FLOAT
  errno = 0;
  check_float ("pow (-0, -0x1.fffffffffffffp+52) == -inf",  FUNC(pow) (minus_zero, -0x1.fffffffffffffp+52L), minus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_int ("errno for pow(-0,-odd) == ERANGE", errno, ERANGE, 0, 0, 0);
  errno = 0;
  check_float ("pow (-0, -0x1.fffffffffffffp+53) == inf",  FUNC(pow) (minus_zero, -0x1.fffffffffffffp+53L), plus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_int ("errno for pow(-0,-even) == ERANGE", errno, ERANGE, 0, 0, 0);
#endif
#ifdef TEST_LDOUBLE
# if LDBL_MANT_DIG >= 64
  errno = 0;
  check_float ("pow (-0, -0x1.fffffffffffffffep+63) == -inf",  FUNC(pow) (minus_zero, -0x1.fffffffffffffffep+63L), minus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_int ("errno for pow(-0,-odd) == ERANGE", errno, ERANGE, 0, 0, 0);
  errno = 0;
  check_float ("pow (-0, -0x1.fffffffffffffffep+64) == inf",  FUNC(pow) (minus_zero, -0x1.fffffffffffffffep+64L), plus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_int ("errno for pow(-0,-even) == ERANGE", errno, ERANGE, 0, 0, 0);
# endif
# if LDBL_MANT_DIG >= 106
  errno = 0;
  check_float ("pow (-0, -0x1.ffffffffffffffffffffffffff8p+105) == -inf",  FUNC(pow) (minus_zero, -0x1.ffffffffffffffffffffffffff8p+105L), minus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_int ("errno for pow(-0,-odd) == ERANGE", errno, ERANGE, 0, 0, 0);
  errno = 0;
  check_float ("pow (-0, -0x1.ffffffffffffffffffffffffff8p+106) == inf",  FUNC(pow) (minus_zero, -0x1.ffffffffffffffffffffffffff8p+106L), plus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_int ("errno for pow(-0,-even) == ERANGE", errno, ERANGE, 0, 0, 0);
# endif
# if LDBL_MANT_DIG >= 113
  errno = 0;
  check_float ("pow (-0, -0x1.ffffffffffffffffffffffffffffp+112) == -inf",  FUNC(pow) (minus_zero, -0x1.ffffffffffffffffffffffffffffp+112L), minus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_int ("errno for pow(-0,-odd) == ERANGE", errno, ERANGE, 0, 0, 0);
  errno = 0;
  check_float ("pow (-0, -0x1.ffffffffffffffffffffffffffffp+113) == inf",  FUNC(pow) (minus_zero, -0x1.ffffffffffffffffffffffffffffp+113L), plus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_int ("errno for pow(-0,-even) == ERANGE", errno, ERANGE, 0, 0, 0);
# endif
#endif

  errno = 0;
  check_float ("pow (0, -2) == inf",  FUNC(pow) (0, -2), plus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_int ("errno for pow(0,-even) == ERANGE", errno, ERANGE, 0, 0, 0);
  errno = 0;
  check_float ("pow (0, -11.1) == inf",  FUNC(pow) (0, -11.1L), plus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_int ("errno for pow(0,-num) == ERANGE", errno, ERANGE, 0, 0, 0);
  errno = 0;
  check_float ("pow (0, -min_subnorm_value) == inf",  FUNC(pow) (0, -min_subnorm_value), plus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_int ("errno for pow(0,-num) == ERANGE", errno, ERANGE, 0, 0, 0);
  errno = 0;
  check_float ("pow (0, -0x1p24) == inf",  FUNC(pow) (0, -0x1p24), plus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_int ("errno for pow(0,-num) == ERANGE", errno, ERANGE, 0, 0, 0);
  errno = 0;
  check_float ("pow (0, -0x1p127) == inf",  FUNC(pow) (0, -0x1p127), plus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_int ("errno for pow(0,-num) == ERANGE", errno, ERANGE, 0, 0, 0);
  errno = 0;
  check_float ("pow (0, -max_value) == inf",  FUNC(pow) (0, -max_value), plus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_int ("errno for pow(0,-num) == ERANGE", errno, ERANGE, 0, 0, 0);
  errno = 0;
  check_float ("pow (-0, -2) == inf",  FUNC(pow) (minus_zero, -2), plus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_int ("errno for pow(-0,-even) == ERANGE", errno, ERANGE, 0, 0, 0);
  errno = 0;
  check_float ("pow (-0, -11.1) == inf",  FUNC(pow) (minus_zero, -11.1L), plus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_int ("errno for pow(-0,-num) == ERANGE", errno, ERANGE, 0, 0, 0);
  errno = 0;
  check_float ("pow (-0, -min_subnorm_value) == inf",  FUNC(pow) (minus_zero, -min_subnorm_value), plus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_int ("errno for pow(-0,-num) == ERANGE", errno, ERANGE, 0, 0, 0);
  errno = 0;
  check_float ("pow (-0, -0x1p24) == inf",  FUNC(pow) (minus_zero, -0x1p24), plus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_int ("errno for pow(-0,-num) == ERANGE", errno, ERANGE, 0, 0, 0);
  errno = 0;
  check_float ("pow (-0, -0x1p127) == inf",  FUNC(pow) (minus_zero, -0x1p127), plus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_int ("errno for pow(-0,-num) == ERANGE", errno, ERANGE, 0, 0, 0);
  errno = 0;
  check_float ("pow (-0, -max_value) == inf",  FUNC(pow) (minus_zero, -max_value), plus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_int ("errno for pow(-0,-num) == ERANGE", errno, ERANGE, 0, 0, 0);

  check_float ("pow (0x1p72, 0x1p72) == inf",  FUNC(pow) (0x1p72L, 0x1p72L), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("pow (10, -0x1p72) == 0",  FUNC(pow) (10, -0x1p72L), 0, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("pow (max_value, max_value) == inf",  FUNC(pow) (max_value, max_value), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("pow (10, -max_value) == 0",  FUNC(pow) (10, -max_value), 0, 0, 0, UNDERFLOW_EXCEPTION);

  check_float ("pow (0, 1) == 0",  FUNC(pow) (0, 1), 0, 0, 0, 0);
  check_float ("pow (0, 11) == 0",  FUNC(pow) (0, 11), 0, 0, 0, 0);

  check_float ("pow (-0, 1) == -0",  FUNC(pow) (minus_zero, 1), minus_zero, 0, 0, 0);
  check_float ("pow (-0, 11) == -0",  FUNC(pow) (minus_zero, 11), minus_zero, 0, 0, 0);

  check_float ("pow (0, 2) == 0",  FUNC(pow) (0, 2), 0, 0, 0, 0);
  check_float ("pow (0, 11.1) == 0",  FUNC(pow) (0, 11.1L), 0, 0, 0, 0);

  check_float ("pow (-0, 2) == 0",  FUNC(pow) (minus_zero, 2), 0, 0, 0, 0);
  check_float ("pow (-0, 11.1) == 0",  FUNC(pow) (minus_zero, 11.1L), 0, 0, 0, 0);
  check_float ("pow (0, inf) == 0",  FUNC(pow) (0, plus_infty), 0, 0, 0, 0);
  check_float ("pow (-0, inf) == 0",  FUNC(pow) (minus_zero, plus_infty), 0, 0, 0, 0);
  check_float ("pow (0, -inf) == inf",  FUNC(pow) (0, minus_infty), plus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION_OK);
  check_float ("pow (-0, -inf) == inf",  FUNC(pow) (minus_zero, minus_infty), plus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION_OK);

#ifndef TEST_INLINE
  /* pow (x, +inf) == +inf for |x| > 1.  */
  check_float ("pow (1.5, inf) == inf",  FUNC(pow) (1.5, plus_infty), plus_infty, 0, 0, 0);

  /* pow (x, +inf) == +0 for |x| < 1.  */
  check_float ("pow (0.5, inf) == 0.0",  FUNC(pow) (0.5, plus_infty), 0.0, 0, 0, 0);

  /* pow (x, -inf) == +0 for |x| > 1.  */
  check_float ("pow (1.5, -inf) == 0.0",  FUNC(pow) (1.5, minus_infty), 0.0, 0, 0, 0);

  /* pow (x, -inf) == +inf for |x| < 1.  */
  check_float ("pow (0.5, -inf) == inf",  FUNC(pow) (0.5, minus_infty), plus_infty, 0, 0, 0);
#endif

  /* pow (+inf, y) == +inf for y > 0.  */
  check_float ("pow (inf, 2) == inf",  FUNC(pow) (plus_infty, 2), plus_infty, 0, 0, 0);
  check_float ("pow (inf, 0xffffff) == inf",  FUNC(pow) (plus_infty, 0xffffff), plus_infty, 0, 0, 0);
#ifndef TEST_FLOAT
  check_float ("pow (inf, 0x1.fffffffffffffp+52) == inf",  FUNC(pow) (plus_infty, 0x1.fffffffffffffp+52L), plus_infty, 0, 0, 0);
#endif
#ifdef TEST_LDOUBLE
# if LDBL_MANT_DIG >= 64
  check_float ("pow (inf, 0x1.fffffffffffffffep+63) == inf",  FUNC(pow) (plus_infty, 0x1.fffffffffffffffep+63L), plus_infty, 0, 0, 0);
# endif
# if LDBL_MANT_DIG >= 106
  check_float ("pow (inf, 0x1.ffffffffffffffffffffffffff8p+105) == inf",  FUNC(pow) (plus_infty, 0x1.ffffffffffffffffffffffffff8p+105L), plus_infty, 0, 0, 0);
# endif
# if LDBL_MANT_DIG >= 113
  check_float ("pow (inf, 0x1.ffffffffffffffffffffffffffffp+112) == inf",  FUNC(pow) (plus_infty, 0x1.ffffffffffffffffffffffffffffp+112L), plus_infty, 0, 0, 0);
# endif
#endif
  check_float ("pow (inf, 0x1p24) == inf",  FUNC(pow) (plus_infty, 0x1p24), plus_infty, 0, 0, 0);
  check_float ("pow (inf, 0x1p127) == inf",  FUNC(pow) (plus_infty, 0x1p127), plus_infty, 0, 0, 0);
  check_float ("pow (inf, max_value) == inf",  FUNC(pow) (plus_infty, max_value), plus_infty, 0, 0, 0);

  /* pow (+inf, y) == +0 for y < 0.  */
  check_float ("pow (inf, -1) == 0.0",  FUNC(pow) (plus_infty, -1), 0.0, 0, 0, 0);
  check_float ("pow (inf, -0xffffff) == 0.0",  FUNC(pow) (plus_infty, -0xffffff), 0.0, 0, 0, 0);
#ifndef TEST_FLOAT
  check_float ("pow (inf, -0x1.fffffffffffffp+52) == 0.0",  FUNC(pow) (plus_infty, -0x1.fffffffffffffp+52L), 0.0, 0, 0, 0);
#endif
#ifdef TEST_LDOUBLE
# if LDBL_MANT_DIG >= 64
  check_float ("pow (inf, -0x1.fffffffffffffffep+63) == 0.0",  FUNC(pow) (plus_infty, -0x1.fffffffffffffffep+63L), 0.0, 0, 0, 0);
# endif
# if LDBL_MANT_DIG >= 106
  check_float ("pow (inf, -0x1.ffffffffffffffffffffffffff8p+105) == 0.0",  FUNC(pow) (plus_infty, -0x1.ffffffffffffffffffffffffff8p+105L), 0.0, 0, 0, 0);
# endif
# if LDBL_MANT_DIG >= 113
  check_float ("pow (inf, -0x1.ffffffffffffffffffffffffffffp+112) == 0.0",  FUNC(pow) (plus_infty, -0x1.ffffffffffffffffffffffffffffp+112L), 0.0, 0, 0, 0);
# endif
#endif
  check_float ("pow (inf, -0x1p24) == 0.0",  FUNC(pow) (plus_infty, -0x1p24), 0.0, 0, 0, 0);
  check_float ("pow (inf, -0x1p127) == 0.0",  FUNC(pow) (plus_infty, -0x1p127), 0.0, 0, 0, 0);
  check_float ("pow (inf, -max_value) == 0.0",  FUNC(pow) (plus_infty, -max_value), 0.0, 0, 0, 0);

  /* pow (-inf, y) == -inf for y an odd integer > 0.  */
  check_float ("pow (-inf, 27) == -inf",  FUNC(pow) (minus_infty, 27), minus_infty, 0, 0, 0);
  check_float ("pow (-inf, 0xffffff) == -inf",  FUNC(pow) (minus_infty, 0xffffff), minus_infty, 0, 0, 0);
  check_float ("pow (-inf, 0x1fffffe) == inf",  FUNC(pow) (minus_infty, 0x1fffffe), plus_infty, 0, 0, 0);
#ifndef TEST_FLOAT
  check_float ("pow (-inf, 0x1.fffffffffffffp+52) == -inf",  FUNC(pow) (minus_infty, 0x1.fffffffffffffp+52L), minus_infty, 0, 0, 0);
  check_float ("pow (-inf, 0x1.fffffffffffffp+53) == inf",  FUNC(pow) (minus_infty, 0x1.fffffffffffffp+53L), plus_infty, 0, 0, 0);
#endif
#ifdef TEST_LDOUBLE
# if LDBL_MANT_DIG >= 64
  check_float ("pow (-inf, 0x1.fffffffffffffffep+63) == -inf",  FUNC(pow) (minus_infty, 0x1.fffffffffffffffep+63L), minus_infty, 0, 0, 0);
  check_float ("pow (-inf, 0x1.fffffffffffffffep+64) == inf",  FUNC(pow) (minus_infty, 0x1.fffffffffffffffep+64L), plus_infty, 0, 0, 0);
# endif
# if LDBL_MANT_DIG >= 106
  check_float ("pow (-inf, 0x1.ffffffffffffffffffffffffff8p+105) == -inf",  FUNC(pow) (minus_infty, 0x1.ffffffffffffffffffffffffff8p+105L), minus_infty, 0, 0, 0);
  check_float ("pow (-inf, 0x1.ffffffffffffffffffffffffff8p+106) == inf",  FUNC(pow) (minus_infty, 0x1.ffffffffffffffffffffffffff8p+106L), plus_infty, 0, 0, 0);
# endif
# if LDBL_MANT_DIG >= 113
  check_float ("pow (-inf, 0x1.ffffffffffffffffffffffffffffp+112) == -inf",  FUNC(pow) (minus_infty, 0x1.ffffffffffffffffffffffffffffp+112L), minus_infty, 0, 0, 0);
  check_float ("pow (-inf, 0x1.ffffffffffffffffffffffffffffp+113) == inf",  FUNC(pow) (minus_infty, 0x1.ffffffffffffffffffffffffffffp+113L), plus_infty, 0, 0, 0);
# endif
#endif

  /* pow (-inf, y) == +inf for y > 0 and not an odd integer.  */
  check_float ("pow (-inf, 28) == inf",  FUNC(pow) (minus_infty, 28), plus_infty, 0, 0, 0);
  check_float ("pow (-inf, 0x1p24) == inf",  FUNC(pow) (minus_infty, 0x1p24), plus_infty, 0, 0, 0);
  check_float ("pow (-inf, 0x1p127) == inf",  FUNC(pow) (minus_infty, 0x1p127), plus_infty, 0, 0, 0);
  check_float ("pow (-inf, max_value) == inf",  FUNC(pow) (minus_infty, max_value), plus_infty, 0, 0, 0);

  /* pow (-inf, y) == -0 for y an odd integer < 0. */
  check_float ("pow (-inf, -3) == -0",  FUNC(pow) (minus_infty, -3), minus_zero, 0, 0, 0);
  check_float ("pow (-inf, -0xffffff) == -0",  FUNC(pow) (minus_infty, -0xffffff), minus_zero, 0, 0, 0);
  check_float ("pow (-inf, -0x1fffffe) == +0",  FUNC(pow) (minus_infty, -0x1fffffe), plus_zero, 0, 0, 0);
#ifndef TEST_FLOAT
  check_float ("pow (-inf, -0x1.fffffffffffffp+52) == -0",  FUNC(pow) (minus_infty, -0x1.fffffffffffffp+52L), minus_zero, 0, 0, 0);
  check_float ("pow (-inf, -0x1.fffffffffffffp+53) == +0",  FUNC(pow) (minus_infty, -0x1.fffffffffffffp+53L), plus_zero, 0, 0, 0);
#endif
#ifdef TEST_LDOUBLE
# if LDBL_MANT_DIG >= 64
  check_float ("pow (-inf, -0x1.fffffffffffffffep+63) == -0",  FUNC(pow) (minus_infty, -0x1.fffffffffffffffep+63L), minus_zero, 0, 0, 0);
  check_float ("pow (-inf, -0x1.fffffffffffffffep+64) == +0",  FUNC(pow) (minus_infty, -0x1.fffffffffffffffep+64L), plus_zero, 0, 0, 0);
# endif
# if LDBL_MANT_DIG >= 106
  check_float ("pow (-inf, -0x1.ffffffffffffffffffffffffff8p+105) == -0",  FUNC(pow) (minus_infty, -0x1.ffffffffffffffffffffffffff8p+105L), minus_zero, 0, 0, 0);
  check_float ("pow (-inf, -0x1.ffffffffffffffffffffffffff8p+106) == +0",  FUNC(pow) (minus_infty, -0x1.ffffffffffffffffffffffffff8p+106L), plus_zero, 0, 0, 0);
# endif
# if LDBL_MANT_DIG >= 113
  check_float ("pow (-inf, -0x1.ffffffffffffffffffffffffffffp+112) == -0",  FUNC(pow) (minus_infty, -0x1.ffffffffffffffffffffffffffffp+112L), minus_zero, 0, 0, 0);
  check_float ("pow (-inf, -0x1.ffffffffffffffffffffffffffffp+113) == +0",  FUNC(pow) (minus_infty, -0x1.ffffffffffffffffffffffffffffp+113L), plus_zero, 0, 0, 0);
# endif
#endif
  /* pow (-inf, y) == +0 for y < 0 and not an odd integer.  */
  check_float ("pow (-inf, -2.0) == 0.0",  FUNC(pow) (minus_infty, -2.0), 0.0, 0, 0, 0);
  check_float ("pow (-inf, -0x1p24) == 0.0",  FUNC(pow) (minus_infty, -0x1p24), 0.0, 0, 0, 0);
  check_float ("pow (-inf, -0x1p127) == 0.0",  FUNC(pow) (minus_infty, -0x1p127), 0.0, 0, 0, 0);
  check_float ("pow (-inf, -max_value) == 0.0",  FUNC(pow) (minus_infty, -max_value), 0.0, 0, 0, 0);

  /* pow (+0, y) == +0 for y an odd integer > 0.  */
  check_float ("pow (0.0, 27) == 0.0",  FUNC(pow) (0.0, 27), 0.0, 0, 0, 0);
  check_float ("pow (0.0, 0xffffff) == 0.0",  FUNC(pow) (0.0, 0xffffff), 0.0, 0, 0, 0);
#ifndef TEST_FLOAT
  check_float ("pow (0.0, 0x1.fffffffffffffp+52) == 0.0",  FUNC(pow) (0.0, 0x1.fffffffffffffp+52L), 0.0, 0, 0, 0);
#endif
#ifdef TEST_LDOUBLE
# if LDBL_MANT_DIG >= 64
  check_float ("pow (0.0, 0x1.fffffffffffffffep+63) == 0.0",  FUNC(pow) (0.0, 0x1.fffffffffffffffep+63L), 0.0, 0, 0, 0);
# endif
# if LDBL_MANT_DIG >= 106
  check_float ("pow (0.0, 0x1.ffffffffffffffffffffffffff8p+105) == 0.0",  FUNC(pow) (0.0, 0x1.ffffffffffffffffffffffffff8p+105L), 0.0, 0, 0, 0);
# endif
# if LDBL_MANT_DIG >= 113
  check_float ("pow (0.0, 0x1.ffffffffffffffffffffffffffffp+112) == 0.0",  FUNC(pow) (0.0, 0x1.ffffffffffffffffffffffffffffp+112L), 0.0, 0, 0, 0);
# endif
#endif

  /* pow (-0, y) == -0 for y an odd integer > 0.  */
  check_float ("pow (-0, 27) == -0",  FUNC(pow) (minus_zero, 27), minus_zero, 0, 0, 0);
  check_float ("pow (-0, 0xffffff) == -0",  FUNC(pow) (minus_zero, 0xffffff), minus_zero, 0, 0, 0);
  check_float ("pow (-0, 0x1fffffe) == +0",  FUNC(pow) (minus_zero, 0x1fffffe), plus_zero, 0, 0, 0);
#ifndef TEST_FLOAT
  check_float ("pow (-0, 0x1.fffffffffffffp+52) == -0",  FUNC(pow) (minus_zero, 0x1.fffffffffffffp+52L), minus_zero, 0, 0, 0);
  check_float ("pow (-0, 0x1.fffffffffffffp+53) == +0",  FUNC(pow) (minus_zero, 0x1.fffffffffffffp+53L), plus_zero, 0, 0, 0);
#endif
#ifdef TEST_LDOUBLE
# if LDBL_MANT_DIG >= 64
  check_float ("pow (-0, 0x1.fffffffffffffffep+63) == -0",  FUNC(pow) (minus_zero, 0x1.fffffffffffffffep+63L), minus_zero, 0, 0, 0);
  check_float ("pow (-0, 0x1.fffffffffffffffep+64) == +0",  FUNC(pow) (minus_zero, 0x1.fffffffffffffffep+64L), plus_zero, 0, 0, 0);
# endif
# if LDBL_MANT_DIG >= 106
  check_float ("pow (-0, 0x1.ffffffffffffffffffffffffff8p+105) == -0",  FUNC(pow) (minus_zero, 0x1.ffffffffffffffffffffffffff8p+105L), minus_zero, 0, 0, 0);
  check_float ("pow (-0, 0x1.ffffffffffffffffffffffffff8p+106) == +0",  FUNC(pow) (minus_zero, 0x1.ffffffffffffffffffffffffff8p+106L), plus_zero, 0, 0, 0);
# endif
# if LDBL_MANT_DIG >= 113
  check_float ("pow (-0, 0x1.ffffffffffffffffffffffffffffp+112) == -0",  FUNC(pow) (minus_zero, 0x1.ffffffffffffffffffffffffffffp+112L), minus_zero, 0, 0, 0);
  check_float ("pow (-0, 0x1.ffffffffffffffffffffffffffffp+113) == +0",  FUNC(pow) (minus_zero, 0x1.ffffffffffffffffffffffffffffp+113L), plus_zero, 0, 0, 0);
# endif
#endif

  /* pow (+0, y) == +0 for y > 0 and not an odd integer.  */
  check_float ("pow (0.0, 4) == 0.0",  FUNC(pow) (0.0, 4), 0.0, 0, 0, 0);
  check_float ("pow (0.0, 0x1p24) == 0.0",  FUNC(pow) (0.0, 0x1p24), 0.0, 0, 0, 0);
  check_float ("pow (0.0, 0x1p127) == 0.0",  FUNC(pow) (0.0, 0x1p127), 0.0, 0, 0, 0);
  check_float ("pow (0.0, max_value) == 0.0",  FUNC(pow) (0.0, max_value), 0.0, 0, 0, 0);
  check_float ("pow (0.0, min_subnorm_value) == 0.0",  FUNC(pow) (0.0, min_subnorm_value), 0.0, 0, 0, 0);

  /* pow (-0, y) == +0 for y > 0 and not an odd integer.  */
  check_float ("pow (-0, 4) == 0.0",  FUNC(pow) (minus_zero, 4), 0.0, 0, 0, 0);
  check_float ("pow (-0, 0x1p24) == 0.0",  FUNC(pow) (minus_zero, 0x1p24), 0.0, 0, 0, 0);
  check_float ("pow (-0, 0x1p127) == 0.0",  FUNC(pow) (minus_zero, 0x1p127), 0.0, 0, 0, 0);
  check_float ("pow (-0, max_value) == 0.0",  FUNC(pow) (minus_zero, max_value), 0.0, 0, 0, 0);
  check_float ("pow (-0, min_subnorm_value) == 0.0",  FUNC(pow) (minus_zero, min_subnorm_value), 0.0, 0, 0, 0);

  check_float ("pow (16, 0.25) == 2",  FUNC(pow) (16, 0.25L), 2, 0, 0, 0);
  check_float ("pow (0x1p64, 0.125) == 256",  FUNC(pow) (0x1p64L, 0.125L), 256, 0, 0, 0);
  check_float ("pow (2, 4) == 16",  FUNC(pow) (2, 4), 16, 0, 0, 0);
  check_float ("pow (256, 8) == 0x1p64",  FUNC(pow) (256, 8), 0x1p64L, 0, 0, 0);

  check_float ("pow (0.75, 1.25) == 0.697953644326574699205914060237425566",  FUNC(pow) (0.75L, 1.25L), 0.697953644326574699205914060237425566L, 0, 0, 0);

#if defined TEST_DOUBLE || defined TEST_LDOUBLE
  check_float ("pow (-7.49321e+133, -9.80818e+16) == 0",  FUNC(pow) (-7.49321e+133, -9.80818e+16), 0, 0, 0, UNDERFLOW_EXCEPTION);
#endif

  check_float ("pow (-1.0, -0xffffff) == -1.0",  FUNC(pow) (-1.0, -0xffffff), -1.0, 0, 0, 0);
  check_float ("pow (-1.0, -0x1fffffe) == 1.0",  FUNC(pow) (-1.0, -0x1fffffe), 1.0, 0, 0, 0);
#ifndef TEST_FLOAT
  check_float ("pow (-1.0, -0x1.fffffffffffffp+52) == -1.0",  FUNC(pow) (-1.0, -0x1.fffffffffffffp+52L), -1.0, 0, 0, 0);
  check_float ("pow (-1.0, -0x1.fffffffffffffp+53) == 1.0",  FUNC(pow) (-1.0, -0x1.fffffffffffffp+53L), 1.0, 0, 0, 0);
#endif
#ifdef TEST_LDOUBLE
# if LDBL_MANT_DIG >= 64
  check_float ("pow (-1.0, -0x1.fffffffffffffffep+63) == -1.0",  FUNC(pow) (-1.0, -0x1.fffffffffffffffep+63L), -1.0, 0, 0, 0);
  check_float ("pow (-1.0, -0x1.fffffffffffffffep+64) == 1.0",  FUNC(pow) (-1.0, -0x1.fffffffffffffffep+64L), 1.0, 0, 0, 0);
# endif
# if LDBL_MANT_DIG >= 106
  check_float ("pow (-1.0, -0x1.ffffffffffffffffffffffffff8p+105) == -1.0",  FUNC(pow) (-1.0, -0x1.ffffffffffffffffffffffffff8p+105L), -1.0, 0, 0, 0);
  check_float ("pow (-1.0, -0x1.ffffffffffffffffffffffffff8p+106) == 1.0",  FUNC(pow) (-1.0, -0x1.ffffffffffffffffffffffffff8p+106L), 1.0, 0, 0, 0);
# endif
# if LDBL_MANT_DIG >= 113
  check_float ("pow (-1.0, -0x1.ffffffffffffffffffffffffffffp+112) == -1.0",  FUNC(pow) (-1.0, -0x1.ffffffffffffffffffffffffffffp+112L), -1.0, 0, 0, 0);
  check_float ("pow (-1.0, -0x1.ffffffffffffffffffffffffffffp+113) == 1.0",  FUNC(pow) (-1.0, -0x1.ffffffffffffffffffffffffffffp+113L), 1.0, 0, 0, 0);
# endif
#endif
  check_float ("pow (-1.0, -max_value) == 1.0",  FUNC(pow) (-1.0, -max_value), 1.0, 0, 0, 0);

  check_float ("pow (-1.0, 0xffffff) == -1.0",  FUNC(pow) (-1.0, 0xffffff), -1.0, 0, 0, 0);
  check_float ("pow (-1.0, 0x1fffffe) == 1.0",  FUNC(pow) (-1.0, 0x1fffffe), 1.0, 0, 0, 0);
#ifndef TEST_FLOAT
  check_float ("pow (-1.0, 0x1.fffffffffffffp+52) == -1.0",  FUNC(pow) (-1.0, 0x1.fffffffffffffp+52L), -1.0, 0, 0, 0);
  check_float ("pow (-1.0, 0x1.fffffffffffffp+53) == 1.0",  FUNC(pow) (-1.0, 0x1.fffffffffffffp+53L), 1.0, 0, 0, 0);
#endif
#ifdef TEST_LDOUBLE
# if LDBL_MANT_DIG >= 64
  check_float ("pow (-1.0, 0x1.fffffffffffffffep+63) == -1.0",  FUNC(pow) (-1.0, 0x1.fffffffffffffffep+63L), -1.0, 0, 0, 0);
  check_float ("pow (-1.0, 0x1.fffffffffffffffep+64) == 1.0",  FUNC(pow) (-1.0, 0x1.fffffffffffffffep+64L), 1.0, 0, 0, 0);
# endif
# if LDBL_MANT_DIG >= 106
  check_float ("pow (-1.0, 0x1.ffffffffffffffffffffffffff8p+105) == -1.0",  FUNC(pow) (-1.0, 0x1.ffffffffffffffffffffffffff8p+105L), -1.0, 0, 0, 0);
  check_float ("pow (-1.0, 0x1.ffffffffffffffffffffffffff8p+106) == 1.0",  FUNC(pow) (-1.0, 0x1.ffffffffffffffffffffffffff8p+106L), 1.0, 0, 0, 0);
# endif
# if LDBL_MANT_DIG >= 113
  check_float ("pow (-1.0, 0x1.ffffffffffffffffffffffffffffp+112) == -1.0",  FUNC(pow) (-1.0, 0x1.ffffffffffffffffffffffffffffp+112L), -1.0, 0, 0, 0);
  check_float ("pow (-1.0, 0x1.ffffffffffffffffffffffffffffp+113) == 1.0",  FUNC(pow) (-1.0, 0x1.ffffffffffffffffffffffffffffp+113L), 1.0, 0, 0, 0);
# endif
#endif
  check_float ("pow (-1.0, max_value) == 1.0",  FUNC(pow) (-1.0, max_value), 1.0, 0, 0, 0);

  check_float ("pow (-2.0, 126) == 0x1p126",  FUNC(pow) (-2.0, 126), 0x1p126, 0, 0, 0);
  check_float ("pow (-2.0, 127) == -0x1p127",  FUNC(pow) (-2.0, 127), -0x1p127, 0, 0, 0);
  /* Allow inexact results for float to be considered to underflow.  */
  check_float ("pow (-2.0, -126) == 0x1p-126",  FUNC(pow) (-2.0, -126), 0x1p-126, 0, 0, UNDERFLOW_EXCEPTION_OK_FLOAT);
  check_float ("pow (-2.0, -127) == -0x1p-127",  FUNC(pow) (-2.0, -127), -0x1p-127, 0, 0, UNDERFLOW_EXCEPTION_OK_FLOAT);

  check_float ("pow (-2.0, -0xffffff) == -0",  FUNC(pow) (-2.0, -0xffffff), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("pow (-2.0, -0x1fffffe) == +0",  FUNC(pow) (-2.0, -0x1fffffe), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
#ifndef TEST_FLOAT
  check_float ("pow (-2.0, -0x1.fffffffffffffp+52) == -0",  FUNC(pow) (-2.0, -0x1.fffffffffffffp+52L), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("pow (-2.0, -0x1.fffffffffffffp+53) == +0",  FUNC(pow) (-2.0, -0x1.fffffffffffffp+53L), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
#endif
#ifdef TEST_LDOUBLE
# if LDBL_MANT_DIG >= 64
  check_float ("pow (-2.0, -0x1.fffffffffffffffep+63) == -0",  FUNC(pow) (-2.0, -0x1.fffffffffffffffep+63L), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("pow (-2.0, -0x1.fffffffffffffffep+64) == +0",  FUNC(pow) (-2.0, -0x1.fffffffffffffffep+64L), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
# endif
# if LDBL_MANT_DIG >= 106
  check_float ("pow (-2.0, -0x1.ffffffffffffffffffffffffff8p+105) == -0",  FUNC(pow) (-2.0, -0x1.ffffffffffffffffffffffffff8p+105L), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("pow (-2.0, -0x1.ffffffffffffffffffffffffff8p+106) == +0",  FUNC(pow) (-2.0, -0x1.ffffffffffffffffffffffffff8p+106L), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
# endif
# if LDBL_MANT_DIG >= 113
  check_float ("pow (-2.0, -0x1.ffffffffffffffffffffffffffffp+112) == -0",  FUNC(pow) (-2.0, -0x1.ffffffffffffffffffffffffffffp+112L), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("pow (-2.0, -0x1.ffffffffffffffffffffffffffffp+113) == +0",  FUNC(pow) (-2.0, -0x1.ffffffffffffffffffffffffffffp+113L), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
# endif
#endif
  check_float ("pow (-2.0, -max_value) == +0",  FUNC(pow) (-2.0, -max_value), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);

  check_float ("pow (-2.0, 0xffffff) == -inf",  FUNC(pow) (-2.0, 0xffffff), minus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("pow (-2.0, 0x1fffffe) == inf",  FUNC(pow) (-2.0, 0x1fffffe), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
#ifndef TEST_FLOAT
  check_float ("pow (-2.0, 0x1.fffffffffffffp+52) == -inf",  FUNC(pow) (-2.0, 0x1.fffffffffffffp+52L), minus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("pow (-2.0, 0x1.fffffffffffffp+53) == inf",  FUNC(pow) (-2.0, 0x1.fffffffffffffp+53L), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
#endif
#ifdef TEST_LDOUBLE
# if LDBL_MANT_DIG >= 64
  check_float ("pow (-2.0, 0x1.fffffffffffffffep+63) == -inf",  FUNC(pow) (-2.0, 0x1.fffffffffffffffep+63L), minus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("pow (-2.0, 0x1.fffffffffffffffep+64) == inf",  FUNC(pow) (-2.0, 0x1.fffffffffffffffep+64L), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
# endif
# if LDBL_MANT_DIG >= 106
  check_float ("pow (-2.0, 0x1.ffffffffffffffffffffffffff8p+105) == -inf",  FUNC(pow) (-2.0, 0x1.ffffffffffffffffffffffffff8p+105L), minus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("pow (-2.0, 0x1.ffffffffffffffffffffffffff8p+106) == inf",  FUNC(pow) (-2.0, 0x1.ffffffffffffffffffffffffff8p+106L), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
# endif
# if LDBL_MANT_DIG >= 113
  check_float ("pow (-2.0, 0x1.ffffffffffffffffffffffffffffp+112) == -inf",  FUNC(pow) (-2.0, 0x1.ffffffffffffffffffffffffffffp+112L), minus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("pow (-2.0, 0x1.ffffffffffffffffffffffffffffp+113) == inf",  FUNC(pow) (-2.0, 0x1.ffffffffffffffffffffffffffffp+113L), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
# endif
#endif
  check_float ("pow (-2.0, max_value) == inf",  FUNC(pow) (-2.0, max_value), plus_infty, 0, 0, OVERFLOW_EXCEPTION);

  check_float ("pow (-max_value, 0.5) == NaN",  FUNC(pow) (-max_value, 0.5), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("pow (-max_value, 1.5) == NaN",  FUNC(pow) (-max_value, 1.5), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("pow (-max_value, 1000.5) == NaN",  FUNC(pow) (-max_value, 1000.5), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("pow (-max_value, -2) == +0",  FUNC(pow) (-max_value, -2), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("pow (-max_value, -3) == -0",  FUNC(pow) (-max_value, -3), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("pow (-max_value, 2) == inf",  FUNC(pow) (-max_value, 2), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("pow (-max_value, 3) == -inf",  FUNC(pow) (-max_value, 3), minus_infty, 0, 0, OVERFLOW_EXCEPTION);

  check_float ("pow (-max_value, -0xffffff) == -0",  FUNC(pow) (-max_value, -0xffffff), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("pow (-max_value, -0x1fffffe) == +0",  FUNC(pow) (-max_value, -0x1fffffe), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
#ifndef TEST_FLOAT
  check_float ("pow (-max_value, -0x1.fffffffffffffp+52) == -0",  FUNC(pow) (-max_value, -0x1.fffffffffffffp+52L), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("pow (-max_value, -0x1.fffffffffffffp+53) == +0",  FUNC(pow) (-max_value, -0x1.fffffffffffffp+53L), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
#endif
#ifdef TEST_LDOUBLE
# if LDBL_MANT_DIG >= 64
  check_float ("pow (-max_value, -0x1.fffffffffffffffep+63) == -0",  FUNC(pow) (-max_value, -0x1.fffffffffffffffep+63L), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("pow (-max_value, -0x1.fffffffffffffffep+64) == +0",  FUNC(pow) (-max_value, -0x1.fffffffffffffffep+64L), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
# endif
# if LDBL_MANT_DIG >= 106
  check_float ("pow (-max_value, -0x1.ffffffffffffffffffffffffff8p+105) == -0",  FUNC(pow) (-max_value, -0x1.ffffffffffffffffffffffffff8p+105L), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("pow (-max_value, -0x1.ffffffffffffffffffffffffff8p+106) == +0",  FUNC(pow) (-max_value, -0x1.ffffffffffffffffffffffffff8p+106L), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
# endif
# if LDBL_MANT_DIG >= 113
  check_float ("pow (-max_value, -0x1.ffffffffffffffffffffffffffffp+112) == -0",  FUNC(pow) (-max_value, -0x1.ffffffffffffffffffffffffffffp+112L), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("pow (-max_value, -0x1.ffffffffffffffffffffffffffffp+113) == +0",  FUNC(pow) (-max_value, -0x1.ffffffffffffffffffffffffffffp+113L), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
# endif
#endif
  check_float ("pow (-max_value, -max_value) == +0",  FUNC(pow) (-max_value, -max_value), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);

  check_float ("pow (-max_value, 0xffffff) == -inf",  FUNC(pow) (-max_value, 0xffffff), minus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("pow (-max_value, 0x1fffffe) == inf",  FUNC(pow) (-max_value, 0x1fffffe), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
#ifndef TEST_FLOAT
  check_float ("pow (-max_value, 0x1.fffffffffffffp+52) == -inf",  FUNC(pow) (-max_value, 0x1.fffffffffffffp+52L), minus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("pow (-max_value, 0x1.fffffffffffffp+53) == inf",  FUNC(pow) (-max_value, 0x1.fffffffffffffp+53L), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
#endif
#ifdef TEST_LDOUBLE
# if LDBL_MANT_DIG >= 64
  check_float ("pow (-max_value, 0x1.fffffffffffffffep+63) == -inf",  FUNC(pow) (-max_value, 0x1.fffffffffffffffep+63L), minus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("pow (-max_value, 0x1.fffffffffffffffep+64) == inf",  FUNC(pow) (-max_value, 0x1.fffffffffffffffep+64L), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
# endif
# if LDBL_MANT_DIG >= 106
  check_float ("pow (-max_value, 0x1.ffffffffffffffffffffffffff8p+105) == -inf",  FUNC(pow) (-max_value, 0x1.ffffffffffffffffffffffffff8p+105L), minus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("pow (-max_value, 0x1.ffffffffffffffffffffffffff8p+106) == inf",  FUNC(pow) (-max_value, 0x1.ffffffffffffffffffffffffff8p+106L), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
# endif
# if LDBL_MANT_DIG >= 113
  check_float ("pow (-max_value, 0x1.ffffffffffffffffffffffffffffp+112) == -inf",  FUNC(pow) (-max_value, 0x1.ffffffffffffffffffffffffffffp+112L), minus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("pow (-max_value, 0x1.ffffffffffffffffffffffffffffp+113) == inf",  FUNC(pow) (-max_value, 0x1.ffffffffffffffffffffffffffffp+113L), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
# endif
#endif
  check_float ("pow (-max_value, max_value) == inf",  FUNC(pow) (-max_value, max_value), plus_infty, 0, 0, OVERFLOW_EXCEPTION);

  check_float ("pow (-0.5, 126) == 0x1p-126",  FUNC(pow) (-0.5, 126), 0x1p-126, 0, 0, 0);
  check_float ("pow (-0.5, 127) == -0x1p-127",  FUNC(pow) (-0.5, 127), -0x1p-127, 0, 0, 0);
  check_float ("pow (-0.5, -126) == 0x1p126",  FUNC(pow) (-0.5, -126), 0x1p126, 0, 0, 0);
  check_float ("pow (-0.5, -127) == -0x1p127",  FUNC(pow) (-0.5, -127), -0x1p127, 0, 0, 0);

  check_float ("pow (-0.5, -0xffffff) == -inf",  FUNC(pow) (-0.5, -0xffffff), minus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("pow (-0.5, -0x1fffffe) == inf",  FUNC(pow) (-0.5, -0x1fffffe), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
#ifndef TEST_FLOAT
  check_float ("pow (-0.5, -0x1.fffffffffffffp+52) == -inf",  FUNC(pow) (-0.5, -0x1.fffffffffffffp+52L), minus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("pow (-0.5, -0x1.fffffffffffffp+53) == inf",  FUNC(pow) (-0.5, -0x1.fffffffffffffp+53L), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
#endif
#ifdef TEST_LDOUBLE
# if LDBL_MANT_DIG >= 64
  check_float ("pow (-0.5, -0x1.fffffffffffffffep+63) == -inf",  FUNC(pow) (-0.5, -0x1.fffffffffffffffep+63L), minus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("pow (-0.5, -0x1.fffffffffffffffep+64) == inf",  FUNC(pow) (-0.5, -0x1.fffffffffffffffep+64L), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
# endif
# if LDBL_MANT_DIG >= 106
  check_float ("pow (-0.5, -0x1.ffffffffffffffffffffffffff8p+105) == -inf",  FUNC(pow) (-0.5, -0x1.ffffffffffffffffffffffffff8p+105L), minus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("pow (-0.5, -0x1.ffffffffffffffffffffffffff8p+106) == inf",  FUNC(pow) (-0.5, -0x1.ffffffffffffffffffffffffff8p+106L), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
# endif
# if LDBL_MANT_DIG >= 113
  check_float ("pow (-0.5, -0x1.ffffffffffffffffffffffffffffp+112) == -inf",  FUNC(pow) (-0.5, -0x1.ffffffffffffffffffffffffffffp+112L), minus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("pow (-0.5, -0x1.ffffffffffffffffffffffffffffp+113) == inf",  FUNC(pow) (-0.5, -0x1.ffffffffffffffffffffffffffffp+113L), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
# endif
#endif
  check_float ("pow (-0.5, -max_value) == inf",  FUNC(pow) (-0.5, -max_value), plus_infty, 0, 0, OVERFLOW_EXCEPTION);

  check_float ("pow (-0.5, 0xffffff) == -0",  FUNC(pow) (-0.5, 0xffffff), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("pow (-0.5, 0x1fffffe) == +0",  FUNC(pow) (-0.5, 0x1fffffe), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
#ifndef TEST_FLOAT
  check_float ("pow (-0.5, 0x1.fffffffffffffp+52) == -0",  FUNC(pow) (-0.5, 0x1.fffffffffffffp+52L), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("pow (-0.5, 0x1.fffffffffffffp+53) == +0",  FUNC(pow) (-0.5, 0x1.fffffffffffffp+53L), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
#endif
#ifdef TEST_LDOUBLE
# if LDBL_MANT_DIG >= 64
  check_float ("pow (-0.5, 0x1.fffffffffffffffep+63) == -0",  FUNC(pow) (-0.5, 0x1.fffffffffffffffep+63L), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("pow (-0.5, 0x1.fffffffffffffffep+64) == +0",  FUNC(pow) (-0.5, 0x1.fffffffffffffffep+64L), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
# endif
# if LDBL_MANT_DIG >= 106
  check_float ("pow (-0.5, 0x1.ffffffffffffffffffffffffff8p+105) == -0",  FUNC(pow) (-0.5, 0x1.ffffffffffffffffffffffffff8p+105L), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("pow (-0.5, 0x1.ffffffffffffffffffffffffff8p+106) == +0",  FUNC(pow) (-0.5, 0x1.ffffffffffffffffffffffffff8p+106L), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
# endif
# if LDBL_MANT_DIG >= 113
  check_float ("pow (-0.5, 0x1.ffffffffffffffffffffffffffffp+112) == -0",  FUNC(pow) (-0.5, 0x1.ffffffffffffffffffffffffffffp+112L), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("pow (-0.5, 0x1.ffffffffffffffffffffffffffffp+113) == +0",  FUNC(pow) (-0.5, 0x1.ffffffffffffffffffffffffffffp+113L), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
# endif
#endif
  check_float ("pow (-0.5, max_value) == +0",  FUNC(pow) (-0.5, max_value), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);

  check_float ("pow (-min_value, 0.5) == NaN",  FUNC(pow) (-min_value, 0.5), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("pow (-min_value, 1.5) == NaN",  FUNC(pow) (-min_value, 1.5), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("pow (-min_value, 1000.5) == NaN",  FUNC(pow) (-min_value, 1000.5), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("pow (-min_value, -2) == inf",  FUNC(pow) (-min_value, -2), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("pow (-min_value, -3) == -inf",  FUNC(pow) (-min_value, -3), minus_infty, 0, 0, OVERFLOW_EXCEPTION);
  /* Allow inexact results to be considered to underflow.  */
  check_float ("pow (-min_value, 1) == -min_value",  FUNC(pow) (-min_value, 1), -min_value, 0, 0, UNDERFLOW_EXCEPTION_OK);
  check_float ("pow (-min_value, 2) == +0",  FUNC(pow) (-min_value, 2), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("pow (-min_value, 3) == -0",  FUNC(pow) (-min_value, 3), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);

  check_float ("pow (-min_value, -0xffffff) == -inf",  FUNC(pow) (-min_value, -0xffffff), minus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("pow (-min_value, -0x1fffffe) == inf",  FUNC(pow) (-min_value, -0x1fffffe), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
#ifndef TEST_FLOAT
  check_float ("pow (-min_value, -0x1.fffffffffffffp+52) == -inf",  FUNC(pow) (-min_value, -0x1.fffffffffffffp+52L), minus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("pow (-min_value, -0x1.fffffffffffffp+53) == inf",  FUNC(pow) (-min_value, -0x1.fffffffffffffp+53L), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
#endif
#ifdef TEST_LDOUBLE
# if LDBL_MANT_DIG >= 64
  check_float ("pow (-min_value, -0x1.fffffffffffffffep+63) == -inf",  FUNC(pow) (-min_value, -0x1.fffffffffffffffep+63L), minus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("pow (-min_value, -0x1.fffffffffffffffep+64) == inf",  FUNC(pow) (-min_value, -0x1.fffffffffffffffep+64L), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
# endif
# if LDBL_MANT_DIG >= 106
  check_float ("pow (-min_value, -0x1.ffffffffffffffffffffffffff8p+105) == -inf",  FUNC(pow) (-min_value, -0x1.ffffffffffffffffffffffffff8p+105L), minus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("pow (-min_value, -0x1.ffffffffffffffffffffffffff8p+106) == inf",  FUNC(pow) (-min_value, -0x1.ffffffffffffffffffffffffff8p+106L), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
# endif
# if LDBL_MANT_DIG >= 113
  check_float ("pow (-min_value, -0x1.ffffffffffffffffffffffffffffp+112) == -inf",  FUNC(pow) (-min_value, -0x1.ffffffffffffffffffffffffffffp+112L), minus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("pow (-min_value, -0x1.ffffffffffffffffffffffffffffp+113) == inf",  FUNC(pow) (-min_value, -0x1.ffffffffffffffffffffffffffffp+113L), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
# endif
#endif
  check_float ("pow (-min_value, -max_value) == inf",  FUNC(pow) (-min_value, -max_value), plus_infty, 0, 0, OVERFLOW_EXCEPTION);

  check_float ("pow (-min_value, 0xffffff) == -0",  FUNC(pow) (-min_value, 0xffffff), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("pow (-min_value, 0x1fffffe) == +0",  FUNC(pow) (-min_value, 0x1fffffe), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
#ifndef TEST_FLOAT
  check_float ("pow (-min_value, 0x1.fffffffffffffp+52) == -0",  FUNC(pow) (-min_value, 0x1.fffffffffffffp+52L), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("pow (-min_value, 0x1.fffffffffffffp+53) == +0",  FUNC(pow) (-min_value, 0x1.fffffffffffffp+53L), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
#endif
#ifdef TEST_LDOUBLE
# if LDBL_MANT_DIG >= 64
  check_float ("pow (-min_value, 0x1.fffffffffffffffep+63) == -0",  FUNC(pow) (-min_value, 0x1.fffffffffffffffep+63L), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("pow (-min_value, 0x1.fffffffffffffffep+64) == +0",  FUNC(pow) (-min_value, 0x1.fffffffffffffffep+64L), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
# endif
# if LDBL_MANT_DIG >= 106
  check_float ("pow (-min_value, 0x1.ffffffffffffffffffffffffff8p+105) == -0",  FUNC(pow) (-min_value, 0x1.ffffffffffffffffffffffffff8p+105L), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("pow (-min_value, 0x1.ffffffffffffffffffffffffff8p+106) == +0",  FUNC(pow) (-min_value, 0x1.ffffffffffffffffffffffffff8p+106L), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
# endif
# if LDBL_MANT_DIG >= 113
  check_float ("pow (-min_value, 0x1.ffffffffffffffffffffffffffffp+112) == -0",  FUNC(pow) (-min_value, 0x1.ffffffffffffffffffffffffffffp+112L), minus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("pow (-min_value, 0x1.ffffffffffffffffffffffffffffp+113) == +0",  FUNC(pow) (-min_value, 0x1.ffffffffffffffffffffffffffffp+113L), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
# endif
#endif
  check_float ("pow (-min_value, max_value) == +0",  FUNC(pow) (-min_value, max_value), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);

  check_float ("pow (0x0.ffffffp0, 10) == 0.999999403953712118183885036774764444747",  FUNC(pow) (0x0.ffffffp0, 10), 0.999999403953712118183885036774764444747L, 0, 0, 0);
  check_float ("pow (0x0.ffffffp0, 100) == 0.999994039553108359406305079606228341585",  FUNC(pow) (0x0.ffffffp0, 100), 0.999994039553108359406305079606228341585L, 0, 0, 0);
  check_float ("pow (0x0.ffffffp0, 1000) == 0.9999403971297699052276650144650733772182",  FUNC(pow) (0x0.ffffffp0, 1000), 0.9999403971297699052276650144650733772182L, 0, 0, 0);
  check_float ("pow (0x0.ffffffp0, 0x1p24) == 0.3678794302077803437135155590023422899744",  FUNC(pow) (0x0.ffffffp0, 0x1p24), 0.3678794302077803437135155590023422899744L, DELTA4128, 0, 0);
  check_float ("pow (0x0.ffffffp0, 0x1p30) == 1.603807831524924233828134753069728224044e-28",  FUNC(pow) (0x0.ffffffp0, 0x1p30), 1.603807831524924233828134753069728224044e-28L, 0, 0, 0);
  check_float ("pow (0x0.ffffffp0, 0x1.234566p30) == 2.374884712135295099971443365381007297732e-32",  FUNC(pow) (0x0.ffffffp0, 0x1.234566p30), 2.374884712135295099971443365381007297732e-32L, 0, 0, 0);
  check_float ("pow (0x0.ffffffp0, -10) == 1.000000596046643153205170848674671339688",  FUNC(pow) (0x0.ffffffp0, -10), 1.000000596046643153205170848674671339688L, 0, 0, 0);
  check_float ("pow (0x0.ffffffp0, -100) == 1.000005960482418779499387594989252621451",  FUNC(pow) (0x0.ffffffp0, -100), 1.000005960482418779499387594989252621451L, 0, 0, 0);
  check_float ("pow (0x0.ffffffp0, -1000) == 1.000059606422943986382898964231519867906",  FUNC(pow) (0x0.ffffffp0, -1000), 1.000059606422943986382898964231519867906L, 0, 0, 0);
  check_float ("pow (0x0.ffffffp0, -0x1p24) == 2.7182819094701610539628664526874952929416",  FUNC(pow) (0x0.ffffffp0, -0x1p24), 2.7182819094701610539628664526874952929416L, DELTA4134, 0, 0);
  check_float ("pow (0x0.ffffffp0, -0x1p30) == 6.2351609734265057988914412331288163636075e+27",  FUNC(pow) (0x0.ffffffp0, -0x1p30), 6.2351609734265057988914412331288163636075e+27L, 0, 0, 0);
  check_float ("pow (0x0.ffffffp0, -0x1.234566p30) == 4.2107307141696353498921307077142537353515e+31",  FUNC(pow) (0x0.ffffffp0, -0x1.234566p30), 4.2107307141696353498921307077142537353515e+31L, 0, 0, 0);
  check_float ("pow (0x1.000002p0, 0x1p24) == 7.3890552180866447284268641248075832310141",  FUNC(pow) (0x1.000002p0, 0x1p24), 7.3890552180866447284268641248075832310141L, DELTA4137, 0, 0);
  check_float ("pow (0x1.000002p0, 0x1.234566p29) == 4.2107033006507495188536371520637025716256e+31",  FUNC(pow) (0x1.000002p0, 0x1.234566p29), 4.2107033006507495188536371520637025716256e+31L, 0, 0, 0);
  check_float ("pow (0x1.000002p0, -0x1.234566p29) == 2.3749001736727769098946062325205705312166e-32",  FUNC(pow) (0x1.000002p0, -0x1.234566p29), 2.3749001736727769098946062325205705312166e-32L, 0, 0, 0);

#if !defined TEST_FLOAT
  check_float ("pow (0x0.fffffffffffff8p0, 0x1.23456789abcdfp62) == 1.0118762747827252817436395051178295138220e-253",  FUNC(pow) (0x0.fffffffffffff8p0L, 0x1.23456789abcdfp62L), 1.0118762747827252817436395051178295138220e-253L, 0, 0, 0);
  check_float ("pow (0x0.fffffffffffff8p0, -0x1.23456789abcdfp62) == 9.8826311568054561811190162420900667121992e+252",  FUNC(pow) (0x0.fffffffffffff8p0L, -0x1.23456789abcdfp62L), 9.8826311568054561811190162420900667121992e+252L, 0, 0, 0);
  check_float ("pow (0x1.0000000000001p0, 0x1.23456789abcdfp61) == 9.8826311568044974397135026217687399395481e+252",  FUNC(pow) (0x1.0000000000001p0L, 0x1.23456789abcdfp61L), 9.8826311568044974397135026217687399395481e+252L, 0, 0, 0);
  check_float ("pow (0x1.0000000000001p0, -0x1.23456789abcdfp61) == 1.0118762747828234466621210689458255908670e-253",  FUNC(pow) (0x1.0000000000001p0L, -0x1.23456789abcdfp61L), 1.0118762747828234466621210689458255908670e-253L, 0, 0, 0);
#endif

#if defined TEST_LDOUBLE && LDBL_MANT_DIG >= 64 && LDBL_MAX_EXP >= 16384
  check_float ("pow (0x0.ffffffffffffffffp0, 0x1.23456789abcdef0ep77) == 1.2079212226420368189981778807634890018840e-4048",  FUNC(pow) (0x0.ffffffffffffffffp0L, 0x1.23456789abcdef0ep77L), 1.2079212226420368189981778807634890018840e-4048L, 0, 0, 0);
  check_float ("pow (0x0.ffffffffffffffffp0, -0x1.23456789abcdef0ep77) == 8.2786855736563746280496724205839522148001e+4047",  FUNC(pow) (0x0.ffffffffffffffffp0L, -0x1.23456789abcdef0ep77L), 8.2786855736563746280496724205839522148001e+4047L, 0, 0, 0);
  check_float ("pow (0x1.0000000000000002p0, 0x1.23456789abcdef0ep76) == 8.2786855736563683535324500168799315131570e+4047",  FUNC(pow) (0x1.0000000000000002p0L, 0x1.23456789abcdef0ep76L), 8.2786855736563683535324500168799315131570e+4047L, 0, 0, 0);
  check_float ("pow (0x1.0000000000000002p0, -0x1.23456789abcdef0ep76) == 1.2079212226420377344964713407722652880280e-4048",  FUNC(pow) (0x1.0000000000000002p0L, -0x1.23456789abcdef0ep76L), 1.2079212226420377344964713407722652880280e-4048L, 0, 0, 0);
#endif

#if defined TEST_LDOUBLE && LDBL_MANT_DIG >= 113
  check_float ("pow (0x0.ffffffffffffffffffffffffffff8p0, 0x1.23456789abcdef0123456789abcdp126) == 1.2079212226420440237790185999151440179953e-4048",  FUNC(pow) (0x0.ffffffffffffffffffffffffffff8p0L, 0x1.23456789abcdef0123456789abcdp126L), 1.2079212226420440237790185999151440179953e-4048L, 0, 0, 0);
  check_float ("pow (0x0.ffffffffffffffffffffffffffff8p0, -0x1.23456789abcdef0123456789abcdp126) == 8.2786855736563252489063231915535105363602e+4047",  FUNC(pow) (0x0.ffffffffffffffffffffffffffff8p0L, -0x1.23456789abcdef0123456789abcdp126L), 8.2786855736563252489063231915535105363602e+4047L, 0, 0, 0);
  check_float ("pow (0x1.0000000000000000000000000001p0, 0x1.23456789abcdef0123456789abcdp125) == 8.2786855736563252489063231915423647547782e+4047",  FUNC(pow) (0x1.0000000000000000000000000001p0L, 0x1.23456789abcdef0123456789abcdp125L), 8.2786855736563252489063231915423647547782e+4047L, 0, 0, 0);
  check_float ("pow (0x1.0000000000000000000000000001p0, -0x1.23456789abcdef0123456789abcdp125) == 1.2079212226420440237790185999167702696503e-4048",  FUNC(pow) (0x1.0000000000000000000000000001p0L, -0x1.23456789abcdef0123456789abcdp125L), 1.2079212226420440237790185999167702696503e-4048L, 0, 0, 0);
#endif

#if defined TEST_LDOUBLE && LDBL_MAX_EXP >= 16384
  check_float ("pow (1e4932, 0.75) == 1e3699",  FUNC(pow) (1e4932L, 0.75L), 1e3699L, 0, 0, 0);
  check_float ("pow (1e4928, 0.75) == 1e3696",  FUNC(pow) (1e4928L, 0.75L), 1e3696L, 0, 0, 0);
  check_float ("pow (1e4924, 0.75) == 1e3693",  FUNC(pow) (1e4924L, 0.75L), 1e3693L, 0, 0, 0);
  check_float ("pow (1e4920, 0.75) == 1e3690",  FUNC(pow) (1e4920L, 0.75L), 1e3690L, 0, 0, 0);
  check_float ("pow (10.0, 4932.0) == 1e4932",  FUNC(pow) (10.0L, 4932.0L), 1e4932L, 0, 0, 0);
  check_float ("pow (10.0, 4931.0) == 1e4931",  FUNC(pow) (10.0L, 4931.0L), 1e4931L, 0, 0, 0);
  check_float ("pow (10.0, 4930.0) == 1e4930",  FUNC(pow) (10.0L, 4930.0L), 1e4930L, 0, 0, 0);
  check_float ("pow (10.0, 4929.0) == 1e4929",  FUNC(pow) (10.0L, 4929.0L), 1e4929L, 0, 0, 0);
  check_float ("pow (10.0, -4931.0) == 1e-4931",  FUNC(pow) (10.0L, -4931.0L), 1e-4931L, 0, 0, 0);
  check_float ("pow (10.0, -4930.0) == 1e-4930",  FUNC(pow) (10.0L, -4930.0L), 1e-4930L, 0, 0, 0);
  check_float ("pow (10.0, -4929.0) == 1e-4929",  FUNC(pow) (10.0L, -4929.0L), 1e-4929L, 0, 0, 0);
  check_float ("pow (1e27, 182.0) == 1e4914",  FUNC(pow) (1e27L, 182.0L), 1e4914L, 0, 0, 0);
  check_float ("pow (1e27, -182.0) == 1e-4914",  FUNC(pow) (1e27L, -182.0L), 1e-4914L, 0, 0, 0);
#endif

  check_float ("pow (min_subnorm_value, min_subnorm_value) == 1.0",  FUNC(pow) (min_subnorm_value, min_subnorm_value), 1.0L, 0, 0, 0);
  check_float ("pow (min_subnorm_value, -min_subnorm_value) == 1.0",  FUNC(pow) (min_subnorm_value, -min_subnorm_value), 1.0L, 0, 0, 0);
  check_float ("pow (max_value, min_subnorm_value) == 1.0",  FUNC(pow) (max_value, min_subnorm_value), 1.0L, 0, 0, 0);
  check_float ("pow (max_value, -min_subnorm_value) == 1.0",  FUNC(pow) (max_value, -min_subnorm_value), 1.0L, 0, 0, 0);
  check_float ("pow (0.99, min_subnorm_value) == 1.0",  FUNC(pow) (0.99L, min_subnorm_value), 1.0L, 0, 0, 0);
  check_float ("pow (0.99, -min_subnorm_value) == 1.0",  FUNC(pow) (0.99L, -min_subnorm_value), 1.0L, 0, 0, 0);
  check_float ("pow (1.01, min_subnorm_value) == 1.0",  FUNC(pow) (1.01L, min_subnorm_value), 1.0L, 0, 0, 0);
  check_float ("pow (1.01, -min_subnorm_value) == 1.0",  FUNC(pow) (1.01L, -min_subnorm_value), 1.0L, 0, 0, 0);

  check_float ("pow (2.0, -100000.0) == +0",  FUNC(pow) (2.0L, -100000.0L), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);

  print_max_error ("pow", DELTApow, 0);
}


static void
pow_test_tonearest (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(pow) (0, 0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_TONEAREST))
    {
  check_float ("pow_tonearest (1.0625, 1.125) == 1.070582293028761362162622578677070098674",  FUNC(pow) (1.0625L, 1.125L), 1.070582293028761362162622578677070098674L, 0, 0, 0);
  check_float ("pow_tonearest (1.5, 1.03125) == 1.519127098714743184071644334163037684948",  FUNC(pow) (1.5L, 1.03125L), 1.519127098714743184071644334163037684948L, 0, 0, 0);
    }

  fesetround (save_round_mode);

  print_max_error ("pow_tonearest", 0, 0);
}


static void
pow_test_towardzero (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(pow) (0, 0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_TOWARDZERO))
    {
  check_float ("pow_towardzero (1.0625, 1.125) == 1.070582293028761362162622578677070098674",  FUNC(pow) (1.0625L, 1.125L), 1.070582293028761362162622578677070098674L, DELTA4176, 0, 0);
  check_float ("pow_towardzero (1.5, 1.03125) == 1.519127098714743184071644334163037684948",  FUNC(pow) (1.5L, 1.03125L), 1.519127098714743184071644334163037684948L, DELTA4177, 0, 0);
    }

  fesetround (save_round_mode);

  print_max_error ("pow_towardzero", DELTApow_towardzero, 0);
}


static void
pow_test_downward (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(pow) (0, 0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_DOWNWARD))
    {
  check_float ("pow_downward (1.0625, 1.125) == 1.070582293028761362162622578677070098674",  FUNC(pow) (1.0625L, 1.125L), 1.070582293028761362162622578677070098674L, DELTA4178, 0, 0);
  check_float ("pow_downward (1.5, 1.03125) == 1.519127098714743184071644334163037684948",  FUNC(pow) (1.5L, 1.03125L), 1.519127098714743184071644334163037684948L, DELTA4179, 0, 0);
    }

  fesetround (save_round_mode);

  print_max_error ("pow_downward", DELTApow_downward, 0);
}


static void
pow_test_upward (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(pow) (0, 0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_UPWARD))
    {
  check_float ("pow_upward (1.0625, 1.125) == 1.070582293028761362162622578677070098674",  FUNC(pow) (1.0625L, 1.125L), 1.070582293028761362162622578677070098674L, DELTA4180, 0, 0);
  check_float ("pow_upward (1.5, 1.03125) == 1.519127098714743184071644334163037684948",  FUNC(pow) (1.5L, 1.03125L), 1.519127098714743184071644334163037684948L, DELTA4181, 0, 0);
    }

  fesetround (save_round_mode);

  print_max_error ("pow_upward", DELTApow_upward, 0);
}


static void
remainder_test (void)
{
  errno = 0;
  FUNC(remainder) (1.625, 1.0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  errno = 0;
  check_float ("remainder (1, 0) == NaN",  FUNC(remainder) (1, 0), nan_value, 0, 0, INVALID_EXCEPTION);
  check_int ("errno for remainder(1, 0) = EDOM ", errno, EDOM, 0, 0, 0);
  errno = 0;
  check_float ("remainder (1, -0) == NaN",  FUNC(remainder) (1, minus_zero), nan_value, 0, 0, INVALID_EXCEPTION);
  check_int ("errno for remainder(1, -0) = EDOM ", errno, EDOM, 0, 0, 0);
  errno = 0;
  check_float ("remainder (inf, 1) == NaN",  FUNC(remainder) (plus_infty, 1), nan_value, 0, 0, INVALID_EXCEPTION);
  check_int ("errno for remainder(INF, 1) = EDOM ", errno, EDOM, 0, 0, 0);
  errno = 0;
  check_float ("remainder (-inf, 1) == NaN",  FUNC(remainder) (minus_infty, 1), nan_value, 0, 0, INVALID_EXCEPTION);
  check_int ("errno for remainder(-INF, 1) = EDOM ", errno, EDOM, 0, 0, 0);
  errno = 0;
  check_float ("remainder (NaN, NaN) == NaN",  FUNC(remainder) (nan_value, nan_value), nan_value, 0, 0, 0);
  check_int ("errno for remainder(NAN, NAN) unchanged", errno, 0, 0, 0, 0);
  errno = 0;
  check_float ("remainder (0, NaN) == NaN",  FUNC(remainder) (0, nan_value), nan_value, 0, 0, 0);
  check_int ("errno for remainder(0, NAN) unchanged", errno, 0, 0, 0, 0);
  errno = 0;
  check_float ("remainder (NaN, 0) == NaN",  FUNC(remainder) (nan_value, 0), nan_value, 0, 0, 0);
  check_int ("errno for remainder(NaN, 0) unchanged", errno, 0, 0, 0, 0);

  check_float ("remainder (1.625, 1.0) == -0.375",  FUNC(remainder) (1.625, 1.0), -0.375, 0, 0, 0);
  check_float ("remainder (-1.625, 1.0) == 0.375",  FUNC(remainder) (-1.625, 1.0), 0.375, 0, 0, 0);
  check_float ("remainder (1.625, -1.0) == -0.375",  FUNC(remainder) (1.625, -1.0), -0.375, 0, 0, 0);
  check_float ("remainder (-1.625, -1.0) == 0.375",  FUNC(remainder) (-1.625, -1.0), 0.375, 0, 0, 0);
  check_float ("remainder (5.0, 2.0) == 1.0",  FUNC(remainder) (5.0, 2.0), 1.0, 0, 0, 0);
  check_float ("remainder (3.0, 2.0) == -1.0",  FUNC(remainder) (3.0, 2.0), -1.0, 0, 0, 0);

  print_max_error ("remainder", 0, 0);
}

static void
remquo_test (void)
{
  /* x is needed.  */
  int x;

  errno = 0;
  FUNC(remquo) (1.625, 1.0, &x);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_float ("remquo (1, 0, &x) == NaN",  FUNC(remquo) (1, 0, &x), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("remquo (1, -0, &x) == NaN",  FUNC(remquo) (1, minus_zero, &x), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("remquo (inf, 1, &x) == NaN",  FUNC(remquo) (plus_infty, 1, &x), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("remquo (-inf, 1, &x) == NaN",  FUNC(remquo) (minus_infty, 1, &x), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("remquo (NaN, NaN, &x) == NaN",  FUNC(remquo) (nan_value, nan_value, &x), nan_value, 0, 0, 0);

  check_float ("remquo (1.625, 1.0, &x) == -0.375",  FUNC(remquo) (1.625, 1.0, &x), -0.375, 0, 0, 0);
  check_int ("remquo (1.625, 1.0, &x) sets x to 2", x, 2, 0, 0, 0);
  check_float ("remquo (-1.625, 1.0, &x) == 0.375",  FUNC(remquo) (-1.625, 1.0, &x), 0.375, 0, 0, 0);
  check_int ("remquo (-1.625, 1.0, &x) sets x to -2", x, -2, 0, 0, 0);
  check_float ("remquo (1.625, -1.0, &x) == -0.375",  FUNC(remquo) (1.625, -1.0, &x), -0.375, 0, 0, 0);
  check_int ("remquo (1.625, -1.0, &x) sets x to -2", x, -2, 0, 0, 0);
  check_float ("remquo (-1.625, -1.0, &x) == 0.375",  FUNC(remquo) (-1.625, -1.0, &x), 0.375, 0, 0, 0);
  check_int ("remquo (-1.625, -1.0, &x) sets x to 2", x, 2, 0, 0, 0);

  check_float ("remquo (5, 2, &x) == 1",  FUNC(remquo) (5, 2, &x), 1, 0, 0, 0);
  check_int ("remquo (5, 2, &x) sets x to 2", x, 2, 0, 0, 0);
  check_float ("remquo (3, 2, &x) == -1",  FUNC(remquo) (3, 2, &x), -1, 0, 0, 0);
  check_int ("remquo (3, 2, &x) sets x to 2", x, 2, 0, 0, 0);

  print_max_error ("remquo", 0, 0);
}

static void
rint_test (void)
{
  init_max_error ();

  check_float ("rint (0.0) == 0.0",  FUNC(rint) (0.0), 0.0, 0, 0, 0);
  check_float ("rint (-0) == -0",  FUNC(rint) (minus_zero), minus_zero, 0, 0, 0);
  check_float ("rint (inf) == inf",  FUNC(rint) (plus_infty), plus_infty, 0, 0, 0);
  check_float ("rint (-inf) == -inf",  FUNC(rint) (minus_infty), minus_infty, 0, 0, 0);

  /* Default rounding mode is round to even.  */
  check_float ("rint (0.5) == 0.0",  FUNC(rint) (0.5), 0.0, 0, 0, 0);
  check_float ("rint (1.5) == 2.0",  FUNC(rint) (1.5), 2.0, 0, 0, 0);
  check_float ("rint (2.5) == 2.0",  FUNC(rint) (2.5), 2.0, 0, 0, 0);
  check_float ("rint (3.5) == 4.0",  FUNC(rint) (3.5), 4.0, 0, 0, 0);
  check_float ("rint (4.5) == 4.0",  FUNC(rint) (4.5), 4.0, 0, 0, 0);
  check_float ("rint (-0.5) == -0.0",  FUNC(rint) (-0.5), -0.0, 0, 0, 0);
  check_float ("rint (-1.5) == -2.0",  FUNC(rint) (-1.5), -2.0, 0, 0, 0);
  check_float ("rint (-2.5) == -2.0",  FUNC(rint) (-2.5), -2.0, 0, 0, 0);
  check_float ("rint (-3.5) == -4.0",  FUNC(rint) (-3.5), -4.0, 0, 0, 0);
  check_float ("rint (-4.5) == -4.0",  FUNC(rint) (-4.5), -4.0, 0, 0, 0);
  check_float ("rint (0.1) == 0.0",  FUNC(rint) (0.1), 0.0, 0, 0, 0);
  check_float ("rint (0.25) == 0.0",  FUNC(rint) (0.25), 0.0, 0, 0, 0);
  check_float ("rint (0.625) == 1.0",  FUNC(rint) (0.625), 1.0, 0, 0, 0);
  check_float ("rint (-0.1) == -0.0",  FUNC(rint) (-0.1), -0.0, 0, 0, 0);
  check_float ("rint (-0.25) == -0.0",  FUNC(rint) (-0.25), -0.0, 0, 0, 0);
  check_float ("rint (-0.625) == -1.0",  FUNC(rint) (-0.625), -1.0, 0, 0, 0);
  check_float ("rint (262144.75) == 262145.0",  FUNC(rint) (262144.75), 262145.0, 0, 0, 0);
  check_float ("rint (262142.75) == 262143.0",  FUNC(rint) (262142.75), 262143.0, 0, 0, 0);
  check_float ("rint (524286.75) == 524287.0",  FUNC(rint) (524286.75), 524287.0, 0, 0, 0);
  check_float ("rint (524288.75) == 524289.0",  FUNC(rint) (524288.75), 524289.0, 0, 0, 0);
  check_float ("rint (1048576.75) == 1048577.0",  FUNC(rint) (1048576.75), 1048577.0, 0, 0, 0);
  check_float ("rint (2097152.75) == 2097153.0",  FUNC(rint) (2097152.75), 2097153.0, 0, 0, 0);
  check_float ("rint (-1048576.75) == -1048577.0",  FUNC(rint) (-1048576.75), -1048577.0, 0, 0, 0);
  check_float ("rint (-2097152.75) == -2097153.0",  FUNC(rint) (-2097152.75), -2097153.0, 0, 0, 0);
#ifndef TEST_FLOAT
  check_float ("rint (70368744177664.75) == 70368744177665.0",  FUNC(rint) (70368744177664.75), 70368744177665.0, 0, 0, 0);
  check_float ("rint (140737488355328.75) == 140737488355329.0",  FUNC(rint) (140737488355328.75), 140737488355329.0, 0, 0, 0);
  check_float ("rint (281474976710656.75) == 281474976710657.0",  FUNC(rint) (281474976710656.75), 281474976710657.0, 0, 0, 0);
  check_float ("rint (562949953421312.75) == 562949953421313.0",  FUNC(rint) (562949953421312.75), 562949953421313.0, 0, 0, 0);
  check_float ("rint (1125899906842624.75) == 1125899906842625.0",  FUNC(rint) (1125899906842624.75), 1125899906842625.0, 0, 0, 0);
  check_float ("rint (-70368744177664.75) == -70368744177665.0",  FUNC(rint) (-70368744177664.75), -70368744177665.0, 0, 0, 0);
  check_float ("rint (-140737488355328.75) == -140737488355329.0",  FUNC(rint) (-140737488355328.75), -140737488355329.0, 0, 0, 0);
  check_float ("rint (-281474976710656.75) == -281474976710657.0",  FUNC(rint) (-281474976710656.75), -281474976710657.0, 0, 0, 0);
  check_float ("rint (-562949953421312.75) == -562949953421313.0",  FUNC(rint) (-562949953421312.75), -562949953421313.0, 0, 0, 0);
  check_float ("rint (-1125899906842624.75) == -1125899906842625.0",  FUNC(rint) (-1125899906842624.75), -1125899906842625.0, 0, 0, 0);
#endif
#ifdef TEST_LDOUBLE
  /* The result can only be represented in long double.  */
  check_float ("rint (4503599627370495.5) == 4503599627370496.0",  FUNC(rint) (4503599627370495.5L), 4503599627370496.0L, 0, 0, 0);
  check_float ("rint (4503599627370496.25) == 4503599627370496.0",  FUNC(rint) (4503599627370496.25L), 4503599627370496.0L, 0, 0, 0);
  check_float ("rint (4503599627370496.5) == 4503599627370496.0",  FUNC(rint) (4503599627370496.5L), 4503599627370496.0L, 0, 0, 0);
  check_float ("rint (4503599627370496.75) == 4503599627370497.0",  FUNC(rint) (4503599627370496.75L), 4503599627370497.0L, 0, 0, 0);
  check_float ("rint (4503599627370497.5) == 4503599627370498.0",  FUNC(rint) (4503599627370497.5L), 4503599627370498.0L, 0, 0, 0);

# if LDBL_MANT_DIG > 100
  check_float ("rint (4503599627370494.5000000000001) == 4503599627370495.0",  FUNC(rint) (4503599627370494.5000000000001L), 4503599627370495.0L, 0, 0, 0);
  check_float ("rint (4503599627370495.5000000000001) == 4503599627370496.0",  FUNC(rint) (4503599627370495.5000000000001L), 4503599627370496.0L, 0, 0, 0);
  check_float ("rint (4503599627370496.5000000000001) == 4503599627370497.0",  FUNC(rint) (4503599627370496.5000000000001L), 4503599627370497.0L, 0, 0, 0);
# endif

  check_float ("rint (-4503599627370495.5) == -4503599627370496.0",  FUNC(rint) (-4503599627370495.5L), -4503599627370496.0L, 0, 0, 0);
  check_float ("rint (-4503599627370496.25) == -4503599627370496.0",  FUNC(rint) (-4503599627370496.25L), -4503599627370496.0L, 0, 0, 0);
  check_float ("rint (-4503599627370496.5) == -4503599627370496.0",  FUNC(rint) (-4503599627370496.5L), -4503599627370496.0L, 0, 0, 0);
  check_float ("rint (-4503599627370496.75) == -4503599627370497.0",  FUNC(rint) (-4503599627370496.75L), -4503599627370497.0L, 0, 0, 0);
  check_float ("rint (-4503599627370497.5) == -4503599627370498.0",  FUNC(rint) (-4503599627370497.5L), -4503599627370498.0L, 0, 0, 0);

# if LDBL_MANT_DIG > 100
  check_float ("rint (-4503599627370494.5000000000001) == -4503599627370495.0",  FUNC(rint) (-4503599627370494.5000000000001L), -4503599627370495.0L, 0, 0, 0);
  check_float ("rint (-4503599627370495.5000000000001) == -4503599627370496.0",  FUNC(rint) (-4503599627370495.5000000000001L), -4503599627370496.0L, 0, 0, 0);
  check_float ("rint (-4503599627370496.5000000000001) == -4503599627370497.0",  FUNC(rint) (-4503599627370496.5000000000001L), -4503599627370497.0L, 0, 0, 0);

  check_float ("rint (9007199254740991.0000000000001) == 9007199254740991.0",  FUNC(rint) (9007199254740991.0000000000001L), 9007199254740991.0L, 0, 0, 0);
  check_float ("rint (9007199254740992.0000000000001) == 9007199254740992.0",  FUNC(rint) (9007199254740992.0000000000001L), 9007199254740992.0L, 0, 0, 0);
  check_float ("rint (9007199254740993.0000000000001) == 9007199254740993.0",  FUNC(rint) (9007199254740993.0000000000001L), 9007199254740993.0L, 0, 0, 0);
  check_float ("rint (9007199254740991.5000000000001) == 9007199254740992.0",  FUNC(rint) (9007199254740991.5000000000001L), 9007199254740992.0L, 0, 0, 0);
  check_float ("rint (9007199254740992.5000000000001) == 9007199254740993.0",  FUNC(rint) (9007199254740992.5000000000001L), 9007199254740993.0L, 0, 0, 0);
  check_float ("rint (9007199254740993.5000000000001) == 9007199254740994.0",  FUNC(rint) (9007199254740993.5000000000001L), 9007199254740994.0L, 0, 0, 0);

  check_float ("rint (-9007199254740991.0000000000001) == -9007199254740991.0",  FUNC(rint) (-9007199254740991.0000000000001L), -9007199254740991.0L, 0, 0, 0);
  check_float ("rint (-9007199254740992.0000000000001) == -9007199254740992.0",  FUNC(rint) (-9007199254740992.0000000000001L), -9007199254740992.0L, 0, 0, 0);
  check_float ("rint (-9007199254740993.0000000000001) == -9007199254740993.0",  FUNC(rint) (-9007199254740993.0000000000001L), -9007199254740993.0L, 0, 0, 0);
  check_float ("rint (-9007199254740991.5000000000001) == -9007199254740992.0",  FUNC(rint) (-9007199254740991.5000000000001L), -9007199254740992.0L, 0, 0, 0);
  check_float ("rint (-9007199254740992.5000000000001) == -9007199254740993.0",  FUNC(rint) (-9007199254740992.5000000000001L), -9007199254740993.0L, 0, 0, 0);
  check_float ("rint (-9007199254740993.5000000000001) == -9007199254740994.0",  FUNC(rint) (-9007199254740993.5000000000001L), -9007199254740994.0L, 0, 0, 0);
# endif

  check_float ("rint (9007199254740991.5) == 9007199254740992.0",  FUNC(rint) (9007199254740991.5L), 9007199254740992.0L, 0, 0, 0);
  check_float ("rint (9007199254740992.25) == 9007199254740992.0",  FUNC(rint) (9007199254740992.25L), 9007199254740992.0L, 0, 0, 0);
  check_float ("rint (9007199254740992.5) == 9007199254740992.0",  FUNC(rint) (9007199254740992.5L), 9007199254740992.0L, 0, 0, 0);
  check_float ("rint (9007199254740992.75) == 9007199254740993.0",  FUNC(rint) (9007199254740992.75L), 9007199254740993.0L, 0, 0, 0);
  check_float ("rint (9007199254740993.5) == 9007199254740994.0",  FUNC(rint) (9007199254740993.5L), 9007199254740994.0L, 0, 0, 0);

  check_float ("rint (-9007199254740991.5) == -9007199254740992.0",  FUNC(rint) (-9007199254740991.5L), -9007199254740992.0L, 0, 0, 0);
  check_float ("rint (-9007199254740992.25) == -9007199254740992.0",  FUNC(rint) (-9007199254740992.25L), -9007199254740992.0L, 0, 0, 0);
  check_float ("rint (-9007199254740992.5) == -9007199254740992.0",  FUNC(rint) (-9007199254740992.5L), -9007199254740992.0L, 0, 0, 0);
  check_float ("rint (-9007199254740992.75) == -9007199254740993.0",  FUNC(rint) (-9007199254740992.75L), -9007199254740993.0L, 0, 0, 0);
  check_float ("rint (-9007199254740993.5) == -9007199254740994.0",  FUNC(rint) (-9007199254740993.5L), -9007199254740994.0L, 0, 0, 0);

  check_float ("rint (72057594037927935.5) == 72057594037927936.0",  FUNC(rint) (72057594037927935.5L), 72057594037927936.0L, 0, 0, 0);
  check_float ("rint (72057594037927936.25) == 72057594037927936.0",  FUNC(rint) (72057594037927936.25L), 72057594037927936.0L, 0, 0, 0);
  check_float ("rint (72057594037927936.5) == 72057594037927936.0",  FUNC(rint) (72057594037927936.5L), 72057594037927936.0L, 0, 0, 0);
  check_float ("rint (72057594037927936.75) == 72057594037927937.0",  FUNC(rint) (72057594037927936.75L), 72057594037927937.0L, 0, 0, 0);
  check_float ("rint (72057594037927937.5) == 72057594037927938.0",  FUNC(rint) (72057594037927937.5L), 72057594037927938.0L, 0, 0, 0);

  check_float ("rint (-72057594037927935.5) == -72057594037927936.0",  FUNC(rint) (-72057594037927935.5L), -72057594037927936.0L, 0, 0, 0);
  check_float ("rint (-72057594037927936.25) == -72057594037927936.0",  FUNC(rint) (-72057594037927936.25L), -72057594037927936.0L, 0, 0, 0);
  check_float ("rint (-72057594037927936.5) == -72057594037927936.0",  FUNC(rint) (-72057594037927936.5L), -72057594037927936.0L, 0, 0, 0);
  check_float ("rint (-72057594037927936.75) == -72057594037927937.0",  FUNC(rint) (-72057594037927936.75L), -72057594037927937.0L, 0, 0, 0);
  check_float ("rint (-72057594037927937.5) == -72057594037927938.0",  FUNC(rint) (-72057594037927937.5L), -72057594037927938.0L, 0, 0, 0);

  check_float ("rint (10141204801825835211973625643007.5) == 10141204801825835211973625643008.0",  FUNC(rint) (10141204801825835211973625643007.5L), 10141204801825835211973625643008.0L, 0, 0, 0);
  check_float ("rint (10141204801825835211973625643008.25) == 10141204801825835211973625643008.0",  FUNC(rint) (10141204801825835211973625643008.25L), 10141204801825835211973625643008.0L, 0, 0, 0);
  check_float ("rint (10141204801825835211973625643008.5) == 10141204801825835211973625643008.0",  FUNC(rint) (10141204801825835211973625643008.5L), 10141204801825835211973625643008.0L, 0, 0, 0);
  check_float ("rint (10141204801825835211973625643008.75) == 10141204801825835211973625643009.0",  FUNC(rint) (10141204801825835211973625643008.75L), 10141204801825835211973625643009.0L, 0, 0, 0);
  check_float ("rint (10141204801825835211973625643009.5) == 10141204801825835211973625643010.0",  FUNC(rint) (10141204801825835211973625643009.5L), 10141204801825835211973625643010.0L, 0, 0, 0);
#endif

  print_max_error ("rint", 0, 0);
}

static void
rint_test_tonearest (void)
{
  int save_round_mode;
  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_TONEAREST))
    {
  check_float ("rint_tonearest (2.0) == 2.0",  FUNC(rint) (2.0), 2.0, 0, 0, 0);
  check_float ("rint_tonearest (1.5) == 2.0",  FUNC(rint) (1.5), 2.0, 0, 0, 0);
  check_float ("rint_tonearest (1.0) == 1.0",  FUNC(rint) (1.0), 1.0, 0, 0, 0);
  check_float ("rint_tonearest (0.5) == 0.0",  FUNC(rint) (0.5), 0.0, 0, 0, 0);
  check_float ("rint_tonearest (0.0) == 0.0",  FUNC(rint) (0.0), 0.0, 0, 0, 0);
  check_float ("rint_tonearest (-0) == -0",  FUNC(rint) (minus_zero), minus_zero, 0, 0, 0);
  check_float ("rint_tonearest (-0.5) == -0.0",  FUNC(rint) (-0.5), -0.0, 0, 0, 0);
  check_float ("rint_tonearest (-1.0) == -1.0",  FUNC(rint) (-1.0), -1.0, 0, 0, 0);
  check_float ("rint_tonearest (-1.5) == -2.0",  FUNC(rint) (-1.5), -2.0, 0, 0, 0);
  check_float ("rint_tonearest (-2.0) == -2.0",  FUNC(rint) (-2.0), -2.0, 0, 0, 0);
  check_float ("rint_tonearest (0.1) == 0.0",  FUNC(rint) (0.1), 0.0, 0, 0, 0);
  check_float ("rint_tonearest (0.25) == 0.0",  FUNC(rint) (0.25), 0.0, 0, 0, 0);
  check_float ("rint_tonearest (0.625) == 1.0",  FUNC(rint) (0.625), 1.0, 0, 0, 0);
  check_float ("rint_tonearest (-0.1) == -0.0",  FUNC(rint) (-0.1), -0.0, 0, 0, 0);
  check_float ("rint_tonearest (-0.25) == -0.0",  FUNC(rint) (-0.25), -0.0, 0, 0, 0);
  check_float ("rint_tonearest (-0.625) == -1.0",  FUNC(rint) (-0.625), -1.0, 0, 0, 0);
  check_float ("rint_tonearest (1048576.75) == 1048577.0",  FUNC(rint) (1048576.75), 1048577.0, 0, 0, 0);
  check_float ("rint_tonearest (2097152.75) == 2097153.0",  FUNC(rint) (2097152.75), 2097153.0, 0, 0, 0);
  check_float ("rint_tonearest (-1048576.75) == -1048577.0",  FUNC(rint) (-1048576.75), -1048577.0, 0, 0, 0);
  check_float ("rint_tonearest (-2097152.75) == -2097153.0",  FUNC(rint) (-2097152.75), -2097153.0, 0, 0, 0);
#ifndef TEST_FLOAT
  check_float ("rint_tonearest (70368744177664.75) == 70368744177665.0",  FUNC(rint) (70368744177664.75), 70368744177665.0, 0, 0, 0);
  check_float ("rint_tonearest (140737488355328.75) == 140737488355329.0",  FUNC(rint) (140737488355328.75), 140737488355329.0, 0, 0, 0);
  check_float ("rint_tonearest (281474976710656.75) == 281474976710657.0",  FUNC(rint) (281474976710656.75), 281474976710657.0, 0, 0, 0);
  check_float ("rint_tonearest (562949953421312.75) == 562949953421313.0",  FUNC(rint) (562949953421312.75), 562949953421313.0, 0, 0, 0);
  check_float ("rint_tonearest (1125899906842624.75) == 1125899906842625.0",  FUNC(rint) (1125899906842624.75), 1125899906842625.0, 0, 0, 0);
  check_float ("rint_tonearest (-70368744177664.75) == -70368744177665.0",  FUNC(rint) (-70368744177664.75), -70368744177665.0, 0, 0, 0);
  check_float ("rint_tonearest (-140737488355328.75) == -140737488355329.0",  FUNC(rint) (-140737488355328.75), -140737488355329.0, 0, 0, 0);
  check_float ("rint_tonearest (-281474976710656.75) == -281474976710657.0",  FUNC(rint) (-281474976710656.75), -281474976710657.0, 0, 0, 0);
  check_float ("rint_tonearest (-562949953421312.75) == -562949953421313.0",  FUNC(rint) (-562949953421312.75), -562949953421313.0, 0, 0, 0);
  check_float ("rint_tonearest (-1125899906842624.75) == -1125899906842625.0",  FUNC(rint) (-1125899906842624.75), -1125899906842625.0, 0, 0, 0);
#endif
#ifdef TEST_LDOUBLE
      /* The result can only be represented in long double.  */
  check_float ("rint_tonearest (4503599627370495.5) == 4503599627370496.0",  FUNC(rint) (4503599627370495.5L), 4503599627370496.0L, 0, 0, 0);
  check_float ("rint_tonearest (4503599627370496.25) == 4503599627370496.0",  FUNC(rint) (4503599627370496.25L), 4503599627370496.0L, 0, 0, 0);
  check_float ("rint_tonearest (4503599627370496.5) == 4503599627370496.0",  FUNC(rint) (4503599627370496.5L), 4503599627370496.0L, 0, 0, 0);
  check_float ("rint_tonearest (4503599627370496.75) == 4503599627370497.0",  FUNC(rint) (4503599627370496.75L), 4503599627370497.0L, 0, 0, 0);
  check_float ("rint_tonearest (4503599627370497.5) == 4503599627370498.0",  FUNC(rint) (4503599627370497.5L), 4503599627370498.0L, 0, 0, 0);
# if LDBL_MANT_DIG > 100
  check_float ("rint_tonearest (4503599627370494.5000000000001) == 4503599627370495.0",  FUNC(rint) (4503599627370494.5000000000001L), 4503599627370495.0L, 0, 0, 0);
  check_float ("rint_tonearest (4503599627370495.5000000000001) == 4503599627370496.0",  FUNC(rint) (4503599627370495.5000000000001L), 4503599627370496.0L, 0, 0, 0);
  check_float ("rint_tonearest (4503599627370496.5000000000001) == 4503599627370497.0",  FUNC(rint) (4503599627370496.5000000000001L), 4503599627370497.0L, 0, 0, 0);
# endif
  check_float ("rint_tonearest (-4503599627370495.5) == -4503599627370496.0",  FUNC(rint) (-4503599627370495.5L), -4503599627370496.0L, 0, 0, 0);
  check_float ("rint_tonearest (-4503599627370496.25) == -4503599627370496.0",  FUNC(rint) (-4503599627370496.25L), -4503599627370496.0L, 0, 0, 0);
  check_float ("rint_tonearest (-4503599627370496.5) == -4503599627370496.0",  FUNC(rint) (-4503599627370496.5L), -4503599627370496.0L, 0, 0, 0);
  check_float ("rint_tonearest (-4503599627370496.75) == -4503599627370497.0",  FUNC(rint) (-4503599627370496.75L), -4503599627370497.0L, 0, 0, 0);
  check_float ("rint_tonearest (-4503599627370497.5) == -4503599627370498.0",  FUNC(rint) (-4503599627370497.5L), -4503599627370498.0L, 0, 0, 0);
# if LDBL_MANT_DIG > 100
  check_float ("rint_tonearest (-4503599627370494.5000000000001) == -4503599627370495.0",  FUNC(rint) (-4503599627370494.5000000000001L), -4503599627370495.0L, 0, 0, 0);
  check_float ("rint_tonearest (-4503599627370495.5000000000001) == -4503599627370496.0",  FUNC(rint) (-4503599627370495.5000000000001L), -4503599627370496.0L, 0, 0, 0);
  check_float ("rint_tonearest (-4503599627370496.5000000000001) == -4503599627370497.0",  FUNC(rint) (-4503599627370496.5000000000001L), -4503599627370497.0L, 0, 0, 0);

  check_float ("rint_tonearest (9007199254740991.0000000000001) == 9007199254740991.0",  FUNC(rint) (9007199254740991.0000000000001L), 9007199254740991.0L, 0, 0, 0);
  check_float ("rint_tonearest (9007199254740992.0000000000001) == 9007199254740992.0",  FUNC(rint) (9007199254740992.0000000000001L), 9007199254740992.0L, 0, 0, 0);
  check_float ("rint_tonearest (9007199254740993.0000000000001) == 9007199254740993.0",  FUNC(rint) (9007199254740993.0000000000001L), 9007199254740993.0L, 0, 0, 0);
  check_float ("rint_tonearest (9007199254740991.5000000000001) == 9007199254740992.0",  FUNC(rint) (9007199254740991.5000000000001L), 9007199254740992.0L, 0, 0, 0);
  check_float ("rint_tonearest (9007199254740992.5000000000001) == 9007199254740993.0",  FUNC(rint) (9007199254740992.5000000000001L), 9007199254740993.0L, 0, 0, 0);
  check_float ("rint_tonearest (9007199254740993.5000000000001) == 9007199254740994.0",  FUNC(rint) (9007199254740993.5000000000001L), 9007199254740994.0L, 0, 0, 0);

  check_float ("rint_tonearest (-9007199254740991.0000000000001) == -9007199254740991.0",  FUNC(rint) (-9007199254740991.0000000000001L), -9007199254740991.0L, 0, 0, 0);
  check_float ("rint_tonearest (-9007199254740992.0000000000001) == -9007199254740992.0",  FUNC(rint) (-9007199254740992.0000000000001L), -9007199254740992.0L, 0, 0, 0);
  check_float ("rint_tonearest (-9007199254740993.0000000000001) == -9007199254740993.0",  FUNC(rint) (-9007199254740993.0000000000001L), -9007199254740993.0L, 0, 0, 0);
  check_float ("rint_tonearest (-9007199254740991.5000000000001) == -9007199254740992.0",  FUNC(rint) (-9007199254740991.5000000000001L), -9007199254740992.0L, 0, 0, 0);
  check_float ("rint_tonearest (-9007199254740992.5000000000001) == -9007199254740993.0",  FUNC(rint) (-9007199254740992.5000000000001L), -9007199254740993.0L, 0, 0, 0);
  check_float ("rint_tonearest (-9007199254740993.5000000000001) == -9007199254740994.0",  FUNC(rint) (-9007199254740993.5000000000001L), -9007199254740994.0L, 0, 0, 0);
# endif
#endif
    }

  fesetround (save_round_mode);

  print_max_error ("rint_tonearest", 0, 0);
}

static void
rint_test_towardzero (void)
{
  int save_round_mode;
  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_TOWARDZERO))
    {
  check_float ("rint_towardzero (2.0) == 2.0",  FUNC(rint) (2.0), 2.0, 0, 0, 0);
  check_float ("rint_towardzero (1.5) == 1.0",  FUNC(rint) (1.5), 1.0, 0, 0, 0);
  check_float ("rint_towardzero (1.0) == 1.0",  FUNC(rint) (1.0), 1.0, 0, 0, 0);
  check_float ("rint_towardzero (0.5) == 0.0",  FUNC(rint) (0.5), 0.0, 0, 0, 0);
  check_float ("rint_towardzero (0.0) == 0.0",  FUNC(rint) (0.0), 0.0, 0, 0, 0);
  check_float ("rint_towardzero (-0) == -0",  FUNC(rint) (minus_zero), minus_zero, 0, 0, 0);
  check_float ("rint_towardzero (-0.5) == -0.0",  FUNC(rint) (-0.5), -0.0, 0, 0, 0);
  check_float ("rint_towardzero (-1.0) == -1.0",  FUNC(rint) (-1.0), -1.0, 0, 0, 0);
  check_float ("rint_towardzero (-1.5) == -1.0",  FUNC(rint) (-1.5), -1.0, 0, 0, 0);
  check_float ("rint_towardzero (-2.0) == -2.0",  FUNC(rint) (-2.0), -2.0, 0, 0, 0);
  check_float ("rint_towardzero (0.1) == 0.0",  FUNC(rint) (0.1), 0.0, 0, 0, 0);
  check_float ("rint_towardzero (0.25) == 0.0",  FUNC(rint) (0.25), 0.0, 0, 0, 0);
  check_float ("rint_towardzero (0.625) == 0.0",  FUNC(rint) (0.625), 0.0, 0, 0, 0);
  check_float ("rint_towardzero (-0.1) == -0.0",  FUNC(rint) (-0.1), -0.0, 0, 0, 0);
  check_float ("rint_towardzero (-0.25) == -0.0",  FUNC(rint) (-0.25), -0.0, 0, 0, 0);
  check_float ("rint_towardzero (-0.625) == -0.0",  FUNC(rint) (-0.625), -0.0, 0, 0, 0);
  check_float ("rint_towardzero (1048576.75) == 1048576.0",  FUNC(rint) (1048576.75), 1048576.0, 0, 0, 0);
  check_float ("rint_towardzero (2097152.75) == 2097152.0",  FUNC(rint) (2097152.75), 2097152.0, 0, 0, 0);
  check_float ("rint_towardzero (-1048576.75) == -1048576.0",  FUNC(rint) (-1048576.75), -1048576.0, 0, 0, 0);
  check_float ("rint_towardzero (-2097152.75) == -2097152.0",  FUNC(rint) (-2097152.75), -2097152.0, 0, 0, 0);
#ifndef TEST_FLOAT
  check_float ("rint_towardzero (70368744177664.75) == 70368744177664.0",  FUNC(rint) (70368744177664.75), 70368744177664.0, 0, 0, 0);
  check_float ("rint_towardzero (140737488355328.75) == 140737488355328.0",  FUNC(rint) (140737488355328.75), 140737488355328.0, 0, 0, 0);
  check_float ("rint_towardzero (281474976710656.75) == 281474976710656.0",  FUNC(rint) (281474976710656.75), 281474976710656.0, 0, 0, 0);
  check_float ("rint_towardzero (562949953421312.75) == 562949953421312.0",  FUNC(rint) (562949953421312.75), 562949953421312.0, 0, 0, 0);
  check_float ("rint_towardzero (1125899906842624.75) == 1125899906842624.0",  FUNC(rint) (1125899906842624.75), 1125899906842624.0, 0, 0, 0);
  check_float ("rint_towardzero (-70368744177664.75) == -70368744177664.0",  FUNC(rint) (-70368744177664.75), -70368744177664.0, 0, 0, 0);
  check_float ("rint_towardzero (-140737488355328.75) == -140737488355328.0",  FUNC(rint) (-140737488355328.75), -140737488355328.0, 0, 0, 0);
  check_float ("rint_towardzero (-281474976710656.75) == -281474976710656.0",  FUNC(rint) (-281474976710656.75), -281474976710656.0, 0, 0, 0);
  check_float ("rint_towardzero (-562949953421312.75) == -562949953421312.0",  FUNC(rint) (-562949953421312.75), -562949953421312.0, 0, 0, 0);
  check_float ("rint_towardzero (-1125899906842624.75) == -1125899906842624.0",  FUNC(rint) (-1125899906842624.75), -1125899906842624.0, 0, 0, 0);
#endif
#ifdef TEST_LDOUBLE
      /* The result can only be represented in long double.  */
  check_float ("rint_towardzero (4503599627370495.5) == 4503599627370495.0",  FUNC(rint) (4503599627370495.5L), 4503599627370495.0L, 0, 0, 0);
  check_float ("rint_towardzero (4503599627370496.25) == 4503599627370496.0",  FUNC(rint) (4503599627370496.25L), 4503599627370496.0L, 0, 0, 0);
  check_float ("rint_towardzero (4503599627370496.5) == 4503599627370496.0",  FUNC(rint) (4503599627370496.5L), 4503599627370496.0L, 0, 0, 0);
  check_float ("rint_towardzero (4503599627370496.75) == 4503599627370496.0",  FUNC(rint) (4503599627370496.75L), 4503599627370496.0L, 0, 0, 0);
  check_float ("rint_towardzero (4503599627370497.5) == 4503599627370497.0",  FUNC(rint) (4503599627370497.5L), 4503599627370497.0L, 0, 0, 0);
# if LDBL_MANT_DIG > 100
  check_float ("rint_towardzero (4503599627370494.5000000000001) == 4503599627370494.0",  FUNC(rint) (4503599627370494.5000000000001L), 4503599627370494.0L, 0, 0, 0);
  check_float ("rint_towardzero (4503599627370495.5000000000001) == 4503599627370495.0",  FUNC(rint) (4503599627370495.5000000000001L), 4503599627370495.0L, 0, 0, 0);
  check_float ("rint_towardzero (4503599627370496.5000000000001) == 4503599627370496.0",  FUNC(rint) (4503599627370496.5000000000001L), 4503599627370496.0L, 0, 0, 0);
# endif
  check_float ("rint_towardzero (-4503599627370495.5) == -4503599627370495.0",  FUNC(rint) (-4503599627370495.5L), -4503599627370495.0L, 0, 0, 0);
  check_float ("rint_towardzero (-4503599627370496.25) == -4503599627370496.0",  FUNC(rint) (-4503599627370496.25L), -4503599627370496.0L, 0, 0, 0);
  check_float ("rint_towardzero (-4503599627370496.5) == -4503599627370496.0",  FUNC(rint) (-4503599627370496.5L), -4503599627370496.0L, 0, 0, 0);
  check_float ("rint_towardzero (-4503599627370496.75) == -4503599627370496.0",  FUNC(rint) (-4503599627370496.75L), -4503599627370496.0L, 0, 0, 0);
  check_float ("rint_towardzero (-4503599627370497.5) == -4503599627370497.0",  FUNC(rint) (-4503599627370497.5L), -4503599627370497.0L, 0, 0, 0);
# if LDBL_MANT_DIG > 100
  check_float ("rint_towardzero (-4503599627370494.5000000000001) == -4503599627370494.0",  FUNC(rint) (-4503599627370494.5000000000001L), -4503599627370494.0L, 0, 0, 0);
  check_float ("rint_towardzero (-4503599627370495.5000000000001) == -4503599627370495.0",  FUNC(rint) (-4503599627370495.5000000000001L), -4503599627370495.0L, 0, 0, 0);
  check_float ("rint_towardzero (-4503599627370496.5000000000001) == -4503599627370496.0",  FUNC(rint) (-4503599627370496.5000000000001L), -4503599627370496.0L, 0, 0, 0);

  check_float ("rint_towardzero (9007199254740991.0000000000001) == 9007199254740991.0",  FUNC(rint) (9007199254740991.0000000000001L), 9007199254740991.0L, 0, 0, 0);
  check_float ("rint_towardzero (9007199254740992.0000000000001) == 9007199254740992.0",  FUNC(rint) (9007199254740992.0000000000001L), 9007199254740992.0L, 0, 0, 0);
  check_float ("rint_towardzero (9007199254740993.0000000000001) == 9007199254740993.0",  FUNC(rint) (9007199254740993.0000000000001L), 9007199254740993.0L, 0, 0, 0);
  check_float ("rint_towardzero (9007199254740991.5000000000001) == 9007199254740991.0",  FUNC(rint) (9007199254740991.5000000000001L), 9007199254740991.0L, 0, 0, 0);
  check_float ("rint_towardzero (9007199254740992.5000000000001) == 9007199254740992.0",  FUNC(rint) (9007199254740992.5000000000001L), 9007199254740992.0L, 0, 0, 0);
  check_float ("rint_towardzero (9007199254740993.5000000000001) == 9007199254740993.0",  FUNC(rint) (9007199254740993.5000000000001L), 9007199254740993.0L, 0, 0, 0);

  check_float ("rint_towardzero (-9007199254740991.0000000000001) == -9007199254740991.0",  FUNC(rint) (-9007199254740991.0000000000001L), -9007199254740991.0L, 0, 0, 0);
  check_float ("rint_towardzero (-9007199254740992.0000000000001) == -9007199254740992.0",  FUNC(rint) (-9007199254740992.0000000000001L), -9007199254740992.0L, 0, 0, 0);
  check_float ("rint_towardzero (-9007199254740993.0000000000001) == -9007199254740993.0",  FUNC(rint) (-9007199254740993.0000000000001L), -9007199254740993.0L, 0, 0, 0);
  check_float ("rint_towardzero (-9007199254740991.5000000000001) == -9007199254740991.0",  FUNC(rint) (-9007199254740991.5000000000001L), -9007199254740991.0L, 0, 0, 0);
  check_float ("rint_towardzero (-9007199254740992.5000000000001) == -9007199254740992.0",  FUNC(rint) (-9007199254740992.5000000000001L), -9007199254740992.0L, 0, 0, 0);
  check_float ("rint_towardzero (-9007199254740993.5000000000001) == -9007199254740993.0",  FUNC(rint) (-9007199254740993.5000000000001L), -9007199254740993.0L, 0, 0, 0);
# endif
#endif
    }

  fesetround (save_round_mode);

  print_max_error ("rint_towardzero", 0, 0);
}

static void
rint_test_downward (void)
{
  int save_round_mode;
  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_DOWNWARD))
    {
  check_float ("rint_downward (2.0) == 2.0",  FUNC(rint) (2.0), 2.0, 0, 0, 0);
  check_float ("rint_downward (1.5) == 1.0",  FUNC(rint) (1.5), 1.0, 0, 0, 0);
  check_float ("rint_downward (1.0) == 1.0",  FUNC(rint) (1.0), 1.0, 0, 0, 0);
  check_float ("rint_downward (0.5) == 0.0",  FUNC(rint) (0.5), 0.0, 0, 0, 0);
  check_float ("rint_downward (0.0) == 0.0",  FUNC(rint) (0.0), 0.0, 0, 0, 0);
  check_float ("rint_downward (-0) == -0",  FUNC(rint) (minus_zero), minus_zero, 0, 0, 0);
  check_float ("rint_downward (-0.5) == -1.0",  FUNC(rint) (-0.5), -1.0, 0, 0, 0);
  check_float ("rint_downward (-1.0) == -1.0",  FUNC(rint) (-1.0), -1.0, 0, 0, 0);
  check_float ("rint_downward (-1.5) == -2.0",  FUNC(rint) (-1.5), -2.0, 0, 0, 0);
  check_float ("rint_downward (-2.0) == -2.0",  FUNC(rint) (-2.0), -2.0, 0, 0, 0);
  check_float ("rint_downward (0.1) == 0.0",  FUNC(rint) (0.1), 0.0, 0, 0, 0);
  check_float ("rint_downward (0.25) == 0.0",  FUNC(rint) (0.25), 0.0, 0, 0, 0);
  check_float ("rint_downward (0.625) == 0.0",  FUNC(rint) (0.625), 0.0, 0, 0, 0);
  check_float ("rint_downward (-0.1) == -1.0",  FUNC(rint) (-0.1), -1.0, 0, 0, 0);
  check_float ("rint_downward (-0.25) == -1.0",  FUNC(rint) (-0.25), -1.0, 0, 0, 0);
  check_float ("rint_downward (-0.625) == -1.0",  FUNC(rint) (-0.625), -1.0, 0, 0, 0);
  check_float ("rint_downward (1048576.75) == 1048576.0",  FUNC(rint) (1048576.75), 1048576.0, 0, 0, 0);
  check_float ("rint_downward (2097152.75) == 2097152.0",  FUNC(rint) (2097152.75), 2097152.0, 0, 0, 0);
  check_float ("rint_downward (-1048576.75) == -1048577.0",  FUNC(rint) (-1048576.75), -1048577.0, 0, 0, 0);
  check_float ("rint_downward (-2097152.75) == -2097153.0",  FUNC(rint) (-2097152.75), -2097153.0, 0, 0, 0);
#ifndef TEST_FLOAT
  check_float ("rint_downward (70368744177664.75) == 70368744177664.0",  FUNC(rint) (70368744177664.75), 70368744177664.0, 0, 0, 0);
  check_float ("rint_downward (140737488355328.75) == 140737488355328.0",  FUNC(rint) (140737488355328.75), 140737488355328.0, 0, 0, 0);
  check_float ("rint_downward (281474976710656.75) == 281474976710656.0",  FUNC(rint) (281474976710656.75), 281474976710656.0, 0, 0, 0);
  check_float ("rint_downward (562949953421312.75) == 562949953421312.0",  FUNC(rint) (562949953421312.75), 562949953421312.0, 0, 0, 0);
  check_float ("rint_downward (1125899906842624.75) == 1125899906842624.0",  FUNC(rint) (1125899906842624.75), 1125899906842624.0, 0, 0, 0);
  check_float ("rint_downward (-70368744177664.75) == -70368744177665.0",  FUNC(rint) (-70368744177664.75), -70368744177665.0, 0, 0, 0);
  check_float ("rint_downward (-140737488355328.75) == -140737488355329.0",  FUNC(rint) (-140737488355328.75), -140737488355329.0, 0, 0, 0);
  check_float ("rint_downward (-281474976710656.75) == -281474976710657.0",  FUNC(rint) (-281474976710656.75), -281474976710657.0, 0, 0, 0);
  check_float ("rint_downward (-562949953421312.75) == -562949953421313.0",  FUNC(rint) (-562949953421312.75), -562949953421313.0, 0, 0, 0);
  check_float ("rint_downward (-1125899906842624.75) == -1125899906842625.0",  FUNC(rint) (-1125899906842624.75), -1125899906842625.0, 0, 0, 0);
#endif
#ifdef TEST_LDOUBLE
      /* The result can only be represented in long double.  */
  check_float ("rint_downward (4503599627370495.5) == 4503599627370495.0",  FUNC(rint) (4503599627370495.5L), 4503599627370495.0L, 0, 0, 0);
  check_float ("rint_downward (4503599627370496.25) == 4503599627370496.0",  FUNC(rint) (4503599627370496.25L), 4503599627370496.0L, 0, 0, 0);
  check_float ("rint_downward (4503599627370496.5) == 4503599627370496.0",  FUNC(rint) (4503599627370496.5L), 4503599627370496.0L, 0, 0, 0);
  check_float ("rint_downward (4503599627370496.75) == 4503599627370496.0",  FUNC(rint) (4503599627370496.75L), 4503599627370496.0L, 0, 0, 0);
  check_float ("rint_downward (4503599627370497.5) == 4503599627370497.0",  FUNC(rint) (4503599627370497.5L), 4503599627370497.0L, 0, 0, 0);
# if LDBL_MANT_DIG > 100
  check_float ("rint_downward (4503599627370494.5000000000001) == 4503599627370494.0",  FUNC(rint) (4503599627370494.5000000000001L), 4503599627370494.0L, 0, 0, 0);
  check_float ("rint_downward (4503599627370495.5000000000001) == 4503599627370495.0",  FUNC(rint) (4503599627370495.5000000000001L), 4503599627370495.0L, 0, 0, 0);
  check_float ("rint_downward (4503599627370496.5000000000001) == 4503599627370496.0",  FUNC(rint) (4503599627370496.5000000000001L), 4503599627370496.0L, 0, 0, 0);
# endif
  check_float ("rint_downward (-4503599627370495.5) == -4503599627370496.0",  FUNC(rint) (-4503599627370495.5L), -4503599627370496.0L, 0, 0, 0);
  check_float ("rint_downward (-4503599627370496.25) == -4503599627370497.0",  FUNC(rint) (-4503599627370496.25L), -4503599627370497.0L, 0, 0, 0);
  check_float ("rint_downward (-4503599627370496.5) == -4503599627370497.0",  FUNC(rint) (-4503599627370496.5L), -4503599627370497.0L, 0, 0, 0);
  check_float ("rint_downward (-4503599627370496.75) == -4503599627370497.0",  FUNC(rint) (-4503599627370496.75L), -4503599627370497.0L, 0, 0, 0);
  check_float ("rint_downward (-4503599627370497.5) == -4503599627370498.0",  FUNC(rint) (-4503599627370497.5L), -4503599627370498.0L, 0, 0, 0);
# if LDBL_MANT_DIG > 100
  check_float ("rint_downward (-4503599627370494.5000000000001) == -4503599627370495.0",  FUNC(rint) (-4503599627370494.5000000000001L), -4503599627370495.0L, 0, 0, 0);
  check_float ("rint_downward (-4503599627370495.5000000000001) == -4503599627370496.0",  FUNC(rint) (-4503599627370495.5000000000001L), -4503599627370496.0L, 0, 0, 0);
  check_float ("rint_downward (-4503599627370496.5000000000001) == -4503599627370497.0",  FUNC(rint) (-4503599627370496.5000000000001L), -4503599627370497.0L, 0, 0, 0);

  check_float ("rint_downward (9007199254740991.0000000000001) == 9007199254740991.0",  FUNC(rint) (9007199254740991.0000000000001L), 9007199254740991.0L, 0, 0, 0);
  check_float ("rint_downward (9007199254740992.0000000000001) == 9007199254740992.0",  FUNC(rint) (9007199254740992.0000000000001L), 9007199254740992.0L, 0, 0, 0);
  check_float ("rint_downward (9007199254740993.0000000000001) == 9007199254740993.0",  FUNC(rint) (9007199254740993.0000000000001L), 9007199254740993.0L, 0, 0, 0);
  check_float ("rint_downward (9007199254740991.5000000000001) == 9007199254740991.0",  FUNC(rint) (9007199254740991.5000000000001L), 9007199254740991.0L, 0, 0, 0);
  check_float ("rint_downward (9007199254740992.5000000000001) == 9007199254740992.0",  FUNC(rint) (9007199254740992.5000000000001L), 9007199254740992.0L, 0, 0, 0);
  check_float ("rint_downward (9007199254740993.5000000000001) == 9007199254740993.0",  FUNC(rint) (9007199254740993.5000000000001L), 9007199254740993.0L, 0, 0, 0);

  check_float ("rint_downward (-9007199254740991.0000000000001) == -9007199254740992.0",  FUNC(rint) (-9007199254740991.0000000000001L), -9007199254740992.0L, 0, 0, 0);
  check_float ("rint_downward (-9007199254740992.0000000000001) == -9007199254740993.0",  FUNC(rint) (-9007199254740992.0000000000001L), -9007199254740993.0L, 0, 0, 0);
  check_float ("rint_downward (-9007199254740993.0000000000001) == -9007199254740994.0",  FUNC(rint) (-9007199254740993.0000000000001L), -9007199254740994.0L, 0, 0, 0);
  check_float ("rint_downward (-9007199254740991.5000000000001) == -9007199254740992.0",  FUNC(rint) (-9007199254740991.5000000000001L), -9007199254740992.0L, 0, 0, 0);
  check_float ("rint_downward (-9007199254740992.5000000000001) == -9007199254740993.0",  FUNC(rint) (-9007199254740992.5000000000001L), -9007199254740993.0L, 0, 0, 0);
  check_float ("rint_downward (-9007199254740993.5000000000001) == -9007199254740994.0",  FUNC(rint) (-9007199254740993.5000000000001L), -9007199254740994.0L, 0, 0, 0);
# endif
#endif
    }

  fesetround (save_round_mode);

  print_max_error ("rint_downward", 0, 0);
}

static void
rint_test_upward (void)
{
  int save_round_mode;
  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_UPWARD))
    {
  check_float ("rint_upward (2.0) == 2.0",  FUNC(rint) (2.0), 2.0, 0, 0, 0);
  check_float ("rint_upward (1.5) == 2.0",  FUNC(rint) (1.5), 2.0, 0, 0, 0);
  check_float ("rint_upward (1.0) == 1.0",  FUNC(rint) (1.0), 1.0, 0, 0, 0);
  check_float ("rint_upward (0.5) == 1.0",  FUNC(rint) (0.5), 1.0, 0, 0, 0);
  check_float ("rint_upward (0.0) == 0.0",  FUNC(rint) (0.0), 0.0, 0, 0, 0);
  check_float ("rint_upward (-0) == -0",  FUNC(rint) (minus_zero), minus_zero, 0, 0, 0);
  check_float ("rint_upward (-0.5) == -0.0",  FUNC(rint) (-0.5), -0.0, 0, 0, 0);
  check_float ("rint_upward (-1.0) == -1.0",  FUNC(rint) (-1.0), -1.0, 0, 0, 0);
  check_float ("rint_upward (-1.5) == -1.0",  FUNC(rint) (-1.5), -1.0, 0, 0, 0);
  check_float ("rint_upward (-2.0) == -2.0",  FUNC(rint) (-2.0), -2.0, 0, 0, 0);
  check_float ("rint_upward (0.1) == 1.0",  FUNC(rint) (0.1), 1.0, 0, 0, 0);
  check_float ("rint_upward (0.25) == 1.0",  FUNC(rint) (0.25), 1.0, 0, 0, 0);
  check_float ("rint_upward (0.625) == 1.0",  FUNC(rint) (0.625), 1.0, 0, 0, 0);
  check_float ("rint_upward (-0.1) == -0.0",  FUNC(rint) (-0.1), -0.0, 0, 0, 0);
  check_float ("rint_upward (-0.25) == -0.0",  FUNC(rint) (-0.25), -0.0, 0, 0, 0);
  check_float ("rint_upward (-0.625) == -0.0",  FUNC(rint) (-0.625), -0.0, 0, 0, 0);
  check_float ("rint_upward (1048576.75) == 1048577.0",  FUNC(rint) (1048576.75), 1048577.0, 0, 0, 0);
  check_float ("rint_upward (2097152.75) == 2097153.0",  FUNC(rint) (2097152.75), 2097153.0, 0, 0, 0);
  check_float ("rint_upward (-1048576.75) == -1048576.0",  FUNC(rint) (-1048576.75), -1048576.0, 0, 0, 0);
  check_float ("rint_upward (-2097152.75) == -2097152.0",  FUNC(rint) (-2097152.75), -2097152.0, 0, 0, 0);
#ifndef TEST_FLOAT
  check_float ("rint_upward (70368744177664.75) == 70368744177665.0",  FUNC(rint) (70368744177664.75), 70368744177665.0, 0, 0, 0);
  check_float ("rint_upward (140737488355328.75) == 140737488355329.0",  FUNC(rint) (140737488355328.75), 140737488355329.0, 0, 0, 0);
  check_float ("rint_upward (281474976710656.75) == 281474976710657.0",  FUNC(rint) (281474976710656.75), 281474976710657.0, 0, 0, 0);
  check_float ("rint_upward (562949953421312.75) == 562949953421313.0",  FUNC(rint) (562949953421312.75), 562949953421313.0, 0, 0, 0);
  check_float ("rint_upward (1125899906842624.75) == 1125899906842625.0",  FUNC(rint) (1125899906842624.75), 1125899906842625.0, 0, 0, 0);
  check_float ("rint_upward (-70368744177664.75) == -70368744177664.0",  FUNC(rint) (-70368744177664.75), -70368744177664.0, 0, 0, 0);
  check_float ("rint_upward (-140737488355328.75) == -140737488355328.0",  FUNC(rint) (-140737488355328.75), -140737488355328.0, 0, 0, 0);
  check_float ("rint_upward (-281474976710656.75) == -281474976710656.0",  FUNC(rint) (-281474976710656.75), -281474976710656.0, 0, 0, 0);
  check_float ("rint_upward (-562949953421312.75) == -562949953421312.0",  FUNC(rint) (-562949953421312.75), -562949953421312.0, 0, 0, 0);
  check_float ("rint_upward (-1125899906842624.75) == -1125899906842624.0",  FUNC(rint) (-1125899906842624.75), -1125899906842624.0, 0, 0, 0);
#endif
#ifdef TEST_LDOUBLE
      /* The result can only be represented in long double.  */
  check_float ("rint_upward (4503599627370495.5) == 4503599627370496.0",  FUNC(rint) (4503599627370495.5L), 4503599627370496.0L, 0, 0, 0);
  check_float ("rint_upward (4503599627370496.25) == 4503599627370497.0",  FUNC(rint) (4503599627370496.25L), 4503599627370497.0L, 0, 0, 0);
  check_float ("rint_upward (4503599627370496.5) == 4503599627370497.0",  FUNC(rint) (4503599627370496.5L), 4503599627370497.0L, 0, 0, 0);
  check_float ("rint_upward (4503599627370496.75) == 4503599627370497.0",  FUNC(rint) (4503599627370496.75L), 4503599627370497.0L, 0, 0, 0);
  check_float ("rint_upward (4503599627370497.5) == 4503599627370498.0",  FUNC(rint) (4503599627370497.5L), 4503599627370498.0L, 0, 0, 0);
# if LDBL_MANT_DIG > 100
  check_float ("rint_upward (4503599627370494.5000000000001) == 4503599627370495.0",  FUNC(rint) (4503599627370494.5000000000001L), 4503599627370495.0L, 0, 0, 0);
  check_float ("rint_upward (4503599627370495.5000000000001) == 4503599627370496.0",  FUNC(rint) (4503599627370495.5000000000001L), 4503599627370496.0L, 0, 0, 0);
  check_float ("rint_upward (4503599627370496.5000000000001) == 4503599627370497.0",  FUNC(rint) (4503599627370496.5000000000001L), 4503599627370497.0L, 0, 0, 0);
# endif
  check_float ("rint_upward (-4503599627370495.5) == -4503599627370495.0",  FUNC(rint) (-4503599627370495.5L), -4503599627370495.0L, 0, 0, 0);
  check_float ("rint_upward (-4503599627370496.25) == -4503599627370496.0",  FUNC(rint) (-4503599627370496.25L), -4503599627370496.0L, 0, 0, 0);
  check_float ("rint_upward (-4503599627370496.5) == -4503599627370496.0",  FUNC(rint) (-4503599627370496.5L), -4503599627370496.0L, 0, 0, 0);
  check_float ("rint_upward (-4503599627370496.75) == -4503599627370496.0",  FUNC(rint) (-4503599627370496.75L), -4503599627370496.0L, 0, 0, 0);
  check_float ("rint_upward (-4503599627370497.5) == -4503599627370497.0",  FUNC(rint) (-4503599627370497.5L), -4503599627370497.0L, 0, 0, 0);
# if LDBL_MANT_DIG > 100
  check_float ("rint_upward (-4503599627370494.5000000000001) == -4503599627370494.0",  FUNC(rint) (-4503599627370494.5000000000001L), -4503599627370494.0L, 0, 0, 0);
  check_float ("rint_upward (-4503599627370495.5000000000001) == -4503599627370495.0",  FUNC(rint) (-4503599627370495.5000000000001L), -4503599627370495.0L, 0, 0, 0);
  check_float ("rint_upward (-4503599627370496.5000000000001) == -4503599627370496.0",  FUNC(rint) (-4503599627370496.5000000000001L), -4503599627370496.0L, 0, 0, 0);

  check_float ("rint_upward (9007199254740991.0000000000001) == 9007199254740992.0",  FUNC(rint) (9007199254740991.0000000000001L), 9007199254740992.0L, 0, 0, 0);
  check_float ("rint_upward (9007199254740992.0000000000001) == 9007199254740993.0",  FUNC(rint) (9007199254740992.0000000000001L), 9007199254740993.0L, 0, 0, 0);
  check_float ("rint_upward (9007199254740993.0000000000001) == 9007199254740994.0",  FUNC(rint) (9007199254740993.0000000000001L), 9007199254740994.0L, 0, 0, 0);
  check_float ("rint_upward (9007199254740991.5000000000001) == 9007199254740992.0",  FUNC(rint) (9007199254740991.5000000000001L), 9007199254740992.0L, 0, 0, 0);
  check_float ("rint_upward (9007199254740992.5000000000001) == 9007199254740993.0",  FUNC(rint) (9007199254740992.5000000000001L), 9007199254740993.0L, 0, 0, 0);
  check_float ("rint_upward (9007199254740993.5000000000001) == 9007199254740994.0",  FUNC(rint) (9007199254740993.5000000000001L), 9007199254740994.0L, 0, 0, 0);

  check_float ("rint_upward (-9007199254740991.0000000000001) == -9007199254740991.0",  FUNC(rint) (-9007199254740991.0000000000001L), -9007199254740991.0L, 0, 0, 0);
  check_float ("rint_upward (-9007199254740992.0000000000001) == -9007199254740992.0",  FUNC(rint) (-9007199254740992.0000000000001L), -9007199254740992.0L, 0, 0, 0);
  check_float ("rint_upward (-9007199254740993.0000000000001) == -9007199254740993.0",  FUNC(rint) (-9007199254740993.0000000000001L), -9007199254740993.0L, 0, 0, 0);
  check_float ("rint_upward (-9007199254740991.5000000000001) == -9007199254740991.0",  FUNC(rint) (-9007199254740991.5000000000001L), -9007199254740991.0L, 0, 0, 0);
  check_float ("rint_upward (-9007199254740992.5000000000001) == -9007199254740992.0",  FUNC(rint) (-9007199254740992.5000000000001L), -9007199254740992.0L, 0, 0, 0);
  check_float ("rint_upward (-9007199254740993.5000000000001) == -9007199254740993.0",  FUNC(rint) (-9007199254740993.5000000000001L), -9007199254740993.0L, 0, 0, 0);
# endif
#endif
    }

  fesetround (save_round_mode);

  print_max_error ("rint_upward", 0, 0);
}

static void
round_test (void)
{
  init_max_error ();

  check_float ("round (0) == 0",  FUNC(round) (0), 0, 0, 0, 0);
  check_float ("round (-0) == -0",  FUNC(round) (minus_zero), minus_zero, 0, 0, 0);
  check_float ("round (0.2) == 0.0",  FUNC(round) (0.2L), 0.0, 0, 0, 0);
  check_float ("round (-0.2) == -0",  FUNC(round) (-0.2L), minus_zero, 0, 0, 0);
  check_float ("round (0.5) == 1.0",  FUNC(round) (0.5), 1.0, 0, 0, 0);
  check_float ("round (-0.5) == -1.0",  FUNC(round) (-0.5), -1.0, 0, 0, 0);
  check_float ("round (0.8) == 1.0",  FUNC(round) (0.8L), 1.0, 0, 0, 0);
  check_float ("round (-0.8) == -1.0",  FUNC(round) (-0.8L), -1.0, 0, 0, 0);
  check_float ("round (1.5) == 2.0",  FUNC(round) (1.5), 2.0, 0, 0, 0);
  check_float ("round (-1.5) == -2.0",  FUNC(round) (-1.5), -2.0, 0, 0, 0);
  check_float ("round (0.1) == 0.0",  FUNC(round) (0.1), 0.0, 0, 0, 0);
  check_float ("round (0.25) == 0.0",  FUNC(round) (0.25), 0.0, 0, 0, 0);
  check_float ("round (0.625) == 1.0",  FUNC(round) (0.625), 1.0, 0, 0, 0);
  check_float ("round (-0.1) == -0.0",  FUNC(round) (-0.1), -0.0, 0, 0, 0);
  check_float ("round (-0.25) == -0.0",  FUNC(round) (-0.25), -0.0, 0, 0, 0);
  check_float ("round (-0.625) == -1.0",  FUNC(round) (-0.625), -1.0, 0, 0, 0);
  check_float ("round (2097152.5) == 2097153",  FUNC(round) (2097152.5), 2097153, 0, 0, 0);
  check_float ("round (-2097152.5) == -2097153",  FUNC(round) (-2097152.5), -2097153, 0, 0, 0);

#ifdef TEST_LDOUBLE
  /* The result can only be represented in long double.  */
  check_float ("round (4503599627370495.5) == 4503599627370496.0",  FUNC(round) (4503599627370495.5L), 4503599627370496.0L, 0, 0, 0);
  check_float ("round (4503599627370496.25) == 4503599627370496.0",  FUNC(round) (4503599627370496.25L), 4503599627370496.0L, 0, 0, 0);
  check_float ("round (4503599627370496.5) == 4503599627370497.0",  FUNC(round) (4503599627370496.5L), 4503599627370497.0L, 0, 0, 0);
  check_float ("round (4503599627370496.75) == 4503599627370497.0",  FUNC(round) (4503599627370496.75L), 4503599627370497.0L, 0, 0, 0);
  check_float ("round (4503599627370497.5) == 4503599627370498.0",  FUNC(round) (4503599627370497.5L), 4503599627370498.0L, 0, 0, 0);
# if LDBL_MANT_DIG > 100
  check_float ("round (4503599627370494.5000000000001) == 4503599627370495.0",  FUNC(round) (4503599627370494.5000000000001L), 4503599627370495.0L, 0, 0, 0);
  check_float ("round (4503599627370495.5000000000001) == 4503599627370496.0",  FUNC(round) (4503599627370495.5000000000001L), 4503599627370496.0L, 0, 0, 0);
  check_float ("round (4503599627370496.5000000000001) == 4503599627370497.0",  FUNC(round) (4503599627370496.5000000000001L), 4503599627370497.0L, 0, 0, 0);
# endif

  check_float ("round (-4503599627370495.5) == -4503599627370496.0",  FUNC(round) (-4503599627370495.5L), -4503599627370496.0L, 0, 0, 0);
  check_float ("round (-4503599627370496.25) == -4503599627370496.0",  FUNC(round) (-4503599627370496.25L), -4503599627370496.0L, 0, 0, 0);
  check_float ("round (-4503599627370496.5) == -4503599627370497.0",  FUNC(round) (-4503599627370496.5L), -4503599627370497.0L, 0, 0, 0);
  check_float ("round (-4503599627370496.75) == -4503599627370497.0",  FUNC(round) (-4503599627370496.75L), -4503599627370497.0L, 0, 0, 0);
  check_float ("round (-4503599627370497.5) == -4503599627370498.0",  FUNC(round) (-4503599627370497.5L), -4503599627370498.0L, 0, 0, 0);
# if LDBL_MANT_DIG > 100
  check_float ("round (-4503599627370494.5000000000001) == -4503599627370495.0",  FUNC(round) (-4503599627370494.5000000000001L), -4503599627370495.0L, 0, 0, 0);
  check_float ("round (-4503599627370495.5000000000001) == -4503599627370496.0",  FUNC(round) (-4503599627370495.5000000000001L), -4503599627370496.0L, 0, 0, 0);
  check_float ("round (-4503599627370496.5000000000001) == -4503599627370497.0",  FUNC(round) (-4503599627370496.5000000000001L), -4503599627370497.0L, 0, 0, 0);
# endif

  check_float ("round (9007199254740991.5) == 9007199254740992.0",  FUNC(round) (9007199254740991.5L), 9007199254740992.0L, 0, 0, 0);
  check_float ("round (9007199254740992.25) == 9007199254740992.0",  FUNC(round) (9007199254740992.25L), 9007199254740992.0L, 0, 0, 0);
  check_float ("round (9007199254740992.5) == 9007199254740993.0",  FUNC(round) (9007199254740992.5L), 9007199254740993.0L, 0, 0, 0);
  check_float ("round (9007199254740992.75) == 9007199254740993.0",  FUNC(round) (9007199254740992.75L), 9007199254740993.0L, 0, 0, 0);
  check_float ("round (9007199254740993.5) == 9007199254740994.0",  FUNC(round) (9007199254740993.5L), 9007199254740994.0L, 0, 0, 0);

  check_float ("round (-9007199254740991.5) == -9007199254740992.0",  FUNC(round) (-9007199254740991.5L), -9007199254740992.0L, 0, 0, 0);
  check_float ("round (-9007199254740992.25) == -9007199254740992.0",  FUNC(round) (-9007199254740992.25L), -9007199254740992.0L, 0, 0, 0);
  check_float ("round (-9007199254740992.5) == -9007199254740993.0",  FUNC(round) (-9007199254740992.5L), -9007199254740993.0L, 0, 0, 0);
  check_float ("round (-9007199254740992.75) == -9007199254740993.0",  FUNC(round) (-9007199254740992.75L), -9007199254740993.0L, 0, 0, 0);
  check_float ("round (-9007199254740993.5) == -9007199254740994.0",  FUNC(round) (-9007199254740993.5L), -9007199254740994.0L, 0, 0, 0);

# if LDBL_MANT_DIG > 100
  check_float ("round (9007199254740991.0000000000001) == 9007199254740991.0",  FUNC(round) (9007199254740991.0000000000001L), 9007199254740991.0L, 0, 0, 0);
  check_float ("round (9007199254740992.0000000000001) == 9007199254740992.0",  FUNC(round) (9007199254740992.0000000000001L), 9007199254740992.0L, 0, 0, 0);
  check_float ("round (9007199254740993.0000000000001) == 9007199254740993.0",  FUNC(round) (9007199254740993.0000000000001L), 9007199254740993.0L, 0, 0, 0);
  check_float ("round (9007199254740991.5000000000001) == 9007199254740992.0",  FUNC(round) (9007199254740991.5000000000001L), 9007199254740992.0L, 0, 0, 0);
  check_float ("round (9007199254740992.5000000000001) == 9007199254740993.0",  FUNC(round) (9007199254740992.5000000000001L), 9007199254740993.0L, 0, 0, 0);
  check_float ("round (9007199254740993.5000000000001) == 9007199254740994.0",  FUNC(round) (9007199254740993.5000000000001L), 9007199254740994.0L, 0, 0, 0);

  check_float ("round (-9007199254740991.0000000000001) == -9007199254740991.0",  FUNC(round) (-9007199254740991.0000000000001L), -9007199254740991.0L, 0, 0, 0);
  check_float ("round (-9007199254740992.0000000000001) == -9007199254740992.0",  FUNC(round) (-9007199254740992.0000000000001L), -9007199254740992.0L, 0, 0, 0);
  check_float ("round (-9007199254740993.0000000000001) == -9007199254740993.0",  FUNC(round) (-9007199254740993.0000000000001L), -9007199254740993.0L, 0, 0, 0);
  check_float ("round (-9007199254740991.5000000000001) == -9007199254740992.0",  FUNC(round) (-9007199254740991.5000000000001L), -9007199254740992.0L, 0, 0, 0);
  check_float ("round (-9007199254740992.5000000000001) == -9007199254740993.0",  FUNC(round) (-9007199254740992.5000000000001L), -9007199254740993.0L, 0, 0, 0);
  check_float ("round (-9007199254740993.5000000000001) == -9007199254740994.0",  FUNC(round) (-9007199254740993.5000000000001L), -9007199254740994.0L, 0, 0, 0);
# endif

  check_float ("round (72057594037927935.5) == 72057594037927936.0",  FUNC(round) (72057594037927935.5L), 72057594037927936.0L, 0, 0, 0);
  check_float ("round (72057594037927936.25) == 72057594037927936.0",  FUNC(round) (72057594037927936.25L), 72057594037927936.0L, 0, 0, 0);
  check_float ("round (72057594037927936.5) == 72057594037927937.0",  FUNC(round) (72057594037927936.5L), 72057594037927937.0L, 0, 0, 0);
  check_float ("round (72057594037927936.75) == 72057594037927937.0",  FUNC(round) (72057594037927936.75L), 72057594037927937.0L, 0, 0, 0);
  check_float ("round (72057594037927937.5) == 72057594037927938.0",  FUNC(round) (72057594037927937.5L), 72057594037927938.0L, 0, 0, 0);

  check_float ("round (-72057594037927935.5) == -72057594037927936.0",  FUNC(round) (-72057594037927935.5L), -72057594037927936.0L, 0, 0, 0);
  check_float ("round (-72057594037927936.25) == -72057594037927936.0",  FUNC(round) (-72057594037927936.25L), -72057594037927936.0L, 0, 0, 0);
  check_float ("round (-72057594037927936.5) == -72057594037927937.0",  FUNC(round) (-72057594037927936.5L), -72057594037927937.0L, 0, 0, 0);
  check_float ("round (-72057594037927936.75) == -72057594037927937.0",  FUNC(round) (-72057594037927936.75L), -72057594037927937.0L, 0, 0, 0);
  check_float ("round (-72057594037927937.5) == -72057594037927938.0",  FUNC(round) (-72057594037927937.5L), -72057594037927938.0L, 0, 0, 0);

  check_float ("round (10141204801825835211973625643007.5) == 10141204801825835211973625643008.0",  FUNC(round) (10141204801825835211973625643007.5L), 10141204801825835211973625643008.0L, 0, 0, 0);
  check_float ("round (10141204801825835211973625643008.25) == 10141204801825835211973625643008.0",  FUNC(round) (10141204801825835211973625643008.25L), 10141204801825835211973625643008.0L, 0, 0, 0);
  check_float ("round (10141204801825835211973625643008.5) == 10141204801825835211973625643009.0",  FUNC(round) (10141204801825835211973625643008.5L), 10141204801825835211973625643009.0L, 0, 0, 0);
  check_float ("round (10141204801825835211973625643008.75) == 10141204801825835211973625643009.0",  FUNC(round) (10141204801825835211973625643008.75L), 10141204801825835211973625643009.0L, 0, 0, 0);
  check_float ("round (10141204801825835211973625643009.5) == 10141204801825835211973625643010.0",  FUNC(round) (10141204801825835211973625643009.5L), 10141204801825835211973625643010.0L, 0, 0, 0);
#endif

  print_max_error ("round", 0, 0);
}


static void
scalb_test (void)
{

  init_max_error ();

  check_float ("scalb (2.0, 0.5) == NaN",  FUNC(scalb) (2.0, 0.5), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("scalb (3.0, -2.5) == NaN",  FUNC(scalb) (3.0, -2.5), nan_value, 0, 0, INVALID_EXCEPTION);

  check_float ("scalb (0, NaN) == NaN",  FUNC(scalb) (0, nan_value), nan_value, 0, 0, 0);
  check_float ("scalb (1, NaN) == NaN",  FUNC(scalb) (1, nan_value), nan_value, 0, 0, 0);

  check_float ("scalb (1, 0) == 1",  FUNC(scalb) (1, 0), 1, 0, 0, 0);
  check_float ("scalb (-1, 0) == -1",  FUNC(scalb) (-1, 0), -1, 0, 0, 0);

  check_float ("scalb (0, inf) == NaN",  FUNC(scalb) (0, plus_infty), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("scalb (-0, inf) == NaN",  FUNC(scalb) (minus_zero, plus_infty), nan_value, 0, 0, INVALID_EXCEPTION);

  check_float ("scalb (0, 2) == 0",  FUNC(scalb) (0, 2), 0, 0, 0, 0);
  check_float ("scalb (-0, -4) == -0",  FUNC(scalb) (minus_zero, -4), minus_zero, 0, 0, 0);
  check_float ("scalb (0, 0) == 0",  FUNC(scalb) (0, 0), 0, 0, 0, 0);
  check_float ("scalb (-0, 0) == -0",  FUNC(scalb) (minus_zero, 0), minus_zero, 0, 0, 0);
  check_float ("scalb (0, -1) == 0",  FUNC(scalb) (0, -1), 0, 0, 0, 0);
  check_float ("scalb (-0, -10) == -0",  FUNC(scalb) (minus_zero, -10), minus_zero, 0, 0, 0);
  check_float ("scalb (0, -inf) == 0",  FUNC(scalb) (0, minus_infty), 0, 0, 0, 0);
  check_float ("scalb (-0, -inf) == -0",  FUNC(scalb) (minus_zero, minus_infty), minus_zero, 0, 0, 0);

  check_float ("scalb (inf, -1) == inf",  FUNC(scalb) (plus_infty, -1), plus_infty, 0, 0, 0);
  check_float ("scalb (-inf, -10) == -inf",  FUNC(scalb) (minus_infty, -10), minus_infty, 0, 0, 0);
  check_float ("scalb (inf, 0) == inf",  FUNC(scalb) (plus_infty, 0), plus_infty, 0, 0, 0);
  check_float ("scalb (-inf, 0) == -inf",  FUNC(scalb) (minus_infty, 0), minus_infty, 0, 0, 0);
  check_float ("scalb (inf, 2) == inf",  FUNC(scalb) (plus_infty, 2), plus_infty, 0, 0, 0);
  check_float ("scalb (-inf, 100) == -inf",  FUNC(scalb) (minus_infty, 100), minus_infty, 0, 0, 0);

  check_float ("scalb (0.1, -inf) == 0.0",  FUNC(scalb) (0.1L, minus_infty), 0.0, 0, 0, 0);
  check_float ("scalb (-0.1, -inf) == -0",  FUNC(scalb) (-0.1L, minus_infty), minus_zero, 0, 0, 0);

  check_float ("scalb (1, inf) == inf",  FUNC(scalb) (1, plus_infty), plus_infty, 0, 0, 0);
  check_float ("scalb (-1, inf) == -inf",  FUNC(scalb) (-1, plus_infty), minus_infty, 0, 0, 0);
  check_float ("scalb (inf, inf) == inf",  FUNC(scalb) (plus_infty, plus_infty), plus_infty, 0, 0, 0);
  check_float ("scalb (-inf, inf) == -inf",  FUNC(scalb) (minus_infty, plus_infty), minus_infty, 0, 0, 0);

  check_float ("scalb (inf, -inf) == NaN",  FUNC(scalb) (plus_infty, minus_infty), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("scalb (-inf, -inf) == NaN",  FUNC(scalb) (minus_infty, minus_infty), nan_value, 0, 0, INVALID_EXCEPTION);

  check_float ("scalb (NaN, 1) == NaN",  FUNC(scalb) (nan_value, 1), nan_value, 0, 0, 0);
  check_float ("scalb (1, NaN) == NaN",  FUNC(scalb) (1, nan_value), nan_value, 0, 0, 0);
  check_float ("scalb (NaN, 0) == NaN",  FUNC(scalb) (nan_value, 0), nan_value, 0, 0, 0);
  check_float ("scalb (0, NaN) == NaN",  FUNC(scalb) (0, nan_value), nan_value, 0, 0, 0);
  check_float ("scalb (NaN, inf) == NaN",  FUNC(scalb) (nan_value, plus_infty), nan_value, 0, 0, 0);
  check_float ("scalb (inf, NaN) == NaN",  FUNC(scalb) (plus_infty, nan_value), nan_value, 0, 0, 0);
  check_float ("scalb (NaN, NaN) == NaN",  FUNC(scalb) (nan_value, nan_value), nan_value, 0, 0, 0);

  check_float ("scalb (0.8, 4) == 12.8",  FUNC(scalb) (0.8L, 4), 12.8L, 0, 0, 0);
  check_float ("scalb (-0.854375, 5) == -27.34",  FUNC(scalb) (-0.854375L, 5), -27.34L, 0, 0, 0);

  print_max_error ("scalb", 0, 0);
}


static void
scalbn_test (void)
{

  init_max_error ();

  check_float ("scalbn (0, 0) == 0",  FUNC(scalbn) (0, 0), 0, 0, 0, 0);
  check_float ("scalbn (-0, 0) == -0",  FUNC(scalbn) (minus_zero, 0), minus_zero, 0, 0, 0);

  check_float ("scalbn (inf, 1) == inf",  FUNC(scalbn) (plus_infty, 1), plus_infty, 0, 0, 0);
  check_float ("scalbn (-inf, 1) == -inf",  FUNC(scalbn) (minus_infty, 1), minus_infty, 0, 0, 0);
  check_float ("scalbn (NaN, 1) == NaN",  FUNC(scalbn) (nan_value, 1), nan_value, 0, 0, 0);

  check_float ("scalbn (0.8, 4) == 12.8",  FUNC(scalbn) (0.8L, 4), 12.8L, 0, 0, 0);
  check_float ("scalbn (-0.854375, 5) == -27.34",  FUNC(scalbn) (-0.854375L, 5), -27.34L, 0, 0, 0);

  check_float ("scalbn (1, 0) == 1",  FUNC(scalbn) (1, 0L), 1, 0, 0, 0);

  check_float ("scalbn (1, INT_MAX) == inf",  FUNC(scalbn) (1, INT_MAX), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("scalbn (1, INT_MIN) == +0",  FUNC(scalbn) (1, INT_MIN), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("scalbn (max_value, INT_MAX) == inf",  FUNC(scalbn) (max_value, INT_MAX), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("scalbn (max_value, INT_MIN) == +0",  FUNC(scalbn) (max_value, INT_MIN), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("scalbn (min_value, INT_MAX) == inf",  FUNC(scalbn) (min_value, INT_MAX), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("scalbn (min_value, INT_MIN) == +0",  FUNC(scalbn) (min_value, INT_MIN), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("scalbn (min_value / 4, INT_MAX) == inf",  FUNC(scalbn) (min_value / 4, INT_MAX), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("scalbn (min_value / 4, INT_MIN) == +0",  FUNC(scalbn) (min_value / 4, INT_MIN), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);

  print_max_error ("scalbn", 0, 0);
}


static void
scalbln_test (void)
{

  init_max_error ();

  check_float ("scalbln (0, 0) == 0",  FUNC(scalbln) (0, 0), 0, 0, 0, 0);
  check_float ("scalbln (-0, 0) == -0",  FUNC(scalbln) (minus_zero, 0), minus_zero, 0, 0, 0);

  check_float ("scalbln (inf, 1) == inf",  FUNC(scalbln) (plus_infty, 1), plus_infty, 0, 0, 0);
  check_float ("scalbln (-inf, 1) == -inf",  FUNC(scalbln) (minus_infty, 1), minus_infty, 0, 0, 0);
  check_float ("scalbln (NaN, 1) == NaN",  FUNC(scalbln) (nan_value, 1), nan_value, 0, 0, 0);

  check_float ("scalbln (0.8, 4) == 12.8",  FUNC(scalbln) (0.8L, 4), 12.8L, 0, 0, 0);
  check_float ("scalbln (-0.854375, 5) == -27.34",  FUNC(scalbln) (-0.854375L, 5), -27.34L, 0, 0, 0);

  check_float ("scalbln (1, 0) == 1",  FUNC(scalbln) (1, 0L), 1, 0, 0, 0);

  check_float ("scalbln (1, INT_MAX) == inf",  FUNC(scalbln) (1, INT_MAX), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("scalbln (1, INT_MIN) == +0",  FUNC(scalbln) (1, INT_MIN), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("scalbln (max_value, INT_MAX) == inf",  FUNC(scalbln) (max_value, INT_MAX), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("scalbln (max_value, INT_MIN) == +0",  FUNC(scalbln) (max_value, INT_MIN), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("scalbln (min_value, INT_MAX) == inf",  FUNC(scalbln) (min_value, INT_MAX), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("scalbln (min_value, INT_MIN) == +0",  FUNC(scalbln) (min_value, INT_MIN), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("scalbln (min_value / 4, INT_MAX) == inf",  FUNC(scalbln) (min_value / 4, INT_MAX), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("scalbln (min_value / 4, INT_MIN) == +0",  FUNC(scalbln) (min_value / 4, INT_MIN), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);

  check_float ("scalbln (1, LONG_MAX) == inf",  FUNC(scalbln) (1, LONG_MAX), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("scalbln (1, LONG_MIN) == +0",  FUNC(scalbln) (1, LONG_MIN), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("scalbln (max_value, LONG_MAX) == inf",  FUNC(scalbln) (max_value, LONG_MAX), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("scalbln (max_value, LONG_MIN) == +0",  FUNC(scalbln) (max_value, LONG_MIN), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("scalbln (min_value, LONG_MAX) == inf",  FUNC(scalbln) (min_value, LONG_MAX), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("scalbln (min_value, LONG_MIN) == +0",  FUNC(scalbln) (min_value, LONG_MIN), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("scalbln (min_value / 4, LONG_MAX) == inf",  FUNC(scalbln) (min_value / 4, LONG_MAX), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("scalbln (min_value / 4, LONG_MIN) == +0",  FUNC(scalbln) (min_value / 4, LONG_MIN), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);

#if LONG_MAX >= 0x100000000
  check_float ("scalbln (1, 0x88000000) == inf",  FUNC(scalbln) (1, 0x88000000L), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("scalbln (1, -0x88000000) == +0",  FUNC(scalbln) (1, -0x88000000L), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("scalbln (max_value, 0x88000000) == inf",  FUNC(scalbln) (max_value, 0x88000000L), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("scalbln (max_value, -0x88000000) == +0",  FUNC(scalbln) (max_value, -0x88000000L), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("scalbln (min_value, 0x88000000) == inf",  FUNC(scalbln) (min_value, 0x88000000L), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("scalbln (min_value, -0x88000000) == +0",  FUNC(scalbln) (min_value, -0x88000000L), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
  check_float ("scalbln (min_value / 4, 0x88000000) == inf",  FUNC(scalbln) (min_value / 4, 0x88000000L), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("scalbln (min_value / 4, -0x88000000) == +0",  FUNC(scalbln) (min_value / 4, -0x88000000L), plus_zero, 0, 0, UNDERFLOW_EXCEPTION);
#endif

  print_max_error ("scalbn", 0, 0);
}


static void
signbit_test (void)
{

  init_max_error ();

  check_bool ("signbit (0) == false", signbit (0), 0, 0, 0, 0);
  check_bool ("signbit (-0) == true", signbit (minus_zero), 1, 0, 0, 0);
  check_bool ("signbit (inf) == false", signbit (plus_infty), 0, 0, 0, 0);
  check_bool ("signbit (-inf) == true", signbit (minus_infty), 1, 0, 0, 0);

  /* signbit (x) != 0 for x < 0.  */
  check_bool ("signbit (-1) == true", signbit (-1), 1, 0, 0, 0);
  /* signbit (x) == 0 for x >= 0.  */
  check_bool ("signbit (1) == false", signbit (1), 0, 0, 0, 0);

  print_max_error ("signbit", 0, 0);
}


static void
sin_test (void)
{
  errno = 0;
  FUNC(sin) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_float ("sin (0) == 0",  FUNC(sin) (0), 0, 0, 0, 0);
  check_float ("sin (-0) == -0",  FUNC(sin) (minus_zero), minus_zero, 0, 0, 0);
  errno = 0;
  check_float ("sin (inf) == NaN",  FUNC(sin) (plus_infty), nan_value, 0, 0, INVALID_EXCEPTION);
  check_int ("errno for sin(+inf) == EDOM", errno, EDOM, 0, 0, 0);
  errno = 0;
  check_float ("sin (-inf) == NaN",  FUNC(sin) (minus_infty), nan_value, 0, 0, INVALID_EXCEPTION);
  check_int ("errno for sin(-inf) == EDOM", errno, EDOM, 0, 0, 0);
  errno = 0;
  check_float ("sin (NaN) == NaN",  FUNC(sin) (nan_value), nan_value, 0, 0, 0);
  check_int ("errno for sin(NaN) unchanged", errno, 0, 0, 0, 0);

  check_float ("sin (pi/6) == 0.5",  FUNC(sin) (M_PI_6l), 0.5, 0, 0, 0);
  check_float ("sin (-pi/6) == -0.5",  FUNC(sin) (-M_PI_6l), -0.5, 0, 0, 0);
  check_float ("sin (pi/2) == 1",  FUNC(sin) (M_PI_2l), 1, 0, 0, 0);
  check_float ("sin (-pi/2) == -1",  FUNC(sin) (-M_PI_2l), -1, 0, 0, 0);
  check_float ("sin (0.75) == 0.681638760023334166733241952779893935",  FUNC(sin) (0.75L), 0.681638760023334166733241952779893935L, 0, 0, 0);

  check_float ("sin (0x1p65) == -0.047183876212354673805106149805700013943218",  FUNC(sin) (0x1p65), -0.047183876212354673805106149805700013943218L, 0, 0, 0);
  check_float ("sin (-0x1p65) == 0.047183876212354673805106149805700013943218",  FUNC(sin) (-0x1p65), 0.047183876212354673805106149805700013943218L, 0, 0, 0);

  check_float ("sin (0x1.7f4134p+103) == -6.6703229329788657073304190650534846045235e-08",  FUNC(sin) (0x1.7f4134p+103), -6.6703229329788657073304190650534846045235e-08L, 0, 0, 0);

#ifdef TEST_DOUBLE
  check_float ("sin (0.80190127184058835) == 0.71867942238767868",  FUNC(sin) (0.80190127184058835), 0.71867942238767868, 0, 0, 0);
  check_float ("sin (2.522464e-1) == 2.4957989804940911e-1",  FUNC(sin) (2.522464e-1), 2.4957989804940911e-1, 0, 0, 0);
#endif

#ifndef TEST_FLOAT
  check_float ("sin (1e22) == -0.8522008497671888017727058937530293682618",  FUNC(sin) (1e22), -0.8522008497671888017727058937530293682618L, 0, 0, 0);
  check_float ("sin (0x1p1023) == 0.5631277798508840134529434079444683477104",  FUNC(sin) (0x1p1023), 0.5631277798508840134529434079444683477104L, 0, 0, 0);
#endif

#if defined TEST_LDOUBLE && LDBL_MAX_EXP >= 16384
  check_float ("sin (0x1p16383) == 0.3893629985894208126948115852610595405563",  FUNC(sin) (0x1p16383L), 0.3893629985894208126948115852610595405563L, 0, 0, 0);
#endif

  check_float ("sin (0x1p+120) == 3.77820109360752022655548470056922991960587e-01",  FUNC(sin) (0x1p+120), 3.77820109360752022655548470056922991960587e-01L, 0, 0, 0);
  check_float ("sin (0x1p+127) == 6.23385512955870240370428801097126489001833e-01",  FUNC(sin) (0x1p+127), 6.23385512955870240370428801097126489001833e-01L, 0, 0, 0);
  check_float ("sin (0x1.fffff8p+127) == 4.85786063130487339701113680434728152037092e-02",  FUNC(sin) (0x1.fffff8p+127), 4.85786063130487339701113680434728152037092e-02L, 0, 0, 0);
  check_float ("sin (0x1.fffffep+127) == -5.21876523333658540551505357019806722935726e-01",  FUNC(sin) (0x1.fffffep+127), -5.21876523333658540551505357019806722935726e-01L, 0, 0, 0);
  check_float ("sin (0x1p+50) == 4.96396515208940840876821859865411368093356e-01",  FUNC(sin) (0x1p+50), 4.96396515208940840876821859865411368093356e-01L, 0, 0, 0);
  check_float ("sin (0x1p+28) == -9.86198211836975655703110310527108292055548e-01",  FUNC(sin) (0x1p+28), -9.86198211836975655703110310527108292055548e-01L, 0, 0, 0);

  print_max_error ("sin", 0, 0);

}


static void
sin_test_tonearest (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(sin) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_TONEAREST))
    {
  check_float ("sin_tonearest (1) == 0.8414709848078965066525023216302989996226",  FUNC(sin) (1), 0.8414709848078965066525023216302989996226L, DELTA4723, 0, 0);
  check_float ("sin_tonearest (2) == 0.9092974268256816953960198659117448427023",  FUNC(sin) (2), 0.9092974268256816953960198659117448427023L, 0, 0, 0);
  check_float ("sin_tonearest (3) == 0.1411200080598672221007448028081102798469",  FUNC(sin) (3), 0.1411200080598672221007448028081102798469L, 0, 0, 0);
  check_float ("sin_tonearest (4) == -0.7568024953079282513726390945118290941359",  FUNC(sin) (4), -0.7568024953079282513726390945118290941359L, DELTA4726, 0, 0);
  check_float ("sin_tonearest (5) == -0.9589242746631384688931544061559939733525",  FUNC(sin) (5), -0.9589242746631384688931544061559939733525L, 0, 0, 0);
  check_float ("sin_tonearest (6) == -0.2794154981989258728115554466118947596280",  FUNC(sin) (6), -0.2794154981989258728115554466118947596280L, 0, 0, 0);
  check_float ("sin_tonearest (7) == 0.6569865987187890903969990915936351779369",  FUNC(sin) (7), 0.6569865987187890903969990915936351779369L, 0, 0, 0);
  check_float ("sin_tonearest (8) == 0.9893582466233817778081235982452886721164",  FUNC(sin) (8), 0.9893582466233817778081235982452886721164L, 0, 0, 0);
  check_float ("sin_tonearest (9) == 0.4121184852417565697562725663524351793439",  FUNC(sin) (9), 0.4121184852417565697562725663524351793439L, DELTA4731, 0, 0);
  check_float ("sin_tonearest (10) == -0.5440211108893698134047476618513772816836",  FUNC(sin) (10), -0.5440211108893698134047476618513772816836L, DELTA4732, 0, 0);
    }

  fesetround (save_round_mode);

  print_max_error ("sin_tonearest", DELTAsin_tonearest, 0);
}


static void
sin_test_towardzero (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(sin) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_TOWARDZERO))
    {
  check_float ("sin_towardzero (1) == 0.8414709848078965066525023216302989996226",  FUNC(sin) (1), 0.8414709848078965066525023216302989996226L, DELTA4733, 0, 0);
  check_float ("sin_towardzero (2) == 0.9092974268256816953960198659117448427023",  FUNC(sin) (2), 0.9092974268256816953960198659117448427023L, 0, 0, 0);
  check_float ("sin_towardzero (3) == 0.1411200080598672221007448028081102798469",  FUNC(sin) (3), 0.1411200080598672221007448028081102798469L, DELTA4735, 0, 0);
  check_float ("sin_towardzero (4) == -0.7568024953079282513726390945118290941359",  FUNC(sin) (4), -0.7568024953079282513726390945118290941359L, DELTA4736, 0, 0);
  check_float ("sin_towardzero (5) == -0.9589242746631384688931544061559939733525",  FUNC(sin) (5), -0.9589242746631384688931544061559939733525L, DELTA4737, 0, 0);
  check_float ("sin_towardzero (6) == -0.2794154981989258728115554466118947596280",  FUNC(sin) (6), -0.2794154981989258728115554466118947596280L, DELTA4738, 0, 0);
  check_float ("sin_towardzero (7) == 0.6569865987187890903969990915936351779369",  FUNC(sin) (7), 0.6569865987187890903969990915936351779369L, DELTA4739, 0, 0);
  check_float ("sin_towardzero (8) == 0.9893582466233817778081235982452886721164",  FUNC(sin) (8), 0.9893582466233817778081235982452886721164L, DELTA4740, 0, 0);
  check_float ("sin_towardzero (9) == 0.4121184852417565697562725663524351793439",  FUNC(sin) (9), 0.4121184852417565697562725663524351793439L, DELTA4741, 0, 0);
  check_float ("sin_towardzero (10) == -0.5440211108893698134047476618513772816836",  FUNC(sin) (10), -0.5440211108893698134047476618513772816836L, DELTA4742, 0, 0);
    }

  fesetround (save_round_mode);

  print_max_error ("sin_towardzero", DELTAsin_towardzero, 0);
}


static void
sin_test_downward (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(sin) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_DOWNWARD))
    {
  check_float ("sin_downward (1) == 0.8414709848078965066525023216302989996226",  FUNC(sin) (1), 0.8414709848078965066525023216302989996226L, DELTA4743, 0, 0);
  check_float ("sin_downward (2) == 0.9092974268256816953960198659117448427023",  FUNC(sin) (2), 0.9092974268256816953960198659117448427023L, 0, 0, 0);
  check_float ("sin_downward (3) == 0.1411200080598672221007448028081102798469",  FUNC(sin) (3), 0.1411200080598672221007448028081102798469L, DELTA4745, 0, 0);
  check_float ("sin_downward (4) == -0.7568024953079282513726390945118290941359",  FUNC(sin) (4), -0.7568024953079282513726390945118290941359L, DELTA4746, 0, 0);
  check_float ("sin_downward (5) == -0.9589242746631384688931544061559939733525",  FUNC(sin) (5), -0.9589242746631384688931544061559939733525L, DELTA4747, 0, 0);
  check_float ("sin_downward (6) == -0.2794154981989258728115554466118947596280",  FUNC(sin) (6), -0.2794154981989258728115554466118947596280L, DELTA4748, 0, 0);
  check_float ("sin_downward (7) == 0.6569865987187890903969990915936351779369",  FUNC(sin) (7), 0.6569865987187890903969990915936351779369L, DELTA4749, 0, 0);
  check_float ("sin_downward (8) == 0.9893582466233817778081235982452886721164",  FUNC(sin) (8), 0.9893582466233817778081235982452886721164L, DELTA4750, 0, 0);
  check_float ("sin_downward (9) == 0.4121184852417565697562725663524351793439",  FUNC(sin) (9), 0.4121184852417565697562725663524351793439L, DELTA4751, 0, 0);
  check_float ("sin_downward (10) == -0.5440211108893698134047476618513772816836",  FUNC(sin) (10), -0.5440211108893698134047476618513772816836L, DELTA4752, 0, 0);
    }

  fesetround (save_round_mode);

  print_max_error ("sin_downward", DELTAsin_downward, 0);
}


static void
sin_test_upward (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(sin) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_UPWARD))
    {
  check_float ("sin_upward (1) == 0.8414709848078965066525023216302989996226",  FUNC(sin) (1), 0.8414709848078965066525023216302989996226L, DELTA4753, 0, 0);
  check_float ("sin_upward (2) == 0.9092974268256816953960198659117448427023",  FUNC(sin) (2), 0.9092974268256816953960198659117448427023L, DELTA4754, 0, 0);
  check_float ("sin_upward (3) == 0.1411200080598672221007448028081102798469",  FUNC(sin) (3), 0.1411200080598672221007448028081102798469L, DELTA4755, 0, 0);
  check_float ("sin_upward (4) == -0.7568024953079282513726390945118290941359",  FUNC(sin) (4), -0.7568024953079282513726390945118290941359L, DELTA4756, 0, 0);
  check_float ("sin_upward (5) == -0.9589242746631384688931544061559939733525",  FUNC(sin) (5), -0.9589242746631384688931544061559939733525L, DELTA4757, 0, 0);
  check_float ("sin_upward (6) == -0.2794154981989258728115554466118947596280",  FUNC(sin) (6), -0.2794154981989258728115554466118947596280L, DELTA4758, 0, 0);
  check_float ("sin_upward (7) == 0.6569865987187890903969990915936351779369",  FUNC(sin) (7), 0.6569865987187890903969990915936351779369L, DELTA4759, 0, 0);
  check_float ("sin_upward (8) == 0.9893582466233817778081235982452886721164",  FUNC(sin) (8), 0.9893582466233817778081235982452886721164L, DELTA4760, 0, 0);
  check_float ("sin_upward (9) == 0.4121184852417565697562725663524351793439",  FUNC(sin) (9), 0.4121184852417565697562725663524351793439L, DELTA4761, 0, 0);
  check_float ("sin_upward (10) == -0.5440211108893698134047476618513772816836",  FUNC(sin) (10), -0.5440211108893698134047476618513772816836L, DELTA4762, 0, 0);
    }

  fesetround (save_round_mode);

  print_max_error ("sin_upward", DELTAsin_upward, 0);
}


static void
sincos_test (void)
{
  FLOAT sin_res, cos_res;

  errno = 0;
  FUNC(sincos) (0, &sin_res, &cos_res);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  /* sincos is treated differently because it returns void.  */
  FUNC (sincos) (0, &sin_res, &cos_res);
  check_float ("sincos (0, &sin_res, &cos_res) puts 0 in sin_res", sin_res, 0, 0, 0, 0);
  check_float ("sincos (0, &sin_res, &cos_res) puts 1 in cos_res", cos_res, 1, 0, 0, 0);

  FUNC (sincos) (minus_zero, &sin_res, &cos_res);
  check_float ("sincos (-0, &sin_res, &cos_res) puts -0 in sin_res", sin_res, minus_zero, 0, 0, 0);
  check_float ("sincos (-0, &sin_res, &cos_res) puts 1 in cos_res", cos_res, 1, 0, 0, 0);
  FUNC (sincos) (plus_infty, &sin_res, &cos_res);
  check_float ("sincos (inf, &sin_res, &cos_res) puts NaN in sin_res", sin_res, nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("sincos (inf, &sin_res, &cos_res) puts NaN in cos_res", cos_res, nan_value, 0, 0, 0);
  FUNC (sincos) (minus_infty, &sin_res, &cos_res);
  check_float ("sincos (-inf, &sin_res, &cos_res) puts NaN in sin_res", sin_res, nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("sincos (-inf, &sin_res, &cos_res) puts NaN in cos_res", cos_res, nan_value, 0, 0, 0);
  FUNC (sincos) (nan_value, &sin_res, &cos_res);
  check_float ("sincos (NaN, &sin_res, &cos_res) puts NaN in sin_res", sin_res, nan_value, 0, 0, 0);
  check_float ("sincos (NaN, &sin_res, &cos_res) puts NaN in cos_res", cos_res, nan_value, 0, 0, 0);

  FUNC (sincos) (M_PI_2l, &sin_res, &cos_res);
  check_float ("sincos (pi/2, &sin_res, &cos_res) puts 1 in sin_res", sin_res, 1, 0, 0, 0);
  check_float ("sincos (pi/2, &sin_res, &cos_res) puts 0 in cos_res", cos_res, 0, DELTA4774, 0, 0);
  FUNC (sincos) (M_PI_6l, &sin_res, &cos_res);
  check_float ("sincos (pi/6, &sin_res, &cos_res) puts 0.5 in sin_res", sin_res, 0.5, 0, 0, 0);
  check_float ("sincos (pi/6, &sin_res, &cos_res) puts 0.86602540378443864676372317075293616 in cos_res", cos_res, 0.86602540378443864676372317075293616L, DELTA4776, 0, 0);
  FUNC (sincos) (M_PI_6l*2.0, &sin_res, &cos_res);
  check_float ("sincos (M_PI_6l*2.0, &sin_res, &cos_res) puts 0.86602540378443864676372317075293616 in sin_res", sin_res, 0.86602540378443864676372317075293616L, DELTA4777, 0, 0);
  check_float ("sincos (M_PI_6l*2.0, &sin_res, &cos_res) puts 0.5 in cos_res", cos_res, 0.5, DELTA4778, 0, 0);
  FUNC (sincos) (0.75L, &sin_res, &cos_res);
  check_float ("sincos (0.75, &sin_res, &cos_res) puts 0.681638760023334166733241952779893935 in sin_res", sin_res, 0.681638760023334166733241952779893935L, 0, 0, 0);
  check_float ("sincos (0.75, &sin_res, &cos_res) puts 0.731688868873820886311838753000084544 in cos_res", cos_res, 0.731688868873820886311838753000084544L, 0, 0, 0);

  FUNC (sincos) (0x1p65, &sin_res, &cos_res);
  check_float ("sincos (0x1p65, &sin_res, &cos_res) puts -0.047183876212354673805106149805700013943218 in sin_res", sin_res, -0.047183876212354673805106149805700013943218L, 0, 0, 0);
  check_float ("sincos (0x1p65, &sin_res, &cos_res) puts 0.99888622066058013610642172179340364209972 in cos_res", cos_res, 0.99888622066058013610642172179340364209972L, 0, 0, 0);
  FUNC (sincos) (-0x1p65, &sin_res, &cos_res);
  check_float ("sincos (-0x1p65, &sin_res, &cos_res) puts 0.047183876212354673805106149805700013943218 in sin_res", sin_res, 0.047183876212354673805106149805700013943218L, 0, 0, 0);
  check_float ("sincos (-0x1p65, &sin_res, &cos_res) puts 0.99888622066058013610642172179340364209972 in cos_res", cos_res, 0.99888622066058013610642172179340364209972L, 0, 0, 0);

#ifdef TEST_DOUBLE
  FUNC (sincos) (0.80190127184058835, &sin_res, &cos_res);
  check_float ("sincos (0.80190127184058835, &sin_res, &cos_res) puts 0.71867942238767868 in sin_res", sin_res, 0.71867942238767868, 0, 0, 0);
  check_float ("sincos (0.80190127184058835, &sin_res, &cos_res) puts 0.69534156199418473 in cos_res", cos_res, 0.69534156199418473, DELTA4786, 0, 0);
#endif

#ifndef TEST_FLOAT
  FUNC (sincos) (1e22, &sin_res, &cos_res);
  check_float ("sincos (1e22, &sin_res, &cos_res) puts -0.8522008497671888017727058937530293682618 in sin_res", sin_res, -0.8522008497671888017727058937530293682618L, 0, 0, 0);
  check_float ("sincos (1e22, &sin_res, &cos_res) puts 0.5232147853951389454975944733847094921409 in cos_res", cos_res, 0.5232147853951389454975944733847094921409L, 0, 0, 0);
  FUNC (sincos) (0x1p1023, &sin_res, &cos_res);
  check_float ("sincos (0x1p1023, &sin_res, &cos_res) puts 0.5631277798508840134529434079444683477104 in sin_res", sin_res, 0.5631277798508840134529434079444683477104L, 0, 0, 0);
  check_float ("sincos (0x1p1023, &sin_res, &cos_res) puts -0.826369834614147994500785680811743734805 in cos_res", cos_res, -0.826369834614147994500785680811743734805L, 0, 0, 0);
#endif

#if defined TEST_LDOUBLE && LDBL_MAX_EXP >= 16384
  FUNC (sincos) (0x1p16383L, &sin_res, &cos_res);
  check_float ("sincos (0x1p16383, &sin_res, &cos_res) puts 0.3893629985894208126948115852610595405563 in sin_res", sin_res, 0.3893629985894208126948115852610595405563L, 0, 0, 0);
  check_float ("sincos (0x1p16383, &sin_res, &cos_res) puts 0.9210843909921906206874509522505756251609 in cos_res", cos_res, 0.9210843909921906206874509522505756251609L, 0, 0, 0);
#endif

  FUNC (sincos) (0x1p+120, &sin_res, &cos_res);
  check_float ("sincos (0x1p+120, &sin_res, &cos_res) puts 3.77820109360752022655548470056922991960587e-01 in sin_res", sin_res, 3.77820109360752022655548470056922991960587e-01L, 0, 0, 0);
  check_float ("sincos (0x1p+120, &sin_res, &cos_res) puts -9.25879022854837867303861764107414946730833e-01 in cos_res", cos_res, -9.25879022854837867303861764107414946730833e-01L, 0, 0, 0);
  FUNC (sincos) (0x1p+127, &sin_res, &cos_res);
  check_float ("sincos (0x1p+127, &sin_res, &cos_res) puts 6.23385512955870240370428801097126489001833e-01 in sin_res", sin_res, 6.23385512955870240370428801097126489001833e-01L, 0, 0, 0);
  check_float ("sincos (0x1p+127, &sin_res, &cos_res) puts 7.81914638714960072263910298466369236613162e-01 in cos_res", cos_res, 7.81914638714960072263910298466369236613162e-01L, 0, 0, 0);
  FUNC (sincos) (0x1.fffff8p+127, &sin_res, &cos_res);
  check_float ("sincos (0x1.fffff8p+127, &sin_res, &cos_res) puts 4.85786063130487339701113680434728152037092e-02 in sin_res", sin_res, 4.85786063130487339701113680434728152037092e-02L, 0, 0, 0);
  check_float ("sincos (0x1.fffff8p+127, &sin_res, &cos_res) puts 9.98819362551949040703862043664101081064641e-01 in cos_res", cos_res, 9.98819362551949040703862043664101081064641e-01L, 0, 0, 0);
  FUNC (sincos) (0x1.fffffep+127, &sin_res, &cos_res);
  check_float ("sincos (0x1.fffffep+127, &sin_res, &cos_res) puts -5.21876523333658540551505357019806722935726e-01 in sin_res", sin_res, -5.21876523333658540551505357019806722935726e-01L, 0, 0, 0);
  check_float ("sincos (0x1.fffffep+127, &sin_res, &cos_res) puts 8.53021039830304158051791467692161107353094e-01 in cos_res", cos_res, 8.53021039830304158051791467692161107353094e-01L, 0, 0, 0);
  FUNC (sincos) (0x1p+50, &sin_res, &cos_res);
  check_float ("sincos (0x1p+50, &sin_res, &cos_res) puts 4.96396515208940840876821859865411368093356e-01 in sin_res", sin_res, 4.96396515208940840876821859865411368093356e-01L, 0, 0, 0);
  check_float ("sincos (0x1p+50, &sin_res, &cos_res) puts 8.68095904660550604334592502063501320395739e-01 in cos_res", cos_res, 8.68095904660550604334592502063501320395739e-01L, 0, 0, 0);
  FUNC (sincos) (0x1p+28, &sin_res, &cos_res);
  check_float ("sincos (0x1p+28, &sin_res, &cos_res) puts -9.86198211836975655703110310527108292055548e-01 in sin_res", sin_res, -9.86198211836975655703110310527108292055548e-01L, 0, 0, 0);
  check_float ("sincos (0x1p+28, &sin_res, &cos_res) puts -1.65568979490578758865468278195361551113358e-01 in cos_res", cos_res, -1.65568979490578758865468278195361551113358e-01L, 0, 0, 0);

  print_max_error ("sincos", DELTAsincos, 0);
}

static void
sinh_test (void)
{
  errno = 0;
  FUNC(sinh) (0.7L);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();
  check_float ("sinh (0) == 0",  FUNC(sinh) (0), 0, 0, 0, 0);
  check_float ("sinh (-0) == -0",  FUNC(sinh) (minus_zero), minus_zero, 0, 0, 0);

#ifndef TEST_INLINE
  check_float ("sinh (inf) == inf",  FUNC(sinh) (plus_infty), plus_infty, 0, 0, 0);
  check_float ("sinh (-inf) == -inf",  FUNC(sinh) (minus_infty), minus_infty, 0, 0, 0);
#endif
  check_float ("sinh (NaN) == NaN",  FUNC(sinh) (nan_value), nan_value, 0, 0, 0);

  check_float ("sinh (0.75) == 0.822316731935829980703661634446913849",  FUNC(sinh) (0.75L), 0.822316731935829980703661634446913849L, 0, 0, 0);
  check_float ("sinh (0x8p-32) == 1.86264514923095703232705808926175479e-9",  FUNC(sinh) (0x8p-32L), 1.86264514923095703232705808926175479e-9L, 0, 0, 0);

  print_max_error ("sinh", 0, 0);
}


static void
sinh_test_tonearest (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(sinh) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_TONEAREST))
    {
  check_float ("sinh_tonearest (22) == 1792456423.065795780701106568345764104225",  FUNC(sinh) (22), 1792456423.065795780701106568345764104225L, 0, 0, 0);
  check_float ("sinh_tonearest (23) == 4872401723.124451299966006944252978187305",  FUNC(sinh) (23), 4872401723.124451299966006944252978187305L, 0, 0, 0);
  check_float ("sinh_tonearest (24) == 13244561064.92173614705070540368454568168",  FUNC(sinh) (24), 13244561064.92173614705070540368454568168L, 0, 0, 0);
    }

  fesetround (save_round_mode);

  print_max_error ("sinh_tonearest", 0, 0);
}


static void
sinh_test_towardzero (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(sinh) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_TOWARDZERO))
    {
  check_float ("sinh_towardzero (22) == 1792456423.065795780701106568345764104225",  FUNC(sinh) (22), 1792456423.065795780701106568345764104225L, DELTA4815, 0, 0);
  check_float ("sinh_towardzero (23) == 4872401723.124451299966006944252978187305",  FUNC(sinh) (23), 4872401723.124451299966006944252978187305L, DELTA4816, 0, 0);
  check_float ("sinh_towardzero (24) == 13244561064.92173614705070540368454568168",  FUNC(sinh) (24), 13244561064.92173614705070540368454568168L, DELTA4817, 0, 0);
    }

  fesetround (save_round_mode);

  print_max_error ("sinh_towardzero", DELTAsinh_towardzero, 0);
}


static void
sinh_test_downward (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(sinh) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_DOWNWARD))
    {
  check_float ("sinh_downward (22) == 1792456423.065795780701106568345764104225",  FUNC(sinh) (22), 1792456423.065795780701106568345764104225L, DELTA4818, 0, 0);
  check_float ("sinh_downward (23) == 4872401723.124451299966006944252978187305",  FUNC(sinh) (23), 4872401723.124451299966006944252978187305L, DELTA4819, 0, 0);
  check_float ("sinh_downward (24) == 13244561064.92173614705070540368454568168",  FUNC(sinh) (24), 13244561064.92173614705070540368454568168L, DELTA4820, 0, 0);
    }

  fesetround (save_round_mode);

  print_max_error ("sinh_downward", DELTAsinh_downward, 0);
}


static void
sinh_test_upward (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(sinh) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_UPWARD))
    {
  check_float ("sinh_upward (22) == 1792456423.065795780701106568345764104225",  FUNC(sinh) (22), 1792456423.065795780701106568345764104225L, DELTA4821, 0, 0);
  check_float ("sinh_upward (23) == 4872401723.124451299966006944252978187305",  FUNC(sinh) (23), 4872401723.124451299966006944252978187305L, DELTA4822, 0, 0);
  check_float ("sinh_upward (24) == 13244561064.92173614705070540368454568168",  FUNC(sinh) (24), 13244561064.92173614705070540368454568168L, 0, 0, 0);
    }

  fesetround (save_round_mode);

  print_max_error ("sinh_upward", DELTAsinh_upward, 0);
}


static void
sqrt_test (void)
{
  errno = 0;
  FUNC(sqrt) (1);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_float ("sqrt (0) == 0",  FUNC(sqrt) (0), 0, 0, 0, 0);
  check_float ("sqrt (NaN) == NaN",  FUNC(sqrt) (nan_value), nan_value, 0, 0, 0);
  check_float ("sqrt (inf) == inf",  FUNC(sqrt) (plus_infty), plus_infty, 0, 0, 0);

  check_float ("sqrt (-0) == -0",  FUNC(sqrt) (minus_zero), minus_zero, 0, 0, 0);

  /* sqrt (x) == NaN plus invalid exception for x < 0.  */
  check_float ("sqrt (-1) == NaN",  FUNC(sqrt) (-1), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("sqrt (-max_value) == NaN",  FUNC(sqrt) (-max_value), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("sqrt (-inf) == NaN",  FUNC(sqrt) (minus_infty), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("sqrt (NaN) == NaN",  FUNC(sqrt) (nan_value), nan_value, 0, 0, 0);

  check_float ("sqrt (2209) == 47",  FUNC(sqrt) (2209), 47, 0, 0, 0);
  check_float ("sqrt (4) == 2",  FUNC(sqrt) (4), 2, 0, 0, 0);
  check_float ("sqrt (2) == M_SQRT2l",  FUNC(sqrt) (2), M_SQRT2l, 0, 0, 0);
  check_float ("sqrt (0.25) == 0.5",  FUNC(sqrt) (0.25), 0.5, 0, 0, 0);
  check_float ("sqrt (6642.25) == 81.5",  FUNC(sqrt) (6642.25), 81.5, 0, 0, 0);
  check_float ("sqrt (15190.5625) == 123.25",  FUNC(sqrt) (15190.5625L), 123.25L, 0, 0, 0);
  check_float ("sqrt (0.75) == 0.866025403784438646763723170752936183",  FUNC(sqrt) (0.75L), 0.866025403784438646763723170752936183L, 0, 0, 0);

  print_max_error ("sqrt", 0, 0);
}


static void
tan_test (void)
{
  errno = 0;
  FUNC(tan) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_float ("tan (0) == 0",  FUNC(tan) (0), 0, 0, 0, 0);
  check_float ("tan (-0) == -0",  FUNC(tan) (minus_zero), minus_zero, 0, 0, 0);
  errno = 0;
  check_float ("tan (inf) == NaN",  FUNC(tan) (plus_infty), nan_value, 0, 0, INVALID_EXCEPTION);
  check_int ("errno for tan(Inf) == EDOM", errno, EDOM, 0, 0, 0);
  errno = 0;
  check_float ("tan (-inf) == NaN",  FUNC(tan) (minus_infty), nan_value, 0, 0, INVALID_EXCEPTION);
  check_int ("errno for tan(-Inf) == EDOM", errno, EDOM, 0, 0, 0);
  errno = 0;
  check_float ("tan (NaN) == NaN",  FUNC(tan) (nan_value), nan_value, 0, 0, 0);
  check_int ("errno for tan(NaN) == 0", errno, 0, 0, 0, 0);

  check_float ("tan (pi/4) == 1",  FUNC(tan) (M_PI_4l), 1, DELTA4844, 0, 0);
  check_float ("tan (0.75) == 0.931596459944072461165202756573936428",  FUNC(tan) (0.75L), 0.931596459944072461165202756573936428L, 0, 0, 0);

  check_float ("tan (0x1p65) == -0.0472364872359047946798414219288370688827",  FUNC(tan) (0x1p65), -0.0472364872359047946798414219288370688827L, 0, 0, 0);
  check_float ("tan (-0x1p65) == 0.0472364872359047946798414219288370688827",  FUNC(tan) (-0x1p65), 0.0472364872359047946798414219288370688827L, 0, 0, 0);

  check_float ("tan (0xc.9p-4) == 0.9995162902115457818029468900654150261381",  FUNC(tan) (0xc.9p-4), 0.9995162902115457818029468900654150261381L, 0, 0, 0);
  check_float ("tan (0xc.908p-4) == 0.9997603425502441410973077452249560802034",  FUNC(tan) (0xc.908p-4), 0.9997603425502441410973077452249560802034L, 0, 0, 0);
  check_float ("tan (0xc.90cp-4) == 0.9998823910588060302788513970802357770031",  FUNC(tan) (0xc.90cp-4), 0.9998823910588060302788513970802357770031L, 0, 0, 0);
  check_float ("tan (0xc.90ep-4) == 0.9999434208994808753305784795924711152508",  FUNC(tan) (0xc.90ep-4), 0.9999434208994808753305784795924711152508L, 0, 0, 0);
  check_float ("tan (0xc.90fp-4) == 0.9999739372166156702433266059635165160515",  FUNC(tan) (0xc.90fp-4), 0.9999739372166156702433266059635165160515L, 0, 0, 0);
  check_float ("tan (0xc.90f8p-4) == 0.9999891957244072765118898375645469865764",  FUNC(tan) (0xc.90f8p-4), 0.9999891957244072765118898375645469865764L, 0, 0, 0);
  check_float ("tan (0xc.90fcp-4) == 0.9999968250656122402859679132395522927393",  FUNC(tan) (0xc.90fcp-4), 0.9999968250656122402859679132395522927393L, 0, 0, 0);
  check_float ("tan (0xc.90fdp-4) == 0.9999987324100083358016192309006353329444",  FUNC(tan) (0xc.90fdp-4), 0.9999987324100083358016192309006353329444L, 0, 0, 0);
  check_float ("tan (0xc.90fd8p-4) == 0.9999996860835706212861509874451585282616",  FUNC(tan) (0xc.90fd8p-4), 0.9999996860835706212861509874451585282616L, 0, 0, 0);
  check_float ("tan (0xc.90fdap-4) == 0.9999999245021033010474530133665235922808",  FUNC(tan) (0xc.90fdap-4), 0.9999999245021033010474530133665235922808L, 0, 0, 0);
  check_float ("tan (0xc.ap-4) == 1.0073556597407272165371804539701396631519",  FUNC(tan) (0xc.ap-4), 1.0073556597407272165371804539701396631519L, 0, 0, 0);
  check_float ("tan (0xc.98p-4) == 1.0034282930863044654045449407466962736255",  FUNC(tan) (0xc.98p-4), 1.0034282930863044654045449407466962736255L, 0, 0, 0);
  check_float ("tan (0xc.94p-4) == 1.0014703786820082237342656561856877993328",  FUNC(tan) (0xc.94p-4), 1.0014703786820082237342656561856877993328L, 0, 0, 0);
  check_float ("tan (0xc.92p-4) == 1.0004928571392300571266638743539017593717",  FUNC(tan) (0xc.92p-4), 1.0004928571392300571266638743539017593717L, 0, 0, 0);
  check_float ("tan (0xc.91p-4) == 1.0000044544650244953647966900221905361131",  FUNC(tan) (0xc.91p-4), 1.0000044544650244953647966900221905361131L, 0, 0, 0);
  check_float ("tan (0xc.90fep-4) == 1.0000006397580424009014454926842136804016",  FUNC(tan) (0xc.90fep-4), 1.0000006397580424009014454926842136804016L, 0, 0, 0);
  check_float ("tan (0xc.90fdcp-4) == 1.0000001629206928242190327320047489394217",  FUNC(tan) (0xc.90fdcp-4), 1.0000001629206928242190327320047489394217L, 0, 0, 0);
  check_float ("tan (0xc.90fdbp-4) == 1.0000000437113909572052640953950483705005",  FUNC(tan) (0xc.90fdbp-4), 1.0000000437113909572052640953950483705005L, 0, 0, 0);

  check_float ("tan (-0xc.9p-4) == -0.9995162902115457818029468900654150261381",  FUNC(tan) (-0xc.9p-4), -0.9995162902115457818029468900654150261381L, 0, 0, 0);
  check_float ("tan (-0xc.908p-4) == -0.9997603425502441410973077452249560802034",  FUNC(tan) (-0xc.908p-4), -0.9997603425502441410973077452249560802034L, 0, 0, 0);
  check_float ("tan (-0xc.90cp-4) == -0.9998823910588060302788513970802357770031",  FUNC(tan) (-0xc.90cp-4), -0.9998823910588060302788513970802357770031L, 0, 0, 0);
  check_float ("tan (-0xc.90ep-4) == -0.9999434208994808753305784795924711152508",  FUNC(tan) (-0xc.90ep-4), -0.9999434208994808753305784795924711152508L, 0, 0, 0);
  check_float ("tan (-0xc.90fp-4) == -0.9999739372166156702433266059635165160515",  FUNC(tan) (-0xc.90fp-4), -0.9999739372166156702433266059635165160515L, 0, 0, 0);
  check_float ("tan (-0xc.90f8p-4) == -0.9999891957244072765118898375645469865764",  FUNC(tan) (-0xc.90f8p-4), -0.9999891957244072765118898375645469865764L, 0, 0, 0);
  check_float ("tan (-0xc.90fcp-4) == -0.9999968250656122402859679132395522927393",  FUNC(tan) (-0xc.90fcp-4), -0.9999968250656122402859679132395522927393L, 0, 0, 0);
  check_float ("tan (-0xc.90fdp-4) == -0.9999987324100083358016192309006353329444",  FUNC(tan) (-0xc.90fdp-4), -0.9999987324100083358016192309006353329444L, 0, 0, 0);
  check_float ("tan (-0xc.90fd8p-4) == -0.9999996860835706212861509874451585282616",  FUNC(tan) (-0xc.90fd8p-4), -0.9999996860835706212861509874451585282616L, 0, 0, 0);
  check_float ("tan (-0xc.90fdap-4) == -0.9999999245021033010474530133665235922808",  FUNC(tan) (-0xc.90fdap-4), -0.9999999245021033010474530133665235922808L, 0, 0, 0);
  check_float ("tan (-0xc.ap-4) == -1.0073556597407272165371804539701396631519",  FUNC(tan) (-0xc.ap-4), -1.0073556597407272165371804539701396631519L, 0, 0, 0);
  check_float ("tan (-0xc.98p-4) == -1.0034282930863044654045449407466962736255",  FUNC(tan) (-0xc.98p-4), -1.0034282930863044654045449407466962736255L, 0, 0, 0);
  check_float ("tan (-0xc.94p-4) == -1.0014703786820082237342656561856877993328",  FUNC(tan) (-0xc.94p-4), -1.0014703786820082237342656561856877993328L, 0, 0, 0);
  check_float ("tan (-0xc.92p-4) == -1.0004928571392300571266638743539017593717",  FUNC(tan) (-0xc.92p-4), -1.0004928571392300571266638743539017593717L, 0, 0, 0);
  check_float ("tan (-0xc.91p-4) == -1.0000044544650244953647966900221905361131",  FUNC(tan) (-0xc.91p-4), -1.0000044544650244953647966900221905361131L, 0, 0, 0);
  check_float ("tan (-0xc.90fep-4) == -1.0000006397580424009014454926842136804016",  FUNC(tan) (-0xc.90fep-4), -1.0000006397580424009014454926842136804016L, 0, 0, 0);
  check_float ("tan (-0xc.90fdcp-4) == -1.0000001629206928242190327320047489394217",  FUNC(tan) (-0xc.90fdcp-4), -1.0000001629206928242190327320047489394217L, 0, 0, 0);
  check_float ("tan (-0xc.90fdbp-4) == -1.0000000437113909572052640953950483705005",  FUNC(tan) (-0xc.90fdbp-4), -1.0000000437113909572052640953950483705005L, 0, 0, 0);

#ifndef TEST_FLOAT
  check_float ("tan (1e22) == -1.628778225606898878549375936939548513545",  FUNC(tan) (1e22), -1.628778225606898878549375936939548513545L, DELTA4884, 0, 0);
  check_float ("tan (0x1p1023) == -0.6814476476066215012854144040167365190368",  FUNC(tan) (0x1p1023), -0.6814476476066215012854144040167365190368L, 0, 0, 0);
#endif

#if defined TEST_LDOUBLE && LDBL_MAX_EXP >= 16384
  check_float ("tan (0x1p16383) == 0.422722393732022337800504160054440141575",  FUNC(tan) (0x1p16383L), 0.422722393732022337800504160054440141575L, DELTA4886, 0, 0);
#endif

  print_max_error ("tan", DELTAtan, 0);
}


static void
tan_test_tonearest (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(tan) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_TONEAREST))
    {
  check_float ("tan_tonearest (1) == 1.5574077246549022305069748074583601730873",  FUNC(tan) (1), 1.5574077246549022305069748074583601730873L, DELTA4887, 0, 0);
  check_float ("tan_tonearest (2) == -2.1850398632615189916433061023136825434320",  FUNC(tan) (2), -2.1850398632615189916433061023136825434320L, DELTA4888, 0, 0);
  check_float ("tan_tonearest (3) == -0.1425465430742778052956354105339134932261",  FUNC(tan) (3), -0.1425465430742778052956354105339134932261L, 0, 0, 0);
  check_float ("tan_tonearest (4) == 1.1578212823495775831373424182673239231198",  FUNC(tan) (4), 1.1578212823495775831373424182673239231198L, 0, 0, 0);
  check_float ("tan_tonearest (5) == -3.3805150062465856369827058794473439087096",  FUNC(tan) (5), -3.3805150062465856369827058794473439087096L, 0, 0, 0);
  check_float ("tan_tonearest (6) == -0.2910061913847491570536995888681755428312",  FUNC(tan) (6), -0.2910061913847491570536995888681755428312L, DELTA4892, 0, 0);
  check_float ("tan_tonearest (7) == 0.8714479827243187364564508896003135663222",  FUNC(tan) (7), 0.8714479827243187364564508896003135663222L, 0, 0, 0);
  check_float ("tan_tonearest (8) == -6.7997114552203786999252627596086333648814",  FUNC(tan) (8), -6.7997114552203786999252627596086333648814L, DELTA4894, 0, 0);
  check_float ("tan_tonearest (9) == -0.4523156594418098405903708757987855343087",  FUNC(tan) (9), -0.4523156594418098405903708757987855343087L, DELTA4895, 0, 0);
  check_float ("tan_tonearest (10) == 0.6483608274590866712591249330098086768169",  FUNC(tan) (10), 0.6483608274590866712591249330098086768169L, 0, 0, 0);
    }

  fesetround (save_round_mode);

  print_max_error ("tan_tonearest", DELTAtan_tonearest, 0);
}


static void
tan_test_towardzero (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(tan) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_TOWARDZERO))
    {
  check_float ("tan_towardzero (1) == 1.5574077246549022305069748074583601730873",  FUNC(tan) (1), 1.5574077246549022305069748074583601730873L, DELTA4897, 0, 0);
  check_float ("tan_towardzero (2) == -2.1850398632615189916433061023136825434320",  FUNC(tan) (2), -2.1850398632615189916433061023136825434320L, DELTA4898, 0, 0);
  check_float ("tan_towardzero (3) == -0.1425465430742778052956354105339134932261",  FUNC(tan) (3), -0.1425465430742778052956354105339134932261L, DELTA4899, 0, 0);
  check_float ("tan_towardzero (4) == 1.1578212823495775831373424182673239231198",  FUNC(tan) (4), 1.1578212823495775831373424182673239231198L, DELTA4900, 0, 0);
  check_float ("tan_towardzero (5) == -3.3805150062465856369827058794473439087096",  FUNC(tan) (5), -3.3805150062465856369827058794473439087096L, DELTA4901, 0, 0);
  check_float ("tan_towardzero (6) == -0.2910061913847491570536995888681755428312",  FUNC(tan) (6), -0.2910061913847491570536995888681755428312L, DELTA4902, 0, 0);
  check_float ("tan_towardzero (7) == 0.8714479827243187364564508896003135663222",  FUNC(tan) (7), 0.8714479827243187364564508896003135663222L, 0, 0, 0);
  check_float ("tan_towardzero (8) == -6.7997114552203786999252627596086333648814",  FUNC(tan) (8), -6.7997114552203786999252627596086333648814L, DELTA4904, 0, 0);
  check_float ("tan_towardzero (9) == -0.4523156594418098405903708757987855343087",  FUNC(tan) (9), -0.4523156594418098405903708757987855343087L, DELTA4905, 0, 0);
  check_float ("tan_towardzero (10) == 0.6483608274590866712591249330098086768169",  FUNC(tan) (10), 0.6483608274590866712591249330098086768169L, DELTA4906, 0, 0);
    }

  fesetround (save_round_mode);

  print_max_error ("tan_towardzero", DELTAtan_towardzero, 0);
}


static void
tan_test_downward (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(tan) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_DOWNWARD))
    {
  check_float ("tan_downward (1) == 1.5574077246549022305069748074583601730873",  FUNC(tan) (1), 1.5574077246549022305069748074583601730873L, DELTA4907, 0, 0);
  check_float ("tan_downward (2) == -2.1850398632615189916433061023136825434320",  FUNC(tan) (2), -2.1850398632615189916433061023136825434320L, DELTA4908, 0, 0);
  check_float ("tan_downward (3) == -0.1425465430742778052956354105339134932261",  FUNC(tan) (3), -0.1425465430742778052956354105339134932261L, DELTA4909, 0, 0);
  check_float ("tan_downward (4) == 1.1578212823495775831373424182673239231198",  FUNC(tan) (4), 1.1578212823495775831373424182673239231198L, DELTA4910, 0, 0);
  check_float ("tan_downward (5) == -3.3805150062465856369827058794473439087096",  FUNC(tan) (5), -3.3805150062465856369827058794473439087096L, DELTA4911, 0, 0);
  check_float ("tan_downward (6) == -0.2910061913847491570536995888681755428312",  FUNC(tan) (6), -0.2910061913847491570536995888681755428312L, DELTA4912, 0, 0);
  check_float ("tan_downward (7) == 0.8714479827243187364564508896003135663222",  FUNC(tan) (7), 0.8714479827243187364564508896003135663222L, 0, 0, 0);
  check_float ("tan_downward (8) == -6.7997114552203786999252627596086333648814",  FUNC(tan) (8), -6.7997114552203786999252627596086333648814L, DELTA4914, 0, 0);
  check_float ("tan_downward (9) == -0.4523156594418098405903708757987855343087",  FUNC(tan) (9), -0.4523156594418098405903708757987855343087L, DELTA4915, 0, 0);
  check_float ("tan_downward (10) == 0.6483608274590866712591249330098086768169",  FUNC(tan) (10), 0.6483608274590866712591249330098086768169L, DELTA4916, 0, 0);
    }

  fesetround (save_round_mode);

  print_max_error ("tan_downward", DELTAtan_downward, 0);
}


static void
tan_test_upward (void)
{
  int save_round_mode;
  errno = 0;
  FUNC(tan) (0);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  save_round_mode = fegetround ();

  if (!fesetround (FE_UPWARD))
    {
  check_float ("tan_upward (1) == 1.5574077246549022305069748074583601730873",  FUNC(tan) (1), 1.5574077246549022305069748074583601730873L, DELTA4917, 0, 0);
  check_float ("tan_upward (2) == -2.1850398632615189916433061023136825434320",  FUNC(tan) (2), -2.1850398632615189916433061023136825434320L, DELTA4918, 0, 0);
  check_float ("tan_upward (3) == -0.1425465430742778052956354105339134932261",  FUNC(tan) (3), -0.1425465430742778052956354105339134932261L, DELTA4919, 0, 0);
  check_float ("tan_upward (4) == 1.1578212823495775831373424182673239231198",  FUNC(tan) (4), 1.1578212823495775831373424182673239231198L, 0, 0, 0);
  check_float ("tan_upward (5) == -3.3805150062465856369827058794473439087096",  FUNC(tan) (5), -3.3805150062465856369827058794473439087096L, DELTA4921, 0, 0);
  check_float ("tan_upward (6) == -0.2910061913847491570536995888681755428312",  FUNC(tan) (6), -0.2910061913847491570536995888681755428312L, DELTA4922, 0, 0);
  check_float ("tan_upward (7) == 0.8714479827243187364564508896003135663222",  FUNC(tan) (7), 0.8714479827243187364564508896003135663222L, DELTA4923, 0, 0);
  check_float ("tan_upward (8) == -6.7997114552203786999252627596086333648814",  FUNC(tan) (8), -6.7997114552203786999252627596086333648814L, DELTA4924, 0, 0);
  check_float ("tan_upward (9) == -0.4523156594418098405903708757987855343087",  FUNC(tan) (9), -0.4523156594418098405903708757987855343087L, DELTA4925, 0, 0);
  check_float ("tan_upward (10) == 0.6483608274590866712591249330098086768169",  FUNC(tan) (10), 0.6483608274590866712591249330098086768169L, DELTA4926, 0, 0);
    }

  fesetround (save_round_mode);

  print_max_error ("tan_upward", DELTAtan_upward, 0);
}


static void
tanh_test (void)
{
  errno = 0;
  FUNC(tanh) (0.7L);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_float ("tanh (0) == 0",  FUNC(tanh) (0), 0, 0, 0, 0);
  check_float ("tanh (-0) == -0",  FUNC(tanh) (minus_zero), minus_zero, 0, 0, 0);

#ifndef TEST_INLINE
  check_float ("tanh (inf) == 1",  FUNC(tanh) (plus_infty), 1, 0, 0, 0);
  check_float ("tanh (-inf) == -1",  FUNC(tanh) (minus_infty), -1, 0, 0, 0);
#endif
  check_float ("tanh (NaN) == NaN",  FUNC(tanh) (nan_value), nan_value, 0, 0, 0);

  check_float ("tanh (0.75) == 0.635148952387287319214434357312496495",  FUNC(tanh) (0.75L), 0.635148952387287319214434357312496495L, 0, 0, 0);
  check_float ("tanh (-0.75) == -0.635148952387287319214434357312496495",  FUNC(tanh) (-0.75L), -0.635148952387287319214434357312496495L, 0, 0, 0);

  check_float ("tanh (1.0) == 0.7615941559557648881194582826047935904",  FUNC(tanh) (1.0L), 0.7615941559557648881194582826047935904L, 0, 0, 0);
  check_float ("tanh (-1.0) == -0.7615941559557648881194582826047935904",  FUNC(tanh) (-1.0L), -0.7615941559557648881194582826047935904L, 0, 0, 0);

  /* 2^-57  */
  check_float ("tanh (0x1p-57) == 6.938893903907228377647697925567626953125e-18",  FUNC(tanh) (0x1p-57L), 6.938893903907228377647697925567626953125e-18L, 0, 0, 0);

  print_max_error ("tanh", 0, 0);
}

static void
tgamma_test (void)
{
  errno = 0;
  FUNC(tgamma) (1);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  init_max_error ();

  check_float ("tgamma (inf) == inf",  FUNC(tgamma) (plus_infty), plus_infty, 0, 0, 0);
  check_float ("tgamma (max_value) == inf",  FUNC(tgamma) (max_value), plus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_float ("tgamma (0) == inf",  FUNC(tgamma) (0), plus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  check_float ("tgamma (-0) == -inf",  FUNC(tgamma) (minus_zero), minus_infty, 0, 0, DIVIDE_BY_ZERO_EXCEPTION);
  /* tgamma (x) == NaN plus invalid exception for integer x <= 0.  */
  check_float ("tgamma (-2) == NaN",  FUNC(tgamma) (-2), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("tgamma (-max_value) == NaN",  FUNC(tgamma) (-max_value), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("tgamma (-inf) == NaN",  FUNC(tgamma) (minus_infty), nan_value, 0, 0, INVALID_EXCEPTION);
  check_float ("tgamma (NaN) == NaN",  FUNC(tgamma) (nan_value), nan_value, 0, 0, 0);

  check_float ("tgamma (0.5) == sqrt (pi)",  FUNC(tgamma) (0.5), M_SQRT_PIl, DELTA4945, 0, 0);
  check_float ("tgamma (-0.5) == -2 sqrt (pi)",  FUNC(tgamma) (-0.5), -M_2_SQRT_PIl, DELTA4946, 0, 0);

  check_float ("tgamma (1) == 1",  FUNC(tgamma) (1), 1, 0, 0, 0);
  check_float ("tgamma (4) == 6",  FUNC(tgamma) (4), 6, DELTA4948, 0, 0);

  check_float ("tgamma (0.7) == 1.29805533264755778568117117915281162",  FUNC(tgamma) (0.7L), 1.29805533264755778568117117915281162L, DELTA4949, 0, 0);
  check_float ("tgamma (1.2) == 0.918168742399760610640951655185830401",  FUNC(tgamma) (1.2L), 0.918168742399760610640951655185830401L, 0, 0, 0);

  print_max_error ("tgamma", DELTAtgamma, 0);
}


static void
trunc_test (void)
{
  init_max_error ();

  check_float ("trunc (inf) == inf",  FUNC(trunc) (plus_infty), plus_infty, 0, 0, 0);
  check_float ("trunc (-inf) == -inf",  FUNC(trunc) (minus_infty), minus_infty, 0, 0, 0);
  check_float ("trunc (NaN) == NaN",  FUNC(trunc) (nan_value), nan_value, 0, 0, 0);

  check_float ("trunc (0) == 0",  FUNC(trunc) (0), 0, 0, 0, 0);
  check_float ("trunc (-0) == -0",  FUNC(trunc) (minus_zero), minus_zero, 0, 0, 0);
  check_float ("trunc (0.1) == 0",  FUNC(trunc) (0.1), 0, 0, 0, 0);
  check_float ("trunc (0.25) == 0",  FUNC(trunc) (0.25), 0, 0, 0, 0);
  check_float ("trunc (0.625) == 0",  FUNC(trunc) (0.625), 0, 0, 0, 0);
  check_float ("trunc (-0.1) == -0",  FUNC(trunc) (-0.1), minus_zero, 0, 0, 0);
  check_float ("trunc (-0.25) == -0",  FUNC(trunc) (-0.25), minus_zero, 0, 0, 0);
  check_float ("trunc (-0.625) == -0",  FUNC(trunc) (-0.625), minus_zero, 0, 0, 0);
  check_float ("trunc (1) == 1",  FUNC(trunc) (1), 1, 0, 0, 0);
  check_float ("trunc (-1) == -1",  FUNC(trunc) (-1), -1, 0, 0, 0);
  check_float ("trunc (1.625) == 1",  FUNC(trunc) (1.625), 1, 0, 0, 0);
  check_float ("trunc (-1.625) == -1",  FUNC(trunc) (-1.625), -1, 0, 0, 0);

  check_float ("trunc (1048580.625) == 1048580",  FUNC(trunc) (1048580.625L), 1048580L, 0, 0, 0);
  check_float ("trunc (-1048580.625) == -1048580",  FUNC(trunc) (-1048580.625L), -1048580L, 0, 0, 0);

  check_float ("trunc (8388610.125) == 8388610.0",  FUNC(trunc) (8388610.125L), 8388610.0L, 0, 0, 0);
  check_float ("trunc (-8388610.125) == -8388610.0",  FUNC(trunc) (-8388610.125L), -8388610.0L, 0, 0, 0);

  check_float ("trunc (4294967296.625) == 4294967296.0",  FUNC(trunc) (4294967296.625L), 4294967296.0L, 0, 0, 0);
  check_float ("trunc (-4294967296.625) == -4294967296.0",  FUNC(trunc) (-4294967296.625L), -4294967296.0L, 0, 0, 0);

#ifdef TEST_LDOUBLE
  /* The result can only be represented in long double.  */
  check_float ("trunc (4503599627370495.5) == 4503599627370495.0",  FUNC(trunc) (4503599627370495.5L), 4503599627370495.0L, 0, 0, 0);
  check_float ("trunc (4503599627370496.25) == 4503599627370496.0",  FUNC(trunc) (4503599627370496.25L), 4503599627370496.0L, 0, 0, 0);
  check_float ("trunc (4503599627370496.5) == 4503599627370496.0",  FUNC(trunc) (4503599627370496.5L), 4503599627370496.0L, 0, 0, 0);
  check_float ("trunc (4503599627370496.75) == 4503599627370496.0",  FUNC(trunc) (4503599627370496.75L), 4503599627370496.0L, 0, 0, 0);
  check_float ("trunc (4503599627370497.5) == 4503599627370497.0",  FUNC(trunc) (4503599627370497.5L), 4503599627370497.0L, 0, 0, 0);

# if LDBL_MANT_DIG > 100
  check_float ("trunc (4503599627370494.5000000000001) == 4503599627370494.0",  FUNC(trunc) (4503599627370494.5000000000001L), 4503599627370494.0L, 0, 0, 0);
  check_float ("trunc (4503599627370495.5000000000001) == 4503599627370495.0",  FUNC(trunc) (4503599627370495.5000000000001L), 4503599627370495.0L, 0, 0, 0);
  check_float ("trunc (4503599627370496.5000000000001) == 4503599627370496.0",  FUNC(trunc) (4503599627370496.5000000000001L), 4503599627370496.0L, 0, 0, 0);
# endif

  check_float ("trunc (-4503599627370495.5) == -4503599627370495.0",  FUNC(trunc) (-4503599627370495.5L), -4503599627370495.0L, 0, 0, 0);
  check_float ("trunc (-4503599627370496.25) == -4503599627370496.0",  FUNC(trunc) (-4503599627370496.25L), -4503599627370496.0L, 0, 0, 0);
  check_float ("trunc (-4503599627370496.5) == -4503599627370496.0",  FUNC(trunc) (-4503599627370496.5L), -4503599627370496.0L, 0, 0, 0);
  check_float ("trunc (-4503599627370496.75) == -4503599627370496.0",  FUNC(trunc) (-4503599627370496.75L), -4503599627370496.0L, 0, 0, 0);
  check_float ("trunc (-4503599627370497.5) == -4503599627370497.0",  FUNC(trunc) (-4503599627370497.5L), -4503599627370497.0L, 0, 0, 0);

# if LDBL_MANT_DIG > 100
  check_float ("trunc (-4503599627370494.5000000000001) == -4503599627370494.0",  FUNC(trunc) (-4503599627370494.5000000000001L), -4503599627370494.0L, 0, 0, 0);
  check_float ("trunc (-4503599627370495.5000000000001) == -4503599627370495.0",  FUNC(trunc) (-4503599627370495.5000000000001L), -4503599627370495.0L, 0, 0, 0);
  check_float ("trunc (-4503599627370496.5000000000001) == -4503599627370496.0",  FUNC(trunc) (-4503599627370496.5000000000001L), -4503599627370496.0L, 0, 0, 0);
# endif

  check_float ("trunc (9007199254740991.5) == 9007199254740991.0",  FUNC(trunc) (9007199254740991.5L), 9007199254740991.0L, 0, 0, 0);
  check_float ("trunc (9007199254740992.25) == 9007199254740992.0",  FUNC(trunc) (9007199254740992.25L), 9007199254740992.0L, 0, 0, 0);
  check_float ("trunc (9007199254740992.5) == 9007199254740992.0",  FUNC(trunc) (9007199254740992.5L), 9007199254740992.0L, 0, 0, 0);
  check_float ("trunc (9007199254740992.75) == 9007199254740992.0",  FUNC(trunc) (9007199254740992.75L), 9007199254740992.0L, 0, 0, 0);
  check_float ("trunc (9007199254740993.5) == 9007199254740993.0",  FUNC(trunc) (9007199254740993.5L), 9007199254740993.0L, 0, 0, 0);

# if LDBL_MANT_DIG > 100
  check_float ("trunc (9007199254740991.0000000000001) == 9007199254740991.0",  FUNC(trunc) (9007199254740991.0000000000001L), 9007199254740991.0L, 0, 0, 0);
  check_float ("trunc (9007199254740992.0000000000001) == 9007199254740992.0",  FUNC(trunc) (9007199254740992.0000000000001L), 9007199254740992.0L, 0, 0, 0);
  check_float ("trunc (9007199254740993.0000000000001) == 9007199254740993.0",  FUNC(trunc) (9007199254740993.0000000000001L), 9007199254740993.0L, 0, 0, 0);
  check_float ("trunc (9007199254740991.5000000000001) == 9007199254740991.0",  FUNC(trunc) (9007199254740991.5000000000001L), 9007199254740991.0L, 0, 0, 0);
  check_float ("trunc (9007199254740992.5000000000001) == 9007199254740992.0",  FUNC(trunc) (9007199254740992.5000000000001L), 9007199254740992.0L, 0, 0, 0);
  check_float ("trunc (9007199254740993.5000000000001) == 9007199254740993.0",  FUNC(trunc) (9007199254740993.5000000000001L), 9007199254740993.0L, 0, 0, 0);
# endif

  check_float ("trunc (-9007199254740991.5) == -9007199254740991.0",  FUNC(trunc) (-9007199254740991.5L), -9007199254740991.0L, 0, 0, 0);
  check_float ("trunc (-9007199254740992.25) == -9007199254740992.0",  FUNC(trunc) (-9007199254740992.25L), -9007199254740992.0L, 0, 0, 0);
  check_float ("trunc (-9007199254740992.5) == -9007199254740992.0",  FUNC(trunc) (-9007199254740992.5L), -9007199254740992.0L, 0, 0, 0);
  check_float ("trunc (-9007199254740992.75) == -9007199254740992.0",  FUNC(trunc) (-9007199254740992.75L), -9007199254740992.0L, 0, 0, 0);
  check_float ("trunc (-9007199254740993.5) == -9007199254740993.0",  FUNC(trunc) (-9007199254740993.5L), -9007199254740993.0L, 0, 0, 0);

# if LDBL_MANT_DIG > 100
  check_float ("trunc (-9007199254740991.0000000000001) == -9007199254740991.0",  FUNC(trunc) (-9007199254740991.0000000000001L), -9007199254740991.0L, 0, 0, 0);
  check_float ("trunc (-9007199254740992.0000000000001) == -9007199254740992.0",  FUNC(trunc) (-9007199254740992.0000000000001L), -9007199254740992.0L, 0, 0, 0);
  check_float ("trunc (-9007199254740993.0000000000001) == -9007199254740993.0",  FUNC(trunc) (-9007199254740993.0000000000001L), -9007199254740993.0L, 0, 0, 0);
  check_float ("trunc (-9007199254740991.5000000000001) == -9007199254740991.0",  FUNC(trunc) (-9007199254740991.5000000000001L), -9007199254740991.0L, 0, 0, 0);
  check_float ("trunc (-9007199254740992.5000000000001) == -9007199254740992.0",  FUNC(trunc) (-9007199254740992.5000000000001L), -9007199254740992.0L, 0, 0, 0);
  check_float ("trunc (-9007199254740993.5000000000001) == -9007199254740993.0",  FUNC(trunc) (-9007199254740993.5000000000001L), -9007199254740993.0L, 0, 0, 0);
# endif

  check_float ("trunc (72057594037927935.5) == 72057594037927935.0",  FUNC(trunc) (72057594037927935.5L), 72057594037927935.0L, 0, 0, 0);
  check_float ("trunc (72057594037927936.25) == 72057594037927936.0",  FUNC(trunc) (72057594037927936.25L), 72057594037927936.0L, 0, 0, 0);
  check_float ("trunc (72057594037927936.5) == 72057594037927936.0",  FUNC(trunc) (72057594037927936.5L), 72057594037927936.0L, 0, 0, 0);
  check_float ("trunc (72057594037927936.75) == 72057594037927936.0",  FUNC(trunc) (72057594037927936.75L), 72057594037927936.0L, 0, 0, 0);
  check_float ("trunc (72057594037927937.5) == 72057594037927937.0",  FUNC(trunc) (72057594037927937.5L), 72057594037927937.0L, 0, 0, 0);

  check_float ("trunc (-72057594037927935.5) == -72057594037927935.0",  FUNC(trunc) (-72057594037927935.5L), -72057594037927935.0L, 0, 0, 0);
  check_float ("trunc (-72057594037927936.25) == -72057594037927936.0",  FUNC(trunc) (-72057594037927936.25L), -72057594037927936.0L, 0, 0, 0);
  check_float ("trunc (-72057594037927936.5) == -72057594037927936.0",  FUNC(trunc) (-72057594037927936.5L), -72057594037927936.0L, 0, 0, 0);
  check_float ("trunc (-72057594037927936.75) == -72057594037927936.0",  FUNC(trunc) (-72057594037927936.75L), -72057594037927936.0L, 0, 0, 0);
  check_float ("trunc (-72057594037927937.5) == -72057594037927937.0",  FUNC(trunc) (-72057594037927937.5L), -72057594037927937.0L, 0, 0, 0);

  check_float ("trunc (10141204801825835211973625643007.5) == 10141204801825835211973625643007.0",  FUNC(trunc) (10141204801825835211973625643007.5L), 10141204801825835211973625643007.0L, 0, 0, 0);
  check_float ("trunc (10141204801825835211973625643008.25) == 10141204801825835211973625643008.0",  FUNC(trunc) (10141204801825835211973625643008.25L), 10141204801825835211973625643008.0L, 0, 0, 0);
  check_float ("trunc (10141204801825835211973625643008.5) == 10141204801825835211973625643008.0",  FUNC(trunc) (10141204801825835211973625643008.5L), 10141204801825835211973625643008.0L, 0, 0, 0);
  check_float ("trunc (10141204801825835211973625643008.75) == 10141204801825835211973625643008.0",  FUNC(trunc) (10141204801825835211973625643008.75L), 10141204801825835211973625643008.0L, 0, 0, 0);
  check_float ("trunc (10141204801825835211973625643009.5) == 10141204801825835211973625643009.0",  FUNC(trunc) (10141204801825835211973625643009.5L), 10141204801825835211973625643009.0L, 0, 0, 0);
#endif

  print_max_error ("trunc", 0, 0);
}

static void
y0_test (void)
{
  FLOAT s, c;
  errno = 0;
  FUNC (sincos) (0, &s, &c);
  if (errno == ENOSYS)
    /* Required function not implemented.  */
    return;
  FUNC(y0) (1);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  /* y0 is the Bessel function of the second kind of order 0 */
  init_max_error ();

  check_float ("y0 (-1.0) == -inf",  FUNC(y0) (-1.0), minus_infty, 0, 0, INVALID_EXCEPTION);
  check_float ("y0 (-max_value) == -inf",  FUNC(y0) (-max_value), minus_infty, 0, 0, INVALID_EXCEPTION);
  check_float ("y0 (0.0) == -inf",  FUNC(y0) (0.0), minus_infty, 0, 0, 0);
  check_float ("y0 (NaN) == NaN",  FUNC(y0) (nan_value), nan_value, 0, 0, 0);
  check_float ("y0 (inf) == 0",  FUNC(y0) (plus_infty), 0, 0, 0, 0);

  check_float ("y0 (0.125) == -1.38968062514384052915582277745018693",  FUNC(y0) (0.125L), -1.38968062514384052915582277745018693L, DELTA5030, 0, 0);
  check_float ("y0 (0.75) == -0.137172769385772397522814379396581855",  FUNC(y0) (0.75L), -0.137172769385772397522814379396581855L, 0, 0, 0);
  check_float ("y0 (1.0) == 0.0882569642156769579829267660235151628",  FUNC(y0) (1.0), 0.0882569642156769579829267660235151628L, DELTA5032, 0, 0);
  check_float ("y0 (1.5) == 0.382448923797758843955068554978089862",  FUNC(y0) (1.5), 0.382448923797758843955068554978089862L, DELTA5033, 0, 0);
  check_float ("y0 (2.0) == 0.510375672649745119596606592727157873",  FUNC(y0) (2.0), 0.510375672649745119596606592727157873L, 0, 0, 0);
  check_float ("y0 (8.0) == 0.223521489387566220527323400498620359",  FUNC(y0) (8.0), 0.223521489387566220527323400498620359L, DELTA5035, 0, 0);
  check_float ("y0 (10.0) == 0.0556711672835993914244598774101900481",  FUNC(y0) (10.0), 0.0556711672835993914244598774101900481L, DELTA5036, 0, 0);

  check_float ("y0 (0x1.3ffp+74) == 1.818984347516051243459467456433028748678e-12",  FUNC(y0) (0x1.3ffp+74L), 1.818984347516051243459467456433028748678e-12L, DELTA5037, 0, 0);

#ifndef TEST_FLOAT
  /* Bug 14155: spurious exception may occur.  */
  check_float ("y0 (0x1.ff00000000002p+840) == 1.846591691699331493194965158699937660696e-127",  FUNC(y0) (0x1.ff00000000002p+840L), 1.846591691699331493194965158699937660696e-127L, DELTA5038, 0, UNDERFLOW_EXCEPTION_OK);
#endif

  check_float ("y0 (0x1p-10) == -4.4865150767109739412411806297168793661098",  FUNC(y0) (0x1p-10L), -4.4865150767109739412411806297168793661098L, DELTA5039, 0, 0);
  check_float ("y0 (0x1p-20) == -8.8992283012125827603076426611387876938160",  FUNC(y0) (0x1p-20L), -8.8992283012125827603076426611387876938160L, DELTA5040, 0, 0);
  check_float ("y0 (0x1p-30) == -1.3311940304267782826037118027401817264906e+1",  FUNC(y0) (0x1p-30L), -1.3311940304267782826037118027401817264906e+1L, DELTA5041, 0, 0);
  check_float ("y0 (0x1p-40) == -1.7724652307320814696990854700366226762563e+1",  FUNC(y0) (0x1p-40L), -1.7724652307320814696990854700366226762563e+1L, DELTA5042, 0, 0);
  check_float ("y0 (0x1p-50) == -2.2137364310373846564919987139743760738155e+1",  FUNC(y0) (0x1p-50L), -2.2137364310373846564919987139743760738155e+1L, DELTA5043, 0, 0);
  check_float ("y0 (0x1p-60) == -2.6550076313426878432849115782108205929120e+1",  FUNC(y0) (0x1p-60L), -2.6550076313426878432849115782108205929120e+1L, 0, 0, 0);
  check_float ("y0 (0x1p-70) == -3.0962788316479910300778244424468159753887e+1",  FUNC(y0) (0x1p-70L), -3.0962788316479910300778244424468159753887e+1L, DELTA5045, 0, 0);
  check_float ("y0 (0x1p-80) == -3.5375500319532942168707373066828113573541e+1",  FUNC(y0) (0x1p-80L), -3.5375500319532942168707373066828113573541e+1L, DELTA5046, 0, 0);
  check_float ("y0 (0x1p-90) == -3.9788212322585974036636501709188067393195e+1",  FUNC(y0) (0x1p-90L), -3.9788212322585974036636501709188067393195e+1L, 0, 0, 0);
  check_float ("y0 (0x1p-100) == -4.420092432563900590456563035154802121284e+1",  FUNC(y0) (0x1p-100L), -4.420092432563900590456563035154802121284e+1L, 0, 0, 0);
  check_float ("y0 (0x1p-110) == -4.861363632869203777249475899390797503250e+1",  FUNC(y0) (0x1p-110L), -4.861363632869203777249475899390797503250e+1L, DELTA5049, 0, 0);

  print_max_error ("y0", DELTAy0, 0);
}


static void
y1_test (void)
{
  FLOAT s, c;
  errno = 0;
  FUNC (sincos) (0, &s, &c);
  if (errno == ENOSYS)
    /* Required function not implemented.  */
    return;
  FUNC(y1) (1);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  /* y1 is the Bessel function of the second kind of order 1 */
  init_max_error ();

  check_float ("y1 (-1.0) == -inf",  FUNC(y1) (-1.0), minus_infty, 0, 0, INVALID_EXCEPTION);
  check_float ("y1 (-max_value) == -inf",  FUNC(y1) (-max_value), minus_infty, 0, 0, INVALID_EXCEPTION);
  check_float ("y1 (0.0) == -inf",  FUNC(y1) (0.0), minus_infty, 0, 0, 0);
  check_float ("y1 (inf) == 0",  FUNC(y1) (plus_infty), 0, 0, 0, 0);
  check_float ("y1 (NaN) == NaN",  FUNC(y1) (nan_value), nan_value, 0, 0, 0);

  check_float ("y1 (0.125) == -5.19993611253477499595928744876579921",  FUNC(y1) (0.125L), -5.19993611253477499595928744876579921L, DELTA5055, 0, 0);
  check_float ("y1 (0.75) == -1.03759455076928541973767132140642198",  FUNC(y1) (0.75L), -1.03759455076928541973767132140642198L, 0, 0, 0);
  check_float ("y1 (1.0) == -0.781212821300288716547150000047964821",  FUNC(y1) (1.0), -0.781212821300288716547150000047964821L, 0, 0, 0);
  check_float ("y1 (1.5) == -0.412308626973911295952829820633445323",  FUNC(y1) (1.5), -0.412308626973911295952829820633445323L, DELTA5058, 0, 0);
  check_float ("y1 (2.0) == -0.107032431540937546888370772277476637",  FUNC(y1) (2.0), -0.107032431540937546888370772277476637L, DELTA5059, 0, 0);
  check_float ("y1 (8.0) == -0.158060461731247494255555266187483550",  FUNC(y1) (8.0), -0.158060461731247494255555266187483550L, DELTA5060, 0, 0);
  check_float ("y1 (10.0) == 0.249015424206953883923283474663222803",  FUNC(y1) (10.0), 0.249015424206953883923283474663222803L, DELTA5061, 0, 0);

  /* Bug 14155: spurious exception may occur.  */
  check_float ("y1 (0x1.27e204p+99) == -8.881610148467797208469612080785210013461e-16",  FUNC(y1) (0x1.27e204p+99L), -8.881610148467797208469612080785210013461e-16L, DELTA5062, 0, UNDERFLOW_EXCEPTION_OK);

#ifndef TEST_FLOAT
  /* Bug 14155: spurious exception may occur.  */
  check_float ("y1 (0x1.001000001p+593) == 3.927269966354206207832593635798954916263e-90",  FUNC(y1) (0x1.001000001p+593L), 3.927269966354206207832593635798954916263e-90L, DELTA5063, 0, UNDERFLOW_EXCEPTION_OK);
#endif

  check_float ("y1 (0x1p-10) == -6.5190099301063115047395187618929589514382e+02",  FUNC(y1) (0x1p-10L), -6.5190099301063115047395187618929589514382e+02L, DELTA5064, 0, 0);
  check_float ("y1 (0x1p-20) == -6.6754421443450423911167962313100637952285e+05",  FUNC(y1) (0x1p-20L), -6.6754421443450423911167962313100637952285e+05L, 0, 0, 0);
  check_float ("y1 (0x1p-30) == -6.8356527557643159612937462812258975438856e+08",  FUNC(y1) (0x1p-30L), -6.8356527557643159612937462812258975438856e+08L, 0, 0, 0);
  check_float ("y1 (0x1p-40) == -6.9997084219026594793707805777425993547887e+11",  FUNC(y1) (0x1p-40L), -6.9997084219026594793707805777425993547887e+11L, 0, 0, 0);
  check_float ("y1 (0x1p-50) == -7.1677014240283233068755952926181262431559e+14",  FUNC(y1) (0x1p-50L), -7.1677014240283233068755952926181262431559e+14L, 0, 0, 0);
  check_float ("y1 (0x1p-60) == -7.3397262582050030662406095795388448059822e+17",  FUNC(y1) (0x1p-60L), -7.3397262582050030662406095795388448059822e+17L, 0, 0, 0);
  check_float ("y1 (0x1p-70) == -7.5158796884019231398303842094477769620063e+20",  FUNC(y1) (0x1p-70L), -7.5158796884019231398303842094477769620063e+20L, 0, 0, 0);
  check_float ("y1 (0x1p-80) == -7.6962608009235692951863134304745236090943e+23",  FUNC(y1) (0x1p-80L), -7.6962608009235692951863134304745236090943e+23L, 0, 0, 0);
  check_float ("y1 (0x1p-90) == -7.8809710601457349582707849528059121757126e+26",  FUNC(y1) (0x1p-90L), -7.8809710601457349582707849528059121757126e+26L, 0, 0, 0);
  check_float ("y1 (0x1p-100) == -8.0701143655892325972692837916732540679297e+29",  FUNC(y1) (0x1p-100L), -8.0701143655892325972692837916732540679297e+29L, 0, 0, 0);
  check_float ("y1 (0x1p-110) == -8.2637971103633741796037466026734121655600e+32",  FUNC(y1) (0x1p-110L), -8.2637971103633741796037466026734121655600e+32L, 0, 0, 0);

  print_max_error ("y1", DELTAy1, 0);
}


static void
yn_test (void)
{
  FLOAT s, c;
  errno = 0;
  FUNC (sincos) (0, &s, &c);
  if (errno == ENOSYS)
    /* Required function not implemented.  */
    return;
  FUNC(yn) (1, 1);
  if (errno == ENOSYS)
    /* Function not implemented.  */
    return;

  /* yn is the Bessel function of the second kind of order n */
  init_max_error ();

  /* yn (0, x) == y0 (x)  */
  check_float ("yn (0, -1.0) == -inf",  FUNC(yn) (0, -1.0), minus_infty, 0, 0, INVALID_EXCEPTION);
  check_float ("yn (0, -max_value) == -inf",  FUNC(yn) (0, -max_value), minus_infty, 0, 0, INVALID_EXCEPTION);
  check_float ("yn (0, 0.0) == -inf",  FUNC(yn) (0, 0.0), minus_infty, 0, 0, 0);
  check_float ("yn (0, NaN) == NaN",  FUNC(yn) (0, nan_value), nan_value, 0, 0, 0);
  check_float ("yn (0, inf) == 0",  FUNC(yn) (0, plus_infty), 0, 0, 0, 0);

  check_float ("yn (0, 0.125) == -1.38968062514384052915582277745018693",  FUNC(yn) (0, 0.125L), -1.38968062514384052915582277745018693L, DELTA5080, 0, 0);
  check_float ("yn (0, 0.75) == -0.137172769385772397522814379396581855",  FUNC(yn) (0, 0.75L), -0.137172769385772397522814379396581855L, 0, 0, 0);
  check_float ("yn (0, 1.0) == 0.0882569642156769579829267660235151628",  FUNC(yn) (0, 1.0), 0.0882569642156769579829267660235151628L, DELTA5082, 0, 0);
  check_float ("yn (0, 1.5) == 0.382448923797758843955068554978089862",  FUNC(yn) (0, 1.5), 0.382448923797758843955068554978089862L, DELTA5083, 0, 0);
  check_float ("yn (0, 2.0) == 0.510375672649745119596606592727157873",  FUNC(yn) (0, 2.0), 0.510375672649745119596606592727157873L, 0, 0, 0);
  check_float ("yn (0, 8.0) == 0.223521489387566220527323400498620359",  FUNC(yn) (0, 8.0), 0.223521489387566220527323400498620359L, DELTA5085, 0, 0);
  check_float ("yn (0, 10.0) == 0.0556711672835993914244598774101900481",  FUNC(yn) (0, 10.0), 0.0556711672835993914244598774101900481L, DELTA5086, 0, 0);

  /* yn (1, x) == y1 (x)  */
  check_float ("yn (1, -1.0) == -inf",  FUNC(yn) (1, -1.0), minus_infty, 0, 0, INVALID_EXCEPTION);
  check_float ("yn (1, 0.0) == -inf",  FUNC(yn) (1, 0.0), minus_infty, 0, 0, 0);
  check_float ("yn (1, inf) == 0",  FUNC(yn) (1, plus_infty), 0, 0, 0, 0);
  check_float ("yn (1, NaN) == NaN",  FUNC(yn) (1, nan_value), nan_value, 0, 0, 0);

  check_float ("yn (1, 0.125) == -5.19993611253477499595928744876579921",  FUNC(yn) (1, 0.125L), -5.19993611253477499595928744876579921L, DELTA5091, 0, 0);
  check_float ("yn (1, 0.75) == -1.03759455076928541973767132140642198",  FUNC(yn) (1, 0.75L), -1.03759455076928541973767132140642198L, 0, 0, 0);
  check_float ("yn (1, 1.0) == -0.781212821300288716547150000047964821",  FUNC(yn) (1, 1.0), -0.781212821300288716547150000047964821L, 0, 0, 0);
  check_float ("yn (1, 1.5) == -0.412308626973911295952829820633445323",  FUNC(yn) (1, 1.5), -0.412308626973911295952829820633445323L, DELTA5094, 0, 0);
  check_float ("yn (1, 2.0) == -0.107032431540937546888370772277476637",  FUNC(yn) (1, 2.0), -0.107032431540937546888370772277476637L, DELTA5095, 0, 0);
  check_float ("yn (1, 8.0) == -0.158060461731247494255555266187483550",  FUNC(yn) (1, 8.0), -0.158060461731247494255555266187483550L, DELTA5096, 0, 0);
  check_float ("yn (1, 10.0) == 0.249015424206953883923283474663222803",  FUNC(yn) (1, 10.0), 0.249015424206953883923283474663222803L, DELTA5097, 0, 0);

  /* yn (3, x)  */
  check_float ("yn (3, inf) == 0",  FUNC(yn) (3, plus_infty), 0, 0, 0, 0);
  check_float ("yn (3, NaN) == NaN",  FUNC(yn) (3, nan_value), nan_value, 0, 0, 0);

  check_float ("yn (3, 0.125) == -2612.69757350066712600220955744091741",  FUNC(yn) (3, 0.125L), -2612.69757350066712600220955744091741L, DELTA5100, 0, 0);
  check_float ("yn (3, 0.75) == -12.9877176234475433186319774484809207",  FUNC(yn) (3, 0.75L), -12.9877176234475433186319774484809207L, DELTA5101, 0, 0);
  check_float ("yn (3, 1.0) == -5.82151760596472884776175706442981440",  FUNC(yn) (3, 1.0), -5.82151760596472884776175706442981440L, 0, 0, 0);
  check_float ("yn (3, 2.0) == -1.12778377684042778608158395773179238",  FUNC(yn) (3, 2.0), -1.12778377684042778608158395773179238L, DELTA5103, 0, 0);
  check_float ("yn (3, 10.0) == -0.251362657183837329779204747654240998",  FUNC(yn) (3, 10.0), -0.251362657183837329779204747654240998L, DELTA5104, 0, 0);

  /* yn (10, x)  */
  check_float ("yn (10, inf) == 0",  FUNC(yn) (10, plus_infty), 0, 0, 0, 0);
  check_float ("yn (10, NaN) == NaN",  FUNC(yn) (10, nan_value), nan_value, 0, 0, 0);

  check_float ("yn (10, 0.125) == -127057845771019398.252538486899753195",  FUNC(yn) (10, 0.125L), -127057845771019398.252538486899753195L, DELTA5107, 0, 0);
  check_float ("yn (10, 0.75) == -2133501638.90573424452445412893839236",  FUNC(yn) (10, 0.75L), -2133501638.90573424452445412893839236L, DELTA5108, 0, 0);
  check_float ("yn (10, 1.0) == -121618014.278689189288130426667971145",  FUNC(yn) (10, 1.0), -121618014.278689189288130426667971145L, DELTA5109, 0, 0);
  check_float ("yn (10, 2.0) == -129184.542208039282635913145923304214",  FUNC(yn) (10, 2.0), -129184.542208039282635913145923304214L, DELTA5110, 0, 0);
  check_float ("yn (10, 10.0) == -0.359814152183402722051986577343560609",  FUNC(yn) (10, 10.0), -0.359814152183402722051986577343560609L, DELTA5111, 0, 0);

  /* Check whether yn returns correct value for LDBL_MIN, DBL_MIN,
     and FLT_MIN.  See Bug 14173.  */
  check_float ("yn (10, min_value) == -inf",  FUNC(yn) (10, min_value), minus_infty, 0, 0, OVERFLOW_EXCEPTION);

  errno = 0;
  check_float ("yn (10, min_value) == -inf",  FUNC(yn) (10, min_value), minus_infty, 0, 0, OVERFLOW_EXCEPTION);
  check_int ("errno for yn(10,-min) == ERANGE", errno, ERANGE, 0, 0, 0);

  print_max_error ("yn", DELTAyn, 0);
}


static void
significand_test (void)
{
  /* significand returns the mantissa of the exponential representation.  */
  init_max_error ();

  check_float ("significand (4.0) == 1.0",  FUNC(significand) (4.0), 1.0, 0, 0, 0);
  check_float ("significand (6.0) == 1.5",  FUNC(significand) (6.0), 1.5, 0, 0, 0);
  check_float ("significand (8.0) == 1.0",  FUNC(significand) (8.0), 1.0, 0, 0, 0);

  print_max_error ("significand", 0, 0);
}


static void
initialize (void)
{
  fpstack_test ("start *init*");
  plus_zero = 0.0;
  nan_value = plus_zero / plus_zero;	/* Suppress GCC warning */

  minus_zero = FUNC(copysign) (0.0, -1.0);
  plus_infty = CHOOSE (HUGE_VALL, HUGE_VAL, HUGE_VALF,
		       HUGE_VALL, HUGE_VAL, HUGE_VALF);
  minus_infty = CHOOSE (-HUGE_VALL, -HUGE_VAL, -HUGE_VALF,
			-HUGE_VALL, -HUGE_VAL, -HUGE_VALF);
  max_value = CHOOSE (LDBL_MAX, DBL_MAX, FLT_MAX,
		      LDBL_MAX, DBL_MAX, FLT_MAX);
  min_value = CHOOSE (LDBL_MIN, DBL_MIN, FLT_MIN,
		      LDBL_MIN, DBL_MIN, FLT_MIN);
  min_subnorm_value = CHOOSE (__LDBL_DENORM_MIN__,
			      __DBL_DENORM_MIN__,
			      __FLT_DENORM_MIN__,
			      __LDBL_DENORM_MIN__,
			      __DBL_DENORM_MIN__,
			      __FLT_DENORM_MIN__);

  (void) &plus_zero;
  (void) &nan_value;
  (void) &minus_zero;
  (void) &plus_infty;
  (void) &minus_infty;
  (void) &max_value;
  (void) &min_value;
  (void) &min_subnorm_value;

  /* Clear all exceptions.  From now on we must not get random exceptions.  */
  feclearexcept (FE_ALL_EXCEPT);

  /* Test to make sure we start correctly.  */
  fpstack_test ("end *init*");
}

/* Definitions of arguments for argp functions.  */
static const struct argp_option options[] =
{
  { "verbose", 'v', "NUMBER", 0, "Level of verbosity (0..3)"},
  { "ulps-file", 'u', NULL, 0, "Output ulps to file ULPs"},
  { "no-max-error", 'f', NULL, 0,
    "Don't output maximal errors of functions"},
  { "no-points", 'p', NULL, 0,
    "Don't output results of functions invocations"},
  { "ignore-max-ulp", 'i', "yes/no", 0,
    "Ignore given maximal errors"},
  { NULL, 0, NULL, 0, NULL }
};

/* Short description of program.  */
static const char doc[] = "Math test suite: " TEST_MSG ;

/* Prototype for option handler.  */
static error_t parse_opt (int key, char *arg, struct argp_state *state);

/* Data structure to communicate with argp functions.  */
static struct argp argp =
{
  options, parse_opt, NULL, doc,
};


/* Handle program arguments.  */
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case 'f':
      output_max_error = 0;
      break;
    case 'i':
      if (strcmp (arg, "yes") == 0)
	ignore_max_ulp = 1;
      else if (strcmp (arg, "no") == 0)
	ignore_max_ulp = 0;
      break;
    case 'p':
      output_points = 0;
      break;
    case 'u':
      output_ulps = 1;
      break;
    case 'v':
      if (optarg)
	verbose = (unsigned int) strtoul (optarg, NULL, 0);
      else
	verbose = 3;
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

#if 0
/* function to check our ulp calculation.  */
void
check_ulp (void)
{
  int i;

  FLOAT u, diff, ulp;
  /* This gives one ulp.  */
  u = FUNC(nextafter) (10, 20);
  check_equal (10.0, u, 1, &diff, &ulp);
  printf ("One ulp: % .4" PRINTF_NEXPR "\n", ulp);

  /* This gives one more ulp.  */
  u = FUNC(nextafter) (u, 20);
  check_equal (10.0, u, 2, &diff, &ulp);
  printf ("two ulp: % .4" PRINTF_NEXPR "\n", ulp);

  /* And now calculate 100 ulp.  */
  for (i = 2; i < 100; i++)
    u = FUNC(nextafter) (u, 20);
  check_equal (10.0, u, 100, &diff, &ulp);
  printf ("100 ulp: % .4" PRINTF_NEXPR "\n", ulp);
}
#endif

int
main (int argc, char **argv)
{

  int remaining;

  verbose = 1;
  output_ulps = 0;
  output_max_error = 1;
  output_points = 1;
  /* XXX set to 0 for releases.  */
  ignore_max_ulp = 0;

  /* Parse and process arguments.  */
  argp_parse (&argp, argc, argv, 0, &remaining, NULL);

  if (remaining != argc)
    {
      fprintf (stderr, "wrong number of arguments");
      argp_help (&argp, stdout, ARGP_HELP_SEE, program_invocation_short_name);
      exit (EXIT_FAILURE);
    }

  if (output_ulps)
    {
      ulps_file = fopen ("ULPs", "a");
      if (ulps_file == NULL)
	{
	  perror ("can't open file `ULPs' for writing: ");
	  exit (1);
	}
    }


  initialize ();
  printf (TEST_MSG);

#if 0
  check_ulp ();
#endif

  /* Keep the tests a wee bit ordered (according to ISO C99).  */
  /* Classification macros:  */
  finite_test ();
  fpclassify_test ();
  isfinite_test ();
  isinf_test ();
  isnan_test ();
  isnormal_test ();
  signbit_test ();

  /* Trigonometric functions:  */
  acos_test ();
  acos_test_tonearest ();
  acos_test_towardzero ();
  acos_test_downward ();
  acos_test_upward ();
  asin_test ();
  asin_test_tonearest ();
  asin_test_towardzero ();
  asin_test_downward ();
  asin_test_upward ();
  atan_test ();
  atan2_test ();
  cos_test ();
  cos_test_tonearest ();
  cos_test_towardzero ();
  cos_test_downward ();
  cos_test_upward ();
  sin_test ();
  sin_test_tonearest ();
  sin_test_towardzero ();
  sin_test_downward ();
  sin_test_upward ();
  sincos_test ();
  tan_test ();
  tan_test_tonearest ();
  tan_test_towardzero ();
  tan_test_downward ();
  tan_test_upward ();

  /* Hyperbolic functions:  */
  acosh_test ();
  asinh_test ();
  atanh_test ();
  cosh_test ();
  cosh_test_tonearest ();
  cosh_test_towardzero ();
  cosh_test_downward ();
  cosh_test_upward ();
  sinh_test ();
  sinh_test_tonearest ();
  sinh_test_towardzero ();
  sinh_test_downward ();
  sinh_test_upward ();
  tanh_test ();

  /* Exponential and logarithmic functions:  */
  exp_test ();
  exp_test_tonearest ();
  exp_test_towardzero ();
  exp_test_downward ();
  exp_test_upward ();
  exp10_test ();
  exp2_test ();
  expm1_test ();
  frexp_test ();
  ldexp_test ();
  log_test ();
  log10_test ();
  log1p_test ();
  log2_test ();
  logb_test ();
  logb_test_downward ();
  modf_test ();
  ilogb_test ();
  scalb_test ();
  scalbn_test ();
  scalbln_test ();
  significand_test ();

  /* Power and absolute value functions:  */
  cbrt_test ();
  fabs_test ();
  hypot_test ();
  pow_test ();
  pow_test_tonearest ();
  pow_test_towardzero ();
  pow_test_downward ();
  pow_test_upward ();
  sqrt_test ();

  /* Error and gamma functions:  */
  erf_test ();
  erfc_test ();
  gamma_test ();
  lgamma_test ();
  tgamma_test ();

  /* Nearest integer functions:  */
  ceil_test ();
  floor_test ();
  nearbyint_test ();
  rint_test ();
  rint_test_tonearest ();
  rint_test_towardzero ();
  rint_test_downward ();
  rint_test_upward ();
  lrint_test ();
  lrint_test_tonearest ();
  lrint_test_towardzero ();
  lrint_test_downward ();
  lrint_test_upward ();
  llrint_test ();
  llrint_test_tonearest ();
  llrint_test_towardzero ();
  llrint_test_downward ();
  llrint_test_upward ();
  round_test ();
  lround_test ();
  llround_test ();
  trunc_test ();

  /* Remainder functions:  */
  fmod_test ();
  remainder_test ();
  remquo_test ();

  /* Manipulation functions:  */
  copysign_test ();
  nextafter_test ();
  nexttoward_test ();

  /* maximum, minimum and positive difference functions */
  fdim_test ();
  fmax_test ();
  fmin_test ();

  /* Multiply and add:  */
  fma_test ();
  fma_test_towardzero ();
  fma_test_downward ();
  fma_test_upward ();

  /* Comparison macros:  */
  isgreater_test ();
  isgreaterequal_test ();
  isless_test ();
  islessequal_test ();
  islessgreater_test ();
  isunordered_test ();

  /* Complex functions:  */
  cabs_test ();
  cacos_test ();
  cacosh_test ();
  carg_test ();
  casin_test ();
  casinh_test ();
  catan_test ();
  catanh_test ();
  ccos_test ();
  ccosh_test ();
  cexp_test ();
  cimag_test ();
  clog10_test ();
  clog_test ();
  conj_test ();
  cpow_test ();
  cproj_test ();
  creal_test ();
  csin_test ();
  csinh_test ();
  csqrt_test ();
  ctan_test ();
  ctan_test_tonearest ();
  ctan_test_towardzero ();
  ctan_test_downward ();
  ctan_test_upward ();
  ctanh_test ();
  ctanh_test_tonearest ();
  ctanh_test_towardzero ();
  ctanh_test_downward ();
  ctanh_test_upward ();

  /* Bessel functions:  */
  j0_test ();
  j1_test ();
  jn_test ();
  y0_test ();
  y1_test ();
  yn_test ();

  if (output_ulps)
    fclose (ulps_file);

  printf ("\nTest suite completed:\n");
  printf ("  %d test cases plus %d tests for exception flags executed.\n",
	  noTests, noExcTests);
  if (noXFails)
    printf ("  %d expected failures occurred.\n", noXFails);
  if (noXPasses)
    printf ("  %d unexpected passes occurred.\n", noXPasses);
  if (noErrors)
    {
      printf ("  %d errors occurred.\n", noErrors);
      return 1;
    }
  printf ("  All tests passed successfully.\n");

  return 0;
}

/*
 * Local Variables:
 * mode:c
 * End:
 */