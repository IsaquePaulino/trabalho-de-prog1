#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ====== LIMITES (não usar malloc, ponteiros, etc) ====== */
#define MAX_LINHAS 37
#define MAX_COLUNAS 102
#define MAX_PISTAS 12
#define MAX_CARROS_PISTA 10
#define MAX_CAMINHO 1000

/* ========================= ESTRUTURAS ========================= */

typedef struct {
    int x;
} tCarro;

typedef struct {
    char direcao;              /* 'D' direita, 'E' esquerda */
    int  velocidade;           /* passos por iteração */
    int  num_carros;
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
    int id_pista;              /* 1..(qtd_pistas-2) */
    int id_carro;              /* 1..num_carros */
    int iteracao;              /* jogada válida (1-based) */
} tRanking;

typedef struct {
    int houve;                 /* 1 se houve colisão nessa checagem */
    int pista;                 /* índice interno 0..qtd_pistas-1 (faixa) */
    int carro;                 /* 1..num_carros */
} tColisao;

typedef struct {
    int largura_mapa;                       /* largura útil sem bordas */
    int qtd_pistas;                         /* quantidade de “pistas” no arquivo */
    int altura_total;                       /* linhas totais incluindo bordas */
    int largura_total;                      /* colunas totais incluindo bordas */

    char mapa_matriz[MAX_LINHAS][MAX_COLUNAS];

    tPista pistas[MAX_PISTAS];
    tGalinha galinha;

    char skin_carro_padrao[2][3];

    int total_movimentos;                   /* SOMENTE jogadas válidas (w/s) */
    int movimentos_tras;                    /* número de 's' aceitos */
    int altura_maxima_alcancada;

    int altura_max_atropelada;              /* maior altura em que foi atropelada */
    int altura_min_atropelada;              /* menor altura em que foi atropelada */
    int total_atropelamentos;

    int heatmap[MAX_LINHAS][MAX_COLUNAS];
} tJogo;

/* ===================== DESENHO E MAPA ===================== */

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
    /* linhas tracejadas entre pistas (a cada 3 linhas, iniciando em 3) */
    for (i = 3; i < jogo.altura_total - 1; i += 3) {
        for (j = 1; j < jogo.largura_total - 1; j++) {
            jogo.mapa_matriz[i][j] = ((j - 1) % 3 == 2) ? ' ' : '-';
        }
    }
    return jogo;
}

/* ATENÇÃO: limites com base em jogo.altura_total/largura_total (não MAX_*) */
void DesenhaSprite(char matriz[MAX_LINHAS][MAX_COLUNAS],
                   int x_centro, int y_sup, char skin[2][3],
                   int altura_total, int largura_total) {
    int i, j, y, x;
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 3; j++) {
            y = y_sup + i;
            x = x_centro - 1 + j;
            if (y > 0 && y < altura_total - 1 && x > 0 && x < largura_total - 1) {
                matriz[y][x] = skin[i][j];
            }
        }
    }
}

tJogo DesenhaCarros(tJogo jogo) {
    int p, i, y;
    for (p = 1; p < jogo.qtd_pistas - 1; p++) {
        y = (p * 3) + 1; /* linha superior da faixa */
        for (i = 0; i < jogo.pistas[p].num_carros; i++) {
            DesenhaSprite(jogo.mapa_matriz,
                          jogo.pistas[p].carros[i].x, y,
                          jogo.skin_carro_padrao,
                          jogo.altura_total, jogo.largura_total);
        }
    }
    return jogo;
}

tJogo DesenhaGalinha(tJogo jogo) {
    DesenhaSprite(jogo.mapa_matriz,
                  jogo.galinha.x_atual, jogo.galinha.y_atual,
                  jogo.galinha.skin,
                  jogo.altura_total, jogo.largura_total);
    return jogo;
}

tJogo AtualizaDesenhoCompleto(tJogo jogo) {
    jogo = DesenhaBaseMapa(jogo);
    jogo = DesenhaCarros(jogo);
    jogo = DesenhaGalinha(jogo);
    return jogo;
}

/* ===================== LEITURA DE ARQUIVOS ===================== */

void MensagemErroArquivo(const char *caminho) {
    printf("ERRO: Nao foi possivel ler o arquivo: %s\n", caminho);
}

