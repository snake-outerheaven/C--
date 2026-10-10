#include <iostream>
#include <cmath>

// considere o seguinte problema:
// Você está desenvolvendo um jogo de plataforma, e precisa representar o personagem em um espaço bidimensional.
// O personagem possui uma posição e um número de vidas.
// Utilizando a estrutura anterior para representar a posição, poderia-se escrever:

using Life = unsigned int;

struct Player {
    Point position;
    Life Life;
};

// Aqui a palavra reservada using para criar um type alias (um "apelido" para um tipo qualquer). 
// Basicamente, dá-se um novo nome para um tipo que já existe. No exemplo acima, Life será equivalente ao tipo unsigned int, ou seja, um inteiro não negativo.
// Deste modo, toda vez que usarmos o tipo Lif por trás dos panos, um unsigned int.e, estaremos criando,