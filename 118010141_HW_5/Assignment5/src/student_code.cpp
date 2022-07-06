#include "student_code.h"
#include "mutablePriorityQueue.h"

using namespace std;

namespace CGL
{

  Vector2D lerp2D(Vector2D p1, Vector2D p2, float t) {
    return t * p1 + (1 - t) * p2;
  }

  Vector3D lerp3D(Vector3D p1, Vector3D p2, double t) {
    return t * p1 + (1 - t) * p2;
  }

  /**
   * Evaluates one step of the de Casteljau's algorithm using the given points and
   * the scalar parameter t (class member).
   *
   * @param points A vector of points in 2D
   * @return A vector containing intermediate points or the final interpolated vector
   */
  std::vector<Vector2D> BezierCurve::evaluateStep(std::vector<Vector2D> const &points)
  { 
    // TODO Task 1.
    std::vector<Vector2D> intermediate_points;
    for (int i = 0; i < points.size() - 1; i++) {
      intermediate_points.push_back(lerp2D(points[i], points[i+1], this->t));
    }

    return intermediate_points;
  }

  /**
   * Evaluates one step of the de Casteljau's algorithm using the given points and
   * the scalar parameter t (function parameter).
   *
   * @param points    A vector of points in 3D
   * @param t         Scalar interpolation parameter
   * @return A vector containing intermediate points or the final interpolated vector
   */
  std::vector<Vector3D> BezierPatch::evaluateStep(std::vector<Vector3D> const &points, double t) const
  {
    // TODO Task 2.
    std::vector<Vector3D> intermediate_points;
    for (int i = 0; i < points.size() - 1; i++) {
      intermediate_points.push_back(lerp3D(points[i], points[i+1], t));
    }
    return intermediate_points;
  }

  /**
   * Fully evaluates de Casteljau's algorithm for a vector of points at scalar parameter t
   *
   * @param points    A vector of points in 3D
   * @param t         Scalar interpolation parameter
   * @return Final interpolated vector
   */
  Vector3D BezierPatch::evaluate1D(std::vector<Vector3D> const &points, double t) const
  {
    // TODO Task 2.
    std::vector<Vector3D> intermediate_points = this->evaluateStep(points, t);
    while (intermediate_points.size() > 1) {
      intermediate_points = this->evaluateStep(intermediate_points, t);
    }
    return intermediate_points[0];
  }

  /**
   * Evaluates the Bezier patch at parameter (u, v)
   *
   * @param u         Scalar interpolation parameter
   * @param v         Scalar interpolation parameter (along the other axis)
   * @return Final interpolated vector
   */
  Vector3D BezierPatch::evaluate(double u, double v) const 
  {  
    // TODO Task 2.
    std::vector<Vector3D> row_points;
    Vector3D final;
    for (int i = 0; i < this->controlPoints.size(); i++) {
      row_points.push_back(this->evaluate1D(this->controlPoints[i], u));
    }
    final = this->evaluate1D(row_points, v);
    return final;
  }

  Vector3D Vertex::normal( void ) const
  {
    // TODO Task 3.
    // Returns an approximate unit normal at this vertex, computed by
    // taking the area-weighted average of the normals of neighboring
    // triangles, then normalizing.
    HalfedgeCIter h = halfedge();
    Vector3D N(0.0, 0.0, 0.0);

    do {
      Vector3D pA = h->vertex()->position;
      Vector3D pB = h->next()->vertex()->position;
      Vector3D pC = h->next()->next()->vertex()->position;
      Vector3D cross_product = cross(pB - pA, pC - pB);
      N += cross_product;
      
      h = h->twin()->next();
    } while (h != halfedge());

    return N.unit();
  }

