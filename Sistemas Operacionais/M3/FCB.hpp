//file control block
#include <string>

struct FCB{
    std::string nome;
    int tamanho;
    enum tipo {arquivo, diretorio} tipo; //enum ou inteiro para dados: numérico, caractere, binário; programa
    time_t dataCriacao;
    time_t dataModificacao;
    time_t dataAcesso;
    int id;
    int permissaoAcesso; //0 - leitura, 1 - escrita, 2 - execução, classes de usuarios: proprietário (owner), grupo (group) e outros (public)
};