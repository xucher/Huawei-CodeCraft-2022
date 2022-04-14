#ifndef HWCOMPETITION_LOCALSEARCH_H
#define HWCOMPETITION_LOCALSEARCH_H

/**
 * 0 服务器版本
 * 1 本地版本，只输出最优解和用时
 * 2 输出构造，搜索，adjust各阶段的解
 * 3 开启 check
 * 4 每隔一段时间输出当前解
 */
#define DEBUG 0

#include <iostream>
#include <iomanip>
#include <list>
#include <random>
#include <vector>
#include "io.h"
#include "maxflow.h"
#include "util.h"
#include "threadPool.h"
#include <climits>
#include <cmath>

class LocalSearch {
private:
  const Input &data;
  ThreadPool* pool;
  // 边缘节点的带宽序列中后 5% 的个数
  const int N5;
  const int threadN = 3;

  // 无效节点个数，
  int invalidSiteN = -1;
  // [site][index] cus 节点的邻接矩阵，最后一位保存节点可服务的客户节点个数
  int** siteAdj;
  // 边缘节点按可服务客户个数从小到大排序后顺序
  int *sp;
  // 边缘节点分组起始序号，最后一位保存分组数
  int *siteGroupMap;

  // [time][site] 每个时刻节点剩余容量
  int **siteLCap;
  // [time][site] 该时刻节点需求是否被选为排序在后5%
  // 无效节点无 N5时刻
  // -1不是N5时刻，但是排序是N5；0表示不是N5时刻，
  // 1表示是N5时刻；2表示被分配N5时刻，但是排序不是N5
  int **isN5;
  // [time][2] 每个时刻除 N5 点外剩余总流量以及剩余有效节点个数
  int **N5LeftD;
  // [time][site] 每个时刻节点接收总需求
  int **siteLoad;

  // [site][order] time 每个节点排序在后 order 的时刻，降序排列
  // 长度为 qtLen + 1，方便调整
  int** quantile;
  const int qtLen;

  // 残量图：前 cusN 个点为客户点，后 siteN 个点为边缘节点
  MaxFlow mf;

  // 公用变量
  int *exceedLoad;

public:
  int f = -1;
  // [time][cus][site]
  int ***solu;
  int bestF = INT_MAX;
  int*** bSolu;

private:
  // 分配每个边缘节点序列后 5% 的时刻
  void cons_phase1(int **cusLDemand);
  void cons_phase1_greed(int **cusLDemand);
  // t 时刻；path 路径；d 路径上的流量，起点必须是客户
  void updateSoluByPath(int t, list<int> &path, int d, int type = 1);
  void repair(int t, int **cusLDemand);
  void updateSoluByPathR(int t, list<int> &path, int d);
  // 给定 N5节点，均化剩余节点, 处理均值余数
  void balanceByTimeEnhanced(int t);
  // 更新残量图中边的权重
  void updateMF(int t);
  // 分配剩余客户需求
  void cons_phase2(int **cusLDemand);
  void construct();

  void saveBestSolu();
  void recoverBestSolu();

  void initF();
  void updateF(int t1, int t2);
  void updateQuantile(int** _quantile);
  void updateQuantile(int t1, int t2);

  // 改变 N5节点后最大化分配到 N5节点的流量
  void maxN5SiteD(int time, int site);

  void searchParallel(int endTime);
  void adjust();

  void loadState(LocalSearch* ls, bool isFirst = true);
  void checkRunnerState(int n);
public:
  explicit LocalSearch(const Input &data);
  LocalSearch(const Input &data, int tN5, int _qtLen);
  ~LocalSearch();

  void reAssignN5(int site, int t1, int t2);
  void updateState(LocalSearch* ls, int site, int t1, int t2, int t3);

  void run();

  void printSiteAdj() const {
    cout << "adjacent list for site: " << endl;
    for (int i = 0;i < data.siteN;i++) {
      cout << sp[i] << ": ";
      for (int j = 0;j < siteAdj[sp[i]][data.cusN];j++) {
        cout << siteAdj[sp[i]][j] << " ";
      }
      cout << endl;
    }
  }

