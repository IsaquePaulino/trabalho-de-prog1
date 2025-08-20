#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h> 

#define MAX_LINHAS 37
#define MAX_COLUNAS 102
#define MAX_PISTAS 12
#define MAX_CARROS_PISTA 10
#define MAX_CAMINHO 1000

// --- ESTRUTURAS DE DADOS ---

typedef struct {
    int x; 
} tCarro;

typedef struct {
    char direcao;
    int velocidade;
    int num_carros;
    tCarro carros[MAX_CARROS_PISTA];
} tPista;

typedef struct {
    int x_inicial;
    int y_inicial;
    int x_atual;
    int y_atual;
    int vidas;
    char skin[2][3];
} tGalinha;

typedef struct {
    int id_pista;
    int id_carro;
    int iteracao;
} tRanking;

typedef struct {
    int largura_mapa;
    int qtd_pistas;
    int altura_total;
    int largura_total;
    char mapa_matriz[MAX_LINHAS][MAX_COLUNAS];
    tPista pistas[MAX_PISTAS];
    tGalinha galinha;
    char skin_carro_padrao[2][3];

    int total_movimentos;
    int movimentos_tras;
    int altura_maxima_alcancada;
    int altura_max_atropelada;
    int altura_min_atropelada;
    int total_atropelamentos;
    int heatmap[MAX_LINHAS][MAX_COLUNAS];
} tJogo;


// --- FUNCOES DE DESENHO ---

tJogo DesenhaBaseMapa(tJogo jogo) {
    int i, j;
    for (i = 0; i < jogo.altura_total; i++) {
        for (j = 0; j < jogo.largura_total; j++) {
            jogo.mapa_matriz[i][j] = ' ';
        }
    }
    for (i = 0; i < jogo.altura_total; i++) {
        jogo.mapa_matriz[i][0] = '|';
        jogo.mapa_matriz[i][jogo.largura_total - 1] = '|';
    }
    for (j = 1; j < jogo.largura_total - 1; j++) {
        jogo.mapa_matriz[0][j] = '-';
        jogo.mapa_matriz[jogo.altura_total - 1][j] = '-';
    }
    for (i = 3; i < jogo.altura_total - 1; i += 3) {
        for (j = 1; j < jogo.largura_total - 1; j++) {
            jogo.mapa_matriz[i][j] = ((j-1) % 3 == 2) ? ' ' : '-';
        }
    }
    return jogo;
}

void DesenhaSprite(char matriz[MAX_LINHAS][MAX_COLUNAS], int x_centro, int y_linha_superior, char skin[2][3]) {
    int i, j;
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 3; j++) {
            int y = y_linha_superior + i;
            int x = x_centro - 1 + j;
            if (y >= 0 && y < MAX_LINHAS && x > 0 && x < MAX_COLUNAS - 1) {
                matriz[y][x] = skin[i][j];
            }
        }
    }
}

tJogo DesenhaCarros(tJogo jogo) {
    int p, i;
    for (p = 1; p < jogo.qtd_pistas - 1; p++) {
        for (i = 0; i < jogo.pistas[p].num_carros; i++) {
            int y_matriz = (p * 3) + 1;
            DesenhaSprite(jogo.mapa_matriz, jogo.pistas[p].carros[i].x, y_matriz, jogo.skin_carro_padrao);
        }
    }
    return jogo;
}

tJogo DesenhaGalinha(tJogo jogo) {
    DesenhaSprite(jogo.mapa_matriz, jogo.galinha.x_atual, jogo.galinha.y_atual, jogo.galinha.skin);
    return jogo;
}

tJogo AtualizaDesenhoCompleto(tJogo jogo) {
    jogo = DesenhaBaseMapa(jogo);
    jogo = DesenhaCarros(jogo);
    jogo = DesenhaGalinha(jogo);
    return jogo;
}


// --- FUNCOES DE INICIALIZACAO ---

