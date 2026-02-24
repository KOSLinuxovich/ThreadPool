#include "ThreadPool.hpp"

int main() 
{
    SimplePool pool(3);
    // At this point, all three threads have been created. 
    // Every single thread acquires the mutex and checks whether the predicate is true. 
    // Because the predicate is false, each thread goes into a sleep state.
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Once a task is added, an arbitrary thread wakes up and evaluates the predicate. 
    // Since it is true, after releasing the mutex the thread performs the task.
    // After it returns to the while loop to check the predicate again.
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
    auto fut_int = pool.enqueue([](int a,int b) -> int 
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return 42 + a + b;
    },
    10,32
    );

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

    std::cout << "int result  = " << fut_int.get() << std::endl;
    std::cout << "str result  = " << fut_str.get() << std::endl;
    fut_void.get();

    std::cout << "All tasks finished" << std::endl;

    return 0;
}