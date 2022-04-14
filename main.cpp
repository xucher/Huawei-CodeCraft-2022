#include <iostream>
#include <ctime>
#include <fstream>
#include "io.h"
#include "localSearch.h"

using namespace std;

void test() {
  ofstream localOut("result.out");
  for (int i = 0;i < 21;i++) {
    startTime = time(nullptr);
    string name = to_string(i);
    if (name.size() == 1) name = "0" + name;
    cout << "instants " << name << endl;
    string inDir = "../instants/" + name + "/";
    string demand_fn = inDir + "demand.csv";
    string site_fn = inDir + "site_bandwidth.csv";
    string qos_fn = inDir + "qos.csv";
    string config_fn = inDir + "config.ini";

    Input data(demand_fn, site_fn, qos_fn, config_fn);
    LocalSearch ls(data);
    ls.run();
    localOut << ls.f << "," << time(nullptr) - startTime << endl;
    cout << "Best f = " << ls.f << "; ";
    cout << "time used " << time(nullptr) - startTime << endl << endl;
  }
  localOut.close();
}

void run() {
  startTime = time(nullptr);
  // 本地路径
#if DEBUG >= 1
  //  string inDir = "../instants/20/";
    string inDir = "../data/data2/";
    string outDir = "./";
#elif DEBUG == 0
  // 服务器路径
  string inDir = "/data/";
  string outDir = "/output/";
#endif

  string demand_fn = inDir + "demand.csv";
  string site_fn = inDir + "site_bandwidth.csv";
  string qos_fn = inDir + "qos.csv";
  string config_fn = inDir + "config.ini";
  string out_fn = outDir + "solution.txt";

  Input data(demand_fn, site_fn, qos_fn, config_fn);
  LocalSearch ls(data);
  ls.run();

  writeResult(out_fn, ls.solu, data);
#if DEBUG >= 1
  cout << "Best f = " << ls.f << "; ";
  cout << "Time used " << time(nullptr) - startTime << endl;
#endif
}

int main() {
//  test();

//  for(int i = 0;i < 10;i++) {
//    run();
//    cout << endl;
//  }

  run();
  return 0;
}