//==============================================================================
//
//  @file floor.c
//  @author Yves Candau <ycandau@sfu.ca>
//
//  @brief A Max external to round floats and lists down.
//
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
//==============================================================================

//==============================================================================
//  Header files
//==============================================================================

#include "ext.h"
#include "ext_obex.h"

//==============================================================================
//  Defines
//==============================================================================

#define WARN(...) do { object_warn((t_object*)x, __VA_ARGS__); } while (0)

//==============================================================================
//  Typedef
//==============================================================================

//******************************************************************************
//  Function pointer to toggle between outputting lists and messages.
//
typedef void* (*t_output_func)(void* outlet, t_symbol* sym, short argc,
    t_atom* argv);

//******************************************************************************
//  Function pointer to toggle between max list lengths.
//
typedef void (*t_process_func)(struct _floor* x, t_symbol* sym, short argc,
    t_atom* argv, t_output_func);

//==============================================================================
//  Structure declaration for the object
//==============================================================================

typedef struct _floor {

  t_object obj;

  // Outlets
  void* outl_output;

  // Function pointers to switch between max list length modes
  t_process_func process_message;

  // Attributes
  char a_verbose;
  t_symbol* a_maxlen_mode;
  short maxlen;

} t_floor;

//******************************************************************************
//  Global pointer to the class.
//
static t_class* floor_class = NULL;

//==============================================================================
//  Function declarations
//==============================================================================

// Object methods
void* floor_new(t_symbol* sym, long argc, t_atom* argv);
void  floor_free(t_floor* x);
void  floor_assist(t_floor* x, void* b, long msg, long arg, char* str);
void  floor_int(t_floor* x, long val);
void  floor_float(t_floor* x, double val);
void  floor_list(t_floor* x, t_symbol* sym, long argc, t_atom* argv);
void  floor_anything(t_floor* x, t_symbol* sym, long argc, t_atom* argv);

// Attribute setters
t_max_err a_set_maxlen(t_floor* x, t_object* attr, long argc, t_atom* argv);

// Process lists and messages
void process_message_256(t_floor* x, t_symbol* sym, short argc, t_atom* argv,
    t_output_func output);
void process_message_1024(t_floor* x, t_symbol* sym, short argc, t_atom* argv,
    t_output_func output);
void process_message_4096(t_floor* x, t_symbol* sym, short argc, t_atom* argv,
    t_output_func output);
void process_message_max(t_floor* x, t_symbol* sym, short argc, t_atom* argv,
    t_output_func output);
void process_message_auto(t_floor* x, t_symbol* sym, short argc, t_atom* argv,
    t_output_func output);

//******************************************************************************
//  Process incoming messages.
//
inline void process_atom(t_atom* atom_in, t_atom* atom_out) {

  switch (atom_gettype(atom_in)) {
    case A_LONG:
      atom_setlong(atom_out, atom_getlong(atom_in));
      break;
    case A_FLOAT:
      atom_setlong(atom_out, (t_atom_long)floor(atom_getfloat(atom_in)));
      break;
    case A_SYM:
      atom_setsym(atom_out, atom_getsym(atom_in));
      break;
    default:
      atom_setsym(atom_out, gensym("<error>"));
      break;
  }
}

//==============================================================================
//  Class definition and life cycle
//==============================================================================

//******************************************************************************
//  Create the Max class and initialize it.
//
void C74_EXPORT ext_main(void* r) {

  t_class* c;

  c = class_new(
    "y.floor",
    (method)floor_new,
    (method)floor_free,
    (long)sizeof(t_floor),
    (method)NULL,
    A_GIMME,
    0);

  class_addmethod(c, (method)floor_assist, "assist", A_CANT, 0);
  class_addmethod(c, (method)floor_int, "int", A_LONG, 0);
  class_addmethod(c, (method)floor_float, "float", A_FLOAT, 0);
  class_addmethod(c, (method)floor_list, "list", A_GIMME, 0);
  class_addmethod(c, (method)floor_anything, "anything", A_GIMME, 0);

  // Attribute: max list length value and mode
  CLASS_ATTR_SYM(c, "maxlen", 0, t_floor, a_maxlen_mode);
  CLASS_ATTR_ORDER(c, "maxlen", 0, "1");
  CLASS_ATTR_ENUM(c, "maxlen", 0, "256 1024 4096 max auto");
  CLASS_ATTR_LABEL(c, "maxlen", 0, "Max list length");
  CLASS_ATTR_SAVE(c, "maxlen", 0);
  CLASS_ATTR_SELFSAVE(c, "maxlen", 0);
  CLASS_ATTR_ACCESSORS(c, "maxlen", NULL, a_set_maxlen);

  // Attribute: to turn warnings on or off
  CLASS_ATTR_CHAR(c, "verbose", 0, t_floor, a_verbose);
  CLASS_ATTR_ORDER(c, "verbose", 0, "2");
  CLASS_ATTR_STYLE(c, "verbose", 0, "onoff");
  CLASS_ATTR_LABEL(c, "verbose", 0, "Report warnings");
  CLASS_ATTR_SAVE(c, "verbose", 0);
  CLASS_ATTR_SELFSAVE(c, "verbose", 0);

  class_register(CLASS_BOX, c);
  floor_class = c;
}

