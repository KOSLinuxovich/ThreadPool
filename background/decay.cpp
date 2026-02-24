#include <iostream>
#include <type_traits>
#include <typeinfo>

template <typename F>
void foo(F&&)
{
    if (std::is_same_v<F,int>)
    {
        std::cout << "F and Int are the same data types!" << std::endl;
    }
    else
    {
        std::cout << "F and Int aren't the same data types!" << std::endl;
    }
}

template <typename F>
void bar(F&&)
{
    if (std::is_same_v<std::decay_t<F>,int>)
    {
        std::cout << "F and Int are the same data types!" << std::endl;
    }
    else
    {
        std::cout << "F and Int aren't the same data types!" << std::endl;
    }
}

template <typename T>
void demonstrate(T&&)
{
    std::cout << "Original: " << typeid(T).name() << std::endl;
    std::cout << "remove_reference: " << typeid(std::remove_reference_t<T>).name() << std::endl;
    std::cout << "remove_cv: " << typeid(std::remove_cv_t<T>).name() << std::endl;
    std::cout << "decay: " << typeid(std::decay_t<T>).name() << std::endl;
    
    // std::decay performs more transformations than std::remove_reference and std::remove_cv combined.
    // 1. Removes references
    // 2. Remove cv qualifiers
    // 3. Array-to-pointer conversion: int[5] --> int*, const char[6] --> const char*
    // 4. Function-to-pointer conversion: void(int) --> void(*)(int), double(double,double) --> double(*)(double,double)
    
    // Therefore, std::decay is equivalent to std::remove_cv<std::remove_reference<T>::type>::type

    // 5. However, functors and lambdas aren't converted to pointers. 
    // The reason is: functors and lambda functions can capture variables (state), whereas function pointers cannot.
    // Forcing a conversion would lead to the loss of this state.
}

int main()
{
    int first = 3;
    foo(first); 
    bar(first); // T = int&, decay_t = int.

    const int second = 5;
    foo(second);
    bar(second);  // T = const int, decay_t = int.
    
    const int& third = 7;

    foo(third);
    bar(third); // T = const int&, decay_t = int.

    std::cout << "-------------------------------------" << std::endl;

    demonstrate(first);
    
    std::cout << "-------------------------------------" << std::endl;

    demonstrate(second);
    
    std::cout << "-------------------------------------" << std::endl;

    demonstrate(third);
    
    std::cout << "-------------------------------------" << std::endl;


    return 0;
}