tJogo LeArquivoConfig(char *diretorio) {
    tJogo jogo;
    char caminho[MAX_CAMINHO];
    FILE *arq;
    char linha[1024];
    int p, k, nchar;
    int animacao;

    memset(&jogo, 0, sizeof(tJogo));
    jogo.altura_min_atropelada = 999;
    jogo.altura_max_atropelada = 0;

    sprintf(caminho, "%s/config_inicial.txt", diretorio);
    arq = fopen(caminho, "r");
    if (arq == NULL) {
        MensagemErroArquivo(caminho);
        exit(1);
    }

    if (fscanf(arq, "%d", &animacao) != 1) {
        MensagemErroArquivo(caminho);
        fclose(arq);
        exit(1);
    }

    if (fscanf(arq, "%d %d", &jogo.largura_mapa, &jogo.qtd_pistas) != 2) {
        MensagemErroArquivo(caminho);
        fclose(arq);
        exit(1);
    }

    jogo.altura_total = (jogo.qtd_pistas * 3) + 1;
    jogo.largura_total = jogo.largura_mapa + 2;

    fgets(linha, sizeof(linha), arq); /* consome resto da linha */

    for (p = 0; p < jogo.qtd_pistas; p++) {
        if (!fgets(linha, sizeof(linha), arq)) break;

        if (sscanf(linha, " G %d %d", &jogo.galinha.x_inicial, &jogo.galinha.vidas) == 2) {
            jogo.pistas[p].num_carros = 0;
        } else if (sscanf(linha, " %c %d %d %n",
                          &jogo.pistas[p].direcao,
                          &jogo.pistas[p].velocidade,
                          &jogo.pistas[p].num_carros,
                          &nchar) >= 3) {
            char *pos = linha + nchar;
            for (k = 0; k < jogo.pistas[p].num_carros; k++) {
                if (sscanf(pos, "%d %n", &jogo.pistas[p].carros[k].x, &nchar) >= 1) {
                    pos += nchar;
                }
            }
        } else {
            jogo.pistas[p].num_carros = 0;
        }
    }

    jogo.galinha.y_inicial = jogo.altura_total - 3; /* calçada de baixo */
    jogo.galinha.y_atual   = jogo.galinha.y_inicial;

    if (jogo.galinha.x_inicial < 2) jogo.galinha.x_inicial = 2;
    if (jogo.galinha.x_inicial > jogo.largura_mapa - 1) jogo.galinha.x_inicial = jogo.largura_mapa - 1;
    jogo.galinha.x_atual = jogo.galinha.x_inicial;

    fclose(arq);
    return jogo;
}

tJogo LeArquivoPersonagens(char *diretorio, tJogo jogo) {
    char caminho[MAX_CAMINHO];
    FILE *arq;
    char buf[32];

    sprintf(caminho, "%s/personagens.txt", diretorio);
    arq = fopen(caminho, "r");
    if (arq == NULL) {
        MensagemErroArquivo(caminho);
        exit(1);
    }

    if (fgets(buf, sizeof(buf), arq)) sscanf(buf, "%c%c%c", &jogo.galinha.skin[0][0], &jogo.galinha.skin[0][1], &jogo.galinha.skin[0][2]);
    if (fgets(buf, sizeof(buf), arq)) sscanf(buf, "%c%c%c", &jogo.galinha.skin[1][0], &jogo.galinha.skin[1][1], &jogo.galinha.skin[1][2]);
    if (fgets(buf, sizeof(buf), arq)) sscanf(buf, "%c%c%c", &jogo.skin_carro_padrao[0][0], &jogo.skin_carro_padrao[0][1], &jogo.skin_carro_padrao[0][2]);
    if (fgets(buf, sizeof(buf), arq)) sscanf(buf, "%c%c%c", &jogo.skin_carro_padrao[1][0], &jogo.skin_carro_padrao[1][1], &jogo.skin_carro_padrao[1][2]);

    fclose(arq);
    return jogo;
}

/* ===================== HEATMAP E AJUDANTES ===================== */

int CelulaValida(tJogo jogo, int y, int x) {
    if (y <= 0 || y >= jogo.altura_total - 1) return 0;
    if (x <= 0 || x >= jogo.largura_total - 1) return 0;
    if (y % 3 == 0) return 0; /* linha tracejada não é faixa útil */
    return 1;
}

tJogo HeatmapIncrementaGalinha(tJogo jogo) {
    int di, dj, hy, hx;
    for (di = 0; di < 2; di++) {
        for (dj = -1; dj <= 1; dj++) {
            hy = jogo.galinha.y_atual + di;
            hx = jogo.galinha.x_atual + dj;
            if (CelulaValida(jogo, hy, hx) && jogo.heatmap[hy][hx] != -1) {
                if (jogo.heatmap[hy][hx] < 99) jogo.heatmap[hy][hx]++;
            }
        }
    }
    return jogo;
}

