# Mini File System – Implementação e Análise de um Mini-Sistema de Arquivos em Memória

**Sistemas Operacionais – Trabalho M3 – UNIVALI**

Simulador de um sistema de arquivos Unix-like que funciona inteiramente em memória, escrito em C++17. Ele implementa:

- uma árvore de diretórios;
- arquivos representados por um File Control Block (FCB) que faz o papel de inode;
- permissões RWX para proprietário, grupo e outros, guardadas em máscaras de bits;
- um disco simulado (array de caracteres) dividido em blocos, com alocação indexada ou contígua.

Nenhum arquivo real do computador é criado ou alterado.

## Requisitos do enunciado e onde estão

| Requisito | Onde está no código |
| --- | --- |
| 3.1 Árvore de diretórios (binária: primeiro filho / próximo irmão), `mkdir` e `cd` | `include/Diretorio.h`; comandos `mkdir`, `rmdir`, `cd`, `pwd`, `ls`, `tree` |
| 3.2 FCB com nome, tamanho, tipo (enum), datas (`time_t`), ID único e permissões | `include/FCB.h` (`struct FCB`, `enum class TipoArquivo`) |
| 3.2 `touch`, `echo >`, `cat`, `cp`, `mv`, `rm` | `src/MiniFileSystem.cpp` |
| 3.3 RWX para owner/group/others com bitmask e `chmod` numérico | `include/Permissoes.h`, `src/Permissoes.cpp`; comando `chmod` |
| 3.3 Definir e exibir permissões | `chmod`, `ls`, `info` |
| 3.3 Verificar o "usuário atual" em toda operação | `verificarPermissao()` e `verificarDono()`; comandos `user` e `whoami` |
| 3.4 Disco como array de `char` e método de alocação | `include/DiscoSimulado.h`, `src/DiscoSimulado.cpp`; comandos `disk`, `blocks`, `alloc` |
| 5.2 Interface por linha de comando | prompt interativo (`aluno@minifs:/$`); também lê um roteiro pela entrada padrão |
| 5.3 Tratamento de erros | mensagens no formato do Linux (`cat: x: Arquivo ou diretorio nao encontrado`) |
| 5.4 Código comentado e gestão de memória | `unique_ptr`, `shared_ptr` e RAII; Valgrind sem vazamentos |
| Bônus: links | `ln` cria links físicos (hard links) com contador de links no FCB |

## Estrutura do projeto

```text
M3/
|-- include/
|   |-- FCB.h              FCB (inode simulado) e enum de tipos
|   |-- Permissoes.h       constantes e funções de bitmask RWX
|   |-- DiscoSimulado.h    disco simulado e métodos de alocação
|   |-- Diretorio.h        nó da árvore binária e entradas de arquivo
|   `-- MiniFileSystem.h   estado do sistema e comandos
|-- src/
|   |-- Permissoes.cpp
|   |-- DiscoSimulado.cpp
|   `-- MiniFileSystem.cpp todos os comandos e o interpretador
|-- testes/
|   |-- demo.txt           roteiro que passa por todos os requisitos
|   |-- erros.txt          casos de erro
|   `-- fragmentacao.txt   alocação contígua x indexada
|-- main.cpp
|-- CMakeLists.txt
`-- README.md
```

## Como compilar e executar

O projeto usa só a biblioteca padrão. Foi testado no Ubuntu 24.04 com g++ 13.3 e clang++ 18, sem nenhum aviso com `-Wall -Wextra -pedantic`.

### g++ (Linux, WSL ou MinGW)

Na pasta do projeto:

```bash
g++ -std=c++17 -Wall -Wextra -pedantic -Iinclude main.cpp src/*.cpp -o mini_file_system
```

Execute no Linux/WSL:

```bash
./mini_file_system
```

No Windows com MinGW:

```powershell
.\mini_file_system.exe
```

### CMake

```bash
cmake -S . -B build
cmake --build build
./build/mini_file_system
```

### Executando um roteiro

O simulador também lê comandos de um arquivo. Nesse caso, cada comando é exibido depois do prompt, o que deixa a saída legível:

```bash
./mini_file_system < testes/demo.txt
./mini_file_system < testes/erros.txt
./mini_file_system < testes/fragmentacao.txt
```

### Verificando a memória (opcional)

```bash
valgrind --leak-check=full ./mini_file_system < testes/demo.txt > /dev/null
```

