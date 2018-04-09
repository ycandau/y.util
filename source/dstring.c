//==============================================================================
//
//  @file dstring.c
//  @author Yves Candau <ycandau@sfu.ca>
//  
//  @brief Dynamic string functions
//  
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
//==============================================================================

//==============================================================================
//  Header files
//==============================================================================

#include "dstring.h"

//==============================================================================
//  Unexposed macros
//==============================================================================

//******************************************************************************
//  Memory functions
//
#define MALLOC(size)       sysmem_newptr((long)(size))
#define REALLOC(ptr, size) sysmem_resizeptr((ptr), (long)(size))
#define FREE(ptr)          sysmem_freeptr((ptr))

//******************************************************************************
//  Null values
//
#define NULL_DSTR (&_null_dstr_struct)
#define NULL_CSTR _null_cstr

//******************************************************************************
//   Numeric constants
//
#define M_LN2      0.693147180559945309417  // ln(2)
#define M_LN10     2.30258509299404568402   // ln(10)
#define M_LN2_LN10 0.301029995663981195214  // ln(2) / ln(10)

//==============================================================================
//  Extern variables definitions
//==============================================================================

char _null_cstr[1] = "";
t_dstr_struct _null_dstr_struct = { _null_cstr, 0, DSTR_LEN_ERR };

//==============================================================================
//  Function declarations withheld from the header file
//==============================================================================

t_dstr _dstr_set_to_null(t_dstr dstr);
t_dstr _dstr_new(const char* src, t_dstr_int len_cur, t_dstr_int len_max);
t_dstr _dstr_realloc(t_dstr dstr, t_dstr_int len_max);
t_dstr _dstr_alloc_new_cstr(t_dstr dstr, t_dstr_int len);

t_dstr_int _dstr_next_pow2(t_dstr_int val);
t_dstr_int _dstr_grow(t_dstr dest, t_dstr_int pos, t_dstr_int len);
t_dstr _dstr_cpy(t_dstr dest, t_dstr_int pos, const char* src, t_dstr_int len);

int _dstr_int_to_cstr(char* str, const __int64 val);
int _dstr_float_to_cstr_sci(char* str, double val, int prec);

//==============================================================================
//  Function definitions
//==============================================================================

//******************************************************************************
//  Set a dstring to NULL, used to propagate allocation errors.
//
//  @param dstr The dstring to set to NULL (ANY).
//
//  @return The dstring.
//
t_dstr _dstr_set_to_null(t_dstr dstr) {

  if (dstr != NULL_DSTR) {
    if ((dstr->cstr) && (dstr->cstr != NULL_CSTR)) { FREE(dstr->cstr); }
    dstr->cstr = NULL_DSTR->cstr;
    dstr->len_cur = NULL_DSTR->len_cur;
    dstr->len_max = NULL_DSTR->len_max;
  }
  return dstr;
}

//******************************************************************************
//  Create a new dstring.
//
//  @param src NULL or a pointer to a C string to copy (ANY).
//  @param len_cur The length to copy (ENSURE: len_cur <= DSTR_LEN_MAX).
//  @param len_max The length to allocate (ENSURE: len_max <= DSTR_LEN_MAX).
//
//  @return The new dstring, or NULL_DSTR if there is an allocation error.
//
t_dstr _dstr_new(const char* src, t_dstr_int len_cur, t_dstr_int len_max) {

  // Allocate the structure
  t_dstr dstr = (t_dstr)MALLOC(sizeof(t_dstr_struct));
  if (dstr == NULL) { return NULL_DSTR; }

  // Allocate the C string
  dstr->cstr = (char*)MALLOC(sizeof(char) * (len_max + 1));

  // Test the allocation and copy
  if (dstr->cstr) {
    dstr->len_cur = len_cur;
    memcpy(dstr->cstr, src, len_cur);
    dstr->cstr[len_cur] = '\0';
    dstr->len_max = len_max;
    return dstr;
  }
  else {
    FREE(dstr);
    return NULL_DSTR;
  }
}

//******************************************************************************
//  Reallocate the C string of an existing dstring.
//
//  @param dstr The dstring to allocate (ANY).
//  @param len The maximum length of the dstring (ENSURE: len <= DSTR_LEN_MAX).
//
//  @return The actual allocated length.
//
t_dstr _dstr_realloc(t_dstr dstr, t_dstr_int len) {

  if (DSTR_IS_NULL(dstr)) { return dstr; }
  
  dstr->cstr = (char*)REALLOC(dstr->cstr, sizeof(char) * (len + 1));

  if (dstr->cstr) {
    dstr->len_cur = MIN(dstr->len_cur, len);  // if dstring is shortened
    dstr->len_max = len;
    dstr->cstr[dstr->len_cur] = '\0';
  }
  else {
    _dstr_set_to_null(dstr);
  }
  return dstr;
}

//******************************************************************************
//  Allocate a new C string for an existing dstring.
//
//  @param dstr The dstring to allocate (ANY).
//  @param len The maximum length of the dstring (ENSURE: len <= DSTR_LEN_MAX).
//
//  @return The dstring.
//
//  IMPORTANT:
//    Free the C string manually if the allocation was succesful.
//    Otherwise it is freed automatically.
//
t_dstr _dstr_alloc_new_cstr(t_dstr dstr, t_dstr_int len) {

  if (DSTR_IS_NULL(dstr)) { return dstr; }

  char* old_cstr = dstr->cstr;
  dstr->cstr = (char*)MALLOC(sizeof(char) * (len + 1));
  if (dstr->cstr) {
    dstr->len_cur = 0;
    dstr->len_max = len;
    dstr->cstr[0] = '\0';
  }
  else {
    FREE(old_cstr);
    _dstr_set_to_null(dstr);
  }
  return dstr;
}

//******************************************************************************
//  Get the smallest power of two greater than a value.
//
//  @param val The input value, which should be an unsigned int (ANY).
//
//  @return The smallest power of two or DSTR_LEN_MAX.
//
t_dstr_int _dstr_next_pow2(t_dstr_int val) {
  
  t_dstr_int i = val;

  val = val ? --val : 0;  // in case val is a power of 2, or 0
  val |= (val >> 1);
  val |= (val >> 2);
  val |= (val >> 4);    // 8 bit type
#if (DSTR_INT_SIZE > 8)
  val |= (val >> 8);    // 16 bit type
#if (DSTR_INT_SIZE > 16)
  val |= (val >> 16);   // 32 bit type
#if (DSTR_INT_SIZE > 32)
  val |= (val >> 32);   // 64 bit type
#endif
#endif 
#endif
  val++;  // this increment could loop back to 0

  // Return the maximum value if val looped back to 0
  return val ? val : DSTR_LEN_MAX;
}

