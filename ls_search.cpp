#include <unordered_set>
#include <algorithm>
#include <unordered_map>
#include "localSearch.h"

vector<LocalSearch *> runner;

int searchTask(int id, int site, int t1, int t2) {
  runner[id]->reAssignN5(site, t1, t2);
  return runner[id]->f;
}

int updateTask(int id, LocalSearch *ls, int site, int t1, int t2, int t3) {
  runner[id]->updateState(ls, site, t1, t2, t3);
  return 1;
}

void LocalSearch::initF() {
  auto siteN = data.siteN;
  auto timeN = data.timeN;

  SortArrTwo stArr(qtLen, siteLoad, isN5);
  for (int i = 0;i < siteN;i++) {
    stArr.reset(i);
    for (int j = 0;j < timeN;j++) stArr.offer(j);
    for (int j = 0;j < qtLen;j++) {
      quantile[i][j] = stArr.pop();
    }
  }

  f = 0;
  for (int i = 0;i < siteN;i++) {
    f += siteLoad[quantile[i][N5]][i];
  }
}

void LocalSearch::updateF(int t1, int t2) {
  auto siteN = data.siteN;

  f = 0;
  bool flag, flag1;
  int t, cnt, qt, j, m;
  for (int i = 0;i < siteN;i++) {
    // t1 为负载较大的一个，t2为负载较小的一个
    if (siteLoad[t1][i] <= siteLoad[t2][i]) swap(t1, t2);

    // 纪录目前找到了排序在第几的时间
    cnt = 0;
    t = t1;
    flag = false;
    flag1 = false;
    for (j = 0;j < qtLen;j++) {
      qt = quantile[i][j];
      if (qt == t1 || qt == t2) continue;
      if (siteLoad[t][i] >= siteLoad[qt][i]) {
        if (cnt++ == N5) {
          flag1 = true;
          break;
        }
        if (flag) break;
        t = t2;
        j--;

        flag = true;
      } else if (cnt++ == N5) break;
    }

    if (cnt == N5 + 1 && flag1) f += siteLoad[t][i];
    else {
      m = N5 + 1 - cnt;
      while (m > 0 && j < qtLen) {
        if (quantile[i][j] != t1 && quantile[i][j] != t2) {
          m--;
          if (m == 0) break;
        }
        j++;
      }
      if (j < qtLen) f += siteLoad[quantile[i][j]][i];
      else {
        vector<int> tmpQuan(qtLen);
        SortArrTwo stArr(qtLen, siteLoad, isN5);
        stArr.reset(i);
        for (int k = 0;k < data.timeN;k++) stArr.offer(k);
        for (int k = 0;k < qtLen;k++) tmpQuan[k] = stArr.pop();
        f += siteLoad[tmpQuan[N5]][i];
      }
    }
  }
}

void LocalSearch::updateQuantile(int t1, int t2) {
  auto siteN = data.siteN;
  auto timeN = data.timeN;

  bool flag;
  SortArrTwo stArr(qtLen, siteLoad, isN5);
  int t, qt, j, cnt;
  for (int i = 0;i < siteN;i++) {
    // 移除原本在 quantile 中的 t1, t2
    // 记录是否需要重算更新
    cnt = 0;
    for (int k = 0;k < qtLen;k++) {
      qt = quantile[i][k];
      if (qt == t1 || qt == t2) {
        move(quantile[i] + k + 1, quantile[i] + qtLen, quantile[i] + k);
        cnt--;
        k--;
        if (cnt == -2) break;
      }
    }


    // t1 为负载较大的一个，t2为负载较小的一个
    // 取等是需要将刚进来的 t2 尽量往前排，在相等时其可以排序在前 N5
    if (siteLoad[t1][i] <= siteLoad[t2][i]) swap(t1, t2);

    t = t1;
    flag = false;
    for (j = 0;j < qtLen;j++) {
      if (j == qtLen + cnt) break;
      qt = quantile[i][j];
      if (siteLoad[t][i] >= siteLoad[qt][i]) {
        cnt++;
        move_backward(quantile[i] + j, quantile[i] + qtLen, quantile[i] + qtLen + 1);
        quantile[i][j] = t;


        if (flag) break;
        t = t2;
        flag = true;
      }
    }

    if (cnt < 0) {
      stArr.reset(i);
      for (int k = 0;k < timeN;k++) stArr.offer(k);
      for (int k = 0;k < qtLen;k++) {
        quantile[i][k] = stArr.pop();
      }
    }
  }
}