/* Marca '*' nas DUAS linhas da faixa onde ocorreu o atropelamento */
tJogo HeatmapMarcaColisaoFaixa(tJogo jogo, int y_pista) {
    int i, x, hy;
    for (i = 0; i < 2; i++) {
        hy = y_pista + i;
        if (hy > 0 && hy < jogo.altura_total - 1 && hy % 3 != 0) {
            for (x = 1; x < jogo.largura_total - 1; x++) {
                jogo.heatmap[hy][x] = -1;
            }
        }
    }
    return jogo;
}

/* ===================== SAÍDAS EM ARQUIVOS ===================== */

void SalvaArquivoInicializacao(char *diretorio, tJogo jogo) {
    char caminho[MAX_CAMINHO];
    FILE *arq;
    int i, j;

    sprintf(caminho, "%s/saida/inicializacao.txt", diretorio);
    arq = fopen(caminho, "w");
    if (arq == NULL) return;

    for (i = 0; i < jogo.altura_total; i++) {
        for (j = 0; j < jogo.largura_total; j++) {
            fputc(jogo.mapa_matriz[i][j], arq);
        }
        fputc('\n', arq);
    }
    fprintf(arq, "A posicao central da galinha iniciara em (%d %d).",
            jogo.galinha.x_inicial, jogo.galinha.y_inicial);
    fclose(arq);
}

void SalvaArquivosFinais(char *diretorio, tJogo jogo, tRanking ranking[], char resumo_log[]) {
    char caminho[MAX_CAMINHO];
    FILE *arq;
    int i, j;
    int trocou;

    /* estatistica.txt (singular) */
    sprintf(caminho, "%s/saida/estatistica.txt", diretorio);
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

    /* heatmap.txt */
    sprintf(caminho, "%s/saida/heatmap.txt", diretorio);
    arq = fopen(caminho, "w");
    if (!arq) return;
    for (i = 1; i < jogo.altura_total; i++) {
        if (i % 3 == 0) continue;
        for (j = 1; j < jogo.largura_total - 1; j++) {
            int valor = jogo.heatmap[i][j];
            if (valor == -1) fprintf(arq, "  *");
            else {
                if (valor > 99) valor = 99;
                fprintf(arq, "%3d", valor);
            }
        }
        fputc('\n', arq);
    }
    fclose(arq);

    /* ranking.txt (ordenar: pista↑, carro↑, iteracao↓) */
    sprintf(caminho, "%s/saida/ranking.txt", diretorio);
    arq = fopen(caminho, "w");
    if (!arq) return;

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
                tRanking tmp = ranking[i];
                ranking[i] = ranking[i + 1];
                ranking[i + 1] = tmp;
                trocou = 1;
            }
        }
    } while (trocou);

    fprintf(arq, "id_pista,id_carro,iteracao\n");
    for (i = 0; i < jogo.total_atropelamentos; i++) {
        fprintf(arq, "%d,%d,%d\n", ranking[i].id_pista, ranking[i].id_carro, ranking[i].iteracao);
    }
    fclose(arq);

    /* resumo.txt */
    sprintf(caminho, "%s/saida/resumo.txt", diretorio);
    arq = fopen(caminho, "w");
    if (!arq) return;
    fprintf(arq, "%s", resumo_log);
    fclose(arq);
}

/* ===================== MECÂNICA DO JOGO ===================== */

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

tColisao VerificaColisao(tJogo jogo) {
    tColisao c;
    int p, i, y_pista;

    c.houve = 0;
    c.pista = 0;
    c.carro = 0;

    for (p = 1; p < jogo.qtd_pistas - 1; p++) {
        y_pista = (p * 3) + 1; /* linha superior da faixa */
        if (jogo.galinha.y_atual == y_pista) {
            for (i = 0; i < jogo.pistas[p].num_carros; i++) {
                if (abs(jogo.galinha.x_atual - jogo.pistas[p].carros[i].x) <= 2) {
                    c.houve = 1;
                    c.pista = p;
                    c.carro = i + 1;
                    return c;
                }
            }
        }
    }
    return c;
}

/* ============================== MAIN ============================== */

