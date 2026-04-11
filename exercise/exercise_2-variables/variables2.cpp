#include <iostream>

int main() {

    int age = 0;
    double const price = 20.0;
    std::cout << "Insira sua idade";
    std::cin >> age;
    std::cout << "Sua Idade é: " << age << '\n';

    if (age <= 17) {
        std::cout << "Você é menor de idade \n";
    }
    else {
        std::cout << "Você é maior de idade \n";
    }
    return 0;
}