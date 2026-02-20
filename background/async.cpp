#include <iostream>
#include <future>
#include <thread>

int factorial(int N)
{
    int res = 1;
    for (int i = N; i > 1; i--)
    {
        res*=i;
    }
    return res;
}

int main()
{
    // std::async is a high-level function that returns a std::future object.
    // Automatically deciding whether to run in a thread or not.
    // However, we can explicitly clarify how to run a task.
    // std::launch::async - run a task in the another thread as soon as expression ends.
    // std::launch::deferred - run a task in the same thread, perform an execution deferred when std::future.get() is invoked.
    
    std::future<int> f = std::async(std::launch::async,factorial,5); // Run the task in the new thread immidiately.
    std::future<int> f2 = std::async(std::launch::deferred,factorial,5);

    std::cout << "Async result equals to: " << f.get() << std::endl;
    std::cout << "Deferred result equals to: " << f2.get() << std::endl; // Run the task in the same thread only when f2.get() is invoked.
    return 0;
}