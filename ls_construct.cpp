#include <algorithm>
#include "localSearch.h"

LocalSearch::LocalSearch(const Input &data): data(data), N5(data.timeN * (1 - QUANTILE)),
                                              qtLen(N5 + 30) {
  pool = new ThreadPool(threadN);
  pool->init();
  auto timeN = data.timeN;
  auto siteN = data.siteN;
  auto cusN = data.cusN;
  auto maxQos = data.maxQos;
  auto qos = data.qos;
  auto siteBW = data.siteBW;
  int m;

  // 客户按可分配节点个数从小到大排序后顺序
  vector<int> cp(cusN);
  iota(cp.begin(), cp.end(), 0);

  // 记录与每个客户节点相连的边缘节点个数
  vector<int> cusPriority(cusN,0);
  for (int i = 0;i < siteN;i++) {
    for (int j = 0;j < cusN;j++) {
      if (qos[i][j] < maxQos) {
        cusPriority[j]++;
      }
    }
  }

  sort(cp.begin(), cp.end(), [&cusPriority](int a, int b) {
    return cusPriority[a] < cusPriority[b];
  });

  invalidSiteN = 0;
  siteAdj = new int*[siteN];
  for (int i = 0;i < siteN;i++) siteAdj[i] = new int[cusN + 1];
  for (int i = 0;i < siteN;i++) {
    m = 0;
    for (int j = 0;j < cusN;j++) {
      if (qos[i][cp[j]] < maxQos) {
        siteAdj[i][m++] = cp[j];
      }
    }
    siteAdj[i][cusN] = m;
    if (m == 0) invalidSiteN++;
  }

  sp = new int[siteN];
  for(int i = 0;i < siteN;i++) sp[i] = i;
  // 排序依据：客户节点个数 > 首个客户节点序号 > 节点容量大小
  sort(sp, sp + siteN, [&](int a, int b) {
    if (siteAdj[a][cusN] < siteAdj[b][cusN]) return true;
    else if (siteAdj[a][cusN] > 0 && siteAdj[a][cusN] == siteAdj[b][cusN]) {
      if (siteAdj[a][0] < siteAdj[b][0] ||
          (siteAdj[a][0] == siteAdj[b][0] && siteBW[a] > siteBW[b]))
        return true;
    }
    return false;
  });

  siteGroupMap = new int[siteN + 1];
  m = 0;
  siteGroupMap[m++] = invalidSiteN;
  for (int i = invalidSiteN;i < siteN - 1;i++) {
    if (siteAdj[sp[i]][cusN] != siteAdj[sp[i + 1]][cusN]) {
      siteGroupMap[m++] = i + 1;
    } else {
      if (!equal(siteAdj[sp[i]], siteAdj[sp[i]] + siteAdj[sp[i]][cusN],
                 siteAdj[sp[i + 1]])) {
        siteGroupMap[m++] = i + 1;
      }
    }
  }
  siteGroupMap[m] = siteN;
  siteGroupMap[siteN] = m;

  siteLCap = new int*[timeN];
  for (int i = 0;i < timeN;i++) siteLCap[i] = new int[siteN];

  isN5 = new int*[timeN];
  for (int i = 0;i < timeN;i++) isN5[i] = new int[siteN];

  N5LeftD = new int*[timeN];
  for (int i = 0;i < timeN;i++) N5LeftD[i] = new int[2];

  siteLoad = new int*[timeN];
  for (int i = 0;i < timeN;i++) siteLoad[i] = new int[siteN + 1];

  quantile = new int*[siteN];
  for (int i = 0;i < siteN;i++) quantile[i] = new int[qtLen + 1];

  mf = MaxFlow(cusN * siteN * 2);
  mf.reset();
  for (int i = 0;i < siteN;i++) {
    for (int j = 0;j < siteAdj[i][cusN];j++) {
      mf.addEdge(siteAdj[i][j], cusN + i, 0, 0);
    }
  }

  exceedLoad = new int[siteN];

  solu = new int**[timeN];
  for (int i = 0;i < timeN;i++) {
    solu[i] = new int*[cusN];
    for (int j = 0;j < cusN;j++) solu[i][j] = new int[siteN];
  }

  bSolu = new int**[timeN];
  for (int i = 0;i < timeN;i++) {
    bSolu[i] = new int*[cusN];
    for (int j = 0;j < cusN;j++) bSolu[i][j] = new int[siteN];
  }
}

