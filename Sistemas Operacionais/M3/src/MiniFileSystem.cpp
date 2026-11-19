//comandos do mini-sistema de arquivos.

#include "MiniFileSystem.h"
#include "Permissoes.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <io.h>
#define isatty _isatty
#define STDIN_FILENO 0
#else
#include <unistd.h>
#endif

namespace {

constexpr std::size_t MAX_NOME = 63;

std::string aparar(const std::string& texto) {
    const auto inicio = texto.find_first_not_of(" \t");
    if (inicio == std::string::npos) return "";
    const auto fim = texto.find_last_not_of(" \t");
    return texto.substr(inicio, fim - inicio + 1);
}

bool ehNumero(const std::string& texto) {
    const std::string t = aparar(texto);
    if (t.empty()) return false;
    char* fim = nullptr;
    std::strtod(t.c_str(), &fim);
    return fim && *fim == '\0';
}

//converte "48 65 6c" ou "48656c" em bytes; retorna false se nao for hexadecimal valido.
bool hexParaBytes(const std::string& texto, std::string& saida) {
    saida.clear();
    int meio = -1;
    for (unsigned char c : texto) {
        if (std::isspace(c)) continue;
        if (!std::isxdigit(c)) return false;
        const int valor = std::isdigit(c) ? c - '0' : std::tolower(c) - 'a' + 10;
        if (meio < 0) {
            meio = valor;
        } else {
            saida += static_cast<char>(meio * 16 + valor);
            meio = -1;
        }
    }
    return meio < 0;  //quantidade impar de digitos e invalida
}

void imprimirHex(const std::string& dados) {
    for (std::size_t i = 0; i < dados.size(); i += 16) {
        std::cout << std::hex << std::setw(4) << std::setfill('0') << i << "  ";
        for (std::size_t j = 0; j < 16; ++j) {
            if (i + j < dados.size())
                std::cout << std::setw(2) << static_cast<int>(static_cast<unsigned char>(dados[i + j])) << ' ';
            else
                std::cout << "   ";
        }
        std::cout << std::dec << std::setfill(' ') << " |";
        for (std::size_t j = 0; j < 16 && i + j < dados.size(); ++j) {
            const unsigned char c = static_cast<unsigned char>(dados[i + j]);
            std::cout << (std::isprint(c) ? static_cast<char>(c) : '.');
        }
        std::cout << "|\n";
    }
}

}

MiniFileSystem::MiniFileSystem()
    : raiz(std::make_unique<Diretorio>("/")), atual(raiz.get()),
      proximoId(1), usuarioAtual("aluno"), grupoAtual("alunos") {}

//------------------------------------------------------------------
//caminhos
//------------------------------------------------------------------

bool MiniFileSystem::nomeValido(const std::string& nome) {
    if (nome.empty() || nome.size() > MAX_NOME || nome == "." || nome == "..") return false;
    return std::all_of(nome.begin(), nome.end(), [](unsigned char c) {
        return c != '/' && std::isgraph(c);  //sem barra, espacos ou caracteres de controle
    });
}

//percorre a arvore a partir da raiz (caminho absoluto) ou do diretorio atual.
Diretorio* MiniFileSystem::buscarDiretorio(const std::string& caminho) const {
    if (caminho.empty()) return atual;
    Diretorio* diretorio = caminho[0] == '/' ? raiz.get() : atual;
    std::stringstream partes(caminho);
    std::string parte;
    while (std::getline(partes, parte, '/')) {
        if (parte.empty() || parte == ".") continue;
        if (parte == "..") {
            if (diretorio->pai) diretorio = diretorio->pai;  //".." na raiz continua na raiz
            continue;
        }
        diretorio = diretorio->buscarFilho(parte);
        if (!diretorio) return nullptr;
    }
    return diretorio;
}

//separa "a/b/nome" em diretorio pai (a/b) e ultimo componente (nome).
MiniFileSystem::ResultadoCaminho MiniFileSystem::separarCaminho(const std::string& caminho,
                                                                Diretorio*& pai,
                                                                std::string& nome) const {
    std::string limpo = caminho;
    while (limpo.size() > 1 && limpo.back() == '/') limpo.pop_back();  //"docs/" vira "docs"

    const std::size_t barra = limpo.find_last_of('/');
    if (barra == std::string::npos) {
        pai = atual;
        nome = limpo;
    } else {
        pai = barra == 0 ? raiz.get() : buscarDiretorio(limpo.substr(0, barra));
        nome = limpo.substr(barra + 1);
    }
    if (!pai) return ResultadoCaminho::PaiInexistente;
    if (!nomeValido(nome)) return ResultadoCaminho::NomeInvalido;
    return ResultadoCaminho::Ok;
}

//encontra o arquivo indicado; em caso de erro imprime "cmd: caminho: motivo".
Entrada* MiniFileSystem::localizarArquivo(const std::string& cmd, const std::string& caminho,
                                          Diretorio** dirSaida) const {
    Diretorio* pai = nullptr;
    std::string nome;
    if (separarCaminho(caminho, pai, nome) != ResultadoCaminho::Ok) {
        std::cout << cmd << ": " << caminho << ": Arquivo ou diretorio nao encontrado\n";
        return nullptr;
    }
    Entrada* entrada = pai->buscarEntrada(nome);
    if (!entrada) {
        if (pai->buscarFilho(nome))
            std::cout << cmd << ": " << caminho << ": E um diretorio\n";
        else
            std::cout << cmd << ": " << caminho << ": Arquivo ou diretorio nao encontrado\n";
        return nullptr;
    }
    if (dirSaida) *dirSaida = pai;
    return entrada;
}