void LocalSearch::updateQuantile(int **_quantile) {
  auto siteN = data.siteN;
  auto timeN = data.timeN;

  for (int i = 0;i < siteN;i++) {
    for (int j = 0;j < qtLen;j++) {
      quantile[i][j] = _quantile[i][j];
    }
  }
}

void LocalSearch::maxN5SiteD(int time, int site) {
  auto cusN = data.cusN;
  auto siteN = data.siteN;

  int st, d;
  // 不移动其他 N5 节点的流量
  for (int i = 0;i < siteN;i++) {
    if (isN5[time][i] > 0 || siteAdj[i][cusN] == 0) {
      exceedLoad[i] = 0;
    } else {
      exceedLoad[i] = siteLoad[time][i];
    }
  }
  exceedLoad[site] = -siteLCap[time][site];

  if (exceedLoad[site] < 0) {
    while (mf.findArgumentR(site + cusN, exceedLoad, cusN)) {
      st = mf.path.back() - cusN;
      d = min({mf.flow, -exceedLoad[site], exceedLoad[st]});

      exceedLoad[st] -= d;
      exceedLoad[site] += d;

      siteLCap[time][st] += d;
      siteLoad[time][st] -= d;

      siteLCap[time][site] -= d;
      siteLoad[time][site] += d;
      mf.updatePath(d);
      updateSoluByPathR(time, mf.path, d);
      if (exceedLoad[site] == 0) break;
    }
  }
}

void LocalSearch::reAssignN5(int site, int t1, int t2) {
  isN5[t1][site] = 0;
  N5LeftD[t1][0] += siteLoad[t1][site];
  N5LeftD[t1][1]++;

  updateMF(t2);
  maxN5SiteD(t2, site);

  isN5[t2][site] = 1;
  N5LeftD[t2][0] -= siteLoad[t2][site];
  N5LeftD[t2][1]--;

  balanceByTimeEnhanced(t2);

  updateMF(t1);
  balanceByTimeEnhanced(t1);

  updateF(t1, t2);
}

void LocalSearch::saveBestSolu() {
  bestF = f;
  for (int i = 0;i < data.timeN;i++) {
    for (int j = 0;j < data.cusN;j++) {
      for (int k = 0;k < data.siteN;k++) {
        bSolu[i][j][k] = solu[i][j][k];
      }
    }
  }
}

void LocalSearch::recoverBestSolu() {
  auto timeN = data.timeN;
  auto siteN = data.siteN;
  auto cusN = data.cusN;
  auto siteBW = data.siteBW;

  f = bestF;
  for (int i = 0;i < timeN;i++) {
    for (int j = 0;j < cusN;j++) {
      for (int k = 0;k < siteN;k++) {
        solu[i][j][k] = bSolu[i][j][k];
      }
    }
  }

  int m;
  for (int i = 0;i < timeN;i++) {
    for (int j = 0;j < siteN;j++) {
      m = 0;
      for (int k = 0;k < cusN;k++) {
        m += solu[i][k][j];
      }
      siteLoad[i][j] = m;
      siteLCap[i][j] = siteBW[j] - m;
    }
  }
}

/** TODO
 * 目标分位点越小，调整效果越好
 * 分为两步，先从大到小排，降低分位点到下一层次
 * 然后从小到大排，降低分位点到0
 * 如何选择放弃某些点的分位点降低，
 */
