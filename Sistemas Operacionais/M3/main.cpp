#include <iostream>
#include <sys/stat.h>
#include <fstream> 
#include <vector>

using namespace std;

int main(){
    
    std::vector<std::string> operacoes = {"criar", "escrever", "ler", "reposicionar", "excluir", "truncar"};

    //utilizar arvore binaria como estrutura de dados para representar um diretório estruturado por árvores.

    //testando função mkdir no POSIX
    mkdir("pasta de testes", 0777);

    return 0;
}