#ifndef DISCO_SIMULADO_H
#define DISCO_SIMULADO_H

#include "FCB.h"
#include <array>
#include <cstddef>
#include <string>

//dois metodos de alocacao podem ser escolhidos durante a execucao (comando alloc).
enum class ModoAlocacao {
    Indexada,  //qualquer bloco livre serve; o FCB guarda a lista de indices
    Contigua   //o arquivo precisa de blocos vizinhos (first-fit)
};

enum class ResultadoDisco {
    Ok,
    SemEspaco,    //menos blocos livres do que o necessario
    Fragmentado   //ha blocos livres, mas nenhuma sequencia contigua (so na alocacao contigua)
};

//o "disco" e um array de caracteres de 4096 bytes dividido em 256 blocos de 16 bytes.
class DiscoSimulado {
public:
    static constexpr std::size_t TAMANHO_BLOCO = 16;
    static constexpr std::size_t QUANTIDADE_BLOCOS = 256;
    static constexpr std::size_t TAMANHO_DISCO = TAMANHO_BLOCO * QUANTIDADE_BLOCOS;

    DiscoSimulado();

    //grava o conteudo no arquivo, substituindo o anterior. Os blocos novos sao
    //escolhidos antes de liberar os antigos: se faltar espaco, nada muda.
    ResultadoDisco armazenar(FCB& arquivo, const std::string& conteudo);
    std::string recuperar(const FCB& arquivo) const;
    void liberar(FCB& arquivo);

    std::size_t blocosLivres() const;
    static std::size_t blocosNecessarios(std::size_t bytes);
    ModoAlocacao modo() const { return modoAtual; }
    void definirModo(ModoAlocacao novo) { modoAtual = novo; }
    static std::string modoTexto(ModoAlocacao modo);

    void mostrarOcupacao() const;
    void mostrarBlocos(const FCB& arquivo, bool mostrarConteudo) const;

private:
    std::array<char, TAMANHO_DISCO> dados;            //o disco propriamente dito
    std::array<bool, QUANTIDADE_BLOCOS> ocupado;      //mapa de blocos livres
    ModoAlocacao modoAtual;

    bool escolherIndexada(const FCB& arquivo, std::size_t necessarios,
                          std::vector<std::size_t>& novos) const;
    bool escolherContigua(const FCB& arquivo, std::size_t necessarios,
                          std::vector<std::size_t>& novos) const;
    void zerarBloco(std::size_t bloco);
};

#endif