## Comandos

| Comando | Função | Equivalente no Linux |
| --- | --- | --- |
| `mkdir CAMINHO` | cria diretório (o pai precisa existir) | `mkdir` |
| `rmdir CAMINHO` | remove diretório vazio | `rmdir` |
| `cd [CAMINHO]` | muda de diretório; sem argumento volta para `/` | `cd` |
| `pwd` | mostra o diretório atual | `pwd` |
| `ls [CAMINHO]` | lista inode, permissões, links, dono, grupo, tamanho, tipo e data | `ls -li` |
| `tree [CAMINHO]` | desenha a árvore | `tree` |
| `touch ARQ [TIPO]` | cria arquivo vazio; TIPO = `caractere`, `numerico`, `binario` ou `programa` | `touch` |
| `echo TEXTO > ARQ` | grava, substituindo o conteúdo (cria o arquivo se não existir) | `echo >` |
| `echo TEXTO >> ARQ` | acrescenta ao final | `echo >>` |
| `cat ARQ` | mostra o conteúdo (binário em hexadecimal) | `cat` / `xxd` |
| `truncate ARQ` | zera o conteúdo e libera os blocos | `truncate -s 0` |
| `info ARQ` ou `stat ARQ` | mostra o FCB completo | `stat` |
| `cp ORIGEM DESTINO` | copia (cria um FCB novo) | `cp` |
| `mv ORIGEM DESTINO` | move ou renomeia (mesmo FCB) | `mv` |
| `rm ARQ` | remove a entrada; libera os blocos quando o último link some | `rm` |
| `ln ORIGEM DESTINO` | cria link físico | `ln` |
| `chmod MODO ARQ` | altera permissões em octal (`640`, `0755`) | `chmod` |
| `user NOME GRUPO` | troca o usuário atual (`su` também funciona) | `su` |
| `whoami` | mostra usuário e grupo atuais | `whoami`, `id` |
| `exec ARQ` | verifica o bit x e simula a execução de um programa | `./arquivo` |
| `disk` | ocupação e mapa de blocos do disco | `df` |
| `blocks ARQ` | bloco de índices do FCB e o trecho gravado em cada bloco | `debugfs` / `filefrag` |
| `alloc [indexada\|contigua]` | mostra ou troca o método de alocação | – |
| `help`, `exit` | ajuda e saída | – |

Todos os comandos que recebem arquivos aceitam caminhos absolutos (`/docs/a.txt`) ou relativos (`../a.txt`). Em `cp`, `mv` e `ln`, se o destino for um diretório existente, o arquivo mantém o nome original dentro dele, como no Linux.

## Escolhas de design e estruturas de dados

```text
 MiniFileSystem
 |-- raiz ----------> unique_ptr<Diretorio> "/"
 |-- atual ---------> Diretorio* (diretório de trabalho)
 |-- disco ---------> DiscoSimulado { array<char, 4096> dados; array<bool, 256> ocupado; modo }
 |-- usuarioAtual / grupoAtual -> usados em toda checagem de permissão

 Diretorio "/"
   |-- primeiroFilho -> "docs" --proximoIrmao--> "tmp"      (cada filho aponta de volta para o pai)
   |-- arquivos:   Entrada "a.txt" <-> Entrada "b.txt"        (std::list)
                        |                  |
                        v                  v
                 shared_ptr<FCB> #1   shared_ptr<FCB> #2 { id, nome, tamanho, tipo, datas,
                                                           owner, group, permissoes,
                                                           blocos = [3, 4, 9], links }
                                                                     |
                                     disco.dados: [ bloco 3 ][ bloco 4 ] ... [ bloco 9 ]
```

### FCB (`struct FCB`)

```cpp
enum class TipoArquivo { Caractere, Numerico, Binario, Programa };

struct FCB {
    int id;                           //identificador unico (numero do inode simulado)
    std::string nome;
    std::size_t tamanho;
    TipoArquivo tipo;
    std::time_t criacao, modificacao, acesso;
    std::string owner, group;
    unsigned int permissoes;          //bitmask de 9 bits (ex.: 0644)
    std::vector<std::size_t> blocos;  //bloco de indices
    int links;                        //entradas de diretorio que apontam para o FCB
};
```

Os IDs vêm de um contador que só cresce, então um número de inode nunca é reutilizado.

