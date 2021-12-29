#pragma once

#include <vector>
#include <unordered_set>
#include <stack>
#include <queue>


namespace parent {


struct TreeNode {
  int val;
  TreeNode *left;
  TreeNode *right;
  TreeNode() : val(0), left(nullptr), right(nullptr) {}
  TreeNode(int x) : val(x), left(nullptr), right(nullptr) {}
  TreeNode(int x, TreeNode *left, TreeNode *right) : val(x), left(left), right(right) {}
};


class Solation {
 public:
  TreeNode* find(TreeNode* root, TreeNode* target, std::vector<TreeNode*>& node_list) {
    if (root == nullptr) {
      return nullptr;
    }
    auto left = find(root->left, target, node_list);
    auto right = find(root->right, target, node_list);
    if (left != nullptr || right != nullptr) {
      node_list.push_back(root);
      return root;
    } else if (root == target) {
      node_list.push_back(root);
      return root;
    } else {
      return nullptr;
    }
  }

  TreeNode* findCom(TreeNode* root, TreeNode* p1, TreeNode* p2, TreeNode* p3) {
    std::vector<TreeNode*> res1;
    find(root, p1, res1);
    std::vector<TreeNode*> res2;
    find(root, p2, res2);
    std::vector<TreeNode*> res3;
    find(root, p3, res3);
    std::unordered_set<TreeNode*> dataset(res1.begin(), res1.end());
    std::stack<TreeNode*> stack1;
    for (const auto& item : res2) {
      if (dataset.find(item) != dataset.end()) {
        stack1.push(item);
      }
    }
    std::stack<TreeNode*> stack2;
    for (const auto& item : res3) {
      if (dataset.find(item) != dataset.end()) {
        stack2.push(item);
      }
    }
    TreeNode* res = root;
    stack1.pop();
    stack2.pop();
    while (!stack1.empty() && !stack2.empty()) {
      if (stack1.top() != stack2.top()) {
        return res;
      }
      res = stack1.top();
      stack1.pop();
      stack2.pop();
    }
    return res;
  }


};

}