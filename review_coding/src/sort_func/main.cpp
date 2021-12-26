#include <iostream>
#include <vector>
#include <algorithm>
#include <bitset>
#include <unordered_set>
#include <queue>
#include <deque>


struct TreeNode {
  int val;
  TreeNode *left;
  TreeNode *right;
  TreeNode() : val(0), left(nullptr), right(nullptr) {}
  TreeNode(int x) : val(x), left(nullptr), right(nullptr) {}
  TreeNode(int x, TreeNode *left, TreeNode *right) : val(x), left(left), right(right) {}
};

struct NodeWrapper {
  TreeNode* node;
  int deep;
};

class Solution {
 public:
  std::vector<int> largestValues(TreeNode* root) {
    std::vector<int> res;
    if (root == nullptr) {
      return res;
    }
    int current_deep = -1;
    int max_size = 0;
    std::queue<NodeWrapper> qu;
    {
      NodeWrapper w;
      w.node = root;
      w.deep = 0;
      qu.push(w);
    }
    while (!qu.empty()) {
      NodeWrapper node = qu.front();
      qu.pop();
      if (current_deep != node.deep) {
        if (current_deep == -1) {
          current_deep = node.deep;
          max_size = node.node->val;
        } else {
          res.push_back(max_size);
          current_deep = node.deep;
          max_size = node.node->val;
        }
      }
      max_size = std::max(max_size, node.node->val);
      if (node.node->left != nullptr) {
        NodeWrapper w;
        w.node = node.node->left;
        w.deep = current_deep + 1;
        qu.push(w);
      }
      if (node.node->right != nullptr) {
        NodeWrapper w;
        w.node = node.node->right;
        w.deep = current_deep + 1;
        qu.push(w);
      }
    }
    return res;
  }
};


int main() {
  TreeNode* root = new TreeNode(1);
  TreeNode* left = new TreeNode(2);
  TreeNode* right = new TreeNode(3);
  root->left = left;
  root->right = right;

  Solution solution;
  auto res = solution.largestValues(root);
  for (const auto& item : res) {
    std::cout << item << " ";
  }
  std::cout << std::endl;

}