//******************************************************************************
//  Resize the dstring if necessary.
//
//  @param dest The destination dstring to grow if necessary (ANY).
//  @param pos The position at which to copy (ENSURE: pos <= DSTR_LEN_MAX).
//  @param len The copy length that the dstring should accomodate (ANY).
//
//  @return The actual copy length.
//
t_dstr_int _dstr_grow(t_dstr dest, t_dstr_int pos, t_dstr_int len) {

  if (len > dest->len_max - pos) {
    len = MIN(len, DSTR_LEN_MAX - pos);
    pos = _dstr_next_pow2(pos + len);
    _dstr_realloc(dest, pos);
  }
  return len;
}

//******************************************************************************
//  Helper function to copy and concatenate into a dstring.
//
//  @param dest The dstring to copy or concatenate into (ANY).
//  @param src The source from which to copy (ENSURE: src != NULL).
//  @param pos The position at which to copy (ENSURE: pos <= DSTR_LEN_MAX).
//  @param len The length of src to copy.
//
//  @return The dstring.
//
t_dstr _dstr_cpy(t_dstr dest, t_dstr_int pos, const char* src, t_dstr_int len) {

  len = _dstr_grow(dest, pos, len);
  if (DSTR_NOT_NULL(dest)) {
    dest->len_cur = pos + len;
    memcpy(dest->cstr + pos, src, len);
    dest->cstr[pos + len] = '\0';
  }
  return dest;
}

//******************************************************************************
//  Create an empty dstring.
//
//  @return The new dstring.
//
t_dstr dstr_new() {

  return _dstr_new(NULL, 0, 8);
}

//******************************************************************************
//  Create an empty dstring of length up to N.
//
//  @param len The length to allocate (ANY).
//
//  @return The new dstring.
//
t_dstr dstr_new_n(t_dstr_int len) {

  return _dstr_new(NULL, 0, MIN(len, DSTR_LEN_MAX));
}

//******************************************************************************
//  Create a dstring from a C string.
//
//  @param cstr A C string to initialize the dstring (ENSURE: cstr != NULL).
//
//  @return The new dstring.
//
t_dstr dstr_new_cstr(const char* cstr) {
  
  t_dstr_int len = (t_dstr_int)strnlen(cstr, DSTR_LEN_MAX);
  return _dstr_new(cstr, len, _dstr_next_pow2(len));
}

//******************************************************************************
//  Create a dstring from a dstring.
//
//  @param dstr A dstring with which to initialize the dstring (ANY).
//
//  @return The new dstring.
//
t_dstr dstr_new_dstr(const t_dstr src) {

  // Propagate errors from the source
  if (DSTR_IS_NULL(src)) { return NULL_DSTR; }
  return _dstr_new(src->cstr, src->len_cur, _dstr_next_pow2(src->len_cur));
}

//******************************************************************************
//  Create a dstring from a binary string.
//
//  @param bin A binary string with which to initialize the dstring (ANY).
//  @param len The length of the binary string (ANY).
//
//  @return The new dstring.
//
t_dstr dstr_new_bin(const char* bin, t_dstr_int len) {

  return _dstr_new(bin, MIN(len, DSTR_LEN_MAX), _dstr_next_pow2(len));
}

//******************************************************************************
//  Create a dstring from an int value.
//
//  @param i The int value to convert (ANY).
//
//  @return The new dstring.
//
t_dstr dstr_new_int(const __int64 i) {

  char str[DSTR_I64_LEN_MAX];
  t_dstr_int len = _dstr_int_to_cstr(str, i);
  return _dstr_new(str, len, _dstr_next_pow2(len));
}

//******************************************************************************
//  Create a dstring from a double value.
//
//  @param val The double value to convert (ANY).
//  @param prec The number of digits after the decimal point (ANY).
//
//  @return The dstring.
//
t_dstr dstr_new_float(double val, t_uint8 prec) {

  char str[DSTR_F64_LEN_MAX];
  int len = snprintf(str, DSTR_F64_LEN_MAX, "%.*f", prec, val);
  if (len < DSTR_F64_LEN_MAX) {
    return _dstr_new(str, len, _dstr_next_pow2(len));
  }
  else {
    return dstr_new_printf("%.*f", prec, val);
  }
}

//******************************************************************************
//  Create a dstring from a double value, using scientific notation.
//
//  @param val The double value to convert (ANY).
//  @param prec The number of digits after the decimal point (ANY).
//
//  @return The dstring.
//
t_dstr dstr_new_float_sci(double val, t_uint8 prec) {

  char str[DSTR_F64_LEN_MAX];
  int len = snprintf(str, DSTR_F64_LEN_MAX, "%.*E", prec, val);
  if (len < DSTR_F64_LEN_MAX) {
    return _dstr_new(str, len, _dstr_next_pow2(len));
  }
  else {
    return dstr_new_printf("%.*E", prec, val);
  }
}

//******************************************************************************
//  Create a dstring from a printf style format string and arguments.
//
//  @param format The printf style formatting string (printf format).
//  @param ... The variadic arguments for printf (printf format).
//
//  @return The new dstring.
//
t_dstr dstr_new_printf(const char* format, ...) {

  va_list ap;
  va_start(ap, format);
  char str[DSTR_PRINTF_TRY_LEN];
  int len = vsnprintf(str, DSTR_PRINTF_TRY_LEN, format, ap);
  t_dstr dstr;

  // If the format string is invalid, return an error message
  if (len == -1) {
    dstr = dstr_new_cstr("<err: printf>");
  }

  // If the buffer is long enough, use it for the dstring
  else if (len < DSTR_PRINTF_TRY_LEN) {
    dstr = _dstr_new(str, len, _dstr_next_pow2(len));
  }

  // Otherwise run printf again, directly into the dstring
  else {
    dstr = _dstr_new(NULL, 0, _dstr_next_pow2(len));
    if (DSTR_NOT_NULL(dstr)) {
      vsnprintf(dstr->cstr, dstr->len_max + 1, format, ap);
      dstr->len_cur = MIN(len, DSTR_LEN_MAX);
    }
  }
  va_end(ap);
  return dstr;
}

