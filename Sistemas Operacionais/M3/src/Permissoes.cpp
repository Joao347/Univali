#include "Permissoes.h"

namespace permissoes {

bool deOctal(const std::string& texto, unsigned int& saida) {
    std::string digitos = texto;
    //aceita 3 digitos (640) ou 4 com zero inicial (0640)
    if (digitos.size() == 4 && digitos[0] == '0') digitos.erase(0, 1);
    if (digitos.size() != 3) return false;

    unsigned int modo = 0;
    for (char c : digitos) {
        if (c < '0' || c > '7') return false;
        modo = (modo << 3) | static_cast<unsigned int>(c - '0');  //cada digito octal = 3 bits
    }
    saida = modo;
    return true;
}

std::string paraTexto(unsigned int modo) {
    std::string texto = "---------";
    const char letras[] = {'r', 'w', 'x'};
    for (int i = 0; i < 9; ++i) {
        const unsigned int bit = 1u << (8 - i);  //do bit 8 (r do owner) ate o bit 0
        if (modo & bit) texto[i] = letras[i % 3];
    }
    return texto;
}

std::string paraOctal(unsigned int modo) {
    std::string texto;
    texto += static_cast<char>('0' + ((modo >> DESLOC_OWNER) & 7u));
    texto += static_cast<char>('0' + ((modo >> DESLOC_GROUP) & 7u));
    texto += static_cast<char>('0' + ((modo >> DESLOC_OTHERS) & 7u));
    return texto;
}

int classe(const FCB& arquivo, const std::string& usuario, const std::string& grupo) {
    if (usuario == arquivo.owner) return DESLOC_OWNER;
    if (grupo == arquivo.group) return DESLOC_GROUP;
    return DESLOC_OTHERS;
}

std::string nomeClasse(int deslocamento) {
    if (deslocamento == DESLOC_OWNER) return "proprietario";
    if (deslocamento == DESLOC_GROUP) return "grupo";
    return "outros";
}

bool permite(const FCB& arquivo, const std::string& usuario, const std::string& grupo,
             unsigned int bit) {
    if (usuario == ROOT) {
        if (bit != EXECUCAO) return true;
        return (arquivo.permissoes & 0111u) != 0;  //algum x em qualquer classe
    }
    const unsigned int bitsDaClasse = (arquivo.permissoes >> classe(arquivo, usuario, grupo)) & 7u;
    return (bitsDaClasse & bit) != 0;
}

bool ehDono(const FCB& arquivo, const std::string& usuario) {
    return usuario == ROOT || usuario == arquivo.owner;
}

}
