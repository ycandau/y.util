#ifndef YC_DSTRING_H_
#define YC_DSTRING_H_

//==============================================================================
//  Header files
//==============================================================================

#include <string.h>
#include <float.h>
#include "ext.h"

//==============================================================================
//  Typedef declarations and type sizes
//==============================================================================

typedef struct _dstring_struct t_dstr_struct;
typedef struct _dstring_struct* t_dstr;

typedef unsigned __int32 t_dstr_int;

//==============================================================================
//  Extern variables declarations
//==============================================================================

extern char _null_cstr[];
extern t_dstr_struct _null_dstr_struct;

//==============================================================================
//  Defines
//==============================================================================

#define DSTR_INT_SIZE 32

// Max length is -2 to avoid overflow on (index + 1) and reserve terminal space
// And -1 is reserved for errors
#define DSTR_LEN_MAX ((t_dstr_int)-2)
#define DSTR_LEN_ERR ((t_dstr_int)-1)

// Max string length for integers
#define DSTR_I64_LEN_MAX 21  // see _MAX_I64TOSTR_BASE10_COUNT: (1 + 19 + 1)

// Max string length for doubles: [-][17 + .][E+][###][\0]
#define DSTR_F64_PREC_MAX 16
#define DSTR_F64_LEN_MAX  25  // (1 + 17 + 1 + 2 + 3 + 1)

// Fixed buffer size for initial printf attempt
#define DSTR_PRINTF_TRY_LEN 512

// Test if the dynamic string is NULL
#define DSTR_IS_NULL(ds) ((ds)->len_max == DSTR_LEN_ERR)
#define DSTR_NOT_NULL(ds) ((ds)->len_max != DSTR_LEN_ERR)

// Test if the dynamic string has been clipped
#define DSTR_IS_CLIPPED(ds) ((ds)->len_cur == DSTR_LEN_MAX)

//==============================================================================
//  Dynamic string structure
//==============================================================================

//******************************************************************************
//  Dynamic string structure
//
//  A valid dstring verifies:
//    dstr != NULL_DSTR
//    cstr != NULL
//    len_max <= DSTR_LEN_MAX
//    len_cur <= len_max
//    (which also implies len_cur <= DSTR_LEN_MAX)
//
//  Allocation errors for the main structure set the dstr pointer to NULL_DSTR.
//  Allocation errors for the C string member set
//    cstr to NULL_CSTR, len_cur to 0, len_max to DSTR_LEN_ERR.
//  Overflow errors clip the C string member at DSTR_LEN_MAX.
//  A dstring with len_cur == DSTR_LEN_MAX is assumed to be clipped.
//  
//  NULL_DSTR is a static variable, with NULL_DSTR->cstr = "<NULL>"
//  to ensure standard string functions do not crash
//
struct _dstring_struct {

	char *cstr;
  t_dstr_int len_cur;
  t_dstr_int len_max;
};

//==============================================================================
//  Function declarations
//==============================================================================

//******************************************************************************
//  Create an empty dstring.
//
//  @return The new dstring.
//
t_dstr dstr_new();

//******************************************************************************
//  Create an empty dstring of length up to N.
//
//  @param len The length to allocate (ANY).
//
//  @return The new dstring.
//
t_dstr dstr_new_n(t_dstr_int len);

//******************************************************************************
//  Create a dstring from a C string.
//
//  @param cstr A C string to initialize the dstring (ENSURE: cstr != NULL).
//
//  @return The new dstring.
//
t_dstr dstr_new_cstr(const char *cstr);

//******************************************************************************
//  Create a dstring from a dstring.
//
//  @param dstr A dstring with which to initialize the dstring (ANY).
//
//  @return The new dstring.
//
t_dstr dstr_new_dstr(const t_dstr dstr);

//******************************************************************************
//  Create a dstring from a binary string.
//
//  @param bin A binary string with which to initialize the dstring (ANY).
//  @param len The length of the binary string (ANY).
//
//  @return The new dstring.
//
t_dstr dstr_new_bin(const char *bin, t_dstr_int len);

//******************************************************************************
//  Create a dstring from an int value.
//
//  @param i The int value to convert (ANY).
//
//  @return The new dstring.
//
t_dstr dstr_new_int(const __int64 i);

//******************************************************************************
//  Create a dstring from a double value.
//
//  @param val The double value to convert (ANY).
//  @param prec The number of digits after the decimal point (ANY).
//
//  @return The dstring.
//
t_dstr dstr_new_float(double val, t_uint8 prec);

//******************************************************************************
//  Create a dstring from a double value, using scientific notation.
//
//  @param val The double value to convert (ANY).
//  @param prec The number of digits after the decimal point (ANY).
//
//  @return The dstring.
//
t_dstr dstr_new_float_sci(double val, t_uint8 prec);

//******************************************************************************
//  Create a dstring from a printf style format string and arguments.
//
//  @param format The printf style formatting string (printf format).
//  @param ... The variadic arguments for printf (printf format).
//
//  @return The new dstring.
//
t_dstr dstr_new_printf(const char *format, ...);

//******************************************************************************
//  Create a dstring from an atom.
//
//  @param atom A pointer to the atom (ANY).
//
//  @return The dstring.
//
t_dstr dstr_new_atom(const t_atom* atom);

//******************************************************************************
//  Create a dstring from an atom value and its type.
//
//  @param atom A pointer to the atom (ANY).
//
//  @return The dstring.
//
t_dstr dstr_new_atom_type(const t_atom* atom);

