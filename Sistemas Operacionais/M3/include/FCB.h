#ifndef FCB_H
#define FCB_H

#include <cstddef>
#include <ctime>
#include <string>
#include <vector>

//tipos de arquivo pedidos no enunciado; o tipo muda como o conteudo e gravado e lido.
enum class TipoArquivo {
    Caractere,  //texto comum
    Numerico,   //cada linha precisa ser um numero
    Binario,    //bytes brutos, escritos e exibidos em hexadecimal
    Programa    //"executavel"; so este tipo pode ser executado com exec
};

inline std::string tipoParaTexto(TipoArquivo tipo) {
    switch (tipo) {
        case TipoArquivo::Caractere: return "caractere";
        case TipoArquivo::Numerico:  return "numerico";
        case TipoArquivo::Binario:   return "binario";
        case TipoArquivo::Programa:  return "programa";
    }
    return "desconhecido";
}

inline bool textoParaTipo(const std::string& texto, TipoArquivo& saida) {
    if (texto == "caractere") { saida = TipoArquivo::Caractere; return true; }
    if (texto == "numerico")  { saida = TipoArquivo::Numerico;  return true; }
    if (texto == "binario")   { saida = TipoArquivo::Binario;   return true; }
    if (texto == "programa")  { saida = TipoArquivo::Programa;  return true; }
    return false;
}

//File Control Block: faz o papel do inode. Guarda os metadados do arquivo
//e o bloco de indices com os blocos do disco simulado que ele ocupa.
struct FCB {
    int id;                           //identificador unico (numero do inode simulado)
    std::string nome;                 //nome com que o arquivo foi criado
    std::size_t tamanho;              //tamanho em bytes
    TipoArquivo tipo;
    std::time_t criacao;
    std::time_t modificacao;          //ultima alteracao do conteudo
    std::time_t acesso;               //ultima leitura
    std::string owner;
    std::string group;
    unsigned int permissoes;          //bitmask de 9 bits: rwx do owner, group e others (ex.: 0644)
    std::vector<std::size_t> blocos;  //bloco de indices
    int links;                        //quantas entradas de diretorio apontam para este FCB

    FCB(int identificador, const std::string& nomeArquivo, TipoArquivo tipoArquivo,
        const std::string& usuario, const std::string& grupo, unsigned int modo)
        : id(identificador), nome(nomeArquivo), tamanho(0), tipo(tipoArquivo),
          criacao(std::time(nullptr)), modificacao(criacao), acesso(criacao),
          owner(usuario), group(grupo), permissoes(modo & 0777u), links(1) {}
};

#endif
