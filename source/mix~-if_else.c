//==============================================================================
//
//  @file mixer~.c
//  @author Yves Candau <ycandau@sfu.ca>
//
//  @brief A Max external to mix channels.
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
#include "z_dsp.h"
#include "args_util.h"
#include "max_util.h"
#include <math.h>

//==============================================================================
//  Defines
//==============================================================================

#define M_PI_2    1.57079632679489661923
#define M_LN10    2.30258509299404568402
#define M_LN10_20 0.115129254649702284201

#define RAMP_DEF 30
#define CNTD_END (t_uint32)(-1)

//==============================================================================
//  Structure declaration for the object
//==============================================================================

typedef struct _mix {

t_pxobject obj;

t_uint8 chan_in_cnt;
t_uint8 chan_out_cnt;

double  master;
double  master_targ;
double* gains;
double* gains_targ;
double* gains_adjust;

t_uint32 cntd;

// Attributes
char a_verbose;
float a_ramp;
t_uint32 ramp_samp;

void* outlet_mess;

} t_mix;

//******************************************************************************
//  Global pointer to the class.
//
static t_class* mix_class = NULL;

//==============================================================================
//  Function declarations
//==============================================================================

// Object methods
void* mix_new(t_symbol* sym, long argc, t_atom* argv);
void mix_free(t_mix* x);
void mix_dsp64(t_mix* x, t_object* dsp64, t_int32* count,
  t_double samplerate, long maxvectorsize, long flags);
void mix_perform64(t_mix* x, t_object* dsp64, t_double** ins, long numins,
  t_double** outs, long numouts, long sampleframes, long flags, void* param);
void mix_assist(t_mix* x, void* b, long msg, long arg, char* str);

t_max_err mix_set_ramp(t_mix* x, t_object* attr, long argc, t_atom* argv);

void mix_bang(t_mix* x);
void mix_int(t_mix* x, long val);
void mix_float(t_mix* x, double val);
void mix_list(t_mix* x, t_symbol* sym, long argc, t_atom* argv);
void mix_anything(t_mix* x, t_symbol* sym, long argc, t_atom* argv);

void mix_pan(t_mix* x, double master, double pan);

void mix_master(t_mix* x, double master);
void mix_adjust(t_mix* x, t_symbol* sym, long argc, t_atom* argv);
void mix_adjust_one(t_mix* x, t_symbol* sym, long argc, t_atom* argv);
void mix_report(t_mix* x);

//==============================================================================
//  Class definition and life cycle
//==============================================================================

//******************************************************************************
//  Create the Max class and initialize it.
//
void C74_EXPORT ext_main(void* r) {

  t_class* c;

  c = class_new(
    "y.mix~",
    (method)mix_new,
    (method)mix_free,
    (long)sizeof(t_mix),
    (method)NULL,
    A_GIMME,
    0);

  class_addmethod(c, (method)mix_dsp64, "dsp64", A_CANT, 0);
  class_addmethod(c, (method)mix_assist, "assist", A_CANT, 0);

  class_addmethod(c, (method)mix_bang, "bang", 0);
  class_addmethod(c, (method)mix_int, "int", A_LONG, 0);
  class_addmethod(c, (method)mix_float, "float", A_FLOAT, 0);
  class_addmethod(c, (method)mix_list, "list", A_GIMME, 0);
  class_addmethod(c, (method)mix_anything, "anything", A_GIMME, 0);

  class_addmethod(c, (method)mix_pan, "pan", A_FLOAT, A_FLOAT, 0);

  class_addmethod(c, (method)mix_master, "master", A_FLOAT, 0);
  class_addmethod(c, (method)mix_adjust, "adjust", A_GIMME, 0);
  class_addmethod(c, (method)mix_adjust_one, "adjust_one", A_GIMME, 0);
  class_addmethod(c, (method)mix_report, "report", 0);

  // Attributes
  CLASS_ATTR_FLOAT(c, "ramp", 0, t_mix, a_ramp);
  attr_set_propr(c, "ramp", "1", NULL, NULL, "Ramp time in ms", "30");
  CLASS_ATTR_ACCESSORS(c, "ramp", NULL, mix_set_ramp);

  CLASS_ATTR_CHAR(c, "verbose", 0, t_mix, a_verbose);
  attr_set_propr(c, "verbose", "2", NULL, "onoff", "Report warnings", "1");

  class_dspinit(c);
  class_register(CLASS_BOX, c);
  mix_class = c;
}

