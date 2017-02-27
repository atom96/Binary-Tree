#ifndef JNP1_7_TREE_H
#define JNP1_7_TREE_H

#include <cstddef>
#include <memory>
#include <vector>
#include <iostream>
#include <algorithm>
#include <functional>
#include <iostream>

template<typename T>
class Node {
private:
    using myNode = std::shared_ptr<Node<T>>;
    using myNodeList = std::vector<myNode>;
    T value;
    myNode left;
    myNode right;
    bool isLazy = false;
    bool isValueSet = false;
    myNode nodeToGetValueFrom = nullptr;
    std::function<T(T)> transform;

    T getValue() {
        if (isLazy && !isValueSet) {
            value = transform(nodeToGetValueFrom->getValue());
            isValueSet = true;
        }
        return value;
    }

    void insertIntoRight(myNode node) {
        if (right == nullptr) right->insertIntoRight(node); else right = node;
    }

public:
    Node(T val) : value(val), left(nullptr), right(nullptr) {
    }

    Node(T val, myNode leftNode, myNode rightNode) : value(val), left(leftNode),
                             right(rightNode) {
    }

    Node(bool lazy, std::function<T(T)> trans, myNode node)
            : left(nullptr), right(nullptr), isLazy(lazy), transform(trans),
              nodeToGetValueFrom(node) {
    }

    Node(myNode leftNode, myNode rightNode,
         bool lazy, std::function<T(T)> trans, myNode node)
            : left(leftNode), right(rightNode), isLazy(lazy), transform(trans),
              nodeToGetValueFrom(node) {
    }

    template<typename Func, typename H>
    H fold(Func operation, H init) {
        getValue();
        return operation(value,
                         left != nullptr ? left->fold(operation, init) : init,
                         right !=
                         nullptr ? right->fold(operation, init) : init);
    }

    template<typename Func, typename H>
    H noValFold(Func operation, H init) const {
        return operation(
                left != nullptr ? left->noValFold(operation, init) : init,
                right != nullptr ? right->noValFold(operation, init) : init);
    }

    template<typename Func>
    myNode filter(Func predicate, myNode you) {
        if (predicate(getValue())) {
            if (right != nullptr)
                right = right->filter(predicate, right);
            if (left != nullptr)
                left = left->filter(predicate, left);
            return you;
        } else {
            if (left == nullptr) (left = right);
            else
                left->insertIntoRight(right);
            return left;
        }
    }

    template<typename Func>
    void apply(Func operation) {
        getValue();
        operation(value);
    };

    myNode makeCopy() {
        return fold([](T val, myNode left,
                       myNode right) {
                        return std::make_shared<Node<T>>(val, left, right);
                    },
                    myNode(nullptr));
    }

    template<typename Func>
    myNode makeLazyCopy(Func trans, myNode you) const {
        return std::make_shared<Node<T>>(
                left != nullptr ? left->makeLazyCopy(trans, left) : nullptr,
                right != nullptr ? right->makeLazyCopy(trans, right) : nullptr,
                true, trans, you);
    }

    myNodeList preorder(myNode you) const {
        myNodeList result;
        result.push_back(you);
        auto leftVecor = left != nullptr ? left->preorder(left) : myNodeList();
        auto rightVector = right != nullptr ? right->preorder(right) : myNodeList();
        result.insert(result.end(), leftVecor.begin(), leftVecor.end());
        result.insert(result.end(), rightVector.begin(), rightVector.end());
        return result;
    }

    myNodeList postorder(myNode you) const {
        auto leftVector = left != nullptr ? left->postorder(left) : myNodeList();
        auto rightVector = right != nullptr ? right->postorder(right) : myNodeList();
        leftVector.insert(leftVector.end(), rightVector.begin(), rightVector.end());
        leftVector.push_back(you);
        return leftVector;
    }

    myNodeList inorder(myNode you) const {
        auto leftVector = left != nullptr ? left->inorder(left) : myNodeList();
        auto rightVector =
                right != nullptr ? right->inorder(right) : myNodeList();
        leftVector.push_back(you);
        leftVector.insert(leftVector.end(), rightVector.begin(), rightVector.end());
        return leftVector;
    }
};

template<typename T>
class Tree {
private:
    using myNode = std::shared_ptr<Node<T>>;
    using myNodeList = std::vector<myNode>;

    myNode root;

public:

    Tree() : root(nullptr) {
    }

    Tree(myNode node) : root(node) {
    }

    static myNodeList inorder(myNode node) {
        return node != nullptr ? node->inorder(node) : myNodeList();
    }

    static myNodeList postorder(myNode node) {
        return node != nullptr ? node->postorder(node) : myNodeList();
    };

    static myNodeList preorder(myNode node) {
        return node != nullptr ? node->preorder(node) : myNodeList();
    };

    static myNode createEmptyNode() {
        return nullptr;
    }

    static myNode createValueNode(T value) {
        return std::make_shared<Node<T>>(value);
    }

    static myNode createValueNode(T value, myNode left, myNode right) {
        return std::make_shared<Node<T>>(value, left, right);
    }

    template<typename Func, typename H>
    H fold(Func operation, H init) {
        return root != nullptr ? root->fold(operation, init) : init;
    }

    template<typename Func, typename H>
    H noValFold(Func operation, H init) const{
        return root != nullptr ? root->noValFold(operation, init) : init;
    }

    template<typename Func>
    Tree<T> filter(Func predicate) {
        if (root != nullptr) {
            auto newRoot = root->makeCopy();
            newRoot = newRoot->filter(predicate, newRoot);
            return Tree(newRoot);
        }
        return Tree();
    }

    template<typename Func>
    Tree<T> map(Func transformer) {
        auto res = Tree<T>(root != nullptr ? root->makeCopy() : nullptr);
        res.apply([&transformer](T &val) { val = transformer(val); },
                  Tree::inorder);
        return res;
    }

    template<typename Func>
    Tree<T> lazy_map(Func transformer) const {
        return Tree<T>(root != nullptr ? root->makeLazyCopy(transformer, root)
                                       : nullptr);
    }

    template<typename Func1, typename Func2>
    T accumulate(Func1 operation, T init, Func2 traversal) {
        apply([&init, &operation](T &value) { init = operation(init, value); },
              traversal);

        return init;
    }

    template<typename Func1, typename Func2>
    void apply(Func1 operation, Func2 traversal) {
        auto nodes = traversal(root);
        for (auto node: nodes) {
            node->apply(operation);
        }

    }

    size_t height() const {
        return noValFold(
                [](size_t h1, size_t h2) { return std::max(h1, h2) + 1; },
                size_t(0));
    }

    size_t size() const {
        return noValFold([](size_t s1, size_t s2) { return s1 + s2 + 1; },
                            size_t(0));
    }

    bool is_bst() {
        auto res = fold([](T val, std::tuple<bool, T, T, bool> left,
                                 std::tuple<bool, T, T, bool> right) {
            static const unsigned int isBst = 0;
            static const unsigned int max = 1;
            static const unsigned int min = 2;
            static const unsigned int isNull = 3;

            bool areSonsBST = std::get<isBst>(left) && std::get<isBst>(right);
            bool isLeftSmaller;
            bool isRightBigger;

            T leftMax;
            T rightMax;
            T leftMin;
            T rightMin;
            T myMax;
            T myMin;

            if (std::get<isNull>(left)) {
                leftMin = val;
                leftMax = val;
                isLeftSmaller = true;
            } else {
                leftMin = std::get<min>(left);
                leftMax = std::get<max>(left);
                isLeftSmaller = leftMax < val;
            }

            if (std::get<isNull>(right)) {
                rightMin = val;
                rightMax = val;
                isRightBigger = true;
            } else {
                rightMin = std::get<min>(right);
                rightMax = std::get<max>(right);
                isRightBigger = rightMin > val;
            }

            if (leftMax > rightMax) {
                myMax = val > leftMax ? val : leftMax;
            } else {
                myMax = val > rightMax ? val : rightMax;
            }

            if (leftMin < rightMin) {
                myMin = val < leftMin ? val : leftMin;
            } else {
                myMin = val > rightMin ? val : rightMin;
            }

            return std::make_tuple(
                    areSonsBST && isLeftSmaller && isRightBigger, myMax, myMin, false);

        }, std::make_tuple(true, T(), T(), true));

        return std::get<0>(res);
    }

    template<typename Func>
    void print(Func traversal) {
        apply([](const T &value) { std::cout << value << " "; }, traversal);
        std::cout << std::endl;
    }

    void print() {
        print(inorder);
    }

};

#endif //JNP1_7_TREE_H
