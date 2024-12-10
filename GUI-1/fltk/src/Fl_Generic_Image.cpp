#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"
#include "FL/Fl_Generic_Image.H"

Fl_Generic_Image::Fl_Generic_Image(const char *_path)
  : Fl_RGB_Image(0, 0, 0)
{
  load_generic(_path);
}

void Fl_Generic_Image::load_generic(const char *_path)
{
  int x;
  int y;
  int n;

  unsigned char *bytes = stbi_load(_path, &x, &y, &n, 0);

  if(bytes)
  {
    if(array)
    {
      stbi_image_free((unsigned char *)array);
    }

    array = bytes;
    alloc_array = 1;

    w(x);
    h(y);
    d(n);
  }
}

Fl_Generic_Image::~Fl_Generic_Image()
{
  if(array)
  {
    stbi_image_free((unsigned char *)array);
    array = NULL;
    alloc_array = 0;
  }
}

