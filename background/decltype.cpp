#include <iostream>
#include <utility>
#include <type_traits>

int x = 42;

// Return type: int, value category: prvalue
auto bar() {
    return x;
}

// Return type: int&, value category: lvalue
auto& bar_ref() {
    return x;
}

// Return type: int. 'auto' loses reference semantics
auto bar_ref_bad_caller() {
    return bar_ref();  // reference is lost
}

// Return type: int&. decltype preserves reference semantics
decltype(auto) bar_ref_good_caller() {
    return bar_ref();  // equivalent to: return decltype(bar_ref());
}

// Return type: int (prvalue)
decltype(auto) baz() {
    return x;
}

// Return type: int& because decltype((x)) treats (x) as an expression
decltype(auto) baz_expression() {
    return (x); 
}

 // Return type: int (prvalue)
int sum(int a, int b) { 
    return a + b;   
}

int& get_int_ref() {
    static int x = 10; // Without 'static' would stumble into dangling reference error.
    return x;        // returns int& (lvalue)
}


// Instance 1: Working with lvalue int. Int x = 10.
template <typename Function, typename... Args>                      
decltype(auto) logger(const Function& fn, Args&&... args) {         
    std::cout << "Logger has been set up. Working." << std::endl;   
    decltype(auto) result = fn(std::forward<Args>(args)...);      
    std::cout << "Logger has been turned off. End." << std::endl;
    return result; // preserves the value category of fn's return
}

void foo(int& value) { std::cout << "lvalue: " << value << std::endl;}

void foo(int&& value) { std::cout << "rvalue: " << value << std::endl;}

template <typename T>
void wrapper(T&& value)
{
    // There is the only one branch at compile time, the second branch is being thrown aside.
    if constexpr (std::is_lvalue_reference<T>::value) {
        std::cout << "Within wrapper we are working with lvalue-reference!" << std::endl;
    }
    else {
        std::cout << "Within wrapper we are working with rvalue-reference!" << std::endl;
    }
    foo(std::forward<T>(value));
}

int main() {
    int x = 10; 

    // wrapper(x): argument is lvalue int
    // T is deduced as int& (due to forwarding reference rules)
    // T&& becomes int& &&, which collapses to int&
    // foo(std::forward<T>(value)) becomes foo(x) -> calls foo(int&)
    wrapper(x); 

    // wrapper(20): argument is prvalue int
    // T is deduced as int (not int&&!)
    // T&& becomes int&&
    // foo(std::forward<T>(value)) becomes foo(20) -> calls foo(int&&)
    wrapper(20); 

    std::cout << "------------------------------------------------" << std::endl;

    // logger(sum, 2, 3) calls sum(2,3) which returns a prvalue int
    // Therefore logger returns a prvalue int (not a reference)
    // Hence, the 'a' cannot refer to the 'logger', as long as 'a' is non-const reference. 
    int a = logger(sum,2,3);
    std::cout << "A equals to: " << a << std::endl;

    int a2 = logger([]() { return sum(2, 3); });
    std::cout << "A2 equals to: " << a << std::endl;

    int &b = logger(get_int_ref);
    b+=12;
    std::cout << "B equals to: " << b << std::endl;

    std::cout << "------------------------------------------------" << std::endl;

    std::cout << std::boolalpha;
    
    // is_same compares types exactly, including references
    std::cout << "int&& vs int: " 
              << std::is_same<int&&, int>::value << std::endl;  // false (rvalue ref ≠ value)
    
    std::cout << "int vs int: " 
              << std::is_same<int, int>::value << std::endl;    // true (identical types)
    
    // remove_reference<int&>::type strips off the reference, yielding int
    std::cout << "remove_reference<int&>::type vs int: " 
              << std::is_same<std::remove_reference<int&>::type, int>::value 
              << std::endl;  // true (reference removed)

    return 0;
}