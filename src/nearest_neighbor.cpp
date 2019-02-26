#include <chrono>
#include <climits>
#include <iostream>
#include <random>
#include <vector>

#include "kd_tree.h"

constexpr int RANGE_BEGIN = -10;
constexpr int RANGE_END = 10;

template <class T>
void test_tree(KDTree<T> & tree, int k) {
  /* Test our functionality for a tree
  Args:
    tree: pre-built k-d tree to search
    k: dimensionality k of kd-tree
  */

  // print out the structure of the tree
  std::cout << std::endl << "K-D Tree Pretty Print:" << std::endl;
  tree.prettyPrint();

  // test iterator
  std::cout << std::endl << "Testing iterator:" << std::endl;
  for (auto it=tree.begin(); it != tree.end(); ++it) {
    std::cout << it->str() << std::endl;
  }

  // test range-for
  std::cout << std::endl << "Testing range-for iteration:" << std::endl;
  for (const auto& point : tree) {
    std::cout << point.str() << std::endl;
  }

  // get user to enter an arbitrary point that we will search for the nearest neighbor of
  Point<int> ref_point(k);
  std::cout << std::endl << "Please enter a " << k << "-dimensional point to do a nearest neighbor search on:" << std::endl;
  for (int i=0; i < k; ++i) {
    std::cout << "Dimension " << i+1 << ": ";
    std::cin >> ref_point[i];
  }

  // repeat back to them
  std::cout << std::endl << "Searching for nearest neighbor of point " << ref_point.str() << std::endl;

  // run both nearest neighbor searches using the kd-tree
  // time how long each function takes

  // Get start time
  auto start = std::chrono::high_resolution_clock::now();

  // Call the brute-force function
  Point<int> NN = findNN_brute_force(tree, ref_point);

  // Get end time
  auto stop = std::chrono::high_resolution_clock::now();

  // Calculate duration
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

  // Print out results
  std::cout << std::endl << "Brute-forced search returned " << NN.str() << "in " << duration.count() << " microseconds" << std::endl;

  // Repeat all of that for the optimized search which prunes branches
  start = std::chrono::high_resolution_clock::now();
  NN = findNN(tree, ref_point);
  stop = std::chrono::high_resolution_clock::now();
  duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

  std::cout << std::endl << "Optimized search returned " << NN.str() << " in " << duration.count() << " microseconds" << std::endl;
}


int main() {

  // Test wikipedia example
  // https://en.wikipedia.org/wiki/K-d_tree#Nearest_neighbour_search

  std::cout << "TESTING WIKIPEDIA EXAMPLE" << std::endl << std::endl;
  KDTree<int> wikipedia_tree {{2,3}, {5,4}, {9,6}, {4,7}, {8,1}, {7,2}};

  test_tree(wikipedia_tree, 2);

  //
  // Test user-defined tree
  //
  std::cout << std::endl << "TESTING USER-INPUT TREE" << std::endl << std::endl;

  // read in N and k from user
  int k, N;
  std::cout << "How many points to generate?" << std::endl << "N: ";
  std::cin >> N;
  std::cout << "How many dimensions will each point have?" << std::endl << "k: ";
  std::cin >> k;

  // create N copies of a default point with size k
  std::vector<Point<int>> points(N, Point<int>(k));

  // make a random generator
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();  // time-based seed
  std::default_random_engine generator (seed);

  std::uniform_int_distribution<int> distribution(RANGE_BEGIN, RANGE_END);

  // fill in points with random values
  for (int i=0; i<N; ++i)
    for (int j=0; j<k; ++j)
      points[i][j] = distribution(generator);

  // print out the points
  std::cout << std::endl << "Generated points:" << std::endl;
  for (auto point : points) {
    std::cout << point.str() << std::endl;
  }

  // construct KD Tree from vector of points
  // destroys vector, as tree moves that memory into itself
  KDTree<int> user_tree(points.begin(), points.end());

  test_tree(user_tree, k);

  return 0;
}
