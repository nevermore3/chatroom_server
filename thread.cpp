//
// Created by jmq on 2020/4/6.
//

#include "common.h"
#include "thread.h"


using namespace std;

ThreadPool::ThreadPool(int size) {
    AddThread(size > THREAD_NUM ? THREAD_NUM : size);
    cout << "ThreadPool Init...,  NUM is : " << (size > THREAD_NUM ? THREAD_NUM : size) << endl;
}


/*
 *  向线程池中添加工作线程
 */
void ThreadPool::AddThread(int size) {

    for (size_t i = 0; i < size; i++) {

        workers_.emplace_back(
                [this] () {
                    cout << "Work Thread : " << this_thread::get_id() << "  is Created" <<endl;
                    while (run_) {
                        Task task;

                        {
                            /*
                             *  1. 获取任务队列锁
                             *  2. 在有任务的情况下，从任务队列中取出一个任务
                             */
                            unique_lock<mutex> taskLock (this->taskMutex_);
                            this->condition_.wait(taskLock, [this] {
                                return !run_ || !tasks_.empty();
                            });

                            if (!run_ && tasks_.empty()) {
                                return;
                            }

                            task = move(this->tasks_.front());
                            this->tasks_.pop();
                        }

                        // 执行该任务
                        idelNum_--;
                        task();
                        idelNum_++;
                    }
                }
            );
    }
}

ThreadPool::~ThreadPool() {
    {
        unique_lock<mutex>lock(taskMutex_);
        run_ = false;
    }
    condition_.notify_all();
    for (thread &work: workers_) {
        if (work.joinable()) {
            work.join();
        }
    }
    cout<<"Thread Pool End"<<endl;
}
