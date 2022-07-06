#include "bvh.h"

#include "CGL/CGL.h"
#include "triangle.h"

#include <iostream>
#include <stack>

using namespace std;

namespace CGL {
namespace SceneObjects {

BVHAccel::BVHAccel(const std::vector<Primitive *> &_primitives,
                   size_t max_leaf_size) {

  primitives = std::vector<Primitive *>(_primitives);
  root = construct_bvh(primitives.begin(), primitives.end(), max_leaf_size);
}

BVHAccel::~BVHAccel() {
  if (root)
    delete root;
  primitives.clear();
}

BBox BVHAccel::get_bbox() const { return root->bb; }

void BVHAccel::draw(BVHNode *node, const Color &c, float alpha) const {
  if (node->isLeaf()) {
    for (auto p = node->start; p != node->end; p++) {
      (*p)->draw(c, alpha);
    }
  } else {
    draw(node->l, c, alpha);
    draw(node->r, c, alpha);
  }
}

void BVHAccel::drawOutline(BVHNode *node, const Color &c, float alpha) const {
  if (node->isLeaf()) {
    for (auto p = node->start; p != node->end; p++) {
      (*p)->drawOutline(c, alpha);
    }
  } else {
    drawOutline(node->l, c, alpha);
    drawOutline(node->r, c, alpha);
  }
}

BVHNode *BVHAccel::construct_bvh(std::vector<Primitive *>::iterator start,
                                 std::vector<Primitive *>::iterator end,
                                 size_t max_leaf_size) {

  // TODO (Part 2.1):
  // Construct a BVH from the given vector of primitives and maximum leaf
  // size configuration. The starter code build a BVH aggregate with a
  // single leaf node (which is also the root) that encloses all the
  // primitives.

  // Split the longest axis & equal object number

  BBox bbox;
  int count = 0;

  for (auto p = start; p != end; p++) {
    BBox bb = (*p)->get_bbox();
    bbox.expand(bb);
    count++;
  }

  BVHNode *node = new BVHNode(bbox);
  node->start = start;
  node->end = end;

  if (count <= max_leaf_size) {
    return node;
  }

  int axis = 0; // x - 0; y - 1; z - 2;
  for (int i = 1; i < 3; i++) {
    if (bbox.extent[i] > bbox.extent[axis]) {
      axis = i;
    }
  }

  double centroid_avg = 0.0;
  for (auto p = start; p != end; p++) {
    centroid_avg += (*p)->get_bbox().centroid()[axis] / count;
  }

  vector<Primitive*>* left_primitive = new vector<Primitive*>;
  vector<Primitive*>* right_primitive = new vector<Primitive*>;

  for (auto p = start; p != end; p++) {
    if ((*p)->get_bbox().centroid()[axis] <= centroid_avg) {
      left_primitive->push_back(*p);
    } else {
      right_primitive->push_back(*p);
    }
  }

  if (left_primitive->empty()) {
    left_primitive->push_back(right_primitive->back());
    right_primitive->pop_back();
  }
  if (right_primitive->empty()) {
    right_primitive->push_back(left_primitive->back());
    left_primitive->pop_back();
  }

  for (int i = 0; i < left_primitive->size(); i++) {
    *(start + i) = (*left_primitive)[i];
  }
  for (int i = 0; i < right_primitive->size(); i++) {
    *(start + left_primitive->size() + i) = (*right_primitive)[i];
  }
  
  // node->l = construct_bvh(left_primitive->begin(), left_primitive->end(), max_leaf_size);
  // node->r = construct_bvh(right_primitive->begin(), right_primitive->end(), max_leaf_size);
  node->l = construct_bvh(start, start + left_primitive->size(), max_leaf_size);
  node->r = construct_bvh(start + left_primitive->size(), end, max_leaf_size);

  delete left_primitive;
  delete right_primitive;

  return node;


}

bool BVHAccel::has_intersection(const Ray &ray, BVHNode *node) const {
  // TODO (Part 2.3):
  // Fill in the intersect function.
  // Take note that this function has a short-circuit that the
  // Intersection version cannot, since it returns as soon as it finds
  // a hit, it doesn't actually have to find the closest hit.
  double t0 = ray.min_t, t1 = ray.max_t;
  if (!node->bb.intersect(ray, t0, t1)) {
    return false;
  }

  if (!(t0 >= ray.min_t && t1 <= ray.max_t)) {
    return false;
  }

  if (node->isLeaf()) {
    for (auto p = node->start; p != node->end; p++) {
      total_isects++;
      if ((*p)->has_intersection(ray)) return true;
    }
    return false;
  }

  return (has_intersection(ray, node->l) || has_intersection(ray, node->r));

}

bool BVHAccel::intersect(const Ray &ray, Intersection *i, BVHNode *node) const {
  // TODO (Part 2.3):
  // Fill in the intersect function.
  double t0 = ray.min_t, t1 = ray.max_t;
  if (node == NULL || !node->bb.intersect(ray, t0, t1)) {
    return false;
  }

  // if (!(t0 >= ray.min_t && t1 <= ray.max_t)) {
  //   return false;
  // }

  if (node->isLeaf()) {
    bool hit = false;
    for (auto p = node->start; p != node->end; p++) {
      total_isects++;
      hit = (*p)->intersect(ray, i) || hit;
    }
    return hit;
  }

  bool hit1 = intersect(ray, i, node->l);
  bool hit2 = intersect(ray, i, node->r);

  return hit1 || hit2; 
}

} // namespace SceneObjects
} // namespace CGL
