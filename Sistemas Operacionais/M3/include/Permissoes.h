#ifndef PERMISSOES_H
#define PERMISSOES_H

#include "FCB.h"
#include <string>

//as permissoes ocupam 9 bits de um inteiro, no mesmo formato do chmod numerico:
//
//      bit:   8 7 6   5 4 3   2 1 0
//             r w x   r w x   r w x
//             owner   group   others        0640 = 110 100 000 = rw-r-----
namespace permissoes {

constexpr unsigned int LEITURA = 4;   //r
constexpr unsigned int ESCRITA = 2;   //w
constexpr unsigned int EXECUCAO = 1;  //x

constexpr int DESLOC_OWNER = 6;
constexpr int DESLOC_GROUP = 3;
constexpr int DESLOC_OTHERS = 0;

constexpr unsigned int PADRAO_ARQUIVO = 0644;  //rw-r--r--, como o umask 022 do Linux

const std::string ROOT = "root";

//converte "640" ou "0640" para bitmask; retorna false se o modo for invalido.
bool deOctal(const std::string& texto, unsigned int& saida);

//"rw-r-----" e "640"
std::string paraTexto(unsigned int modo);
std::string paraOctal(unsigned int modo);

//deslocamento da classe (owner, group ou others) em que o usuario cai para este arquivo.
int classe(const FCB& arquivo, const std::string& usuario, const std::string& grupo);
std::string nomeClasse(int deslocamento);

//testa um bit (LEITURA, ESCRITA ou EXECUCAO). O root ignora r e w e so executa
//se alguma classe tiver o bit x, como no Linux.
bool permite(const FCB& arquivo, const std::string& usuario, const std::string& grupo,
             unsigned int bit);

//proprietario ou root (usado em chmod, mv, rm e ln).
bool ehDono(const FCB& arquivo, const std::string& usuario);

}

#endif