//destino de cp/mv/ln: se for um diretorio existente, o arquivo mantem o nome original.
bool MiniFileSystem::resolverDestino(const std::string& cmd, const std::string& destino,
                                     const std::string& nomeOrigem, Diretorio*& dir,
                                     std::string& nome) const {
    if (Diretorio* comoDiretorio = buscarDiretorio(destino)) {
        dir = comoDiretorio;
        nome = nomeOrigem;
        return true;
    }
    const ResultadoCaminho r = separarCaminho(destino, dir, nome);
    if (r == ResultadoCaminho::PaiInexistente) {
        std::cout << cmd << ": " << destino << ": Diretorio de destino nao encontrado\n";
        return false;
    }
    if (r == ResultadoCaminho::NomeInvalido) {
        std::cout << cmd << ": " << destino << ": Nome de destino invalido\n";
        return false;
    }
    return true;
}

std::string MiniFileSystem::caminhoCompleto(const Diretorio* diretorio) const {
    if (!diretorio->pai) return "/";
    std::vector<std::string> partes;
    for (; diretorio && diretorio->pai; diretorio = diretorio->pai) partes.push_back(diretorio->nome);
    std::string caminho;
    for (auto it = partes.rbegin(); it != partes.rend(); ++it) caminho += "/" + *it;
    return caminho;
}

//------------------------------------------------------------------
//protecao
//------------------------------------------------------------------

bool MiniFileSystem::verificarPermissao(const FCB& arquivo, unsigned int bit, const std::string& cmd,
                                        const std::string& caminho) const {
    if (permissoes::permite(arquivo, usuarioAtual, grupoAtual, bit)) return true;

    const std::string simbolico = permissoes::paraTexto(arquivo.permissoes);
    if (usuarioAtual == permissoes::ROOT) {
        //o root so e barrado na execucao, quando nenhuma classe tem x
        std::cout << cmd << ": " << caminho
                  << ": Permissao negada (nem o root executa: nenhuma classe tem x; permissoes "
                  << simbolico << ")\n";
        return false;
    }
    const std::string operacao = bit == permissoes::LEITURA ? "leitura (r)"
                               : bit == permissoes::ESCRITA ? "escrita (w)" : "execucao (x)";
    std::cout << cmd << ": " << caminho << ": Permissao negada (usuario '" << usuarioAtual
              << "' esta na classe '"
              << permissoes::nomeClasse(permissoes::classe(arquivo, usuarioAtual, grupoAtual))
              << "' e nao tem " << operacao << "; permissoes " << simbolico << ")\n";
    return false;
}

bool MiniFileSystem::verificarDono(const FCB& arquivo, const std::string& cmd,
                                   const std::string& caminho) const {
    if (permissoes::ehDono(arquivo, usuarioAtual)) return true;
    std::cout << cmd << ": " << caminho << ": Operacao nao permitida (somente o proprietario '"
              << arquivo.owner << "' ou root)\n";
    return false;
}

//------------------------------------------------------------------
//apoio
//------------------------------------------------------------------

std::shared_ptr<FCB> MiniFileSystem::criarArquivo(Diretorio* dir, const std::string& nome,
                                                  TipoArquivo tipo, unsigned int modo) {
    auto fcb = std::make_shared<FCB>(proximoId++, nome, tipo, usuarioAtual, grupoAtual, modo);
    dir->arquivos.push_back({nome, fcb});  //IDs nunca sao reutilizados
    return fcb;
}

//remove uma entrada; quando o ultimo link some, devolve os blocos ao disco.
//retorna quantos links restaram (-1 se a entrada nao existir).
int MiniFileSystem::desvincular(Diretorio* dir, const std::string& nome) {
    auto it = std::find_if(dir->arquivos.begin(), dir->arquivos.end(),
                           [&](const Entrada& e) { return e.nome == nome; });
    if (it == dir->arquivos.end()) return -1;
    std::shared_ptr<FCB> fcb = it->fcb;
    dir->arquivos.erase(it);
    const int restantes = --fcb->links;
    if (restantes == 0) disco.liberar(*fcb);  //o FCB e destruido quando o shared_ptr sai de escopo
    return restantes;
}

void MiniFileSystem::mensagemErroDisco(ResultadoDisco resultado, const std::string& cmd,
                                       const std::string& caminho, std::size_t bytes) const {
    const std::size_t necessarios = DiscoSimulado::blocosNecessarios(bytes);
    const std::size_t livres = disco.blocosLivres();
    if (resultado == ResultadoDisco::SemEspaco)
        std::cout << cmd << ": " << caminho << ": Nao ha espaco no disco simulado (precisa de "
                  << necessarios << " blocos, ha " << livres << " livres)\n";
    else if (resultado == ResultadoDisco::Fragmentado)
        std::cout << cmd << ": " << caminho << ": Ha " << livres
                  << " blocos livres, mas nenhuma sequencia contigua de " << necessarios
                  << " blocos (fragmentacao externa da alocacao contigua)\n";
}

