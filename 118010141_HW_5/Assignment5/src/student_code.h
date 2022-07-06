#ifndef STUDENT_CODE_H
#define STUDENT_CODE_H

#include "halfEdgeMesh.h"
#include "bezierPatch.h"
#include "bezierCurve.h"

using namespace std;

namespace CGL {

  class MeshResampler{

  public:

    MeshResampler(){};
    ~MeshResampler(){}

    void upsample(HalfedgeMesh& mesh);
  };

  Vector2D lerp2D(Vector2D p1, Vector2D p2, float t);
  Vector3D lerp3D(Vector3D p1, Vector3D p2, double t);
}

#endif // STUDENT_CODE_H
