#include "DiscoSimulado.h"
#include <algorithm>
#include <cctype>
#include <iostream>

DiscoSimulado::DiscoSimulado() : modoAtual(ModoAlocacao::Indexada) {
    dados.fill('\0');
    ocupado.fill(false);
}

std::size_t DiscoSimulado::blocosNecessarios(std::size_t bytes) {
    //teto(bytes / TAMANHO_BLOCO): o ultimo bloco pode ficar parcialmente vazio
    return (bytes + TAMANHO_BLOCO - 1) / TAMANHO_BLOCO;
}

std::size_t DiscoSimulado::blocosLivres() const {
    return static_cast<std::size_t>(std::count(ocupado.begin(), ocupado.end(), false));
}

std::string DiscoSimulado::modoTexto(ModoAlocacao modo) {
    return modo == ModoAlocacao::Contigua ? "contigua" : "indexada";
}

//indexada: reaproveita os blocos que o arquivo ja tinha e completa com os primeiros livres.
bool DiscoSimulado::escolherIndexada(const FCB& arquivo, std::size_t necessarios,
                                     std::vector<std::size_t>& novos) const {
    for (std::size_t bloco : arquivo.blocos) {
        if (novos.size() == necessarios) break;
        novos.push_back(bloco);
    }
    for (std::size_t i = 0; i < QUANTIDADE_BLOCOS && novos.size() < necessarios; ++i)
        if (!ocupado[i]) novos.push_back(i);
    return novos.size() == necessarios;
}

//contigua (first-fit): primeira sequencia de blocos vizinhos livres ou do proprio arquivo.
bool DiscoSimulado::escolherContigua(const FCB& arquivo, std::size_t necessarios,
                                     std::vector<std::size_t>& novos) const {
    std::size_t sequencia = 0;
    for (std::size_t i = 0; i < QUANTIDADE_BLOCOS; ++i) {
        const bool doArquivo = std::find(arquivo.blocos.begin(), arquivo.blocos.end(), i)
                               != arquivo.blocos.end();
        sequencia = (!ocupado[i] || doArquivo) ? sequencia + 1 : 0;
        if (sequencia == necessarios) {
            for (std::size_t b = i + 1 - necessarios; b <= i; ++b) novos.push_back(b);
            return true;
        }
    }
    return false;
}

void DiscoSimulado::zerarBloco(std::size_t bloco) {
    ocupado[bloco] = false;
    std::fill_n(dados.begin() + bloco * TAMANHO_BLOCO, TAMANHO_BLOCO, '\0');
}

ResultadoDisco DiscoSimulado::armazenar(FCB& arquivo, const std::string& conteudo) {
    const std::size_t necessarios = blocosNecessarios(conteudo.size());

    //1) escolhe os blocos sem alterar nada; uma falha preserva o conteudo anterior
    std::vector<std::size_t> novos;
    novos.reserve(necessarios);
    if (necessarios > 0) {
        if (blocosLivres() + arquivo.blocos.size() < necessarios) return ResultadoDisco::SemEspaco;
        const bool ok = modoAtual == ModoAlocacao::Contigua
            ? escolherContigua(arquivo, necessarios, novos)
            : escolherIndexada(arquivo, necessarios, novos);
        if (!ok) return ResultadoDisco::Fragmentado;
    }

    //2) libera somente os blocos antigos que nao serao reutilizados
    for (std::size_t antigo : arquivo.blocos)
        if (std::find(novos.begin(), novos.end(), antigo) == novos.end()) zerarBloco(antigo);

    //3) divide o conteudo em pedacos de TAMANHO_BLOCO e grava cada um no seu bloco
    for (std::size_t i = 0; i < novos.size(); ++i) {
        const std::size_t bloco = novos[i];
        const std::size_t inicio = i * TAMANHO_BLOCO;
        const std::size_t quantidade = std::min(TAMANHO_BLOCO, conteudo.size() - inicio);
        auto destino = dados.begin() + bloco * TAMANHO_BLOCO;
        std::fill_n(destino, TAMANHO_BLOCO, '\0');
        std::copy_n(conteudo.data() + inicio, quantidade, destino);
        ocupado[bloco] = true;
    }

    //4) o FCB passa a referenciar a nova lista de blocos
    arquivo.blocos = std::move(novos);
    arquivo.tamanho = conteudo.size();
    arquivo.modificacao = std::time(nullptr);
    return ResultadoDisco::Ok;
}