tJogo LeArquivoConfig(char *diretorio) {
    tJogo jogo;
    char caminho_completo[MAX_CAMINHO];
    FILE *arquivo;

    sprintf(caminho_completo, "%s/config_inicial.txt", diretorio);
    arquivo = fopen(caminho_completo, "r");
    if (arquivo == NULL) { exit(1); }

    memset(&jogo, 0, sizeof(tJogo));
    jogo.altura_min_atropelada = 999999;
    jogo.altura_max_atropelada = -1;

    int i, j;
    for (i = 0; i < MAX_LINHAS; i++) {
        for (j = 0; j < MAX_COLUNAS; j++) {
            jogo.heatmap[i][j] = 0;
        }
    }

    int animacao;
    fscanf(arquivo, "%d", &animacao);
    fscanf(arquivo, "%d %d", &jogo.largura_mapa, &jogo.qtd_pistas);

    jogo.altura_total = (jogo.qtd_pistas * 3) + 1;
    jogo.largura_total = jogo.largura_mapa + 2;

    char buffer[1024];
    fgets(buffer, sizeof(buffer), arquivo); 

    int p, k;
    for (p = 0; p < jogo.qtd_pistas; p++) {
        if (!fgets(buffer, sizeof(buffer), arquivo)) break;
        
        char* pos = buffer;
        int chars_lidos = 0;
        
        if (sscanf(pos, " G %d %d", &jogo.galinha.x_inicial, &jogo.galinha.vidas) >= 2) {
            jogo.pistas[p].num_carros = 0;
        } else if (sscanf(pos, " %c %d %d %n", &jogo.pistas[p].direcao, &jogo.pistas[p].velocidade, &jogo.pistas[p].num_carros, &chars_lidos) >= 3) {
            pos += chars_lidos;
            for (k = 0; k < jogo.pistas[p].num_carros; k++) {
                if(sscanf(pos, "%d %n", &jogo.pistas[p].carros[k].x, &chars_lidos) >= 1) {
                    pos += chars_lidos;
                }
            }
        } else {
            jogo.pistas[p].num_carros = 0;
        }
    }
    
    jogo.galinha.x_atual = jogo.galinha.x_inicial;
    jogo.galinha.y_inicial = jogo.altura_total - 3;
    jogo.galinha.y_atual = jogo.galinha.y_inicial;

    fclose(arquivo);
    return jogo;
}

tJogo LeArquivoPersonagens(char *diretorio, tJogo jogo) {
    char caminho_completo[MAX_CAMINHO];
    FILE *arquivo;
    char buffer[16];

    sprintf(caminho_completo, "%s/personagens.txt", diretorio);
    arquivo = fopen(caminho_completo, "r");
    if (arquivo == NULL) { exit(1); }
    
    if (fgets(buffer, sizeof(buffer), arquivo)) {
        sscanf(buffer, "%c%c%c", &jogo.galinha.skin[0][0], &jogo.galinha.skin[0][1], &jogo.galinha.skin[0][2]);
    }
    if (fgets(buffer, sizeof(buffer), arquivo)) {
        sscanf(buffer, "%c%c%c", &jogo.galinha.skin[1][0], &jogo.galinha.skin[1][1], &jogo.galinha.skin[1][2]);
    }
    
    if (fgets(buffer, sizeof(buffer), arquivo)) {
        sscanf(buffer, "%c%c%c", &jogo.skin_carro_padrao[0][0], &jogo.skin_carro_padrao[0][1], &jogo.skin_carro_padrao[0][2]);
    }
    if (fgets(buffer, sizeof(buffer), arquivo)) {
        sscanf(buffer, "%c%c%c", &jogo.skin_carro_padrao[1][0], &jogo.skin_carro_padrao[1][1], &jogo.skin_carro_padrao[1][2]);
    }

    fclose(arquivo);
    return jogo;
}

