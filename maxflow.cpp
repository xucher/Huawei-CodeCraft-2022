#include <algorithm>
#include <queue>
#include <climits>
#include "maxflow.h"

void MaxFlow::reset() {
  cnt = 0;
  fill(head.begin(), head.end(), -1);
  fill(tail.begin(), tail.end(), -1);
  for (auto &edge: edges) delete edge;
}

void MaxFlow::_addEdge(int v1, int v2, int w) {
  edges[cnt] = new Edge(v1, v2, w, head[v1], tail[v2]);
  head[v1] = cnt;
  tail[v2] = cnt;
  cnt++;
}
void MaxFlow::addEdge(int v1, int v2, int w1, int w2) {
  _addEdge(v1, v2, w1);
  _addEdge(v2, v1, w2);
}

void MaxFlow::updateW(int n, int w1, int w2) {
  edges[2 * n]->w = w1;
  edges[2 * n + 1]->w = w2;
}

bool MaxFlow::findArgument(int st, const int *siteLCap, int cusN, int type) {
  queue<int> que;
  vector<bool> visit(edges.size(), false);
  // 记录每个点与起点的距离
  vector<int> dis(edges.size(), 0);
  visit[st] = true;
  pre[st].first = st;

  que.push(st);
  int v1, v2;
  while (!que.empty()) {
    v1 = que.front();que.pop();
    for (int i = head[v1];i != -1;i = edges[i]->hNext) {
      v2 = edges[i]->to;
      if (!visit[v2] && edges[i]->w > 0) {
        visit[v2] = true;
        dis[v2] = dis[v1] + 1;
        pre[v2].first = v1;
        pre[v2].second = i;
        if (dis[v2] % 2 == type && siteLCap[v2 - cusN] > 0) {
          genePath(st, v2);
          return true;
        }
        que.push(v2);
      }
    }
  }
  return false;
}

bool MaxFlow::findArgumentR(int ed, const int* exceedLoad, int cusN) {
  queue<int> que;
  vector<bool> visit(edges.size(), false);
  // 记录每个点与起点的距离
  vector<int> dis(edges.size(), 0);
  visit[ed] = true;
  pre[ed].first = ed;

  que.push(ed);
  int v1, v2;
  while (!que.empty()) {
    v1 = que.front();que.pop();
    for (int i = tail[v1];i != -1;i = edges[i]->tNext) {
      v2 = edges[i]->from;
      if (!visit[v2] && edges[i]->w > 0) {
        visit[v2] = true;
        dis[v2] = dis[v1] + 1;
        pre[v2].first = v1;
        pre[v2].second = i;
        if (dis[v2] % 2 == 0 && exceedLoad[v2 - cusN] > 0) {
          genePath(ed, v2);
          return true;
        }
        que.push(v2);
      }
    }
  }
  return false;
}

void MaxFlow::genePath(int st, int ed) {
  flow = INT_MAX;
  path.clear();

  for (int i = ed;i != st;i = pre[i].first) {
    flow = min(flow, edges[pre[i].second]->w);
    path.push_front(i);
  }
  path.push_front(st);
}

void MaxFlow::updatePath(int d) {
  for (int i = path.back();i != path.front();i = pre[i].first) {
    edges[pre[i].second]->w -= d;
    edges[pre[i].second ^ 1]->w += d;
  }
}

void MaxFlow::saveState() {
  for (int i = 0;i < cnt;i++) {
    edgeWBackup[i] = edges[i]->w;
  }
}

void MaxFlow::recover() {
  for (int i = 0;i < cnt;i++) {
    edges[i]->w = edgeWBackup[i];
  }
}