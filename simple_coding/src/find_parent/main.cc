#include <iostream>
#include <vector>

namespace subtree {

struct TreeNode {
  int val;
  TreeNode *left;
  TreeNode *right;
  TreeNode() : val(0), left(nullptr), right(nullptr) {}
  TreeNode(int x) : val(x), left(nullptr), right(nullptr) {}
  TreeNode(int x, TreeNode *left, TreeNode *right) : val(x), left(left), right(right) {}
};

/**
 * Definition for a binary tree node.
 * struct TreeNode {
 *     int val;
 *     TreeNode *left;
 *     TreeNode *right;
 *     TreeNode() : val(0), left(nullptr), right(nullptr) {}
 *     TreeNode(int x) : val(x), left(nullptr), right(nullptr) {}
 *     TreeNode(int x, TreeNode *left, TreeNode *right) : val(x), left(left), right(right) {}
 * };
 */
class Solution {
 public:
  void visit(TreeNode* root, std::vector<int>& data) {
    if (root == nullptr) {
      return;
    }
    visit(root->left, data);
    data.push_back(root->val);
    visit(root->right, data);
  }

  bool isSubtree(TreeNode* root, TreeNode* subRoot) {
    std::vector<int> total;
    visit(root, total);
    std::vector<int> sub;
    visit(subRoot, sub);
    int left = 0;
    int right = 0;
    bool begin = false;
    while (left < total.size()) {
      if (right >= sub.size()) {
        break;
      }
      if (total[left] == sub[right]) {
        ++right;
      } else {
        right = 0;
      }
      ++left;
    }
    return right >= sub.size();
  }
};

}



int main() {
  using subtree::TreeNode;
  TreeNode tree_node1{1};
  TreeNode tree_node2{2};
  TreeNode tree_node3{3};
  TreeNode tree_node4{4};
  TreeNode tree_node5{5};
  TreeNode tree_node6{6};
  tree_node3.left = &tree_node4;
  tree_node3.right = &tree_node5;
  tree_node4.left = &tree_node1;
  tree_node4.right = &tree_node2;

  subtree::Solution solution;
  std::cout << std::boolalpha;
  std::cout << solution.isSubtree(&tree_node3, &tree_node4);
}
