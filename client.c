#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <winsock2.h>
#include <windows.h>
#include <conio.h>
#include <pthread.h>

#pragma comment(lib, "ws2_32.lib")

//===================== CONSTANTES=======================
#define LINHAS       13
#define COLUNAS      13
#define PORTA        51171
#define BOMB_TEMPO   3000
#define EXPL_TEMPO   500
#define EXPL_RAIO    2
#define MAX_BOMBAS   6
#define BOMBAS_PJ    3
#define TICK         50

#define COR_NORMAL   7
#define COR_P1       11
#define COR_P2       13
#define COR_BOMBA    12
#define COR_EXPLOSAO 14
#define COR_BLOCO    6
#define COR_MURO     8

/* ================PROTOCOLO DE MENSAGENS=======================

MOV:D ->  moveu na direcao D (W/A/S/D)
BOM:L:C ->  plantou bomba na linha L, coluna C
FIM:P ->  jogador P morreu  quee é igual a empate =0
SAI ->  jogador desconectou
RDY ->  jogador pronto pra comecar
REV->  pedido de revanche ouu jogar novamente
REV:S->  aceitou revanche
REV:N-> recusou revanche
FAS:N -> iniciar fase N

da mensagem eh terminada com '\n'.*/




//===================== ESTRUTURAS================================

typedef struct{
    int ativa;
    int dono;
    int lin, col;
    DWORD t_inicio;
    int explodindo;
    DWORD t_explosao;
    int exp_pos[17][2];
    int n_exp;
}Bomba;

//============= VARIAVEIS COMPARTILHADAs=============

char mapa[LINHAS][COLUNAS];
int p1_lin, p1_col;
int p2_lin, p2_col;
int meu_id = 2;
int rodando = 1;
int fim_jogo = 0;
int vencedor = 0;
Bomba bombas[MAX_BOMBAS];
int fase_atual = 1;

SOCKET clientSocket;
pthread_mutex_t mutex_mapa = PTHREAD_MUTEX_INITIALIZER;
int cor_atual = COR_NORMAL;

int pedido_revanche = 0; // 1=eu pedi e 2=é ele pediu
int resposta_revanche = 0;  //1=aceito/ 2=recusado
int nova_fase_recebida = 0;



// ==================== FUNCAO DE ERRO====================

void error_handling(char *message){
    fprintf(stderr, "%s: %d\n", message, WSAGetLastError());
    WSACleanup();
    exit(1);
}

//================== FUNCOES DE TELA =========

void gotoxy(int x, int y){
    COORD pos = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

void esconder_cursor() {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO ci;
    ci.dwSize = 25;
    ci.bVisible = FALSE;
    SetConsoleCursorInfo(h, &ci);
}

void mostrar_cursor(){
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO ci;
    ci.dwSize = 25;
    ci.bVisible = TRUE;
    SetConsoleCursorInfo(h, &ci);
}

void cor(int c){
    if (c != cor_atual) {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), c);
        cor_atual = c;
    }
}

//===================== FUNCOES DO MAPA =====================