  void printIsN5() const {
    cout << "isN5" << endl;
    int m = 0;

    for (int i = 0;i < data.siteN;i++) {
      if (siteAdj[i][data.cusN] == 0) {
        cout << "0" << " ";
      } else cout << "1" << " ";
    }
    cout << endl;
    for (int i = 0;i < data.timeN;i++) {
      for (int j = 0;j < data.siteN;j++) {
        cout << isN5[i][j] << " ";
        m += isN5[i][j] > 0;
      }
      cout << "--> " << m << endl;
    }
  }

  void checkIsN5(int** siteN5T) const {
    int m;
    for (int i = 0;i < data.timeN;i++) {
      m = 0;
      for (int j = 0;j < data.siteN;j++) {
        if (siteAdj[j][data.cusN] == 0) {
          if (isN5[i][j] > 0) {
            cout << "isN5 for invalid site should be 0" << endl;
            return;
          }
        } else {
          m += isN5[i][j] > 0;
        }
      }
      if (m + invalidSiteN + N5LeftD[i][1] != data.siteN) {
        cout << m << ", " << N5LeftD[i][1] << ";";
        cout << "Conflict between isN5 and N5LeftD" << endl;
        return;
      }
    }

    for (int i = 0;i < data.siteN;i++) {
      if (siteAdj[i][data.cusN] > 0) {
        for (int j = 0;j < N5;j++) {
          m = siteN5T[i][j];
          if (isN5[m][i] <= 0) {
            cout << "Conflict between isN5 and siteN5T" << endl;
            return;
          }
        }
      }
    }
  };

  void checkN5LeftD() {
    auto timeN = data.timeN;
    auto siteN = data.siteN;
    int m, n;
    for (int i = 0;i < timeN;i++) {
      m = data.demands[i][data.cusN];
      n = siteN - invalidSiteN;
      for (int j = 0;j < siteN;j++) {
        if (isN5[i][j] > 0) {
          m -= siteLoad[i][j];
          n--;
        }
      }
      if (m != N5LeftD[i][0]) {
        cout << "error in update N5LeftD[0]: " << i << ": ";
        cout << m << ", " << N5LeftD[i][0] << endl;
        return;
      }
      if (n != N5LeftD[i][1]) {
        cout << "error in update N5LeftD[1]" << endl;
        return;
      }
    }
  }

  void checkSiteLoad() {
    auto timeN = data.timeN;
    auto siteN = data.siteN;
    auto cusN = data.cusN;
    int m;

    for (int i = 0;i < timeN;i++) {
      for (int j = 0;j < siteN;j++) {
        m = 0;
        for (int k = 0;k < cusN;k++) {
          m += solu[i][k][j];
        }

        if (siteLoad[i][j] != m) {
          cout << "error in update siteLoad" << endl;
          return;
        }
        if (siteLCap[i][j] != data.siteBW[j] - m) {
          cout << "error in update siteLCap" << endl;
          return;
        }
      }
    }
  }

  /**
   * 节点不是 N5且为分位点 *
   * 节点N5节点 且满载 +
   *            非满载 -
   */
  void printSiteLoad(int wd = 4) const {
    cout << "site load: +- is N5, and +* is quantile" << endl;
    char c;
    for (int i = 0;i < data.timeN;i++) {
      for (int j = 0;j < data.siteN;j++) {
        cout << setw(wd) << siteLoad[i][j];
        switch (isN5[i][j]) {
          case 0: { c = i == quantile[j][N5] ? '*' : ' ';break; }
          case 1: {
            c = siteLCap[i][j] > 0? '-' : '+';
            break;
          }
          default: { }
        }
        cout << c << " ";
      }
      cout << endl;
    }
  }

