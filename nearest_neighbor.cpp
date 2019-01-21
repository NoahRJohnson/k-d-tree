#include <iostream>
#include <vector>
#include "kd_tree.h"

int main()
{
  // use vector of vectors to represent a dynamic-sized array of dynamic-sized points
  std::vector<std::vector<int>> points = { {2,3}, {5,4}, {9,6}, {4,7}, {8,1}, {7,2} };

  // print out the points
  std::cout << "Input list of points:" << std::endl;
  for (auto i=points.begin(); i!= points.end(); ++i) {
    std::cout << "( ";
    for (auto j=(*i).begin(); j != (*i).end(); ++j) {
      std::cout << *j << ' ';
    }
    std::cout << ")" << std::endl;
  }

  // construct KD Tree from points
  KDTree<int, 2> tree = KDTree<int,2>(std::begin(points), std::end(points));

  // print out the structure of the tree
  std::cout << std::endl << "K-D Tree:" << std::endl;
  tree.prettyPrint();

  // arbitrary point that we will search for nearest neighbors of
  int x,y;
  std::cout << "Please enter a point to do a nearest neighbor search on:" << std::endl << "x: ";
  std::cin >> x;
  std::cout << "y: ";
  std::cin >> y;

  std::vector<int> ref_point {x,y};

  // run a nearest neighbor search using the kd-tree
  std::vector<int> NN = findNN_brute_force(tree, ref_point);

  std::cout << std::endl << "Nearest neighbor to reference point (";
  for (auto i : ref_point) {
    std::cout << i << ' ';
  }
  std::cout << ") is (";
  for (auto i : NN) {
    std::cout << i << ' ';
  }
  std::cout << ")" << std::endl;

  // test deletor
  //delete &tree;

  return 0;
}
