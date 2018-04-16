#ifndef YC_MAX_UTIL_H_
#define YC_MAX_UTIL_H_

//==============================================================================
//  Header files
//==============================================================================

#include "ext.h"

//==============================================================================
//  Defines
//==============================================================================

#define POST(...) do { object_post((t_object*)x, __VA_ARGS__); } while (0)
#define WARN(...) do { object_warn((t_object*)x, __VA_ARGS__); } while (0)

#define CLIP(x, low, high) ((x) < (low) ? (low) : ((x) > (high) ? (high) : (x)))


//******************************************************************************
//  Utility function to set the properties of an attribute.
//
//  @param c The class.
//  @param attrname The name of the attribute as a C string.
//  @param order The order of the attribute in the inspector menu.
//  @param category The name of the category in the inspector menu.
//  @param style The style of the attribute ("text", "onoff", "rgba", "enum",
//    "enumindex", "rect", "font", "file")
//  @param label A short description for the inspector menu.
//  @param def Default values.
//  @param paint Toggle automatic call to paint on attribute change.
//
void attr_set_propr(t_class* c, const char* attrname,
  const char* order, const char* category, const char* style,
  const char* label, const char* def);

//******************************************************************************
//  Utility function to set methods for an attribute.
//
//  @param c The class.
//  @param attrname The name of the attribute as a C string.
//  @param setter Custom setter method or NULL.
//  @param getter Custom getter method or NULL.
//  @param filter Custom filter method or NULL.
//
void attr_set_methods(t_class* c, const char* attrname,
  method setter, method getter, method filter);

#endif