### Diretórios: árvore binária "primeiro filho / próximo irmão"

Cada `Diretorio` tem três ponteiros:

- `pai`: ponteiro simples para o diretório de cima (`nullptr` na raiz), usado no `cd ..` e no `pwd`;
- `primeiroFilho` (esquerda): `unique_ptr` para o primeiro subdiretório;
- `proximoIrmao` (direita): `unique_ptr` para o próximo diretório do mesmo nível.

Como cada nó tem no máximo dois filhos na estrutura (esquerda e direita), a árvore é binária. Mesmo assim, uma pasta pode ter qualquer número de subpastas, encadeadas pelo `proximoIrmao`:

```text
Árvore de pastas             Árvore binária (esq = primeiro filho, dir = próximo irmão)

        /                     /
     /  |  \                  `-esq-> docs -dir-> tmp -dir-> home
  docs tmp home                        `-esq-> projetos
   |
 projetos
```

- `mkdir` chama `adicionarFilho`, que percorre os irmãos e encaixa o novo nó no fim, mantendo a ordem de criação.
- `rmdir` chama `removerFilho`, que liga o irmão anterior ao seguinte. O `unique_ptr` do nó removido libera a memória.
- `buscarFilho`, `ls` e `tree` percorrem a lista `primeiroFilho → proximoIrmao → ...`.

Cada nó é dono do primeiro filho e do próximo irmão, então destruir a raiz libera a árvore inteira automaticamente.

### Entradas de diretório separadas do FCB

Os arquivos de um diretório ficam em uma `std::list<Entrada>`, em que `Entrada { nome, shared_ptr<FCB> }` liga um nome a um FCB. Essa separação permite:

- `mv`: cria uma entrada nova e apaga a antiga, sem tocar no FCB nem nos blocos;
- `ln`: cria uma segunda entrada para o mesmo FCB e incrementa `links`;
- `rm`: apaga a entrada e decrementa `links`; os blocos só voltam para o disco quando `links` chega a zero.

O `shared_ptr` é usado porque, com links físicos, um mesmo FCB pode ter várias entradas, até em diretórios diferentes. O FCB é destruído quando a última entrada some. A `std::list` mantém as referências válidas quando outras entradas são incluídas ou removidas.

O campo `nome` do FCB guarda o nome com que o arquivo foi criado, como pede o enunciado. Ele acompanha o `mv` enquanto o arquivo tiver um único link. Com vários links, cada entrada tem o seu nome, e o `info` mostra os dois.

### Disco simulado

O disco é um `std::array<char, 4096>` dividido em 256 blocos de 16 bytes. Um `std::array<bool, 256>` funciona como mapa de blocos livres. A gravação acontece em quatro passos:

1. calcula quantos blocos são necessários (⌈tamanho / 16⌉);
2. escolhe os blocos novos **antes** de liberar os antigos, para que um erro de falta de espaço não destrua o conteúdo anterior;
3. libera os blocos antigos que não foram reaproveitados;
4. divide o conteúdo em pedaços de 16 bytes, copia cada pedaço para o seu bloco e grava a lista de índices no FCB.

### Usuário atual

O usuário e o grupo atuais são duas strings no `MiniFileSystem`, trocadas pelo comando `user`. O usuário inicial é `aluno`, do grupo `alunos`. O usuário `root` recebe o tratamento do Linux.

## Conceitos teóricos demonstrados

### Arquivo e seus atributos

Para o sistema, um arquivo é um tipo abstrato de dado: o usuário enxerga um nome e um conteúdo, e o sistema guarda os atributos no FCB. O comando `info` mostra todos eles: nome, identificador, tipo, tamanho, proteção, dono, grupo, links e as três datas.

Cada operação atualiza a data correspondente. A gravação (`echo`, `truncate`) muda a data de modificação. A leitura (`cat`, `cp`, `exec`) muda a data de último acesso. O `touch` em um arquivo existente atualiza as duas.

O tipo é um `enum class` e muda o comportamento do arquivo:

| Tipo | Gravação (`echo`) | Leitura (`cat`) | `exec` |
| --- | --- | --- | --- |
| `caractere` | texto + `\n` | texto | recusa: não é programa |
| `numerico` | só aceita um número por linha (`echo quarenta > idade.txt` é recusado) | texto | recusa |
| `binario` | bytes em hexadecimal (`echo 48 65 6c 6c 6f > logo.bin`) | dump hexadecimal | recusa |
| `programa` | texto + `\n` | texto | simula a execução se houver bit x |

