#ifndef _KDTREE_H_
#define _KDTREE_H_

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <vector>

template <class T>
class Point {
  private:
    std::size_t k_;
    T* data_;
  public:
    Point(): k_{0}, data_{nullptr} {}  // default constructor (only needed for member variable init)

    explicit Point(std::size_t num_dims) {  // no conversion from int
      if (num_dims < 0)
        throw std::length_error("Negative point dimensions");

      k_ = num_dims;
      data_ = k_ ? new T[k_] : nullptr;

      // init data to all zeroes
      for (int i=0; i<k_; ++i) {
        data_[i] = 0;
      }
    }
    Point(std::initializer_list<T> input)  // copy from {...} initializer list
      : k_{input.size()},
        data_{k_ ? new T[k_] : nullptr}
    {
      // copy data_
      int i=0;
      auto it = input.begin(), end = input.end();
      while (it != end) {
        data_[i++] = *it++;  // cursed
      }
    }

    Point(std::vector<T> &input) // copy from vector
      : k_{input.size()},
        data_{k_ ? new T[k_] : nullptr}
    {
      // copy data_
      for (int i=0; i < k_; ++i) {
        data_[i] = input[i];
      }
    }

    Point(const Point<T>& rhs) // copy constructor
      : k_{rhs.k_},
        data_{k_ ? new T[k_] : nullptr}
    {
      // copy data_
      for (int i=0; i < k_; ++i) {
        data_[i] = rhs.data_[i];
      }
    }

    Point(Point<T>&& rhs) // move constructor
      : k_{rhs.k_},
        data_{rhs.data_}  // steal rhs resources
    {
      // mangle rhs
      rhs.data_ = nullptr;
      rhs.k_ = 0;
    }

    // https://stackoverflow.com/questions/3279543/what-is-the-copy-and-swap-idiom
    friend void swap(Point<T>& first, Point<T>& second) // nothrow
    {
      // enable ADL
      using std::swap;

      // by swapping the members of two objects,
      // the two objects are effectively swapped
      swap(first.k_, second.k_);
      swap(first.data_, second.data_);
    }

    Point& operator=(Point<T> rhs) // copy assignment / move assignment
    {
      swap(*this, rhs);

      return *this;
    }

    ~Point() {
      delete[] data_;
    }

    T distance_to(Point<T> const& other) const {
      // squared euclidean distance

      T total = 0;
      for (int i=0; i < k_; ++i) {
        total += (data_[i] - other[i]) * (data_[i] - other[i]);
      }
      return total;
    }

    std::string str() const {
      // returns the string representation of this point
      std::ostringstream buffer;
      buffer << '(';
      for (int i=0; i < k_; ++i) {
        buffer << data_[i] << ", ";
      }
      buffer.seekp(-2, std::ios_base::end);  // move write head back by 2 characters, to overwrite last comma
      buffer << ')';
      return buffer.str();
    }

    T& operator[] (int i) {  // [] index non-const operator
      if (i < 0 || i >= k_)
        throw std::out_of_range("Point::operator[]");
      else
        return data_[i];
    }
    T& operator[] (int i) const {  // [] index const operator
      if (i < 0 || i >= k_)
        throw std::out_of_range("Point::operator[]");
      else
        return data_[i];
    }
    bool operator== (const Point<T> &rhs) const {  // == operator
      if (k_ != rhs.k_)
        throw std::logic_error("Trying to compare two points of different size.");

      bool all_equal = true;
      for (int i=0; i<k_; ++i)
        all_equal &= data_[i] == rhs.data_[i];

      return all_equal;
    }
    bool operator!= (const Point<T> &b) const { return !(*this == b); }  // define != in terms of ==

    int size() const {return k_;}
};


template <class T>
class KDTree {

  /*
  A tree and a node are one and the same in this implementation.
  */

  private:
    KDTree<T>* left_child_;
    KDTree<T>* right_child_;
    KDTree<T>* parent_;
    int split_axis_;
    int k_;

  public:

    Point<T> split_point;

