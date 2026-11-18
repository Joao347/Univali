#include "MiniFileSystem.h"
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

MiniFileSystem::MiniFileSystem()
    : raiz(std::make_unique<Diretorio>("/")), atual(raiz.get()),
      proximoId(1), usuarioAtual("aluno"), grupoAtual("alunos") {}

bool MiniFileSystem::nomeValido(const std::string& nome) {
    return !nome.empty() && nome != "." && nome != ".." &&
           nome.find('/') == std::string::npos;
}

std::string MiniFileSystem::dataHora(std::time_t instante) {
    std::tm* info = std::localtime(&instante);
    if (!info) return "data indisponivel";
    std::ostringstream saida;
    saida << std::put_time(info, "%d/%m/%Y %H:%M:%S");
    return saida.str();
}

bool MiniFileSystem::modoValido(const std::string& modo) {
    return modo.size() == 3 && std::all_of(modo.begin(), modo.end(),
        [](unsigned char c) { return c >= '0' && c <= '7'; });
}

int MiniFileSystem::modoParaInteiro(const std::string& modo) {
    return (modo[0] - '0') * 64 + (modo[1] - '0') * 8 + (modo[2] - '0');
}

std::string MiniFileSystem::modoTexto(int modo) {
    std::string texto = "---------";
    const int bits[] = {4, 2, 1};
    const char letras[] = {'r', 'w', 'x'};
    for (int classe = 0; classe < 3; ++classe) {
        const int valor = (modo >> (6 - classe * 3)) & 7;
        for (int b = 0; b < 3; ++b)
            if (valor & bits[b]) texto[classe * 3 + b] = letras[b];
    }
    return texto;
}

std::string MiniFileSystem::modoOctal(int modo) {
    std::string texto;
    texto += static_cast<char>('0' + ((modo >> 6) & 7));
    texto += static_cast<char>('0' + ((modo >> 3) & 7));
    texto += static_cast<char>('0' + (modo & 7));
    return texto;
}

Diretorio* MiniFileSystem::buscarDiretorio(Diretorio* base, const std::string& caminho) const {
    if (caminho.empty()) return base;
    Diretorio* diretorio = caminho[0] == '/' ? raiz.get() : base;
    std::stringstream partes(caminho);
    std::string parte;
    while (std::getline(partes, parte, '/')) {
        if (parte.empty() || parte == ".") continue;
        if (parte == "..") {
            if (diretorio->pai) diretorio = diretorio->pai;
            continue;
        }
        auto it = std::find_if(diretorio->diretorios.begin(), diretorio->diretorios.end(),
            [&](const std::unique_ptr<Diretorio>& item) { return item->nome == parte; });
        if (it == diretorio->diretorios.end()) return nullptr;
        diretorio = it->get();
    }
    return diretorio;
}

bool MiniFileSystem::separarDestino(const std::string& caminho, Diretorio*& destino,
                                    std::string& nome) const {
    if (caminho.empty()) return false;
    const std::size_t barra = caminho.find_last_of('/');
    if (barra == std::string::npos) {
        destino = atual;
        nome = caminho;
    } else {
        const std::string pasta = barra == 0 ? "/" : caminho.substr(0, barra);
        nome = caminho.substr(barra + 1);
        destino = buscarDiretorio(atual, pasta);
    }
    return destino && nomeValido(nome);
}

FCB* MiniFileSystem::buscarArquivo(Diretorio* diretorio, const std::string& nome) const {
    if (!diretorio) return nullptr;
    auto it = std::find_if(diretorio->arquivos.begin(), diretorio->arquivos.end(),
        [&](const std::unique_ptr<FCB>& arquivo) { return arquivo->nome == nome; });
    return it == diretorio->arquivos.end() ? nullptr : it->get();
}