### Operações com arquivos

As operações básicas são feitas sobre as estruturas simuladas:

- **criar**: `touch` e `echo >`;
- **escrever**: `echo >` substitui o conteúdo e `echo >>` acrescenta;
- **ler**: `cat`;
- **reposicionar/renomear**: `mv`;
- **copiar**: `cp`;
- **truncar**: `truncate` zera o arquivo e devolve todos os blocos;
- **excluir**: `rm`.

### File Control Block e inode simulado

O FCB separa a identidade do arquivo (o inode, `id`) do nome usado para encontrá-lo (a entrada de diretório). Isso aparece em três comandos:

- `mv`: o arquivo muda de nome e de diretório, mas continua com o mesmo inode;
- `cp`: cria um FCB novo, com outro inode e outros blocos;
- `ln`: duas entradas passam a mostrar o mesmo inode, e o `ls` exibe 2 na coluna de links. Depois de `rm` em uma delas, o conteúdo continua acessível pela outra.

### Estrutura de diretórios em árvore

A árvore traz as três vantagens vistas na disciplina:

- **Nomeação**: nomes só precisam ser únicos dentro do próprio diretório. Dois `x.txt` podem existir em `/` e em `/docs`, e o caminho completo identifica cada um.
- **Agrupamento**: arquivos relacionados ficam juntos (`documentos/projetos`), e `tree` mostra a hierarquia.
- **Eficiência**: a busca segue apenas o caminho indicado, um nível por vez, em vez de varrer todos os arquivos do sistema.

Caminhos absolutos partem da raiz e caminhos relativos partem do ponteiro `atual`. `.` e `..` funcionam em qualquer posição do caminho. O `rmdir` só remove diretórios vazios e recusa remover a raiz ou um diretório que contém o diretório atual.

### Proteção de acesso RWX e chmod com bitmask

As permissões ocupam 9 bits de um `unsigned int`, no mesmo formato do Linux:

```text
bit:    8 7 6   5 4 3   2 1 0
        r w x   r w x   r w x
        owner   group   others        0640 = 110 100 000 = rw-r-----
```

- **`chmod 640`**: cada dígito octal vira 3 bits (`modo = (modo << 3) | digito`).
- **Verificação de acesso**: o simulador descobre a classe do usuário (dono, grupo ou outros), desloca os bits dessa classe para o fim (`>> 6`, `>> 3` ou `>> 0`), aplica a máscara `& 7` e testa o bit da operação (`LEITURA = 4`, `ESCRITA = 2`, `EXECUCAO = 1`).
- **Exibição**: `permissoes::paraTexto` percorre os 9 bits e escreve `r`, `w`, `x` ou `-`.

A verificação acontece em toda operação, e uma operação negada não altera nada (bloqueio obrigatório):

| Operação | Exige |
| --- | --- |
| `cat`, `cp` (origem), `blocks` (conteúdo) | r |
| `echo >`, `truncate` | w |
| `echo >>` | r e w (o conteúdo atual precisa ser lido) |
| `exec` | x |
| `chmod`, `mv`, `rm` | ser o proprietário ou root |
| `ln` | ser o proprietário ou ter r e w (regra `protected_hardlinks` do Linux) |

Como no Linux, a classe é escolhida uma única vez: se o usuário é o dono, valem só os bits do dono, mesmo que o grupo tenha mais permissões. O `root` ignora r e w, e só executa se alguma classe tiver o bit x.

As mensagens de bloqueio explicam o motivo:

```text
visitante@minifs:/documentos$ cat notas.txt
cat: notas.txt: Permissao negada (usuario 'visitante' esta na classe 'outros' e nao tem leitura (r); permissoes rw-r-----)
```

### Simulação da alocação de blocos

Os três métodos clássicos são:

| Método | Como o arquivo é localizado | Vantagem | Desvantagem |
| --- | --- | --- | --- |
| Contígua | bloco inicial + tamanho | leitura sequencial e acesso direto rápidos | fragmentação externa; o arquivo não cresce se o vizinho estiver ocupado |
| Encadeada | cada bloco aponta para o próximo | sem fragmentação externa | acesso direto lento (precisa seguir a cadeia); um ponteiro corrompido perde o resto |
| Indexada | um bloco de índices lista todos os blocos | sem fragmentação externa e com acesso direto | espaço extra para o índice |