//******************************************************************************
//  Attribute setter for maxlen.
//
t_max_err a_set_maxlen(t_floor* x, t_object* attr, long argc, t_atom* argv) {

  // Input could be a symbol or an integer
  t_symbol* symbol = NULL;
  t_atom_long number = 0;

  switch (atom_gettype(argv)) {
  case A_LONG:
    number = atom_getlong(argv);
    break;
  case A_SYM:
    symbol = atom_getsym(argv);
    break;
  }

  if ((symbol == gensym("256")) || (number == 256)) {
    x->a_maxlen_mode = gensym("256");
    x->maxlen = 256;
    x->process_message = process_message_256;
  }
  else if ((symbol == gensym("1024")) || (number == 1024)) {
    x->a_maxlen_mode = gensym("1024");
    x->maxlen = 1024;
    x->process_message = process_message_1024;
  }
  else if ((symbol == gensym("4096")) || (number == 4096)) {
    x->a_maxlen_mode = gensym("4096");
    x->maxlen = 4096;
    x->process_message = process_message_4096;
  }
  else if (symbol == gensym("max")) {
    x->a_maxlen_mode = gensym("max");
    x->maxlen = SHRT_MAX;
    x->process_message = process_message_max;
  }
  else if (symbol == gensym("auto")) {
    x->a_maxlen_mode = symbol;
    x->maxlen = SHRT_MAX;
    x->process_message = process_message_auto;
  }
  else {
    WARN("maxlen: use 256, 1024, 4096, max or auto.");
  }

  return MAX_ERR_NONE;
}

//******************************************************************************
//  Create a new instance of the class.
//
void* floor_new(t_symbol* sym, long argc, t_atom* argv) {

  t_floor* x = NULL;

  x = (t_floor*)object_alloc(floor_class);

  if (x == NULL) {
    error("floor: Object allocation failed.");
    return NULL;
  }

  x->outl_output = outlet_new(x, NULL);

  object_attr_setsym(x, gensym("maxlen"), gensym("256"));
  object_attr_setchar(x, gensym("verbose"), 1);

  return x;
}

//******************************************************************************
//  Free the instance.
//
void floor_free(t_floor* x) {

  // Nothing to free
}

//******************************************************************************
//  Assist function.
//
void floor_assist(t_floor* x, void* b, long msg, long arg, char* str) {

  if (msg == ASSIST_INLET) {
    switch (arg) {
    case 0:
      sprintf(str, "Number, list or message to be rounded down.");
      break;
    default:
      break;
    }
  }

  else if (msg == ASSIST_OUTLET) {
    switch (arg) {
    case 0:
      sprintf(str, "Rounded down values. Symbols are passed unchanged.");
      break;
    default:
      break;
    }
  }
}

//******************************************************************************
//  Process incoming integers.
//
void floor_int(t_floor* x, long val) {

  outlet_int(x->outl_output, val);
}

//******************************************************************************
//  Process incoming floats, rounding down.
//
void floor_float(t_floor* x, double val) {

  outlet_int(x->outl_output, (t_atom_long)floor(val));
}

//******************************************************************************
//  Process incoming lists.
//
void floor_list(t_floor* x, t_symbol* sym, long argc, t_atom* argv) {

  if (argc > x->maxlen) {
    if (x->a_verbose) {
      WARN("Max list length exceeded: %i clipped to %i", argc, x->maxlen);
    }
    argc = x->maxlen;
  }
  x->process_message(x, sym, (short)argc, argv, outlet_list);
}

//******************************************************************************
//  Process incoming messages.
//
void floor_anything(t_floor* x, t_symbol* sym, long argc, t_atom* argv) {

  if (argc > x->maxlen) {
    if (x->a_verbose) {
      WARN("Max list length exceeded: %i clipped to %i", argc, x->maxlen);
    }
    argc = x->maxlen;
  }
  x->process_message(x, sym, (short)argc, argv, outlet_anything);
}

//******************************************************************************
//  Process a message of at most 256 length.
//
void process_message_256(t_floor* x, t_symbol* sym, short argc, t_atom* argv,
    t_output_func output) {

  t_atom atoms_out[256];
  for (short i = 0; i < argc; i++) {
    process_atom(argv + i, atoms_out + i);
  }
  output(x->outl_output, sym, argc, atoms_out);
}

//******************************************************************************
//  Process a message of at most 1024 length.
//
void process_message_1024(t_floor* x, t_symbol* sym, short argc, t_atom* argv,
    t_output_func output) {

  t_atom atoms_out[1024];
  for (short i = 0; i < argc; i++) {
    process_atom(argv + i, atoms_out + i);
  }
  output(x->outl_output, sym, argc, atoms_out);
}

//******************************************************************************
//  Process a message of at most 4096 length.
//
void process_message_4096(t_floor* x, t_symbol* sym, short argc, t_atom* argv,
   t_output_func output) {

  t_atom atoms_out[4096];
  for (short i = 0; i < argc; i++) {
    process_atom(argv + i, atoms_out + i);
  }
  output(x->outl_output, sym, argc, atoms_out);
}

//******************************************************************************
//  Process a message of at most SHRT_MAX length.
//
void process_message_max(t_floor* x, t_symbol* sym, short argc, t_atom* argv,
    t_output_func output) {

  t_atom atoms_out[SHRT_MAX];
  for (short i = 0; i < argc; i++) {
    process_atom(argv + i, atoms_out + i);
  }
  output(x->outl_output, sym, argc, atoms_out);
}

//******************************************************************************
// Process a message and adapt to its length.
//
void process_message_auto(t_floor* x, t_symbol* sym, short argc, t_atom* argv,
    t_output_func output) {

  if (argc <= 256) {
    process_message_256(x, sym, argc, argv, output);
  }
  else if (argc <= 1024) {
    process_message_1024(x, sym, argc, argv, output);
  }
  else if (argc <= 4096) {
    process_message_4096(x, sym, argc, argv, output);
  }
  else if (argc <= SHRT_MAX) {
    process_message_max(x, sym, argc, argv, output);
  }
}
