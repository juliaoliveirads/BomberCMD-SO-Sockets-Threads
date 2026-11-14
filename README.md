<p align="center">
  <img src="assets/Logo%20Bomber%20Friends%20com%20Personagem%20Explosivo.png" alt="Bomber Friends Logo" width="500"/>
</p>

<h1 align="center">💣 Bomber Friends no CMD 💣</h1>

<p align="center">
  <b>Reprodução do clássico Bomber Friends direto no terminal do Windows!</b><br>
  Jogo multiplayer via rede (TCP/IP) feito em C puro, usando Sockets e Threads.
</p>

<p align="center">
  <img src="assets/Bomba%20Cartunesca%20com%20Fa%C3%ADsca%20Explosiva.png" alt="bomba" width="40"/>
  <img src="assets/Bomba%20Cartunesca%20com%20Fa%C3%ADsca%20Explosiva.png" alt="bomba" width="40"/>
  <img src="assets/Bomba%20Cartunesca%20com%20Fa%C3%ADsca%20Explosiva.png" alt="bomba" width="40"/>
</p>

---

## 📚 Bibliotecas Utilizadas

| Biblioteca | Header | Função no projeto |
|-----------|--------|-------------------|
| **Winsock2** | `<winsock2.h>` | Comunicação via sockets TCP/IP entre servidor e cliente |
| **Windows API** | `<windows.h>` | Funções do sistema: `Sleep()`, `GetTickCount()`, manipulação do console (cores, cursor, tamanho da janela) |
| **Pthreads** | `<pthread.h>` | Criação de threads para recebimento assíncrono de mensagens e mutex para exclusão mútua |
| **Conio** | `<conio.h>` | Captura de teclas sem bloquear o jogo (`kbhit()` e `getch()`) |
| **Stdio** | `<stdio.h>` | Entrada e saída padrão (`printf`, `sprintf`, `fgets`) |
| **Stdlib** | `<stdlib.h>` | Funções utilitárias (`atoi`, `system`) |
| **String** | `<string.h>` | Manipulação de strings (`strcmp`, `strncmp`, `strcpy`, `strlen`) |

> **Todas essas bibliotecas já vêm inclusas no TDM-GCC (MinGW)** que é instalado junto com o Dev-C++. Não é necessário baixar ou instalar nada extra.

**Flags de compilação necessárias:**
- `-lws2_32` → linka a biblioteca do Winsock2
- `-lpthread` → linka a biblioteca de threads POSIX

<p align="center">
  <img src="assets/Bomba%20Cartunesca%20com%20Fa%C3%ADsca%20Explosiva.png" alt="bomba" width="30"/>
</p>

---

## 🎮 Sobre o Jogo

Um Bomberman completo rodando no Prompt de Comando do Windows! Dois jogadores se conectam pela rede local (ou na mesma máquina) e disputam quem sobrevive plantando bombas e destruindo blocos.

- **Mapa 13x13** com blocos destrutíveis e indestrutíveis
- **3 bombas simultâneas** por jogador
- **3 fases** com dificuldade progressiva (Fácil → Médio → Difícil)
- **Sistema de revanche** — ao final de cada partida dá pra pedir revanche e ir pro próximo nível
- **Conexão TCP/IP** — jogue em LAN ou na mesma máquina

<p align="center">
  <img src="assets/Bomba%20Cartunesca%20com%20Fa%C3%ADsca%20Explosiva.png" alt="bomba" width="30"/>
</p>

---

## 🛠️ Compilação

### Pré-requisitos

- **Dev-C++** com **TDM-GCC 4.9.2** (32-bit) — já vem instalado junto com o Dev-C++
- Não precisa instalar nenhuma biblioteca externa! O `winsock2` e `pthread` já vêm com o MinGW/TDM-GCC

### Como compilar no Dev-C++

1. Abra o Dev-C++
2. Vá em **Arquivo → Novo → Projeto → Console Application → C**
3. Cole o código do arquivo `.c` desejado
4. Vá em **Ferramentas → Opções do Compilador**
5. Marque **"Adicionar os seguintes comandos ao linker"** e cole:

```
-static-libgcc -lws2_32 -lpthread
```

6. Compile com **F11** (Compilar e Executar) ou **F9** (Compilar)

### Compilando pelo terminal (GCC direto)

```bash
gcc server.c -o server.exe -static-libgcc -lws2_32 -lpthread
gcc client.c -o client.exe -static-libgcc -lws2_32 -lpthread
```

<p align="center">
  <img src="assets/Bomba%20Cartunesca%20com%20Fa%C3%ADsca%20Explosiva.png" alt="bomba" width="30"/>
