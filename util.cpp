#include <chrono>
#include "util.h"

//mt19937_64 randEngine(0); // NOLINT(cert-err58-cpp,cert-msc51-cpp)
mt19937_64 randEngine(chrono::system_clock::now().time_since_epoch().count()); // NOLINT(cert-err58-cpp)
int startTime = 0;

int getRandInt(int end, int start) {
  std::uniform_int_distribution<int> u{start, end - 1};
  return u(randEngine);
}

void SortArr::offer(int n) {
  int t = 0;
  bool flag = false;
  _List_iterator<int> tmpIter;
  _List_iterator<int> iter;
  for (iter = mArr.begin();iter != mArr.end();iter++) {
    if (comp[n] == comp[*iter]) {
      if (getRandInt(++t) == 0) tmpIter = iter;
    } else if (comp[n] < comp[*iter]) {
      if (getRandInt(++t) == 0) tmpIter = iter;
      flag = true;
      break;
    }
  }
  if (t > 0) {
    if (!flag && getRandInt(++t) == 0) mArr.push_back(n);
    else mArr.insert(tmpIter, n);
  } else mArr.push_back(n);

  if (mArr.size() > maxLen) mArr.pop_front();
}
void SortArrTwo::offer(int n) {
  int t = 0;
  bool flag = false;
  _List_iterator<int> tmpIter;
  _List_iterator<int> iter;
  for (iter = mArr.begin();iter != mArr.end();iter++) {
    if (comp[n][ind] == comp[*iter][ind]) {
      t++;
      if (comp2[n][ind] < comp2[*iter][ind]) {
        tmpIter = iter;
        flag = true;
        break;
      } else if (getRandInt(t) == 0) tmpIter = iter;
    } else if (comp[n][ind] < comp[*iter][ind]) {
      if (getRandInt(++t) == 0) tmpIter = iter;
      flag = true;
      break;
    }
  }
  if (t > 0) {
    if (!flag && getRandInt(++t) == 0) mArr.push_back(n);
    else mArr.insert(tmpIter, n);
  } else mArr.push_back(n);

  if (mArr.size() > maxLen) mArr.pop_front();
}