#ifndef KD_TEMPLATE_SRC_KD_TREE_H_
#define KD_TEMPLATE_SRC_KD_TREE_H_

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace kd {

template <class T>
class Point {
  private:

    /* !!!! Careful !!!!
     * The order of these class members determines the initialization
     * order within constructor member initialization lists */
    std::size_t k_;
    T* data_;

  public:
    Point(): k_{0}, data_{nullptr} {}  // default constructor (only needed for member variable init)

    explicit Point(std::size_t num_dims) {  // no conversion from int
      if (num_dims < 0)
        throw std::length_error("Negative point dimensions");

      k_ = num_dims;
      data_ = new T[k_];

      // init data to all zeroes
      std::memset(data_, 0, k_ * sizeof(T));
    }

    Point(std::initializer_list<T> input)  // copy from {...} initializer list
      : k_{input.size()},
        data_{new T[k_]}
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
        data_{new T[k_]}
    {
      // copy data_
      for (std::size_t  i=0; i < k_; ++i) {
        data_[i] = input[i];
      }
    }

    template <class Iterator>
    Point(Iterator begin, Iterator end) // copy from iterator
    {
      k_ = std::distance(begin, end);
      data_ = new T[k_];

      // copy data_
      std::copy(begin, end, data_);
    }

    Point(const Point<T>& rhs) // copy constructor
      : k_{rhs.k_},
        data_{new T[k_]}
    {
      // deep copy data_
      for (std::size_t i=0; i < k_; ++i) {
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

    Point& operator=(const Point<T>& rhs) // copy assignment
    {
      if (k_ != rhs.k_) {  // Only re-size array if needed
        delete [] data_;

        data_ = nullptr;  // clear this...
        k_ = 0u;          // ...and this in case the next line throws

        data_ = new T[rhs.k_];
        k_ = rhs.k_;
      }

      std::copy(rhs.data_, rhs.data_ + k_, data_);  // deep copy data
      return *this;
    }

    Point& operator=(Point<T>&& rhs) // move assignment
    {
      // Delete old resource
      delete [] data_;

      // Steal resources from rhs
      data_ = rhs.data_;
      k_ = rhs.k_;

      // Reset rhs
      rhs.data_ = nullptr;
      rhs.k_ = 0u;

      return *this;
    }

    ~Point() {
      delete [] data_;
    }

    T distance_to(Point<T> const& other) const {
      // squared euclidean distance

      T total = 0;
      for (std::size_t  i=0; i < k_; ++i) {
        total += (data_[i] - other[i]) * (data_[i] - other[i]);
      }
      return total;
    }


    template <class U>
    friend std::ostream& operator<< (std::ostream& stream, const Point<U>& point);

    T& operator[] (std::size_t i) const {  // [] index operator -- const / non-const objects both use this
      if (i < 0 || i >= k_)
        throw std::out_of_range("Point::operator[]");
      else
        return data_[i];
    }

    bool operator== (const Point<T> &rhs) const {  // == operator
      if (k_ != rhs.k_)
        throw std::logic_error("Trying to compare two points of different size.");

      bool all_equal = true;
      for (std::size_t  i=0; i<k_; ++i)
        all_equal &= data_[i] == rhs.data_[i];

      return all_equal;
    }

    bool operator!= (const Point<T> &b) const { return !(*this == b); }  // define != in terms of ==

    std::size_t size() const {return k_;}
};

template <class T>
std::ostream& operator<< (std::ostream& stream, const Point<T>& point) {
  // outputs the string representation of this point

  // generate the string representation of this point
  std::ostringstream buffer;
  buffer << '(';
  for (std::size_t  i=0; i < point.k_; ++i) {
    buffer << point.data_[i] << ", ";
  }
  buffer.seekp(-2, std::ios_base::end);  // move write head back by 2 characters, to overwrite last comma
  buffer << ')';

  // write out string to stream
  stream << buffer.str();

  // Return stream reference to allow chaining
  return stream;
}

template <class T>
class Tree {

  /*
  A tree and a node are one and the same in this implementation.
  */

  private:
    Tree<T>* left_child_;
    Tree<T>* right_child_;
    Tree<T>* parent_;
    int split_axis_;
    int k_;

  public:

    Point<T> split_point;

    Tree() {}  // default constructor

    template <class Container>
    Tree(Container&& input, int depth=0) {
      /*
      Creates a balanced tree from points.

      Takes ownership of underlying data from vector.
      Assumes all Points have the same dimensionality k.

      Args:
        input: Container of Points to move into tree
        depth: depth or height of this tree node compared to the root
      */
      this->RecursiveInit(std::begin(input), std::end(input), depth);
    }

    Tree(std::initializer_list<Point<T>> points)
    {
      // initializer lists are read-only (iterators of type const Point<T>*)
      // therefore we try to move over to another container, before we can
      // move into the tree
      // however, initializer_lists can be static, in which case the data will be copied not moved
      // no helping that
      std::vector<Point<T>> points_vec = std::move(points);
      this->RecursiveInit(points_vec.begin(), points_vec.end(), 0);
    }

    template <class Iterator>
    void RecursiveInit(Iterator points_begin,
           Iterator points_end,
           int depth=0) {
      /*
      Recursively creates a balanced tree from points.

      Takes ownership of underlying data being iterated over, i.e.
      constructing a tree moves all the points into that tree.

      Assumes all Points have the same dimensionality k.

      Args:
        points_begin: begin iterator of container of Points to move into tree
        points_end: end iterator of container of Points to move into tree
        depth: depth or height of this tree node compared to the root
      */

      // Special case for root
      if (depth == 0) {
        parent_ = nullptr;

        // Validate k
        int one_common_k = -1;
        for (auto it=points_begin, end=points_end; it != end; ++it) {
          int k = it->size();
          if (one_common_k == -1) {
            one_common_k = k;
          } else {
            assert(k == one_common_k);
          }
        }
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
        left_child_ = new Tree<T>();
        left_child_->RecursiveInit(points_begin, median_it, depth + 1);
        left_child_->parent_ = this;
      }

      // Recurse on sub-array of points to the right of median point
      if (std::distance(std::next(median_it), points_end) <= 0) {
        right_child_ = nullptr;
      } else {
        right_child_ = new Tree<T>();
        right_child_->RecursiveInit(std::next(median_it), points_end, depth + 1);
        right_child_->parent_ = this;
      }
    }

    ~Tree() {  // destructor
      // recursively delete children
      delete left_child_;
      delete right_child_;
    }

    template <class U>
    friend std::ostream& operator<< (std::ostream& stream, const Tree<U>& tree);

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
    struct TreeIterator {
      // http://www.drdobbs.com/the-standard-librarian-defining-iterato/184401331

      typedef std::forward_iterator_tag iterator_category;  // category type
      typedef Point<T> value_type;  // the type of the object pointed to
      typedef std::ptrdiff_t difference_type;  // type for the distance between two elements of a sequence
      typedef typename choose<const_flag, const Point<T>&, Point<T>&>::type reference;  // reference to the TreeIterator's value type (using compile-time ternary)
      typedef typename choose<const_flag, const Point<T>*, Point<T>*>::type pointer;  // pointer to the TreeIterator's value type (using compile-time ternary)
      typedef typename choose<const_flag, const Tree<T>*, Tree<T>*>::type nodeptr;

      TreeIterator(): p(nullptr) {} // empty constructor

      TreeIterator(nodeptr node) { // constructor which takes a KD-Tree pointer to root

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

      TreeIterator(const TreeIterator<false>& other)  // copy constructor (if this TreeIterator is non-const) or converting constructor
        : p(other.p) { }

      reference operator* () const { return p->split_point; }  // * operator
      pointer operator -> () const { return &(p->split_point); }  // -> operator

      TreeIterator& operator++ () {  // prefix ++ operator

        // go to the right child and find the leftmost child node
        if (p->right_child_) {
          p = p->right_child_;
          while (p->left_child_) {
            p = p->left_child_;
          }
        }
        else {  // if there is no right child, go up until one exists (that isn't us) or we hit the null parent above root
          Tree<T>* parent = p->parent_;
          while (parent && parent->right_child_ == p) {
            p = parent;
            parent = parent->parent();
          }
          if (!parent || p->right_child_ != parent) {
            p = parent;
          }
        }
        return *this;  // return reference to this TreeIterator
      }

      TreeIterator operator++ (int) { // postfix ++
        TreeIterator tmp(*this);
        ++(*this);
        return tmp;
      }

      friend bool operator== (const TreeIterator& a, const TreeIterator& b) {
        return a.p == b.p;
      }
      friend bool operator!= (const TreeIterator& a, const TreeIterator& b) {
        return a.p != b.p;
      }

      nodeptr p;
    };

    // typedefs for Tree iterator
    typedef TreeIterator<false> iterator;
    typedef TreeIterator<true> const_iterator;

    iterator begin() {  // iterator from non-const Tree
      return iterator(this);
    }
    const_iterator cbegin() {  // const iterator from non-const Tree
      return const_iterator(this);
    }
    const_iterator begin() const {  // const iterator from const Tree
      return const_iterator(this);
    }
    iterator end() {  // iterator from non-const Tree
      return iterator();
    }
    const_iterator cend() {  // const iterator from non-const Tree
      return const_iterator();
    }
    const_iterator end() const {  // const iterator from const Tree
      return const_iterator();
    }

    // Getters
    Tree<T>* left_child() const {
      return left_child_;
    }
    Tree<T>* right_child() const {
      return right_child_;
    }
    Tree<T>* parent() const {
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
std::ostream& operator<< (std::ostream& stream, const Tree<T>& tree) {
  // https://stackoverflow.com/a/26699993
  // Prints out tree in ASCII art sideways left-to-right

  // Recurse on right child
  if (tree.right_child_) stream << *tree.right_child_;

  // Format
  size_t depth = 0;
  Tree<T>* parent = tree.parent_;
  while (parent != nullptr) {
    depth += 1;
    parent = parent->parent_;
  }
  size_t indent = depth*4;

  stream << std::setw(indent) << ' ';

  if (tree.right_child_) stream << " /\n" << std::setw(indent) << ' ';

  // Print split point
  stream << tree.split_point << "\n ";

  // Format and recurse on left child
  if (tree.left_child_) {
    stream << std::setw(indent) << ' ' << " \\\n";
    stream << *tree.left_child_;
  }

  // Return stream reference to allow chaining
  return stream;
}

}  // ends namespace

#endif  // KD_TEMPLATE_SRC_KD_TREE_H_