void LocalSearch::adjust() {
  auto siteN = data.siteN;
  auto cusN = data.cusN;
  auto timeN = data.timeN;
  int t, validTimeN, c;

  // 计算分位点
  SortArrTwo stArr(qtLen, siteLoad, isN5);
  for (int i = 0;i < siteN;i++) {
    stArr.reset(i);
    for (int j = 0;j < timeN;j++) stArr.offer(j);
    for (int j = 0;j < qtLen;j++) {
      quantile[i][j] = stArr.pop();
    }
  }

  // 计算需要调整的时刻
  unordered_set<int> tSet;
  vector<int> tp(timeN, -1);
  vector<int> rtp(timeN, -1);
  for (int i = 0;i < siteN;i++) {
    for (int j = N5;j < qtLen;j++) {
      tSet.insert(quantile[i][j]);
    }
  }
  validTimeN = 0;
  for (auto &x: tSet) {
    tp[validTimeN] = x;
    rtp[x] = validTimeN;
    validTimeN++;
  }

  vector<MaxFlow> mfs(validTimeN);
  for (int i = 0;i < validTimeN;i++) {
    mfs[i] = MaxFlow(cusN * siteN * 2);
    mfs[i].reset();
    for (int j = 0;j < siteN;j++) {
      for (int k = 0;k < siteAdj[j][cusN];k++) {
        t = tp[i];
        c = siteAdj[j][k];
        mfs[i].addEdge(c, cusN + j, data.siteBW[j] - solu[t][c][j], solu[t][c][j]);
      }
    }
  }
  int **exceedLoad = new int *[validTimeN];
  for (int i = 0;i < validTimeN;i++) exceedLoad[i] = new int[siteN + 1];
  vector<bool> exclude(siteN);
  int excludeN = 0;

  for (int i = 0;i < validTimeN;i++) {
    exceedLoad[i][siteN] = 0;
  }

  for (int i = 0;i < siteN;i++) {
    if (siteAdj[i][cusN] == 0) {
      exclude[i] = true;
      excludeN++;
      for (int j = 0;j < validTimeN;j++) {
        exceedLoad[j][i] = 0;
      }
    } else {
      t = quantile[i][N5];
      if (siteLoad[t][i] == siteLoad[quantile[i][qtLen - 1]][i]) {
        exclude[i] = true;
        excludeN++;
        // TODo 先调整将其降低到 siteLoad[t][siteN]
        for (int j = 0;j < validTimeN;j++) {
          if (siteLoad[tp[j]][i] < siteLoad[t][i]) {
            exceedLoad[j][i] = siteLoad[t][i] - siteLoad[tp[j]][i];
            exceedLoad[j][siteN] += exceedLoad[j][i];
          } else exceedLoad[j][i] = 0;
        }
      } else {
        exclude[i] = false;
        for (int j = 0;j < validTimeN;j++) {
          if (siteLoad[tp[j]][i] < siteLoad[quantile[i][qtLen - 1]][i]) {
            exceedLoad[j][i] = siteLoad[quantile[i][qtLen - 1]][i] - siteLoad[tp[j]][i];
            exceedLoad[j][siteN] += exceedLoad[j][i];
          } else exceedLoad[j][i] = 0;
        }
      }
    }
  }

  vector<list<int>> paths;
  vector<int> pathFlow;
  vector<int> pathInd(qtLen - N5);
  vector<vector<int>> exceedBack(validTimeN, vector<int>(siteN));

  vector<int> sites(siteN);
  iota(sites.begin(), sites.end(), 0);

  int st, ed, flag1, flag2, t1, t2, d, q;
  int lastUpdate = 0;
  for (int i = N5 + 1;i < qtLen;i++) {
    flag1 = false;
    // 节点按分位点排序
    sort(sites.begin(), sites.end(), [this](int a, int b) {
      return siteLoad[quantile[a][N5]][a] > siteLoad[quantile[b][N5]][b];
    });
    for (auto &j: sites) {
      if (!exclude[j]) {
        flag2 = true;
        paths.clear();
        pathFlow.clear();
        for (int k = N5;k < i;k++) {
          t1 = quantile[j][k];
          t2 = quantile[j][i];
          pathInd[k - N5] = -1;
          if (siteLoad[t1][j] > siteLoad[t2][j]) {
            exceedLoad[rtp[t1]][j] = siteLoad[t2][j] - siteLoad[t1][j];
            if (-exceedLoad[rtp[t1]][j] <= exceedLoad[rtp[t1]][siteN]) {
              pathInd[k - N5] = paths.size();
              mfs[rtp[t1]].saveState();
              for (int l = 0;l < siteN;l++) exceedBack[rtp[t1]][l] = exceedLoad[rtp[t1]][l];

              while (exceedLoad[rtp[t1]][j] < 0) {
                if (mfs[rtp[t1]].findArgument(j + cusN, exceedLoad[rtp[t1]], cusN, 0)) {
                  ed = mfs[rtp[t1]].path.back() - cusN;
                  d = min({-exceedLoad[rtp[t1]][j], mfs[rtp[t1]].flow, exceedLoad[rtp[t1]][ed]});
                  mfs[rtp[t1]].updatePath(d);
                  paths.emplace_back(mfs[rtp[t1]].path);
                  pathFlow.push_back(d);
                  exceedLoad[rtp[t1]][j] += d;
                  exceedLoad[rtp[t1]][ed] -= d;
                } else {
                  // 不能调整
                  flag2 = false;
                  break;
                }
              }
            } else flag2 = false;

            if (!flag2) {
              for (int l = N5;l <= k;l++) {
                if (pathInd[l - N5] != -1) {
                  t = rtp[quantile[j][l]];
                  mfs[t].recover();
                  for (int k1 = 0;k1 < siteN;k1++) exceedLoad[t][k1] = exceedBack[t][k1];
                }
              }

              // 设定其多余流量
              for (int l = 0;l < validTimeN;l++) {
                if (siteLoad[tp[l]][j] < siteLoad[quantile[j][i - 1]][j]) {
                  exceedLoad[l][j] = siteLoad[quantile[j][i - 1]][j] - siteLoad[tp[l]][j];
                  exceedLoad[l][siteN] += exceedLoad[l][j];
                }
              }
              exclude[j] = true;
              excludeN++;
              break;
            }
          }
        }

        if (flag2) {
          flag1 = true;
          lastUpdate = i;
          q = N5;
          for (int k = 0;k < paths.size();k++) {
            if (k == pathInd[q - N5]) {
              t = quantile[j][q];
              q++;
            }
            st = paths[k].front() - cusN;
            ed = paths[k].back() - cusN;

            siteLCap[t][st] += pathFlow[k];
            siteLoad[t][st] -= pathFlow[k];

            siteLCap[t][ed] -= pathFlow[k];
            siteLoad[t][ed] += pathFlow[k];
            updateSoluByPath(t, paths[k], pathFlow[k], 0);
            exceedLoad[rtp[t]][siteN] -= pathFlow[k];
          }
        }
      }
    }
    if (!flag1 || excludeN == siteN) break;
  }

#if DEBUG >= 2
  cout << "validTime = " << validTimeN << ", ";
  cout << "lastUpdate = " << lastUpdate << ", ";
  cout << "left site N = " << siteN - excludeN << ", ";
  int totalLeft = 0;
  for (int i = 0;i < validTimeN;i++) {
    totalLeft += exceedLoad[i][siteN];
  }
  cout << "left load " << totalLeft << endl;
#endif

  int initF = f;
  f = 0;
  for (int i = 0;i < siteN;i++) {
    f += siteLoad[quantile[i][N5]][i];
  }


#if DEBUG >= 2
  cout << "Adjust phase: from " << initF << " to " << f << endl;
#endif

  for (int i = 0;i < validTimeN;i++) delete[] exceedLoad[i];
  delete[] exceedLoad;
}

