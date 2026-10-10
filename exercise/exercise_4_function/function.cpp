#include <iostream>

int calcular_soma(int a, int b){

    int sum = 0;
    for (int i = a; i <= b; ++i)
    {
        sum += i;
    };
    return sum;    
}

int main(){

    int a = 0;
    int b = 0;

    std::cout << "Digite o valor de A:" << '\n';
    std::cin >> a;

    std::cout << "Digite o valor de B:" << '\n';
    std::cin >> b;


    std::cout << "A soma é [" << a << "," << b << "]" << calcular_soma(a, b) << std::endl;

    return 0;
}