//******************************************************************************
//  Create a dstring from an atom.
//
//  @param atom A pointer to the atom (ANY).
//
//  @return The dstring.
//
t_dstr dstr_new_atom(const t_atom* atom) {

  switch (atom_gettype(atom)) {
    case A_LONG: return dstr_new_int(atom_getlong(atom));
    case A_FLOAT: return dstr_new_float(atom_getfloat(atom), 6);
    case A_SYM: return dstr_new_cstr(atom_getsym(atom)->s_name);
    default: return dstr_new_cstr("<err: atom>");
  }
}

//******************************************************************************
//  Create a dstring from an atom value and its type.
//
//  @param atom A pointer to the atom (ANY).
//
//  @return The dstring.
//
t_dstr dstr_new_atom_type(const t_atom* atom) {

  switch (atom_gettype(atom)) {
    case A_LONG:
      return dstr_new_printf("%i (int)", atom_getlong(atom));
    case A_FLOAT:
      return dstr_new_printf("%.6f (float)", atom_getfloat(atom));
    case A_SYM:
      return dstr_new_printf("%s (sym)", atom_getsym(atom));
    default:
      return dstr_new_cstr("<err: atom>");
  }
}

//******************************************************************************
//  Free a dstring.
//
//  The dstring is set to NULL_DSTR.
//
//  @param dstr A pointer to the dstring to free (ANY).
//
void dstr_free(t_dstr* p_dstr) {

  if (p_dstr == NULL) { return; }
  if (*p_dstr == NULL) { *p_dstr = NULL_DSTR; return; }
  if (*p_dstr == NULL_DSTR) { return; }

  if (((*p_dstr)->cstr) && ((*p_dstr)->cstr != NULL_CSTR)) {
    FREE((*p_dstr)->cstr);
  }
  FREE(*p_dstr);
  *p_dstr = NULL_DSTR;
}

//******************************************************************************
//  Concatenate a C string into a dstring.
//
//  @param dest The dstring to concatenate into (ANY).
//  @param cstr The C string to concatenate (ENSURE: cstr != NULL).
//
//  @return The dstring.
//
t_dstr dstr_cat_cstr(t_dstr dest, const char* cstr) {

  return _dstr_cpy(
    dest, dest->len_cur, cstr, (t_dstr_int)strnlen(cstr, DSTR_LEN_MAX));
}

//******************************************************************************
//  Concatenate a dstring into a dstring.
//
//  @param dest The dstring to concatenate into (ANY).
//  @param dstr The dstring to concatenate (ANY).
//
//  @return The dstring.
//
t_dstr dstr_cat_dstr(t_dstr dest, const t_dstr dstr) {

  // Propagate errors from the source
  if (DSTR_IS_NULL(dstr)) { return _dstr_set_to_null(dest); }
  return _dstr_cpy(dest, dest->len_cur, dstr->cstr, dstr->len_cur);
}

//******************************************************************************
//  Concatenate a binary string into a dstring.
//
//  @param dest The dstring to concatenate into (ANY).
//  @param bin The binary string to concatenate (ANY).
//
//  @return The dstring.
//
t_dstr dstr_cat_bin(t_dstr dest, const char* bin, t_dstr_int len) {

  return _dstr_cpy(dest, dest->len_cur, bin, len);
}

//******************************************************************************
//  Concatenate a string from an int value into a dstring.
//
//  @param dest The dstring to concatenate into (ANY).
//  @param i The int value to concatenate as a string (ANY).
//
//  @return The dstring.
//
t_dstr dstr_cat_int(t_dstr dest, const __int64 i) {

  char cstr[DSTR_I64_LEN_MAX];
  t_dstr_int len = _dstr_int_to_cstr(cstr, i);
  return _dstr_cpy(dest, dest->len_cur, cstr, len);
}

//******************************************************************************
//  Concatenate a string from a double value into a dstring.
//
//  @param dest The dstring to concatenate into (ANY).
//  @param val The double value to convert (ANY).
//  @param prec The number of digits after the decimal point (ANY).
//
//  @return The dstring.
//
t_dstr dstr_cat_float(t_dstr dest, double val, t_uint8 prec) {

  char str[DSTR_F64_LEN_MAX];
  int len = snprintf(str, DSTR_F64_LEN_MAX, "%.*f", prec, val);
  if (len < DSTR_F64_LEN_MAX) {
    return _dstr_cpy(dest, dest->len_cur, str, len);
  }
  else {
    return dstr_cat_printf(dest, "%.*f", prec, val);
  }
}

//******************************************************************************
//  Concatenate a string from a double value, using scientific notation.
//
//  @param dest The dstring to concatenate into (ANY).
//  @param val The double value to convert (ANY).
//  @param prec The number of digits after the decimal point (ANY).
//
//  @return The dstring.
//
t_dstr dstr_cat_float_sci(t_dstr dest, double val, t_uint8 prec) {

  char str[DSTR_F64_LEN_MAX];
  int len = snprintf(str, DSTR_F64_LEN_MAX, "%.*E", prec, val);
  if (len < DSTR_F64_LEN_MAX) {
    return _dstr_cpy(dest, dest->len_cur, str, len);
  }
  else {
    return dstr_cat_printf(dest, "%.*E", prec, val);
  }
}

//******************************************************************************
//  Concatenate a printf style generated string into a dstring.
//
//  @param dest The dstring to concatenate into (ANY).
//  @param format The printf style formatting string (printf format).
//  @param ... The variadic arguments for printf (printf format).
//
//  @return The dstring.
//
t_dstr dstr_cat_printf(t_dstr dest, const char* format, ...) {

  va_list ap;
  va_start(ap, format);
  char str[DSTR_PRINTF_TRY_LEN];
  int len = vsnprintf(str, DSTR_PRINTF_TRY_LEN, format, ap);

  // If the format string is invalid, concatenate an error message
  if (len == -1) {
    dstr_cat_cstr(dest, "<err: printf>");
  }
  // If the buffer is long enough, concatenate it into the dstring
  else if (len < DSTR_PRINTF_TRY_LEN) {
    _dstr_cpy(dest, dest->len_cur, str, len);
  }
  // Otherwise grow the dstring, and run printf again directly into it
  else {
    len = _dstr_grow(dest, dest->len_cur, len);
    if (DSTR_NOT_NULL(dest)) {
      vsnprintf(
        dest->cstr + dest->len_cur,
        dest->len_max - dest->len_cur + 1,
        format, ap);
      dest->len_cur += len;
    }
  }
  va_end(ap);
  return dest;
}

