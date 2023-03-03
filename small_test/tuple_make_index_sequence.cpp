#include <iostream>
#include <tuple>
#include <vector>
#include <string>

// 演示如何根据1个tuple生成另外1个tuple

// 可变模板参数展开示例
// template<typename… CT> -> template<typename CT1, typename CT2>
// sizeof…(CT)  -> 数值2
// size_t… I -> 数值
// void emplace(Args&&...args) -> emplace(Arg1 &&arg1, Arg2 &&arg2)

// std::index_sequence<I…>   
// std::get<I>(m_st)… -> std::get<0>(m_st), std::get<1>(m_st)
// emplace_impl(std::forward<Args>(args)…)  -> callback(std::forward<Arg1>(arg1),std::forward<Arg2>(arg2))

// 从c++17开始，折叠表达式可以将二元运算符作用于所有parameter pack的参数上：
// 更复杂，不是人看的，暂时先不看
// template<typename... T>
// auto foldSum (T... s) {
//   return (... + s);           // ((s1 + s2) + s3) ...
// }

template<typename... CT>
class Foobar {
public:
    Foobar(std::tuple<CT...>& elements) : _elements(elements) {}
    
    // 根据elements生成1个新的tuple
    auto get_firsts() {
        auto f = [](auto& vec) { return vec.front(); };
        return _get_firsts_proxy(f, std::make_index_sequence<sizeof...(CT)>());
    }
private:
    // make_index_sequence一般搭配1个代理函数来使用
    // 把I看成1个常量序列，比如{0,1,2}
    template<typename Func, std::size_t... I>
    auto _get_firsts_proxy(Func &&f, std::index_sequence<I...>) {
        return std::make_tuple(f(std::get<I>(_elements))...);
    }
private:
    std::tuple<CT...> _elements;
};

int main() {
    auto elements = std::make_tuple(
        std::vector<int>{3,4,5},
        std::vector<float>{8.0,9.0,7.0},
        std::vector<std::string>{"hello", "world"}
    );
    using FoobarTuple = Foobar<std::vector<int>,std::vector<float>,std::vector<std::string>>;
    FoobarTuple foobar(elements);
    auto firsts = foobar.get_firsts();
    std::cout << std::get<0>(firsts) << ","
                << std::get<1>(firsts) << ","
                << std::get<2>(firsts) << std::endl;
    return 0;
}