void SalvaArquivoInicializacao(char *diretorio, tJogo jogo) {
    char caminho_completo[MAX_CAMINHO];
    FILE *arquivo;
    sprintf(caminho_completo, "%s/saida/inicializacao.txt", diretorio);
    arquivo = fopen(caminho_completo, "w");
    if (arquivo == NULL) return;
    int i, j;
    for (i = 0; i < jogo.altura_total; i++) {
        for (j = 0; j < jogo.largura_total; j++) {
            fprintf(arquivo, "%c", jogo.mapa_matriz[i][j]);
        }
        fprintf(arquivo, "\n");
    }
    fprintf(arquivo, "A posicao central da galinha iniciara em (%d %d).", jogo.galinha.x_inicial, jogo.galinha.y_inicial);
    fclose(arquivo);
}


// --- FUNCOES DO JOGO ---

tJogo MoveCarros(tJogo jogo) {
    int p, i;
    for (p = 1; p < jogo.qtd_pistas - 1; p++) {
        for (i = 0; i < jogo.pistas[p].num_carros; i++) {
            if (jogo.pistas[p].direcao == 'D') {
                jogo.pistas[p].carros[i].x += jogo.pistas[p].velocidade;
                if (jogo.pistas[p].carros[i].x > jogo.largura_mapa) {
                    jogo.pistas[p].carros[i].x = (jogo.pistas[p].carros[i].x - 1) % jogo.largura_mapa + 1;
                }
            } else if (jogo.pistas[p].direcao == 'E') {
                jogo.pistas[p].carros[i].x -= jogo.pistas[p].velocidade;
                while (jogo.pistas[p].carros[i].x < 1) {
                    jogo.pistas[p].carros[i].x += jogo.largura_mapa;
                }
            }
        }
    }
    return jogo;
}

int ConverteYparaAltura(tJogo jogo, int y_matriz) {
    return (jogo.altura_total - 1) - y_matriz;
}

int VerificaColisao(tJogo jogo, int *pista_colisao, int *carro_colisao) {
    int p, i;
    for (p = 1; p < jogo.qtd_pistas - 1; p++) {
        int y_pista_matriz = (p * 3) + 1;
        if (jogo.galinha.y_atual == y_pista_matriz) {
            for (i = 0; i < jogo.pistas[p].num_carros; i++) {
                if (abs(jogo.galinha.x_atual - jogo.pistas[p].carros[i].x) <= 2) {
                    *pista_colisao = p; 
                    *carro_colisao = i + 1;
                    return 1;
                }
            }
        }
    }
    return 0;
}


// --- UTILS ---

void MarcaHeatGalinha(tJogo *pjogo) {
    int i, j;
    for (i = 0; i < 2; i++) {
        for (j = -1; j <= 1; j++) {
            int hx = pjogo->galinha.x_atual + j;
            int hy = pjogo->galinha.y_atual + i;
            if (hy >= 0 && hy < MAX_LINHAS && hx >= 0 && hx < MAX_COLUNAS) {
                if (pjogo->heatmap[hy][hx] < 99) pjogo->heatmap[hy][hx]++;
            }
        }
    }
}

void OrdenaRanking(tRanking v[], int n) {
    int i, trocou = 1;
    while (trocou) {
        trocou = 0;
        for (i = 0; i < n - 1; i++) {
            int a1 = v[i].id_pista, b1 = v[i].id_carro, c1 = v[i].iteracao;
            int a2 = v[i+1].id_pista, b2 = v[i+1].id_carro, c2 = v[i+1].iteracao;
            int troca = 0;
            if (a1 > a2) troca = 1;
            else if (a1 == a2) {
                if (b1 > b2) troca = 1;
                else if (b1 == b2 && c1 < c2) troca = 1;
            }
            if (troca) {
                tRanking tmp = v[i];
                v[i] = v[i+1];
                v[i+1] = tmp;
                trocou = 1;
            }
        }
    }
}


// --- SALVA ARQUIVOS FINAIS ---

