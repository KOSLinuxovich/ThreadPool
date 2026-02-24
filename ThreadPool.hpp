#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

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
    explicit SimplePool(int n) 
    {
        for (int i = 0; i < n; ++i) 
        {
            workers.emplace_back([this, i] 
            {
                while (true) 
                {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(mtx);

                        // done = 0 || !queue_tasks.empty() = 0 --> 0 || 0 = 0
                        // done = 0 || !queue_tasks.empty() = 1 --> 0 || 1 = 1
                        // done = 1 || !queue_tasks.empty() = 0 --> 1 || 0 = 1
                        // done = 1 || !queue_tasks.empty() = 1 --> 1 || 1 = 1
                        cv.wait(lock, [this] { return done || !queue_tasks.empty(); });

                        if (done && queue_tasks.empty()) 
                        {
                            return;
                        }

                        task = std::move(queue_tasks.front());
                        queue_tasks.pop();
                    }
                    
                    task();

                    {
                        std::lock_guard lk(global_cout_mutex);
                        std::cout << "Task completed by thread:\t" << i << std::endl;
                    }
                }
            });
        }
    }

    template <typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args) 
            -> std::future<std::invoke_result_t<std::decay_t<F>,std::decay_t<Args>...>> 
    {
        // Step 1: Deduce the return type of the callable when invoked with given arguments.
        // 'std::decay' gives us clean types that 'std::invoke_result_t' can work with.
        // - If f returns int --> future<int>
        // - If f returns void --> future<void>
        // - If f returns std::string --> future<std::string>
        using ReturnType = std::invoke_result_t<std::decay_t<F>,std::decay_t<Args>...>;

        // Step 2: Create a packaged_task that will hold the bound function.
        // 'std::packaged_task' wraps a callable and allows capturing its result via 'std::future'.
        // The template parameter ReturnType() means - callable returning ReturnType and taking no arguments.
        // Later on we'll bind the arguments to the function using 'std::bind'. 
        using TaskType   = std::packaged_task<ReturnType()>;

        // Step 3: Create a callable object with arguments bound
        // 'std::bind' creates a new function object that will call f with args when invoked.
        // 'std::forward' preserves the value category of f and args:
        // - If enqueue received lvalues, forward passes them as lvalues (copy)
        // - If enqueue received rvalues, forward passes them as rvalues (move)
        // This helps to maintain better scalability.
        auto bound_task = std::bind(std::forward<F>(f),std::forward<Args>(args)...);

        // Step 4: Allocate task on heap using 'std::shared_ptr'
        // Shared_ptr is being used because the task will be:
        // - Stored in queue (needs to outlive this function)
        // - Executed by a worker thread (needs to be alive when executed)
        // - Need to keep the future valid until result is retrieved
        std::shared_ptr<TaskType> task_ptr = std::make_shared<TaskType>(std::move(bound_task));

        // Step 5: Extract future before the 'enqueue' operation.
        // The future is connected to the packaged_task and will be signaled when task completes.
        std::future<ReturnType> result = task_ptr->get_future();

        // Step 6: Add task to queue under lock protection
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (done) 
            {
                throw std::runtime_error("Cannot enqueue: pool is stopped");
            }

            // Wrap the task in a lambda that simply calls it.
            // This lambda has no arguments (void()) to match 'std::function<void()>' in queue.
            // The shared_ptr ensures the task stays alive until executed.
            queue_tasks.emplace([task_ptr] ()
            { 
                (*task_ptr)(); 
            });
        }

        // Step 7: Notify one waiting thread that a new task is available.
        cv.notify_one();

        // Step 8: Return future to the caller.
        return result;
    }

    ~SimplePool() 
    {
        {
            std::lock_guard<std::mutex> lock(mtx);
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

#endif // THREAD_POOL_HPP