//******************************************************************************
//  Free a dstring.
//
//  The dstring is set to NULL_DSTR.
//
//  @param dstr A pointer to the dstring to free (ANY).
//
void   dstr_free(t_dstr *dstr);

//******************************************************************************
//  Concatenate a C string into a dstring.
//
//  @param dest The dstring to concatenate into (ANY).
//  @param cstr The C string to concatenate (ENSURE: cstr != NULL).
//
//  @return The dstring.
//
t_dstr dstr_cat_cstr(t_dstr dest, const char *cstr);

//******************************************************************************
//  Concatenate a dstring into a dstring.
//
//  @param dest The dstring to concatenate into (ANY).
//  @param dstr The dstring to concatenate (ANY).
//
//  @return The dstring.
//
t_dstr dstr_cat_dstr(t_dstr dest, const t_dstr dstr);

//******************************************************************************
//  Concatenate a binary string into a dstring.
//
//  @param dest The dstring to concatenate into (ANY).
//  @param bin The binary string to concatenate (ANY).
//
//  @return The dstring.
//
t_dstr dstr_cat_bin(t_dstr dest, const char *src, t_dstr_int len);

//******************************************************************************
//  Concatenate a string from an int value into a dstring.
//
//  @param dest The dstring to concatenate into (ANY).
//  @param i The int value to concatenate as a string (ANY).
//
//  @return The dstring.
//
t_dstr dstr_cat_int(t_dstr dest, const __int64 i);

//******************************************************************************
//  Concatenate a string from a double value into a dstring.
//
//  @param dest The dstring to concatenate into (ANY).
//  @param val The double value to convert (ANY).
//  @param prec The number of digits after the decimal point (ANY).
//
//  @return The dstring.
//
t_dstr dstr_cat_float(t_dstr dest, double val, t_uint8 prec);

//******************************************************************************
//  Concatenate a string from a double value, using scientific notation.
//
//  @param dest The dstring to concatenate into (ANY).
//  @param val The double value to convert (ANY).
//  @param prec The number of digits after the decimal point (ANY).
//
//  @return The dstring.
//
t_dstr dstr_cat_float_sci(t_dstr dest, double val, t_uint8 prec);

//******************************************************************************
//  Concatenate a printf style generated string into a dstring.
//
//  @param dest The dstring to concatenate into (ANY).
//  @param format The printf style formatting string (printf format).
//  @param ... The variadic arguments for printf (printf format).
//
//  @return The dstring.
//
t_dstr dstr_cat_printf(t_dstr dest, const char *format, ...);

//******************************************************************************
//  Concatenate a string from an atom into a dstring.
//
//  @param dest The dstring to concatenate into (ANY).
//  @param atom A pointer to the atom (ANY).
//
//  @return The dstring.
//
t_dstr dstr_cat_atom(t_dstr dest, const t_atom* atom);

//******************************************************************************
//  Concatenate a string from an atom value and its type.
//
//  @param dest The dstring to concatenate into (ANY).
//  @param atom A pointer to the atom (ANY).
//
//  @return The dstring.
//
t_dstr dstr_cat_atom_type(t_dstr dest, const t_atom* atom);

//******************************************************************************
//  Clear a dstring without resizing.
//
//  @param dstr The dstring to clear (ANY).
//
//  @return The dstring.
//
t_dstr dstr_clear(t_dstr dstr);

//******************************************************************************
//  Clip the dstring to a set length.
//
//  @param dstr The dstring to resize (ANY).
//  @param len The length at which to clip (ANY).
//
//  @return The dstring.
//
t_dstr dstr_clip(t_dstr dstr, t_dstr_int len);

//******************************************************************************
//  Update the current length of a dstring, in case its C string was modified.
//
//  @param dstr The dstring to update (ANY).
//
//  @return The dstring.
//
t_dstr dstr_update(t_dstr dstr);

//******************************************************************************
//  Resize the dstring to fit the current length.
//
//  @param dstr The dstring to resize (ANY).
//
//  @return The dstring.
//
t_dstr dstr_fit(t_dstr dstr);

//******************************************************************************
//  Resize the dstring to smallest power of 2 greater than the current length.
//
//  @param dstr The dstring to resize (ANY).
//
//  @return The dstring.
//
t_dstr dstr_shrink(t_dstr dstr);

//******************************************************************************
//  Resize the dstring to a set length, clipping the C string of necessary.
//
//  @param dstr The dstring to resize (ANY).
//  @param len The length to resize to (ANY).
//
//  @return The dstring.
//
t_dstr dstr_resize(t_dstr dstr, t_dstr_int len);

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
t_dstr dstr_cat_join(t_dstr dest, long argc, t_atom* argv, char* sep);

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
  t_dstr dest, long argc, t_atom_long* longs, char* sep);

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
  t_dstr dest, long argc, t_atom_float* floats, t_uint8 prec, char* sep);

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
  t_dstr dest, long argc, t_symbol** symbols, char* sep);

//******************************************************************************
//  Search and replace in a dstring.
//
//  @param dstr The dstring to search and replace in (ANY).
//  @param search The C string to search for (ENSURE: search != NULL).
//  @param replace The C string to replace with (ENSURE: search != NULL).
//
//  @return The dstring.
//
t_dstr dstr_replace(t_dstr dstr, const char *search, const char *replace);

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
  t_dstr_int len_cur, t_dstr_int len_max, const char* cstr, const char* msg);

//******************************************************************************
//  Test the dstring functions.
//
void dstr_test();

#endif
