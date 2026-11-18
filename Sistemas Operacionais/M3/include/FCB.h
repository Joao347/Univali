#ifndef FCB_H
#define FCB_H

#include <ctime>
#include <cstddef>
#include <string>
#include <vector>

//File Control Block: metadados e referencias aos blocos do arquivo.
struct FCB {
    unsigned long long id;
    std::string nome;
    std::size_t tamanho;
    std::string tipo;
    std::time_t criacao;
    std::time_t modificacao;
    std::time_t acesso;
    std::string owner;
    std::string group;
    int permissoes; //RWX: owner, group e others; exemplo 0644.
    std::vector<std::size_t> blocos;

    FCB(const std::string& nomeArquivo, unsigned long long identificador,
        const std::string& usuario, const std::string& grupo)
        : id(identificador), nome(nomeArquivo), tamanho(0), tipo("texto"),
          criacao(std::time(nullptr)), modificacao(criacao), acesso(criacao),
          owner(usuario), group(grupo), permissoes(0644) {}
};

#endif
