//==============================================================================
//  Header files
//==============================================================================

#include "args_util.h"

//==============================================================================
//  Function definitions
//==============================================================================

//------------------------------------------------------------------------------
//  Predicates for range checking on longs
//------------------------------------------------------------------------------

t_bool is_any_l(t_atom_long val, t_atom_long low, t_atom_long high) {
  return true;
}
t_bool is_above_l(t_atom_long val, t_atom_long low, t_atom_long high) {
  return (val >= low);
}
t_bool is_below_l(t_atom_long val, t_atom_long low, t_atom_long high) {
  return (val <= high);
}
t_bool is_between_l(t_atom_long val, t_atom_long low, t_atom_long high) {
  return ((val >= low) && (val <= high));
}

//------------------------------------------------------------------------------
//  Predicates for range checking on floats
//------------------------------------------------------------------------------

t_bool is_any_f(t_atom_float val, t_atom_float low, t_atom_float high) {
  return true;
}
t_bool is_above_f(t_atom_float val, t_atom_float low, t_atom_float high) {
  return (val >= low);
}
t_bool is_below_f(t_atom_float val, t_atom_float low, t_atom_float high) {
  return (val <= high);
}
t_bool is_between_f(t_atom_float val, t_atom_float low, t_atom_float high) {
  return ((val >= low) && (val <= high));
}

//------------------------------------------------------------------------------
//  Warning functions
//------------------------------------------------------------------------------

//******************************************************************************
//  Post a warning for any argument.
//
//  @param x The pointer the Max object.
//  @param sym The message name as a symbol.
//  @param index The index of the argument.
//  @param atom An atom holding the argument.
//  @param type The type of the argument (A_LONG, A_FLOAT, or A_SYM).
//  @param filter_str A string with a filter specific warning message.
//
void args_warn(void* x, t_symbol* sym, short index, t_atom* atom,
  short type, const char* filter_str) {

  t_dstr dstr = dstr_new_n(256);
  dstr_cat_printf(dstr, "%s arg[%i] = ", sym->s_name, index);
  dstr_cat_atom(dstr, atom);
  dstr_cat_bin(dstr, ": Should be ", 12);
  
  switch (type) {
    case A_LONG: dstr_cat_bin(dstr, "an int", 6); break;
    case A_FLOAT: dstr_cat_bin(dstr, "a float", 7); break;
    case A_SYM: dstr_cat_bin(dstr, "a symbol", 8); break;
    default: dstr_cat_bin(dstr, "<err: type>", 11); break;
  }
  dstr_cat_printf(dstr, "%s.", filter_str);
  WARN(dstr->cstr);
  dstr_free(&dstr);
}

//******************************************************************************
//  Post a warning for a long argument.
//
//  @param x The pointer the Max object.
//  @param sym The message name as a symbol.
//  @param index The index of the argument.
//  @param atom An atom holding the argument.
//  @param filter A filter function pointer.
//  @param mini A minimum value (only matters if used by filter).
//  @param maxi A maximum value (only matters if used by filter).
//
void args_warn_long(void* x, t_symbol* sym, short index, t_atom* atom,
  t_filter_long filter, t_atom_long mini, t_atom_long maxi) {

  t_dstr filter_str = dstr_new_n(64);

  // Generate the filter specific warning message.
  if (filter == &is_above_l) {
    dstr_cat_printf(filter_str, " above %i", mini);
  }
  else if (filter == &is_below_l) {
    dstr_cat_printf(filter_str, " below %i", maxi);
  }
  else if (filter == &is_between_l) {
    dstr_cat_printf(filter_str, " between %i and %i", mini, maxi);
  }
  args_warn(x, sym, index, atom, A_LONG, filter_str->cstr);
  dstr_free(&filter_str);
}

//******************************************************************************
//  Post a warning for a float argument.
//
//  @param x The pointer the Max object.
//  @param sym The message name as a symbol.
//  @param index The index of the argument.
//  @param atom An atom holding the argument.
//  @param filter A filter function pointer.
//  @param mini A minimum value (only matters if used by filter).
//  @param maxi A maximum value (only matters if used by filter).
//
void args_warn_float(void* x, t_symbol* sym, short index, t_atom* atom,
  t_filter_float filter, t_atom_float mini, t_atom_float maxi) {

  t_dstr filter_str = dstr_new_n(64);

  if (filter == &is_above_f) {
    dstr_cat_printf(filter_str, " above %f", mini);
  }
  else if (filter == &is_below_f) {
    dstr_cat_printf(filter_str, " below %f", maxi);
  }
  else if (filter == &is_between_f) {
    dstr_cat_printf(filter_str, " between %f and %f", mini, maxi);
  }
  args_warn(x, sym, index, atom, A_FLOAT, filter_str->cstr);
  dstr_free(&filter_str);
}

