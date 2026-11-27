/*
 * TRABALHO AVALIATIVO - ESTRUTURAS DE DADOS APLICADAS A SITUACOES REAIS
 * Aluno: Luan Masao Hirahata
 * Disciplina: Estrutura de Dados | Docente: Prof. Heleno Cardoso (UniRuy Wyden)
 *
 * TEMA: Filtragem de Ruido (Media Movel) e Historico Recente de Sensores em
 *       Sistemas Embarcados (Microcontroladores Arduino / ESP32 / STM32)
 *
 * DESCRICAO:
 * Implementacao comparativa de tres Estruturas de Dados para manter uma janela
 * deslizante (FIFO - First-In, First-Out) das ultimas N leituras de um sensor
 * de temperatura de uma estufa agricola automatizada:
 *   1. Vetor Simples (Array Estatico com Deslocamento / Shift)
 *   2. Fila Dinamica (Lista Simplesmente Encadeada com malloc/free)
 *   3. Fila Circular Estatica (Ring Buffer / Buffer Circular) -> ESCOLHIDA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAM_JANELA 5          /* Quantidade de amostras mantidas na janela */
#define LIMIAR_ALARME 30.0f   /* Temperatura limite (C) para ligar ventilacao */
#define LEITURA_MIN -50.0f    /* Faixa valida do sensor para entrada manual (C) */
#define LEITURA_MAX 150.0f

/* ESTRUTURA DE DADOS 1: VETOR SIMPLES (ARRAY ESTATICO COM DESLOCAMENTO / SHIFT)
 * Caracteristicas:
 * - Alocacao estatica em memoria contigua.
 * - Quando o vetor enche, desloca (faz shift de) N-1 elementos para a esquerda
 *   a cada nova leitura do sensor -> Custo de insercao/remocao: O(N).
 */
typedef struct {
    float dados[TAM_JANELA];
    int quantidade;
    int totalDeslocamentosMemoria; /* Contador didatico do custo operacional */
} VetorSensor;

void inicializarVetor(VetorSensor *v) {
    v->quantidade = 0;
    v->totalDeslocamentosMemoria = 0;
}

void inserirVetor(VetorSensor *v, float novaLeitura) {
    if (v->quantidade < TAM_JANELA) {
        v->dados[v->quantidade] = novaLeitura;
        v->quantidade++;
    } else {
        /* Desloca todos os elementos uma posicao para a esquerda: O(N) */
        for (int i = 0; i < TAM_JANELA - 1; i++) {
            v->dados[i] = v->dados[i + 1];
            v->totalDeslocamentosMemoria++;
        }
        v->dados[TAM_JANELA - 1] = novaLeitura;
    }
}

float calcularMediaVetor(const VetorSensor *v) {
    if (v->quantidade == 0) return 0.0f;
    float soma = 0.0f;
    for (int i = 0; i < v->quantidade; i++) {
        soma += v->dados[i];
    }
    return soma / v->quantidade;
}

void exibirVetor(const VetorSensor *v) {
    printf("  [ED 1 - Vetor Simples]    Elementos: [ ");
    for (int i = 0; i < v->quantidade; i++) {
        printf("%.1f ", v->dados[i]);
    }
    printf("] | Media: %.2f C | Copias na RAM acum.: %d\n",
           calcularMediaVetor(v), v->totalDeslocamentosMemoria);
}

/* ESTRUTURA DE DADOS 2: FILA DINAMICA (LISTA SIMPLESMENTE ENCADEADA)
 * Caracteristicas:
 * - Alocacao dinamica no Heap (malloc/free) a cada leitura do sensor.
 * - Insercao no fim (enqueue) e remocao no inicio (dequeue) em O(1).
 * - Desvantagem critica em microcontroladores: gasto extra de RAM com ponteiros
 *   em cada no e risco severo de fragmentacao da memoria Heap.
 */
typedef struct No {
    float leitura;
    struct No *prox;
} No;

typedef struct {
    No *inicio;
    No *fim;
    int quantidade;
    int totalAlocacoesHeap; /* Contador didatico de chamadas a malloc */
} FilaEncadeada;

void inicializarFilaEncadeada(FilaEncadeada *f) {
    f->inicio = NULL;
    f->fim = NULL;
    f->quantidade = 0;
    f->totalAlocacoesHeap = 0;
}