//******************************************************************************
//  Concatenate a string from an atom into a dstring.
//
//  @param dest The dstring to concatenate into (ANY).
//  @param atom A pointer to the atom (ANY).
//
//  @return The dstring.
//
t_dstr dstr_cat_atom(t_dstr dest, const t_atom* atom) {

  switch (atom_gettype(atom)) {
    case A_LONG: return dstr_cat_int(dest, atom_getlong(atom));
    case A_FLOAT: return dstr_cat_float(dest, atom_getfloat(atom), 6);
    case A_SYM: return dstr_cat_cstr(dest, atom_getsym(atom)->s_name);
    default: return dstr_cat_cstr(dest, "<err: atom>");
  }
}

//******************************************************************************
//  Concatenate a string from an atom value and its type.
//
//  @param dest The dstring to concatenate into (ANY).
//  @param atom A pointer to the atom (ANY).
//
//  @return The dstring.
//
t_dstr dstr_cat_atom_type(t_dstr dest, const t_atom* atom) {

  switch (atom_gettype(atom)) {
    case A_LONG:
      dstr_cat_int(dest, atom_getlong(atom));
      return dstr_cat_cstr(dest, " (int)");
    case A_FLOAT:
      dstr_cat_float(dest, atom_getfloat(atom), 6);
      return dstr_cat_cstr(dest, " (float)");
    case A_SYM:
      dstr_cat_cstr(dest, atom_getsym(atom)->s_name);
      return dstr_cat_cstr(dest, " (sym)");
    default:
      return dstr_new_cstr("<err: atom>");
  }
}

//******************************************************************************
//  Clear a dstring without resizing.
//
//  @param dstr The dstring to clear (ANY).
//
//  @return The dstring.
//
t_dstr dstr_clear(t_dstr dstr) {

  dstr->cstr[0] = '\0';
  dstr->len_cur = 0;
  return dstr;
}

//******************************************************************************
//  Clip the dstring to a set length.
//
//  @param dstr The dstring to resize (ANY).
//  @param len The length at which to clip (ANY).
//
//  @return The dstring.
//
t_dstr dstr_clip(t_dstr dstr, t_dstr_int len) {

  if (len >= dstr->len_cur) { return dstr; }
  dstr->cstr[len] = '\0';
  dstr->len_cur = len;
  return dstr;
}

//******************************************************************************
//  Update the current length of a dstring, in case its C string was modified.
//
//  @param dstr The dstring to update (ANY).
//
//  @return The dstring.
//
t_dstr dstr_update(t_dstr dstr) {

  char* pc = dstr->cstr;
  t_dstr_int len = 0;
  while ((*pc != '\0') && (len != dstr->len_max)) { pc++; len++; }
  dstr->len_cur = len;
  return dstr;
}

//******************************************************************************
//  Resize the dstring to fit the current length.
//
//  @param dstr The dstring to resize (ANY).
//
//  @return The dstring.
//
t_dstr dstr_fit(t_dstr dstr) {

  return _dstr_realloc(dstr, dstr->len_cur);
}

//******************************************************************************
//  Resize the dstring to smallest power of 2 greater than the current length.
//
//  @param dstr The dstring to resize (ANY).
//
//  @return The dstring.
//
t_dstr dstr_shrink(t_dstr dstr) {

  return _dstr_realloc(dstr, _dstr_next_pow2(dstr->len_cur));
}

//******************************************************************************
//  Resize the dstring to a set length, clipping the C string of necessary.
//
//  @param dstr The dstring to resize (ANY).
//  @param len The length to resize to (ANY).
//
//  @return The dstring.
//
t_dstr dstr_resize(t_dstr dstr, t_dstr_int len) {

  return _dstr_realloc(dstr, MIN(len, DSTR_LEN_MAX));
}

//******************************************************************************
//  Concatenate a list of atoms into a dstring.
//
//  @param dest The dstring to copy into.
//  @param argc The number of atoms in the list.
//  @param argv A pointer to the array of atoms.
//  @param sep A C string separator to add between the atoms.
//
//  @return The dstring.
//
t_dstr dstr_cat_join(t_dstr dest, long argc, t_atom* argv, char* sep) {

  if (argc > 0) { dstr_cat_atom(dest, argv); }
  for (long i = 1; i < argc; i++) {
    dstr_cat_cstr(dest, sep);
    dstr_cat_atom(dest, argv + i);
  }
  return dest;
}

//******************************************************************************
//  Concatenate a list of longs into a dstring.
//
//  @param dest The dstring to copy into.
//  @param argc The number of atoms in the list.
//  @param longs A pointer to the array of longs.
//  @param sep A C string separator to add between the atoms.
//
//  @return The dstring.
//
t_dstr dstr_cat_join_longs(
  t_dstr dest, long argc, t_atom_long* longs, char* sep) {

  if (argc > 0) { dstr_cat_int(dest, longs[0]); }
  for (long i = 1; i < argc; i++) {
    dstr_cat_cstr(dest, sep);
    dstr_cat_int(dest, longs[i]);
  }
  return dest;
}

//******************************************************************************
//  Concatenate a list of floats into a dstring.
//
//  @param dest The dstring to copy into.
//  @param argc The number of atoms in the list.
//  @param floats A pointer to the array of longs.
//  @param sep A C string separator to add between the atoms.
//
//  @return The dstring.
//
t_dstr dstr_cat_join_floats(
  t_dstr dest, long argc, t_atom_float* floats, t_uint8 prec, char* sep) {

  if (argc > 0) { dstr_cat_float(dest, floats[0], prec); }
  for (long i = 1; i < argc; i++) {
    dstr_cat_cstr(dest, sep);
    dstr_cat_float(dest, floats[i], prec);
  }
  return dest;
}

