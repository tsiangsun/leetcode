// http://stackoverflow.com/questions/15293232/how-to-design-an-algorithm-to-calculate-countdown-style-maths-number-puzzle
/*
The basic idea is to use a stack-based evaluation (see Reverse Polish notation) and convert the viable solutions to infix notation for display purposes only.

If we have N input digits, we'll use (N-1) operators, as each operator is binary.

First we create valid permutations of operands and operators (the selector_ array). A valid permutation is one that can be evaluated without stack underflow and which ends with exactly one value (the result) on the stack. Thus 1 1 + is valid, but 1 + 1 is not.

We test each such operand-operator permutation with every permutation of operands (the values_ array) and every combination of operators (the ops_ array). Matching results are pretty-printed.

Arguments are taken from command line as [-s] <target> <digit>[ <digit>...]. The -s switch prevents exhaustive search, only the first matching result is printed.

(use ./mathpuzzle 348 1 3 7 6 8 3 to get the answer for the original question)
*/
#include <iostream>
#include <vector>
#include <algorithm>
#include <stack>
#include <iterator>
#include <string>
#include <set>

namespace {

enum class Op {
    Add,
    Sub,
    Mul,
    Div,
};

const std::size_t NumOps = static_cast<std::size_t>(Op::Div) + 1;
const Op FirstOp = Op::Add;

using Number = long;

class Evaluator {
    std::vector<Number> values_; // stores our digits/number we can use
    std::vector<Op> ops_; // stores the operators
    std::vector<char> selector_; // used to select digit (0) or operator (1) when evaluating. should be std::vector<bool>, but that's broken

    template <typename T>
    using Stack = std::stack<T, std::vector<T>>;

    // checks if a given number/operator order can be evaluated or not
    bool isSelectorValid() const {
        int numValues = 0;
        for (auto s : selector_) {
            if (s) {
                if (--numValues <= 0) {
                    return false;
                }
            }
            else {
                ++numValues;
            }
        }
        return (numValues == 1);
    }

    // evaluates the current values_ and ops_ based on selector_
    Number eval(Stack<Number> &stack) const {
        auto vi = values_.cbegin();
        auto oi = ops_.cbegin();
        for (auto s : selector_) {
            if (!s) {
                stack.push(*(vi++));
                continue;
            }
            Number top = stack.top();
            stack.pop();
            switch (*(oi++)) {
                case Op::Add:
                    stack.top() += top;
                    break;
                case Op::Sub:
                    stack.top() -= top;
                    break;
                case Op::Mul:
                    stack.top() *= top;
                    break;
                case Op::Div:
                    if (top == 0) {
                        return std::numeric_limits<Number>::max();
                    }
                    Number res = stack.top() / top;
                    if (res * top != stack.top()) {
                        return std::numeric_limits<Number>::max();
                    }
                    stack.top() = res;
                    break;
            }
        }
        Number res = stack.top();
        stack.pop();
        return res;
    }

    bool nextValuesPermutation() {
        return std::next_permutation(values_.begin(), values_.end());
    }

    bool nextOps() {
        for (auto i = ops_.rbegin(), end = ops_.rend(); i != end; ++i) {
            std::size_t next = static_cast<std::size_t>(*i) + 1;
            if (next < NumOps) {
                *i = static_cast<Op>(next);
                return true;
            }
            *i = FirstOp;
        }
        return false;
    }

    bool nextSelectorPermutation() {
        // the start permutation is always valid
        do {
            if (!std::next_permutation(selector_.begin(), selector_.end())) {
                return false;
            }
        } while (!isSelectorValid());
        return true;
    }

    static std::string buildExpr(const std::string& left, char op, const std::string &right) {
        return std::string("(") + left + ' ' + op + ' ' + right + ')';
    }

    std::string toString() const {
        Stack<std::string> stack;
        auto vi = values_.cbegin();
        auto oi = ops_.cbegin();
        for (auto s : selector_) {
            if (!s) {
                stack.push(std::to_string(*(vi++)));
                continue;
            }
            std::string top = stack.top();
            stack.pop();
            switch (*(oi++)) {
                case Op::Add:
                    stack.top() = buildExpr(stack.top(), '+', top);
                    break;
                case Op::Sub:
                    stack.top() = buildExpr(stack.top(), '-', top);
                    break;
                case Op::Mul:
                    stack.top() = buildExpr(stack.top(), '*', top);
                    break;
                case Op::Div:
                    stack.top() = buildExpr(stack.top(), '/', top);
                    break;
            }
        }
        return stack.top();
    }

public:
    Evaluator(const std::vector<Number>& values) :
            values_(values),
            ops_(values.size() - 1, FirstOp),
            selector_(2 * values.size() - 1, 0) {
        std::fill(selector_.begin() + values_.size(), selector_.end(), 1);
        std::sort(values_.begin(), values_.end());
    }