std::string MiniFileSystem::dataHora(std::time_t instante) {
    std::tm* info = std::localtime(&instante);
    if (!info) return "data indisponivel";
    std::ostringstream saida;
    saida << std::put_time(info, "%d/%m/%Y %H:%M:%S");
    return saida.str();
}

//------------------------------------------------------------------
//diretorios: mkdir, rmdir, cd, pwd, ls, tree
//------------------------------------------------------------------

void MiniFileSystem::mkdir(const std::string& caminho) {
    Diretorio* pai = nullptr;
    std::string nome;
    const ResultadoCaminho r = separarCaminho(caminho, pai, nome);
    if (r == ResultadoCaminho::PaiInexistente) {
        std::cout << "mkdir: " << caminho << ": Diretorio pai nao encontrado\n";
        return;
    }
    if (r == ResultadoCaminho::NomeInvalido) {
        std::cout << "mkdir: " << caminho << ": Nome invalido (sem '/', espacos, '.' ou '..'; ate "
                  << MAX_NOME << " caracteres)\n";
        return;
    }
    if (pai->nomeOcupado(nome)) {
        std::cout << "mkdir: nao foi possivel criar '" << caminho << "': Arquivo existe\n";
        return;
    }
    pai->adicionarFilho(std::make_unique<Diretorio>(nome, pai));
    std::cout << "Diretorio criado: " << caminho << "\n";
}

void MiniFileSystem::rmdir(const std::string& caminho) {
    Diretorio* alvo = buscarDiretorio(caminho);
    if (!alvo) {
        Diretorio* pai = nullptr;
        std::string nome;
        if (separarCaminho(caminho, pai, nome) == ResultadoCaminho::Ok && pai->buscarEntrada(nome))
            std::cout << "rmdir: " << caminho << ": Nao e um diretorio\n";
        else
            std::cout << "rmdir: " << caminho << ": Arquivo ou diretorio nao encontrado\n";
        return;
    }
    if (alvo == raiz.get()) {
        std::cout << "rmdir: " << caminho << ": Nao e possivel remover a raiz\n";
        return;
    }
    for (const Diretorio* d = atual; d; d = d->pai) {
        if (d == alvo) {
            std::cout << "rmdir: " << caminho
                      << ": Dispositivo ou recurso esta ocupado (e o diretorio atual ou um ancestral)\n";
            return;
        }
    }
    if (!alvo->vazio()) {
        std::cout << "rmdir: " << caminho << ": Diretorio nao vazio\n";
        return;
    }
    alvo->pai->removerFilho(alvo);  //o unique_ptr libera o no
    std::cout << "Diretorio removido: " << caminho << "\n";
}

void MiniFileSystem::cd(const std::string& caminho) {
    Diretorio* destino = caminho.empty() ? raiz.get() : buscarDiretorio(caminho);
    if (!destino) {
        Diretorio* pai = nullptr;
        std::string nome;
        if (separarCaminho(caminho, pai, nome) == ResultadoCaminho::Ok && pai->buscarEntrada(nome))
            std::cout << "cd: " << caminho << ": Nao e um diretorio\n";
        else
            std::cout << "cd: " << caminho << ": Arquivo ou diretorio nao encontrado\n";
        return;
    }
    atual = destino;
}

void MiniFileSystem::pwd() const { std::cout << caminhoCompleto(atual) << "\n"; }

void MiniFileSystem::imprimirLinhaArquivo(const Entrada& entrada) const {
    const FCB& f = *entrada.fcb;
    std::cout << std::setw(5) << f.id << "  -" << permissoes::paraTexto(f.permissoes) << ' '
              << std::setw(2) << f.links << "  " << std::left << std::setw(10) << f.owner << ' '
              << std::setw(10) << f.group << std::right << ' ' << std::setw(6) << f.tamanho << "  "
              << std::left << std::setw(9) << tipoParaTexto(f.tipo) << std::right << "  "
              << dataHora(f.modificacao).substr(0, 16) << "  " << entrada.nome << "\n";
}

void MiniFileSystem::ls(const std::string& caminho) const {
    Diretorio* dir = buscarDiretorio(caminho);
    if (!dir) {
        //"ls arquivo" mostra so a linha daquele arquivo, como no Linux
        Diretorio* pai = nullptr;
        std::string nome;
        Entrada* entrada = nullptr;
        if (separarCaminho(caminho, pai, nome) == ResultadoCaminho::Ok) entrada = pai->buscarEntrada(nome);
        if (!entrada) {
            std::cout << "ls: " << caminho << ": Arquivo ou diretorio nao encontrado\n";
            return;
        }
        imprimirLinhaArquivo(*entrada);
        return;
    }

    std::cout << "Conteudo de " << caminhoCompleto(dir) << ":\n";
    if (dir->vazio()) {
        std::cout << "  (diretorio vazio)\n";
        return;
    }
    std::cout << "INODE  PERMISSOES LK  DONO       GRUPO       BYTES  TIPO       MODIFICACAO       NOME\n";
    for (const Diretorio* filho = dir->primeiroFilho.get(); filho; filho = filho->proximoIrmao.get())
        std::cout << "    -  d          -  -          -               -  diretorio  -                 "
                  << filho->nome << "/\n";
    for (const auto& entrada : dir->arquivos) imprimirLinhaArquivo(entrada);
}

