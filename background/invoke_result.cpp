#include <iostream>
#include <type_traits>
#include <utility>

void example_function(int) {
    std::cout << "Called" << std::endl;
}

// Both decltype and invoke_result work at compile-time to determine types
int main()
{
    int x = 10;

    // 1. decltype(expression) - requires an actual expression/object
    //    The expression must be valid in the current scope
    using R1 = decltype(example_function(x));  // void

    // 2. decltype + declval - works with types only, no objects needed
    //    declval<T>() creates a "fake" object of type T for compile-time only
    //    Useful in templates when you don't have actual objects, but looks clumsy as hell.
    using R2 = decltype(example_function(std::declval<int>()));  // void

    // 3. invoke_result_t<F, Args...> - clean, robust syntax (C++17)
    //    First parameter must be a CALLABLE TYPE, not a function name
    //    That's why we need decltype(example_function) - to get the type, without we'd pass a pointer to the function. 
    //    Then we pass the argument types separately
    using R3 = std::invoke_result_t<decltype(example_function), int>;  // void

    std::cout << std::boolalpha;
    std::cout << "All are void: "
              << std::is_same_v<R1, void> << " "
              << std::is_same_v<R2, void> << " "
              << std::is_same_v<R3, void> << std::endl;

    return 0;
}