bool MiniFileSystem::nomeOcupado(Diretorio* diretorio, const std::string& nome) const {
    if (buscarArquivo(diretorio, nome)) return true;
    return std::any_of(diretorio->diretorios.begin(), diretorio->diretorios.end(),
        [&](const std::unique_ptr<Diretorio>& item) { return item->nome == nome; });
}

std::string MiniFileSystem::caminhoCompleto(const Diretorio* diretorio) const {
    if (!diretorio->pai) return "/";
    std::vector<std::string> partes;
    while (diretorio && diretorio->pai) {
        partes.push_back(diretorio->nome);
        diretorio = diretorio->pai;
    }
    std::string caminho;
    for (auto it = partes.rbegin(); it != partes.rend(); ++it) caminho += "/" + *it;
    return caminho;
}

int MiniFileSystem::permissoesDoUsuario(const FCB& arquivo) const {
    if (usuarioAtual == "root" || usuarioAtual == arquivo.owner)
        return (arquivo.permissoes >> 6) & 7;
    if (grupoAtual == arquivo.group) return (arquivo.permissoes >> 3) & 7;
    return arquivo.permissoes & 7;
}

bool MiniFileSystem::pode(const FCB& arquivo, int bit, const std::string& operacao) const {
    if ((permissoesDoUsuario(arquivo) & bit) != 0) return true;
    std::cout << "Permissao negada: usuario '" << usuarioAtual << "' nao pode "
              << operacao << " o arquivo '" << arquivo.nome << "'.\n";
    return false;
}

void MiniFileSystem::mkdir(const std::string& nome) {
    if (!nomeValido(nome)) { std::cout << "Nome de diretorio invalido.\n"; return; }
    if (nomeOcupado(atual, nome)) { std::cout << "Ja existe um item com esse nome.\n"; return; }
    atual->diretorios.push_back(std::make_unique<Diretorio>(nome, atual));
    std::cout << "Diretorio criado: " << nome << "\n";
}

void MiniFileSystem::cd(const std::string& caminho) {
    Diretorio* destino = buscarDiretorio(atual, caminho);
    if (!destino) { std::cout << "Diretorio nao encontrado.\n"; return; }
    atual = destino;
    std::cout << "Diretorio atual: " << caminhoCompleto(atual) << "\n";
}

void MiniFileSystem::pwd() const { std::cout << caminhoCompleto(atual) << "\n"; }

void MiniFileSystem::ls() const {
    std::cout << "Conteudo de " << caminhoCompleto(atual) << ":\n";
    for (const auto& diretorio : atual->diretorios)
        std::cout << "  [DIR]  " << diretorio->nome << "\n";
    for (const auto& arquivo : atual->arquivos)
        std::cout << "  [FILE] " << modoTexto(arquivo->permissoes) << " " << arquivo->nome
                  << " (" << arquivo->tamanho << " bytes, ID " << arquivo->id << ")\n";
    if (atual->diretorios.empty() && atual->arquivos.empty()) std::cout << "  (diretorio vazio)\n";
}

void MiniFileSystem::touch(const std::string& nome) {
    if (!nomeValido(nome)) { std::cout << "Nome de arquivo invalido.\n"; return; }
    if (nomeOcupado(atual, nome)) { std::cout << "Ja existe um item com esse nome.\n"; return; }
    atual->arquivos.push_back(std::make_unique<FCB>(nome, proximoId++, usuarioAtual, grupoAtual));
    std::cout << "Arquivo criado: " << nome << " (permissao inicial 644).\n";
}

void MiniFileSystem::echo(const std::string& nome, const std::string& texto) {
    FCB* arquivo = buscarArquivo(atual, nome);
    if (!arquivo) { std::cout << "Arquivo nao encontrado. Use touch antes.\n"; return; }
    if (!pode(*arquivo, 2, "escrever")) return;
    if (disco.armazenar(*arquivo, texto)) std::cout << "Conteudo gravado em " << nome << ".\n";
}

