#pragma once

#include "ShapeData.h"

class ShapeGenerator
{
private:
  static ShapeData makePlaneVerts(unsigned int dimensions);
  static ShapeData makePlaneIndices(unsigned int dimensions);

public:
  static ShapeData makeTriangle();
  static ShapeData makeCube();
  static ShapeData makeArrow();
  static ShapeData makePlane(unsigned int dimensions = 10);
};