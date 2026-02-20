#include <iostream>
#include <functional>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <future>
#include <memory>

std::mutex global_cout_mutex;

class SimplePool
{
    std::queue<std::function<void()>> queue_tasks;
    std::vector<std::thread> workers;
    std::mutex mtx;
    std::condition_variable cv;
    bool done = false;
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
                        std::lock_guard<std::mutex> cout_lock(global_cout_mutex);
                        std::cout << i << " thread executing the task!" << std::endl;
                    }
                    task();
                    {
                        std::lock_guard<std::mutex> cout_lock(global_cout_mutex);
                        std::cout << i << " thread done with the task!" << std::endl;
                    }
                }
            });
        }
    }

    // The 'enqueue' method is implemented as a template function to enable perfect forwarding (universal references + std::forward).
    // Previously, regardless of what was passed to the function, a copy operations was always performed - even for rvalues. Inefficient.
    // Now we achieved better scalability : rvalues are moved, lvalues are copied.

    // - Free functions are lvalue;
    // - Functors are lvalues;
    // - Lambdas (temporary) are rvalues (prvalue);
    // - The results od std::bind() are rvalues;
    template <typename F>
    std::future<void> enqueue(F&& f)
    {
        using task_type = std::packaged_task<void()>;
        
        // enqueue(lambda1) --> F&& = SomeLambda&& --> std::forward<SomeLambda&&>(lambda1) ~ std::move(lambda1)
        // enqueue(functor1) --> F&& = SomeFunctor& && --> reference collapsing --> SomeFunctor& --> std::forward<SomeFunctor&>(functor1) ~ no-op. Copy.

        // The shared_ptr<task_type> (task) remains alive as long as at least one of the following is true:
        //   - the task is still present in the queue (has not yet been picked up and executed by a worker thread)
        //   - or some std::future still holds a reference to it (nobody has called .get() / .wait() yet, or the future object is still alive)
        //
        // In other words: the task object is destroyed only after both
        //   - it has been executed (removed from queue)
        //   - and all futures associated with it have been destroyed or completed (.get()/.wait() called)
        std::shared_ptr<task_type> task_ptr = std::make_shared<task_type>(std::forward<F>(f));
        std::future<void> future = task_ptr->get_future();

        {
            std::lock_guard<std::mutex> lock_g(mtx);
            if (done)
            {
                throw std::runtime_error{"Enqueue operation on stopped pool"};
            }
            queue_tasks.emplace([task_ptr]
            {
                (*task_ptr)();
            });
        }

        cv.notify_one();
        return future;
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

    // Once a task is added, an arbitrary thread wakes up and evaluates the predicate. 
    // Since it is true, after releasing the mutex the thread performs the task.
    // After it returns to the while loop to check the predicate again.
    for (int i = 1; i <= 8; ++i) {
        pool.enqueue([i] {
            std::this_thread::sleep_for(std::chrono::milliseconds(200 * i));
            std::lock_guard<std::mutex> lock_g(global_cout_mutex);
            std::cout << "Errand " << i << " done\n";
        });
    }

    return 0;
}