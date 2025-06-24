#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

class ThreadPool
{
public:
    ThreadPool(size_t threads);

    template <class F, class... Args>
    auto enqueue(F &&f, Args &&...args)
        -> std::future<decltype(f(args...))>;

    // 等待直到有空闲线程可用
    void waitForAvailableThread();

    // 获取当前空闲线程数量
    size_t getAvailableThreads() const;

    ~ThreadPool();

private:
    // 工作线程
    std::vector<std::thread> workers;
    // 任务队列
    std::queue<std::function<void()>> tasks;

    // 同步
    mutable std::mutex queue_mutex;
    std::condition_variable condition;
    std::condition_variable available_condition; // 用于等待空闲线程
    bool stop;

    // 线程状态跟踪
    size_t total_threads;
    size_t busy_threads;
};

// 构造函数启动一定数量的工作线程
inline ThreadPool::ThreadPool(size_t threads)
    : stop(false), total_threads(threads), busy_threads(0)
{
    for (size_t i = 0; i < threads; ++i)
    {
        workers.emplace_back([this]
                             {
            for(;;) {
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    this->condition.wait(lock, [this]{ return this->stop || !this->tasks.empty(); });

                    if(this->stop && this->tasks.empty())
                        return;

                    task = std::move(this->tasks.front());
                    this->tasks.pop();

                    // 标记线程为忙碌状态
                    this->busy_threads++;
                }

                // 执行任务
                task();

                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    // 任务完成，标记线程为空闲
                    this->busy_threads--;
                    // 通知等待空闲线程的调用者
                    this->available_condition.notify_one();
                }
            } });
    }
}

// 等待直到有空闲线程可用
inline void ThreadPool::waitForAvailableThread()
{
    std::unique_lock<std::mutex> lock(queue_mutex);
    available_condition.wait(lock, [this]
                             { return this->stop || (this->busy_threads < this->total_threads); });
}

// 获取当前空闲线程数量
inline size_t ThreadPool::getAvailableThreads() const
{
    std::lock_guard<std::mutex> lock(queue_mutex);
    return total_threads - busy_threads;
}

// 添加新任务到线程池
template <class F, class... Args>
auto ThreadPool::enqueue(F &&f, Args &&...args)
    -> std::future<decltype(f(args...))>
{
    using return_type = decltype(f(args...));

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        // 不允许在停止的线程池中加入新任务
        if (stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");

        tasks.emplace([task]()
                      { (*task)(); });
    }
    condition.notify_one();
    return res;
}

// 析构函数等待所有线程完成
inline ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for (std::thread &worker : workers)
        worker.join();
}

#endif