LocalSearch::LocalSearch(const Input &data, int tN5, int _qtLen): data(data), N5(tN5),
                                          qtLen(_qtLen) {
  auto timeN = data.timeN;
  auto siteN = data.siteN;
  auto cusN = data.cusN;

  pool = nullptr;

  siteAdj = nullptr;

  sp = nullptr;
  siteGroupMap = nullptr;

  siteLCap = new int*[timeN];
  for (int i = 0;i < timeN;i++) siteLCap[i] = new int[siteN];

  isN5 = new int*[timeN];
  for (int i = 0;i < timeN;i++) isN5[i] = new int[siteN];

  N5LeftD = new int*[timeN];
  for (int i = 0;i < timeN;i++) N5LeftD[i] = new int[2];

  siteLoad = new int*[timeN];
  for (int i = 0;i < timeN;i++) siteLoad[i] = new int[siteN + 1];

  quantile = new int*[siteN];
  for (int i = 0;i < siteN;i++) quantile[i] = new int[qtLen + 1];

  mf = MaxFlow(cusN * siteN * 2);

  exceedLoad = new int[siteN];

  solu = new int**[timeN];
  for (int i = 0;i < timeN;i++) {
    solu[i] = new int*[cusN];
    for (int j = 0;j < cusN;j++) solu[i][j] = new int[siteN];
  }

  bSolu = nullptr;
}

LocalSearch::~LocalSearch() {
  auto siteN = data.siteN;
  auto timeN = data.timeN;
  auto cusN = data.cusN;

  for (int i = 0;i < siteN;i++) delete[] siteAdj[i];
  delete[] siteAdj;

  delete[] sp;

  delete[] siteGroupMap;

  for (int i = 0;i < timeN;i++) delete[] siteLCap[i];
  delete[] siteLCap;

  for (int i = 0;i < timeN;i++) delete[] isN5[i];
  delete[] isN5;

  for (int i = 0;i < timeN;i++) delete[] N5LeftD[i];
  delete[] N5LeftD;

  for (int i = 0;i < timeN;i++) delete[] siteLoad[i];
  delete[] siteLoad;

  for (int i = 0;i < siteN;i++) delete[] quantile[i];
  delete[] quantile;

  delete[] exceedLoad;

  for (int i = 0;i < timeN;i++) {
    for (int j = 0;j < cusN;j++) delete[] solu[i][j];
    delete[] solu[i];
  }
  delete[] solu;

  for (int i = 0;i < timeN;i++) {
    for (int j = 0;j < cusN;j++) delete[] bSolu[i][j];
    delete[] bSolu[i];
  }
  delete[] bSolu;
}

void LocalSearch::cons_phase1(int **cusLDemand) {
  auto timeN = data.timeN;
  auto siteN = data.siteN;
  auto cusN = data.cusN;

  int *totalDemand = new int[timeN];
  SortArr stArr(N5, totalDemand);
  int t, c, m1, m2;
  // TODO 分配N5时刻时尽量保证总需求较大的时刻的剩余需求较为平均
  for (int i = 0;i < siteN;i++) {
    if (siteAdj[i][cusN] > 0) {
      // 计算节点每个时间段的最大需求
      fill(totalDemand, totalDemand + timeN, 0);
      for (int j = 0;j < timeN;j++) {
        for (int k = 0;k < siteAdj[i][cusN];k++) {
          c = siteAdj[i][k];
          totalDemand[j] += cusLDemand[j][c];
        }
      }

      // 找到 totalDemand 中最大的 N5 个时刻
      stArr.clear();
      for (int j = 0;j < timeN;j++) stArr.offer(j);

      // 分配并更新
      for (int j = 0;j < N5;j++) {
        t = stArr.pop();
        m1 = min(totalDemand[t], siteLCap[t][i]);
        cusLDemand[t][cusN] -= m1;
        siteLCap[t][i] -= m1;
        N5LeftD[t][0] -= m1;
        N5LeftD[t][1]--;
        isN5[t][i] = 1;
        // 分配客户需求
        for (int k = 0;k < siteAdj[i][cusN];k++) {
          c = siteAdj[i][k];
          if (cusLDemand[t][c] > 0) {
            m2 = min(m1, cusLDemand[t][c]);
            solu[t][c][i] += m2;
            cusLDemand[t][c] -= m2;
            m1 -= m2;
            if (m1 == 0) break;
          }
        }
      }
    }
  }

  delete[] totalDemand;
}

