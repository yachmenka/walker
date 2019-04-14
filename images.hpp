#ifndef IMAGES_HPP
#define IMAGES_HPP

#include "utils.hpp"

enum image_position
{
  IMAGE_POSITION_TOP,
  IMAGE_POSITION_LEFT,
};

enum image
{
  IMAGE_NONE,
  IMAGE_COW,
  IMAGE_CENTAUR,
  IMAGE_HORSEBACK_FIGHT,
  IMAGE_MOUNTAINS,
  IMAGE_PIKEMAN,
};

extern const text *images[];

#endif // IMAGES_HPP