void SalvaArquivosFinais(char *diretorio, tJogo jogo, tRanking ranking[], int qtdRank, char resumo_log[]) {
    char caminho_completo[MAX_CAMINHO];
    FILE *arquivo;
    int i, j;

    // estatisticas.txt
    sprintf(caminho_completo, "%s/saida/estatisticas.txt", diretorio);
    arquivo = fopen(caminho_completo, "w");
    if (!arquivo) return;
    fprintf(arquivo, "Numero total de movimentos: %d\n", jogo.total_movimentos);
    fprintf(arquivo, "Altura maxima que a galinha chegou: %d\n", jogo.altura_maxima_alcancada);
    if (jogo.total_atropelamentos == 0) {
        fprintf(arquivo, "Altura maxima que a galinha foi atropelada: 0\n");
        fprintf(arquivo, "Altura minima que a galinha foi atropelada: 0\n");
    } else {
        fprintf(arquivo, "Altura maxima que a galinha foi atropelada: %d\n", jogo.altura_max_atropelada);
        fprintf(arquivo, "Altura minima que a galinha foi atropelada: %d\n", jogo.altura_min_atropelada);
    }
    fprintf(arquivo, "Numero de movimentos na direcao oposta: %d\n", jogo.movimentos_tras);
    fclose(arquivo);

    // heatmap.txt
    sprintf(caminho_completo, "%s/saida/heatmap.txt", diretorio);
    arquivo = fopen(caminho_completo, "w");
    if (!arquivo) return;
    for (i = 1; i < jogo.altura_total; i++) {
        if (i % 3 == 0) continue;
        for (j = 1; j < jogo.largura_total - 1; j++) {
            int valor = jogo.heatmap[i][j];
            if (valor > 99) valor = 99;
            fprintf(arquivo, "%3d", valor);
        }
        fprintf(arquivo, "\n");
    }
    fclose(arquivo);

    // ranking.txt
    sprintf(caminho_completo, "%s/saida/ranking.txt", diretorio);
    arquivo = fopen(caminho_completo, "w");
    if (!arquivo) return;
    OrdenaRanking(ranking, qtdRank);
    fprintf(arquivo, "id_pista,id_carro,iteracao\n");
    for (i = 0; i < qtdRank; i++) {
        fprintf(arquivo, "%d,%d,%d\n", ranking[i].id_pista + 1, ranking[i].id_carro, ranking[i].iteracao);
    }
    fclose(arquivo);

    // resumo.txt
    sprintf(caminho_completo, "%s/saida/resumo.txt", diretorio);
    arquivo = fopen(caminho_completo, "w");
    if (!arquivo) return;
    fprintf(arquivo, "%s", resumo_log);
    fclose(arquivo);
}


