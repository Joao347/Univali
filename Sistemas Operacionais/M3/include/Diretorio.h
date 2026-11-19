#ifndef DIRETORIO_H
#define DIRETORIO_H

#include "FCB.h"
#include <list>
#include <memory>
#include <string>

//uma entrada de diretorio liga um NOME a um FCB. Separar o nome do FCB permite
//links fisicos: duas entradas (talvez em diretorios diferentes) apontando para o
//mesmo FCB. Por isso o FCB fica em um shared_ptr.
struct Entrada {
    std::string nome;
    std::shared_ptr<FCB> fcb;
};

//arvore binaria "primeiro filho / proximo irmao": cada no tem so dois filhos na
//arvore binaria -- o primeiro subdiretorio (esquerda) e o proximo diretorio do
//mesmo nivel (direita). Assim uma pasta pode ter qualquer numero de subpastas.
struct Diretorio {
    std::string nome;
    Diretorio* pai;                              //nullptr apenas na raiz
    std::unique_ptr<Diretorio> primeiroFilho;    //filho da esquerda
    std::unique_ptr<Diretorio> proximoIrmao;     //filho da direita
    std::list<Entrada> arquivos;                 //lista encadeada de entradas

    explicit Diretorio(const std::string& nomeDiretorio, Diretorio* diretorioPai = nullptr)
        : nome(nomeDiretorio), pai(diretorioPai) {}

    //insere o novo subdiretorio no fim da lista de irmaos (mantem a ordem de criacao)
    void adicionarFilho(std::unique_ptr<Diretorio> novo) {
        std::unique_ptr<Diretorio>* posicao = &primeiroFilho;
        while (*posicao) posicao = &(*posicao)->proximoIrmao;
        *posicao = std::move(novo);
    }

    //desliga o filho da lista de irmaos; o unique_ptr libera o no
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
