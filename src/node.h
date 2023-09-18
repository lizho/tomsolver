#pragma once
#include "error_type.h"
#include "math_operator.h"

#include <iostream>

#include <vector>
#include <type_traits>
#include <memory>
#include <string>
#include <cassert>

namespace tomsolver {

enum class NodeType { NUMBER, OPERATOR, VARIABLE, FUNCTION };

/* 单个元素 */
struct NodeImpl {

    NodeImpl(NodeType type, MathOperator op, double value, std::string varname) noexcept
        : type(type), op(op), value(value), varname(varname), parent(nullptr) {}

    NodeImpl(const NodeImpl &rhs) noexcept {
        operator=(rhs);
    }

    NodeImpl &operator=(const NodeImpl &rhs) noexcept {
        type = rhs.type;
        op = rhs.op;
        value = rhs.value;
        varname = rhs.varname;
        parent = rhs.parent;
        left = CopyTree(rhs.left);
        right = CopyTree(rhs.right);
        return *this;
    }

    NodeImpl(NodeImpl &&rhs) noexcept {
        operator=(std::move(rhs));
    }

    NodeImpl &operator=(NodeImpl &&rhs) noexcept {
        type = rhs.type;
        op = rhs.op;
        value = rhs.value;
        varname = rhs.varname;
        parent = rhs.parent;
        left = std::move(rhs.left);
        right = std::move(rhs.right);

        rhs.parent = nullptr;
        return *this;
    }

    ~NodeImpl() = default;

    /**
     * 把整个节点以中序遍历的顺序输出为字符串。
     */
    std::string ToString() const noexcept {
        std::string ret;
        TraverseInOrder(ret);
        return ret;
    }

    /**
     * 迭代计算
     * @exception
     */
    double Vpa() {
        double l=0, r=0;
        if (left != nullptr)
            l = left->Vpa();
        if (right != nullptr)
            r = right->Vpa();

        if (type == NodeType::NUMBER) {
            return value;
        }

        if (type == NodeType::VARIABLE) {
            throw std::runtime_error("has variable. can not calculate to be a number");
        }

        if (type == NodeType::OPERATOR) {
            assert((GetOperatorNum(op) == 1 && left != nullptr && right == nullptr) ||
                   (GetOperatorNum(op) == 2 && left != nullptr && right != nullptr));
            return Calc(op, l, r);
        }

        throw std::runtime_error("unsupported node type");
        return std::numeric_limits<double>::quiet_NaN();
    }

private:
    NodeType type;
    MathOperator op;
    double value;
    std::string varname;
    NodeImpl *parent;
    std::unique_ptr<NodeImpl> left, right;

    std::unique_ptr<NodeImpl> CopyTree(const std::unique_ptr<NodeImpl> &rhs) const noexcept {
        if (rhs == nullptr) {
            return nullptr;
        }
        auto ret = std::make_unique<NodeImpl>(rhs->type, rhs->op, rhs->value, rhs->varname);
        ret->left = CopyTree(rhs->left);
        ret->right = CopyTree(rhs->right);
        return ret;
    }

    /**
     * 节点转string。仅限本节点，不含子节点。
     */
    std::string NodeToStr() const noexcept {
        switch (type) {
        case NodeType::NUMBER:
            return std::to_string(value);
        case NodeType::VARIABLE:
            return varname;
        case NodeType::OPERATOR:
            return MathOperatorToStr(op);
        }
        assert(0 && "unexpected NodeType. maybe this is a bug.");
        return "";
    }

    void TraverseInOrder(std::string &output) const noexcept {
        switch (type) {
        case NodeType::NUMBER:
        case NodeType::VARIABLE:
            output += NodeToStr();
            return;
        }

        int has_parenthesis = 0;
        if (GetOperatorNum(op) == 1) //一元运算符：函数和取负
        {
            if (type == NodeType::FUNCTION) {
                output += NodeToStr() + "(";
                has_parenthesis = 1;
            } else {
                output += "(" + NodeToStr();
                has_parenthesis = 1;
            }
        } else {
            //非一元运算符才输出，即一元运算符的输出顺序已改变
            if (type == NodeType::OPERATOR) //本级为运算符
                if (parent != nullptr)
                    if ((GetOperatorNum(parent->op) == 2 && //父运算符存在，为二元，
                         (Rank(parent->op) > Rank(op)       //父级优先级高于本级->加括号

                          || ( //两级优先级相等
                                 Rank(parent->op) == Rank(op) &&
                                 (
                                     //本级为父级的右子树 且父级不满足结合律->加括号
                                     (InAssociativeLaws(parent->op) == false && this == parent->right.get()) ||
                                     //两级都是右结合
                                     (InAssociativeLaws(parent->op) == false && IsLeft2Right(op) == false)))))

                        //||

                        ////父运算符存在，为除号，且本级为分子，则添加括号
                        //(now->parent->eOperator == MATH_DIVIDE && now == now->parent->right)
                    ) {
                        output += "(";
                        has_parenthesis = 1;
                    }
        }

        if (left != nullptr) //左遍历
        {
            left->TraverseInOrder(output);
        }

        if (GetOperatorNum(op) != 1) //非一元运算符才输出，即一元运算符的输出顺序已改变
        {
            output += NodeToStr();
        }

        if (right != nullptr) //右遍历
        {
            right->TraverseInOrder(output);
        }

        //回到本级时补齐右括号，包住前面的东西
        if (has_parenthesis) {
            output += ")";
        }
    }

    friend void CopyOrMoveTo(NodeImpl *parent, std::unique_ptr<NodeImpl> &child, std::unique_ptr<NodeImpl> &&n1) noexcept;
    friend void CopyOrMoveTo(NodeImpl *parent, std::unique_ptr<NodeImpl> &child, const std::unique_ptr<NodeImpl> &n1) noexcept;

