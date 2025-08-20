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


// --- FUNCOES DO JOGO ---

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

tJogo LeArquivoConfig(char *diretorio) {
    tJogo jogo;
    char caminho_completo[MAX_CAMINHO];
    FILE *arquivo;

    sprintf(caminho_completo, "%s/config_inicial.txt", diretorio);
    arquivo = fopen(caminho_completo, "r");
    if (arquivo == NULL) { exit(1); }

    memset(&jogo, 0, sizeof(tJogo));
    jogo.altura_min_atropelada = 999;

    int animacao;
    fscanf(arquivo, "%d", &animacao);
    fscanf(arquivo, "%d %d", &jogo.largura_mapa, &jogo.qtd_pistas);

    jogo.altura_total = (jogo.qtd_pistas * 3) + 1;
    jogo.largura_total = jogo.largura_mapa + 2;

    char buffer[1024];
    fgets(buffer, sizeof(buffer), arquivo); 

    int p, i;
    for (p = 0; p < jogo.qtd_pistas; p++) {
        if (!fgets(buffer, sizeof(buffer), arquivo)) break;
        
        char* pos = buffer;
        int chars_lidos = 0;
        
        if (sscanf(pos, " G %d %d", &jogo.galinha.x_inicial, &jogo.galinha.vidas) >= 2) {
            jogo.pistas[p].num_carros = 0;
        } else if (sscanf(pos, " %c %d %d %n", &jogo.pistas[p].direcao, &jogo.pistas[p].velocidade, &jogo.pistas[p].num_carros, &chars_lidos) >= 3) {
            pos += chars_lidos;
            for (i = 0; i < jogo.pistas[p].num_carros; i++) {
                if(sscanf(pos, "%d %n", &jogo.pistas[p].carros[i].x, &chars_lidos) >= 1) {
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

void SalvaArquivosFinais(char *diretorio, tJogo jogo, tRanking ranking[], char resumo_log[]) {
    char caminho_completo[MAX_CAMINHO];
    FILE *arquivo;

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

    sprintf(caminho_completo, "%s/saida/heatmap.txt", diretorio);
    arquivo = fopen(caminho_completo, "w");
    if (!arquivo) return;
    int i, j;
    for (i = 1; i < jogo.altura_total; i++) {
        if(i % 3 == 0) continue;
        for (j = 1; j < jogo.largura_total - 1; j++) {
            if (jogo.heatmap[i][j] == -1) {
                fprintf(arquivo, "  *");
            } else {
                int valor = jogo.heatmap[i][j] > 99 ? 99 : jogo.heatmap[i][j];
                fprintf(arquivo, "%3d", valor);
            }
        }
        fprintf(arquivo, "\n");
    }
    fclose(arquivo);

    sprintf(caminho_completo, "%s/saida/ranking.txt", diretorio);
    arquivo = fopen(caminho_completo, "w");
    if (!arquivo) return;
    int trocou;
    do {
        trocou = 0;
        for (i = 0; i < jogo.total_atropelamentos - 1; i++) {
            int trocar = 0;
            if (ranking[i].id_pista > ranking[i + 1].id_pista) trocar = 1;
            else if (ranking[i].id_pista == ranking[i + 1].id_pista) {
                if (ranking[i].id_carro > ranking[i + 1].id_carro) trocar = 1;
                else if (ranking[i].id_carro == ranking[i + 1].id_carro) {
                    if (ranking[i].iteracao < ranking[i + 1].iteracao) trocar = 1;
                }
            }
            if (trocar) {
                tRanking temp = ranking[i];
                ranking[i] = ranking[i + 1];
                ranking[i + 1] = temp;
                trocou = 1;
            }
        }
    } while (trocou);
    fprintf(arquivo, "id_pista,id_carro,iteracao\n");
    for (i = 0; i < jogo.total_atropelamentos; i++) {
        // CORRIGIDO: Indice da pista agora eh +1 para comecar em 1.
        fprintf(arquivo, "%d,%d,%d\n", ranking[i].id_pista + 1, ranking[i].id_carro, ranking[i].iteracao);
    }
    fclose(arquivo);

    sprintf(caminho_completo, "%s/saida/resumo.txt", diretorio);
    arquivo = fopen(caminho_completo, "w");
    if (!arquivo) return;
    fprintf(arquivo, "%s", resumo_log);
    fclose(arquivo);
}

// --- FUNCAO MAIN ---
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("ERRO: Informe o diretorio com os arquivos de configuracao.\n");
        return 1;
    }
    char *diretorio = argv[1];

    tJogo jogo = LeArquivoConfig(diretorio);
    jogo = LeArquivoPersonagens(diretorio, jogo);
    
    char caminho_saida_dir[MAX_CAMINHO];
    sprintf(caminho_saida_dir, "%s/saida", diretorio);

    jogo = AtualizaDesenhoCompleto(jogo);
    SalvaArquivoInicializacao(diretorio, jogo);
    
    char comando;
    int iteracao = 0;
    int pontos = 0;
    int jogo_acabou = 0;
    
    char resumo_log[16384] = ""; 
    tRanking ranking[500]; 
    
    char caminho_saida_txt[MAX_CAMINHO];
    sprintf(caminho_saida_txt, "%s/saida/saida.txt", diretorio);
    FILE *arquivo_saida = fopen(caminho_saida_txt, "w");
    if (!arquivo_saida) { return 1; }

    // --- LOOP PRINCIPAL DO JOGO (REESTRUTURADO) ---
    while (1) {
        // ETAPA 1 (PDF): Verifica condicoes de vitoria/derrota do estado ANTERIOR
        if (jogo.galinha.y_atual <= 1) { // Vitoria
            pontos += 10;
            jogo = AtualizaDesenhoCompleto(jogo);
            printf("Pontos: %d | Vidas: %d | Iteracoes: %d\n", pontos, jogo.galinha.vidas, iteracao);
            fprintf(arquivo_saida, "Pontos: %d | Vidas: %d | Iteracoes: %d\n", pontos, jogo.galinha.vidas, iteracao);
            // Imprime o ultimo mapa
            for (int i = 0; i < jogo.altura_total; i++) {
                for (int j = 0; j < jogo.largura_total; j++) {
                    printf("%c", jogo.mapa_matriz[i][j]);
                    fprintf(arquivo_saida, "%c", jogo.mapa_matriz[i][j]);
                }
                printf("\n");
                fprintf(arquivo_saida, "\n");
            }
            printf("Parabens! Voce atravessou todas as pistas e venceu!\n");
            fprintf(arquivo_saida, "Parabens! Voce atravessou todas as pistas e venceu!\n");
            sprintf(resumo_log + strlen(resumo_log), "[%d] Fim de jogo\n", iteracao);
            jogo_acabou = 1;
        } else if (jogo.galinha.vidas <= 0) { // Derrota
            printf("Voce perdeu todas as vidas! Fim de jogo.\n");
            fprintf(arquivo_saida, "Voce perdeu todas as vidas! Fim de jogo.\n");
            sprintf(resumo_log + strlen(resumo_log), "[%d] Fim de jogo\n", iteracao);
            jogo_acabou = 1;
        }

        // Se o jogo acabou, sai do loop
        if (jogo_acabou) break;
        
        // ETAPA 7 (PDF): Exibe o estado ATUAL antes de pedir o proximo comando
        // Atualiza heatmap e estatisticas para o estado atual ANTES de mover
        int altura_atual = ConverteYparaAltura(jogo, jogo.galinha.y_atual);
        if (altura_atual > jogo.altura_maxima_alcancada) {
            jogo.altura_maxima_alcancada = altura_atual;
        }
        int i, j;
        for(i = 0; i < 2; i++){
            for(j = -1; j <= 1; j++){
                int hx = jogo.galinha.x_atual + j;
                int hy = jogo.galinha.y_atual + i;
                if(hy >= 0 && hy < MAX_LINHAS && hx >= 0 && hx < MAX_COLUNAS && jogo.heatmap[hy][hx] != -1) {
                    jogo.heatmap[hy][hx]++;
                }
            }
        }
        
        jogo = AtualizaDesenhoCompleto(jogo);
        printf("Pontos: %d | Vidas: %d | Iteracoes: %d\n", pontos, jogo.galinha.vidas, iteracao);
        fprintf(arquivo_saida, "Pontos: %d | Vidas: %d | Iteracoes: %d\n", pontos, jogo.galinha.vidas, iteracao);
        for (i = 0; i < jogo.altura_total; i++) {
            for (j = 0; j < jogo.largura_total; j++) {
                printf("%c", jogo.mapa_matriz[i][j]);
                fprintf(arquivo_saida, "%c", jogo.mapa_matriz[i][j]);
            }
            printf("\n");
            fprintf(arquivo_saida, "\n");
        }


        // ETAPA 2 (PDF): Le a jogada
        if (scanf(" %c", &comando) != 1) {
            break; // Fim da entrada
        }

        // ETAPA 3 (PDF): Move a galinha e atualiza pontos
        if (comando == 'w') {
            jogo.galinha.y_atual -= 3;
            pontos++;
        } else if (comando == 's') {
            jogo.movimentos_tras++;
            if (jogo.galinha.y_atual < jogo.galinha.y_inicial) {
                jogo.galinha.y_atual += 3;
            }
        }
        
        // ETAPA 4 (PDF): Move os carros
        jogo = MoveCarros(jogo);
        
        // ETAPA 5 (PDF): Verifica colisao
        int pista_colidida = 0, carro_colidido = 0;
        if (VerificaColisao(jogo, &pista_colidida, &carro_colidido)) {
            // Atualiza logs e estatisticas da colisao
            int altura_atrop = ConverteYparaAltura(jogo, jogo.galinha.y_atual);
            if (altura_atrop > jogo.altura_max_atropelada) jogo.altura_max_atropelada = altura_atrop;
            if (altura_atrop < jogo.altura_min_atropelada) jogo.altura_min_atropelada = altura_atrop;
            jogo.total_atropelamentos++;
            
            sprintf(resumo_log + strlen(resumo_log), "[%d] Na pista %d o carro %d atropelou a galinha na posicao (%d,%d).\n",
                    iteracao + 1, pista_colidida + 1, carro_colidido, jogo.galinha.x_atual, jogo.galinha.y_atual + 1);

            if(jogo.total_atropelamentos <= 500) {
                ranking[jogo.total_atropelamentos-1].id_pista = pista_colidida;
                ranking[jogo.total_atropelamentos-1].id_carro = carro_colidido;
                ranking[jogo.total_atropelamentos-1].iteracao = iteracao + 1;
            }
            
            // Reseta estado da galinha
            jogo.galinha.vidas--;
            pontos = 0;
            jogo.galinha.x_atual = jogo.galinha.x_inicial;
            jogo.galinha.y_atual = jogo.galinha.y_inicial;
        }
        
        // ETAPA 6 (PDF): Incrementa iteracao
        iteracao++;
    }
    
    jogo.total_movimentos = iteracao;
    int altura_final = ConverteYparaAltura(jogo, jogo.galinha.y_atual);
    if (altura_final > jogo.altura_maxima_alcancada) {
        jogo.altura_maxima_alcancada = altura_final;
    }

    SalvaArquivosFinais(diretorio, jogo, ranking, resumo_log);

    fclose(arquivo_saida);
    return 0;
}