int main(int argc, char *argv[]) {
    tJogo jogo;
    tRanking ranking[500];
    char resumo_log[32768];                /* mais folga */
    char caminho_saida_txt[MAX_CAMINHO];
    FILE *arquivo_saida;
    char *diretorio;

    int i, j;

    int iteracao_tela;                     /* número de telas impressas (inclui espaço) */
    int jogada_valida;                     /* 1..N (conta apenas w/s) para logs/ranking */
    int pontos;

    int venceu;                            /* 1 quando alcança o topo ao menos uma vez */
    int bonus_dado;                        /* para não somar +10 mais de uma vez */
    int invencivel_ticks;                  /* evita colisão “fantasma” logo após reset */

    char comando;

    if (argc < 2) {
        printf("ERRO: Informe o diretorio com os arquivos de configuracao.\n");
        return 1;
    }
    diretorio = argv[1];

    jogo = LeArquivoConfig(diretorio);
    jogo = LeArquivoPersonagens(diretorio, jogo);

    /* zera heatmap e estatísticas */
    for (i = 0; i < MAX_LINHAS; i++) {
        for (j = 0; j < MAX_COLUNAS; j++) {
            jogo.heatmap[i][j] = 0;
        }
    }
    resumo_log[0] = '\0';
    jogo.total_atropelamentos = 0;
    jogo.movimentos_tras = 0;
    jogo.altura_maxima_alcancada = 0;

    venceu = 0;
    bonus_dado = 0;
    invencivel_ticks = 0;

    /* Desenha inicial e salva inicializacao.txt */
    jogo = AtualizaDesenhoCompleto(jogo);
    SalvaArquivoInicializacao(diretorio, jogo);

    /* iteração 0 conta no heatmap e altura */
    jogo = HeatmapIncrementaGalinha(jogo);
    {
        int a0 = ConverteYparaAltura(jogo, jogo.galinha.y_atual);
        if (a0 > jogo.altura_maxima_alcancada) jogo.altura_maxima_alcancada = a0;
    }

    /* abre saida.txt */
    sprintf(caminho_saida_txt, "%s/saida/saida.txt", diretorio);
    arquivo_saida = fopen(caminho_saida_txt, "w");
    if (!arquivo_saida) { return 1; }

    /* ---- TELA DA ITERACAO 0 ---- */
    jogo = AtualizaDesenhoCompleto(jogo);
    printf("Pontos: %d | Vidas: %d | Iteracoes: %d\n", 0, jogo.galinha.vidas, 0);
    fprintf(arquivo_saida, "Pontos: %d | Vidas: %d | Iteracoes: %d\n", 0, jogo.galinha.vidas, 0);
    for (i = 0; i < jogo.altura_total; i++) {
        for (j = 0; j < jogo.largura_total; j++) {
            putchar(jogo.mapa_matriz[i][j]);
            fputc(jogo.mapa_matriz[i][j], arquivo_saida);
        }
        putchar('\n');
        fputc('\n', arquivo_saida);
    }

    /* LOOP PRINCIPAL */
    iteracao_tela = 0;    /* quantas telas já imprimimos (0 já feita acima) */
    jogada_valida = 0;    /* contador de w/s */
    jogo.total_movimentos = 0;
    pontos = 0;

    while (1) {
        /* lê comando (pode ser espaço) */
        if (scanf(" %c", &comando) != 1) {
            /* fim da entrada: apenas finaliza fora do loop */
            break;
        }

        /* processa comando da galinha */
        if (comando == 'w') {
            jogo.galinha.y_atual -= 3;
            jogada_valida++;
            jogo.total_movimentos++;
            pontos++;
        } else if (comando == 's') {
            if (jogo.galinha.y_atual < jogo.galinha.y_inicial) {
                jogo.galinha.y_atual += 3;
            }
            jogo.movimentos_tras++;
            jogada_valida++;
            jogo.total_movimentos++;
        } else {
            /* espaço: não altera posição da galinha, e NÃO conta movimento */
        }

        /* checa vitória, mas NÃO encerra o jogo agora */
        if (jogo.galinha.y_atual <= 1) {
            venceu = 1;
            if (!bonus_dado) {
                pontos += 10;  /* bônus uma única vez */
                bonus_dado = 1;
            }
            /* y_atual pode ficar em 1; continua simulando carros normalmente */
        }

        /* move carros sempre */
        jogo = MoveCarros(jogo);

        /* colisão (pulando um tick após reset para evitar colisão fantasma) */
        if (invencivel_ticks > 0) {
            invencivel_ticks--;  /* ignora checagem nesta rodada */
        } else {
            tColisao col = VerificaColisao(jogo);
            if (col.houve) {
                int y_faixa, h_atrop;

                y_faixa = (col.pista * 3) + 1; /* linha superior da faixa */
                h_atrop = ConverteYparaAltura(jogo, y_faixa);

                if (h_atrop > jogo.altura_max_atropelada) jogo.altura_max_atropelada = h_atrop;
                if (h_atrop < jogo.altura_min_atropelada) jogo.altura_min_atropelada = h_atrop;

                /* LOG de colisão: usar jogada_valida (1-based) e (x, y_atual da sprite superior) */
                {
                    int len = (int)strlen(resumo_log);
                    sprintf(resumo_log + len,
                            "[%d] Na pista %d o carro %d atropelou a galinha na posicao (%d,%d).\n",
                            jogada_valida, col.pista + 1, col.carro,
                            jogo.galinha.x_atual, y_faixa);
                }

                /* ranking: mesma iteração (jogada válida atual) */
                if (jogo.total_atropelamentos < 500) {
                    ranking[jogo.total_atropelamentos].id_pista = col.pista + 1;
                    ranking[jogo.total_atropelamentos].id_carro = col.carro;
                    ranking[jogo.total_atropelamentos].iteracao = jogada_valida;
                }
                jogo.total_atropelamentos++;

                /* heatmap: marca a faixa inteira (duas linhas) */
                jogo = HeatmapMarcaColisaoFaixa(jogo, y_faixa);

                /* penalidades e reset da galinha */
                jogo.galinha.vidas--;
                pontos = 0;
                jogo.galinha.x_atual = jogo.galinha.x_inicial;
                jogo.galinha.y_atual = jogo.galinha.y_inicial;

                /* evita uma nova colisão já no próximo tick após reset */
                invencivel_ticks = 1;

                /* registra retorno no heatmap */
                jogo = HeatmapIncrementaGalinha(jogo);
            }
        }

        /* estatísticas de altura e presença */
        {
            int altura_atual = ConverteYparaAltura(jogo, jogo.galinha.y_atual);
            if (altura_atual > jogo.altura_maxima_alcancada) {
                jogo.altura_maxima_alcancada = altura_atual;
            }
            jogo = HeatmapIncrementaGalinha(jogo);
        }

        /* imprime a tela desta jogada */
        iteracao_tela++;

        jogo = AtualizaDesenhoCompleto(jogo);
        printf("Pontos: %d | Vidas: %d | Iteracoes: %d\n", pontos, jogo.galinha.vidas, iteracao_tela);
        fprintf(arquivo_saida, "Pontos: %d | Vidas: %d | Iteracoes: %d\n", pontos, jogo.galinha.vidas, iteracao_tela);
        for (i = 0; i < jogo.altura_total; i++) {
            for (j = 0; j < jogo.largura_total; j++) {
                putchar(jogo.mapa_matriz[i][j]);
                fputc(jogo.mapa_matriz[i][j], arquivo_saida);
            }
            putchar('\n');
            fputc('\n', arquivo_saida);
        }

        /* checa derrota (só para abortar entrada cedo se quiser) — mas não damos "break" aqui,
           deixamos terminar pela leitura da entrada para bater com os casos de teste. */
        if (jogo.galinha.vidas <= 0) {
            /* continua até acabar a entrada, a mensagem de fim vai ao final */
        }
    }

    /* fim da entrada: imprime a mensagem final apropriada e registra no resumo */
    {
        int len = (int)strlen(resumo_log);
        if (jogo.galinha.vidas <= 0) {
            printf("Voce perdeu todas as vidas! Fim de jogo.\n");
            fprintf(arquivo_saida, "Voce perdeu todas as vidas! Fim de jogo.\n");
            sprintf(resumo_log + len, "[%d] Fim de jogo\n", jogada_valida);
        } else if (venceu) {
            printf("Parabens! Voce atravessou todas as pistas e venceu!\n");
            fprintf(arquivo_saida, "Parabens! Voce atravessou todas as pistas e venceu!\n");
            sprintf(resumo_log + len, "[%d] Fim de jogo\n", jogada_valida);
        } else {
            /* nem ganhou nem perdeu, acabou a entrada */
            printf("Fim de jogo.\n");
            fprintf(arquivo_saida, "Fim de jogo.\n");
            sprintf(resumo_log + len, "[%d] Fim de jogo\n", jogada_valida);
        }
    }

    fclose(arquivo_saida);

    /* total_movimentos já é contado só em w/s */
    {
        int hf = ConverteYparaAltura(jogo, jogo.galinha.y_atual);
        if (hf > jogo.altura_maxima_alcancada) jogo.altura_maxima_alcancada = hf;
    }

    SalvaArquivosFinais(diretorio, jogo, ranking, resumo_log);
    return 0;
}
