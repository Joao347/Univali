#ifndef DISCO_SIMULADO_H
#define DISCO_SIMULADO_H

#include "FCB.h"
#include <cstddef>
#include <vector>

class DiscoSimulado {
private:
    static constexpr std::size_t TAMANHO_BLOCO = 16;
    static constexpr std::size_t QUANTIDADE_BLOCOS = 256;
    std::vector<char> dados;
    std::vector<bool> ocupado;

public:
    DiscoSimulado();
    bool armazenar(FCB& arquivo, const std::string& conteudo);
    std::string recuperar(const FCB& arquivo) const;
    void liberar(FCB& arquivo);
    void mostrarOcupacao() const;
    void mostrarBlocos(const FCB& arquivo) const;
};

#endif
