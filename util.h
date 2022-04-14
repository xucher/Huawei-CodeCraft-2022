#ifndef HWCOMPETITION_UTIL_H
#define HWCOMPETITION_UTIL_H

#include <random>
#include <list>
#include <vector>
#include <iostream>

using namespace std;
extern mt19937_64 randEngine;
extern int startTime;
int getRandInt(int end, int start = 0);

// 链表内数据从小到大排序, 用于 cons_phase1_greed
class SortArr {
private:
  const int *comp;
protected:
  list<int> mArr;
  int maxLen = 0;
public:
  explicit SortArr(int _maxLen, const int* _comp = nullptr):
      maxLen(_maxLen), comp(_comp) {}
  void clear() { mArr.clear();};
  virtual void offer(int n);
  int pop() {
    int m = mArr.back();
    mArr.pop_back();
    return m;
  }
  int randChoose() {
    int m = getRandInt(mArr.size());
    _List_iterator<int> iter;
    for (iter = mArr.begin();iter != mArr.end();iter++) {
      if (m-- == 0) break;
    }
    m = *iter;
    mArr.erase(iter);
    return m;
  }

  void print() const {
    for (auto &num: mArr) {
      cout << num << " ";
    }
    cout << endl;
  }
};

// 用于计算 quantile，N5节点尽量往前排
class SortArrTwo: public SortArr {
private:
  int **comp = nullptr;
  int **comp2 = nullptr;
  int ind = -1;
public:
  SortArrTwo(int _maxLen, int** _comp, int** _comp2): SortArr(_maxLen), comp(_comp), comp2(_comp2) {};
  void reset(int _ind) {
    mArr.clear();
    ind = _ind;
  };
  void offer(int n) override;
  int front() { return mArr.front(); };
};

class SiteGroup {
public:
  // 集和中节点个数
  int sn;
  // 包含的每个节点的最大容量，从大到小排序
  vector<int> cap;
  // 可分配最大容量
  int maxCap = 0;
  // 节点组可以服务的客户, 按可分配节点数从小到大排序
  list<int> cus;

  explicit SiteGroup(int sn): sn(sn), cap(sn) {};
};


#endif //HWCOMPETITION_UTIL_H