void LocalSearch::loadState(LocalSearch *ls, bool isFirst) {
  auto timeN = data.timeN;
  auto siteN = data.siteN;
  auto cusN = data.cusN;

  if (isFirst) {
    siteAdj = ls->siteAdj;
    invalidSiteN = ls->invalidSiteN;

    mf.reset();
    for (int i = 0;i < siteN;i++) {
      for (int j = 0;j < siteAdj[i][cusN];j++) {
        mf.addEdge(siteAdj[i][j], cusN + i, 0, 0);
      }
    }
  }

  for (int i = 0;i < timeN;i++) {
    for (int j = 0;j < siteN;j++) {
      siteLCap[i][j] = ls->siteLCap[i][j];
      isN5[i][j] = ls->isN5[i][j];
      siteLoad[i][j] = ls->siteLoad[i][j];
    }
    for (int j = 0;j < 2;j++) {
      N5LeftD[i][j] = ls->N5LeftD[i][j];
    }
  }

  for (int i = 0;i < timeN;i++) {
    for (int j = 0;j < cusN;j++) {
      for (int k = 0;k < siteN;k++) {
        solu[i][j][k] = ls->solu[i][j][k];
      }
    }
  }

  for (int i = 0;i < siteN;i++) {
    for (int j = 0;j < qtLen;j++) {
      quantile[i][j] = ls->quantile[i][j];
    }
  }
}