void MiniFileSystem::imprimirArvore(const Diretorio* dir, int nivel) const {
    const std::string recuo(static_cast<std::size_t>(nivel) * 4, ' ');
    for (const Diretorio* filho = dir->primeiroFilho.get(); filho; filho = filho->proximoIrmao.get()) {
        std::cout << recuo << "|-- " << filho->nome << "/\n";
        imprimirArvore(filho, nivel + 1);
    }
    for (const auto& entrada : dir->arquivos)
        std::cout << recuo << "|-- " << entrada.nome << "  (inode " << entrada.fcb->id << ", "
                  << entrada.fcb->tamanho << " bytes)\n";
}

void MiniFileSystem::tree(const std::string& caminho) const {
    Diretorio* dir = buscarDiretorio(caminho);
    if (!dir) {
        std::cout << "tree: " << caminho << ": Diretorio nao encontrado\n";
        return;
    }
    std::cout << caminhoCompleto(dir) << "\n";
    imprimirArvore(dir, 0);
}

//------------------------------------------------------------------
//arquivos: touch, echo, cat, truncate, info, cp, mv, rm, ln
//------------------------------------------------------------------

void MiniFileSystem::touch(const std::string& caminho, const std::string& tipoTexto) {
    TipoArquivo tipo = TipoArquivo::Caractere;
    if (!tipoTexto.empty() && !textoParaTipo(tipoTexto, tipo)) {
        std::cout << "touch: tipo invalido '" << tipoTexto
                  << "' (use caractere, numerico, binario ou programa)\n";
        return;
    }

    Diretorio* pai = nullptr;
    std::string nome;
    const ResultadoCaminho r = separarCaminho(caminho, pai, nome);
    if (r == ResultadoCaminho::PaiInexistente) {
        std::cout << "touch: " << caminho << ": Diretorio nao encontrado\n";
        return;
    }
    if (r == ResultadoCaminho::NomeInvalido) {
        std::cout << "touch: " << caminho << ": Nome invalido (sem '/', espacos, '.' ou '..'; ate "
                  << MAX_NOME << " caracteres)\n";
        return;
    }
    if (pai->buscarFilho(nome)) {
        std::cout << "touch: " << caminho << ": E um diretorio\n";
        return;
    }

    if (Entrada* existente = pai->buscarEntrada(nome)) {
        //arquivo ja existe: como no Linux, so atualiza as datas (exige ser dono ou ter w)
        FCB& f = *existente->fcb;
        if (!permissoes::ehDono(f, usuarioAtual) &&
            !verificarPermissao(f, permissoes::ESCRITA, "touch", caminho))
            return;
        f.acesso = f.modificacao = std::time(nullptr);
        std::cout << "touch: " << caminho << " ja existe; datas de acesso e modificacao atualizadas\n";
        return;
    }

    auto novo = criarArquivo(pai, nome, tipo, permissoes::PADRAO_ARQUIVO);
    std::cout << "Arquivo criado: " << caminho << " (inode " << novo->id << ", tipo "
              << tipoParaTexto(tipo) << ", permissoes 644 rw-r--r--)\n";
}

