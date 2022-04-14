#include <fstream>
#include <iostream>
#include <regex>
#include <unordered_map>
#include "io.h"

const int MAX_TIME = 8928;
const int MAX_CUSTOMER = 35;
const int MAX_DEMAND = 550000;
const int MAX_SITE = 135;
const int MAX_SITE_BW = 1000000;
const int MAX_QOS = 1000;
const double QUANTILE = 0.95;

Input::Input(string &demand_fn, string &site_fn, string &qos_fn, string &config_fn) {
  string line;
  regex re(",");
  sregex_token_iterator iter;
  sregex_token_iterator end;
  int s;
  unordered_map<string, int> cusMap;
  unordered_map<string, int> siteMap;

  ifstream fDemand(demand_fn);
  if (fDemand) {
    fDemand >> line;
    iter = sregex_token_iterator(line.begin(), line.end(), re, -1);
    cusN = 0;
    cusID = new string[MAX_CUSTOMER];
    iter++;
    while (iter != end) {
      cusID[cusN] = *iter++;
      cusMap[cusID[cusN]] = cusN;
      cusN++;
    }

    demands = new int *[MAX_TIME];
    for (int i = 0;i < MAX_TIME;i++) demands[i] = new int[cusN + 1];

    timeN = 0;
    while (true) {
      fDemand >> line;
      if (fDemand.eof()) break;

      iter = sregex_token_iterator(line.begin(), line.end(), re, -1);
      iter++;
      s = 0;
      for (int i = 0;i < cusN;i++) {
        demands[timeN][i] = stoi(*iter++);
        s += demands[timeN][i];
      }
      demands[timeN][cusN] = s;
      timeN++;
    }
    fDemand.close();
  } else {
    cout << "file " << demand_fn << " not found" << endl;
    exit(EXIT_FAILURE);
  }

  ifstream fSite(site_fn);
  if (fSite) {
    fSite >> line;

    siteN = 0;
    siteID = new string[MAX_SITE];
    siteBW = new int[MAX_SITE];
    while(true) {
      fSite >> line;
      if (fSite.eof()) break;
      iter = sregex_token_iterator(line.begin(), line.end(), re, -1);
      siteID[siteN] = *iter++;
      siteMap[siteID[siteN]] = siteN;
      siteBW[siteN] = stoi(*iter);
      siteN++;
    }
    fSite.close();
  } else {
    cout << "file " << site_fn << " not found" << endl;
    exit(EXIT_FAILURE);
  }

  ifstream fQos(qos_fn);
  if (fQos) {
    int cnt;

    fQos >> line;
    iter = sregex_token_iterator(line.begin(), line.end(), re, -1);
    iter++;
    vector<int> cusOrder(cusN);
    cnt = 0;
    while (iter != end) {
      cusOrder[cnt++] = cusMap[*iter++];
    }

    qos = new int*[siteN];
    for (int i = 0;i < siteN;i++) qos[i] = new int[cusN];

    while (true) {
      fQos >> line;
      if (fQos.eof()) break;
      iter = sregex_token_iterator(line.begin(), line.end(), re, -1);
      int site = siteMap[*iter++];
      for (int i = 0;i < cusN;i++) {
        int a = stoi(*iter++);
        qos[site][cusOrder[i]] = a;
      }
    }
    fQos.close();
  } else {
    cout << "file " << qos_fn << " not found" << endl;
    exit(EXIT_FAILURE);
  }

  ifstream fConfig(config_fn);
  if (fConfig) {
    fConfig >> line;
    fConfig >> line;
    maxQos = stoi(line.substr(15));
    fConfig.close();
  } else {
    cout << "file " << config_fn << " not found" << endl;
    exit(EXIT_FAILURE);
  }
}

Input::~Input() {
  delete[] cusID;

  for (int i = 0;i < MAX_TIME;i++) delete[] demands[i];
  delete[] demands;

  delete[] siteID;
  delete[] siteBW;

  for (int i = 0;i < siteN;i++) delete[] qos[i];
  delete[] qos;
}

void writeResult(string &out_fn, int ***s, const Input &data) {
  ofstream outFile(out_fn);
  if (outFile) {
    bool flag;
    for (int i = 0;i < data.timeN;i++) {
      for (int j = 0;j < data.cusN;j++) {
        flag = true;
        outFile << data.cusID[j] << ":";
        for (int k = 0;k < data.siteN;k++) {
          if (s[i][j][k] > 0) {
            if (flag) flag = false;
            else outFile << ",";
            outFile << "<" << data.siteID[k] << "," << s[i][j][k] << ">";
          }
        }
        if (i != data.timeN - 1 || j !=  data.cusN - 1) outFile << endl;
      }
    }

    outFile.close();
  } else {
    cout << "file " << out_fn << " not found" << endl;
    exit(EXIT_FAILURE);
  }
}