std::string DiscoSimulado::recuperar(const FCB& arquivo) const {
    std::string resultado;
    resultado.reserve(arquivo.tamanho);
    for (std::size_t bloco : arquivo.blocos) {
        const std::size_t quantidade = std::min(TAMANHO_BLOCO, arquivo.tamanho - resultado.size());
        resultado.append(dados.data() + bloco * TAMANHO_BLOCO, quantidade);
        if (resultado.size() == arquivo.tamanho) break;
    }
    return resultado;
}

void DiscoSimulado::liberar(FCB& arquivo) {
    for (std::size_t bloco : arquivo.blocos) zerarBloco(bloco);
    arquivo.blocos.clear();
    arquivo.tamanho = 0;
}

void DiscoSimulado::mostrarOcupacao() const {
    const std::size_t livres = blocosLivres();
    std::cout << "Disco simulado: " << QUANTIDADE_BLOCOS << " blocos de " << TAMANHO_BLOCO
              << " bytes (" << TAMANHO_DISCO << " bytes no total)\n"
              << "Metodo de alocacao: " << modoTexto(modoAtual) << "\n"
              << "Blocos ocupados: " << QUANTIDADE_BLOCOS - livres << " | Livres: " << livres
              << " (" << livres * TAMANHO_BLOCO << " bytes livres)\n"
              << "Mapa ('#' ocupado, '.' livre; 64 blocos por linha):\n";
    for (std::size_t linha = 0; linha < QUANTIDADE_BLOCOS / 64; ++linha) {
        std::string faixa = std::to_string(linha * 64) + "-" + std::to_string(linha * 64 + 63);
        std::cout << "  " << std::string(8 - faixa.size(), ' ') << faixa << "  ";
        for (std::size_t c = 0; c < 64; ++c) std::cout << (ocupado[linha * 64 + c] ? '#' : '.');
        std::cout << "\n";
    }
}

void DiscoSimulado::mostrarBlocos(const FCB& arquivo, bool mostrarConteudo) const {
    std::cout << "Arquivo '" << arquivo.nome << "' (inode " << arquivo.id << "): "
              << arquivo.tamanho << " bytes em " << arquivo.blocos.size() << " bloco(s) de "
              << TAMANHO_BLOCO << " bytes\n";
    if (arquivo.blocos.empty()) {
        std::cout << "  (arquivo vazio: nenhum bloco alocado)\n";
        return;
    }

    std::cout << "Bloco de indices no FCB: [";
    for (std::size_t i = 0; i < arquivo.blocos.size(); ++i)
        std::cout << (i ? ", " : "") << arquivo.blocos[i];
    std::cout << "]\n";

    if (!mostrarConteudo) {
        std::cout << "  (conteudo dos blocos oculto: sem permissao de leitura)\n";
        return;
    }

    std::size_t restantes = arquivo.tamanho;
    for (std::size_t i = 0; i < arquivo.blocos.size(); ++i) {
        const std::size_t usados = std::min(TAMANHO_BLOCO, restantes);
        const std::size_t bloco = arquivo.blocos[i];
        std::string trecho;
        for (std::size_t j = 0; j < TAMANHO_BLOCO; ++j) {
            const unsigned char c = static_cast<unsigned char>(dados[bloco * TAMANHO_BLOCO + j]);
            trecho += j >= usados ? ' ' : (std::isprint(c) ? static_cast<char>(c) : '.');
        }
        std::string idx = std::to_string(i), num = std::to_string(bloco);
        std::cout << "  indice " << std::string(idx.size() < 2 ? 2 - idx.size() : 0, ' ') << idx
                  << " -> bloco " << std::string(num.size() < 3 ? 3 - num.size() : 0, ' ') << num
                  << ": |" << trecho << "| " << usados << " byte(s)"
                  << (usados < TAMANHO_BLOCO ? " (fragmentacao interna)" : "") << "\n";
        restantes -= usados;
    }
}
