//==============================================================================
//
//  @file multigain.c
//  @author Yves Candau <ycandau@sfu.ca>
//
//  @brief A Max external to calculate multi-panning gain values.
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
#include "args_util.h"
#include <math.h>

//==============================================================================
//  Defines
//==============================================================================

#define M_PI_2    1.57079632679489661923
#define M_LN10    2.30258509299404568402
#define M_LN10_20 0.115129254649702284201

//==============================================================================
//  Structure declaration for the object
//==============================================================================

typedef struct _multigain {

  t_object obj;

  // Outlets
  void** outlets;
  double* gains_chan;
  double* gains_adjust;
  double  gain;
  t_uint8 chan_cnt;

  // Attributes
  char a_verbose;

} t_multigain;

//******************************************************************************
//  Global pointer to the class.
//
static t_class* multigain_class = NULL;

//==============================================================================
//  Function declarations
//==============================================================================

// Object methods
void* multigain_new(t_symbol* sym, long argc, t_atom* argv);
void multigain_free(t_multigain* x);
void multigain_assist(t_multigain* x, void* b, long msg, long arg, char* str);
void multigain_bang(t_multigain* x);
void multigain_int(t_multigain* x, long val);
void multigain_float(t_multigain* x, double val);
void multigain_list(t_multigain* x, t_symbol* sym, long argc, t_atom* argv);
void multigain_anything(
  t_multigain* x, t_symbol* sym, long argc, t_atom* argv);
void multigain_adjust(
  t_multigain* x, t_symbol* sym, long argc, t_atom* argv);
void multigain_adjust_one(
  t_multigain* x, t_symbol* sym, long argc, t_atom* argv);
void multigain_report(t_multigain* x);

//==============================================================================
//  Class definition and life cycle
//==============================================================================

//******************************************************************************
//  Create the Max class and initialize it.
//
void C74_EXPORT ext_main(void* r) {

  t_class* c;

  c = class_new(
    "y.multigain",
    (method)multigain_new,
    (method)multigain_free,
    (long)sizeof(t_multigain),
    (method)NULL,
    A_GIMME,
    0);

  class_addmethod(c, (method)multigain_assist, "assist", A_CANT, 0);
  class_addmethod(c, (method)multigain_bang, "bang", 0);
  class_addmethod(c, (method)multigain_int, "int", A_LONG, 0);
  class_addmethod(c, (method)multigain_float, "float", A_FLOAT, 0);
  class_addmethod(c, (method)multigain_list, "list", A_GIMME, 0);
  class_addmethod(c, (method)multigain_anything, "anything", A_GIMME, 0);
  class_addmethod(c, (method)multigain_adjust, "adjust", A_GIMME, 0);
  class_addmethod(c, (method)multigain_adjust_one, "adjust_one", A_GIMME, 0);
  class_addmethod(c, (method)multigain_report, "report", 0);

  // Attribute: 'verbose' to turn warnings on or off
  CLASS_ATTR_CHAR(c, "verbose", 0, t_multigain, a_verbose);
  CLASS_ATTR_ORDER(c, "verbose", 0, "2");
  CLASS_ATTR_STYLE(c, "verbose", 0, "onoff");
  CLASS_ATTR_LABEL(c, "verbose", 0, "Report warnings");
  CLASS_ATTR_SAVE(c, "verbose", 0);
  CLASS_ATTR_SELFSAVE(c, "verbose", 0);

  class_register(CLASS_BOX, c);
  multigain_class = c;
}

//******************************************************************************
//  Create a new instance of the class.
//
void* multigain_new(t_symbol* sym, long argc, t_atom* argv) {

  t_multigain* x = NULL;
  x = (t_multigain*)object_alloc(multigain_class);

  if (x == NULL) {
    error("floor: Object allocation failed.");
    return NULL;
  }

  // Process arguments: get the number of channels
  t_atom_long cnt = 0;
  if (argc == 0) {
    x->chan_cnt = 2;
  }
  else if ((argc == 1)
      && (atom_gettype(argv) == A_LONG)
      && ((cnt = atom_getlong(argv)) >= 2)
      && (cnt <= 0xFF)) {
    x->chan_cnt = (t_uint8)cnt;
  }
  else {
    x->chan_cnt = 2;
    WARN("Invalid arg(0): [int: 2-255] - number of channels");
  }

  // Allocate dynamic arrays
  x->outlets = NULL;
  x->gains_chan = NULL;
  x->gains_adjust = NULL;
  x->outlets = (void**)sysmem_newptr(x->chan_cnt * sizeof(void*));
  x->gains_chan = (double*)sysmem_newptr(x->chan_cnt * sizeof(double));
  x->gains_adjust = (double*)sysmem_newptr(x->chan_cnt * sizeof(double));
  if ((!x->outlets) || (!x->gains_chan) || (!x->gains_chan)) {
    multigain_free(x);
    object_error((t_object*)x, "Allocation error");
    return NULL;
  }

  // Initialize
  x->gain = 1.0;
  for (int i = x->chan_cnt - 1; i >= 0; i--) {  // Outlet 0 is leftmost
    x->outlets[i] = floatout(x);
    x->gains_chan[i] = 0.0;
    x->gains_adjust[i] = 1.0;
  }

  return x;
}

