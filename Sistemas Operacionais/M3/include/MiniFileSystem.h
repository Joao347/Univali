#ifndef MINI_FILE_SYSTEM_H
#define MINI_FILE_SYSTEM_H

#include "Diretorio.h"
#include "DiscoSimulado.h"
#include <memory>
#include <string>
#include <vector>

class MiniFileSystem {
public:
    MiniFileSystem();

    //interpreta uma linha de comando; retorna false quando o usuario pede para sair.
    bool executarLinha(const std::string& linha);

    //laco interativo: le comandos da entrada padrao ate "exit" ou fim da entrada.
    void executar();

private:
    enum class ResultadoCaminho { Ok, PaiInexistente, NomeInvalido };

    std::unique_ptr<Diretorio> raiz;
    Diretorio* atual;
    int proximoId;
    std::string usuarioAtual;
    std::string grupoAtual;
    DiscoSimulado disco;

    //caminhos e busca na arvore
    static bool nomeValido(const std::string& nome);
    Diretorio* buscarDiretorio(const std::string& caminho) const;
    ResultadoCaminho separarCaminho(const std::string& caminho, Diretorio*& pai,
                                    std::string& nome) const;
    Entrada* localizarArquivo(const std::string& cmd, const std::string& caminho,
                              Diretorio** dirSaida = nullptr) const;
    bool resolverDestino(const std::string& cmd, const std::string& destino,
                         const std::string& nomeOrigem, Diretorio*& dir, std::string& nome) const;
    std::string caminhoCompleto(const Diretorio* diretorio) const;

    //protecao
    bool verificarPermissao(const FCB& arquivo, unsigned int bit, const std::string& cmd,
                            const std::string& caminho) const;
    bool verificarDono(const FCB& arquivo, const std::string& cmd,
                       const std::string& caminho) const;

    //apoio as operacoes
    std::shared_ptr<FCB> criarArquivo(Diretorio* dir, const std::string& nome, TipoArquivo tipo,
                                      unsigned int modo);
    int desvincular(Diretorio* dir, const std::string& nome);
    void mensagemErroDisco(ResultadoDisco resultado, const std::string& cmd,
                           const std::string& caminho, std::size_t bytes) const;
    static std::string dataHora(std::time_t instante);
    void imprimirLinhaArquivo(const Entrada& entrada) const;
    void imprimirArvore(const Diretorio* dir, int nivel) const;
    static void mostrarAjuda();

    //comandos de diretorio
    void mkdir(const std::string& caminho);
    void rmdir(const std::string& caminho);
    void cd(const std::string& caminho);
    void pwd() const;
    void ls(const std::string& caminho) const;
    void tree(const std::string& caminho) const;

    //comandos de arquivo
    void touch(const std::string& caminho, const std::string& tipo);
    void echo(const std::string& argumentos);
    void cat(const std::string& caminho);
    void truncate(const std::string& caminho);
    void info(const std::string& caminho) const;
    void cp(const std::string& origem, const std::string& destino);
    void mv(const std::string& origem, const std::string& destino);
    void rm(const std::string& caminho);
    void ln(const std::string& origem, const std::string& destino);

    //protecao e disco
    void chmod(const std::string& modo, const std::string& caminho);
    void usuario(const std::string& usuario, const std::string& grupo);
    void executarArquivo(const std::string& caminho);
    void mostrarBlocos(const std::string& caminho) const;
    void alloc(const std::string& modo);
};

#endif
