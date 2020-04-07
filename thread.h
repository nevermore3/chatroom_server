//
// Created by jmq on 2020/4/6.
//

#ifndef SOCKET_THREAD_H
#define SOCKET_THREAD_H

#include <vector>
#include <utility>
#include <queue>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>
using namespace std;

/*
 *  用于创建和管理线程 ，添加任务
 */

class ThreadPool {
public:
    //任务类型
    using Task = function<void()>;

public:

    ThreadPool(int size);

    void AddThread(int size);


    template <class F, class... Args>
    auto AddTask(F&& f, Args&&... args) -> future<typename result_of<F(Args...)>::type>;


    ~ThreadPool();

private:
    ThreadPool(const ThreadPool&);//禁止复制拷贝.
    const ThreadPool& operator=(const ThreadPool&);

private:

    //线程池中线程，在没有任务时处于等待状态，可以循环的执行任务
    vector<thread> workers_;

    //task queue 用于存放没有处理的任务。提供一种缓冲机制。
    queue<function<void()> >tasks_;

    // synchronization
    mutex taskMutex_;

    condition_variable condition_;


    // 空闲线程数量
    atomic<int> idelNum_{THREAD_NUM};

    //控制线程池的执行
    atomic<bool> run_{true};

};

// 添加new task 到 任务队列中
template <class F, class... Args>
auto ThreadPool::AddTask(F&& f, Args&&... args) -> future<typename result_of<F(Args...)>::type> {
    using returnType = typename result_of<F(Args...)>::type;

    auto task = make_shared<packaged_task<returnType ()> >(bind(forward<F>(f), forward<Args>(args)...));

    future<returnType >ret = task->get_future();
    {
        unique_lock<mutex> lock(taskMutex_);
        if (!run_) {
            throw runtime_error("thread pool stop");
        }

        tasks_.emplace([task]() {
            (*task)();
        });

    }

    condition_.notify_one();
    return ret;
}

#endif //SOCKET_THREAD_H