void LocalSearch::cons_phase1_greed(int **cusLDemand) {
  auto timeN = data.timeN;
  auto siteN = data.siteN;
  auto cusN = data.cusN;

  int *totalDemand = new int[timeN];
  SortArr stArr(N5 + 4, totalDemand);
  int t, c, m1, m2, site;
  for (int i = invalidSiteN;i < siteN;i++) {
    site = sp[i];
    // 计算节点每个时间段的最大需求
    fill(totalDemand, totalDemand + timeN, 0);
    for (int j = 0;j < timeN;j++) {
      for (int k = 0;k < siteAdj[site][cusN];k++) {
        c = siteAdj[site][k];
        totalDemand[j] += cusLDemand[j][c];
      }
    }

    // 找到 totalDemand 中最大的 N5 个时刻
    stArr.clear();
    for (int j = 0;j < timeN;j++) stArr.offer(j);

    // 分配并更新
    for (int j = 0;j < N5;j++) {
      t = stArr.randChoose();
      m1 = min(totalDemand[t], siteLCap[t][site]);
      cusLDemand[t][cusN] -= m1;
      siteLCap[t][site] -= m1;
      N5LeftD[t][0] -= m1;
      N5LeftD[t][1]--;
      isN5[t][site] = 1;
      // 分配客户需求
      for (int k = 0;k < siteAdj[site][cusN];k++) {
        c = siteAdj[site][k];
        if (cusLDemand[t][c] > 0) {
          m2 = min(m1, cusLDemand[t][c]);
          solu[t][c][site] += m2;
          cusLDemand[t][c] -= m2;
          m1 -= m2;
          if (m1 == 0) break;
        }
      }
    }
  }

  delete[] totalDemand;
}

void LocalSearch::updateSoluByPath(int t, list<int> &path, int d, int type) {
  auto cusN = data.cusN;
  auto iter = path.begin();
  int v1 = *iter, v2, flag = 1 - type;
  if (flag == 1) v1 -= cusN;
  while (++iter != path.end()) {
    v2 = *iter;
    if (flag == 0) {
      v2 -= cusN;
      solu[t][v1][v2] += d;
    } else {
      solu[t][v2][v1] -= d;
    }
    v1 = v2;
    flag = 1 - flag;
  }
}

void LocalSearch::repair(int t, int **cusLDemand) {
  auto cusN = data.cusN;

  int d;
  for (int i = 0;i < cusN;i++) {
    while (cusLDemand[t][i] > 0) {
      // 寻找增广路，一定是奇数条边
      if (mf.findArgument(i, siteLCap[t], cusN)) {
        d = min({cusLDemand[t][i], mf.flow, siteLCap[t][mf.path.back()- cusN]});
        cusLDemand[t][i] -= d;
        cusLDemand[t][cusN] -= d;
        siteLCap[t][mf.path.back()- cusN] -= d;
        mf.updatePath(d);
        updateSoluByPath(t, mf.path, d);
      } else {
        if (cusLDemand[t][i] > 0) {
          // 运行到这里表示问题无解
          break;
        }
      }
    }
  }
}

