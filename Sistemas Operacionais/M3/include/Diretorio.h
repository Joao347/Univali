#ifndef DIRETORIO_H
#define DIRETORIO_H

#include "FCB.h"
#include <memory>
#include <string>
#include <vector>

//os diretorios formam uma arvore; cada no possui arquivos e subdiretorios.
struct Diretorio {
    std::string nome;
    Diretorio* pai;
    std::vector<std::unique_ptr<Diretorio>> diretorios;
    std::vector<std::unique_ptr<FCB>> arquivos;

    explicit Diretorio(const std::string& nomeDiretorio, Diretorio* diretorioPai = nullptr)
        : nome(nomeDiretorio), pai(diretorioPai) {}
};

#endif
