#include <iostream>
#include <cmath>

// No exemplo abaixo, uma estrutura de nome Point é definida
// Ela possui dois membros: x e y, ambos do tipo primitivo double.
// Essas linhas definem um novo tipo de dado de nome Point, que pode ser utilizado em qualquer lugar do programa. 
// criam duas instâncias de duas variáveis com o novo tipo Point.

struct Point {
    
    double x;
    double y;

};

double calculate_distance(Point p1, Point p2) 
{
    return sqrt(pow(p1.x - p2.x, 2 ) + pow(p1.y - p2.y, 2));
    // pow (3, 2)  = 3² = 9
    // pow (4, 2)  = 4² = 16
    // sqrt(valor) calcula a raiz quadrada
}

int main () {
    //   X  Y
Point p1{4, 5};   
Point p2{2, 10};

    std::cout << "A distancia entre os pontos p1 e p2 é" << calculate_distance(p1, p2) << '\n' << std::endl;

    return 0;

}


// Teorema de pitagoras