void iniciar_mapa() {
    int i, j;
    memset(bombas, 0, sizeof(bombas));

    for (i = 0; i < LINHAS; i++){
        for (j = 0; j < COLUNAS; j++){
            if (i == 0 || i == LINHAS - 1 || j == 0 || j == COLUNAS - 1)
                mapa[i][j] = '#';
            else if (i % 2 == 0 && j % 2 == 0)
                mapa[i][j] = '#';
            else
                mapa[i][j] = ' ';
        }
    }

    if (fase_atual == 1) {
        //FASE 1 = FACIL: poucos blocos e bastante espacço
        int bl[][2] = {
            {1,5},{1,7},
            {3,3},{3,5},{3,9},
            {5,1},{5,5},{5,7},{5,11},
            {7,3},{7,7},{7,9},
            {9,3},{9,5},{9,9},
            {11,5},{11,7}
        };
        int n = sizeof(bl) / sizeof(bl[0]);
        for (i = 0; i < n; i++)
            mapa[bl[i][0]][bl[i][1]] = '*';
    }
    else if (fase_atual == 2) {
        //FASE 2 - MEDIO: mais bloco e caminhos mais estreitos
        int bl[][2] = {
            {1,3},{1,5},{1,7},{1,9},
            {3,1},{3,3},{3,5},{3,7},{3,9},{3,11},
            {5,1},{5,3},{5,5},{5,7},{5,9},{5,11},
            {7,1},{7,3},{7,5},{7,7},{7,9},{7,11},
            {9,1},{9,3},{9,5},{9,7},{9,9},{9,11},
            {11,3},{11,5},{11,7},{11,9}
        };
        int n = sizeof(bl) / sizeof(bl[0]);
        for (i = 0; i < n; i++)
            mapa[bl[i][0]][bl[i][1]] = '*';
    }
    else {
        //FASE 3 - DIFICIL: muitos blocos e um labirinto apertado
        int bl[][2] = {
            {1,3},{1,5},{1,7},{1,9},
            {2,3},{2,5},{2,9},
            {3,1},{3,3},{3,5},{3,7},{3,9},{3,11},
            {4,3},{4,7},{4,9},
            {5,1},{5,3},{5,5},{5,7},{5,9},{5,11},
            {6,3},{6,5},{6,7},{6,9},
            {7,1},{7,3},{7,5},{7,7},{7,9},{7,11},
            {8,3},{8,5},{8,9},
            {9,1},{9,3},{9,5},{9,7},{9,9},{9,11},
            {10,3},{10,7},{10,9},
            {11,3},{11,5},{11,7},{11,9}
        };
        int n = sizeof(bl) / sizeof(bl[0]);
        for (i = 0; i < n; i++)
            mapa[bl[i][0]][bl[i][1]] = '*';
    }

    //zona segura P1 canto superior-esquerdo
    mapa[1][1] = ' ';
	mapa[1][2] = ' ';
    mapa[2][1] = ' ';
	mapa[3][1] = ' ';
    //zona segura P2 canto inferior-direito
    mapa[11][11] = ' ';
	mapa[11][10] = ' ';
    mapa[10][11] = ' ';
	mapa[9][11] = ' ';

    p1_lin = 1;  p1_col = 1;
    p2_lin = 11; p2_col = 11;
}

int pode_mover(int l, int c){
    if (l <= 0 || l >= LINHAS - 1 || c <= 0 || c >= COLUNAS - 1)
        return 0;
    return (mapa[l][c] == ' ' || mapa[l][c] == '+');
}

/* ==================== DESENHAR TELA ==================== */

void desenhar(){
    int i, j;

    pthread_mutex_lock(&mutex_mapa);

    gotoxy(0, 0);
    cor(COR_NORMAL);
    printf("============= BOMBERMAN CMD ==============\n");
    printf(" Voce: Jogador %d    Fase %d/3   \n\n", meu_id, fase_atual);

    for (i = 0; i < LINHAS; i++){
        printf("  ");
        for (j = 0; j < COLUNAS; j++){
            if (i == p1_lin && j == p1_col && !fim_jogo) {
                cor(COR_P1); printf("1 ");
            }
            else if (i == p2_lin && j == p2_col && !fim_jogo){
                cor(COR_P2); printf("2 ");
            }
            else {
                switch (mapa[i][j]){
                    case '#':
						cor(COR_MURO);
						printf("# ");
					break;
                    case '*':
						cor(COR_BLOCO);
						printf("* ");
					break;
                    case 'B':
						cor(COR_BOMBA);
						printf("B ");
					break;
                    case '+':
						cor(COR_EXPLOSAO);
						printf("+ ");
					break;
                    default:
						cor(COR_NORMAL);
						printf("  ");
					break;
                }
            }
        }
        cor(COR_NORMAL);
        printf("\n");
    }

    printf("\n  WASD=mover  ESPACO=bomba  Q=sair    \n");
    printf("                                        \n");

    if (fim_jogo){
        printf("\n");
        if (vencedor == meu_id)
            printf("  >>> VOCE VENCEU! <<<\n");
        else if (vencedor == 3)
            printf("  >>> EMPATE! <<<\n");
        else
            printf("  >>> VOCE PERDEU! <<<\n");
        printf("                                          \n");
        if (vencedor == meu_id) {
            printf("  [1] Jogar novamente (proximo nivel)\n");
            printf("  [2] Voltar ao inicio \n");
        } else {
            printf("  [1] Pedir revanche \n");
            printf("  [2] Voltar ao inicio                  \n");
        }
        printf("                                          \n");
        if (pedido_revanche == 1)
            printf("  Aguardando resposta do oponente...\n");
        else if (pedido_revanche == 2)
            printf("  Oponente quer revanche! [S/N]\n");
        if (resposta_revanche == 2)
            printf("  Oponente recusou. Pressione Q.\n");
        printf("                                          \n");
    }

    pthread_mutex_unlock(&mutex_mapa);
}

