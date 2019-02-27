#ifndef KD_TEMPLATE_SRC_POINT_H_
#define KD_TEMPLATE_SRC_POINT_H_

#include <cstring>
#include <iomanip>
#include <vector>

namespace kd {

template <class T>
class Point {
  private:

    /* !!!! Careful !!!!
     * The order of these class members determines the initialization
     * order within constructor member initialization lists */
    std::size_t dims_;
    T* data_;

  public:
    Point(): dims_{0}, data_{nullptr} {}  // default constructor (only needed for member variable init)

    explicit Point(std::size_t num_dims) {  // no conversion from int
      if (num_dims < 0)
        throw std::length_error("Negative point dimensions");

      dims_ = num_dims;
      data_ = new T[dims_];

      // init data to all zeroes
      std::memset(data_, 0, dims_ * sizeof(T));
    }

    Point(std::initializer_list<T> input)  // copy from {...} initializer list
      : dims_{input.size()},
        data_{new T[dims_]}
    {
      // copy data_
      int i=0;
      auto it = input.begin(), end = input.end();
      while (it != end) {
        data_[i++] = *it++;  // cursed
      }
    }

    Point(std::vector<T> &input) // copy from vector
      : dims_{input.size()},
        data_{new T[dims_]}
    {
      // copy data_
      for (std::size_t  i=0; i < dims_; ++i) {
        data_[i] = input[i];
      }
    }

    template <class Iterator>
    Point(Iterator begin, Iterator end) // copy from iterator
    {
      dims_ = std::distance(begin, end);
      data_ = new T[dims_];

      // copy data_
      std::copy(begin, end, data_);
    }

    Point(const Point<T>& rhs) // copy constructor
      : dims_{rhs.dims_},
        data_{new T[dims_]}
    {
      // deep copy data_
      for (std::size_t i=0; i < dims_; ++i) {
        data_[i] = rhs.data_[i];
      }
    }

    Point(Point<T>&& rhs) // move constructor
      : dims_{rhs.dims_},
        data_{rhs.data_}  // steal rhs resources
    {
      // mangle rhs
      rhs.data_ = nullptr;
      rhs.dims_ = 0;
    }

    Point& operator=(const Point<T>& rhs) // copy assignment
    {
      if (dims_ != rhs.dims_) {  // Only re-size array if needed
        delete [] data_;

        data_ = nullptr;  // clear this...
        dims_ = 0u;       // ...and this in case the next line throws

        data_ = new T[rhs.dims_];
        dims_ = rhs.dims_;
      }

      std::copy(rhs.data_, rhs.data_ + dims_, data_);  // deep copy data
      return *this;
    }

    Point& operator=(Point<T>&& rhs) // move assignment
    {
      // Delete old resource
      delete [] data_;

      // Steal resources from rhs
      data_ = rhs.data_;
      dims_ = rhs.dims_;

      // Reset rhs
      rhs.data_ = nullptr;
      rhs.dims_ = 0u;

      return *this;
    }

    ~Point() {
      delete [] data_;
    }

    T distance_to(Point<T> const& other) const {
      // squared euclidean distance

      T total = 0;
      for (std::size_t  i=0; i < dims_; ++i) {
        total += (data_[i] - other[i]) * (data_[i] - other[i]);
      }
      return total;
    }


    template <class U>
    friend std::ostream& operator<< (std::ostream& stream, const Point<U>& point);

    T& operator[] (std::size_t i) const {  // [] index operator -- const / non-const objects both use this
      if (i < 0 || i >= dims_)
        throw std::out_of_range("Point::operator[]");
      else
        return data_[i];
    }

    bool operator== (const Point<T> &rhs) const {  // == operator
      if (dims_ != rhs.dims_)
        throw std::logic_error("Trying to compare two points of different size.");

      bool all_equal = true;
      for (std::size_t  i=0; i<dims_; ++i)
        all_equal &= data_[i] == rhs.data_[i];

      return all_equal;
    }

    bool operator!= (const Point<T> &b) const { return !(*this == b); }  // define != in terms of ==

    std::size_t size() const {return dims_;}
};

template <class T>
std::ostream& operator<< (std::ostream& stream, const Point<T>& point) {
  // outputs the string representation of this point

  // generate the string representation of this point
  std::ostringstream buffer;
  buffer << '(';
  for (std::size_t  i=0; i < point.dims_; ++i) {
    buffer << point.data_[i] << ", ";
  }
  buffer.seekp(-2, std::ios_base::end);  // move write head back by 2 characters, to overwrite last comma
  buffer << ')';

  // write out string to stream
  stream << buffer.str();

  // Return stream reference to allow chaining
  return stream;
}

}  // namespace kd

#endif  // KD_TEMPLATE_SRC_POINT_H_
