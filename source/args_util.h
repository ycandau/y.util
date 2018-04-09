#ifndef YC_ARGS_UTIL_H_
#define YC_ARGS_UTIL_H_

//==============================================================================
//  Header files
//==============================================================================

#include "max_util.h"
#include "dstring.h"

//==============================================================================
//  Defines
//==============================================================================

//==============================================================================
//  Typedef
//==============================================================================

//******************************************************************************
//  Function pointer type for long filter predicates
//
typedef t_bool (*t_filter_long)(
  t_atom_long val, t_atom_long low, t_atom_long high);

//******************************************************************************
//  Function pointer type for float filter predicates
//
typedef t_bool (*t_filter_float)(
  t_atom_float val, t_atom_float low, t_atom_float high);

//******************************************************************************
//  Function pointer type for symbol filter predicates
//
typedef t_bool (*t_filter_sym)(
  t_symbol* val, t_symbol** low, t_atom_float high);

//==============================================================================
//  Function declarations
//==============================================================================

//------------------------------------------------------------------------------
//  Predicates for range checking on longs
//------------------------------------------------------------------------------

t_bool is_any_l(t_atom_long val, t_atom_long low, t_atom_long high);
t_bool is_above_l(t_atom_long val, t_atom_long low, t_atom_long high);
t_bool is_below_l(t_atom_long val, t_atom_long low, t_atom_long high);
t_bool is_between_l(t_atom_long val, t_atom_long low, t_atom_long high);

//------------------------------------------------------------------------------
//  Predicates for range checking on floats
//------------------------------------------------------------------------------

t_bool is_any_f(t_atom_float val, t_atom_float low, t_atom_float high);
t_bool is_above_f(t_atom_float val, t_atom_float low, t_atom_float high);
t_bool is_below_f(t_atom_float val, t_atom_float low, t_atom_float high);
t_bool is_between_f(t_atom_float val, t_atom_float low, t_atom_float high);

//------------------------------------------------------------------------------
//  Warning functions
//------------------------------------------------------------------------------

//******************************************************************************
//  Post a warning for a long argument.
//
void args_warn_long(void* x, t_symbol* sym, short index, t_atom* atom,
  t_filter_long filter, t_atom_long low, t_atom_long high);

//******************************************************************************
//  Post a warning for a float argument.
//
void args_warn_float(void* x, t_symbol* sym, short index, t_atom* atom,
  t_filter_float filter, t_atom_float low, t_atom_float high);

//******************************************************************************
//  Post a warning for a symbol argument.
//
void args_warn_sym(void* x, t_symbol* sym, short index, t_atom* atom,
  short symbols_cnt, t_symbol** symbols);

//------------------------------------------------------------------------------
//  Argument testing functions
//------------------------------------------------------------------------------

//******************************************************************************
//  Test that the number of arguments equals a value.
//
t_bool args_count_is(void* x, t_symbol* sym, long argc, long cnt);

//******************************************************************************
//  Test that the number of arguments is within a range.
//
t_bool args_count_is_between(void* x, t_symbol* sym, long argc,
  long low, long high);

//******************************************************************************
//  Test that an argument is a long and satisfies a filter.
//
t_bool args_is_long(void* x, t_symbol* sym, t_atom* argv, short index,
  t_filter_long filter, t_atom_long low, t_atom_long high);

//******************************************************************************
//  Test that an argument is a float and satisfies a filter.
//
t_bool args_is_float(void* x, t_symbol* sym, t_atom* argv, short index,
  t_filter_float filter, t_atom_float low, t_atom_float high);

//******************************************************************************
//  Test that an argument is numeric (long or float) and satisfies a filter.
//
t_bool args_is_number(void* x, t_symbol* sym, t_atom* argv, short index,
  t_filter_float filter, t_atom_float low, t_atom_float high);

//******************************************************************************
//  Test that an argument is a symbol and within an optional list.
//
t_bool args_is_sym(void* x, t_symbol* sym, t_atom* argv, short index,
  short symbols_cnt, t_symbol** symbols);

//******************************************************************************
//  Test a list of numeric argument (long or float cast to float).
//
t_bool args_are_numbers(void* x, t_symbol* sym, t_atom* argv,
  short index, short cnt,
  t_filter_float filter, t_atom_float low, t_atom_float high);

#endif