//================== COMUNICACAO==================

void enviar_msg(const char* msg) {
    char buf[256];
    int bytesSent;
    sprintf(buf, "%s\n", msg);
    bytesSent = send(clientSocket, buf, (int)strlen(buf), 0);
    if (bytesSent == SOCKET_ERROR) {
        if (!fim_jogo) rodando = 0;
    }
}

//====================LOGICA DE MOVIMENTO E BOMBA ==========================

void mover_jogador(char dir){
    int *lin, *col;
    int nl, nc;
    int moveu = 0;
    char msg[32];

    if (meu_id == 1){
		lin = &p1_lin;
		col = &p1_col;
	}else{
		lin = &p2_lin;
		col = &p2_col;
	}

    nl = *lin;
    nc = *col;

    switch (dir){
        case 'w':
		case 'W':
			nl--;
	    break;
        case 's': case 'S': nl++; break;
        case 'a': case 'A': nc--; break;
        case 'd':
		case 'D':
			nc++;
		break;
        default: return;
    }

    pthread_mutex_lock(&mutex_mapa);

    if (pode_mover(nl, nc)){
        *lin = nl;
        *col = nc;
        moveu = 1;

        if (mapa[nl][nc] == '+'){
            fim_jogo = 1;
            vencedor = (meu_id == 1) ? 2 : 1;
            sprintf(msg, "FIM:%d", meu_id);
            pthread_mutex_unlock(&mutex_mapa);
            enviar_msg(msg);
            return;
        }
    }

    pthread_mutex_unlock(&mutex_mapa);

    if (moveu){
        sprintf(msg,"MOV:%c", dir);
        enviar_msg(msg);
    }
}

void plantar_bomba(){
    int base = (meu_id - 1) * BOMBAS_PJ;
    int i, idx = -1;
    int lin, col;
    char msg[32];

    //Aachar slot livre entre as 3 bombas do jogador
    for (i = 0; i < BOMBAS_PJ; i++){
        if (!bombas[base + i].ativa){
            idx = base + i;
            break;
        }
    }
    if (idx == -1) return;

    if (meu_id == 1){
		lin = p1_lin;
		col = p1_col;
	}else{
		lin = p2_lin;
		col = p2_col;
	}

    pthread_mutex_lock(&mutex_mapa);

    //nao plantar se jÁ tem bomba na celula
    if (mapa[lin][col] == 'B'){
        pthread_mutex_unlock(&mutex_mapa);
        return;
    }

    bombas[idx].ativa = 1;
    bombas[idx].dono = meu_id;
    bombas[idx].lin = lin;
    bombas[idx].col = col;
    bombas[idx].t_inicio = GetTickCount();
    bombas[idx].explodindo = 0;
    bombas[idx].n_exp = 0;
    mapa[lin][col] = 'B';

    pthread_mutex_unlock(&mutex_mapa);

    sprintf(msg, "BOM:%d:%d", lin, col);
    enviar_msg(msg);
}

