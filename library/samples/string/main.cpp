
#include <safememory/string.h>
#include <fmt/printf.h>
#include <fmt/format.h>
#include <safememory/string_format.h>

#include <iostream>

namespace sm = safememory;

void someFunc(const sm::string& str) {}

int main() {

    sm::string_literal lit = "hello!";
    sm::string s = sm::string_literal("hello!");
    sm::string s2("hello!");
    safememory::string s3 = sm::string{"hello!"}; //error

    s3 = "w3";

    if(s3 == "world")
        someFunc(sm::string{"hello!"});

    s.append("! - ");

    for(auto it = s.begin(); it != s.end(); ++it)
        s2 += *it;

    for(auto its = s2.begin(); its != s2.end(); ++its) {
        s += *its;
    }

//    fmt::print("{}\n", sm::to_string(42)); //sm::string_literal
    fmt::print("{}\n", lit); //sm::string_literal
    fmt::print("{}\n", s2); //sm::string

    std::cout << lit << std::endl << s2 << std::endl;

    fmt::printf(s2.c_str());

    s2.erase(s2.cbegin() + 7, s2.cend());
    fmt::printf(s2.c_str());

    fmt::printf("done\n");
    return 0;
}



