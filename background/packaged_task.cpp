#include <iostream>
#include <future>
#include <thread>
#include <functional>
#include <deque>

int factorial(int N)
{
    int res = 1;
    for (int i = N; i > 1; i--)
    {
        res*=i;
    }
    return res;
}

std::deque<std::packaged_task<int()>> task_q;
std::mutex mtx;
std::condition_variable cv;

// Thread 1 would pop off the task and execute it.
void thread_1()
{
    std::packaged_task<int()> t;
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock,[](){ return !task_q.empty();});
        t = std::move(task_q.front());
        task_q.pop_front();
    }
    t(); // when task is executed, a result would be stored in the associated std::future.
}

int main()
{  

    // std::packaged_task is a low-level wrapper that separates task creation from its execution. It allows us to:
    // 1. Create a task and get its future
    // 2. Pass the task to another thread for execution
    // 3. Retrieve the result later via the future

    // std::packaged_task is allowing us manually control where,when and how the task runs.

    // Start worker thread that will execute tasks from queue
    std::thread t1(thread_1);
    
    // Main thread creates a packaged_task
    std::packaged_task<int()> t(std::bind(factorial,6)); 
    std::future<int> f = t.get_future();

    {
        std::lock_guard<std::mutex> lock_g(mtx);
        task_q.push_back(std::move(t)); // Transfer task to queue
    }
    cv.notify_one();


    // Later on, when we need the result:
    std::cout << "Packaged task result equals to: " << f.get() << std::endl;

    t1.join();
    return 0;
}