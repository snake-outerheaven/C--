    #include <iostream>
    #include <stdexcept>

    int calcular_soma(int a, int b){
        return a + b;
    }

    int calcular_sub (int a, int b){
        return a - b;
    }

    int calcular_multiplicao(int a, int b){
        return a * b;
    }

    #include <stdexcept>

    int calcular_div(int a, int b){
        if (b == 0) {
        throw std::runtime_error("Erro: Divisao por zero!"); // Sinaliza o erro [9] 
    }
    return a / b;
}



    int main(){


        int a = 0;
        int b = 0;

        std::cout << "Digite o valor para A:" << '\n';
        std::cin >> a;

        std::cout << "Digite o valor para B:" << '\n';
        std::cin >> b;

        
    std::cout << "Valores Utilizados [" << a << ", " << b << "]" << '\n';

    std::cout << "=============SOMA=============" << '\n';
    std::cout << calcular_soma(a, b) << '\n' << std::endl;

    std::cout << "=============SUBTRAÇÃO=============" << '\n';
    std::cout << calcular_sub(a, b) << '\n' << std::endl;

    std::cout << "=============MULTIPLICAÇÃO=============" << '\n';
    std::cout << calcular_multiplicao(a, b) << '\n' << std::endl;

    std::cout << "=============DIVISÃO=============" << '\n';
 
    
        

        return 0;
    }


