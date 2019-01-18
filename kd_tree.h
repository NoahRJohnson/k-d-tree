#ifndef _KDTREE_H_
#define _KDTREE_H_

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <vector>

// template < class T, std::size_t dims >
// class Point {
//   public:

//     Point() {
//       // Default to origin
//       for (int i=0; i < dims; i++) {
//         data_[i] = 0;
//       }
//     }

//     Point(T data[dims]) {
//       data_ = data;
//     }

//     T get_num_dims() {
//       return dims;
//     }

//     T get(int axis) {
//       // Get the value of this point along one particular axis
//       return data_[axis];
//     }
//   private:
//     T data_[dims];  // fixed-length array to hold data
// };

template <class T>
class Node {

  private:
    Node<T>* left_child_;
    Node<T>* right_child_;
    std::vector<T> split_point_;

  public:

    Node(std::vector<T> split_point, Node<T>* left_child, Node<T>* right_child):
      split_point_(split_point), left_child_(left_child), right_child_(right_child) {}

    std::vector<T> const split_point() {
      return split_point_;
    }
    Node<T>* const left_child() {
      return left_child_;
    }
    Node<T>* const right_child() {
      return right_child_;
    }

    void const prettyPrint(int indent=0) {
      // https://stackoverflow.com/a/26699993
      // Prints out tree in ASCII art sideways left-to-right

      // Recurse on right child
      if (right_child_) right_child_->prettyPrint(indent+4);
      if (indent) {
        std::cout << std::setw(indent) << ' ';
      }
      if (right_child_) std::cout << " /\n" << std::setw(indent) << ' ';

      // Print root point
      std::cout << "( ";
      for (auto el : split_point_) {
        std::cout << el << ' ';
      }
      std::cout << ")" << "\n ";

      // Recurse on left child
      if (left_child_) {
        std::cout << std::setw(indent) << ' ' << " \\\n";
        left_child_->prettyPrint(indent+4);
      }
    }

    class iterator {
      private:
        Node<T>* p_;
      public:
        iterator() {}
        iterator(const iterator&) {};
        std::vector<T>& operator* () {
          return p_->split_point();  // dereferencing will return the point at the current node
        }
        iterator& operator++ () {
          p_ = p_;
          return *this;  // return reference to this iterator
        }
        iterator operator++ (int) {
          iterator tmp = *this;
          ++(*this);
          return tmp;
        }
        bool operator== (const iterator& other) const {
          return p_ == other.p_; // TODO: Fix
        }
        bool operator!= (const iterator& other) const {
          return p_ != other.p_; // TODO: Fix
        }
    };

    iterator begin() {
      return iterator(this);
    }
    iterator end() {
      return iterator();
    }

};


template <class T>
Node<T>* KDTree(typename std::vector<std::vector<T>>::iterator points_begin, typename std::vector<std::vector<T>>::iterator points_end, int depth=0) {
  /* Recursively construct a k-d tree from a set of points
  From: https://en.wikipedia.org/wiki/K-d_tree#Construction
  Params:
    points_begin: pointer to first point in points array
    points_end: pointer to one past the last point in points array
    k: the dimension of each point
    depth: depth from root of tree
  Returns:
    Root of tree
  */

  // Handle base case
  if (std::distance(points_begin, points_end) <= 0) {
    return nullptr;
  }

  // Assume all points share the same number of dimensions
  size_t k = (*points_begin).size();

  // Cycle through axes to split on
  int axis = depth % k;

  /*
  std::vector<std::vector<T>> cp(points_begin, points_end);
  std::sort(cp.begin(), cp.end(), [axis](const std::vector<T>& a, const std::vector<T>& b) {
            return a[axis] < b[axis];
            });
  // Select median point along this axis
  typename std::vector<std::vector<T>>::iterator median_it = std::next(cp.begin(), cp.size() / 2);
  std::vector<T> median_point = *(median_it);

  // Recurse on sub-arrays of points to the left and right of median point
  Node<T>* leftChild = KDTree<T>(cp.begin(), median_it, depth + 1);
  Node<T>* rightChild = KDTree<T>(std::next(median_it), cp.end(), depth + 1);
  */

  // Sort points along this axis using a lambda comparator
  std::sort(points_begin, points_end, [axis](const std::vector<T>& a, const std::vector<T>& b) {
            return a[axis] < b[axis];
            });

  // Select median point along this axis
  typename std::vector<std::vector<T>>::iterator median_it = std::next(points_begin, std::distance(points_begin, points_end) / 2);
  std::vector<T> median_point = *(median_it);

  // Recurse on sub-arrays of points to the left and right of median point
  Node<T>* leftChild = KDTree<T>(points_begin, median_it, depth + 1);
  Node<T>* rightChild = KDTree<T>(std::next(median_it), points_end, depth + 1);

  // After sub-trees have been built, construct and return this split point node
  // This memory will persist outside of this function, and will need to be deleted eventually
  // to avoid memory leaks
  Node<T>* node = new Node<T>(median_point, leftChild, rightChild);

  return node;
}

template <class T>
void deleteTree(Node<T>* root) {

  // Get children
  Node<T>* left_child = (*root).left_child();
  Node<T>* right_child = (*root).right_child();

  // Recurse over children
  if (left_child != nullptr) {
    deleteTree(left_child);
  }
  if (right_child != nullptr) {
    deleteTree(right_child);
  }

  // Free memory of this object
  delete root;

  return;
}

#endif