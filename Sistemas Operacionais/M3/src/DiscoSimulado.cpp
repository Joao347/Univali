#include "DiscoSimulado.h"
#include <algorithm>
#include <iostream>

DiscoSimulado::DiscoSimulado()
    : dados(TAMANHO_BLOCO * QUANTIDADE_BLOCOS, '\0'),
      ocupado(QUANTIDADE_BLOCOS, false) {}

bool DiscoSimulado::armazenar(FCB& arquivo, const std::string& conteudo) {
    const std::size_t necessarios = conteudo.empty()
        ? 0 : (conteudo.size() + TAMANHO_BLOCO - 1) / TAMANHO_BLOCO;

    //primeiro escolhe os blocos. Assim, uma falha nao apaga o conteudo anterior.
    std::vector<std::size_t> novosBlocos;
    novosBlocos.reserve(necessarios);
    for (std::size_t bloco : arquivo.blocos) {
        if (novosBlocos.size() == necessarios) break;
        novosBlocos.push_back(bloco);
    }
    for (std::size_t i = 0; i < QUANTIDADE_BLOCOS && novosBlocos.size() < necessarios; ++i) {
        if (!ocupado[i]) novosBlocos.push_back(i);
    }

    if (novosBlocos.size() != necessarios) {
        std::cout << "Disco simulado cheio: espaco insuficiente.\n";
        return false;
    }

    //libera somente os blocos antigos que nao serao reutilizados.
    for (std::size_t antigo : arquivo.blocos) {
        if (std::find(novosBlocos.begin(), novosBlocos.end(), antigo) == novosBlocos.end()) {
            ocupado[antigo] = false;
            std::fill(dados.begin() + antigo * TAMANHO_BLOCO,
                      dados.begin() + (antigo + 1) * TAMANHO_BLOCO, '\0');
        }
    }

    std::size_t posicao = 0;
    for (std::size_t bloco : novosBlocos) {
        ocupado[bloco] = true;
        std::fill(dados.begin() + bloco * TAMANHO_BLOCO,
                  dados.begin() + (bloco + 1) * TAMANHO_BLOCO, '\0');
        const std::size_t quantidade = std::min(TAMANHO_BLOCO, conteudo.size() - posicao);
        if (quantidade > 0) {
            std::copy_n(conteudo.data() + posicao, quantidade,
                        dados.data() + bloco * TAMANHO_BLOCO);
            posicao += quantidade;
        }
    }

    arquivo.blocos = std::move(novosBlocos);
    arquivo.tamanho = conteudo.size();
    arquivo.modificacao = std::time(nullptr);
    return true;
}

std::string DiscoSimulado::recuperar(const FCB& arquivo) const {
    std::string resultado;
    resultado.reserve(arquivo.tamanho);
    for (std::size_t bloco : arquivo.blocos) {
        const std::size_t restantes = arquivo.tamanho - resultado.size();
        const std::size_t quantidade = std::min(TAMANHO_BLOCO, restantes);
        resultado.append(dados.data() + bloco * TAMANHO_BLOCO, quantidade);
        if (resultado.size() == arquivo.tamanho) break;
    }
    return resultado;
}

void DiscoSimulado::liberar(FCB& arquivo) {
    for (std::size_t bloco : arquivo.blocos) {
        ocupado[bloco] = false;
        std::fill(dados.begin() + bloco * TAMANHO_BLOCO,
                  dados.begin() + (bloco + 1) * TAMANHO_BLOCO, '\0');
    }
    arquivo.blocos.clear();
    arquivo.tamanho = 0;
}

void DiscoSimulado::mostrarOcupacao() const {
    const std::size_t usados = static_cast<std::size_t>(
        std::count(ocupado.begin(), ocupado.end(), true));
    std::cout << "Disco simulado: " << QUANTIDADE_BLOCOS << " blocos de "
              << TAMANHO_BLOCO << " bytes (" << dados.size() << " bytes no total).\n"
              << "Blocos ocupados: " << usados << " | Livres: "
              << QUANTIDADE_BLOCOS - usados << "\n";
}

void DiscoSimulado::mostrarBlocos(const FCB& arquivo) const {
    std::cout << "Arquivo '" << arquivo.nome << "' (ID " << arquivo.id << ") usa os blocos: ";
    if (arquivo.blocos.empty()) std::cout << "(nenhum; arquivo vazio)";
    else {
        for (std::size_t i = 0; i < arquivo.blocos.size(); ++i) {
            if (i) std::cout << ", ";
            std::cout << arquivo.blocos[i];
        }
    }
    std::cout << "\nBytes armazenados: " << arquivo.tamanho << "\n";
}
