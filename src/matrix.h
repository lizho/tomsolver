#pragma once

#include "node.h"

#include <unordered_map>

namespace tomsolver {

class Vec;
class Mat {
public:
    /**
     *
     */
    Mat() noexcept {}

    /**
     *
     */
    Mat(int rows, int cols) noexcept;

    bool Empty() const noexcept;

    int Rows() const noexcept;

    int Cols() const noexcept;

    void Subs(const std::unordered_map<std::string, double> &varValues) noexcept;

    /**
     * 如果rhs和自己的维数不匹配会触发assert。
     */
    Mat operator-(const Mat &rhs) const noexcept;

    std::string ToString() const noexcept;

protected:
    std::vector<std::vector<Node>> data;

    friend Mat Jacobian(const Vec &equations, const std::vector<std::string> &vars) noexcept;
};

Mat operator-(const Mat &lhs, const Mat &rhs) noexcept;

class Vec : public Mat {
public:
    /**
     * 使用初始化列表构造。注意列表内的对象将被强行移动至Vec内部。
     */
    Vec(const std::initializer_list<Node> &lst) noexcept;

    Node &operator[](std::size_t index) noexcept;

    const Node &operator[](std::size_t index) const noexcept;
};

Vec operator-(const Vec &lhs, const Vec &rhs) noexcept;

Mat Jacobian(const Vec &equations, const std::vector<std::string> &vars) noexcept;

} // namespace tomsolver