//******************************************************************************
//  Create a new instance of the class.
//
void* mix_new(t_symbol* sym, long argc, t_atom* argv) {

  t_mix* x = NULL;
  x = (t_mix*)object_alloc(mix_class);

  if (x == NULL) {
    error("floor: Object allocation failed.");
    return NULL;
  }

  // Process arguments
  args_count_is_between(x, sym, argc, 0, 2);
  x->chan_in_cnt =
    (argc >= 1) && args_is_long(x, sym, argv, 0, is_between_l, 2, 0xFF)
    ? (t_uint8)atom_getlong(argv)
    : 4;
  x->chan_out_cnt =
    (argc >= 2) && args_is_long(x, sym, argv + 1, 0, is_between_l, 1, 2)
    ? (t_uint8)atom_getlong(argv + 1)
    : 1;

  // Inlets and outlets
  dsp_setup((t_pxobject*)x, x->chan_in_cnt * x->chan_out_cnt);
  x->outlet_mess = outlet_new((t_object*)x, NULL);
  for (int i = 0; i < x->chan_out_cnt; i++) {
    outlet_new((t_object*)x, "signal");
  }
  x->obj.z_misc |= Z_NO_INPLACE;

  // Allocate the dynamic arrays
  x->gains = NULL;
  x->gains_targ = NULL;
  x->gains_adjust = NULL;
  x->gains = (double*)sysmem_newptr(x->chan_in_cnt * sizeof(double));
  x->gains_targ = (double*)sysmem_newptr(x->chan_in_cnt * sizeof(double));
  x->gains_adjust = (double*)sysmem_newptr(x->chan_in_cnt * sizeof(double));
  if ((!x->gains) || (!x->gains_targ) || (!x->gains_adjust)) {
    mix_free(x);
    object_error((t_object*)x, "Allocation error");
    return NULL;
  }

  // Initialize
  x->master = 1.0;
  x->master_targ = 1.0;
  for (int i = 0; i < x->chan_in_cnt; i++) {
    x->gains[i] = 0.0;
    x->gains_targ[i] = 0.0;
    x->gains_adjust[i] = 1.0;
  }
  object_attr_setfloat(x, gensym("ramp"), RAMP_DEF);

  return x;
}

//******************************************************************************
//  Free the instance.
//
void mix_free(t_mix* x) {

  dsp_free((t_pxobject*)x);
  if (x->gains) { sysmem_freeptr(x->gains); }
  if (x->gains_targ) { sysmem_freeptr(x->gains_targ); }
  if (x->gains_adjust) { sysmem_freeptr(x->gains_adjust); }
  x->gains = NULL;
  x->gains_targ = NULL;
  x->gains_adjust = NULL;
}

//******************************************************************************
//  Called when the DAC is turned on.
//
void mix_dsp64(t_mix* x, t_object* dsp64, t_int32* count,
  t_double samplerate, long maxvectorsize, long flags) {

  object_method(dsp64, gensym("dsp_add64"), x, mix_perform64, 0, NULL);
  x->ramp_samp = (t_uint32)(x->a_ramp * samplerate / 1000);
}