//echo TEXTO            -> mostra o texto
//echo TEXTO > ARQ      -> substitui o conteudo (cria o arquivo se nao existir)
//echo TEXTO >> ARQ     -> acrescenta ao final
void MiniFileSystem::echo(const std::string& argumentos) {
    //procura o primeiro '>' fora de aspas
    std::size_t redir = std::string::npos;
    char aspas = 0;
    for (std::size_t i = 0; i < argumentos.size(); ++i) {
        const char c = argumentos[i];
        if (aspas) { if (c == aspas) aspas = 0; continue; }
        if (c == '"' || c == '\'') { aspas = c; continue; }
        if (c == '>') { redir = i; break; }
    }

    std::string texto = aparar(argumentos.substr(0, redir));
    if (texto.size() >= 2 && (texto[0] == '"' || texto[0] == '\'') && texto.back() == texto[0])
        texto = texto.substr(1, texto.size() - 2);  //tira as aspas externas

    if (redir == std::string::npos) {  //sem redirecionamento: comporta-se como o echo comum
        std::cout << texto << "\n";
        return;
    }
    const bool acrescentar = redir + 1 < argumentos.size() && argumentos[redir + 1] == '>';
    const std::string destino = aparar(argumentos.substr(redir + (acrescentar ? 2 : 1)));
    if (destino.empty() || destino.find_first_of(" \t>") != std::string::npos) {
        std::cout << "echo: informe um unico arquivo depois de '>' ou '>>'\n";
        return;
    }

    Diretorio* pai = nullptr;
    std::string nome;
    const ResultadoCaminho r = separarCaminho(destino, pai, nome);
    if (r == ResultadoCaminho::PaiInexistente) {
        std::cout << "echo: " << destino << ": Diretorio nao encontrado\n";
        return;
    }
    if (r == ResultadoCaminho::NomeInvalido) {
        std::cout << "echo: " << destino << ": Nome de arquivo invalido\n";
        return;
    }
    if (pai->buscarFilho(nome)) {
        std::cout << "echo: " << destino << ": E um diretorio\n";
        return;
    }

    Entrada* entrada = pai->buscarEntrada(nome);
    std::shared_ptr<FCB> arquivo = entrada ? entrada->fcb : nullptr;
    const TipoArquivo tipo = arquivo ? arquivo->tipo : TipoArquivo::Caractere;

    if (arquivo && !verificarPermissao(*arquivo, permissoes::ESCRITA, "echo", destino)) return;
    //para acrescentar o simulador precisa ler o conteudo atual
    if (acrescentar && arquivo && !verificarPermissao(*arquivo, permissoes::LEITURA, "echo", destino))
        return;

    //converte o texto conforme o tipo do arquivo
    std::string dados;
    if (tipo == TipoArquivo::Binario) {
        if (!hexParaBytes(texto, dados)) {
            std::cout << "echo: " << destino
                      << ": arquivo binario; informe bytes em hexadecimal (ex.: 48 65 6c 6c 6f)\n";
            return;
        }
    } else {
        if (tipo == TipoArquivo::Numerico && !ehNumero(texto)) {
            std::cout << "echo: " << destino << ": arquivo numerico; '" << texto << "' nao e um numero\n";
            return;
        }
        dados = texto + "\n";  //como o echo do Linux, termina com quebra de linha
    }

    if (!arquivo) {
        arquivo = criarArquivo(pai, nome, TipoArquivo::Caractere, permissoes::PADRAO_ARQUIVO);
        std::cout << "Arquivo criado: " << destino << " (inode " << arquivo->id
                  << ", permissoes 644 rw-r--r--)\n";
    }

    const std::string conteudo = acrescentar ? disco.recuperar(*arquivo) + dados : dados;
    const ResultadoDisco resultado = disco.armazenar(*arquivo, conteudo);
    if (resultado != ResultadoDisco::Ok) {
        mensagemErroDisco(resultado, "echo", destino, conteudo.size());
        return;
    }
    std::cout << conteudo.size() << " bytes gravados em " << destino << " (" << arquivo->blocos.size()
              << " bloco(s), alocacao " << DiscoSimulado::modoTexto(disco.modo()) << ")\n";
}

void MiniFileSystem::cat(const std::string& caminho) {
    Entrada* entrada = localizarArquivo("cat", caminho);
    if (!entrada || !verificarPermissao(*entrada->fcb, permissoes::LEITURA, "cat", caminho)) return;

    FCB& f = *entrada->fcb;
    f.acesso = std::time(nullptr);
    const std::string conteudo = disco.recuperar(f);
    if (f.tipo == TipoArquivo::Binario) {
        imprimirHex(conteudo);
        return;
    }
    std::cout << conteudo;
    if (!conteudo.empty() && conteudo.back() != '\n') std::cout << "\n";
}

//truncate ARQ: zera o arquivo e devolve todos os blocos (como "truncate -s 0").
void MiniFileSystem::truncate(const std::string& caminho) {
    Entrada* entrada = localizarArquivo("truncate", caminho);
    if (!entrada || !verificarPermissao(*entrada->fcb, permissoes::ESCRITA, "truncate", caminho)) return;
    const std::size_t blocos = entrada->fcb->blocos.size();
    disco.armazenar(*entrada->fcb, "");  //tamanho 0 nunca falta espaco
    std::cout << caminho << " truncado para 0 bytes (" << blocos << " bloco(s) liberados)\n";
}

void MiniFileSystem::info(const std::string& caminho) const {
    Entrada* entrada = localizarArquivo("info", caminho);
    if (!entrada) return;
    const FCB& f = *entrada->fcb;
    auto simNao = [&](unsigned int bit) {
        return permissoes::permite(f, usuarioAtual, grupoAtual, bit) ? "sim" : "nao";
    };

    std::cout << "  Arquivo: " << entrada->nome << "\n"
              << "  Nome no FCB: " << f.nome << "\n"
              << "  Inode (ID unico): " << f.id << "   Links: " << f.links << "\n"
              << "  Tamanho: " << f.tamanho << " bytes   Blocos: " << f.blocos.size() << " de "
              << DiscoSimulado::TAMANHO_BLOCO << " bytes\n"
              << "  Tipo: " << tipoParaTexto(f.tipo) << "\n"
              << "  Permissoes: " << permissoes::paraOctal(f.permissoes) << " (-"
              << permissoes::paraTexto(f.permissoes) << ")   Dono: " << f.owner
              << "   Grupo: " << f.group << "\n"
              << "  Criacao:       " << dataHora(f.criacao) << "\n"
              << "  Modificacao:   " << dataHora(f.modificacao) << "\n"
              << "  Ultimo acesso: " << dataHora(f.acesso) << "\n"
              << "  Usuario atual '" << usuarioAtual << "' (grupo '" << grupoAtual << "'): classe "
              << permissoes::nomeClasse(permissoes::classe(f, usuarioAtual, grupoAtual))
              << " -> r=" << simNao(permissoes::LEITURA) << " w=" << simNao(permissoes::ESCRITA)
              << " x=" << simNao(permissoes::EXECUCAO) << "\n";
}

