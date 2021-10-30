// clang++ --std=c++20 ref-new.cpp

#include <iostream>
#include <vector>
#include <any>

using namespace std;

template<typename T, size_t I>
struct ArgPack
{
    using TArg = T;
    static const size_t Index = I;
};

template<typename ...TArgs>
struct ArgPacks
{
};

template<typename TArgPacks, typename ...TArgs>
struct MakeArgPacks_;

template<typename ...TPacked>
struct MakeArgPacks_<ArgPacks<TPacked...>>
{
    using Type = ArgPacks<TPacked...>;
};

template<typename ...TPacked, typename T0, typename ...TArgs>
struct MakeArgPacks_<ArgPacks<TPacked...>, T0, TArgs...>
{
    using Type = typename MakeArgPacks_<
        ArgPacks<TPacked..., ArgPack<T0, sizeof...(TPacked)>>,
        TArgs...
        >::Type;
};

template<typename ...TArgs>
using MakeArgPacks = typename MakeArgPacks_<ArgPacks<>, TArgs...>::Type;

template<typename TArgPacks>
struct Call_;

template<typename ...TArgPacks>
struct Call_<ArgPacks<TArgPacks...>>
{
    template<typename TFunc>
    static decltype(auto) Call(TFunc&& func, const vector<any>& arguments)
    {
        return func(
            any_cast<typename TArgPacks::TArg>(arguments[TArgPacks::Index])
            ...);
    }
};

template<typename R, typename ...TArgs>
R Call(R(*func)(TArgs...), const vector<any>& arguments)
{
    return Call_<MakeArgPacks<TArgs...>>::Call(func, arguments);
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
