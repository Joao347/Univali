# Mini-Sistema de Arquivos em Memória

Trabalho M3 de **Sistemas Operacionais**

Simulador de um sistema de arquivos Unix-like que roda **inteiramente em memória**: diretórios organizados em árvore, arquivos representados por um *File Control Block* (FCB) que faz o papel de inode, controle de acesso RWX para proprietário/grupo/outros com `chmod` numérico e um disco simulado dividido em blocos, com alocação indexada. Nenhum arquivo real do computador é criado ou alterado.

> **Sobre a linguagem:** uso de C++. Os conceitos exigidos continuam presentes — `struct` para o FCB e para os nós da árvore, ponteiros (`Diretorio* pai`, `Diretorio* atual`), `enum` para o tipo do arquivo, bitmasks para permissões e alocação dinâmica. A diferença é que `malloc`/`free` foram substituídos por `std::make_unique`/`std::unique_ptr`, que liberam a memória automaticamente (RAII) e eliminam *memory leaks* por construção.

---

## Sumário

1. [Estrutura do projeto](#1-estrutura-do-projeto)
2. [Como compilar e executar](#2-como-compilar-e-executar)
3. [Comandos do simulador](#3-comandos-do-simulador)
4. [Escolhas de design e estruturas de dados](#4-escolhas-de-design-e-estruturas-de-dados)
5. [Conceitos teóricos demonstrados](#5-conceitos-teóricos-demonstrados)
6. [Exemplo de uso e comparação com o Linux](#6-exemplo-de-uso-e-comparação-com-o-linux)
7. [Tratamento de erros](#7-tratamento-de-erros)
8. [Gerenciamento de memória](#8-gerenciamento-de-memória)
9. [Testes realizados em Linux](#9-testes-realizados-em-linux)
10. [Limitações](#10-limitações)

---

## 1. Estrutura do projeto

```text
M3/
├── include/
│   ├── FCB.h              # File Control Block (o "inode") e enum TipoArquivo
│   ├── Diretorio.h        # Nó da árvore de diretórios
│   ├── DiscoSimulado.h    # Disco em blocos e alocação
│   └── MiniFileSystem.h   # Interface do simulador (comandos)
├── src/
│   ├── DiscoSimulado.cpp  # Gravar, ler e liberar blocos
│   └── MiniFileSystem.cpp # Comandos, navegação, permissões e menu
├── main.cpp               # Ponto de entrada
├── roteiro.txt            # Sessão de testes pronta (./mini_file_system < roteiro.txt)
├── CMakeLists.txt
└── README.md
```

| Módulo | Responsabilidade |
|---|---|
| `FCB.h` | Metadados de um arquivo: nome, tamanho, tipo, ID único, datas, dono, grupo, permissões e lista de blocos. |
| `Diretorio.h` | Nó da árvore: nome, ponteiro para o pai, subdiretórios e arquivos. |
| `DiscoSimulado` | Vetor de 4096 bytes dividido em 256 blocos de 16 bytes, com mapa de blocos livres/ocupados. |
| `MiniFileSystem` | Interpretador de comandos, resolução de caminhos, operações de arquivo e verificação de permissões. |

---

## 2. Como compilar e executar

Requisitos: compilador com suporte a **C++17** (g++ 7+ ou clang 5+). O ambiente indicado é **Linux** ou **WSL**.

### Linux / WSL com g++

Instale o compilador, se necessário (Ubuntu/Debian):

```bash
sudo apt update && sudo apt install -y g++
```

Dentro da pasta `M3`:

```bash
g++ -std=c++17 -Wall -Wextra -pedantic -Iinclude main.cpp src/MiniFileSystem.cpp src/DiscoSimulado.cpp -o mini_file_system
```

```bash
./mini_file_system
```

### Com CMake

```bash
cmake -S . -B build
```

```bash
cmake --build build
```

```bash
./build/mini_file_system
```

### Executando um roteiro de comandos

Como o simulador lê da entrada padrão, também é possível passar um arquivo com comandos (útil para demonstrações):

```bash
./mini_file_system < roteiro.txt
```

### Windows (MinGW)

O mesmo comando `g++` funciona no MinGW; execute com `.\mini_file_system.exe`.

---

## 3. Comandos do simulador

| Comando | Função | Equivalente Linux |
|---|---|---|
| `mkdir NOME` | Cria um diretório no diretório atual | `mkdir` |
| `cd CAMINHO` | Navega; aceita `/`, `..`, `.`, caminhos relativos e absolutos | `cd` |
| `ls` | Lista subdiretórios e arquivos (com permissões, tamanho e ID) | `ls -l` / `ls -i` |
| `pwd` | Mostra o caminho atual | `pwd` |
| `touch ARQUIVO [TIPO]` | Cria arquivo vazio; tipo: `numerico`, `caractere`, `binario` ou `programa` | `touch` |
| `echo ARQUIVO TEXTO` | Grava (substitui) o conteúdo; o texto pode ter espaços | `echo "TEXTO" > ARQUIVO` |
| `cat ARQUIVO` | Lê o conteúdo | `cat` |
| `info ARQUIVO` | Mostra todos os metadados do FCB | `stat` |
| `cp ORIGEM DESTINO` | Copia; destino pode ser nome, caminho ou diretório existente | `cp` |
| `mv ORIGEM DESTINO` | Move/renomeia; destino pode ser nome, caminho ou diretório existente | `mv` |
| `rm ARQUIVO` | Exclui o arquivo e libera seus blocos | `rm` |
| `chmod MODO ARQUIVO` | Altera permissões com 3 dígitos octais (ex.: `640`) | `chmod 640` |
| `user NOME GRUPO` | Troca o usuário/grupo simulado (`root` tem privilégios) | `su` / `sudo` |
| `exec ARQUIVO` | Verifica a permissão de execução (não roda nada de verdade) | `./arquivo` |
| `disk` | Ocupação do disco simulado | `df` |
| `blocks ARQUIVO` | Blocos do disco usados pelo arquivo | `debugfs -R "blocks"` / `filefrag -v` |
| `help` | Ajuda | — |
| `exit` ou `sair` | Encerra | — |

Observações:

- Os comandos de arquivo (`cat`, `echo`, `rm`, `chmod`, `info`, `blocks`, `exec`) procuram o nome **no diretório atual**. `cp` e `mv` aceitam caminho no **destino**.
- Se o tipo não for informado no `touch`, o simulador pergunta interativamente.
- Usuário inicial: `aluno`, grupo `alunos`.

---

## 4. Escolhas de design e estruturas de dados

### 4.1 Árvore de diretórios

```cpp
struct Diretorio {
    std::string nome;
    Diretorio* pai;                                   // ponteiro "para cima" (..)
    std::vector<std::unique_ptr<Diretorio>> diretorios; // filhos (dono da memória)
    std::vector<std::unique_ptr<FCB>> arquivos;         // arquivos deste diretório
};
```

- Cada nó **possui** seus filhos através de `unique_ptr`; o ponteiro `pai` é um ponteiro bruto que **não** possui o pai, apenas aponta para ele. Assim não há ciclos de posse e a destruição da raiz libera a árvore inteira.
- O simulador mantém `raiz` (a `/`) e `atual` (o diretório de trabalho, equivalente ao *cwd* de um processo).
- **Resolução de caminho** (`buscarDiretorio`): o caminho é quebrado por `/`; se começa com `/` a busca parte da raiz, senão de `atual`. `..` sobe pelo ponteiro `pai`, `.` é ignorado. É o mesmo algoritmo de *namei* do Unix, componente por componente.
- **`pwd`** reconstrói o caminho subindo pelos ponteiros `pai` até a raiz.

### 4.2 FCB (inode simulado)

```cpp
enum class TipoArquivo { NUMERICO, CARACTERE, BINARIO, PROGRAMA };

struct FCB {
    unsigned long long id;            // número do inode
    std::string nome;
    std::size_t tamanho;              // bytes
    TipoArquivo tipo;
    std::time_t criacao, modificacao, acesso;
    std::string owner, group;
    int permissoes;                   // bitmask rwxrwxrwx (ex.: 0644)
    std::vector<std::size_t> blocos;  // bloco de índices (alocação indexada)
};
```

Todos os campos pedidos no enunciado (nome, tamanho, tipo como enum, datas em `time_t`, ID único e permissões em bitmask) estão no FCB. O ID vem de um contador global (`proximoId`) que só cresce — IDs nunca são reutilizados, como números de inode durante a vida de um sistema de arquivos simples.

### 4.3 Disco simulado

```cpp
class DiscoSimulado {
    static constexpr std::size_t TAMANHO_BLOCO = 16;
    static constexpr std::size_t QUANTIDADE_BLOCOS = 256;
    std::vector<char> dados;     // o "disco": 4096 bytes contíguos
    std::vector<bool> ocupado;   // mapa de bits de blocos livres
};
```

O disco é um vetor de `char` (o `char[]` sugerido no enunciado) e o mapa `ocupado` é o **bitmap de blocos livres**. O tamanho é pequeno de propósito para que seja fácil ver a divisão em blocos e provocar "disco cheio".

---

## 5. Conceitos teóricos demonstrados

### 5.1 Conceito de arquivo e seus atributos

Um arquivo é um **tipo abstrato de dados**: para o usuário é um nome com conteúdo; para o sistema é um conjunto de atributos mais referências aos dados. No simulador, todos os atributos clássicos aparecem no comando `info` (análogo ao `stat`):

| Atributo (teoria) | Campo no FCB |
|---|---|
| Nome | `nome` |
| Identificador | `id` |
| Tipo | `tipo` (`enum TipoArquivo`) |
| Localização | `blocos` (índices no disco) |
| Tamanho | `tamanho` |
| Proteção | `owner`, `group`, `permissoes` |
| Data e hora | `criacao`, `modificacao`, `acesso` |

O **tipo** é uma classificação guardada no FCB, não deduzida pela extensão (como no Unix, onde `.txt` é só parte do nome). Para demonstrar que o tipo pode ter efeito, arquivos `numerico` só aceitam números (inteiros ou decimais, com ponto ou vírgula, separados por espaço).

### 5.2 Operações com arquivos

| Operação (teoria) | Comando | O que acontece internamente |
|---|---|---|
| Criar | `touch` | Novo FCB com ID único, tamanho 0, sem blocos, permissão `644`, dono = usuário atual. |
| Escrever | `echo` | Verifica `w`; calcula blocos necessários, reserva-os **antes** de liberar os antigos e copia o texto bloco a bloco; atualiza `tamanho` e `modificacao`. |
| Ler | `cat` | Verifica `r`; percorre a lista de blocos remontando o conteúdo até `tamanho` bytes; atualiza `acesso`. |
| Reposicionar / truncar | `echo ARQUIVO` (texto vazio) | Trunca para 0 bytes e devolve todos os blocos ao bitmap. |
| Copiar | `cp` | Verifica `r` na origem; cria **novo FCB** (novo ID, novos blocos) com o mesmo conteúdo, tipo e permissões. |
| Mover / renomear | `mv` | Não copia dados: apenas move o **mesmo FCB** (mesmo ID, mesmos blocos) para outra entrada de diretório — como `rename(2)` no Unix, que só altera entradas de diretório. |
| Excluir | `rm` | Libera os blocos no bitmap e destrói o FCB. |

A diferença entre `cp` e `mv` é visível no simulador: após `cp`, `ls` mostra um ID novo; após `mv`, o ID e os blocos (`blocks`) continuam os mesmos.

### 5.3 FCB e o "inode" simulado

No Unix, o diretório guarda apenas pares *nome → número do inode*, e o inode guarda os metadados e os ponteiros para os blocos. Aqui o FCB cumpre o papel do inode: o diretório guarda os FCBs, e o FCB guarda os metadados e a lista de blocos. O campo `id` é o número do inode (o que `ls -i` mostraria). Ver `info` e `blocks` lado a lado mostra exatamente essa separação entre **metadados** e **dados**.

### 5.4 Estrutura de diretórios em árvore

A estrutura usada é a **árvore** (diretório de níveis arbitrários), e suas vantagens aparecem na prática:

- **Eficiência de busca:** para achar um arquivo só é preciso percorrer os componentes do caminho, não todos os arquivos do sistema.
- **Nomeação:** o mesmo nome pode existir em diretórios diferentes (`/documentos/notas.txt` e `/backup/notas.txt`); unicidade só é exigida dentro de um diretório (`nomeOcupado`).
- **Agrupamento:** arquivos relacionados ficam no mesmo diretório; há caminhos **absolutos** (a partir de `/`) e **relativos** (a partir de `atual`), além de `.` e `..`.

### 5.5 Proteção de acesso: RWX, proprietário/grupo/outros e `chmod` com bitmask

As permissões são um inteiro de 9 bits, três para cada classe:

```text
  owner   group   others
  r w x   r w x   r w x
  8 7 6   5 4 3   2 1 0   ← posição do bit
  4 2 1   4 2 1   4 2 1   ← valor octal de cada bit
```

**`chmod` numérico** — cada dígito octal vira 3 bits:

```cpp
// "640" -> 6*64 + 4*8 + 0 = 0640 = 110 100 000 (rw- r-- ---)
return (modo[0] - '0') * 64 + (modo[1] - '0') * 8 + (modo[2] - '0');
```

**Escolha da classe** — deslocamento e máscara `& 7` isolam os 3 bits da classe do usuário atual:

```cpp
if (usuarioAtual == arquivo.owner) return (arquivo.permissoes >> 6) & 7; // owner
if (grupoAtual   == arquivo.group) return (arquivo.permissoes >> 3) & 7; // group
return arquivo.permissoes & 7;                                           // others
```

**Verificação** — antes de cada operação, um `AND` com o bit da operação (`r = 4`, `w = 2`, `x = 1`) decide se ela prossegue:

```cpp
if ((permissoesDoUsuario(arquivo) & bit) != 0) return true;
std::cout << "Permissao negada: ...";
```

| Operação | Bit verificado |
|---|---|
| `cat`, `cp` (origem) | `r` (4) |
| `echo` | `w` (2) |
| `exec` | `x` (1) |
| `chmod`, `mv`, `rm` | ser o proprietário ou `root` |

Como no Unix, **apenas uma classe é aplicada**: se você é o dono, valem os bits do dono, mesmo que "outros" tenha mais permissões. O "usuário atual" é a variável `usuarioAtual`/`grupoAtual`, trocada com `user`. O usuário **`root`** segue a regra do Linux: lê e escreve qualquer arquivo e só executa se algum bit `x` estiver ligado.

A verificação ocorre em **toda** tentativa de acesso e a operação é bloqueada no próprio simulador, independentemente da vontade do usuário — é a ideia de **bloqueio obrigatório** (*mandatory*) pedida no enunciado.

Em `rm` e `mv` foi adotada a regra "somente o dono ou root", uma simplificação do *sticky bit* (no Linux real, excluir depende da permissão `w` do diretório, que aqui não é simulada).

### 5.6 Simulação da alocação de blocos

O enunciado cita três métodos; o simulador implementa a **alocação indexada**:

| Método | Como funciona | Prós | Contras |
|---|---|---|---|
| Contígua | Arquivo ocupa blocos consecutivos; FCB guarda início + tamanho | Acesso sequencial e direto rápidos | Fragmentação externa; difícil crescer o arquivo |
| Encadeada | Cada bloco aponta para o próximo | Sem fragmentação externa | Acesso direto lento; um ponteiro corrompido perde o resto |
| **Indexada (usada)** | Um **bloco de índices** lista todos os blocos do arquivo | Acesso direto; sem fragmentação externa; arquivo cresce fácil | Espaço extra para o índice |

No simulador, o vetor `FCB::blocos` é o bloco de índices. Exemplo com um texto de 64 bytes e blocos de 16 bytes:

```text
Conteúdo: "Este texto tem mais de dezesseis bytes para ocupar varios blocos" (64 bytes)

FCB mensagem.txt                     Disco (256 blocos x 16 bytes)
┌───────────────────┐                ┌────┬──────────────────┐
│ id = 2            │                │  0 │ (notas.txt)      │
│ tamanho = 64      │      ┌───────► │  1 │ "Este texto tem m"│
│ blocos = [1,2,3,4]│ ─────┼───────► │  2 │ "ais de dezesseis"│
└───────────────────┘      ├───────► │  3 │ " bytes para ocup"│
                           └───────► │  4 │ "ar varios blocos"│
                                     │  5 │ livre            │
                                     └────┴──────────────────┘
```

Detalhes do algoritmo (`DiscoSimulado::armazenar`):

1. Calcula `ceil(tamanho / 16)` blocos necessários.
2. Reaproveita os blocos que o arquivo já tinha e procura os demais no bitmap (*first-fit*). Como a alocação é indexada, os blocos **não precisam ser contíguos** — após exclusões, um arquivo novo pode receber blocos espalhados, e `blocks` mostra isso.
3. Se não houver espaço, a gravação é cancelada **sem perder o conteúdo antigo** (os blocos novos são escolhidos antes de liberar os antigos).
4. O último bloco pode ficar parcialmente ocupado (**fragmentação interna**); por isso a leitura usa `tamanho` para saber onde parar.

`disk` mostra o total de blocos ocupados e livres, e `rm` devolve os blocos ao bitmap.

---

## 6. Exemplo de uso e comparação com o Linux

Sessão real do simulador (o cabeçalho que se repete a cada comando foi omitido):

```text
/ $ mkdir documentos
Diretorio criado: documentos
/ $ cd documentos
Diretorio atual: /documentos
/documentos $ touch notas.txt numerico
Arquivo criado: notas.txt | Tipo: numerico (permissao inicial 644).
/documentos $ echo notas.txt 10 20,5 30
Conteudo gravado em notas.txt.
/documentos $ echo notas.txt dez
Conteudo invalido: arquivo numerico aceita apenas numeros.
/documentos $ touch mensagem.txt caractere
Arquivo criado: mensagem.txt | Tipo: caractere (permissao inicial 644).
/documentos $ echo mensagem.txt Este texto tem mais de dezesseis bytes para ocupar varios blocos
Conteudo gravado em mensagem.txt.
/documentos $ blocks mensagem.txt
Arquivo 'mensagem.txt' (ID 2) usa os blocos: 1, 2, 3, 4
Bytes armazenados: 64
/documentos $ info notas.txt
Nome: notas.txt
ID unico: 1
Tamanho: 10 bytes
Tipo: numerico
Criacao: 18/11/2026 23:10:27
Modificacao: 18/11/2026 23:10:27
Ultimo acesso: 18/11/2026 23:10:27
Proprietario: aluno
Grupo: alunos
Permissoes: 644 (rw-r--r--)
/documentos $ chmod 640 notas.txt
Permissoes de notas.txt alteradas para 640 (rw-r-----).
/documentos $ user visitante visitantes
Usuario atual: visitante | Grupo atual: visitantes
/documentos $ cat notas.txt
Permissao negada: usuario 'visitante' nao pode ler o arquivo 'notas.txt'.
/documentos $ user colega alunos
Usuario atual: colega | Grupo atual: alunos
/documentos $ cat notas.txt
Conteudo de notas.txt:
10 20,5 30
/documentos $ echo notas.txt 5
Permissao negada: usuario 'colega' nao pode escrever o arquivo 'notas.txt'.
/documentos $ user aluno alunos
Usuario atual: aluno | Grupo atual: alunos
/documentos $ mkdir backup
Diretorio criado: backup
/documentos $ cp mensagem.txt copia.txt
Arquivo copiado para /documentos/copia.txt.
/documentos $ mv mensagem.txt backup/renomeado.txt
Arquivo movido/renomeado para /documentos/backup/renomeado.txt.
/documentos $ ls
Conteudo de /documentos:
  [DIR]  backup
  [FILE] rw-r----- notas.txt (10 bytes, ID 1)
  [FILE] rw-r--r-- copia.txt (64 bytes, ID 3)
/documentos $ disk
Disco simulado: 256 blocos de 16 bytes (4096 bytes no total).
Blocos ocupados: 9 | Livres: 247
```

Note que `copia.txt` recebeu um **ID novo** (3) e blocos próprios, enquanto `renomeado.txt` manteve o **ID 2** e os blocos 1–4 do original.

### O mesmo roteiro no Linux real

```bash
mkdir documentos && cd documentos
echo "10 20,5 30" > notas.txt
stat notas.txt                 # Inode, Size, Access: (0644/-rw-r--r--), Uid, Gid, datas
chmod 640 notas.txt
ls -l notas.txt                # -rw-r----- 1 aluno alunos 11 ... notas.txt
sudo -u visitante cat notas.txt   # cat: notas.txt: Permission denied
cp notas.txt copia.txt && ls -i   # copia.txt tem inode diferente
mv copia.txt backup.txt && ls -i  # backup.txt mantém o inode de copia.txt
```

| Situação | Simulador | Linux |
|---|---|---|
| Metadados | `info` mostra ID, tamanho, tipo, datas, dono, grupo, modo | `stat` mostra inode, tamanho, datas, Uid/Gid, modo |
| Permissões em texto | `rw-r-----` no `ls` e no `info` | `-rw-r-----` no `ls -l` (o 1º caractere é o tipo) |
| Acesso negado | `Permissao negada: ...` | `Permission denied` |
| `cp` | Novo ID, novos blocos | Novo inode, novos blocos |
| `mv` no mesmo FS | Mesmo ID, mesmos blocos | Mesmo inode (só muda a entrada de diretório) |
| `root` | Ignora `r`/`w`; precisa de algum `x` para executar | Igual |
| Tamanho de `echo` | Não adiciona `\n` (10 bytes) | `echo` adiciona `\n` (11 bytes) |
| Blocos | 16 bytes, visíveis com `blocks` | Normalmente 4096 bytes, visíveis com `filefrag -v`/`debugfs` |

---

## 7. Tratamento de erros

| Situação | Mensagem |
|---|---|
| Arquivo inexistente | `Arquivo nao encontrado.` |
| Diretório inexistente | `Diretorio nao encontrado.` |
| Nome inválido (vazio, `.`, `..` ou contendo `/`) | `Nome de arquivo invalido.` / `Nome de diretorio invalido.` |
| Nome já usado no diretório | `Ja existe um item com esse nome.` |
| Sem permissão de leitura/escrita/execução | `Permissao negada: usuario 'X' nao pode ler/escrever/executar o arquivo 'Y'.` |
| Não é dono nem root (`chmod`/`mv`/`rm`) | `Permissao negada: somente o proprietario ou root pode ...` |
| Modo do `chmod` inválido (ex.: `999`, `64`) | `Modo invalido. Use tres digitos octais ...` |
| Tipo de arquivo desconhecido | `Tipo invalido. Use: numerico, caractere, binario ou programa.` |
| Conteúdo não numérico em arquivo `numerico` | `Conteudo invalido: arquivo numerico aceita apenas numeros.` |
| Disco cheio | `Disco simulado cheio: espaco insuficiente.` (conteúdo anterior preservado) |
| Destino de `cp`/`mv` inválido | `Diretorio ou nome de destino invalido.` |
| Argumentos faltando | `Uso: ...` |
| Comando inexistente | `Comando desconhecido. Digite 'help'.` |

Fim da entrada (Ctrl+D ou fim do arquivo de roteiro) encerra o programa normalmente.

---

## 8. Gerenciamento de memória

- Toda alocação dinâmica é feita com `std::make_unique` e a posse fica em `std::unique_ptr` (o raiz no `MiniFileSystem`, os filhos e os FCBs nos vetores do `Diretorio`).
- `rm` apaga o `unique_ptr` do vetor → o FCB é destruído na hora; `mv` **transfere** a posse com `std::move`, sem copiar nem vazar.
- Ao sair, destruir `raiz` destrói recursivamente toda a árvore (RAII). Não há `new`/`delete` manuais.
- Verificação com Valgrind no Linux:

```bash
g++ -std=c++17 -g -Iinclude main.cpp src/*.cpp -o mini_file_system
valgrind --leak-check=full ./mini_file_system < roteiro.txt
```

Resultado esperado: `All heap blocks were freed -- no leaks are possible`.

---

## 9. Roteiro de testes

O arquivo [`roteiro.txt`](roteiro.txt) contém uma sessão completa que pode ser executada de uma vez:

```bash
./mini_file_system < roteiro.txt
```

### Validação em Linux

O projeto foi compilado e o `roteiro.txt` executado em **Linux x86-64**:

| Compilador | Flags | Resultado |
|---|---|---|
| g++ 9.4 (padrão do Ubuntu 20.04) | `-std=c++17 -Wall -Wextra -pedantic -O2` | compila sem warnings, roteiro OK |
| g++ 11.4 (padrão do Ubuntu 22.04) | idem | compila sem warnings, roteiro OK |
| g++ 13.3 (padrão do Ubuntu 24.04) | idem | compila sem warnings, roteiro OK |
| g++ 16.2 | idem | compila sem warnings, roteiro OK |
| clang++ 20.1 | idem | compila sem warnings, roteiro OK |
| g++ 16.2 e clang++ 20.1 | `-g -fsanitize=address,undefined` | nenhum erro de memória, comportamento indefinido ou vazamento (LeakSanitizer) |

A saída no Linux é idêntica à do Windows (MinGW), exceto pelas datas.

O roteiro exercita:

- `mkdir` (incluindo nome duplicado e nome com `/`), `cd` com caminhos relativos, absolutos, `..` e inexistentes, `pwd`, `ls`;
- `touch` com os quatro tipos, tipo inválido e tipo perguntado interativamente;
- `echo`/`cat` com conteúdo de vários blocos, validação de arquivo numérico;
- `chmod` válido e inválido; leitura/escrita negadas para *others* e para *group*; `chmod`/`rm` negados para quem não é dono;
- `root` lendo arquivo `000` e sendo impedido de executar arquivo sem bit `x`; `exec` liberado após `chmod 755`;
- `cp` para nome, para `..` e para diretório; `mv` para diretório e com renomeação; `rm` e liberação de blocos (`disk`/`blocks`);
- comando desconhecido e encerramento por `exit` e por fim da entrada.

---

## 10. Limitações

- Os dados existem apenas durante a execução (não há persistência em arquivo).
- Permissões de **diretórios** não são simuladas; o RWX vale para arquivos.
- `rmdir` e links (*hard*/*symbolic*) não foram implementados.
- `exec` apenas verifica o bit `x`; nenhum programa é executado.
- A troca de usuário (`user`) é didática: não há autenticação.
- Nomes de arquivos não podem conter espaços (o interpretador separa argumentos por espaço).