void inserirFilaEncadeada(FilaEncadeada *f, float novaLeitura) {
    /* Se a janela estiver cheia, remove o no mais antigo (dequeue em O(1)) */
    if (f->quantidade == TAM_JANELA && f->inicio != NULL) {
        No *maisAntigo = f->inicio;
        f->inicio = f->inicio->prox;
        if (f->inicio == NULL) {
            f->fim = NULL;
        }
        free(maisAntigo);
        f->quantidade--;
    }

    /* Aloca um novo no dinamicamente no Heap (enqueue em O(1)) */
    No *novo = (No *)malloc(sizeof(No));
    if (novo == NULL) {
        printf("  [ERRO CRITICO] Falha de alocacao de memoria (Heap cheio/fragmentado)!\n");
        return;
    }
    novo->leitura = novaLeitura;
    novo->prox = NULL;
    f->totalAlocacoesHeap++;

    if (f->quantidade == 0) {
        f->inicio = novo;
        f->fim = novo;
    } else {
        f->fim->prox = novo;
        f->fim = novo;
    }
    f->quantidade++;
}

float calcularMediaFilaEncadeada(const FilaEncadeada *f) {
    if (f->quantidade == 0) return 0.0f;
    float soma = 0.0f;
    No *atual = f->inicio;
    while (atual != NULL) {
        soma += atual->leitura;
        atual = atual->prox;
    }
    return soma / f->quantidade;
}

void exibirFilaEncadeada(const FilaEncadeada *f) {
    printf("  [ED 2 - Fila Encadeada]   Nos Heap:  ");
    No *atual = f->inicio;
    while (atual != NULL) {
        printf("[%.1f]->", atual->leitura);
        atual = atual->prox;
    }
    printf("NULL | Media: %.2f C | Alocacoes malloc: %d\n",
           calcularMediaFilaEncadeada(f), f->totalAlocacoesHeap);
}

void liberarFilaEncadeada(FilaEncadeada *f) {
    while (f->inicio != NULL) {
        No *temp = f->inicio;
        f->inicio = f->inicio->prox;
        free(temp);
    }
    f->fim = NULL;
    f->quantidade = 0;
}

/* ESTRUTURA DE DADOS 3: FILA CIRCULAR ESTATICA (RING BUFFER) -> MELHOR ESCOLHA
 * Caracteristicas:
 * - Alocacao 100% estatica em tempo de compilacao (zero malloc/free, zero
 *   fragmentacao de memoria e zero overhead de ponteiros por elemento).
 * - Usa aritmetica modular ((indice + 1) % TAM_JANELA) para sobrescrever a
 *   amostra mais antiga sem deslocar dados na memoria -> Insercao em O(1).
 * - Mantem a soma acumulada atualizada em O(1).
 */
typedef struct {
    float buffer[TAM_JANELA];
    int inicio;          /* Indice da leitura mais antiga */
    int fim;             /* Indice onde a proxima leitura sera gravada */
    int quantidade;      /* Quantidade atual de amostras validas */
    float somaAcumulada; /* Permite obter a media movel em O(1) */
} FilaCircular;

void inicializarFilaCircular(FilaCircular *fc) {
    for (int i = 0; i < TAM_JANELA; i++) {
        fc->buffer[i] = 0.0f;
    }
    fc->inicio = 0;
    fc->fim = 0;
    fc->quantidade = 0;
    fc->somaAcumulada = 0.0f;
}

void inserirFilaCircular(FilaCircular *fc, float novaLeitura) {
    /* Se o buffer circular ja esta cheio, subtrai o valor mais antigo e avanca 'inicio' */
    if (fc->quantidade == TAM_JANELA) {
        fc->somaAcumulada -= fc->buffer[fc->inicio];
        fc->inicio = (fc->inicio + 1) % TAM_JANELA;
        fc->quantidade--;
    }

    /* Escreve a nova leitura na posicao 'fim' e avanca circularmente em O(1) */
    fc->buffer[fc->fim] = novaLeitura;
    fc->somaAcumulada += novaLeitura;
    fc->fim = (fc->fim + 1) % TAM_JANELA;
    fc->quantidade++;
}

float calcularMediaFilaCircular(const FilaCircular *fc) {
    if (fc->quantidade == 0) return 0.0f;
    return fc->somaAcumulada / fc->quantidade;
}