void MiniFileSystem::cp(const std::string& origem, const std::string& destino) {
    Entrada* entrada = localizarArquivo("cp", origem);
    if (!entrada || !verificarPermissao(*entrada->fcb, permissoes::LEITURA, "cp", origem)) return;
    std::shared_ptr<FCB> original = entrada->fcb;

    Diretorio* dir = nullptr;
    std::string nome;
    if (!resolverDestino("cp", destino, entrada->nome, dir, nome)) return;
    if (dir->buscarFilho(nome)) {
        std::cout << "cp: " << destino << ": E um diretorio\n";
        return;
    }

    Entrada* existente = dir->buscarEntrada(nome);
    if (existente && existente->fcb == original) {
        std::cout << "cp: '" << origem << "' e '" << destino << "' sao o mesmo arquivo\n";
        return;
    }
    if (existente && !verificarPermissao(*existente->fcb, permissoes::ESCRITA, "cp", destino)) return;

    const std::string conteudo = disco.recuperar(*original);
    original->acesso = std::time(nullptr);

    //destino existente: sobrescreve o conteudo dele; senao cria um FCB novo
    std::shared_ptr<FCB> alvo = existente ? existente->fcb
                                          : criarArquivo(dir, nome, original->tipo, original->permissoes);
    const ResultadoDisco resultado = disco.armazenar(*alvo, conteudo);
    if (resultado != ResultadoDisco::Ok) {
        mensagemErroDisco(resultado, "cp", destino, conteudo.size());
        if (!existente) desvincular(dir, nome);  //desfaz a copia incompleta
        return;
    }
    std::cout << "Copiado: " << origem << " -> " << destino << " (inode " << alvo->id << ", "
              << (existente ? "conteudo sobrescrito" : "novo FCB") << ")\n";
}

void MiniFileSystem::mv(const std::string& origem, const std::string& destino) {
    Diretorio* dirOrigem = nullptr;
    Entrada* entrada = localizarArquivo("mv", origem, &dirOrigem);
    if (!entrada || !verificarDono(*entrada->fcb, "mv", origem)) return;

    Diretorio* dir = nullptr;
    std::string nome;
    if (!resolverDestino("mv", destino, entrada->nome, dir, nome)) return;
    if (dir == dirOrigem && nome == entrada->nome) {
        std::cout << "mv: '" << origem << "' e '" << destino << "' sao o mesmo arquivo\n";
        return;
    }
    if (dir->nomeOcupado(nome)) {
        std::cout << "mv: " << destino << ": Ja existe um item com esse nome no destino (remova-o antes)\n";
        return;
    }

    //mover = criar a entrada nova e apagar a antiga; o FCB (inode) e os blocos sao os mesmos
    std::shared_ptr<FCB> fcb = entrada->fcb;
    const std::string nomeAntigo = entrada->nome;
    dir->arquivos.push_back({nome, fcb});
    dirOrigem->arquivos.remove_if([&](const Entrada& e) { return e.nome == nomeAntigo && e.fcb == fcb; });
    if (fcb->links == 1) fcb->nome = nome;
    std::cout << "Movido: " << origem << " -> " << destino << " (mesmo inode " << fcb->id << ")\n";
}

void MiniFileSystem::rm(const std::string& caminho) {
    Diretorio* dir = nullptr;
    Entrada* entrada = localizarArquivo("rm", caminho, &dir);
    if (!entrada || !verificarDono(*entrada->fcb, "rm", caminho)) return;

    const int id = entrada->fcb->id;
    const std::size_t blocos = entrada->fcb->blocos.size();
    const int restantes = desvincular(dir, entrada->nome);
    if (restantes > 0)
        std::cout << "Removido: " << caminho << " (inode " << id << " continua existindo: "
                  << restantes << " link(s) restante(s))\n";
    else
        std::cout << "Removido: " << caminho << " (inode " << id << " liberado, " << blocos
                  << " bloco(s) devolvidos ao disco)\n";
}

void MiniFileSystem::ln(const std::string& origem, const std::string& destino) {
    Entrada* entrada = localizarArquivo("ln", origem);
    if (!entrada) return;
    const FCB& f = *entrada->fcb;
    //regra do Linux (protected_hardlinks): ser dono ou ter leitura e escrita
    if (!permissoes::ehDono(f, usuarioAtual) &&
        !(permissoes::permite(f, usuarioAtual, grupoAtual, permissoes::LEITURA) &&
          permissoes::permite(f, usuarioAtual, grupoAtual, permissoes::ESCRITA))) {
        std::cout << "ln: " << origem << ": Operacao nao permitida (e preciso ser dono ou ter leitura e escrita)\n";
        return;
    }

    Diretorio* dir = nullptr;
    std::string nome;
    if (!resolverDestino("ln", destino, entrada->nome, dir, nome)) return;
    if (dir->nomeOcupado(nome)) {
        std::cout << "ln: " << destino << ": Arquivo existe\n";
        return;
    }
    dir->arquivos.push_back({nome, entrada->fcb});
    ++entrada->fcb->links;
    std::cout << "Link fisico criado: " << destino << " -> inode " << entrada->fcb->id << " (agora com "
              << entrada->fcb->links << " links)\n";
}

//------------------------------------------------------------------
//protecao e disco: chmod, user, exec, blocks, alloc
//------------------------------------------------------------------

