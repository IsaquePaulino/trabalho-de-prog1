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

void DesenhaSprite(char matriz[MAX_LINHAS][MAX_COLUNAS], int x_centro, int y_sup, char skin[2][3]) {
    int i, j;
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 3; j++) {
            int y = y_sup + i;
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
            int y = (p * 3) + 1;
            DesenhaSprite(jogo.mapa_matriz, jogo.pistas[p].carros[i].x, y, jogo.skin_carro_padrao);
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
    char caminho[MAX_CAMINHO];
    FILE *arq;

    sprintf(caminho, "%s/config_inicial.txt", diretorio);
    arq = fopen(caminho, "r");
    if (arq == NULL) exit(1);

    memset(&jogo, 0, sizeof(tJogo));
    jogo.altura_min_atropelada = 999;
    jogo.altura_max_atropelada = 0;

    int animacao;
    fscanf(arq, "%d", &animacao);

    fscanf(arq, "%d %d", &jogo.largura_mapa, &jogo.qtd_pistas);
    jogo.altura_total = (jogo.qtd_pistas * 3) + 1;
    jogo.largura_total = jogo.largura_mapa + 2;

    char linha[1024];
    fgets(linha, sizeof(linha), arq); 

    int p, k;
    for (p = 0; p < jogo.qtd_pistas; p++) {
        if (!fgets(linha, sizeof(linha), arq)) break;
        char *pos = linha;
        int nchar = 0;

        if (sscanf(pos, " G %d %d", &jogo.galinha.x_inicial, &jogo.galinha.vidas) == 2) {
            jogo.pistas[p].num_carros = 0;
        } else if (sscanf(pos, " %c %d %d %n",
                      &jogo.pistas[p].direcao,
                      &jogo.pistas[p].velocidade,
                      &jogo.pistas[p].num_carros,
                      &nchar) >= 3) {
            pos += nchar;
            for (k = 0; k < jogo.pistas[p].num_carros; k++) {
                if (sscanf(pos, "%d %n", &jogo.pistas[p].carros[k].x, &nchar) >= 1) pos += nchar;
            }
        } else {
            jogo.pistas[p].num_carros = 0;
        }
    }

    jogo.galinha.x_atual = jogo.galinha.x_inicial;
    jogo.galinha.y_inicial = jogo.altura_total - 3;
    jogo.galinha.y_atual = jogo.galinha.y_inicial;

    fclose(arq);
    return jogo;
}

tJogo LeArquivoPersonagens(char *diretorio, tJogo jogo) {
    char caminho[MAX_CAMINHO];
    FILE *arq;
    char buf[32];

    sprintf(caminho, "%s/personagens.txt", diretorio);
    arq = fopen(caminho, "r");
    if (arq == NULL) exit(1);

    if (fgets(buf, sizeof(buf), arq)) sscanf(buf, "%c%c%c", &jogo.galinha.skin[0][0], &jogo.galinha.skin[0][1], &jogo.galinha.skin[0][2]);
    if (fgets(buf, sizeof(buf), arq)) sscanf(buf, "%c%c%c", &jogo.galinha.skin[1][0], &jogo.galinha.skin[1][1], &jogo.galinha.skin[1][2]);
    if (fgets(buf, sizeof(buf), arq)) sscanf(buf, "%c%c%c", &jogo.skin_carro_padrao[0][0], &jogo.skin_carro_padrao[0][1], &jogo.skin_carro_padrao[0][2]);
    if (fgets(buf, sizeof(buf), arq)) sscanf(buf, "%c%c%c", &jogo.skin_carro_padrao[1][0], &jogo.skin_carro_padrao[1][1], &jogo.skin_carro_padrao[1][2]);

    fclose(arq);
    return jogo;
}