void MiniFileSystem::cat(const std::string& nome) {
    FCB* arquivo = buscarArquivo(atual, nome);
    if (!arquivo) { std::cout << "Arquivo nao encontrado.\n"; return; }
    if (!pode(*arquivo, 4, "ler")) return;
    arquivo->acesso = std::time(nullptr);
    std::cout << "Conteudo de " << nome << ":\n" << disco.recuperar(*arquivo) << "\n";
}

void MiniFileSystem::info(const std::string& nome) const {
    FCB* arquivo = buscarArquivo(atual, nome);
    if (!arquivo) { std::cout << "Arquivo nao encontrado.\n"; return; }
    std::cout << "Nome: " << arquivo->nome << "\nID unico: " << arquivo->id
              << "\nTamanho: " << arquivo->tamanho << " bytes\nTipo: " << arquivo->tipo
              << "\nCriacao: " << dataHora(arquivo->criacao)
              << "\nModificacao: " << dataHora(arquivo->modificacao)
              << "\nUltimo acesso: " << dataHora(arquivo->acesso)
              << "\nProprietario: " << arquivo->owner << "\nGrupo: " << arquivo->group
              << "\nPermissoes: " << modoOctal(arquivo->permissoes)
              << " (" << modoTexto(arquivo->permissoes) << ")\n";
}

void MiniFileSystem::cp(const std::string& origem, const std::string& destinoCaminho) {
    FCB* original = buscarArquivo(atual, origem);
    if (!original) { std::cout << "Arquivo de origem nao encontrado no diretorio atual.\n"; return; }
    if (!pode(*original, 4, "ler/copiar")) return;
    Diretorio* destino = nullptr;
    std::string nome;
    if (!separarDestino(destinoCaminho, destino, nome)) {
        std::cout << "Diretorio ou nome de destino invalido.\n"; return;
    }
    if (nomeOcupado(destino, nome)) { std::cout << "Ja existe um item com esse nome no destino.\n"; return; }

    auto copia = std::make_unique<FCB>(nome, proximoId, usuarioAtual, grupoAtual);
    copia->tipo = original->tipo;
    copia->permissoes = original->permissoes;
    if (!disco.armazenar(*copia, disco.recuperar(*original))) return;
    ++proximoId;
    destino->arquivos.push_back(std::move(copia));
    std::cout << "Arquivo copiado para " << caminhoCompleto(destino)
              << (caminhoCompleto(destino) == "/" ? "" : "/") << nome << ".\n";
}

void MiniFileSystem::mv(const std::string& origem, const std::string& destinoCaminho) {
    auto it = std::find_if(atual->arquivos.begin(), atual->arquivos.end(),
        [&](const std::unique_ptr<FCB>& arquivo) { return arquivo->nome == origem; });
    if (it == atual->arquivos.end()) { std::cout << "Arquivo de origem nao encontrado no diretorio atual.\n"; return; }
    FCB& arquivo = **it;
    if (usuarioAtual != "root" && usuarioAtual != arquivo.owner) {
        std::cout << "Permissao negada: somente o proprietario ou root pode mover/renomear este arquivo.\n";
        return;
    }
    Diretorio* destino = nullptr;
    std::string nome;
    if (!separarDestino(destinoCaminho, destino, nome)) {
        std::cout << "Diretorio ou nome de destino invalido.\n"; return;
    }
    if (destino == atual && nome == origem) { std::cout << "O arquivo ja possui esse nome e local.\n"; return; }
    if (nomeOcupado(destino, nome)) { std::cout << "Ja existe um item com esse nome no destino.\n"; return; }

    std::unique_ptr<FCB> movido = std::move(*it);
    atual->arquivos.erase(it);
    movido->nome = nome;
    movido->modificacao = std::time(nullptr);
    destino->arquivos.push_back(std::move(movido));
    std::cout << "Arquivo movido/renomeado para " << caminhoCompleto(destino)
              << (caminhoCompleto(destino) == "/" ? "" : "/") << nome << ".\n";
}

