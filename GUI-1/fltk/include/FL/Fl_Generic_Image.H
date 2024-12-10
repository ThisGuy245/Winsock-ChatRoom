#ifndef FL_FLGENERICIMAGE_H
#define FL_FLGENERICIMAGE_H

#include <FL/Fl_RGB_Image.H>

struct Fl_Generic_Image : Fl_RGB_Image
{
  Fl_Generic_Image(const char *_path);
  ~Fl_Generic_Image();

private:
  Fl_Generic_Image(const Fl_Generic_Image& _copy);
  Fl_Generic_Image& operator=(const Fl_Generic_Image& _assign);

  void load_generic(const char *_path);
};

#endif
