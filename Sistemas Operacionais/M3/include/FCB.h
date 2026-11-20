#ifndef FCB_H
#define FCB_H

#include <cstddef>
#include <ctime>
#include <string>
#include <vector>

//tipos de arquivo pedidos no enunciado
enum class TipoArquivo {
    Caractere, //texto
    Numerico, //numero
    Binario, //bytes em hexadecimal
    Programa //executavel
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
    if (texto == "numerico") { saida = TipoArquivo::Numerico;  return true; }
    if (texto == "binario") { saida = TipoArquivo::Binario;   return true; }
    if (texto == "programa") { saida = TipoArquivo::Programa;  return true; }
    return false;
}

//FCB faz o papel do inode e guarda os metadados do arquivo
struct FCB {
    int id;
    std::string nome;                 
    std::size_t tamanho;              
    TipoArquivo tipo;
    std::time_t criacao;
    std::time_t modificacao;         
    std::time_t acesso;              
    std::string owner;
    std::string group;
    unsigned int permissoes; //cuida do controle de acesso      
    std::vector<std::size_t> blocos; 
    int links;                       

    FCB(int identificador, const std::string& nomeArquivo, TipoArquivo tipoArquivo,
        const std::string& usuario, const std::string& grupo, unsigned int modo)
        : id(identificador), nome(nomeArquivo), tamanho(0), tipo(tipoArquivo),
          criacao(std::time(nullptr)), modificacao(criacao), acesso(criacao),
          owner(usuario), group(grupo), permissoes(modo & 0777u), links(1) {} //777 seria aonde todo mundo tem permissão. 640 seria rw- (4+2+0) pra dono, r-- para grupo e --- para outros
};

#endif
