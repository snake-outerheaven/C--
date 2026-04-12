#include <iostream>

// Em C++, usamos aspas simples (' ') para representar um único caractere (char),
// como 's' ou 'n'. Já aspas duplas (" ") representam uma string (texto),
// como "s", que na verdade é um conjunto de caracteres (const char*).
// Por isso, não podemos comparar um char com uma string diretamente.
// O correto é usar 's' e 'n' ao trabalhar com variáveis do tipo char.


int main(){
    
    char answer = '\0';

    while (answer != 's' && answer != 'n') {
        std::cout << "Você quer aprender C++ [s/n]: ";
        std::cin >> answer;
    }

    if (answer == 's') {
        std::cout << "Continue assim\n";
    
    } else {
        std::cout << "Moggado \n";
    }

    return 0;
}