void MiniFileSystem::chmod(const std::string& modo, const std::string& caminho) {
    unsigned int novo = 0;
    if (!permissoes::deOctal(modo, novo)) {
        std::cout << "chmod: modo invalido '" << modo
                  << "' (use 3 digitos octais de 0 a 7, ex.: 644, 750)\n";
        return;
    }
    Entrada* entrada = localizarArquivo("chmod", caminho);
    if (!entrada || !verificarDono(*entrada->fcb, "chmod", caminho)) return;

    FCB& f = *entrada->fcb;
    std::cout << "Permissoes de " << caminho << ": " << permissoes::paraOctal(f.permissoes) << " ("
              << permissoes::paraTexto(f.permissoes) << ") -> ";
    f.permissoes = novo;
    std::cout << permissoes::paraOctal(novo) << " (" << permissoes::paraTexto(novo) << ")\n";
}

void MiniFileSystem::usuario(const std::string& novoUsuario, const std::string& novoGrupo) {
    if (novoUsuario.size() > 31 || novoGrupo.size() > 31) {
        std::cout << "user: nome muito longo (maximo 31 caracteres)\n";
        return;
    }
    usuarioAtual = novoUsuario;
    grupoAtual = novoGrupo;
    std::cout << "Usuario atual: " << usuarioAtual << " | Grupo: " << grupoAtual << "\n";
}

void MiniFileSystem::executarArquivo(const std::string& caminho) {
    Entrada* entrada = localizarArquivo("exec", caminho);
    if (!entrada || !verificarPermissao(*entrada->fcb, permissoes::EXECUCAO, "exec", caminho)) return;
    FCB& f = *entrada->fcb;
    f.acesso = std::time(nullptr);
    if (f.tipo != TipoArquivo::Programa) {
        std::cout << "exec: " << caminho << ": permissao x concedida, mas o arquivo e do tipo "
                  << tipoParaTexto(f.tipo) << ", nao programa\n";
        return;
    }
    std::cout << "Execucao simulada de " << caminho << " (" << f.tamanho
              << " bytes). Nenhum programa real foi iniciado.\n";
}

void MiniFileSystem::mostrarBlocos(const std::string& caminho) const {
    Entrada* entrada = localizarArquivo("blocks", caminho);
    if (!entrada) return;
    //os indices sao metadados; o conteudo dos blocos so aparece com permissao de leitura
    disco.mostrarBlocos(*entrada->fcb,
                        permissoes::permite(*entrada->fcb, usuarioAtual, grupoAtual, permissoes::LEITURA));
}

void MiniFileSystem::alloc(const std::string& modo) {
    if (modo.empty()) {
        std::cout << "Metodo de alocacao atual: " << DiscoSimulado::modoTexto(disco.modo()) << "\n";
        return;
    }
    if (modo == "indexada") disco.definirModo(ModoAlocacao::Indexada);
    else if (modo == "contigua") disco.definirModo(ModoAlocacao::Contigua);
    else {
        std::cout << "alloc: use 'alloc indexada' ou 'alloc contigua'\n";
        return;
    }
    std::cout << "Metodo de alocacao: " << DiscoSimulado::modoTexto(disco.modo())
              << " (vale para as proximas gravacoes)\n";
}

//------------------------------------------------------------------
//interpretador
//------------------------------------------------------------------

void MiniFileSystem::mostrarAjuda() {
    std::cout <<
        "Diretorios\n"
        "  mkdir CAMINHO             cria um diretorio\n"
        "  rmdir CAMINHO             remove um diretorio vazio\n"
        "  cd [CAMINHO]              muda de diretorio (sem argumento: volta para /)\n"
        "  pwd                       mostra o diretorio atual\n"
        "  ls [CAMINHO]              lista com inode, permissoes, links, dono, tamanho e tipo\n"
        "  tree [CAMINHO]            mostra a arvore de diretorios\n"
        "Arquivos\n"
        "  touch ARQ [TIPO]          cria arquivo vazio (TIPO: caractere, numerico, binario, programa)\n"
        "  echo TEXTO > ARQ          grava (substitui); cria o arquivo se nao existir\n"
        "  echo TEXTO >> ARQ         acrescenta ao final\n"
        "  cat ARQ                   mostra o conteudo (binario em hexadecimal)\n"
        "  truncate ARQ              zera o conteudo e libera os blocos\n"
        "  info ARQ | stat ARQ       mostra o FCB completo\n"
        "  cp ORIGEM DESTINO         copia (novo inode)\n"
        "  mv ORIGEM DESTINO         move/renomeia (mesmo inode)\n"
        "  rm ARQ                    remove a entrada; libera os blocos no ultimo link\n"
        "  ln ORIGEM DESTINO         cria link fisico (hard link)\n"
        "Protecao\n"
        "  chmod MODO ARQ            altera permissoes em octal (ex.: chmod 640 notas.txt)\n"
        "  user NOME GRUPO           troca o usuario/grupo atual (ex.: user root root)\n"
        "  whoami                    mostra o usuario e o grupo atuais\n"
        "  exec ARQ                  testa a permissao x e executa (simulado) um programa\n"
        "Disco\n"
        "  disk                      ocupacao e mapa de blocos\n"
        "  blocks ARQ                blocos do arquivo e o pedaco gravado em cada um\n"
        "  alloc [indexada|contigua] mostra ou troca o metodo de alocacao\n"
        "Outros\n"
        "  help                      esta ajuda\n"
        "  exit                      encerra\n"
        "Caminhos podem ser absolutos (/docs/a.txt) ou relativos (../a.txt).\n";
}

