#include <iostream>
#include <functional>
#include <queue>

// Queue of the tasks
std::queue<std::function<void()>> tasks;

// Adding task into the queue
void add_task(std::function<void()> task)
{
    tasks.emplace(task);
    std::cout << "Task was added. Task in the queue. ID:\t" << tasks.size() << std::endl;
}

// Running all tasks in one single parent thread
void run_all()
{
    std::cout << "Starting with errands!" << std::endl;

    while(!tasks.empty())
    {
        std::function<void()> current_task = tasks.front();
        tasks.pop();
        current_task();
    }

    std::cout << "All errands are completed!" << std::endl;
}

void freefunction()
{
    std::cout << "Hello from the free function1!" << std::endl;
}

class Functor
{
    public:
    void operator()()
    {
        std::cout << "Hello from the functor!" << std::endl;
    }
};

int main()
{
    Functor f;

    add_task(freefunction);

    add_task(f);

    add_task([](){std::cout << "Hello from the lambda1" << std::endl;});

    add_task([]()
    {
        std::cout << "Hello from the lambda2! Much cooler!" << std::endl;
    });

    run_all();

    return 0;
}