  void printSiteLoadByGroup(int t = -1, int wd = 5) const {
    if (t != -1) cout << "time " << t <<": ";
    cout << "site load: +- is N5, and +* is quantile" << endl;
    char c;
    int m = 0;
    for (int i = invalidSiteN;i < data.siteN;i++) {
      if (i == siteGroupMap[m]) {
        m++;
        cout << setw(wd) << i;
      } else cout << setw(wd) << "";
      cout << "  ";
    }
    cout << endl;
    for (int i = invalidSiteN;i < data.siteN;i++) {
      cout << setw(wd) << sp[i] << "  ";
    }
    cout << endl;
    for (int i = invalidSiteN;i < data.siteN;i++) {
      for (int j = 0;j < wd + 2;j++) cout << "-";
    }
    cout << endl;
    int st, ed;
    if (t == -1) {
      st = 0;
      ed = data.timeN;
    } else {
      st = t;
      ed = t + 1;
    }
    for (int i = st;i < ed;i++) {
      for (int j = invalidSiteN;j < data.siteN;j++) {
        cout << setw(wd) << siteLoad[i][sp[j]];
        switch (isN5[i][sp[j]]) {
          case 0: { c = i == quantile[sp[j]][N5] ? '*' : ' ';break; }
          case 1: {
            c = siteLCap[i][sp[j]] > 0? '-' : '+';
            break;
          }
          default: { }
        }
        cout << c << " ";
      }
      cout << endl;
    }
  }

  void checkQuantile() {
    auto siteN = data.siteN;
    auto timeN = data.timeN;

    SortArrTwo stArr(qtLen, siteLoad, isN5);
    vector<vector<int>> _quantile(siteN, vector<int>(qtLen));
    for (int i = 0;i < siteN;i++) {
      stArr.reset(i);
      for (int j = 0;j < timeN;j++) stArr.offer(j);
      for (int j = 0;j < qtLen;j++) {
        _quantile[i][j] = stArr.pop();
      }
    }

    for (int i = 0;i < siteN;i++) {
      for (int j = 0;j < qtLen;j++) {
        if (_quantile[i][j] != quantile[i][j] &&
            siteLoad[_quantile[i][j]][i] != siteLoad[quantile[i][j]][i]) {
          cout << "site " << i << endl;
          cout << "1: ";
          for (int k = 0;k < qtLen;k++) {
            cout << siteLoad[_quantile[i][k]][i] << " ";
          }
          cout << endl << "0: ";
          for (int k = 0;k < qtLen;k++) {
            cout << siteLoad[quantile[i][k]][i] << " ";
          }
          cout << endl << "error in update quantile" << endl;
          return;
        }
      }
    }

    int m = 0;
    for (int i = 0;i < siteN;i++) {
      m += siteLoad[_quantile[i][N5]][i];
    }
    if (m != f) {
      cout << "error in updateF" << endl;
    }
  }

  void verify(int*** _solu) {
    int td;
    for (int i = 0;i < data.timeN;i++) {
      for (int j = 0;j < data.cusN;j++) {
        td = 0;
        for (int k = 0;k < data.siteN;k++) {
          td += _solu[i][j][k];
          if (_solu[i][j][k] > 0 && data.qos[k][j] >= data.maxQos) {
            cout << "assigned to invalid site";
            return;
          }
          if (_solu[i][j][k] < 0) {
            cout << "time " << i;
            cout << " customer " << j ;
            cout << " site " << k << ": " << _solu[i][j][k] << endl;
            return;
          }
        }
        // 只检查需求多分配的情况，少分配视为问题无解
        if (td > data.demands[i][j]) {
          cout << "time " << i << ";";
          cout << "customer " << j << ": demands are not fully met;";
          cout << "left demands: " << data.demands[i][j] - td << endl;
          return;
        }
      }

      for (int j = 0;j < data.siteN;j++) {
        td = 0;
        for (int k = 0;k < data.cusN;k++) td += _solu[i][k][j];
        if (td > data.siteBW[j]) {
          cout << "time " << i << ": ";
          cout << "site " << j << " has exceed it's capacity for "<< td - data.siteBW[j] << endl;
          return;
        }
        if (td != siteLoad[i][j]) {
          cout << "error in update siteLoad" << endl;
        }
        if (data.siteBW[j] - td != siteLCap[i][j]) {
          cout << "error in update siteLCap" << endl;
        }
      }
    }

    int tf = f;
    initF();
    if (f != tf) {
      cout << "error in update f, should be " << f << "but get " << tf << endl;
    }
  }
};

#endif //HWCOMPETITION_LOCALSEARCH_H