void LocalSearch::updateState(LocalSearch *ls, int site, int t1, int t2, int t3) {
  auto siteN = data.siteN;
  auto cusN = data.cusN;

  isN5[t1][site] = ls->isN5[t1][site];
  isN5[t2][site] = ls->isN5[t2][site];
  isN5[t3][site] = ls->isN5[t3][site];

  N5LeftD[t1][0] = ls->N5LeftD[t1][0];
  N5LeftD[t1][1] = ls->N5LeftD[t1][1];

  N5LeftD[t2][0] = ls->N5LeftD[t2][0];
  N5LeftD[t2][1] = ls->N5LeftD[t2][1];

  N5LeftD[t3][0] = ls->N5LeftD[t3][0];
  N5LeftD[t3][1] = ls->N5LeftD[t3][1];

  for (int i = 0;i < siteN;i++) {
    siteLoad[t1][i] = ls->siteLoad[t1][i];
    siteLoad[t2][i] = ls->siteLoad[t2][i];
    siteLoad[t3][i] = ls->siteLoad[t3][i];

    siteLCap[t1][i] = ls->siteLCap[t1][i];
    siteLCap[t2][i] = ls->siteLCap[t2][i];
    siteLCap[t3][i] = ls->siteLCap[t3][i];
  }

  for (int i = 0;i < cusN;i++) {
    for (int j = 0;j < siteN;j++) {
      solu[t1][i][j] = ls->solu[t1][i][j];
      solu[t2][i][j] = ls->solu[t2][i][j];
      solu[t3][i][j] = ls->solu[t3][i][j];
    }
  }
}


