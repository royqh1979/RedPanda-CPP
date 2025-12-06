#include <iostream>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <queue>
#include <future>
#include <atomic>
#include <functional>
#include <vector>

#define TEST 123

class ThreadPool
{
public:
    ThreadPool(uint32_t num):stop{false}
    {
        for (uint32_t i = 0;i < num; i ++){
            threads.emplace_back([this]{
                while(true)
                {
                    std::unique_lock<std::mutex> lock(this->mutex);
                    conv.wait(lock,[this]{
                        return !this->tasks.empty() or this->stop;
                    });
                    
                    if (this->tasks.empty() and stop)
                        return;
                    
                    std::function<auto ()->void> task = this->tasks.front();
                    this->tasks.pop();
                    lock.unlock();
                    task();
                }
            });
        }
    }
    template<typename F, typename ... Args>
    auto addTask(F && f,Args && ...args)-> void
    {
        std::function<auto()->void> task = std::bind(std::forward<F>(f),std::forward<Args>(args)...);
        std::lock_guard<std::mutex>lock(mutex);
        this->tasks.push(task);
        conv.notify_one();
        
        tasks
    }
    
    ~ThreadPool(){
        stop.store(true);
        
        conv.notify_all();
        for(auto &t: threads)
            t.join();
        tasks;
    }
    
private:
    std::mutex mutex;
    std::atomic<bool> stop;
    std::condition_variable conv;
    std::queue<std::function<auto()->void>> tasks;
    std::vector<std::thread> threads;
};

int main()
{
    ThreadPool pool(3);
    for (int i = 0;i < 9; i ++)
    {
        pool.addTask([i]{         
            std::printf("task %d start\n",i);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::printf("task %d over\n",i);
        });
    }  
}