    // check for solutions
    // 1) we create valid permutations of our selector_ array (eg: "1 1 + 1 +",
    //    "1 1 1 + +", but skip "1 + 1 1 +" as that cannot be evaluated
    // 2) for each evaluation order, we permutate our values
    // 3) for each value permutation we check with each combination of
    //    operators
    // 
    // In the first version I used a local stack in eval() (see toString()) but
    // it turned out to be a performance bottleneck, so now I use a cached
    // stack. Reusing the stack gives an order of magnitude speed-up (from
    // 4.3sec to 0.7sec) due to avoiding repeated allocations.  Using
    // std::vector as a backing store also gives a slight performance boost
    // over the default std::deque.
    std::size_t check(Number target, bool singleResult = false) {
        Stack<Number> stack;

        std::size_t res = 0;
        do {
            do {
                do {
                    Number value = eval(stack);
                    if (value == target) {
                        ++res;
                        std::cout << target << " = " << toString() << "\n";
                        if (singleResult) {
                            return res;
                        }
                    }
                } while (nextOps());
            } while (nextValuesPermutation());
        } while (nextSelectorPermutation());
        return res;
    }
};

class Combiner {
    std::vector<Number> values_;
    std::vector<char> splitPoints_;
public:
    Combiner(std::vector<Number> values, std::size_t numSplits) :
            values_(values),
            splitPoints_(values.size() - 1, 0) {
        std::fill(splitPoints_.begin() + (splitPoints_.size() - numSplits), splitPoints_.end(), 1);
    }
    bool next() {
        return std::next_permutation(splitPoints_.begin(), splitPoints_.end());
    }
    std::vector<Number> curr() const {
        std::vector<Number> res;
        res.reserve(splitPoints_.size() + 1);
        auto vi = values_.cbegin();
        auto vend = values_.cend();
        Number tmp = 0;
        for (auto s : splitPoints_) {
            tmp = 10 * tmp + *(vi++);
            if (s) {
                res.push_back(tmp);
                tmp = 0;
            }
        }
        while (vi != vend) {
            tmp = 10 * tmp + *(vi++);
        }
        res.push_back(tmp);
        return res;
    }
};

std::size_t check(Number target, std::vector<Number> values, bool singleResult = false) {
    std::size_t res = 0;
    std::sort(values.begin(), values.end());
    std::set<std::vector<Number>> processedInputs;
    do {
        for (int i = 0; i < values.size(); ++i) {
            Combiner c(values, i);
            do {
                std::vector<Number> combinedValues = c.curr();
                std::sort(combinedValues.begin(), combinedValues.end());
                if (!processedInputs.insert(combinedValues).second) {
                    continue; // already processed
                }
                Evaluator e(combinedValues);
                res += e.check(target, singleResult);
                if (singleResult && res) {
                    return res;
                }
            } while (c.next());
        }
    } while (std::next_permutation(values.begin(), values.end()));
    return res;
}

} // namespace

int main(int argc, const char **argv) {
    int i = 1;
    bool singleResult = false;
    bool allowConcat = false;
    if (argc > i && std::string("-s") == argv[i]) {
        singleResult = true;
        ++i;
    }
    if (argc > i && std::string("-c") == argv[i]) {
        allowConcat = true;
        ++i;
    }
    if (argc < i + 2) {
        std::cerr << argv[0] << " [-s] [-c] <target> <digit>[ <digit>]...\n";
        std::exit(1);
    }
    Number target = std::stoi(argv[i]);
    std::vector<Number> values;
    while (++i <  argc) {
        values.push_back(std::stoi(argv[i]));
    }
    std::size_t res;
    if (allowConcat) {
        res = check(target, values, singleResult);
    }
    else {
        Evaluator e(values);
        res = e.check(target, singleResult);
    }
    if (!singleResult) {
        std::cout << "Number of solutions: " << res << "\n";
    }
    return 0;
}