O simulador implementa **indexada** (padrão) e **contígua**, escolhidas com `alloc`. Nas duas, o FCB guarda o vetor de índices `blocos`. Na contígua, esse vetor sempre contém números consecutivos.

- **Indexada**: reaproveita os blocos que o arquivo já tinha e completa com os primeiros blocos livres.
- **Contígua**: procura a primeira sequência de blocos vizinhos livres que caiba o arquivo (first-fit).

O comando `blocks` mostra como o conteúdo foi dividido:

```text
aluno@minifs:/documentos$ blocks notas.txt
Arquivo 'notas.txt' (inode 1): 41 bytes em 3 bloco(s) de 16 bytes
Bloco de indices no FCB: [0, 1, 2]
  indice  0 -> bloco   0: |Este e um arquiv| 16 byte(s)
  indice  1 -> bloco   1: |o de teste.segun| 16 byte(s)
  indice  2 -> bloco   2: |da linha.       | 9 byte(s) (fragmentacao interna)
```

O roteiro `testes/fragmentacao.txt` compara os dois métodos:

1. enche o disco;
2. apaga dois arquivos de 1 bloco que não são vizinhos (sobram os blocos 252 e 254);
3. tenta gravar 21 bytes, que precisam de 2 blocos.

Na alocação contígua, a gravação falha por fragmentação externa. Na indexada, ela usa os blocos 252 e 254:

```text
echo: q.txt: Ha 2 blocos livres, mas nenhuma sequencia contigua de 2 blocos (fragmentacao externa da alocacao contigua)
...
Bloco de indices no FCB: [252, 254]
```

Também aparece a **fragmentação interna**: o último bloco de um arquivo pode ficar parcialmente vazio, desperdiçando até 15 bytes.

## Tratamento de erros

| Situação | Mensagem |
| --- | --- |
| arquivo ou diretório inexistente | `cat: fantasma.txt: Arquivo ou diretorio nao encontrado` |
| diretório pai inexistente | `mkdir: a/b: Diretorio pai nao encontrado` |
| nome inválido (vazio, `.`, `..`, com `/`, espaço ou mais de 63 caracteres) | `mkdir: ..: Nome invalido (...)` |
| nome já usado | `mkdir: nao foi possivel criar 'x.txt': Arquivo existe` |
| tipo errado de alvo | `cd: x.txt: Nao e um diretorio`, `cat: docs: E um diretorio` |
| permissão negada | `echo: notas.txt: Permissao negada (...)` |
| não é o dono | `chmod: notas.txt: Operacao nao permitida (somente o proprietario 'aluno' ou root)` |
| modo de chmod inválido | `chmod: modo invalido '999' (use 3 digitos octais de 0 a 7, ex.: 644, 750)` |
| conteúdo incompatível com o tipo | `echo: idade.txt: arquivo numerico; 'quarenta' nao e um numero` |
| disco cheio | `echo: g4.txt: Nao ha espaco no disco simulado (precisa de 63 blocos, ha 51 livres)` |
| fragmentação externa | `Ha 2 blocos livres, mas nenhuma sequencia contigua de 2 blocos (...)` |
| `rmdir` em diretório não vazio ou em uso | `Diretorio nao vazio`, `Dispositivo ou recurso esta ocupado` |
| `cp`/`mv` para o próprio arquivo | `cp: 'x.txt' e 'x.txt' sao o mesmo arquivo` |
| argumentos faltando ou sobrando | `Uso: cp ORIGEM DESTINO` |
| comando inexistente | `foo: comando nao encontrado (digite 'help')` |

## Gerenciamento de memória

| Estrutura | Dono | Liberada quando |
| --- | --- | --- |
| `Diretorio` | `unique_ptr` no pai (`primeiroFilho`) ou no irmão anterior (`proximoIrmao`); a raiz no `MiniFileSystem` | `rmdir` ou fim do programa |
| `Entrada` | `std::list` do diretório | `rm`, `mv` ou destruição do diretório |
| `FCB` | `shared_ptr` em cada entrada que aponta para ele | a última entrada é removida |
| vetor `blocos` do FCB | o próprio FCB | próxima gravação ou destruição do FCB |
| disco simulado | membro do `MiniFileSystem` | fim do programa |