bool MiniFileSystem::executarLinha(const std::string& linha) {
    std::istringstream entrada(linha);
    std::vector<std::string> args;
    for (std::string palavra; entrada >> palavra;) args.push_back(palavra);
    if (args.empty()) return true;

    const std::string& cmd = args[0];
    const std::size_t n = args.size() - 1;  //quantidade de argumentos
    auto arg = [&](std::size_t i) { return i < args.size() ? args[i] : std::string(); };
    auto conferir = [&](std::size_t minimo, std::size_t maximo, const char* uso) {
        if (n >= minimo && n <= maximo) return true;
        std::cout << "Uso: " << uso << "\n";
        return false;
    };

    if (cmd == "exit" || cmd == "sair") return false;

    if (cmd == "echo") {
        //o echo usa a linha original, para preservar os espacos do texto
        echo(linha.substr(linha.find("echo") + 4));
    } else if (cmd == "help") {
        mostrarAjuda();
    } else if (cmd == "mkdir") {
        if (conferir(1, 1, "mkdir CAMINHO")) mkdir(arg(1));
    } else if (cmd == "rmdir") {
        if (conferir(1, 1, "rmdir CAMINHO")) rmdir(arg(1));
    } else if (cmd == "cd") {
        if (conferir(0, 1, "cd [CAMINHO]")) cd(arg(1));
    } else if (cmd == "pwd") {
        pwd();
    } else if (cmd == "ls") {
        if (conferir(0, 1, "ls [CAMINHO]")) ls(arg(1));
    } else if (cmd == "tree") {
        if (conferir(0, 1, "tree [CAMINHO]")) tree(arg(1));
    } else if (cmd == "touch") {
        if (conferir(1, 2, "touch ARQ [caractere|numerico|binario|programa]")) touch(arg(1), arg(2));
    } else if (cmd == "cat") {
        if (conferir(1, 1, "cat ARQ")) cat(arg(1));
    } else if (cmd == "truncate") {
        if (conferir(1, 1, "truncate ARQ")) truncate(arg(1));
    } else if (cmd == "info" || cmd == "stat") {
        if (conferir(1, 1, "info ARQ")) info(arg(1));
    } else if (cmd == "cp") {
        if (conferir(2, 2, "cp ORIGEM DESTINO")) cp(arg(1), arg(2));
    } else if (cmd == "mv") {
        if (conferir(2, 2, "mv ORIGEM DESTINO")) mv(arg(1), arg(2));
    } else if (cmd == "rm") {
        if (conferir(1, 1, "rm ARQ")) rm(arg(1));
    } else if (cmd == "ln") {
        if (conferir(2, 2, "ln ORIGEM DESTINO")) ln(arg(1), arg(2));
    } else if (cmd == "chmod") {
        if (conferir(2, 2, "chmod MODO ARQ")) chmod(arg(1), arg(2));
    } else if (cmd == "user" || cmd == "su") {
        if (conferir(2, 2, "user NOME GRUPO")) usuario(arg(1), arg(2));
    } else if (cmd == "whoami") {
        std::cout << usuarioAtual << " (grupo " << grupoAtual << ")\n";
    } else if (cmd == "exec") {
        if (conferir(1, 1, "exec ARQ")) executarArquivo(arg(1));
    } else if (cmd == "disk") {
        disco.mostrarOcupacao();
    } else if (cmd == "blocks") {
        if (conferir(1, 1, "blocks ARQ")) mostrarBlocos(arg(1));
    } else if (cmd == "alloc") {
        if (conferir(0, 1, "alloc [indexada|contigua]")) alloc(arg(1));
    } else {
        std::cout << cmd << ": comando nao encontrado (digite 'help')\n";
    }
    return true;
}

void MiniFileSystem::executar() {
    const bool interativo = isatty(STDIN_FILENO) != 0;
    std::cout << "Mini-Sistema de Arquivos em memoria\n"
              << "Disco simulado: " << DiscoSimulado::TAMANHO_DISCO << " bytes em "
              << DiscoSimulado::QUANTIDADE_BLOCOS << " blocos de " << DiscoSimulado::TAMANHO_BLOCO
              << " bytes. Nenhum arquivo real e alterado.\n"
              << "Digite 'help' para ver os comandos.\n";

    std::string linha;
    while (true) {
        std::cout << usuarioAtual << "@minifs:" << caminhoCompleto(atual) << "$ " << std::flush;
        if (!std::getline(std::cin, linha)) {
            std::cout << "\n";
            break;
        }
        if (!linha.empty() && linha.back() == '\r') linha.pop_back();  //arquivos salvos no Windows
        if (!interativo) std::cout << linha << "\n";  //ao ler de arquivo, mostra o comando
        if (!executarLinha(linha)) break;
    }
    std::cout << "Simulador encerrado. A memoria e liberada automaticamente (RAII).\n";
}