void MiniFileSystem::rm(const std::string& nome) {
    auto it = std::find_if(atual->arquivos.begin(), atual->arquivos.end(),
        [&](const std::unique_ptr<FCB>& arquivo) { return arquivo->nome == nome; });
    if (it == atual->arquivos.end()) { std::cout << "Arquivo nao encontrado.\n"; return; }
    if (usuarioAtual != "root" && usuarioAtual != (*it)->owner) {
        std::cout << "Permissao negada: somente o proprietario ou root pode excluir este arquivo.\n";
        return;
    }
    disco.liberar(**it);
    atual->arquivos.erase(it);
    std::cout << "Arquivo removido: " << nome << "\n";
}

void MiniFileSystem::chmod(const std::string& modo, const std::string& nome) {
    FCB* arquivo = buscarArquivo(atual, nome);
    if (!arquivo) { std::cout << "Arquivo nao encontrado.\n"; return; }
    if (!modoValido(modo)) { std::cout << "Modo invalido. Use tres digitos octais, por exemplo 644 ou 755.\n"; return; }
    if (usuarioAtual != "root" && usuarioAtual != arquivo->owner) {
        std::cout << "Permissao negada: somente o proprietario ou root pode alterar permissoes.\n";
        return;
    }
    arquivo->permissoes = modoParaInteiro(modo);
    std::cout << "Permissoes de " << nome << " alteradas para " << modo
              << " (" << modoTexto(arquivo->permissoes) << ").\n";
}

void MiniFileSystem::usuario(const std::string& novoUsuario, const std::string& novoGrupo) {
    if (novoUsuario.empty() || novoGrupo.empty()) { std::cout << "Uso: user NOME GRUPO\n"; return; }
    usuarioAtual = novoUsuario;
    grupoAtual = novoGrupo;
    std::cout << "Usuario atual: " << usuarioAtual << " | Grupo atual: " << grupoAtual << "\n";
}

void MiniFileSystem::executarArquivo(const std::string& nome) {
    FCB* arquivo = buscarArquivo(atual, nome);
    if (!arquivo) { std::cout << "Arquivo nao encontrado.\n"; return; }
    if (!pode(*arquivo, 1, "executar")) return;
    std::cout << "Permissao de execucao concedida para '" << nome
              << "'. (Execucao simulada; nenhum programa real foi iniciado.)\n";
}

void MiniFileSystem::mostrarDisco() const { disco.mostrarOcupacao(); }

void MiniFileSystem::mostrarBlocos(const std::string& nome) const {
    FCB* arquivo = buscarArquivo(atual, nome);
    if (!arquivo) { std::cout << "Arquivo nao encontrado.\n"; return; }
    disco.mostrarBlocos(*arquivo);
}

std::string MiniFileSystem::lerCampo(const std::string& mensagem) {
    std::cout << mensagem;
    std::string valor;
    if (!std::getline(std::cin, valor)) return "";
    return valor;
}

void MiniFileSystem::mostrarAjuda() const {
    std::cout << "Comandos disponiveis:\n"
              << "  mkdir NOME              cria um diretorio\n"
              << "  cd CAMINHO              muda de diretorio (aceita /, .. e caminhos)\n"
              << "  ls                      lista diretorios e arquivos\n"
              << "  pwd                     mostra o caminho atual\n"
              << "  touch ARQUIVO           cria um arquivo vazio\n"
              << "  echo ARQUIVO TEXTO      grava/substitui o conteudo (texto pode ter espacos)\n"
              << "  cat ARQUIVO             le o conteudo\n"
              << "  info ARQUIVO            mostra metadados e permissoes\n"
              << "  cp ORIGEM DESTINO       copia para nome ou caminho de destino\n"
              << "  mv ORIGEM DESTINO       move/renomeia para nome ou caminho de destino\n"
              << "  rm ARQUIVO              exclui e libera seus blocos\n"
              << "  chmod MODO ARQUIVO      altera permissoes (ex.: chmod 640 notas.txt)\n"
              << "  user NOME GRUPO         troca o usuario/grupo simulado\n"
              << "  exec ARQUIVO            testa a permissao de execucao\n"
              << "  disk                    mostra ocupacao do disco simulado\n"
              << "  blocks ARQUIVO          mostra os blocos usados pelo arquivo\n"
              << "  help                    mostra esta ajuda\n"
              << "  exit                    encerra o simulador\n"
              << "OBS: operacoes de arquivo usam nomes do diretorio atual; cp/mv aceitam destino com caminho.\n";
}