void explodir_bomba(int idx){
    int i, d, r;
    int lin =bombas[idx].lin;
    int col = bombas[idx].col;
    int p1_morto = 0, p2_morto = 0;
    int dl[] ={-1, 1, 0, 0};
    int dc[] = {0, 0, -1, 1};

    bombas[idx].explodindo = 1;
    bombas[idx].t_explosao = GetTickCount();
    bombas[idx].n_exp = 0;

    mapa[lin][col] ='+';
    bombas[idx].exp_pos[bombas[idx].n_exp][0] = lin;
    bombas[idx].exp_pos[bombas[idx].n_exp][1] = col;
    bombas[idx].n_exp++;

    for (d = 0; d < 4; d++){
        for (r = 1; r <= EXPL_RAIO; r++) {
            int nl = lin + dl[d] * r;
            int nc = col + dc[d] * r;

            if (nl <= 0 || nl >= LINHAS - 1 || nc <= 0 || nc >= COLUNAS - 1)
                break;
            if (mapa[nl][nc] == '#')
                break;

            if (mapa[nl][nc] == '*'){
                mapa[nl][nc] = '+';
                bombas[idx].exp_pos[bombas[idx].n_exp][0] = nl;
                bombas[idx].exp_pos[bombas[idx].n_exp][1] = nc;
                bombas[idx].n_exp++;
                break;
            }

            mapa[nl][nc] = '+';
            bombas[idx].exp_pos[bombas[idx].n_exp][0] = nl;
            bombas[idx].exp_pos[bombas[idx].n_exp][1] = nc;
            bombas[idx].n_exp++;
        }
    }

    for (i = 0; i < bombas[idx].n_exp; i++){
        int el = bombas[idx].exp_pos[i][0];
        int ec = bombas[idx].exp_pos[i][1];
        if (el == p1_lin && ec == p1_col)
			p1_morto = 1;
        if (el == p2_lin && ec == p2_col)
			p2_morto = 1;
    }

    if (p1_morto && p2_morto){
        fim_jogo = 1;
        vencedor = 3;
    } else if (p1_morto){
        fim_jogo = 1;
        vencedor = 2;
    } else if (p2_morto){
        fim_jogo = 1;
        vencedor = 1;
    }
}

void limpar_explosao(int idx){
    int i;
    for (i = 0; i < bombas[idx].n_exp; i++){
        int l = bombas[idx].exp_pos[i][0];
        int c = bombas[idx].exp_pos[i][1];
        if (mapa[l][c] == '+')
            mapa[l][c] = ' ';
    }
    bombas[idx].ativa = 0;
    bombas[idx].n_exp = 0;
}

void atualizar_bombas(){
    int i;
    DWORD agora = GetTickCount();

    pthread_mutex_lock(&mutex_mapa);

    for (i = 0; i < MAX_BOMBAS; i++){
        if (!bombas[i].ativa)
			continue;

        if (!bombas[i].explodindo){
            if (agora - bombas[i].t_inicio >= BOMB_TEMPO)
                explodir_bomba(i);
        } else {
            if (agora - bombas[i].t_explosao >= EXPL_TEMPO)
                limpar_explosao(i);
        }
    }

    pthread_mutex_unlock(&mutex_mapa);
}

//============ PROCESSAR MENSAGEM DO OPONENTE ===============