void exibirFilaCircular(const FilaCircular *fc) {
    printf("  [ED 3 - Fila Circular]    Ordem FIFO:[ ");
    for (int i = 0; i < fc->quantidade; i++) {
        int idx = (fc->inicio + i) % TAM_JANELA;
        printf("%.1f ", fc->buffer[idx]);
    }
    printf("] | Media: %.2f C | Indices (ini=%d, fim=%d)\n",
           calcularMediaFilaCircular(fc), fc->inicio, fc->fim);
}

/* FUNCOES AUXILIARES DE SIMULACAO E DEMONSTRACAO */
void processarAmostra(VetorSensor *v, FilaEncadeada *fe, FilaCircular *fc,
                      float leituraBruta, int numAmostra) {
    inserirVetor(v, leituraBruta);
    inserirFilaEncadeada(fe, leituraBruta);
    inserirFilaCircular(fc, leituraBruta);

    float mediaFiltrada = calcularMediaFilaCircular(fc);

    printf("\n--------------------------------------------------------------------------------\n");
    printf("AMOSTRA #%02d | Leitura Bruta do Sensor: %.1f C\n", numAmostra, leituraBruta);
    printf("--------------------------------------------------------------------------------\n");
    exibirVetor(v);
    exibirFilaEncadeada(fe);
    exibirFilaCircular(fc);

    /* Comparacao entre decisao sem filtro vs. decisao com Estrutura de Dados */
    printf("  -> Diagnostico do Controle Embarcado (Limiar de Ventilacao = %.1f C):\n", LIMIAR_ALARME);
    if (leituraBruta > LIMIAR_ALARME && mediaFiltrada <= LIMIAR_ALARME) {
        printf("     [!] PICO DE RUIDO FILTRADO! Leitura bruta (%.1f C) acionaria o exaustor\n", leituraBruta);
        printf("         por engano, mas a Media Movel (%.2f C) manteve o sistema ESTAVEL!\n", mediaFiltrada);
    } else if (mediaFiltrada > LIMIAR_ALARME) {
        /* A media movel atenua picos, mas nao os elimina: um pico isolado muito alto
         * (acima de ~50 C nesta janela) ainda pode elevar a media acima do limiar. */
        printf("     [ACIONADO] Media movel acima do limiar (Media: %.2f C). Exaustor LIGADO.\n", mediaFiltrada);
    } else {
        printf("     [NORMAL] Temperatura estavel (Media: %.2f C). Exaustor DESLIGADO.\n", mediaFiltrada);
    }
}

/* Le uma linha do teclado e troca virgula por ponto, aceitando "25,5" e "25.5".
 * Retorna 0 no fim da entrada (EOF). */
int lerLinha(char *buf, size_t tam) {
    if (fgets(buf, (int)tam, stdin) == NULL) {
        return 0;
    }
    if (strchr(buf, '\n') == NULL) {
        /* Linha maior que o buffer: descarta o restante para nao afetar a proxima leitura */
        int c;
        while ((c = getchar()) != '\n' && c != EOF) {
        }
    }
    for (char *p = buf; *p != '\0'; p++) {
        if (*p == ',') *p = '.';
    }
    return 1;
}

void exibirComparativoMemoria(void) {
    printf("\n================================================================================\n");
    printf("  ANALISE DE CONSUMO DE MEMORIA RAM (Janela = %d amostras)\n", TAM_JANELA);
    printf("================================================================================\n");
    /* Casts para unsigned long/%lu: %zu nao funciona em compiladores Windows antigos */
    printf("  1. Vetor Simples:   %lu bytes (fixo em compilacao, mas exige %d copias por amostra)\n",
           (unsigned long)sizeof(VetorSensor), TAM_JANELA - 1);
    printf("  2. Fila Encadeada:  %lu bytes de controle + %d x %lu bytes por no = %lu bytes\n",
           (unsigned long)sizeof(FilaEncadeada), TAM_JANELA, (unsigned long)sizeof(No),
           (unsigned long)(sizeof(FilaEncadeada) + (TAM_JANELA * sizeof(No))));
    printf("                      (alem do overhead do alocador malloc e fragmentacao do Heap)\n");
    printf("  3. Fila Circular:   %lu bytes totais (fixo, 0 copias de dados e 0 chamadas malloc)\n",
           (unsigned long)sizeof(FilaCircular));
    printf("--------------------------------------------------------------------------------\n");
    printf("  Obs.: valores medidos com sizeof NESTA maquina (ponteiro = %lu bytes, int = %lu).\n",
           (unsigned long)sizeof(void *), (unsigned long)sizeof(int));
    printf("        Em MCUs de 32 bits (ESP32/STM32) ponteiro e int ocupam 4 bytes; no\n");
    printf("        Arduino Uno (AVR 8 bits), 2 bytes -> os valores reais sao menores.\n");
    printf("================================================================================\n");
}

