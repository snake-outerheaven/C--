#include <iostream>

int main()

{
    for (int i = 1; i <= 100; i = i + 1) {
        std::cout << "Insira o valor de i";
        std::cin >> i;
        std::cout << "Iteração #" << i << '\n' << "Você deseja continuar continuar? [s/n]";
    

char op;

std::cin >> op;
if (op == 'n')
    break;

    std::cout << "Incrementando i  como:" << i << std::endl;


    return 0;

    }    


}