//******************************************************************************
//  Audio function for mono mode.
//
void mix_perform64_mono(
  t_mix* x, t_double** ins, t_double** outs, long sampleframes) {

  t_uint32 chunk_len = 0;
  t_uint32 samp_left = sampleframes;
  t_uint32 samp_proc = 0;

  t_double gain0;
  t_double dgain;

  t_double* in1 = NULL;
  t_double* out1 = outs[0];

  // Initialize output to 0.0
  for (int s = 0; s < sampleframes; s++) {
    out1[s] = 0.0;
  }
  if ((x->master == 0) && (x->master_targ == 0)) { return; }

  // Loop over chunks
  while (samp_left) {

    // ==== No ramping
    if (x->cntd == CNTD_END) {
      chunk_len = samp_left;

      // == Loop over the input channels
      for (int i = 0; i < x->chan_in_cnt; i++) {
        gain0 = x->master * x->gains_adjust[i] * x->gains[i];
        if (gain0 != 0) {
          in1 = ins[i] + samp_proc;
          for (t_uint32 s = 0; s < chunk_len; s++) {
            out1[s] += gain0 * in1[s];
          }
        }
      }
    }

    // ==== Ramping
    else {
      chunk_len = (x->cntd > samp_left) ? samp_left : x->cntd;

      // == Loop over the input channels
      for (int i = 0; i < x->chan_in_cnt; i++) {
        in1 = ins[i] + samp_proc;

        // No ramping if the current and target gain are equals
        if (x->gains[i] == x->gains_targ[i]) {
          gain0 = x->gains_adjust[i] * x->gains[i];
          if (gain0 != 0) {
            for (t_uint32 s = 0; s < chunk_len; s++) {
              out1[s] += gain0 * in1[s];
            }
          }
        }
        // Otherwise ramp
        else {
          gain0 = x->gains_adjust[i] * x->gains[i];
          dgain = x->gains_adjust[i] * (x->gains_targ[i] - x->gains[i]) / x->cntd;
          for (t_uint32 s = 0; s < chunk_len; s++) {
            out1[s] += (gain0 + s * dgain) * in1[s];
          }
          x->gains[i] += chunk_len * (x->gains_targ[i] - x->gains[i]) / x->cntd;
        }
      }

      // == Multiply by the master gain
      // No master ramping if the current and target values are equals
      if (x->master == x->master_targ) {
        for (t_uint32 s = 0; s < chunk_len; s++) {
          out1[s] *= x->master;
        }
      }
      else {
        gain0 = x->master;
        dgain = (x->master_targ - x->master) / x->cntd;
        for (t_uint32 s = 0; s < chunk_len; s++) {
          out1[s] *= gain0 + s * dgain;
        }
        x->master = gain0 + dgain;
      }

      // Process the potential end of the countdown
      x->cntd -= chunk_len;
      if (x->cntd == 0) {
        x->cntd = CNTD_END;
        x->master = x->master_targ;
        for (int i = 0; i < x->chan_in_cnt; i++) {
          x->gains[i] = x->gains_targ[i];
        }
      }
    }
    // Increment the chunk through the audio vectors
    samp_left -= chunk_len;
    samp_proc += chunk_len;
  }
}