//******************************************************************************
//  Concatenate a list of symbols into a dstring.
//
//  @param dest The dstring to copy into.
//  @param argc The number of atoms in the list.
//  @param symbols A pointer to the array of symbols.
//  @param sep A C string separator to add between the atoms.
//
//  @return The dstring.
//
t_dstr dstr_cat_join_symbols(
  t_dstr dest, long argc, t_symbol** symbols, char* sep) {

  if (argc > 0) { dstr_cat_cstr(dest, symbols[0]->s_name); }
  for (long i = 1; i < argc; i++) {
    dstr_cat_cstr(dest, sep);
    dstr_cat_cstr(dest, symbols[i]->s_name);
  }
  return dest;
}

//******************************************************************************
//  Search and replace in a dstring.
//
//  @param dstr The dstring to search and replace in (ANY).
//  @param search The C string to search for (ENSURE: search != NULL).
//  @param replace The C string to replace with (ENSURE: search != NULL).
//
//  @return The dstring.
//
t_dstr dstr_replace(t_dstr dstr, const char* search, const char* replace) {

  t_dstr_int search_len = (t_dstr_int)strnlen(search, DSTR_LEN_MAX);
  t_dstr_int replace_len = (t_dstr_int)strnlen(replace, DSTR_LEN_MAX);
  char* src_beg = dstr->cstr;
  char* src_next;
  
  // Replace shorter: replace in place and shift, use memmove for overlaps
  if (replace_len < search_len) {
    t_dstr_int segment_len;
    char* dest = dstr->cstr;

    while (src_next = strstr(src_beg, search)) {
      segment_len = (t_dstr_int)(src_next - src_beg);
      memmove(dest, src_beg, segment_len);
      dest += segment_len;
      memcpy(dest, replace, replace_len);
      dest += replace_len;
      src_beg = src_next + search_len;
    }
    segment_len = dstr->len_cur - (t_dstr_int)(src_beg - dstr->cstr);
    memmove(dest, src_beg, segment_len);
    dstr->len_cur = (t_dstr_int)(dest - dstr->cstr) + segment_len;
    dstr->cstr[dstr->len_cur] = '\0';
  }

  // Equal lengths: substitute in place
  else if (replace_len == search_len) {
    while (src_next = strstr(src_beg, search)) {
      memcpy(src_next, replace, replace_len);
      src_beg = src_next + search_len;
    }
  }

  // Replace longer: replace into new C string, use memcpy
  else {
    char* old_cstr = dstr->cstr;
    char* src_end = dstr->cstr + dstr->len_cur;
    _dstr_alloc_new_cstr(dstr,
      MIN(dstr->len_cur + 10 * (replace_len - search_len), DSTR_LEN_MAX));
    if (DSTR_IS_NULL(dstr)) { return dstr; }

    while (src_next = strstr(src_beg, search)) {
      _dstr_cpy(dstr, dstr->len_cur, src_beg, (t_dstr_int)(src_next - src_beg));
      _dstr_cpy(dstr, dstr->len_cur, replace, replace_len);
      src_beg = src_next + search_len;
    }
    _dstr_cpy(dstr, dstr->len_cur, src_beg, (t_dstr_int)(src_end - src_beg));
    _dstr_realloc(dstr, _dstr_next_pow2(dstr->len_cur));
    FREE(old_cstr);
  }

  return dstr;
}

//******************************************************************************
//  Convert an int value into a C string.
//
//  @param str An allocated C string - ENSURE: length >= DSTR_I64_LEN_MAX.
//  @param i The int value to convert (ANY).
//
//  @return The length of the string.
//
int _dstr_int_to_cstr(char* str, const __int64 val) {

  char* pc = str;
  unsigned __int64 uint = (val > 0) ? val : -val;

  // Copy each digit from the lowest
  do {
    *pc++ = '0' + (uint % 10);
    uint /= 10;
  } while (uint);

  // Add a minus sign if necessary, and the terminal character
  if (val < 0) { *pc++ = '-'; }
  *pc = '\0';

  // Calculate the length
  uint = pc-- - str;

  // Reverse the string
  char tmp;
  while (str < pc) {
    tmp = *str;
    *str++ = *pc;
    *pc-- = tmp;
  }

  return (int)uint;
}

//******************************************************************************
//  Convert a float value into a C string.
//
//  @param str A pointer to the C string.
//  @param val The float value to convert.
//
//  @return The length of the string.
//
int _dstr_float_to_cstr_sci(char* str, double val, int prec) {
  
  // Process the sign of the value
  char* pc = str;
  if (val < 0) {
    val = -val;
    *pc++ = '-';
  }

  // Calculate the fractional part and exponent for base 10
  int exp2;
  double frac2 = frexp(val, &exp2);
  int exp10 = (int)floor(exp2 * M_LN2_LN10);
  double frac10 = exp(exp2 * M_LN2 - exp10 * M_LN10) * frac2;
  
  // Shift by one multiple of 10 if necessary
  if ((frac10 < 1) && (frac10 != 0)) {
    frac10 *= 10;
    exp10--;
  }

  double test = frac10 * pow(10, exp10);
  post("%f %f - (%f) * 10^ (%i)", val, test, frac10, exp10);

  // Set the first significant digit
  int digit = (int)frac10;
  *pc++ = '0' + digit;
  *pc++ = '.';

  // Set the fractional part
  for (int i = 0; i < prec; i++) {
    frac10 = (frac10 - digit) * 10;
    digit = (int)frac10;
    *pc++ = '0' + digit;
  }

  // Process the sign of the exponent
  *pc++ = 'E';
  if (exp10 >= 0) {
    *pc++ = '+';
  }
  else {
    exp10 = -exp10;
    *pc++ = '-';
  }

  // Set the exponent
  char* pc0 = pc;
  do {
    *pc++ = '0' + (exp10 % 10);
    exp10 /= 10;
  } while (exp10);
  char* pc1 = pc - 1;
  
  // Reverse the exponent string
  char tmp;
  while (pc0 < pc1) {
    tmp = *pc0;
    *pc0++ = *pc1;
    *pc1-- = tmp;
  }
  
  *pc = '\0';

  post("%s", str);

  return (int)(pc - str);;
}