  EdgeIter HalfedgeMesh::flipEdge( EdgeIter e0 )
  {
    // TODO Task 4.
    // This method should flip the given edge and return an iterator to the flipped edge.
    HalfedgeIter bc = e0->halfedge();
    HalfedgeIter cb = bc->twin();
    HalfedgeIter ca = bc->next();
    HalfedgeIter ac = ca->twin();
    HalfedgeIter ab = ca->next();
    HalfedgeIter ba = ab->twin();
    HalfedgeIter bd = cb->next();
    HalfedgeIter db = bd->twin();
    HalfedgeIter dc = bd->next();
    HalfedgeIter cd = dc->twin();
    HalfedgeIter ad = bc;
    HalfedgeIter da = cb;
    
    VertexIter a = ab->vertex();
    VertexIter b = bc->vertex();
    VertexIter c = ca->vertex();
    VertexIter d = dc->vertex();

    EdgeIter Ebc = e0;
    EdgeIter Eca = ca->edge();
    EdgeIter Eab = ab->edge();
    EdgeIter Ebd = bd->edge();
    EdgeIter Edc = dc->edge();
    EdgeIter Ead = Ebc;

    FaceIter Fabc = bc->face();
    FaceIter Fcbd = cb->face();
    FaceIter Fadc = Fabc;
    FaceIter Fabd = Fcbd;

    a->halfedge() = ad;
    b->halfedge() = bd;
    c->halfedge() = ca;
    d->halfedge() = da;

    Ebc = Ead;
    Eca->halfedge() = ca;
    Eab->halfedge() = ab;
    Ebd->halfedge() = bd;
    Edc->halfedge() = dc;
    Ead->halfedge() = ad;

    Fadc->halfedge() = ad;
    Fabd->halfedge() = da;
    Fabc = Fadc;
    Fcbd = Fabd;
    
    ca->setNeighbors(ad, ac, c, Eca, Fadc);
    ac->setNeighbors(ac->next(), ca, a, Eca, ac->face());
    ab->setNeighbors(bd, ba, a, Eab, Fabd);
    ba->setNeighbors(ba->next(), ab, b, Eab, ba->face());
    bd->setNeighbors(da, db, b, Ebd, Fabd);
    db->setNeighbors(db->next(), bd, d, Ebd, db->face());
    dc->setNeighbors(ca, cd, d, Edc, Fadc);
    cd->setNeighbors(cd->next(), dc, c, Edc, cd->face());
    ad->setNeighbors(dc, da, a, Ead, Fadc);
    da->setNeighbors(ab, ad, d, Ead, Fabd);
    bc = ad;
    cb = da;
    
    // this->check_f or(ad);
    // cout << endl;
    // this->check_for(da);

    return Ead;
  }

  VertexIter HalfedgeMesh::splitEdge( EdgeIter e0 )
  {
    // TODO Task 5.
    // This method should split the given edge and return an iterator to the newly inserted vertex.
    // The halfedge of this vertex should point along the edge that was split, rather than the new edges.
    HalfedgeIter bc = e0->halfedge();
    HalfedgeIter cb = bc->twin();
    HalfedgeIter ca = bc->next();
    HalfedgeIter ac = ca->twin();
    HalfedgeIter ab = ca->next();
    HalfedgeIter ba = ab->twin();
    HalfedgeIter bd = cb->next();
    HalfedgeIter db = bd->twin();
    HalfedgeIter dc = bd->next();
    HalfedgeIter cd = dc->twin();
    HalfedgeIter mc = newHalfedge();
    HalfedgeIter cm = newHalfedge();
    HalfedgeIter bm = newHalfedge();
    HalfedgeIter mb = newHalfedge();
    HalfedgeIter am = newHalfedge();
    HalfedgeIter ma = newHalfedge();
    HalfedgeIter md = newHalfedge();
    HalfedgeIter dm = newHalfedge();
  
    VertexIter a = ab->vertex();
    VertexIter b = bc->vertex();
    VertexIter c = ca->vertex();
    VertexIter d = dc->vertex();
    VertexIter m = newVertex();

    EdgeIter Ebc = e0;
    EdgeIter Eca = ca->edge();
    EdgeIter Eab = ab->edge();
    EdgeIter Ebd = bd->edge();
    EdgeIter Edc = dc->edge();
    EdgeIter Emc = newEdge();
    EdgeIter Ebm = newEdge();
    EdgeIter Eam = newEdge();
    EdgeIter Emd = newEdge();

    FaceIter Fabc = bc->face();
    FaceIter Fcbd = cb->face();
    FaceIter Famc = newFace();
    FaceIter Fmdc = newFace();
    FaceIter Fabm = newFace();
    FaceIter Fmbd = newFace();

    a->halfedge() = am; // ab
    b->halfedge() = bm;
    c->halfedge() = cm;
    d->halfedge() = dm; // dc
    m->halfedge() = mc;
    m->isNew = true;
    m->position = (b->position + c->position) / 2;
    m->newPosition = e0->newPosition;

    // Ebc = Emc;
    Eca->halfedge() = ca;
    Eab->halfedge() = ab;
    Ebd->halfedge() = bd;
    Edc->halfedge() = dc;
    Emc->halfedge() = mc;
    Ebm->halfedge() = bm;
    Eam->halfedge() = am;
    Emd->halfedge() = md;
    Eam->isNew = true;
    Emd->isNew = true;
    Ebm->isNew = true;
    Emc->isNew = true;

    Famc->halfedge() = am;
    Fmdc->halfedge() = md;
    Fabm->halfedge() = bm;
    Fmbd->halfedge() = mb;

    ca->setNeighbors(am, ac, c, Eca, Famc);
    ac->setNeighbors(ac->next(), ca, a, Eca, ac->face());
    ab->setNeighbors(bm, ba, a, Eab, Fabm);
    ba->setNeighbors(ba->next(), ab, b, Eab, ba->face());
    bd->setNeighbors(dm, db, b, Ebd, Fmbd);
    db->setNeighbors(db->next(), bd, d, Ebd, db->face());
    dc->setNeighbors(cm, cd, d, Edc, Fmdc);
    cd->setNeighbors(cd->next(), dc, c, Edc, cd->face());
    mc->setNeighbors(ca, cm, m, Emc, Famc);
    cm->setNeighbors(md, mc, c, Emc, Fmdc);
    bm->setNeighbors(ma, mb, b, Ebm, Fabm);
    mb->setNeighbors(bd, bm, m, Ebm, Fmbd);
    am->setNeighbors(mc, ma, a, Eam, Famc);
    ma->setNeighbors(ab, am, m, Eam, Fabm);
    md->setNeighbors(dc, dm, m, Emd, Fmdc);
    dm->setNeighbors(mb, md, d, Emd, Fmbd);

    deleteEdge(Ebc);
    deleteFace(Fabc); deleteFace(Fcbd);
    deleteHalfedge(bc); deleteHalfedge(cb);
    
    return m;
  }



