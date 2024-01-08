#include "lambda.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <memory>
#include <stack>
#include <string>

#include "parser.hpp"

/* [TODO]
 * - [done] tokenizer ("$x:(%(y)(z)).(*)" -> ['$'Lambda, 'x'Var, ':'Colon, '('LPar, '%'Appl, ...])
 * - [done] parser ([...] -> AbstLambda)
 *      - use Location or some sort
 * - [done] let Location have original lines reference (too tedious to tell reference every time)
 * - [done] (easy) for backward compatibility, convert def_file with new notation to the one written in conventional notation
 *      - e.g.) "$x:A.B" -> "$x:(A).(B)"
 *              "A:*" -> "A\n*"
 *              "implies := ?x:A.B : *" -> "implies\n?x:(A).(B)\n*"
 * [IDEA]
 * - rename: AbstLambda -> Lambda, AbstPi -> Pi
 * - flag: environment variable for definition
 */

std::string to_string(const Kind& k) {
    switch (k) {
        case Kind::Star:
            return "Kind::Star";
        case Kind::Square:
            return "Kind::Square";
        case Kind::Variable:
            return "Kind::Variable";
        case Kind::Application:
            return "Kind::Application";
        case Kind::AbstLambda:
            return "Kind::AbstLambda";
        case Kind::AbstPi:
            return "Kind::AbstPi";
        case Kind::Constant:
            return "Kind::Constant";
        case Kind::Test01:
            return "Kind::Test01";
        case Kind::Test02:
            return "Kind::Test02";
        case Kind::Test03:
            return "Kind::Test03";
        default:
            return "[to_string(const Kind&): TBI]";
    }
}

template <typename PtrType, typename std::enable_if_t<
                                std::is_pointer<PtrType>::value || std::is_same<PtrType, std::unique_ptr<typename PtrType::element_type>>::value || std::is_same<PtrType, std::shared_ptr<typename PtrType::element_type>>::value,
                                int> = 0>
std::ostream& operator<<(std::ostream& os, const PtrType& ptr) {
    return os << ptr->string();
}

std::ostream& operator<<(std::ostream& os, const Kind& k) {
    return os << to_string(k);
}

template <typename T, typename = std::void_t<decltype(std::declval<T>().string())>>
std::ostream& operator<<(std::ostream& os, const T& x) {
    os << x.string();
    return os;
}
