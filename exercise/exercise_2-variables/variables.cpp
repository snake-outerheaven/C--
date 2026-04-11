#include <string>

int main(){
    int some_int = 8;
    float some_float = 9.2;
    char some_char = 'a';
    bool some_bool = true;

    std::string some_string = "First string";
    auto some_string_2 = std::string  ("Second String");
    using namespace std::string_literals;
    auto some_string_3 = "Third string"s; // note o 's' após a string!

    return 0;
}