// --- MAIN ---

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("ERRO: Informe o diretorio com os arquivos de configuracao.\n");
        return 1;
    }
    char *diretorio = argv[1];

    tJogo jogo = LeArquivoConfig(diretorio);
    jogo = LeArquivoPersonagens(diretorio, jogo);

    jogo = AtualizaDesenhoCompleto(jogo);
    SalvaArquivoInicializacao(diretorio, jogo);

    char comando;
    int iteracao = 0;
    int pontos = 0;
    int jogo_acabou = 0;

    char resumo_log[20000] = "";
    tRanking ranking[500];
    int qtdRank = 0;

    char caminho_saida_txt[MAX_CAMINHO];
    sprintf(caminho_saida_txt, "%s/saida/saida.txt", diretorio);
    FILE *arquivo_saida = fopen(caminho_saida_txt, "w");
    if (!arquivo_saida) { return 1; }

    // heatmap inicial
    MarcaHeatGalinha(&jogo);

    while (1) {
        // Condicoes de vitoria/derrota
        if (jogo.galinha.y_atual <= 1) {
            pontos += 10;
            jogo = AtualizaDesenhoCompleto(jogo);
            printf("Pontos: %d\n", pontos);
            for (int i = 0; i < jogo.altura_total; i++) {
                for (int j = 0; j < jogo.largura_total; j++) {
                    printf("%c", jogo.mapa_matriz[i][j]);
                    fprintf(arquivo_saida, "%c", jogo.mapa_matriz[i][j]);
                }
                printf("\n");
                fprintf(arquivo_saida, "\n");
            }
            printf("A galinha venceu o jogo com %d pontos!\n", pontos);
            fprintf(arquivo_saida, "A galinha venceu o jogo com %d pontos!\n", pontos);
            sprintf(resumo_log + strlen(resumo_log), "[%d] Fim de jogo.\n", iteracao);
            break;
        }
        if (jogo.galinha.vidas <= 0) {
            jogo = AtualizaDesenhoCompleto(jogo);
            printf("Pontos: %d\n", pontos);
            for (int i = 0; i < jogo.altura_total; i++) {
                for (int j = 0; j < jogo.largura_total; j++) {
                    printf("%c", jogo.mapa_matriz[i][j]);
                    fprintf(arquivo_saida, "%c", jogo.mapa_matriz[i][j]);
                }
                printf("\n");
                fprintf(arquivo_saida, "\n");
            }
            printf("A galinha perdeu todas as suas vidas e terminou o jogo com %d pontos.\n", pontos);
            fprintf(arquivo_saida, "A galinha perdeu todas as suas vidas e terminou o jogo com %d pontos.\n", pontos);
            sprintf(resumo_log + strlen(resumo_log), "[%d] Fim de jogo.\n", iteracao);
            break;
        }

        jogo = AtualizaDesenhoCompleto(jogo);
        printf("Pontos: %d\n", pontos);
        for (int i = 0; i < jogo.altura_total; i++) {
            for (int j = 0; j < jogo.largura_total; j++) {
                printf("%c", jogo.mapa_matriz[i][j]);
                fprintf(arquivo_saida, "%c", jogo.mapa_matriz[i][j]);
            }
            printf("\n");
            fprintf(arquivo_saida, "\n");
        }

        // comando
        if (scanf(" %c", &comando) != 1) break;

        if (comando == 'w') {
            jogo.galinha.y_atual -= 3;
            pontos++;
            jogo.total_movimentos++;
        } else if (comando == 's') {
            if (jogo.galinha.y_atual < jogo.galinha.y_inicial) {
                jogo.galinha.y_atual += 3;
            }
            jogo.movimentos_tras++;
            jogo.total_movimentos++;
        }

        // atualizar altura maxima
        int alturaAtual = ConverteYparaAltura(jogo, jogo.galinha.y_atual);
        if (alturaAtual > jogo.altura_maxima_alcancada) {
            jogo.altura_maxima_alcancada = alturaAtual;
        }

        // carros
        jogo = MoveCarros(jogo);

        // colisao
        int pista_colidida = 0, carro_colidido = 0;
        if (VerificaColisao(jogo, &pista_colidida, &carro_colidido)) {
            int altura_atrop = ConverteYparaAltura(jogo, jogo.galinha.y_atual);
            if (altura_atrop > jogo.altura_max_atropelada) jogo.altura_max_atropelada = altura_atrop;
            if (altura_atrop < jogo.altura_min_atropelada) jogo.altura_min_atropelada = altura_atrop;
            jogo.total_atropelamentos++;

            sprintf(resumo_log + strlen(resumo_log),
                "[%d] Na pista %d o carro %d atropelou a galinha na posicao (%d,%d).\n",
                iteracao + 1, pista_colidida + 1, carro_colidido,
                jogo.galinha.x_atual, jogo.galinha.y_atual + 1);

            if (qtdRank < 500) {
                ranking[qtdRank].id_pista = pista_colidida;
                ranking[qtdRank].id_carro = carro_colidido;
                ranking[qtdRank].iteracao = iteracao + 1;
                qtdRank++;
            }

            jogo.galinha.vidas--;
            pontos = 0;
            jogo.galinha.x_atual = jogo.galinha.x_inicial;
            jogo.galinha.y_atual = jogo.galinha.y_inicial;
        }

        // marcar heatmap
        MarcaHeatGalinha(&jogo);

        iteracao++;
    }

    fclose(arquivo_saida);

    SalvaArquivosFinais(diretorio, jogo, ranking, qtdRank, resumo_log);
    return 0;
}
