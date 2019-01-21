#ifndef _KDTREE_H_
#define _KDTREE_H_

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
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

template <class T, int k>
class KDTree {

  private:
    KDTree<T,k>* left_child_;
    KDTree<T,k>* right_child_;
    KDTree<T,k>* parent_;
    std::vector<T> split_point_;
    int split_axis_;

  public:

    KDTree(typename std::vector<std::vector<T>>::iterator points_begin, typename std::vector<std::vector<T>>::iterator points_end,
           int depth=0) {

      // Special case for root
      if (depth == 0) {
        parent_ = nullptr;
      }

      // Cycle through axes to split on
      int axis = depth % k;

      // Sort points along this axis using a lambda comparator
      std::sort(points_begin, points_end, [axis](const std::vector<T>& a, const std::vector<T>& b) {
                return a[axis] < b[axis];
                });

      // select median point along this axis
      typename std::vector<std::vector<T>>::iterator median_it = std::next(points_begin, std::distance(points_begin, points_end) / 2);

      // split on that median point
      split_point_ = *(median_it);

      // Recurse on sub-arrays of points to the left and right of median point
      if (std::distance(points_begin, median_it) <= 0) {
        left_child_ = nullptr;
      } else {
        left_child_ = new KDTree<T,k>(points_begin, median_it, depth + 1);
        left_child_->setParent(this);
      }
      if (std::distance(std::next(median_it), points_end) <= 0) {
        left_child_ = nullptr;
      } else {
        right_child_ = new KDTree<T,k>(std::next(median_it), points_end, depth + 1);
        right_child_->setParent(this);
      }
    }

    /*KDTree(std::vector<T> split_point, KDTree<T>* left_child, KDTree<T>* right_child, int split_axis):
      split_point_(split_point), left_child_(left_child), right_child_(right_child), split_axis_(split_axis) {}*/

    ~KDTree() {
      // recursively delete children
      delete left_child_;
      delete right_child_;
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

      // Print split point
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
      public:
        KDTree<T,k>* node_ptr_;

        iterator(): node_ptr_(nullptr) {} // empty constructor
        iterator(KDTree<T,k>* node) { // constructor which takes a KD-Tree pointer to root
          while (node->left_child() || node->right_child()) {
            if (node->left_child()) {
              node = node->left_child();
            } else {
              node = node->right_child();
            }
          }
          node_ptr_ = node;
        }
        iterator(const iterator& other) {
          node_ptr_ = other.node_ptr_;
        };
        std::vector<T> operator* () {
          return node_ptr_->split_point();  // dereferencing will return the point at the current node
        }
        iterator& operator++ () { // prefix ++
          if (node_ptr_->right_child()) { // go to the right child and find the leftmost child node
            node_ptr_ = node_ptr_->right_child();
            while (node_ptr_->left_child()) {
              node_ptr_ = node_ptr_->left_child();
            }
          } else { // if there is no right child, go up until one exists or we hit the null parent above root
            KDTree<T,k>* parent = node_ptr_->parent();
            while (parent && parent->right_child() == node_ptr_) {
              node_ptr_ = parent;
              parent = parent->parent();
            }
            if (!parent || node_ptr_->right_child() != parent) {
              node_ptr_ = parent;
            }
          }
          return *this;  // return reference to this iterator
        }
        iterator operator++ (int) { // postfix ++
          iterator tmp = *this;
          ++(*this);
          return tmp;
        }
        bool operator== (const iterator& other) const {
          return node_ptr_ == other.node_ptr_; // TODO: Fix
        }
        bool operator!= (const iterator& other) const {
          return node_ptr_ != other.node_ptr_; // TODO: Fix
        }
    };

    iterator begin() {
      return iterator(this);
    }
    iterator begin() const {
      // TODO
      ;
      //return const_iterator(this);
    }
    iterator end() const {
      return iterator();
    }

    // Setters
    void setParent(KDTree<T,k>* parent) {
      parent_ = parent;
    }

    // Getters
    KDTree<T,k>* left_child() const {
      return left_child_;
    }
    KDTree<T,k>* right_child() const {
      return right_child_;
    }
    KDTree<T,k>* parent() const {
      return parent_;
    }
    std::vector<T> split_point() const {
      return split_point_;
    }
    int split_axis() const {
      return split_axis_;
    }

};

template <class T, int k>
std::vector<T> findNN_brute_force(KDTree<T,k> & tree, std::vector<T> const& ref_point) {
  // Brute force search

  std::vector<T> current_best;
  auto current_best_dist = std::numeric_limits<int>::max();

  for (typename KDTree<T,k>::iterator it = tree.begin(); it != tree.end(); ++it) {
    std::vector<T> point = *it;

    auto dist = std::inner_product(point.begin(), point.end(), ref_point.begin(),
                                   T(0), std::plus<T>(), [](T x, T y) {return (y-x)*(y-x);});
    if (dist < current_best_dist) {
      current_best = point;
      current_best_dist = dist;
    }
  }
  return current_best;


  /*
  std::vector<T> current_best = split_point_;

  std::vector<T> left_best;
  if (left_child_) left_best = left_child_->findNN(ref_point);

  std::vector<T> right_best;
  if (right_child_) right_best = right_child_->findNN(ref_point);
  */

}

/*

template <class T, int k>
class KDTree {
  private:
    Node<T> root;
  public:
    KDTree(typename std::vector<std::vector<T>>::iterator points_begin, typename std::vector<std::vector<T>>::iterator points_end) {
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
      root = new Node<T>(median_point, leftChild, rightChild);
    }

    Node<T>* createTree(typename std::vector<std::vector<T>>::iterator points_begin,
                        typename std::vector<std::vector<T>>::iterator points_end,
                        int depth=0) {
      d
    }
    std::vector<T> findNN(std::vector<T> ref) {
      std::vector<T> current_best;
    }

}; */


//template <class T>
//Node<T>* KDTree(typename std::vector<std::vector<T>>::iterator points_begin, typename std::vector<std::vector<T>>::iterator points_end, int depth=0) {
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
/*
  // Handle base case
  if (std::distance(points_begin, points_end) <= 0) {
    return nullptr;
  }

  // Assume all points share the same number of dimensions
  size_t k = (*points_begin).size();

  // Cycle through axes to split on
  int axis = depth % k;
*/
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
/*
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
*/

#endif