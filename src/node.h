#pragma once
#include "error_type.h"
#include "math_operator.h"

#include <iostream>

#include <vector>
#include <type_traits>
#include <memory>
#include <string>
#include <stack>
#include <cassert>

namespace tomsolver {

enum class NodeType { NUMBER, OPERATOR, VARIABLE };

namespace internal {
struct NodeImpl;
}

/**
 * 表达式节点。
 */
using Node = std::unique_ptr<internal::NodeImpl>;

namespace internal {

/**
 * 单个节点的实现。通常应该以std::unique_ptr包裹。
 */
struct NodeImpl {

    NodeImpl(NodeType type, MathOperator op, double value, std::string varname) noexcept
        : type(type), op(op), value(value), varname(varname), parent(nullptr) {}

    NodeImpl(const NodeImpl &rhs) noexcept;
    NodeImpl &operator=(const NodeImpl &rhs) noexcept;

    NodeImpl(NodeImpl &&rhs) noexcept;
    NodeImpl &operator=(NodeImpl &&rhs) noexcept;

    ~NodeImpl();

    /**
     * 把整个节点以中序遍历的顺序输出为字符串。
     * 例如：
     *      Node n = (Var("a") + Num(1)) * Var("b");
     *   则
     *      n->ToString() == "(a+1.000000)*b"
     */
    std::string ToString() const noexcept;

    /**
     * 计算出整个表达式的数值。
     * @exception runtime_error 如果有变量存在，则无法计算
     * @exception MathError 不符合定义域, 除0等情况。
     */
    double Vpa() const;

    /**
     * 检查整个节点数的parent指针是否正确。
     */
    void CheckParent() const noexcept;

private:
    NodeType type;
    MathOperator op;
    double value;
    std::string varname;
    const NodeImpl *parent;
    std::unique_ptr<NodeImpl> left, right;

    /**
     * 本节点如果是OPERATOR，检查操作数数量和left, right指针是否匹配。
     */
    void CheckOperatorNum() const noexcept;

    /**
     * 节点转string。仅限本节点，不含子节点。
     */
    std::string NodeToStr() const noexcept;

    // TODO: to non-recursively
    void ToStringRecursively(std::string &output) const noexcept;

    // TODO: to non-recursively
    void ToStringNonRecursively(std::string &output) const noexcept;

    /**
     * 计算表达式数值。递归实现。
     * @exception runtime_error 如果有变量存在，则无法计算
     * @exception MathError 不符合定义域, 除0等情况。
     */
    double VpaRecursively() const;

    /**
     * 计算表达式数值。非递归实现。
     * 性能弱于递归实现。但不会导致栈溢出。
     * 根据benchmark，生成一组含4000个随机四则运算节点的表达式，生成1000次，Release下测试耗时3000ms。递归实现耗时2500ms。
     * 粗略计算，即 1333 ops/ms。
     * @exception runtime_error 如果有变量存在，则无法计算
     * @exception MathError 不符合定义域, 除0等情况。
     */
    double VpaNonRecursively() const;

    /**
     * 释放整个节点树，除了自己。
     * 实际是二叉树的非递归后序遍历。
     */
    void Release() noexcept;

    friend std::unique_ptr<NodeImpl> CloneRecursively(const std::unique_ptr<NodeImpl> &rhs) noexcept;
    friend std::unique_ptr<NodeImpl> CloneNonRecursively(const std::unique_ptr<NodeImpl> &rhs) noexcept;

    friend void CopyOrMoveTo(NodeImpl *parent, std::unique_ptr<NodeImpl> &child,
                             std::unique_ptr<NodeImpl> &&n1) noexcept;
    friend void CopyOrMoveTo(NodeImpl *parent, std::unique_ptr<NodeImpl> &child,
                             const std::unique_ptr<NodeImpl> &n1) noexcept;

    friend std::ostream &operator<<(std::ostream &out, const std::unique_ptr<NodeImpl> &n) noexcept;

    template <typename T>
    friend std::unique_ptr<NodeImpl> UnaryOperator(MathOperator op, T &&n) noexcept;