void processar_msg(char* msg){
    pthread_mutex_lock(&mutex_mapa);

    if (strncmp(msg, "MOV:", 4) == 0){
        char dir = msg[4];
        int *lin, *col;

        if (meu_id == 1){
			lin = &p2_lin;
			col = &p2_col;
		}
    	else{
			lin = &p1_lin;
			col = &p1_col;
		}

        switch (dir){
            case 'w': case 'W': (*lin)--; break;
            case 's': case 'S': (*lin)++; break;
            case 'a': case 'A': (*col)--; break;
            case 'd': case 'D': (*col)++; break;
        }

        if (mapa[*lin][*col] == '+'){
            fim_jogo = 1;
            vencedor = meu_id;
        }
    }
    else if (strncmp(msg, "BOM:", 4) == 0){
        int lin, col;
        int opp_base = (meu_id == 1) ? BOMBAS_PJ : 0;
        int i, idx = -1;

        sscanf(msg + 4, "%d:%d", &lin, &col);

        for (i = 0; i < BOMBAS_PJ; i++){
            if (!bombas[opp_base + i].ativa) {
                idx = opp_base + i;
                break;
            }
        }

        if (idx != -1){
            bombas[idx].ativa = 1;
            bombas[idx].dono = (meu_id == 1) ? 2 : 1;
            bombas[idx].lin = lin;
            bombas[idx].col = col;
            bombas[idx].t_inicio = GetTickCount();
            bombas[idx].explodindo = 0;
            bombas[idx].n_exp = 0;
            mapa[lin][col] = 'B';
        }
    }
    else if (strncmp(msg, "FIM:", 4) == 0){
        int morto = atoi(msg + 4);
        fim_jogo = 1;
        if (morto == 0)
            vencedor = 3;
        else
            vencedor = (morto == meu_id) ? (3 - meu_id) : meu_id;
    }
    else if (strcmp(msg, "SAI") == 0){
        fim_jogo = 1;
        vencedor = meu_id;
    }
    else if (strcmp(msg, "REV") == 0){
        pedido_revanche = 2;
    }
    else if (strcmp(msg, "REV:S") == 0){
        resposta_revanche = 1;
    }
    else if (strcmp(msg, "REV:N") == 0){
        resposta_revanche = 2;
    }
    else if (strncmp(msg, "FAS:", 4) == 0){
        fase_atual = atoi(msg + 4);
        nova_fase_recebida = 1;
    }

    pthread_mutex_unlock(&mutex_mapa);
}

//================THREAD DE RECEBIMENTO ============

void* thread_receber(void* arg){
    char recvBuffer[512];
    char parcial[512];
    int pos = 0;
    int bytesReceived;
    int i;

    (void)arg;

    while (rodando){
        bytesReceived = recv(clientSocket, recvBuffer, sizeof(recvBuffer) - 1, 0);
        if(bytesReceived == SOCKET_ERROR){
            int err = WSAGetLastError();
            if(err == WSAETIMEDOUT) continue;
            if (!fim_jogo) rodando = 0;
            break;
        }else if (bytesReceived == 0){
            if (!fim_jogo) rodando = 0;
            break;
        }
        recvBuffer[bytesReceived] = '\0';

        for (i = 0; i < bytesReceived; i++) {
            if (recvBuffer[i] == '\n') {
                parcial[pos] = '\0';
                if (pos > 0)
                    processar_msg(parcial);
                pos = 0;
            } else {
                if (pos < (int)sizeof(parcial) - 1)
                    parcial[pos++] = recvBuffer[i];
            }
        }
    }

    return NULL;
}

//========================================================================== MAIN ========================================================================

int main(){
    WSADATA winsocketsDados;
    struct sockaddr_in serverAddr;
    pthread_t hThread;
    char recvBuffer[64];
    int bytesReceived;
    int jogar_de_novo;
    char ip_servidor[64];

    //INICIAR WINSOCK
    if (WSAStartup(MAKEWORD(2, 2), &winsocketsDados) != 0){
        error_handling("Falha ao inicializar o Winsock");
    } else{
        printf("WSAStartup carregado com sucesso\n");
    }

    //CRIAR SOCKET DO CLIENTE
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET){
        error_handling("Erro ao criar o socket");
    } else{
        printf("Socket criado com sucesso\n");
    }

    printf("\n==========================================\n");
    printf("       BOMBERMAN CMD - CLIENTE\n");
    printf("==========================================\n\n");

    printf("  Digite o IP do servidor (ou ENTER para 127.0.0.1): ");
    fgets(ip_servidor, sizeof(ip_servidor), stdin);
    ip_servidor[strcspn(ip_servidor, "\n")] = '\0';
    if (strlen(ip_servidor) == 0)
        strcpy(ip_servidor, "127.0.0.1");

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(ip_servidor);
    serverAddr.sin_port = htons(PORTA);

    printf("  Conectando em %s:%d...\n", ip_servidor, PORTA);

    //CONECTAR NO SERVIDOR
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR){
        error_handling("Erro ao conectar ao servidor");
    }/*else{
        printf("Conectado ao servidor com sucesso\n");
    }*/

    //timeout pra conseguir parar a thread de recv
    {
        DWORD tmout = 500;
        setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&tmout, sizeof(tmout));
    }

    printf("  Aguardando inicio...\n");

    //sincronizacao inicial
    bytesReceived = recv(clientSocket, recvBuffer, sizeof(recvBuffer) - 1, 0);
    if (bytesReceived > 0) recvBuffer[bytesReceived] = '\0';
    enviar_msg("RDY");

    Sleep(500);