void LocalSearch::checkRunnerState(int n) {
  auto timeN = data.timeN;
  auto siteN = data.siteN;
  auto cusN = data.cusN;

  for (int kk = 0;kk < n;kk++) {
    for (int i = 0;i < timeN;i++) {
      for (int j = 0;j < siteN;j++) {
        if (runner[kk]->siteLCap[i][j] != siteLCap[i][j]
            || runner[kk]->isN5[i][j] != isN5[i][j]
            || runner[kk]->siteLoad[i][j] != siteLoad[i][j])
          cout << "error in runner state";
      }
      for (int j = 0;j < 2;j++) {
        if (runner[kk]->N5LeftD[i][j] != N5LeftD[i][j])
          cout << "error in runner state";
      }
    }

    for (int i = 0;i < timeN;i++) {
      for (int j = 0;j < cusN;j++) {
        for (int k = 0;k < siteN;k++) {
          if (runner[kk]->solu[i][j][k] != solu[i][j][k]) {
            cout << runner[kk]->solu[i][j][k] << ", " << solu[i][j][k] << endl;
            cout << "error in runner state";
          }
        }
      }
    }

    for (int i = 0;i < siteN;i++) {
      for (int j = 0;j < qtLen;j++) {
        if (runner[kk]->quantile[i][j] != quantile[i][j])
          cout << "error in runner state";
      }
    }
  }
}

