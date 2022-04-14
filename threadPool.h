#ifndef HWCOMPETITION_THREADPOOL_H
#define HWCOMPETITION_THREADPOOL_H

#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <future>
#include <functional>

template<typename T>
class ConcurrentQueue {
private:
  std::queue<T> mQueue;
  std::mutex mMutex;
public:
  bool empty() {
    std::unique_lock<std::mutex> lock(mMutex);
    return mQueue.empty();
  }
  int size() {
    std::unique_lock<std::mutex> lock(mMutex);
    return mQueue.size();
  }
  void push(T &t) {
    std::unique_lock<std::mutex> lock(mMutex);
    mQueue.template emplace(t);
  }
  bool pop(T &t) {
    std::unique_lock<std::mutex> lock(mMutex);
    if (mQueue.empty()) return false;
    t = std::move(mQueue.front());
    mQueue.pop();
    return true;
  }
};

class ThreadPool {
private:
  bool isShutDown;
  ConcurrentQueue<std::function<void()>> tasks;
  std::vector<std::thread> workers;
  std::mutex conMutex;  //
  std::condition_variable conLock;
private:
  class ThreadWorker {
  private:
    const int id;
    ThreadPool *pool;  // the ThreadPool for which it works
  public:
    ThreadWorker(ThreadPool *_pool, int _id): pool(_pool), id(_id) {};
    void operator()() {
      std::function<void()> func;
      bool dequeued;
      while (!pool->isShutDown) {
        {
          std::unique_lock<std::mutex> lock(pool->conMutex);
          if (pool->tasks.empty())
            pool->conLock.wait(lock);
          dequeued = pool->tasks.pop(func);
        }
        if (dequeued) func();
      }
    }
  };
public:
  explicit ThreadPool(int nThread = 4): workers(std::vector<std::thread>(nThread)), isShutDown(false) {};
  ThreadPool(const ThreadPool &) = delete;
  ThreadPool(ThreadPool &&) = delete;
  ThreadPool &operator=(const ThreadPool &) = delete;
  ThreadPool &operator=(ThreadPool &&) = delete;
  void init() {
    for (int i = 0;i < workers.size();i++) {
      workers[i] = std::thread(ThreadWorker(this, i));
    }
  }
  // shutdown until all workers finish their work
  void shutDown() {
    isShutDown = true;
    conLock.notify_all();

    for (auto & worker : workers) {
      if (worker.joinable()) worker.join();
    }
  }

  template<typename F, typename... Args>
  auto submit(F &&f, Args &&...args) -> std::future<decltype(f(args...))> {
    std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
    auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);
    std::function<void()> wrapper_func = [task_ptr]() { (*task_ptr)(); };
    tasks.push(wrapper_func);
    conLock.notify_one();
    return task_ptr->get_future();
  }

};


#endif //HWCOMPETITION_THREADPOOL_H
