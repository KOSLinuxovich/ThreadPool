# ThreadPool

Minimalist thread pool implementation in C++17 for asynchronous task execution.

## Features

- **Generic callable support**: Accepts any callable (functions, lambdas, functors) with arbitrary arguments.
- **Type-safe result handling**: Returns `std::future` where `T` is deduced from the callable's return type using `std::invoke_result_t`.
- **Perfect forwarding**: Preserves value categories of arguments via `std::forward` to avoid unnecessary copies.
- **Thread-safe queue**: Tasks are stored in a `std::queue<std::function<void()>>` protected by `std::mutex` and synchronized with `std::conditional_variable`.
- **Worker-lifecycle management**: Threads wait efficiently on a predicate `(done || !queue.empty())` and terminate gracefully on destruction.
- **No external dependencies**: Standard library only (`<thread>`, `<mutex>`, `<queue>`, `<future>`, etc.)

## Build

```bash
g++ -std=c++17 example.cpp -o example -pthread
```

## Example
```cpp
#include "ThreadPool.hpp"

int main()
{
    SimplePool sp(1);

    // Task with no arguments and no return value
    sp.enqueue([]()
    {
        std::cout << "Errand executed" << std::endl;
    });

    // Task with arguments and return value
    auto future = sp.enqueue([](int a, int b)
    {
        return a + b + 42;
    },
    10, 32
    );

    int result = future.get();
    return 0;
}
```