  void MeshResampler::upsample( HalfedgeMesh& mesh )
  {
    // TODO Task 6.
    // This routine should increase the number of triangles in the mesh using Loop subdivision.
    // One possible solution is to break up the method as listed below.

    // 1. Compute new positions for all the vertices in the input mesh, using the Loop subdivision rule,
    // and store them in Vertex::newPosition. At this point, we also want to mark each vertex as being
    // a vertex of the original mesh.
    // cout << "check1" << endl;

    for (VertexIter v = mesh.verticesBegin(); v != mesh.verticesEnd(); v++) {
      int n = v->degree();
      double u = 3.0 / (8 * n);
      if (n == 3) u = 3.0 / 16;

      HalfedgeIter h = v->halfedge();
      Vector3D position_sum(0.0, 0.0, 0.0);
      do {
        VertexIter neighbor = h->next()->vertex();
        position_sum += neighbor->position;
        h = h->twin()->next();
      } while (h != v->halfedge());
      

      v->newPosition = (1 - u * n) * v->position + u * position_sum;
    }

    // cout << "check2" << endl;

    // 2. Compute the updated vertex positions associated with edges, and store it in Edge::newPosition.
    for (EdgeIter e = mesh.edgesBegin(); e != mesh.edgesEnd(); e++) {
      HalfedgeIter h = e->halfedge();
      VertexIter A = h->vertex();
      VertexIter B = h->twin()->vertex();
      VertexIter C = h->next()->next()->vertex();
      VertexIter D = h->twin()->next()->next()->vertex();

      e->newPosition = 0.375 * (A->position + B->position) + 0.125 * (C->position + D->position);
    }

    // cout << "check3" << endl;
    // 3. Split every edge in the mesh, in any order. For future reference, we're also going to store some
    // information about which subdivide edges come from splitting an edge in the original mesh, and which edges
    // are new, by setting the flat Edge::isNew. Note that in this loop, we only want to iterate over edges of
    // the original mesh---otherwise, we'll end up splitting edges that we just split (and the loop will never end!)  
    vector<EdgeIter> flip_edges;

    EdgeIter e = mesh.edgesBegin();
    EdgeIter next = e;
    while (e != mesh.edgesEnd()) {
      next = e;
      next++;
      if (!e->isNew) {
        VertexIter m = mesh.splitEdge(e);
        flip_edges.push_back(m->halfedge()->next()->next()->edge());
        flip_edges.push_back(m->halfedge()->twin()->next()->edge());
      }
      e = next;
    }

    // cout << "check4" << endl;  
    // 4. Flip any new edge that connects an old and new vertex.
    for (EdgeIter e : flip_edges) {
      if (e->isNew) {
        HalfedgeIter h = e->halfedge();
        VertexIter start = h->vertex();
        VertexIter end = h->twin()->vertex();
        if (start->isNew ^ end->isNew) {
          mesh.flipEdge(e);
        }
      }
    }
    // cout << "check5" << endl;

    // 5. Copy the new vertex positions into final Vertex::position.
    for (VertexIter v = mesh.verticesBegin(); v != mesh.verticesEnd(); v++) {
        v->position = v->newPosition;
        v->isNew = false;
    }

    for (EdgeIter e = mesh.edgesBegin(); e != mesh.edgesEnd(); e++) {
      e->isNew = false;
    }
    // cout << "check6" << endl;

    return;
  }
  
}