void LocalSearch::searchParallel(int endTime) {
  auto siteN = data.siteN;
  auto cusN = data.cusN;
  auto timeN = data.timeN;

  for (int i = 0;i < threadN;i++) {
    runner.emplace_back(new LocalSearch(data, N5, qtLen));
    runner[i]->loadState(this);
  }

  // 初始化 siteN5T[site][ind] time 每个节点对应的 N5个时刻
  int **siteN5T = new int *[siteN];
  for (int i = 0;i < siteN;i++) siteN5T[i] = new int[N5];
  int m;
  for (int i = 0;i < siteN;i++) {
    m = 0;
    for (int j = 0;j < timeN;j++) {
      if (isN5[j][i] > 0) {
        siteN5T[i][m++] = j;
      }
    }
  }
  // TODO 标记 isN5

  vector<int> rdSite(siteN);
  iota(rdSite.begin(), rdSite.end(), 0);
  vector<int> rdT1(N5);
  iota(rdT1.begin(), rdT1.end(), 0);

  int site, t1, t2, bf, d;
  int moveFlag = 1;

//TODO 以下三个参数为模拟退火需要调整的
  int temperature = 1000;//模拟退火的初始温度
  int frequencyM = 100;//退火频率,每经过frequency，降温一次
  float alpha = 0.95;//退火速率

  int counterM = 1;//记录move的次数
  float acceptPro;//接受解的概率
  float epsilon;//随机生成的概率


#if DEBUG >= 4
  int timeInterval = 2;
  int outT = 2;
#endif
  bool flag;
  future<int> futures[threadN];
  vector<int> fs(threadN);
  vector<int> ts(threadN);

  while (moveFlag) {
#if DEBUG >= 4
    if ((clock() - startTime) / CLOCKS_PER_SEC > outT) {
      cout << "current f = " << f << endl;
      outT += timeInterval;
    }
#endif
    moveFlag = 0;

    shuffle(rdSite.begin(), rdSite.end(), randEngine);

    for (int i = 0;i < siteN;i++) {
      if (counterM % frequencyM == 0)
        temperature = temperature * alpha;
      if (temperature < 50)temperature = 50;//避免temperature太小而跳出while
      site = rdSite[i];
      if (siteAdj[site][cusN] == 0) continue;

      // 优先考虑未被选为 N5，但排序为 N5的节点
      t2 = -1;
      for (int j = 0;j < N5;j++) {
        if (isN5[quantile[site][j]][site] <= 0) {
          t2 = quantile[site][j];
          break;
        }
      }
      if (t2 == -1) {
        t2 = quantile[site][N5];
        if (isN5[t2][site] > 0) continue;
      }

      shuffle(rdT1.begin(), rdT1.end(), randEngine);

      m = 0;
      for (int j = 0;j < N5;j++) {
        t1 = siteN5T[site][rdT1[j]];

        futures[m] = pool->submit(searchTask, m, site, t1, t2);
        ts[m] = rdT1[j];
        m++;

        if (m == threadN || j == N5 - 1) {
          for (int k = 0;k < m;k++) fs[k] = futures[k].get();
          d = distance(min(fs.begin(), fs.end()), fs.begin());
          //以下为模拟退火

          if (fs[d] < f)acceptPro = 1;
          else acceptPro = exp(-(fs[d] - f) / temperature);
          epsilon = (float) getRandInt(100) / 100;

          if (epsilon < acceptPro) {
            f = fs[d];

            for (int k = 0;k < m;k++) {
              if (k != d) {
                runner[k]->updateState(runner[d], site, siteN5T[site][ts[k]], t2, siteN5T[site][ts[d]]);
              } else {
                updateState(runner[d], site, 0, t2, siteN5T[site][ts[d]]);
                updateQuantile(siteN5T[site][ts[d]], t2);
              }
            }

            if (f < bestF) {
              saveBestSolu();
#if DEBUG >= 2
              cout<<"find better solution, f="<<bestF<<", temperature="<<temperature<<endl;
#endif
            }

            for (int k = 0;k < m;k++) {
              runner[k]->updateQuantile(quantile);
            }
            for (int k = m;k < threadN;k++) {
              runner[k]->updateState(runner[d], site, 0, t2, siteN5T[site][ts[d]]);
              runner[k]->updateQuantile(quantile);
            }
            siteN5T[site][ts[d]] = t2;
            moveFlag = 1;
            counterM++;
          } else {
            for (int k = 0;k < m;k++) {
              runner[k]->updateState(this, site, siteN5T[site][ts[k]], t2, 0);
            }
          }
#if DEBUG >= 3
          checkRunnerState(threadN);
          checkSiteLoad();
          checkN5LeftD();
          checkQuantile();
          checkIsN5(siteN5T);
#endif
          if (time(nullptr) - startTime > endTime) return;
          if (moveFlag) break;
          m = 0;
        }
        if (time(nullptr) - startTime > endTime) return;
      }
#if DEBUG >= 3
      checkRunnerState(threadN);
      checkSiteLoad();
      checkN5LeftD();
      checkQuantile();
      checkIsN5(siteN5T);
#endif
      if (moveFlag) break;
    }
    moveFlag = 1;//避免跳出while
    if (time(nullptr) - startTime > 280) break;
  }

  vector<int> avgs(timeN);
  for (int i = 0;i < timeN;i++) {
    avgs[i] = N5LeftD[i][0] / N5LeftD[i][1];
  }

  vector<int> t2Ind(timeN);
  for(int i = 0;i < timeN;i++) t2Ind[i] = i;
  sort(t2Ind.begin(), t2Ind.end(), [&avgs](int a, int b) {
    return avgs[a] > avgs[b];
  });
  for (int i = 0;i < 30;i++) {
    cout << setw(5) << avgs[t2Ind[i]] << " ";
  }
  cout << endl;
  unordered_map<int, unordered_map<int, int>> N5SiteLoad;
  for (int i = 0;i < siteN;i++) {
    N5SiteLoad[quantile[i][N5]][i] = 1;
  }
  
  for (int i = 0;i < 30;i++) {
    cout << setw(5) << N5SiteLoad[t2Ind[i]].size() << " ";
  }
  cout << endl;

  for (int i = 0;i < 30;i++) {
    cout << setw(5) << siteN - invalidSiteN - N5LeftD[t2Ind[i]][1] << " ";
  }
  cout << endl;

  for (int i = 0;i < siteN;i++) delete[] siteN5T[i];
  delete[] siteN5T;
}

void LocalSearch::run() {
  construct();
#if DEBUG >= 2
  cout << "construct end time " << time(nullptr) - startTime << endl;
  cout << "init solution f = " << f << endl;
#endif
  searchParallel(295);
  if (f > bestF) recoverBestSolu();
#if DEBUG >= 2
  cout << "search end time " << time(nullptr) - startTime << endl;
#endif
  adjust();
#if DEBUG >= 2
  verify(solu);
#endif
//  printSiteLoadByGroup(-1, 6);
  // TODO 直接结束线程
  pool->shutDown();
}