void LocalSearch::updateSoluByPathR(int t, list<int> &path, int d) {
  auto cusN = data.cusN;
  auto iter = path.rbegin();
  int v1 = *iter - cusN, v2, flag = 1;
  while (++iter != path.rend()) {
    v2 = *iter;
    if (flag == 1) {
      solu[t][v2][v1] -= d;
    } else {
      v2 -= cusN;
      solu[t][v1][v2] += d;
    }
    v1 = v2;
    flag = 1 - flag;
  }
}

void LocalSearch::balanceByTimeEnhanced(int t) {
  if (N5LeftD[t][1] == 0) return;

  auto cusN = data.cusN;
  auto siteN = data.siteN;

  // flag1 标记能否继续优化；flag2 标记是否需求不够；flag3 标记是否容量不够
  bool flag1;
  int flag2, flag3;
  int d, st, avgD, avgDT, ld, ls;

  // 0 N5节点或无效节点；1 需求不够；2 容量不够；3 未定
  vector<int> exclude(siteN);
  /** 计算均值
   * 节点不能达到平均值：1. 容量不够；2.需求不够
   */
  ld = N5LeftD[t][0];
  ls = N5LeftD[t][1];
  avgD = ld / ls;

  // exceedLoad 表示节点超出平均值多少，低于平均值为负值
  for (int i = 0;i < siteN;i++) {
    if (isN5[t][i] > 0 || siteAdj[i][cusN] == 0) {
      exclude[i] = 0;
      exceedLoad[i] = 0;
    } else {
      exclude[i] = 3;
      exceedLoad[i] = siteLoad[t][i] - avgD;
    }
  }

  flag1 = true;
  while (flag1) {
    flag1 = false;
    for (int i = 0;i < siteN;i++) {
      flag2 = 0;
      flag3 = 0;
      if (exclude[i] == 3 && exceedLoad[i] < 0) {
        if (siteLCap[t][i] > 0) {
          while (exceedLoad[i] < 0) {
            if (mf.findArgumentR(i + cusN, exceedLoad, cusN)) {
              flag1 = true;
              st = mf.path.back() - cusN;
              d = min({-exceedLoad[i], exceedLoad[st], mf.flow, siteLCap[t][i]});
              exceedLoad[i] += d;
              exceedLoad[st] -= d;

              siteLCap[t][st] += d;
              siteLoad[t][st] -= d;

              siteLCap[t][i] -= d;
              siteLoad[t][i] += d;
              mf.updatePath(d);
              updateSoluByPathR(t, mf.path, d);
              if (siteLCap[t][i] == 0) {
                if (exceedLoad[i] < 0) flag3 = 2;
                break;
              }
            } else {
              flag2 = 1;
              break;
            }
          }
        } else flag3 = 2;

        // 更新 avgD
        if (flag2 > 0 || flag3 > 0) {
          ld -= siteLoad[t][i];
          ls--;
          if (ls == 0) {
            flag1 = false;
            break;
          }
          avgDT = ld / ls;
          exclude[i] = max(flag2, flag3);

          if (flag3 > 0) exceedLoad[i] = 0;

          for (int j = 0;j < siteN;j++) {
            if (exclude[j] == 3) exceedLoad[j] -= avgDT - avgD;
          }
          avgD = avgDT;
          // 从头开始计算
          flag1 = true;
          break;
        }
      }
    }

    // TODO 处理均值除不尽的情况
//    if (!flag1) {
//      for (int i = 0;i < siteN;i++) {
//        if (exclude[i] == 3 && exceedLoad[i] > 0) {
//          flag1 = true;
//          for (int j = 0;j < siteN;j++) {
//            if (exclude[j] % 2 == 1) exceedLoad[j] -= 1;
//          }
//        }
//      }
//    }
  }

//  for (int i = 0;i < siteN;i++) {
//    if (exceedLoad[i] > 0) {
//      cout << exclude[i] << ", " << exceedLoad[i] << " | ";
//    }
//  }
//  cout << endl;
}