void SalvaArquivoInicializacao(char *diretorio, tJogo jogo) {
    char caminho[MAX_CAMINHO];
    FILE *arq;
    sprintf(caminho, "%s/saida/inicializacao.txt", diretorio);
    arq = fopen(caminho, "w");
    if (arq == NULL) return;

    int i, j;
    for (i = 0; i < jogo.altura_total; i++) {
        for (j = 0; j < jogo.largura_total; j++) {
            fprintf(arq, "%c", jogo.mapa_matriz[i][j]);
        }
        fprintf(arq, "\n");
    }
    fprintf(arq, "A posicao central da galinha iniciara em (%d %d).", jogo.galinha.x_inicial, jogo.galinha.y_inicial);
    fclose(arq);
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
        int y_pista = (p * 3) + 1;
        if (jogo.galinha.y_atual == y_pista) {
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
    char caminho[MAX_CAMINHO];
    FILE *arq;
    int i, j;

    sprintf(caminho, "%s/saida/estatisticas.txt", diretorio);
    arq = fopen(caminho, "w");
    if (!arq) return;
    fprintf(arq, "Numero total de movimentos: %d\n", jogo.total_movimentos);
    fprintf(arq, "Altura maxima que a galinha chegou: %d\n", jogo.altura_maxima_alcancada);
    if (jogo.total_atropelamentos == 0) {
        fprintf(arq, "Altura maxima que a galinha foi atropelada: 0\n");
        fprintf(arq, "Altura minima que a galinha foi atropelada: 0\n");
    } else {
        fprintf(arq, "Altura maxima que a galinha foi atropelada: %d\n", jogo.altura_max_atropelada);
        fprintf(arq, "Altura minima que a galinha foi atropelada: %d\n", jogo.altura_min_atropelada);
    }
    fprintf(arq, "Numero de movimentos na direcao oposta: %d\n", jogo.movimentos_tras);
    fclose(arq);

    sprintf(caminho, "%s/saida/heatmap.txt", diretorio);
    arq = fopen(caminho, "w");
    if (!arq) return;
    for (i = 1; i < jogo.altura_total; i++) {
        if (i % 3 == 0) continue;
        for (j = 1; j < jogo.largura_total - 1; j++) {
            int valor = jogo.heatmap[i][j];
             if (valor == -1) {
                 fprintf(arq, "  *");
            } else {
                 if (valor > 99) valor = 99;
                 fprintf(arq, "%3d", valor);
            }
        }
        fprintf(arq, "\n");
    }
    fclose(arq);

    sprintf(caminho, "%s/saida/ranking.txt", diretorio);
    arq = fopen(caminho, "w");
    if (!arq) return;
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
    fprintf(arq, "id_pista,id_carro,iteracao\n");
    for (i = 0; i < jogo.total_atropelamentos; i++) {
        fprintf(arq, "%d,%d,%d\n", ranking[i].id_pista, ranking[i].id_carro, ranking[i].iteracao);
    }
    fclose(arq);

    sprintf(caminho, "%s/saida/resumo.txt", diretorio);
    arq = fopen(caminho, "w");
    if (!arq) return;
    fprintf(arq, "%s", resumo_log);
    fclose(arq);
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
    
    char resumo_log[16384] = ""; 
    tRanking ranking[500]; 
    
    char caminho_saida_txt[MAX_CAMINHO];
    sprintf(caminho_saida_txt, "%s/saida/saida.txt", diretorio);
    FILE *arquivo_saida = fopen(caminho_saida_txt, "w");
    if (!arquivo_saida) { return 1; }

    // Loop principal com a ordem de eventos corrigida para seguir o PDF
    while (1) {
        // PASSO 1: Atualiza heatmap e estatisticas para a posicao atual
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
                    if (jogo.heatmap[hy][hx] < 99) jogo.heatmap[hy][hx]++;
                }
            }
        }

        // PASSO 2: Exibe o estado da iteracao atual
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
        
        // PASSO 3: Verifica se o jogo acabou
        if (jogo.galinha.y_atual <= 1) { // Vitoria
            printf("Parabens! Voce atravessou todas as pistas e venceu!\n");
            fprintf(arquivo_saida, "Parabens! Voce atravessou todas as pistas e venceu!\n");
            pontos += 10;
            sprintf(resumo_log + strlen(resumo_log), "[%d] Fim de jogo\n", iteracao);
            break; 
        }
        if (jogo.galinha.vidas <= 0) { // Derrota
            printf("Voce perdeu todas as vidas! Fim de jogo.\n");
            fprintf(arquivo_saida, "Voce perdeu todas as vidas! Fim de jogo.\n");
            sprintf(resumo_log + strlen(resumo_log), "[%d] Fim de jogo\n", iteracao);
            break;
        }

        // PASSO 4: Le o proximo comando
        if (scanf(" %c", &comando) != 1) {
            break; 
        }
        
        iteracao++; // A iteracao avanca apos um comando ser lido
        
        // PASSO 5: Processa o comando e atualiza o estado do jogo
        if (comando == 'w') {
            jogo.galinha.y_atual -= 3;
            pontos++;
        } else if (comando == 's') {
            jogo.movimentos_tras++;
            if (jogo.galinha.y_atual < jogo.galinha.y_inicial) {
                jogo.galinha.y_atual += 3;
            }
        }
        
        jogo = MoveCarros(jogo);
        
        int pista_colidida = 0, carro_colidido = 0;
        if (VerificaColisao(jogo, &pista_colidida, &carro_colidido)) {
            int h_atrop = ConverteYparaAltura(jogo, jogo.galinha.y_atual);
            if (h_atrop > jogo.altura_max_atropelada) jogo.altura_max_atropelada = h_atrop;
            if (h_atrop < jogo.altura_min_atropelada) jogo.altura_min_atropelada = h_atrop;
            
            sprintf(resumo_log + strlen(resumo_log), "[%d] Na pista %d o carro %d atropelou a galinha na posicao (%d,%d).\n",
                    iteracao, pista_colidida + 1, carro_colidido, jogo.galinha.x_atual, jogo.galinha.y_atual + 1);

            if (jogo.total_atropelamentos < 500) {
                ranking[jogo.total_atropelamentos].id_pista = pista_colidida + 1;
                ranking[jogo.total_atropelamentos].id_carro = carro_colidido;
                ranking[jogo.total_atropelamentos].iteracao = iteracao;
            }
            jogo.total_atropelamentos++;
            
            for(i = 0; i < 2; i++){
                for(j = 1; j < jogo.largura_total - 1; j++){
                    jogo.heatmap[jogo.galinha.y_atual + i][j] = -1;
                }
            }
            
            jogo.galinha.vidas--;
            pontos = 0;
            jogo.galinha.x_atual = jogo.galinha.x_inicial;
            jogo.galinha.y_atual = jogo.galinha.y_inicial;
        }
    }
    
    // PASSO FINAL: Salva os arquivos de resultado
    fclose(arquivo_saida);
    
    int hf = ConverteYparaAltura(jogo, jogo.galinha.y_atual);
    if (hf > jogo.altura_maxima_alcancada) jogo.altura_maxima_alcancada = hf;
    jogo.total_movimentos = iteracao;
    
    SalvaArquivosFinais(diretorio, jogo, ranking, resumo_log);
    
    return 0;
}