#ifndef MINI_FILE_SYSTEM_H
#define MINI_FILE_SYSTEM_H

#include "Diretorio.h"
#include "DiscoSimulado.h"
#include <memory>
#include <string>
#include <vector>

class MiniFileSystem {
private:
    std::unique_ptr<Diretorio> raiz;
    Diretorio* atual;
    unsigned long long proximoId;
    std::string usuarioAtual;
    std::string grupoAtual;
    DiscoSimulado disco;

    static bool nomeValido(const std::string& nome);
    static std::string dataHora(std::time_t instante);
    static bool modoValido(const std::string& modo);
    static int modoParaInteiro(const std::string& modo);
    static std::string modoTexto(int modo);
    static std::string modoOctal(int modo);
    Diretorio* buscarDiretorio(Diretorio* base, const std::string& caminho) const;
    bool separarDestino(const std::string& caminho, Diretorio*& destino, std::string& nome) const;
    FCB* buscarArquivo(Diretorio* diretorio, const std::string& nome) const;
    bool nomeOcupado(Diretorio* diretorio, const std::string& nome) const;
    std::string caminhoCompleto(const Diretorio* diretorio) const;
    int permissoesDoUsuario(const FCB& arquivo) const;
    bool pode(const FCB& arquivo, int bit, const std::string& operacao) const;
    static std::string lerCampo(const std::string& mensagem);
    void mostrarAjuda() const;
    void mostrarMenu() const;

public:
    MiniFileSystem();
    void mkdir(const std::string& nome);
    void cd(const std::string& caminho);
    void pwd() const;
    void ls() const;
    void touch(const std::string& nome);
    void echo(const std::string& nome, const std::string& texto);
    void cat(const std::string& nome);
    void info(const std::string& nome) const;
    void cp(const std::string& origem, const std::string& destino);
    void mv(const std::string& origem, const std::string& destino);
    void rm(const std::string& nome);
    void chmod(const std::string& modo, const std::string& nome);
    void usuario(const std::string& usuario, const std::string& grupo);
    void executarArquivo(const std::string& nome);
    void mostrarDisco() const;
    void mostrarBlocos(const std::string& nome) const;
    void executar();
};

#endif