    template <class Iterator>
    KDTree(Iterator points_begin,
           Iterator points_end,
           int depth=0) {
//    KDTree(typename std::vector<Point<T>>::iterator points_begin,
//           typename std::vector<Point<T>>::iterator points_end,
//           int depth=0) {
      /*
      Creates a balanced tree from points.

      Takes ownership of underlying data from vector.
      Assumes all Points have the same dimensionality k.

      Args:
        points_begin: begin iterator of container of Points to move into tree
        points_end: end iterator of container of Points to move into tree
        depth: depth or height of this tree node compared to the root
      */

      // Special case for root
      if (depth == 0) {
        parent_ = nullptr;
      }

      // Assume all points have the same dimensionality
      k_ = points_begin->size();  // size of first Point object

      // Cycle through axes to split on
      int axis = depth % k_;  // temp variable for lambda function
      split_axis_ = axis;

      // Sort points along this axis using a lambda comparator
      std::sort(points_begin, points_end, [axis](const Point<T>& a, const Point<T>& b) {
                return a[axis] < b[axis];
                });

      // select median point along this axis
      auto median_it = std::next(points_begin, std::distance(points_begin, points_end) / 2);

      // split on that median point, and take ownership of this Point element in the vector
      split_point = std::move(*(median_it));

      // Recurse on sub-array of points to the left of median point
      if (std::distance(points_begin, median_it) <= 0) {
        left_child_ = nullptr;
      } else {
        left_child_ = new KDTree<T>(points_begin, median_it, depth + 1);
        left_child_->parent_ = this;
      }

      // Recurse on sub-array of points to the right of median point
      if (std::distance(std::next(median_it), points_end) <= 0) {
        right_child_ = nullptr;
      } else {
        right_child_ = new KDTree<T>(std::next(median_it), points_end, depth + 1);
        right_child_->parent_ = this;
      }
    }

    KDTree(std::initializer_list<Point<T>> points) { // constructor from {...} initializer list

      // copy data from possibly static list
      std::vector<Point<T>> points_vec = points;

      // call main constructor, which will move the data from the vector into the tree
      new (this) KDTree(points_vec.begin(), points_vec.end());
    }


    ~KDTree() {  // destructor
      // recursively delete children
      delete left_child_;
      delete right_child_;
    }

    void const prettyPrint(int indent=0) {
      // https://stackoverflow.com/a/26699993
      // Prints out tree in ASCII art sideways left-to-right

      // Recurse on right child
      if (right_child_) right_child_->prettyPrint(indent+4);

      // Format
      if (indent) {
        std::cout << std::setw(indent) << ' ';
      }
      if (right_child_) std::cout << " /\n" << std::setw(indent) << ' ';

      // Print split point
      std::cout << split_point.str() << "\n ";

      // Format and recurse on left child
      if (left_child_) {
        std::cout << std::setw(indent) << ' ' << " \\\n";
        left_child_->prettyPrint(indent+4);
      }
    }

    // http://www.drdobbs.com/the-standard-librarian-defining-iterato/184401331?pgno=3
    template <bool flag, class IsTrue, class IsFalse>
    struct choose;

    template <class IsTrue, class IsFalse>
    struct choose<true, IsTrue, IsFalse> {
      typedef IsTrue type;
    };

    template <class IsTrue, class IsFalse>
    struct choose<false, IsTrue, IsFalse> {
      typedef IsFalse type;
    };

    template <bool const_flag = false>
    struct kdtree_iterator {
      // http://www.drdobbs.com/the-standard-librarian-defining-iterato/184401331

      typedef std::forward_iterator_tag iterator_category;  // category type
      typedef Point<T> value_type;  // the type of the object pointed to
      typedef std::ptrdiff_t difference_type;  // type for the distance between two elements of a sequence
      typedef typename choose<const_flag, const Point<T>&, Point<T>&>::type reference;  // reference to the kdtree_iterator's value type (using compile-time ternary)
      typedef typename choose<const_flag, const Point<T>*, Point<T>*>::type pointer;  // pointer to the kdtree_iterator's value type (using compile-time ternary)
      typedef typename choose<const_flag, const KDTree<T>*, KDTree<T>*>::type nodeptr;

