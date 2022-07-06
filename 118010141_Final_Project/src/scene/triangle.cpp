#include "triangle.h"

#include "CGL/CGL.h"
#include "GL/glew.h"

namespace CGL
{
  namespace SceneObjects
  {

    Triangle::Triangle(const Mesh *mesh, size_t v1, size_t v2, size_t v3)
    {
      p1 = mesh->positions[v1];
      p2 = mesh->positions[v2];
      p3 = mesh->positions[v3];
      n1 = mesh->normals[v1];
      n2 = mesh->normals[v2];
      n3 = mesh->normals[v3];
      bbox = BBox(p1);
      bbox.expand(p2);
      bbox.expand(p3);

      bsdf = mesh->get_bsdf();
    }

    BBox Triangle::get_bbox() const { return bbox; }

    bool Triangle::has_intersection(const Ray &r) const
    {
      // Part 1, Task 3: implement ray-triangle intersection
      // The difference between this function and the next function is that the next
      // function records the "intersection" while this function only tests whether
      // there is a intersection.


      // r(t) = o + td; p: (p - p')N = 0;
      double t = dot(p1 - r.o, n1) / dot(r.d, n1);
      // cout << "t = " << t << endl;
      // cout << r.min_t << " " << r.max_t << endl;

      if (t >= r.min_t && t <= r.max_t)
      {
        Vector3D x = r.o + t * r.d;
        double area = 0.5 * cross(p3 - p1, p2 - p1).norm();
        double alpha = 0.5 * cross(x - p2, p3 - p2).norm() / area;
        double beta = 0.5 * cross(x - p1, p3 - p1).norm() / area;
        // double gamma = 0.5 * cross(x - p1, p2 - p1).norm() / area;
        double gamma = 1 - alpha - beta;

        if (alpha >= 0 && beta >= 0 && gamma >= 0) {
          // cout << alpha << " " << beta << " " << gamma << " judge " << endl << endl;
          r.max_t = t;
          return true;
        }
      }

      return false;

    }

    bool Triangle::intersect(const Ray &r, Intersection *isect) const
    {
      // Part 1, Task 3:
      // implement ray-triangle intersection. When an intersection takes
      // place, the Intersection data should be updated accordingly
      // Moller Trumbore algorithm
      Vector3D E1 = p2 - p1;
      Vector3D E2 = p3 - p1;
      Vector3D S = r.o - p1;
      Vector3D S1 = cross(r.d, E2);
      Vector3D S2 = cross(S, E1);

      Vector3D sol = Vector3D(dot(S2, E2), dot(S1, S), dot(S2, r.d)) / dot(S1, E1);
      double t = sol[0];
      if (t >= r.min_t && t <= r.max_t) {
        double b1 = sol[1], b2 = sol[2];
        if (b1 >= 0 && b1 <= 1 && b2 >= 0 && b2 <= 1 && (b1 + b2) <= 1)
        {
          // cout << 1 - b1 - b2 << " " << b1 << " " << b2 << " value " << endl;
          r.max_t = t;
          isect->t = t;
          isect->n = ((1 - b1 - b2) * n1 + b1 * n2 + b2 * n3).unit();
          isect->primitive = this;
          isect->bsdf = this->get_bsdf();
          return true;
        }
      }
      return false;

    }

    void Triangle::draw(const Color &c, float alpha) const
    {
      glColor4f(c.r, c.g, c.b, alpha);
      glBegin(GL_TRIANGLES);
      glVertex3d(p1.x, p1.y, p1.z);
      glVertex3d(p2.x, p2.y, p2.z);
      glVertex3d(p3.x, p3.y, p3.z);
      glEnd();
    }

    void Triangle::drawOutline(const Color &c, float alpha) const
    {
      glColor4f(c.r, c.g, c.b, alpha);
      glBegin(GL_LINE_LOOP);
      glVertex3d(p1.x, p1.y, p1.z);
      glVertex3d(p2.x, p2.y, p2.z);
      glVertex3d(p3.x, p3.y, p3.z);
      glEnd();
    }

  } // namespace SceneObjects
} // namespace CGL