int main(void) {
    VetorSensor vetor;
    FilaEncadeada filaDinamica;
    FilaCircular filaCircular;

    inicializarVetor(&vetor);
    inicializarFilaEncadeada(&filaDinamica);
    inicializarFilaCircular(&filaCircular);

    /* Cenario real: 11 leituras seguidas do sensor na estufa agricola.
     * As 5 primeiras preenchem a janela (~25.0 C).
     * A 6a leitura (42.0 C) e um pico isolado de ruido eletrico (> 30.0 C)!
     * As 5 seguintes mostram o pico percorrendo a janela FIFO ate ser descartado. */
    float cenarioSimulado[] = {
        24.8f, 25.0f, 25.2f, 24.9f, 25.1f, /* Janela inicial estavel (media = 25.00 C) */
        42.0f,                             /* Amostra #06: PICO DE RUIDO ELETRICO! */
        25.0f, 25.2f, 24.9f, 25.1f, 25.0f  /* Retorno a estabilidade e descarte do ruido */
    };
    int totalSimuladas = (int)(sizeof(cenarioSimulado) / sizeof(cenarioSimulado[0]));
    int contadorAmostras = 0;

    printf("================================================================================\n");
    printf("  SISTEMA EMBARCADO DE MONITORAMENTO DE TEMPERATURA - ESTUFA AGRICOLA\n");
    printf("  Aluno: Luan Masao Hirahata | Docente: Prof. Heleno Cardoso (UniRuy Wyden)\n");
    printf("  Demonstracao Pratica das 3 Estruturas de Dados (Janela FIFO = %d amostras)\n", TAM_JANELA);
    printf("================================================================================\n");

    for (int i = 0; i < totalSimuladas; i++) {
        contadorAmostras++;
        processarAmostra(&vetor, &filaDinamica, &filaCircular, cenarioSimulado[i], contadorAmostras);
    }

    exibirComparativoMemoria();

    /* Modo interativo opcional para demonstracao ao vivo durante a apresentacao */
    int opcao = -1;
    char linha[64];
    char extra;
    do {
        printf("\nMenu de Demonstracao:\n");
        printf("  [1] Inserir nova leitura manual do sensor\n");
        printf("  [2] Exibir comparativo de memoria RAM das 3 Estruturas de Dados\n");
        printf("  [0] Encerrar programa\n");
        printf("Escolha uma opcao: ");
        if (!lerLinha(linha, sizeof(linha))) {
            break; /* Fim da entrada (EOF) */
        }
        /* "%d %c" == 1 garante que a linha contem somente o numero */
        if (sscanf(linha, "%d %c", &opcao, &extra) != 1) {
            printf("  Opcao invalida! Digite apenas 0, 1 ou 2.\n");
            opcao = -1;
            continue;
        }

        if (opcao == 1) {
            float novaLeitura = 0.0f;
            printf("Digite a nova leitura de temperatura em C (ex.: 25,5): ");
            if (!lerLinha(linha, sizeof(linha))) {
                break;
            }
            if (sscanf(linha, "%f %c", &novaLeitura, &extra) != 1) {
                printf("  Leitura invalida! Use apenas numeros (ex.: 25,5 ou 25.5).\n");
            } else if (!(novaLeitura >= LEITURA_MIN && novaLeitura <= LEITURA_MAX)) {
                /* A negacao tambem rejeita NaN/infinito, que corromperiam a soma acumulada */
                printf("  Leitura fora da faixa do sensor (%.0f a %.0f C). Ignorada.\n",
                       LEITURA_MIN, LEITURA_MAX);
            } else {
                contadorAmostras++;
                processarAmostra(&vetor, &filaDinamica, &filaCircular, novaLeitura, contadorAmostras);
            }
        } else if (opcao == 2) {
            exibirComparativoMemoria();
        } else if (opcao != 0) {
            printf("  Opcao invalida! Digite apenas 0, 1 ou 2.\n");
        }
    } while (opcao != 0);

    liberarFilaEncadeada(&filaDinamica);
    printf("\nMemoria dinamica liberada. Encerrando simulacao.\n");
    return 0;
}