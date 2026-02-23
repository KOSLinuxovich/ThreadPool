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

  
    template <typename F>
    auto enqueue(F&& f) -> std::future<std::invoke_result_t<F>> 
    {
        using ReturnType = std::invoke_result_t<F>;
        using TaskType   = std::packaged_task<ReturnType()>;

        std::shared_ptr<TaskType> task_ptr = std::make_shared<TaskType>(std::forward<F>(f));
        std::future<ReturnType> result   = task_ptr->get_future();

        {
            std::lock_guard<std::mutex> lock(mtx);
            if (done) 
            {
                throw std::runtime_error("Cannot enqueue: pool is stopped");
            }
            queue_tasks.emplace([task_ptr] 
            { 
                (*task_ptr)(); 
            });
        }

        cv.notify_one();
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

int main() 
{
    SimplePool pool(5);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    for (int i = 1; i <= 8; ++i) 
    {
        pool.enqueue([i] 
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(200 * i));
            {
                std::lock_guard lk(global_cout_mutex);
                std::cout << "Task " << i << " completed\n";
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200 * 9));
    auto fut_int = pool.enqueue([]() -> int 
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return 42;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(200 * 10));
    auto fut_str = pool.enqueue([]() -> std::string 
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return "Hello from pool!";
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(200 * 11));
    auto fut_void = pool.enqueue([] 
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        {
            std::lock_guard lk(global_cout_mutex);
            std::cout << "Void task says hi\n";
        }
    });

    std::cout << "int result  = " << fut_int.get() << '\n';
    std::cout << "str result  = " << fut_str.get() << '\n';
    fut_void.get();

    std::cout << "All tasks finished\n";
    return 0;
}