//******************************************************************************
//  Free the instance.
//
void multigain_free(t_multigain* x) {

  if (x->outlets) { sysmem_freeptr(x->outlets); }
  if (x->gains_chan) { sysmem_freeptr(x->gains_chan); }
  if (x->gains_adjust) { sysmem_freeptr(x->gains_adjust); }
  x->outlets = NULL;
  x->gains_chan = NULL;
  x->gains_adjust = NULL;
}

//******************************************************************************
//  Assist function.
//
void multigain_assist(t_multigain* x, void* b, long msg, long arg, char* str) {

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
      sprintf(str, "Rounded down values from input. Symbols are passed unchanged.");
      break;
    default:
      break;
    }
  }
}

//******************************************************************************
//  Output the gains.
//
inline void multigain_output(t_multigain* x) {

  for (int i = x->chan_cnt - 1; i >= 0; i--) {
    post("%i - %.3f %.3f %.3f", i, x->gain, x->gains_chan[i], x->gains_adjust[i]);
    outlet_float(x->outlets[i],
      x->gain * x->gains_chan[i] * x->gains_adjust[i]);
  }
}

//******************************************************************************
//  Process incoming integers.
//
void multigain_bang(t_multigain* x) {

  dstr_test();
  multigain_output(x);
}

//******************************************************************************
//  Process incoming integers.
//
void multigain_int(t_multigain* x, long val) {

  // Initialize to 0
  for (int i = 0; i < x->chan_cnt; i++) {
    x->gains_chan[i] = 0;
  }
  // Calculate pan values
  x->gains_chan[CLIP(val, 0, x->chan_cnt - 1)] = 1;

  multigain_output(x);
}

//******************************************************************************
//  Process incoming floats, rounding down.
//
void multigain_float(t_multigain* x, double val) {

  // Initialize to 0
  for (int i = 0; i < x->chan_cnt; i++) {
    x->gains_chan[i] = 0;
  }

  // Calculate pan values
  if (val <= 0) {
    x->gains_chan[0] = 1;
  }
  else if (val >= x->chan_cnt - 1) {
    x->gains_chan[x->chan_cnt - 1] = 1;
  }
  else {
    int index = (int)val;
    double r = cos((val - index) * M_PI_2);
    x->gains_chan[index] = r;
    x->gains_chan[index + 1] = sqrt(1 - r * r);
  }

  multigain_output(x);
}

//******************************************************************************
//  Process incoming lists.
//
void multigain_list(t_multigain* x, t_symbol* sym, long argc, t_atom* argv) {

}

//******************************************************************************
//  Process incoming messages.
//
void multigain_anything(
  t_multigain* x, t_symbol* sym, long argc, t_atom* argv) {

}

//******************************************************************************
//  Set the adjustment gains.
//
void multigain_adjust(
  t_multigain* x, t_symbol* sym, long argc, t_atom* argv) {

  t_symbol* symbols[2] = { gensym("ampl"), gensym("db") };
  if (args_count_is(x, sym, argc, x->chan_cnt + 1)
    && args_is_sym(x, sym, argv, 0, 2, symbols)) {

    if (atom_getsym(argv) == gensym("ampl")
      && args_are_numbers(x, sym, argv, 1, x->chan_cnt, is_above_f, 0, 0)) {
      for (int i = 0; i < x->chan_cnt; i++) {
        x->gains_adjust[i] = atom_getfloat(argv + i + 1);
      }
    }
    else if (atom_getsym(argv) == gensym("db")
      && args_are_numbers(x, sym, argv, 1, x->chan_cnt, NULL, 0, 0)) {
      for (int i = 0; i < x->chan_cnt; i++) {
        x->gains_adjust[i] = exp(atom_getfloat(argv + i + 1) * M_LN10_20);
      }
    }
  }
}

//******************************************************************************
//  Set the adjustment gains by decibels.
//
void multigain_adjust_one(
  t_multigain* x, t_symbol* sym, long argc, t_atom* argv) {

  t_symbol* symbols[2] = { gensym("ampl"), gensym("db") };
  if (args_count_is(x, sym, argc, 3)
    && args_is_sym(x, sym, argv, 0, 2, symbols)
    && args_is_long(x, sym, argv, 1, is_between_l, 0, x->chan_cnt - 1)) {

    if ((atom_getsym(argv) == gensym("ampl"))
      && args_is_number(x, sym, argv, 2, is_above_f, 0, 0)) {
      x->gains_adjust[atom_getlong(argv + 1)] =
        atom_getfloat(argv + 2);
    }
    else if ((atom_getsym(argv) == gensym("db"))
      && args_is_number(x, sym, argv, 2, NULL, 0, 0)) {
      x->gains_adjust[atom_getlong(argv + 1)] =
        exp(atom_getfloat(argv + 2) * M_LN10_20);
    }
  }
}

//******************************************************************************
//  Post the structure values in the console.
//
void multigain_report(t_multigain* x) {

  t_dstr dstr = dstr_new();
  dstr_cat_printf(dstr, "Channels: %i - Gain: %.4f", x->chan_cnt, x->gain);
  POST("%s", dstr->cstr);
  dstr_clear(dstr);
  dstr_cat_cstr(dstr, "    Channel gains: ");
  dstr_cat_join_floats(dstr, x->chan_cnt, x->gains_chan, 4, ", ");
  POST("%s", dstr->cstr);
  dstr_clear(dstr);
  dstr_cat_cstr(dstr, "    Adjust gains:    ");
  dstr_cat_join_floats(dstr, x->chan_cnt, x->gains_adjust, 4, ", ");
  POST("%s", dstr->cstr);
  dstr_free(&dstr);
}