    template <typename T1, typename T2>
    friend std::unique_ptr<NodeImpl> BinaryOperator(MathOperator op, T1 &&n1, T2 &&n2) noexcept;
};

std::unique_ptr<NodeImpl> CloneRecursively(const std::unique_ptr<NodeImpl> &rhs) noexcept;

std::unique_ptr<NodeImpl> CloneNonRecursively(const std::unique_ptr<NodeImpl> &rhs) noexcept;

/**
 * 对于一个节点n和另一个节点n1，把n1移动到作为n的子节点。
 * 用法：CopyOrMoveTo(n->parent, n->left, std::forward<T>(n1));
 */
void CopyOrMoveTo(NodeImpl *parent, std::unique_ptr<NodeImpl> &child, std::unique_ptr<NodeImpl> &&n1) noexcept;

/**
 * 对于一个节点n和另一个节点n1，把n1整个拷贝一份，把拷贝的副本设为n的子节点。
 * 用法：CopyOrMoveTo(n->parent, n->left, std::forward<T>(n1));
 */
void CopyOrMoveTo(NodeImpl *parent, std::unique_ptr<NodeImpl> &child, const std::unique_ptr<NodeImpl> &n1) noexcept;

/**
 * 重载std::ostream的<<操作符以输出一个Node节点。
 */
std::ostream &operator<<(std::ostream &out, const std::unique_ptr<internal::NodeImpl> &n) noexcept;

template <typename T>
std::unique_ptr<NodeImpl> UnaryOperator(MathOperator op, T &&n) noexcept {
    auto ret = std::make_unique<NodeImpl>(NodeType::OPERATOR, op, 0, "");
    CopyOrMoveTo(ret.get(), ret->left, std::forward<T>(n));
    return ret;
}

template <typename T1, typename T2>
std::unique_ptr<NodeImpl> BinaryOperator(MathOperator op, T1 &&n1, T2 &&n2) noexcept {
    auto ret = std::make_unique<NodeImpl>(NodeType::OPERATOR, op, 0, "");
    CopyOrMoveTo(ret.get(), ret->left, std::forward<T1>(n1));
    CopyOrMoveTo(ret.get(), ret->right, std::forward<T2>(n2));
    return ret;
}

} // namespace internal

// TODO: to non-recursively
Node Clone(const Node &rhs) noexcept;

/**
 * 对节点进行移动。等同于std::move。
 */
Node Move(Node &rhs) noexcept;

/**
 * 新建一个数值节点。
 */
std::unique_ptr<internal::NodeImpl> Num(double num) noexcept;

/**
 * 返回变量名是否有效。（只支持英文数字或者下划线，第一个字符必须是英文或者下划线）
 */
bool VarNameIsLegal(const std::string &varname) noexcept;

/**
 * 新建一个变量节点。
 * @exception runtime_error 名字不合法
 */
std::unique_ptr<internal::NodeImpl> Var(const std::string &varname);

template <typename T1, typename T2>
std::unique_ptr<internal::NodeImpl> operator+(T1 &&n1, T2 &&n2) noexcept {
    return internal::BinaryOperator(MathOperator::MATH_ADD, std::forward<T1>(n1), std::forward<T2>(n2));
}

template <typename T>
std::unique_ptr<internal::NodeImpl> &operator+=(std::unique_ptr<internal::NodeImpl> &n1, T &&n2) noexcept {
    n1 = internal::BinaryOperator(MathOperator::MATH_ADD, std::move(n1), std::forward<T>(n2));
    return n1;
}

template <typename T1, typename T2>
std::unique_ptr<internal::NodeImpl> operator-(T1 &&n1, T2 &&n2) noexcept {
    return internal::BinaryOperator(MathOperator::MATH_SUB, std::forward<T1>(n1), std::forward<T2>(n2));
}

template <typename T>
std::unique_ptr<internal::NodeImpl> &operator-=(std::unique_ptr<internal::NodeImpl> &n1, T &&n2) noexcept {
    n1 = internal::BinaryOperator(MathOperator::MATH_SUB, std::move(n1), std::forward<T>(n2));
    return n1;
}

template <typename T1, typename T2>
std::unique_ptr<internal::NodeImpl> operator*(T1 &&n1, T2 &&n2) noexcept {
    return internal::BinaryOperator(MathOperator::MATH_MULTIPLY, std::forward<T1>(n1), std::forward<T2>(n2));
}

template <typename T>
std::unique_ptr<internal::NodeImpl> &operator*=(std::unique_ptr<internal::NodeImpl> &n1, T &&n2) noexcept {
    n1 = internal::BinaryOperator(MathOperator::MATH_MULTIPLY, std::move(n1), std::forward<T>(n2));
    return n1;
}

template <typename T1, typename T2>
std::unique_ptr<internal::NodeImpl> operator/(T1 &&n1, T2 &&n2) noexcept {
    return internal::BinaryOperator(MathOperator::MATH_DIVIDE, std::forward<T1>(n1), std::forward<T2>(n2));
}

template <typename T>
std::unique_ptr<internal::NodeImpl> &operator/=(std::unique_ptr<internal::NodeImpl> &n1, T &&n2) noexcept {
    n1 = internal::BinaryOperator(MathOperator::MATH_DIVIDE, std::move(n1), std::forward<T>(n2));
    return n1;
}

//
// class Matrix {
// public:
// private:
//};

} // namespace tomsolver