</p>

---

## 🚀 Como Jogar

### Arquivos

| Arquivo | Descrição |
|---------|-----------|
| `server.c` | **Servidor** (Jogador 1) — cria a sala e aguarda conexão |
| `client.c` | **Cliente** (Jogador 2) — conecta pelo IP digitado (pra jogar em LAN) |
### Jogando sozinho (mesma máquina)

Se quiser testar sozinho ou jogar contra você mesmo, use o `client.c com ip local`:

1. Compile `server.c` e `client_local.c`
2. Abra o `server.exe` — ele vai mostrar seu IP e a porta **51171**
3. Abra o `client.exe` em outra janela — ele já conecta automaticamente em `127.0.0.1 apertando enter`
4. Pronto! As duas janelas ficam lado a lado e você controla os dois

### Jogando em LAN (dois computadores)

Pra jogar com um amigo na mesma rede:

1. Compile `server.c` no computador do **Jogador 1** e `client.c` no do **Jogador 2**
2. O Jogador 1 abre o `server.exe` — ele mostra o IP local (ex: `192.168.1.100`)
3. O Jogador 2 abre o `client.exe` e digita o IP que apareceu no servidor
4. Pronto! Partida começa automaticamente

> **Dica:** Os dois computadores precisam estar na mesma rede Wi-Fi ou conectados no mesmo roteador. Se não conectar, verifique se o firewall do Windows está permitindo a conexão na porta 51171.

### Controles

| Tecla | Ação |
|-------|------|
| `W` `A` `S` `D` | Mover (cima, esquerda, baixo, direita) |
| `ESPAÇO` | Plantar bomba |
| `Q` | Sair da partida |

<p align="center">
  <img src="assets/Bomba%20Cartunesca%20com%20Fa%C3%ADsca%20Explosiva.png" alt="bomba" width="30"/>
</p>

---

## 📡 Como funciona a conexão

O jogo usa **Sockets TCP** (via `winsock2.h`) pra comunicação entre os dois jogadores. O servidor escuta na porta **51171** e aceita uma conexão do cliente.

Toda a troca de mensagens segue um protocolo simples com delimitador `\n`:

| Mensagem | Significado |
|----------|-------------|
| `MOV:D` | Jogador moveu na direção D (W/A/S/D) |
| `BOM:L:C` | Bomba plantada na linha L, coluna C |
| `FIM:P` | Jogador P morreu (0 = empate) |
| `SAI` | Jogador desconectou |
| `REV` | Pedido de revanche |
| `REV:S` / `REV:N` | Aceitou / Recusou revanche |
| `FAS:N` | Iniciar fase N |

As **threads** (`pthread.h`) são usadas pra receber mensagens de forma assíncrona — uma thread fica escutando o socket enquanto o jogo roda no loop principal. Um **mutex** protege o acesso ao mapa compartilhado.

<p align="center">
  <img src="assets/Bomba%20Cartunesca%20com%20Fa%C3%ADsca%20Explosiva.png" alt="bomba" width="30"/>
</p>

---

## 🗺️ As Fases

O jogo tem **3 fases** com mapas diferentes:

- **Fase 1 (Fácil)** — Poucos blocos destrutíveis, bastante espaço pra se mover
- **Fase 2 (Médio)** — Mais blocos, caminhos mais apertados
- **Fase 3 (Difícil)** — Mapa lotado, precisa de estratégia pra não se prender

Ao vencer uma partida e pedir/aceitar revanche, o jogo avança pra próxima fase automaticamente. Depois da fase 3, volta pra fase 1.

---

## 🏗️ Estrutura do Projeto

```
bomberman-cmd/
├── server.c            # Código do servidor (Jogador 1)
├── client.c            # Código do cliente com IP manual (LAN)
├── Executaveis/
│   ├── server.exe      # Servidor compilado
│   └── client.exe      # Cliente compilado
├── assets/
│   ├── bomber_friends_logo.png
│   └── bomba.png
└── README.md
```

---

<p align="center">
  <img src="assets/Bomba%20Cartunesca%20com%20Fa%C3%ADsca%20Explosiva.png" alt="bomba" width="40"/>
  <img src="assets/Bomba%20Cartunesca%20com%20Fa%C3%ADsca%20Explosiva.png" alt="bomba" width="40"/>
  <img src="assets/Bomba%20Cartunesca%20com%20Fa%C3%ADsca%20Explosiva.png" alt="bomba" width="40"/>
</p>

<p align="center">
  Feito com 💥 pra disciplina de <b>Sistemas Operacionais</b><br>
  Unicesumar — ESOFT-4S
</p>