inicio_jogo:
    jogar_de_novo = 0;
    fim_jogo = 0;
    vencedor = 0;
    rodando = 1;
    pedido_revanche = 0;
    resposta_revanche = 0;
    nova_fase_recebida = 0;

    iniciar_mapa();
    system("mode con cols=50 lines=28");
    system("cls");
    esconder_cursor();

    //thread de recebimento
    pthread_create(&hThread, NULL, thread_receber, NULL);

    while (rodando && !fim_jogo) {
        if (kbhit()) {
            char tecla = getch();

            if (tecla == 'q' || tecla == 'Q') {
                enviar_msg("SAI");
                rodando = 0;
                break;
            }
            if (tecla == 'w' || tecla == 'W' ||
                tecla == 'a' || tecla == 'A' ||
                tecla == 's' || tecla == 'S' ||
                tecla == 'd' || tecla == 'D') {
                mover_jogador(tecla);
            }
            if (tecla == ' ') {
                plantar_bomba();
            }
        }

        atualizar_bombas();
        desenhar();
        Sleep(TICK);
    }

    desenhar();

    //menu pos jogop
    mostrar_cursor();
    {
    int ult_pedido = 0;
    int ult_resp = 0;
    int ult_fase = 0;
    while (fim_jogo && rodando) {
        if (kbhit()) {
            char tecla = getch();

            if(tecla == '1'){
                if(pedido_revanche == 2){
                    //ele ja pediu, entao aceita direto
                    enviar_msg("REV:S");
                    /* cliente espera o servidor enviar FAS:N */
                }
                else{
                    enviar_msg("REV");
                    pedido_revanche = 1;
                }
                desenhar();
            }
            else if (tecla == '2' || tecla == 'q' || tecla == 'Q'){
                enviar_msg("SAI");
                rodando = 0;
                break;
            }
            else if(pedido_revanche == 2 && (tecla == 's' || tecla == 'S')){
                enviar_msg("REV:S");
                /* cliente espera o servidor enviar FAS:N */
            }
            else if (pedido_revanche == 2 && (tecla == 'n' || tecla == 'N')){
                enviar_msg("REV:N");
                rodando = 0;
                break;
            }
        }

        if (resposta_revanche == 1 || nova_fase_recebida) {
            jogar_de_novo = 1;
            break;
        }
        //oponente recusou
        if (resposta_revanche == 2) {
            desenhar();
            Sleep(2000);
            rodando = 0;
            break;
        }

        //so redesenha quando muda algo (senao cursor fica louco)
        if(pedido_revanche != ult_pedido || resposta_revanche != ult_resp || nova_fase_recebida != ult_fase){
            ult_pedido = pedido_revanche;
            ult_resp = resposta_revanche;
            ult_fase = nova_fase_recebida;
            desenhar();
        }
        Sleep(TICK);
    }
    }

    //parar thread
    rodando = 0;

    if (jogar_de_novo) {
        Sleep(300);
        pthread_join(hThread, NULL);
        goto inicio_jogo;
    }

    shutdown(clientSocket, SD_BOTH);
    pthread_join(hThread, NULL);

    //CLEAr
    closesocket(clientSocket);
    WSACleanup();

    cor(COR_NORMAL);
    printf("\n Obrigado por jogar!\n");
    getch();
    return 0;
}
