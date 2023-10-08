# 自己实现所有STL容器！

比起艰深的STL源码解读，不如自己动手实现STL，更能生动直观地掌握底层技术细节，破除模棱两可心理，了解常见的最佳实践。

## 第一课：Function

视频链接：[BV1yH4y1d74e](https://www.bilibili.com/video/BV1yH4y1d74e)

相关文件：

- [Function.hpp](Function.hpp)
- [test_function.cpp](test_function.cpp)

这是本系列试水的第一课，自己实现std::function容器。本期视频中我们从C语言传统函数指针的痛点入手，介绍了现代C++的仿函数思想，通过cppinsights.io这个在线工具解构了C++11 lambda语法糖的原理，最终，我们借助类型擦除思想，实现了和标准库一样的万能函数容器：std::function。
