#ifndef DIRETORIO_H
#define DIRETORIO_H

#include "FCB.h"
#include <list>
#include <memory>
#include <string>

struct Entrada {
    std::string nome;
    std::shared_ptr<FCB> fcb;
};

//arvore binaria
struct Diretorio {
    std::string nome;
    Diretorio* pai;                              //nullptr apenas na raiz
    std::unique_ptr<Diretorio> primeiroFilho;    //filho da esquerda
    std::unique_ptr<Diretorio> proximoIrmao;     //filho da direita
    std::list<Entrada> arquivos;                 //lista encadeada de entradas

    explicit Diretorio(const std::string& nomeDiretorio, Diretorio* diretorioPai = nullptr)
        : nome(nomeDiretorio), pai(diretorioPai) {}

    void adicionarFilho(std::unique_ptr<Diretorio> novo) {
        std::unique_ptr<Diretorio>* posicao = &primeiroFilho;
        while (*posicao) posicao = &(*posicao)->proximoIrmao;
        *posicao = std::move(novo);
    }

    //desliga o filho da lista de irmaos
    void removerFilho(Diretorio* alvo) {
        std::unique_ptr<Diretorio>* posicao = &primeiroFilho;
        while (*posicao && posicao->get() != alvo) posicao = &(*posicao)->proximoIrmao;
        if (*posicao) *posicao = std::move((*posicao)->proximoIrmao);
    }

    Diretorio* buscarFilho(const std::string& nomeFilho) const {
        for (Diretorio* filho = primeiroFilho.get(); filho; filho = filho->proximoIrmao.get())
            if (filho->nome == nomeFilho) return filho;
        return nullptr;
    }

    Entrada* buscarEntrada(const std::string& nomeArquivo) {
        for (auto& entrada : arquivos)
            if (entrada.nome == nomeArquivo) return &entrada;
        return nullptr;
    }

    bool nomeOcupado(const std::string& nomeItem) {
        return buscarEntrada(nomeItem) != nullptr || buscarFilho(nomeItem) != nullptr;
    }

    bool vazio() const { return !primeiroFilho && arquivos.empty(); }
};

#endif