//******************************************************************************
//  Audio function for stereo mode.
//
void mix_perform64(t_mix* x, t_object* dsp64, t_double** ins, long numins,
  t_double** outs, long numouts, long sampleframes, long flags, void* param) {
  // void mix_perform64_stereo(
  //   t_mix* x, t_double** ins, t_double** outs, long sampleframes) {

  t_uint32 chunk_len = 0;
  t_uint32 samp_left = sampleframes;
  t_uint32 samp_proc = 0;

  t_double gain;
  t_double gain0;
  t_double dgain;

  t_double* in1 = NULL;
  t_double* in2 = NULL;
  t_double* out1 = outs[0];
  t_double* out2 = outs[1];

  // Initialize output to 0.0
  for (int s = 0; s < sampleframes; s++) {
    out1[s] = 0.0;
    out2[s] = 0.0;
  }
  if ((x->master == 0) && (x->master_targ == 0)) { return; }

  // Loop over chunks
  while (samp_left) {

    // ==== No ramping
    if (x->cntd == CNTD_END) {
      chunk_len = samp_left;

      // == Loop over the input channels
      for (int i = 0; i < x->chan_in_cnt; i++) {
        gain = x->master * x->gains_adjust[i] * x->gains[i];
        if (gain != 0) {
          in1 = ins[i] + samp_proc;
          in2 = ins[i + x->chan_in_cnt] + samp_proc;
          for (t_uint32 s = 0; s < chunk_len; s++) {
            out1[s] += gain * in1[s];
            out2[s] += gain * in2[s];
          }
        }
      }
    }

    // ==== Ramping
    else {
      chunk_len = (x->cntd > samp_left) ? samp_left : x->cntd;

      // == Loop over the input channels
      for (int i = 0; i < x->chan_in_cnt; i++) {
        in1 = ins[i] + samp_proc;
        in2 = ins[i + x->chan_in_cnt] + samp_proc;

        // No ramping if the current and target gain are equals
        if (x->gains[i] == x->gains_targ[i]) {
          gain = x->gains_adjust[i] * x->gains[i];
          if (gain != 0) {
            for (t_uint32 s = 0; s < chunk_len; s++) {
              out1[s] += gain * in1[s];
              out2[s] += gain * in2[s];
            }
          }
        }
        // Otherwise ramp
        else {
          gain0 = x->gains_adjust[i] * x->gains[i];
          dgain = x->gains_adjust[i] * (x->gains_targ[i] - x->gains[i]) / x->cntd;
          for (t_uint32 s = 0; s < chunk_len; s++) {
            gain = gain0 + s * dgain;
            out1[s] += gain * in1[s];
            out2[s] += gain * in2[s];
          }
          x->gains[i] += chunk_len * (x->gains_targ[i] - x->gains[i]) / x->cntd;
        }
      }

      // == Multiply by the master gain
      // No master ramping if the current and target values are equals
      if (x->master == x->master_targ) {
        for (t_uint32 s = 0; s < chunk_len; s++) {
          out1[s] *= x->master;
          out2[s] *= x->master;
        }
      }
      else {
        gain0 = x->master;
        dgain = (x->master_targ - x->master) / x->cntd;
        for (t_uint32 s = 0; s < chunk_len; s++) {
          gain = gain0 + s * dgain;
          out1[s] *= gain;
          out2[s] *= gain;
        }
        x->master = gain + dgain;
      }

      // Process the potential end of the countdown
      x->cntd -= chunk_len;
      if (x->cntd == 0) {
        x->cntd = CNTD_END;
        x->master = x->master_targ;
        for (int i = 0; i < x->chan_in_cnt; i++) {
          x->gains[i] = x->gains_targ[i];
        }
      }
    }
    // Increment the chunk through the audio vectors
    samp_left -= chunk_len;
    samp_proc += chunk_len;
  }
}

//******************************************************************************
//  Audio function: toggle between mono and stereo mode.
//
/*void mix_perform64__(t_mix* x, t_object* dsp64, t_double** ins, long numins,
  t_double** outs, long numouts, long sampleframes, long flags, void* param) {

  switch (numouts) {
    case 1: mix_perform64_mono(x, ins, outs, sampleframes); break;
    case 2: mix_perform64_stereo(x, ins, outs, sampleframes); break;
  }
} */

//******************************************************************************
//  Assist function.
//
void mix_assist(t_mix* x, void* b, long msg, long arg, char* str) {

  if (msg == ASSIST_INLET) {
    switch (arg) {
    case 0:
      sprintf(str, "All purpose and Audio Input %i (list / signal)", arg);
      break;
    default:
      sprintf(str, "Audio Input %i (signal)", arg);
      break;
    }
  }
  else if (msg == ASSIST_OUTLET) {
    if (arg < x->chan_out_cnt) {
      sprintf(str, "Audio Output %i (signal)", arg);
    }
    else {
      sprintf(str, "All purpose (list)");
    }
  }
}

//******************************************************************************
//  Set the ramp attribute.
//
t_max_err mix_set_ramp(t_mix* x, t_object* attr, long argc, t_atom* argv) {

  if (args_count_is(x, gensym("attr ramp"), argc, 1)
    && args_is_number(x, gensym("attr ramp"), argv, 0, is_above_f, 1, 0)) {
    x->a_ramp = (float)atom_getfloat(argv);
  }
  else {
    x->a_ramp = (float)RAMP_DEF;
  }
  x->ramp_samp = (t_uint32)(x->a_ramp * sys_getsr() / 1000);
  return MAX_ERR_NONE;
}

//******************************************************************************
//  Process incoming integers.
//
void mix_bang(t_mix* x) {

}

//******************************************************************************
//  Process incoming integers.
//
void mix_int(t_mix* x, long val) {

}

//******************************************************************************
//  Process incoming floats, rounding down.
//
void mix_float(t_mix* x, double val) {

}

