#ifndef HWCOMPETITION_MAXFLOW_H
#define HWCOMPETITION_MAXFLOW_H
#include <vector>
#include <list>

using namespace std;
/**
 * 邻接表表示，适用于稀疏图
 */
class MaxFlow {
private:
  class Edge {
  public:
    int from;
    int to;
    int w;
    // 同起点的下一条边的编号
    int hNext;
    // 同终点的下一条边的编号
    int tNext;
    Edge() = default;
    Edge(int from, int to, int w, int hNext, int tNext): from(from), to(to),
        w(w), hNext(hNext), tNext(tNext) {};
  };
  vector<Edge*> edges;
  // 边的权重备份
  vector<int> edgeWBackup;
  // 起点为顶点 i 的第一条边的编号
  vector<int> head;
  // 终点为顶点 i 的第一条边的编号
  vector<int> tail;
  // 加边时的计数器
  int cnt = 0;
  // 记录增广路路径，记录代表路径中前一个点及序号
  vector<pair<int, int>> pre;
public:
  MaxFlow() = default;
  explicit MaxFlow(int edgeN): edges(edgeN), edgeWBackup(edgeN), head(edgeN), pre(edgeN), tail(edgeN) {};
  ~MaxFlow() {
    for (auto &edge : edges) delete edge;
  };

  list<int> path;
  int flow = -1;

private:
  void _addEdge(int v1, int v2, int w);
public:
  void reset();
  // w1 为正向边权重，w2 为反向边权重
  void addEdge(int v1, int v2, int w1, int w2);
  // 更新边权
  void updateW(int n, int w1, int w2);
  // edi 路径中最后一条边的编号
  void genePath(int st, int ed);
  // 寻找增广路。 type = 1，cus->site; type = 0, site -> site
  bool findArgument(int st, const int *siteLCap, int cusN, int type = 1);
  // 从终点开始寻找增广路
  bool findArgumentR(int ed, const int* exceedLoad, int cusN);
  // 更新边权，d 为路径上的流量
  void updatePath(int d);
  void saveState();
  void recover();
};

#endif //HWCOMPETITION_MAXFLOW_H