//******************************************************************************
//  Verify the values of a dstring.
//
//  @param dstr The dstring to verify.
//  @param len_cur The expected current length value.
//  @param len_max The expected maximum length value.
//  @param cstr The expected C string
//
//  @return True if the dstring is verified, false otherwise.
//
t_bool dstr_verify(t_dstr dstr,
  t_dstr_int len_cur, t_dstr_int len_max, const char* cstr, const char* msg) {

  if ((dstr->len_cur == len_cur)
    && (dstr->len_max == len_max)
    && (strcmp(dstr->cstr, cstr) == 0)) {
    return true;
  }
  else {
    post("%s", msg);
    post("    %i - %i - <%s>", dstr->len_cur, dstr->len_max, dstr->cstr);
    post("    %i - %i - <%s>", len_cur, len_max, cstr);
    return false;
  }
}

//******************************************************************************
//  Test the dstring functions.
//
void dstr_test() {

  dstr_verify(NULL_DSTR, 0, DSTR_LEN_ERR, "", "1: NULL_DSTR");
  
  t_dstr dstr = dstr_new();
  dstr_verify(dstr, 0, 8, "", "2: dstr_new");
  dstr_free(&dstr);
  dstr_verify(dstr, 0, DSTR_LEN_ERR, "", "3: dstr_new / free");

  dstr = dstr_new_n(7);
  dstr_verify(dstr, 0, 7, "", "4: dstr_new_n");
  _dstr_set_to_null(dstr);
  dstr_verify(dstr, 0, DSTR_LEN_ERR, "", "5: dstr_new_n / dstr_to_null");
  dstr_free(&dstr);
  dstr_verify(dstr, 0, DSTR_LEN_ERR, "", "6: dstr_new_n / dstr_to_null / free");

  dstr = dstr_new_cstr("abcdef");
  dstr_verify(dstr, 6, 8, "abcdef", "7: dstr_new_cstr");
  dstr_cat_cstr(dstr, "");
  dstr_verify(dstr, 6, 8, "abcdef", "8: dstr_cat_cstr");
  dstr_cat_cstr(dstr, "1");
  dstr_verify(dstr, 7, 8, "abcdef1", "9: dstr_cat_cstr");
  dstr_cat_cstr(dstr, "2");
  dstr_verify(dstr, 8, 8, "abcdef12", "10: dstr_cat_cstr");
  dstr_cat_cstr(dstr, "3");
  dstr_verify(dstr, 9, 16, "abcdef123", "11: dstr_cat_cstr");
  dstr_fit(dstr);
  dstr_verify(dstr, 9, 9, "abcdef123", "12: dstr_fit");
  dstr_fit(dstr);
  dstr_verify(dstr, 9, 9, "abcdef123", "13: dstr_fit");
  dstr_resize(dstr, 6);
  dstr_verify(dstr, 6, 6, "abcdef", "14: dstr_resize");
  dstr_resize(dstr, 12);
  dstr_verify(dstr, 6, 12, "abcdef", "15: dstr_resize");
  dstr_clip(dstr, 7);
  dstr_verify(dstr, 6, 12, "abcdef", "16: dstr_clip");
  dstr_clip(dstr, 6);
  dstr_verify(dstr, 6, 12, "abcdef", "17: dstr_clip");
  dstr_clip(dstr, 5);
  dstr_verify(dstr, 5, 12, "abcde", "18: dstr_clip");
  dstr->cstr[5] = '#';
  dstr->cstr[6] = '\0';
  dstr_update(dstr);
  dstr_verify(dstr, 6, 12, "abcde#", "19: dstr_update");
  dstr->cstr[3] = '\0';
  dstr_update(dstr);
  dstr_verify(dstr, 3, 12, "abc", "20: dstr_update");
  dstr_clear(dstr);
  dstr_verify(dstr, 0, 12, "", "21: dstr_update");
  dstr_cat_cstr(dstr, "abcdef");

  t_dstr dstr2 = dstr_new_dstr(dstr);
  dstr_verify(dstr2, 6, 8, "abcdef", "22: dstr_new_dstr");
  
  dstr_free(&dstr);
  dstr_free(&dstr2);

  dstr = dstr_new_bin("abc\0def", 7);
  dstr_verify(dstr, 7, 8, "abc\0def", "23: dstr_new_bin");
  dstr_free(&dstr);
  dstr = dstr_new_int(0);
  dstr_verify(dstr, 1, 1, "0", "24: dstr_new_int");
  dstr_free(&dstr);
  dstr = dstr_new_int(123);
  dstr_verify(dstr, 3, 4, "123", "25: dstr_new_int");
  dstr_free(&dstr);
  dstr = dstr_new_int(-123);
  dstr_verify(dstr, 4, 4, "-123", "26: dstr_new_int");
  dstr_free(&dstr);
  dstr = dstr_new_int(9223372036854775807);
  dstr_verify(dstr, 19, 32, "9223372036854775807", "27: dstr_new_int");
  dstr_free(&dstr);
  dstr = dstr_new_int(-9223372036854775807);
  dstr_verify(dstr, 20, 32, "-9223372036854775807", "28: dstr_new_int");
  dstr_free(&dstr);

  dstr = dstr_new_float(1.2345, 6);
  dstr_verify(dstr, 8, 8, "1.234500", "29: dstr_new_float");
  dstr_free(&dstr);
  dstr = dstr_new_float(-1.2345, 6);
  dstr_verify(dstr, 9, 16, "-1.234500", "30: dstr_new_float");
  dstr_free(&dstr);

  dstr = dstr_new_float_sci(1234.5678, 6);
  dstr_verify(dstr, 12, 16, "1.234568E+03", "31: dstr_new_float_sci");
  dstr_free(&dstr);
  dstr = dstr_new_float_sci(-0.00012345, 6);
  dstr_verify(dstr, 13, 16, "-1.234500E-04", "32: dstr_new_float_sci");
  dstr_free(&dstr);
  dstr = dstr_new_float_sci(1234.567890123456789, 22);
  dstr_verify(dstr, 28, 32, "1.2345678901234568911605E+03",
    "33: dstr_new_float_sci");
  dstr_free(&dstr);
  dstr = dstr_new_float_sci(-0.0001234567890123456789, 22);
  dstr_verify(dstr, 29, 32, "-1.2345678901234567129835E-04",
    "34: dstr_new_float_sci");
  dstr_free(&dstr);

  dstr = dstr_new_printf("%i / %.1f / %.1E / %s", 1, 1.234, 0.00001234, "ab");
  dstr_verify(dstr, 22, 32, "1 / 1.2 / 1.2E-05 / ab", "35: dstr_new_printf");
  dstr_free(&dstr);
  dstr = dstr_new_printf("%i / %.6f / %.6E / %s", 1, 1.234, 0.00001234,
    "abcdefghi");
  dstr_verify(dstr, 39, 64, "1 / 1.234000 / 1.234000E-05 / abcdefghi",
    "36: dstr_new_printf");
  dstr_free(&dstr);

  t_atom atom[1];
  atom_setlong(atom, 1234);
  dstr = dstr_new_atom(atom);
  dstr_verify(dstr, 4, 4, "1234", "37: dstr_new_atom");
  dstr_free(&dstr);
  atom_setfloat(atom, 1.234);
  dstr = dstr_new_atom(atom);
  dstr_verify(dstr, 8, 8, "1.234000", "38: dstr_new_atom");
  dstr_free(&dstr);
  atom_setsym(atom, gensym("abc"));
  dstr = dstr_new_atom(atom);
  dstr_verify(dstr, 3, 4, "abc", "39: dstr_new_atom");
  dstr_free(&dstr);
  
  dstr = dstr_new();
  dstr2 = dstr_new();

  dstr_cat_dstr(dstr, dstr2);
  dstr_verify(dstr, 0, 8, "", "40: dstr_cat_dstr");
  dstr_cat_cstr(dstr, "123456789/");
  dstr_verify(dstr, 10, 16, "123456789/", "41: dstr_cat_cstr");
  dstr_cat_int(dstr2, 1234567890);
  dstr_verify(dstr2, 10, 16, "1234567890", "42: dstr_cat_int");
  dstr_cat_dstr(dstr, dstr2);
  dstr_verify(dstr, 20, 32, "123456789/1234567890", "43: dstr_cat_dstr");
  dstr_cat_bin(dstr, "/abcdefghi", 5);
  dstr_verify(dstr, 25, 32, "123456789/1234567890/abcd", "44: dstr_cat_bin");
  dstr_cat_cstr(dstr, "/");
  dstr_cat_float(dstr, 1, 4);
  dstr_verify(dstr, 32, 32, "123456789/1234567890/abcd/1.0000",
    "45: dstr_cat_float");
  dstr_cat_printf(dstr, "%i", 9);
  dstr_verify(dstr, 33, 64, "123456789/1234567890/abcd/1.00009",
    "46: dstr_cat_printf");
  dstr_clear(dstr);
  dstr_fit(dstr);
  
  dstr_cat_cstr(dstr, "0123456/");
  atom_setlong(atom, 1234);
  dstr_cat_atom(dstr, atom);
  dstr_verify(dstr, 12, 16, "0123456/1234", "47: dstr_cat_atom");
  atom_setfloat(atom, 1.234);
  dstr_cat_atom(dstr, atom);
  dstr_verify(dstr, 20, 32, "0123456/12341.234000", "48: dstr_cat_atom");
  atom_setsym(atom, gensym("/abc"));
  dstr_cat_atom(dstr, atom);
  dstr_verify(dstr, 24, 32, "0123456/12341.234000/abc", "49: dstr_cat_atom");
  dstr_cat_printf(dstr, "/%i/%.2f/%s", -9, 1.234, "abcde");
  dstr_verify(dstr, 38, 64, "0123456/12341.234000/abc/-9/1.23/abcde",
    "50: dstr_cat_printf");
  dstr_clip(dstr, 10);
  dstr_cat_printf(dstr, "/%i/%.8f/%s", -12345678, -1.23456789, "abcdefghi");
  dstr_verify(dstr, 42, 64, "0123456/12/-12345678/-1.23456789/abcdefghi",
    "51: dstr_cat_printf");
  dstr_clear(dstr);
  dstr_resize(dstr, 0);
  dstr_cat_float_sci(dstr, -1.2345678901234567890, 25);
  dstr_verify(dstr, 32, 32, "-1.2345678901234566904321355E+00",
    "52: dstr_cat_float_sci");
  dstr_cat_cstr(dstr, "/");
  dstr_cat_float_sci(dstr, -1.23456, 2);
  dstr_verify(dstr, 42, 64, "-1.2345678901234566904321355E+00/-1.23E+00",
    "53: dstr_cat_float_sci");

  t_atom list[3];
  atom_setlong(list, 1);
  atom_setfloat(list + 1, 2);
  atom_setsym(list + 2, gensym("abc"));
  dstr_clear(dstr);
  dstr_resize(dstr, 0);
  dstr_cat_join(dstr, 0, list, " // ");
  dstr_verify(dstr, 0, 0, "", "54: dstr_cat_join");
  dstr_cat_join(dstr, 1, list, " // ");
  dstr_verify(dstr, 1, 1, "1", "55: dstr_cat_join");
  dstr_cat_join(dstr, 3, list, " // ");
  dstr_verify(dstr, 21, 32, "11 // 2.000000 // abc", "56: dstr_cat_join");

  dstr_fit(dstr_clear(dstr));
  dstr_cat_cstr(dstr, "abcXXdefXXghi");
  dstr_replace(dstr, "XX", "==");
  dstr_verify(dstr, 13, 16, "abc==def==ghi", "57: dstr_replace");
  dstr_replace(dstr, "==", "-");
  dstr_verify(dstr, 11, 16, "abc-def-ghi", "58: dstr_replace");
  dstr_replace(dstr, "-", "[1]");
  dstr_verify(dstr, 15, 16, "abc[1]def[1]ghi", "59: dstr_replace");
  dstr_replace(dstr, "abc", "ABC");
  dstr_verify(dstr, 15, 16, "ABC[1]def[1]ghi", "60: dstr_replace");
  dstr_replace(dstr, "ABC", "#");
  dstr_verify(dstr, 13, 16, "#[1]def[1]ghi", "61: dstr_replace");
  dstr_replace(dstr, "#", "abcdef");
  dstr_verify(dstr, 18, 32, "abcdef[1]def[1]ghi", "62: dstr_replace");
  dstr_replace(dstr, "ghi", "GHI");
  dstr_verify(dstr, 18, 32, "abcdef[1]def[1]GHI", "63: dstr_replace");
  dstr_replace(dstr, "GHI", "#");
  dstr_verify(dstr, 16, 32, "abcdef[1]def[1]#", "64: dstr_replace");
  dstr_replace(dstr, "#", "abcdef");
  dstr_verify(dstr, 21, 32, "abcdef[1]def[1]abcdef", "65: dstr_replace");

  _dstr_set_to_null(dstr);
  dstr_verify(dstr, 0, DSTR_LEN_ERR, "", "66: null");
  dstr_cat_cstr(dstr, "abc");
  dstr_verify(dstr, 0, DSTR_LEN_ERR, "", "67: null");
  dstr_cat_dstr(dstr, dstr2);
  dstr_verify(dstr, 0, DSTR_LEN_ERR, "", "68: null");
  dstr_cat_bin(dstr, "abc", 3);
  dstr_verify(dstr, 0, DSTR_LEN_ERR, "", "69: null");
  dstr_cat_int(dstr, 1);
  dstr_verify(dstr, 0, DSTR_LEN_ERR, "", "70: null");
  dstr_cat_float(dstr, 1.2, 2);
  dstr_verify(dstr, 0, DSTR_LEN_ERR, "", "71: null");
  dstr_cat_float_sci(dstr, 1.2, 2);
  dstr_verify(dstr, 0, DSTR_LEN_ERR, "", "72: null");
  dstr_cat_printf(dstr, "%i", 1);
  dstr_verify(dstr, 0, DSTR_LEN_ERR, "", "73: null");
  dstr_cat_atom(dstr, list);
  dstr_verify(dstr, 0, DSTR_LEN_ERR, "", "74: null");
  dstr_clip(dstr, 8);
  dstr_verify(dstr, 0, DSTR_LEN_ERR, "", "75: null");
  dstr_update(dstr);
  dstr_verify(dstr, 0, DSTR_LEN_ERR, "", "76: null");
  dstr_fit(dstr);
  dstr_verify(dstr, 0, DSTR_LEN_ERR, "", "77: null");
  dstr_shrink(dstr);
  dstr_verify(dstr, 0, DSTR_LEN_ERR, "", "78: null");
  dstr_resize(dstr, 8);
  dstr_verify(dstr, 0, DSTR_LEN_ERR, "", "79: null");
  dstr_cat_join(dstr, 3, list, " // ");
  dstr_verify(dstr, 0, DSTR_LEN_ERR, "", "80: null");
  dstr_replace(dstr, "XX", "==");
  dstr_verify(dstr, 0, DSTR_LEN_ERR, "", "81: null");
  dstr_clear(dstr);
  dstr_verify(dstr, 0, DSTR_LEN_ERR, "", "82: null");
  dstr_free(&dstr);

  _dstr_set_to_null(NULL_DSTR);
  dstr_verify(NULL_DSTR, 0, DSTR_LEN_ERR, "", "83: null");
  dstr_cat_cstr(NULL_DSTR, "abc");
  dstr_verify(NULL_DSTR, 0, DSTR_LEN_ERR, "", "84: null");
  dstr_cat_dstr(NULL_DSTR, dstr2);
  dstr_verify(NULL_DSTR, 0, DSTR_LEN_ERR, "", "85: null");
  dstr_cat_bin(NULL_DSTR, "abc", 3);
  dstr_verify(NULL_DSTR, 0, DSTR_LEN_ERR, "", "86: null");
  dstr_cat_int(NULL_DSTR, 1);
  dstr_verify(NULL_DSTR, 0, DSTR_LEN_ERR, "", "87: null");
  dstr_cat_float(NULL_DSTR, 1.2, 2);
  dstr_verify(NULL_DSTR, 0, DSTR_LEN_ERR, "", "88: null");
  dstr_cat_float_sci(NULL_DSTR, 1.2, 2);
  dstr_verify(NULL_DSTR, 0, DSTR_LEN_ERR, "", "89: null");
  dstr_cat_printf(NULL_DSTR, "%i", 1);
  dstr_verify(NULL_DSTR, 0, DSTR_LEN_ERR, "", "90: null");
  dstr_cat_atom(NULL_DSTR, list);
  dstr_verify(NULL_DSTR, 0, DSTR_LEN_ERR, "", "91: null");
  dstr_clip(NULL_DSTR, 8);
  dstr_verify(NULL_DSTR, 0, DSTR_LEN_ERR, "", "92: null");
  dstr_update(NULL_DSTR);
  dstr_verify(NULL_DSTR, 0, DSTR_LEN_ERR, "", "93: null");
  dstr_fit(NULL_DSTR);
  dstr_verify(NULL_DSTR, 0, DSTR_LEN_ERR, "", "94: null");
  dstr_shrink(NULL_DSTR);
  dstr_verify(NULL_DSTR, 0, DSTR_LEN_ERR, "", "95: null");
  dstr_resize(NULL_DSTR, 8);
  dstr_verify(NULL_DSTR, 0, DSTR_LEN_ERR, "", "96: null");
  dstr_cat_join(NULL_DSTR, 3, list, " // ");
  dstr_verify(NULL_DSTR, 0, DSTR_LEN_ERR, "", "97: null");
  dstr_replace(NULL_DSTR, "XX", "==");
  dstr_verify(NULL_DSTR, 0, DSTR_LEN_ERR, "", "98: null");
  dstr_clear(NULL_DSTR);
  dstr_verify(NULL_DSTR, 0, DSTR_LEN_ERR, "", "99: null");

  _dstr_set_to_null(dstr2);

  dstr = dstr_new_dstr(dstr2);
  dstr_verify(dstr, 0, DSTR_LEN_ERR, "", "100: null");
  dstr_free(&dstr);

  dstr = dstr_new_cstr("abcd");
  dstr_cat_dstr(dstr, dstr2);
  dstr_verify(dstr, 0, DSTR_LEN_ERR, "", "101: null");

  dstr_free(&dstr);
  dstr_free(&dstr2);

  dstr = dstr_new_dstr(NULL_DSTR);
  dstr_verify(dstr, 0, DSTR_LEN_ERR, "", "102: null");
  dstr_free(&dstr);

  dstr = dstr_new_cstr("abcd");
  dstr_cat_dstr(dstr, NULL_DSTR);
  dstr_verify(dstr, 0, DSTR_LEN_ERR, "", "103: null");
  dstr_free(&dstr);

  dstr = NULL_DSTR;
  dstr_free(&dstr);

  dstr_verify(NULL_DSTR, 0, DSTR_LEN_ERR, "", "1: NULL_DSTR");
}
