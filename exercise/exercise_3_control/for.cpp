#include <iostream>

//No código abaixo, o laço "for" itera com valores
// de i = a até valor de i = b, somando 1 ao valor de i a cada iteração.
// Uma forma alternativa e mais compacta de escrever i = i + 1 seria escrever ++i.

int main (){

    int a = 0;
    int b = 0;

    std::cout << "Digite o valor de A: \n";
    std::cin >> a;

    std::cout << "Digite o Valor de B: \n";
    std::cin >> b;

    int sum = 0;

    // Esse loop percorre todos os números de 'a' até 'b', um por vez.
    // A variável 'i' começa em 'a' e vai aumentando de 1 em 1 até chegar em 'b'.
    // Em cada repetição, o valor atual de 'i' é somado na variável 'sum'.
    // No final, 'sum' terá a soma de todos os números entre 'a' e 'b'.

        for (int i = a; i <= b; i = i + 3){
            sum += i;
        }

    std::cout << "Sum [" << a  << "," << b << "] = " << sum << std::endl;

    return 0;

} 