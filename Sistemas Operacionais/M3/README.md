# Mini File System - Implementação e Análise de um Mini-Sistema de Arquivos em Memória em C

Simulador didatico de um sistema de arquivos em memoria. O projeto organiza diretorios em arvore, representa arquivos por FCBs, controla permissoes RWX e simula um disco dividido em blocos. Nenhum arquivo real do computador e alterado.

## Estrutura

```text
mini_file_system/
|-- include/
|   |-- FCB.h
|   |-- Diretorio.h
|   |-- DiscoSimulado.h
|   `-- MiniFileSystem.h
|-- src/
|   |-- DiscoSimulado.cpp
|   `-- MiniFileSystem.cpp
|-- main.cpp
|-- CMakeLists.txt
`-- README.md
```

- `FCB.h`: metadados do arquivo, identificador unico, proprietario, grupo, permissoes e lista de blocos.
- `Diretorio.h`: no da arvore de diretorios, com ponteiro para o pai e colecoes de arquivos/subdiretorios.
- `DiscoSimulado`: disco de 4096 bytes, dividido em 256 blocos de 16 bytes; armazena, recupera e libera conteudo.
- `MiniFileSystem`: comandos, navegacao, operacoes de arquivo e verificacao de permissoes.
- `main.cpp`: ponto de entrada do programa.

## Compilacao

### g++ (Linux, WSL ou MinGW)

Na pasta do projeto:

```bash
g++ -std=c++17 -Wall -Wextra -pedantic -Iinclude main.cpp src/MiniFileSystem.cpp src/DiscoSimulado.cpp -o mini_file_system
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
```

## Comandos

| Comando | Funcao |
|---|---|
| `mkdir NOME` | Cria diretorio |
| `cd CAMINHO` | Navega entre diretorios; aceita `/`, `..` e caminhos relativos/absolutos |
| `ls` | Lista diretorios e arquivos |
| `pwd` | Mostra o caminho atual |
| `touch ARQUIVO` | Cria arquivo vazio |
| `echo ARQUIVO TEXTO` | Grava ou substitui o conteudo; aceita espacos |
| `cat ARQUIVO` | Le o conteudo |
| `info ARQUIVO` | Exibe metadados e permissoes |
| `cp ORIGEM DESTINO` | Copia arquivo; destino pode incluir caminho |
| `mv ORIGEM DESTINO` | Move/renomeia arquivo; destino pode incluir caminho |
| `rm ARQUIVO` | Exclui arquivo e libera seus blocos |
| `chmod MODO ARQUIVO` | Altera permissoes octais, como `chmod 640 notas.txt` |
| `user NOME GRUPO` | Troca o usuario e grupo simulados |
| `exec ARQUIVO` | Verifica permissao de execucao (nao executa programa real) |
| `disk` | Exibe ocupacao do disco simulado |
| `blocks ARQUIVO` | Exibe os blocos associados ao arquivo |
| `help` | Exibe ajuda |
| `exit` | Encerra |

Os comandos que recebem arquivo procuram o nome no diretorio atual. `cp` e `mv` aceitam um caminho no destino. O texto de `echo` e tudo o que vier depois do nome do arquivo.

## Conceitos demonstrados

### Diretorios

Os diretorios formam uma arvore. Cada diretorio guarda um ponteiro para o pai e listas de subdiretorios e arquivos. O ponteiro `atual` indica o diretorio de trabalho. `cd` percorre a arvore e `pwd` monta o caminho a partir dos pais.

### FCB e metadados

Cada arquivo possui um FCB com nome, tamanho, tipo, ID unico, datas de criacao/modificacao/acesso, owner, group, permissoes e indices dos blocos utilizados. O ID e incrementado a cada novo arquivo criado/copiad​o.

### Permissoes RWX 

As permissoes sao armazenadas em tres grupos de tres bits: proprietario, grupo e outros. Os valores octais seguem o modelo usual: leitura = 4, escrita = 2 e execucao = 1. Exemplos: `644` (rw-r--r--) e `755` (rwxr-xr-x). `echo`, `cat` e `exec` verificam, respectivamente, escrita, leitura e execucao. Apenas o proprietario ou `root` pode usar `chmod`, mover ou excluir arquivos. A troca de usuario e uma simulacao didatica, nao uma autenticacao do sistema operacional.

### Alocacao de blocos

O disco possui 4096 bytes, organizados em 256 blocos de 16 bytes. O FCB guarda os indices dos blocos associados ao arquivo. O conteudo e dividido entre esses blocos; `cat` remonta o conteudo usando o tamanho registrado. A alocacao e indexada simplificada, pois o FCB mantem a lista de blocos. Ao excluir um arquivo, os blocos sao liberados para reutilizacao.

A gravacao verifica a disponibilidade de espaco antes de substituir os blocos antigos, preservando o conteudo anterior se nao houver espaco suficiente.

## Roteiro rapido de teste

```text
mkdir documentos
cd documentos
touch notas.txt
echo notas.txt Este e um arquivo de teste
cat notas.txt
info notas.txt
blocks notas.txt
disk
chmod 640 notas.txt
user visitante visitantes
cat notas.txt
echo notas.txt tentativa de escrita
user aluno alunos
cp notas.txt copia.txt
mv copia.txt backup.txt
ls
rm backup.txt
disk
```

## Observacoes e limites

- Os dados existem somente durante a execucao; ao encerrar, a simulacao e reiniciada.
- A permissao de diretorios nao e simulada; o controle RWX e aplicado aos arquivos.
- A operacao `exec` apenas verifica o bit de execucao e informa o resultado.
- O disco e propositalmente pequeno para facilitar a demonstracao de alocacao e falta de espaco.
- O codigo usa C++17 e `unique_ptr` para liberar automaticamente os nos da arvore e os FCBs.