    friend std::ostream &operator<<(std::ostream &out, const std::unique_ptr<NodeImpl> &n) noexcept;

    template <typename T1, typename T2>
    friend std::unique_ptr<NodeImpl> OperatorSome(MathOperator op, T1 &&n1, T2 &&n2) noexcept;
};

using Node = std::unique_ptr<NodeImpl>;

/**
 * 重载std::ostream的<<操作符以输出一个Node节点。
 */
std::ostream &operator<<(std::ostream &out, const std::unique_ptr<NodeImpl> &n) noexcept {
    out << n->ToString();
    return out;
}

/**
 * 新建一个数值节点。
 */
std::unique_ptr<NodeImpl> Num(double num) noexcept {
    return std::make_unique<NodeImpl>(NodeType::NUMBER, MathOperator::MATH_NULL, num, "");
}

/**
 * 有效性检查（返回0则出现异常字符）
 */
// bool IsLegal(char c) {
//	if (isDoubleChar(c)) return true;
//	if (isBaseOperator(c)) return true;
//	if (IsCharAlpha(c) || c == '_') return true;
//	return false;
//}

bool VarNameIsLegal(const std::string &varname) noexcept {
    if (varname.empty()) {
        return false;
    }
    char c = varname[0];
    if (!isalpha(c) && c != '_') {
        return false;
    }
    auto n = varname.size();
    for (size_t i = 1; i < n; ++i) {
        c = varname[i];
        if (isalnum(c) || c == '_') {
            continue;
        }
        return false;
    }
    return true;
}

/**
 * 新建一个变量节点。
 * @exception runtime_error 名字不合法
 */
std::unique_ptr<NodeImpl> Var(const std::string &varname) {
    if (!VarNameIsLegal(varname)) {
        throw std::runtime_error("Illegal varname: " + varname);
    }
    return std::make_unique<NodeImpl>(NodeType::VARIABLE, MathOperator::MATH_NULL, 0, varname);
}

/**
 * 对于一个节点n和另一个节点n1，把n1移动到作为n的子节点。
 */
void CopyOrMoveTo(NodeImpl *parent, std::unique_ptr<NodeImpl> &child, std::unique_ptr<NodeImpl> &&n1) noexcept {
    n1->parent = parent;
    child = std::move(n1);
}

/**
 * 对于一个节点n和另一个节点n1，把n1整个拷贝一份，把拷贝的副本设为n的子节点。
 */
void CopyOrMoveTo(NodeImpl *parent, std::unique_ptr<NodeImpl> &child, const std::unique_ptr<NodeImpl> &n1) noexcept {
    auto n1Clone = std::unique_ptr<NodeImpl>(new NodeImpl(*n1));
    n1Clone->parent = parent;
    child = std::move(n1Clone);
}

template <typename T1, typename T2>
std::unique_ptr<NodeImpl> OperatorSome(MathOperator op, T1 &&n1, T2 &&n2) noexcept {
    auto ret = std::make_unique<NodeImpl>(NodeType::OPERATOR, op, 0, "");
    CopyOrMoveTo(ret.get(), ret->left, std::forward<T1>(n1));
    CopyOrMoveTo(ret.get(), ret->right, std::forward<T2>(n2));
    return ret;
}

template <typename T1, typename T2>
std::unique_ptr<NodeImpl> operator+(T1 &&n1, T2 &&n2) noexcept {
    return OperatorSome(MathOperator::MATH_ADD, std::forward<T1>(n1), std::forward<T2>(n2));
}

template <typename T>
std::unique_ptr<NodeImpl>& operator+=(std::unique_ptr<NodeImpl> &n1, T &&n2) noexcept {
    n1 = OperatorSome(MathOperator::MATH_ADD, n1, std::forward<T>(n2));
    return n1;
}

template <typename T1, typename T2>
std::unique_ptr<NodeImpl> operator-(T1 &&n1, T2 &&n2) noexcept {
    return OperatorSome(MathOperator::MATH_SUB, std::forward<T1>(n1), std::forward<T2>(n2));
}

template <typename T>
std::unique_ptr<NodeImpl> &operator-=(std::unique_ptr<NodeImpl> &n1, T &&n2) noexcept {
    n1 = OperatorSome(MathOperator::MATH_SUB, n1, std::forward<T>(n2));
    return n1;
}

template <typename T1, typename T2>
std::unique_ptr<NodeImpl> operator*(T1 &&n1, T2 &&n2) noexcept {
    return OperatorSome(MathOperator::MATH_MULTIPLY, std::forward<T1>(n1), std::forward<T2>(n2));
}

template <typename T>
std::unique_ptr<NodeImpl> &operator*=(std::unique_ptr<NodeImpl> &n1, T &&n2) noexcept {
    n1 = OperatorSome(MathOperator::MATH_MULTIPLY, n1, std::forward<T>(n2));
    return n1;
}

template <typename T1, typename T2>
std::unique_ptr<NodeImpl> operator/(T1 &&n1, T2 &&n2) noexcept {
    return OperatorSome(MathOperator::MATH_DIVIDE, std::forward<T1>(n1), std::forward<T2>(n2));
}

template <typename T>
std::unique_ptr<NodeImpl> &operator/=(std::unique_ptr<NodeImpl> &n1, T &&n2) noexcept {
    n1 = OperatorSome(MathOperator::MATH_DIVIDE, n1, std::forward<T>(n2));
    return n1;
}


//
// class Matrix {
// public:
// private:
//};

} // namespace tomsolver
