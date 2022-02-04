#include <queue>
#include <vector>
#include <list>
#include <iostream>
#include <unordered_set>
#include <algorithm>
#include <unordered_map>
#include <any>
#include <memory>

using namespace std;

struct AA{
  int a=2;
};

struct BB {
  int b = 3;
};

int main() {

  vector<shared_ptr<any>> session;
  session.push_back(make_shared<any>(AA()));
  session.push_back(make_shared<any>(BB()));
  AA* a = nullptr;
  a->a = 2;
}