void MiniFileSystem::mostrarMenu() const {
    std::cout << "\nMini-Sistema de Arquivos:\n"
              << "Usuario: " << usuarioAtual << " | Grupo: " << grupoAtual << "\n"
              << "Diretorio: " << caminhoCompleto(atual) << "\n"
              << "Digite 'help' para ver os comandos.\n";
}

void MiniFileSystem::executar() {
    std::cout << "Mini-Sistema de Arquivos - simulacao em memoria.\n"
              << "Disco didatico: 4096 bytes, dividido em 256 blocos de 16 bytes.\n"
              << "Nenhum arquivo real do computador sera criado ou alterado.\n";

    std::string linha;
    while (true) {
        mostrarMenu();
        std::cout << caminhoCompleto(atual) << " $ ";
        if (!std::getline(std::cin, linha)) break;
        if (linha.empty()) continue;

        std::istringstream entrada(linha);
        std::string comando, a, b;
        entrada >> comando;
        if (comando == "exit" || comando == "sair") break;
        if (comando == "help") { mostrarAjuda(); continue; }
        if (comando == "mkdir") {
            entrada >> a;
            if (a.empty()) std::cout << "Uso: mkdir NOME\n"; else mkdir(a);
        } else if (comando == "cd") {
            entrada >> a;
            if (a.empty()) std::cout << "Uso: cd CAMINHO\n"; else cd(a);
        } else if (comando == "ls") ls();
        else if (comando == "pwd") pwd();
        else if (comando == "touch") {
            entrada >> a;
            if (a.empty()) std::cout << "Uso: touch ARQUIVO\n"; else touch(a);
        } else if (comando == "echo") {
            entrada >> a;
            std::string texto;
            std::getline(entrada, texto);
            if (!texto.empty() && texto[0] == ' ') texto.erase(0, 1);
            if (a.empty()) std::cout << "Uso: echo ARQUIVO TEXTO\n"; else echo(a, texto);
        } else if (comando == "cat" || comando == "info" || comando == "rm" ||
                   comando == "exec" || comando == "blocks") {
            entrada >> a;
            if (a.empty()) std::cout << "Informe o nome do arquivo.\n";
            else if (comando == "cat") cat(a);
            else if (comando == "info") info(a);
            else if (comando == "rm") rm(a);
            else if (comando == "exec") executarArquivo(a);
            else mostrarBlocos(a);
        } else if (comando == "cp" || comando == "mv") {
            entrada >> a >> b;
            if (a.empty() || b.empty()) std::cout << "Uso: " << comando << " ORIGEM DESTINO\n";
            else if (comando == "cp") cp(a, b);
            else mv(a, b);
        } else if (comando == "chmod") {
            entrada >> a >> b;
            if (a.empty() || b.empty()) std::cout << "Uso: chmod MODO ARQUIVO\n";
            else chmod(a, b);
        } else if (comando == "user") {
            entrada >> a >> b;
            if (a.empty() || b.empty()) std::cout << "Uso: user NOME GRUPO\n";
            else usuario(a, b);
        } else if (comando == "disk") mostrarDisco();
        else std::cout << "Comando desconhecido. Digite 'help'.\n";
    }
    std::cout << "Encerrando. A memoria sera liberada automaticamente (RAII).\n";
}
