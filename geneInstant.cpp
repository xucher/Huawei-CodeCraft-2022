#pragma clang diagnostic push
#pragma ide diagnostic ignored "cppcoreguidelines-interfaces-global-init"
#pragma ide diagnostic ignored "cert-err58-cpp"
#include <iostream>
#include <fstream>
#include <random>
#include <chrono>
#include "io.h"
using namespace std;

std::mt19937 randEngine(std::chrono::system_clock::now().time_since_epoch().count());
string inDir;
string demand_fn;
string site_fn;
string qos_fn;
string config_fn;

int getRandInt(int end, int start = 0) {
  std::uniform_int_distribution<int> u{start, end - 1};
  return u(randEngine);
}

//int cusN = getRandInt(MAX_CUSTOMER, 10);
//int siteN = getRandInt(MAX_SITE, 20);
//int timeN = getRandInt(200, 100);
int cusN = 10;
int siteN = 20;
int timeN = 200;

void geneDemand() {
  ofstream outF(demand_fn);
  if (outF) {
    outF << "mtime,stream_id,";
    for (int i = 0;i < cusN;i++) {
      outF << "C" << i + 1;
      if (i != cusN - 1) outF << ",";
    }
    outF << endl;

    for (int i = 0;i < timeN;i++) {
      int typeN = getRandInt(20, 10);
      for (int k = 0;k < typeN;k++) {
        outF << i + 1 << "," << "T" << k + 1 << ",";
        for (int j = 0;j < cusN;j++) {
          outF << getRandInt(500, 100) / typeN;
          if (j != cusN - 1) outF << ",";
        }
        if (i != timeN - 1 || k != typeN - 1) outF << endl;
      }
    }

    outF.close();
  }
}

void geneSiteBW() {
  ofstream outF(site_fn);
  if (outF) {
    outF << "site_name,bandwidth" << endl;
    for (int i = 0;i < siteN;i++) {
      outF << "S" << i + 1 << ",";
      outF << getRandInt(200, 100);
//      outF << getRandInt(MAX_SITE_BW);
      outF << endl;
    }
    outF.close();
  }
}

void geneQos() {
  ofstream outF(qos_fn);
  if (outF) {
    outF << "site_name,";
    for (int i = 0;i < cusN;i++) {
      outF << "C" << i + 1;
      if (i != cusN - 1) outF << ",";
    }
    outF << endl;

    for (int i = 0;i < siteN;i++) {
      outF << "S" << i + 1 << ",";
      for (int j = 0;j < cusN;j++) {
        outF << getRandInt(MAX_QOS);
        if (j != cusN - 1) outF << ",";
      }
      outF << endl;
    }

    outF.close();
  }
}

void geneConfig() {
  ofstream outF(config_fn);
  if (outF) {
    int a = getRandInt(MAX_QOS);
    outF << "[config]" << endl;
    outF << "qos_constraint=" << a << endl;
    outF << "base_cost=" << getRandInt(2000);
    outF.close();
  }
}


int main() {
  for (int i = 20;i < 21;i++) {
    inDir = "../instants/" + to_string(i) + "/";
    demand_fn = inDir + "demand.csv";
    site_fn = inDir + "site_bandwidth.csv";
    qos_fn = inDir + "qos.csv";
    config_fn = inDir + "config.ini";

    geneDemand();
    geneSiteBW();
    geneQos();
    geneConfig();
  }
}
#pragma clang diagnostic pop