//******************************************************************************
//  Post a warning for a symbol argument.
//
void args_warn_sym(void* x, t_symbol* sym, short index, t_atom* atom,
  short symbols_cnt, t_symbol** symbols) {

  t_dstr filter_str = dstr_new_n(64);

  if (symbols_cnt != 0) {
    dstr_cat_cstr(filter_str, " in [");
    dstr_cat_join_symbols(filter_str, symbols_cnt, symbols, ", ");
    dstr_cat_cstr(filter_str, "]");
  }
  args_warn(x, sym, index, atom, A_SYM, filter_str->cstr);
  dstr_free(&filter_str);
}

//------------------------------------------------------------------------------
//  Argument testing functions
//------------------------------------------------------------------------------

//******************************************************************************
//  Test that the number of arguments equals a value.
//
t_bool args_count_is(void* x, t_symbol* sym, long argc, long cnt) {

  if (argc == cnt) {
    return true;
  }
  else {
    WARN("%s: Should have %i arguments instead of %i.", sym->s_name, cnt, argc);
    return false;
  }
}

//******************************************************************************
//  Test that the number of arguments is within a range.
//
t_bool args_count_is_between(void* x, t_symbol* sym, long argc,
  long mini, long maxi) {

  if ((argc >= mini) && (argc <= maxi)) {
    return true;
  }
  else {
    WARN("%s: Should have between %i and %i arguments instead of %i.",
      sym->s_name, mini, maxi, argc);
    return false;
  }
}

//******************************************************************************
//  Test that an argument is a long and satisfies a filter.
//
t_bool args_is_long(void* x, t_symbol* sym, t_atom* argv, short index,
  t_filter_long filter, t_atom_long low, t_atom_long high) {

  argv += index;
  if ((atom_gettype(argv) == A_LONG)
    && (!filter || filter(atom_getlong(argv), low, high))) {
    return true;
  }
  else {
    args_warn_long(x, sym, index, argv, filter, low, high);
    return false;
  }
}

//******************************************************************************
//  Test that an argument is a float and satisfies a filter.
//
t_bool args_is_float(void* x, t_symbol* sym, t_atom* argv, short index,
  t_filter_float filter, t_atom_float low, t_atom_float high) {

  argv += index;
  if ((atom_gettype(argv) == A_FLOAT)
    && (!filter || filter(atom_getfloat(argv), low, high))) {
    return true;
  }
  else {
    args_warn_float(x, sym, index, argv, filter, low, high);
    return false;
  }
}

//******************************************************************************
//  Test that an argument is numeric (long or float) and satisfies a filter.
//
t_bool args_is_number(void* x, t_symbol* sym, t_atom* argv, short index,
  t_filter_float filter, t_atom_float low, t_atom_float high) {

  argv += index;
  if (((atom_gettype(argv) == A_LONG)
    || (atom_gettype(argv) == A_FLOAT))
    && (!filter || filter(atom_getfloat(argv), low, high))) {
    return true;
  }
  else {
    args_warn_float(x, sym, index, argv, filter, low, high);
    return false;
  }
}

//******************************************************************************
//  Test that an argument is a symbol and within an optional list.
//
t_bool args_is_sym(void* x, t_symbol* sym, t_atom* argv, short index,
  short symbols_cnt, t_symbol** symbols) {

  argv += index;
  if (atom_gettype(argv) == A_SYM) {
    if (symbols_cnt == 0) {
      return true;
    }
    for (short i = 0; i < symbols_cnt; i++) {
      if (atom_getsym(argv) == symbols[i]) {
        return true;
      }
    }
  }
  args_warn_sym(x, sym, index, argv, symbols_cnt, symbols);
  return false;
}

//******************************************************************************
//  Test a list of numeric argument (long or float cast to float).
//
t_bool args_are_numbers(void* x, t_symbol* sym, t_atom* argv,
  short index, short cnt,
  t_filter_float filter, t_atom_float low, t_atom_float high) {

  argv += index;
  for (short i = 0; i < cnt; i++, argv++) {
    if (((atom_gettype(argv) != A_LONG)
      && (atom_gettype(argv) != A_FLOAT))
      || (filter && !filter(atom_getfloat(argv), low, high))) {
      args_warn_float(x, sym, index + i, argv, filter, low, high);
      return false;
    }
  }
  return true;
}
