//confirmar se é util haver arquivo.hpp, caso contrario apagar e utilizar apenas FCB.hpp

#include <string>

struct Arquivo{
    std::string nome;
    int id;
    std::string tipo;
    std::string localizacao;
    short int tamanho;
    bool protecao;
    int dataCriacao;
    int hora;
};