//******************************************************************************
//  Process lists as the master gain and a gain for each input channel.
//
void mix_list(t_mix* x, t_symbol* sym, long argc, t_atom* argv) {

  x->master_targ = atom_getfloat(argv);
  for (int i = 0; i < MIN(argc - 1, x->chan_in_cnt); i++) {
    x->gains_targ[i] = atom_getfloat(argv + i + 1);
  }
  for (int i = argc - 1; i < x->chan_in_cnt; i++) {
    x->gains_targ[i] = 0;
  }
  x->cntd = x->ramp_samp;
}

//******************************************************************************
//  Process incoming messages.
//
void mix_anything(t_mix* x, t_symbol* sym, long argc, t_atom* argv) {

}

//******************************************************************************
//  Set the master gain.
//
void mix_master(t_mix* x, double master) {

  x->master_targ = master;
  x->cntd = x->ramp_samp;
}

//******************************************************************************
//  Pan between the input channels.
//
void mix_pan(t_mix* x, double master, double pan) {

  x->master_targ = master;

  // Initialize to 0
  for (int i = 0; i < x->chan_in_cnt; i++) {
    x->gains_targ[i] = 0;
  }

  // Calculate the pan values
  if (pan <= 0) {
    x->gains_targ[0] = 1;
  }
  else if (pan >= x->chan_in_cnt - 1) {
    x->gains_targ[x->chan_in_cnt - 1] = 1;
  }
  else {
    int index = (int)pan;
    double r = cos((pan - index) * M_PI_2);
    x->gains_targ[index] = r;
    x->gains_targ[index + 1] = sqrt(1 - r * r);
  }
  x->cntd = x->ramp_samp;
}

//******************************************************************************
//  Set all the adjustment gains.
//
void mix_adjust(t_mix* x, t_symbol* sym, long argc, t_atom* argv) {

  t_symbol* symbols[2] = { gensym("ampl"), gensym("db") };
  if (args_count_is(x, sym, argc, x->chan_in_cnt + 1)
    && args_is_sym(x, sym, argv, 0, 2, symbols)) {

    if (atom_getsym(argv) == gensym("ampl")
      && args_are_numbers(x, sym, argv, 1, x->chan_in_cnt, is_above_f, 0, 0)) {
      for (int i = 0; i < x->chan_in_cnt; i++) {
        x->gains_adjust[i] = atom_getfloat(argv + i + 1);
      }
    }
    else if (atom_getsym(argv) == gensym("db")
      && args_are_numbers(x, sym, argv, 1, x->chan_in_cnt, NULL, 0, 0)) {
      for (int i = 0; i < x->chan_in_cnt; i++) {
        x->gains_adjust[i] = exp(atom_getfloat(argv + i + 1) * M_LN10_20);
      }
    }
  }
}

//******************************************************************************
//  Set one of the adjustment gains.
//
void mix_adjust_one(t_mix* x, t_symbol* sym, long argc, t_atom* argv) {

  t_symbol* symbols[2] = { gensym("ampl"), gensym("db") };
  if (args_count_is(x, sym, argc, 3)
    && args_is_sym(x, sym, argv, 0, 2, symbols)
    && args_is_long(x, sym, argv, 1, is_between_l, 0, x->chan_in_cnt - 1)) {

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
void mix_report(t_mix* x) {

  t_dstr dstr = dstr_new();
  dstr_cat_printf(dstr, "Channels IN: %i - Channels OUT: %i - "
    "Ramp (ms): %.1f - Master Gain: %.4f",
    x->chan_in_cnt, x->chan_out_cnt, x->a_ramp, x->master);
  POST("%s", dstr->cstr);
  dstr_clear(dstr);
  dstr_cat_cstr(dstr, "    Current gains: ");
  dstr_cat_join_floats(dstr, x->chan_in_cnt, x->gains, 4, ", ");
  POST("%s", dstr->cstr);
  dstr_clear(dstr);
  dstr_cat_cstr(dstr, "    Target gains: ");
  dstr_cat_join_floats(dstr, x->chan_in_cnt, x->gains_targ, 4, ", ");
  POST("%s", dstr->cstr);
  dstr_clear(dstr);
  dstr_cat_cstr(dstr, "    Adjust gains:    ");
  dstr_cat_join_floats(dstr, x->chan_in_cnt, x->gains_adjust, 4, ", ");
  POST("%s", dstr->cstr);
  dstr_free(&dstr);
}
