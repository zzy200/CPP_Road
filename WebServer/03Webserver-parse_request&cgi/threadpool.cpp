#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <functional>



class Threadpool {

public:
    Threadpool(int threadnum) : stop(false) {
        for(int i = 0; i < threadnum; i++) {
            threads.emplace_back([this] {
                while(true) {
                    std::unique_lock<std::mutex> lock(mutex);   //unique lock, set lock when initializing the lock. unique lock is the only lock that can coorperate with condition variable.
                    condition.wait(lock, [this] {return stop || !tasks.empty();});  //condition variable, let the thread sleep until the function returns true. the cv set the lock unlocked while waiting
                    if(stop && tasks.empty()) { 
                        return;     //this means there are no more tasks and the stop signal is true
                    }
                    auto task = std::move(tasks.front());
                    tasks.pop();
                    lock.unlock();
                    task();     //execute the task
                }
            });
        }
    }
    ~Threadpool() {
        std::unique_lock<std::mutex> lock(mutex);
        stop = true;
        lock.unlock();
        //awake all threads
        condition.notify_all();
        for(std::thread& thread : threads) {
            thread.join();
        }
    }

    template<class F>
    void enqueue(F&& task) {
        std::unique_lock<std::mutex> lock(mutex);
        tasks.emplace(task);
        lock.unlock();
        condition.notify_one();
    }
    
private:
    std::vector<std::thread> threads;
    std::queue<std::function<void()>> tasks;
    std::mutex mutex;
    std::condition_variable condition;
    bool stop;
};

// int main() {
//     Threadpool threadpool(4);
//     for(int i = 0; i < 8; i++) {
//         threadpool.enqueue([i] {
//             std::cout << "task " << i << " is executing" << std::endl;
//         });
//     }
// }