void LocalSearch::updateMF(int t) {
  int siteN = data.siteN;
  auto cusN = data.cusN;
  auto siteBW = data.siteBW;
  int c, cnt = 0;
  for (int j = 0;j < siteN;j++) {
    for (int k = 0;k < siteAdj[j][cusN];k++) {
      c = siteAdj[j][k];
      mf.updateW(cnt++, siteBW[j] - solu[t][c][j], solu[t][c][j]);
    }
  }
}

void LocalSearch::cons_phase2(int **cusLDemand) {
  auto timeN = data.timeN;
  int siteN = data.siteN;
  auto cusN = data.cusN;
  auto siteBW = data.siteBW;

  int ev, c, ld, ls, m1, m2, m3;
  for (int i = 0;i < timeN;i++) {
    if (cusLDemand[i][cusN] > 0) {
      // 期望需求值
      ld = N5LeftD[i][0];
      ls = N5LeftD[i][1];
      ev = ld / ls;
      for (int j = 0;j < siteN;j++) {
        if (siteAdj[j][cusN] > 0 && isN5[i][j] <= 0) {
          m1 = min(ev, siteLCap[i][j]);
          m3 = ev - m1;  // 容量不够而少分配的流量
          for (int k = 0;k < siteAdj[j][cusN];k++) {
            c = siteAdj[j][k];
            if (cusLDemand[i][c] > 0) {
              m2 = min(cusLDemand[i][c], m1);
              solu[i][c][j] += m2;
              cusLDemand[i][c] -= m2;
              cusLDemand[i][cusN] -= m2;
              siteLCap[i][j] -= m2;
              m1 -= m2;
              if (m1 == 0) break;
            }
          }
          if (cusLDemand[i][cusN] == 0) break;
          m3 += m1;  // 需求不够而少分配的流量
          ld -= ev - m3;
          ls--;
          if (m3 > 0 && ls != 0) ev = ld / ls;
        }
      }
    }

    updateMF(i);

    if (cusLDemand[i][cusN] > 0) repair(i, cusLDemand);
    for (int j = 0;j < siteN;j++) siteLoad[i][j] = siteBW[j] - siteLCap[i][j];
    balanceByTimeEnhanced(i);
  }
}

void LocalSearch::construct() {
  auto timeN = data.timeN;
  auto siteN = data.siteN;
  auto cusN = data.cusN;
  auto demands = data.demands;
  auto siteBW = data.siteBW;

  // 初始化 solu = 0
  for (int i = 0;i < timeN;i++) {
    for (int j = 0;j < cusN;j++) {
      for (int k = 0;k < siteN;k++) {
        solu[i][j][k] = 0;
      }
    }
  }
  // 初始化 isN5 = 0
  for (int i = 0;i < timeN;i++) {
    for (int j = 0;j < siteN;j++) {
      isN5[i][j] = 0;
    }
  }
  // 初始化 siteLCap[i][j] = siteBW[j]
  for (int i = 0;i < timeN;i++) {
    for (int j = 0;j < siteN;j++) {
      siteLCap[i][j] = siteBW[j];
    }
  }
  // 初始化 N5LeftD
  for (int i = 0;i < timeN;i++) {
    N5LeftD[i][0] = demands[i][cusN];
    N5LeftD[i][1] = siteN - invalidSiteN;
  }

  // 每个时刻客户剩余未分配流量，每行最后一位保存和
  int **cusLDemand = new int*[timeN];
  for (int i = 0;i < timeN;i++) cusLDemand[i] = new int[cusN + 1];
  for (int i = 0;i < timeN;i++) {
    copy(demands[i], demands[i] + cusN + 1, cusLDemand[i]);
  }

  cons_phase1_greed(cusLDemand);
  cons_phase2(cusLDemand);
  // 构造解的时候一定不会出现 N5节点还能继续分的情况

  // 判断是否存在未分配需求
//  for (int i = 0;i < timeN;i++) {
//    if (cusLDemand[i][cusN] > 0) {
//      cout << "time " << i << ": left " << cusLDemand[i][cusN] << endl;
//    }
//  }

  initF();

  for (int i = 0;i < timeN;i++) delete[] cusLDemand[i];
  delete[] cusLDemand;
}