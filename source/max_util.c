//==============================================================================
//  Header files
//==============================================================================

#include "max_util.h"

//==============================================================================
//  Function definitions
//==============================================================================

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
//
void attr_set_propr(t_class* c, const char* attrname,
  const char* order, const char* category, const char* style,
  const char* label, const char* def) {

  CLASS_ATTR_BASIC(c, attrname, 0);
  CLASS_ATTR_SAVE(c, attrname, 0);
  CLASS_ATTR_SELFSAVE(c, attrname, 0);
  if (order) { CLASS_ATTR_ORDER(c, attrname, 0, order); }
  if (category) { CLASS_ATTR_CATEGORY(c, attrname, 0, category); }
  if (style) { CLASS_ATTR_STYLE(c, attrname, 0, style); }
  if (label) { CLASS_ATTR_LABEL(c, attrname, 0, label); }
  if (def) { CLASS_ATTR_DEFAULT(c, attrname, 0, def); }
}

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
  method setter, method getter, method filter) {

  if (setter || getter) { CLASS_ATTR_ACCESSORS(c, attrname, getter, setter); }
  if (filter) {
    attr_addfilterset_proc(class_attr_get(c, gensym(attrname)), filter);
  }
}