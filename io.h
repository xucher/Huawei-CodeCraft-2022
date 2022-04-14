#ifndef HWCOMPETITION_IO_H
#define HWCOMPETITION_IO_H

#include <string>
using namespace std;

// 时刻数最大值
extern const int MAX_TIME;

// 客户节点数最大值
extern const int MAX_CUSTOMER;

// 客户节点带宽需求最大值
extern const int MAX_DEMAND;

// 边缘节点数最大值
extern const int MAX_SITE;

// 边缘节点的带宽上限
extern const int MAX_SITE_BW;

// 网络时延最大值，单位 ms
extern const int MAX_QOS;

// 分位百分比
extern const double QUANTILE;

class Input {
public:
  int cusN;
  string *cusID;
  int timeN;
  // [time][cus], 每个时刻最后一维为需求总和
  int **demands;

  int siteN;
  string *siteID;
  int *siteBW;

  // [site][cus]
  int **qos;
  int maxQos;

  Input(string &demand_fn, string &site_fn, string &qos_fn, string &config_fn);
  ~Input();

  void printDemands() const {
    for (int i = 0;i < cusN;i++) {
      cout << cusID[i] << " ";
    }
    cout << endl;
    for (int i = 0;i < timeN;i++) {
      for (int j = 0;j < cusN + 1;j++) {
        cout << demands[i][j] << " ";
      }
      cout << endl;
    }
    cout << endl;
  }

  void printQos() const {
    for (int i = 0;i < siteN;i++) {
      cout << siteID[i] << " ";
    }
    cout << endl;

    for (int i = 0;i < siteN;i++) {
      for (int j = 0;j < cusN;j++) {
        cout << qos[i][j] << " ";
      }
      cout << endl;
    }
    cout << endl;
  }
};

void writeResult(string &out_fn, int ***s, const Input &data);
#endif //HWCOMPETITION_IO_H
