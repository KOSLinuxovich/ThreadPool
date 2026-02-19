#include <iostream>
#include <functional>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>

class SimplePool
{
    std::queue<std::function<void()>> queue_tasks;
    std::vector<std::thread> workers;
    std::mutex mtx;
    std::condition_variable cv;
    bool done = false;
    static inline std::mutex cout_mtx;
    public:
    SimplePool(int n)
    {
        for (int i = 0; i < n; ++i)
        {
            workers.emplace_back([this,i]()
            {
                while (true)
                {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(mtx);
                        // done = 0 || !tasks.empty() = 0 --> 0 || 0 = 0
                        // done = 0 || !tasks.empty() = 1 --> 0 || 1 = 1
                        // done = 1 || !tasks.empty() = 0 --> 1 || 0 = 1
                        // done = 1 || !tasks.empty() = 1 --> 1 || 1 = 1
                        cv.wait(lock,[this](){return this->done || !queue_tasks.empty();});

                        if (done && queue_tasks.empty()) 
                        { 
                            return; 
                        }

                        task = std::move(queue_tasks.front());
                        queue_tasks.pop();
                    }

                    {
                        std::lock_guard<std::mutex> cout_lock(cout_mtx);
                        std::cout << i << " thread executing the task!" << std::endl;
                    }
                    task();
                    {
                        std::lock_guard<std::mutex> cout_lock(cout_mtx);
                        std::cout << i << " thread done with the task!" << std::endl;
                    }
                }
            });
        }
    }

    void add_task(std::function<void()> f)
    {
        {
            std::lock_guard<std::mutex> lock_g(mtx);
            queue_tasks.emplace(f);
        }
        cv.notify_one();
    }

    ~SimplePool()
    {
        {
            std::lock_guard<std::mutex> lock_g(mtx);
            done = true;
        }
        cv.notify_all();

        for (auto& worker : workers)
        {
            if (worker.joinable())
            {
                worker.join();
            }
        }
    }
};

int main()
{
    SimplePool pool(5);
    // At this point, all five threads have been created. 
    // Every single thread acquires the mutex and checks whether the predicate is true. 
    // Because the predicate is false, each thread goes into a sleep state.
    std::this_thread::sleep_for(std::chrono::seconds(3));

    pool.add_task([](){std::cout << "Errand1" << std::endl;});
    // Once a task is added, an arbitrary thread wakes up and evaluates the predicate. 
    // Since it is true, after releasing the mutex the thread performs the task.
    // After it returns to the while loop to check the predicate again.
    pool.add_task([](){std::cout << "Errand2" << std::endl;});
    pool.add_task([](){std::cout << "Errand3" << std::endl;});
    pool.add_task([](){std::cout << "Errand4" << std::endl;});
    pool.add_task([](){std::cout << "Errand5" << std::endl;});
    pool.add_task([](){std::cout << "Errand6" << std::endl;});
    pool.add_task([](){std::cout << "Errand7" << std::endl;});
    pool.add_task([](){std::cout << "Errand8" << std::endl;});

    return 0;
}