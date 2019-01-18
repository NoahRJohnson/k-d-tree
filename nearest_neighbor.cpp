#include <iostream>
#include <vector>
#include "kd_tree.h"

int main()
{
  std::vector<std::vector<int>> points = { {2,3}, {5,4}, {9,6}, {4,7}, {8,1}, {7,2} };

  std::cout << "Input list of points:" << std::endl;
  for (auto i=points.begin(); i!= points.end(); ++i) {
    std::cout << "( ";
    for (auto j=(*i).begin(); j != (*i).end(); ++j) {
      std::cout << *j << ' ';
    }
    std::cout << ")" << std::endl;
  }

  Node<int>* tree = KDTree<int>(std::begin(points), std::end(points));

  std::cout << std::endl << "K-D Tree:" << std::endl;
  tree->prettyPrint();

  deleteTree(tree);
  return 0;
}