Nenhum `new` ou `delete` aparece no código: toda a memória é liberada automaticamente (RAII). Os três roteiros da pasta `testes/` foram executados no Valgrind (`--leak-check=full`) e com `-fsanitize=address,undefined`. Resultado: `All heap blocks were freed -- no leaks are possible` e 0 erros.

## Exemplo de uso

```text
aluno@minifs:/$ mkdir documentos
Diretorio criado: documentos
aluno@minifs:/$ cd documentos
aluno@minifs:/documentos$ echo "Este e um arquivo de teste" > notas.txt
Arquivo criado: notas.txt (inode 1, permissoes 644 rw-r--r--)
27 bytes gravados em notas.txt (2 bloco(s), alocacao indexada)
aluno@minifs:/documentos$ chmod 640 notas.txt
Permissoes de notas.txt: 644 (rw-r--r--) -> 640 (rw-r-----)
aluno@minifs:/documentos$ user maria alunos
Usuario atual: maria | Grupo: alunos
maria@minifs:/documentos$ cat notas.txt
Este e um arquivo de teste
maria@minifs:/documentos$ echo tentativa > notas.txt
echo: notas.txt: Permissao negada (usuario 'maria' esta na classe 'grupo' e nao tem escrita (w); permissoes rw-r-----)
maria@minifs:/documentos$ user aluno alunos
Usuario atual: aluno | Grupo: alunos
aluno@minifs:/documentos$ ln notas.txt atalho.txt
Link fisico criado: atalho.txt -> inode 1 (agora com 2 links)
aluno@minifs:/documentos$ rm notas.txt
Removido: notas.txt (inode 1 continua existindo: 1 link(s) restante(s))
aluno@minifs:/documentos$ cat atalho.txt
Este e um arquivo de teste
```

O roteiro completo está em `testes/demo.txt`.

## Comparação com comandos Linux reais

Os mesmos passos foram executados em um terminal Linux (Ubuntu 24.04, ext4) e no simulador:

| Passo | Linux | Simulador |
| --- | --- | --- |
| `echo "Este e um arquivo de teste" > notas.txt` e `echo segunda linha >> notas.txt` | 41 bytes | 41 bytes (o `echo` também acrescenta `\n`) |
| `chmod 640 notas.txt` + `ls -li` | `1138789 -rw-r----- 1 root root 41 notas.txt` | `1  -rw-r-----  1  aluno  alunos  41  caractere  ...  notas.txt` |
| leitura por usuário de "outros" | `cat: notas.txt: Permission denied` | `cat: notas.txt: Permissao negada (...)` |
| `cp` | inode novo (1138790) | inode novo (2) |
| `mv copia.txt projetos/backup.txt` | mantém o inode | mantém o inode |
| `ln notas.txt atalho.txt` | mesmo inode, contador de links = 2 | mesmo inode, `LK` = 2 |
| `rm notas.txt` | `atalho.txt` continua com o conteúdo, links = 1 | igual |
| root lendo arquivo `000` | lê normalmente | lê normalmente |
| root executando arquivo sem bit x | `Permission denied` | `Permissao negada (nem o root executa...)` |
| `rmdir` de diretório com arquivos | `Directory not empty` | `Diretorio nao vazio` |
| `chmod 999` | `invalid mode: '999'` | `modo invalido '999'` |
| `cat fantasma` | `No such file or directory` | `Arquivo ou diretorio nao encontrado` |
| tamanho do bloco | 4096 bytes (`stat`: 8 setores de 512) | 16 bytes, para que a divisão em blocos fique visível |

## Limitações

- Os dados existem só durante a execução; não há persistência em arquivo.
- Diretórios não têm dono nem permissões próprias. Por isso, `rm` e `mv` exigem ser o dono do arquivo, regra que no Linux vale para diretórios com sticky bit, como `/tmp`.
- `mv` não sobrescreve um arquivo existente no destino; é preciso removê-lo antes.
- Não há links simbólicos, `mkdir -p` nem leitura/escrita em posições arbitrárias (`seek`).
- O `exec` não executa código; apenas verifica as permissões e o tipo.
- A troca de usuário não tem autenticação, e cada usuário pertence a um só grupo.
- A alocação encadeada é explicada, mas não implementada.