      kdtree_iterator(): p(nullptr) {} // empty constructor

      kdtree_iterator(nodeptr node) { // constructor which takes a KD-Tree pointer to root

        // Find the deepest, leftmost leaf node
        while (node->left_child_ || node->right_child_) {
          if (node->left_child_) {
            node = node->left_child_;
          } else {
            node = node->right_child_;
          }
        }

        // Set that as the initial state
        p = node;
      }

      kdtree_iterator(const kdtree_iterator<false>& other)  // copy constructor (if this kdtree_iterator is non-const) or converting constructor
        : p(other.p) { }

      reference operator* () const { return p->split_point; }  // * operator
      pointer operator -> () const { return &(p->split_point); }  // -> operator

      kdtree_iterator& operator++ () {  // prefix ++ operator

        // go to the right child and find the leftmost child node
        if (p->right_child_) {
          p = p->right_child_;
          while (p->left_child_) {
            p = p->left_child_;
          }
        }
        else {  // if there is no right child, go up until one exists (that isn't us) or we hit the null parent above root
          KDTree<T>* parent = p->parent_;
          while (parent && parent->right_child_ == p) {
            p = parent;
            parent = parent->parent();
          }
          if (!parent || p->right_child_ != parent) {
            p = parent;
          }
        }
        return *this;  // return reference to this kdtree_iterator
      }

      kdtree_iterator operator++ (int) { // postfix ++
        kdtree_iterator tmp(*this);
        ++(*this);
        return tmp;
      }

      friend bool operator== (const kdtree_iterator& a, const kdtree_iterator& b) {
        return a.p == b.p;
      }
      friend bool operator!= (const kdtree_iterator& a, const kdtree_iterator& b) {
        return a.p != b.p;
      }

      nodeptr p;
    };

    // typedefs for KDTree class
    typedef kdtree_iterator<false> iterator;
    typedef kdtree_iterator<true> const_iterator;

    iterator begin() {
      return iterator(this);
    }
    const_iterator begin() const {
      return const_iterator(this);
    }
    iterator end() {
      return iterator();
    }
    const_iterator end() const {
      return const_iterator();
    }

    // Getters
    KDTree<T>* left_child() const {
      return left_child_;
    }
    KDTree<T>* right_child() const {
      return right_child_;
    }
    KDTree<T>* parent() const {
      return parent_;
    }
    int split_axis() const {
      return split_axis_;
    }
    int k() const {
      return k_;
    }

};

template <class T>
Point<T> findNN_brute_force(const KDTree<T> & tree, const Point<T> & ref_point) {
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
Point<T> findNN(const KDTree<T> & tree, const Point<T> & ref_point) {
  /* find nearest neighbor
  wrapper for recursive search with pruning
  copies the final point to return to user

  Args:
    tree: k-d tree to search
    ref_point: the reference point for which we seek to find the nearest neighbor

  Returns:
    A copy of the Point object from the tree node which was the nearest neighbor
  */
  const KDTree<T> * tree_ptr = &tree;  // pointer to constant KDTree object
  const Point<T> * best_point = &tree.split_point;  // pointer to root Point
  T best_dist = std::numeric_limits<T>::max();  // set initial distance to max. - assumes distance between points is of type T

  findNN_(tree_ptr, ref_point, &best_point, &best_dist);  // recursively find best_point

  return Point<T>(*best_point);  // create a copy of the Point object selected
}

template <class T>
void findNN_(const KDTree<T> * tree_node, const Point<T> & ref_point,
             const Point<T> ** best_point, T * best_dist) {
  /* find nearest neighbor
  recursive search with pruning
  returns point by reference

  Following algorithm from: https://en.wikipedia.org/wiki/K-d_tree_node#Nearest_neighbour_search

  Args:
    tree_node: k-d tree_node to search
    ref_point: the reference point for which we seek to find the nearest neighbor
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
  KDTree<T>* closer_child = (d > 0) ? tree_node->right_child() : tree_node->left_child();
  KDTree<T>* farther_child = (d > 0) ? tree_node->left_child() : tree_node->right_child();

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

#endif