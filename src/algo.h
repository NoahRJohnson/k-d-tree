#ifndef KD_TEMPLATE_SRC_ALGO_H_
#define KD_TEMPLATE_SRC_ALGO_H_

#include "kd_tree.h"

namespace kd {

template <class T>
Point<T> findNN_brute_force(const Tree<T> & tree, const Point<T> & ref_point) {
  // Brute force search

  const Point<T>* best_point;
  T best_dist = std::numeric_limits<T>::max();

  for (const auto& point : tree) {  // get references so the memory addresses stay valid

    auto dist = point.distance_to(ref_point);
    if (dist < best_dist) {
      best_point = &point;
      best_dist = dist;
    }
  }

  return Point<T>(*best_point);  // copy best point
}

template <class T>
Point<T> findNN(const Tree<T> & tree, const Point<T> & ref_point) {
  /* find nearest neighbor
  wrapper for recursive search with pruning
  copies the best point in the tree to return to user

  Args:
    tree: k-d tree to search
    ref_point: the reference point whose nearest neighbor we're searching for

  Returns:
    A copy of the Point object from the tree node which was the nearest neighbor
  */

  const Tree<T> * tree_ptr = &tree;  // pointer to constant Tree object
  const Point<T> * best_point = &tree.split_point;  // pointer to root Point
  T best_dist = std::numeric_limits<T>::max();  // set initial distance to max. - assumes distance between points is of type T

  findNN_(tree_ptr, ref_point, &best_point, &best_dist);  // recursively find best_point

  return Point<T>(*best_point);  // create a copy of the Point object selected
}

template <class T>
void findNN_(const Tree<T> * tree_node, const Point<T> & ref_point,
             const Point<T> ** best_point, T * best_dist) {
  /* find nearest neighbor
  recursive search with pruning
  returns point by pointer

  Following algorithm from: https://en.wikipedia.org/wiki/K-d_tree_node#Nearest_neighbour_search

  Args:
    tree_node: k-d tree to search
    ref_point: the reference point whose nearest neighbor we're searching for
    best_point: memory to store the pointer to the best Point object in the tree
    best_dist: memory to store the current best (shortest) distance to the best point

  Returns:
    Nothing, but sets best_point to point to the Point object from the tree node which was the nearest
    neighbor to ref_point
  */

  // get a reference to the Point data for this node
  const Point<T> & local_point = tree_node->split_point;  // unmodifiable

  // calculate the squared distance between this point and the reference point
  // in overall coordinates
  T local_dist = local_point.distance_to(ref_point);

  // if this point is closer than the current best, make it the current best
  if (local_dist < *best_dist) {
    *best_point = &local_point;  // change pointer to point to this node's Point data
    *best_dist = local_dist;  // copy local_dist into variable that best_dist points to
  }

  // Move down the tree recursively according to the split value
  int split_dim = tree_node->split_axis();
  auto d = ref_point[split_dim] - local_point[split_dim];

  // Determine which direction (right or left) to go
  Tree<T>* closer_child = (d > 0) ? tree_node->right_child() : tree_node->left_child();
  Tree<T>* farther_child = (d > 0) ? tree_node->left_child() : tree_node->right_child();

  // Recurse
  if (closer_child)
    findNN_(closer_child, ref_point, best_point, best_dist);

  // Now check if we need to recurse down the other branch too
  // This would be the case if the hypersphere around the ref point with radius best_dist
  //   could include points on the other side of the hyperplane
  if (d*d < *best_dist) {  // if it could, move down the other branch

    if (farther_child)
      findNN_(farther_child, ref_point, best_point, best_dist);
  }
  else {  // if it can't, ignore the other branch (this is the pruning optimization)
    return;
  }
}

}  // end of namespace

#endif  // KD_TEMPLATE_SRC_ALGO_H_
