// clang++ --std=c++20 ref-old.cpp

#include <iostream>
#include <vector>
#include <any>
#include <type_traits>

using namespace std;

template<size_t index, typename ...TArgs>
struct FillArguments;

template<size_t index>
struct FillArguments<index>
{
    static void Fill(const vector<any>& arguments) {}
};

template<size_t index, typename T0, typename ...TArgs>
struct FillArguments<index, T0, TArgs...>
{
    static void Fill(const vector<any>& arguments, T0& arg0, TArgs& ...args)
    {
        arg0 = any_cast<T0>(arguments[index]);
        FillArguments<index + 1, TArgs...>::Fill(arguments, args...);
    }
};

template<typename R, typename ...TArgs>
R Call_(R(*func)(TArgs...), const vector<any>& arguments, remove_cvref_t<TArgs>&& ...args)
{
    FillArguments<0, TArgs...>::Fill(arguments, args...);
    return func(args...);
}

template<typename R, typename ...TArgs>
R Call(R(*func)(TArgs...), const vector<any>& arguments)
{
    return Call_(func, arguments, remove_cvref_t<TArgs>{}...);
}

double Add(int a, float b, double c)
{
    return c + b + a;
}

int main()
{
    vector<any> arguments = {10, 20.f, 30.0};
    cout << Call(&Add, arguments) << endl;
    return 0;
}
