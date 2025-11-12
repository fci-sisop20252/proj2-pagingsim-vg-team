#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Variáveis Globais de Configuração ---
int NUM_FRAMES;
int TAMANHO_DA_PAGINA;
int NUM_PROCESSOS;

// --- Estruturas de Dados ---
typedef struct {
    int frame_fisico;
    int presente;
    int r_bit;
    int tempo_carga;
} EntradaPagina;

typedef struct {
    int pid;
    int tamanho_virtual;
    int num_paginas;
    EntradaPagina *tabela_paginas;
} Processo;

typedef struct {
    int ocupado;
    int pid_dono;
    int pagina_dono;
} FrameFisico;

// --- Ponteiros para os Dados Principais ---
Processo *processos;
FrameFisico *memoria_fisica;

// --- Variáveis de Controle de Algoritmos e Estatísticas ---
int total_acessos = 0;
int total_page_faults = 0;
int ponteiro_clock = 0;
int contador_carga = 0;

// ----------------------------------------------------------------------
// --- Funções Auxiliares ---
// ----------------------------------------------------------------------

void traduzir_endereco(int endereco_virtual, int *pagina, int *deslocamento) {
    *pagina = endereco_virtual / TAMANHO_DA_PAGINA;
    *deslocamento = endereco_virtual % TAMANHO_DA_PAGINA;
}

int encontrar_frame_livre() {
    for (int f = 0; f < NUM_FRAMES; f++) {
        if (memoria_fisica[f].ocupado == 0) {
            return f;
        }
    }
    return -1;
}

int selecionar_vitima_fifo() {
    int frame_vitima = -1;
    int menor_tempo_carga = -1;
    for (int f = 0; f < NUM_FRAMES; f++) {
        int pid_dono = memoria_fisica[f].pid_dono;
        int pagina_dono = memoria_fisica[f].pagina_dono;
        int tempo = processos[pid_dono].tabela_paginas[pagina_dono].tempo_carga;
        if (frame_vitima == -1 || tempo < menor_tempo_carga) {
            menor_tempo_carga = tempo;
            frame_vitima = f;
        }
    }
    return frame_vitima;
}

int selecionar_vitima_clock() {
    while (1) {
        int pid_dono = memoria_fisica[ponteiro_clock].pid_dono;
        int pagina_dono = memoria_fisica[ponteiro_clock].pagina_dono;

        EntradaPagina *entrada_vitima = &processos[pid_dono].tabela_paginas[pagina_dono];

        if (entrada_vitima->r_bit == 0) {
            int frame_vitima = ponteiro_clock;
            ponteiro_clock = (ponteiro_clock + 1) % NUM_FRAMES;
            return frame_vitima;
        } else {
            entrada_vitima->r_bit = 0;
            ponteiro_clock = (ponteiro_clock + 1) % NUM_FRAMES;
        }
    }
}

void liberar_memoria() {
    if (processos) {
        for (int i = 0; i < NUM_PROCESSOS; i++) {
            if (processos[i].tabela_paginas) {
                free(processos[i].tabela_paginas);
            }
        }
        free(processos);
        processos = NULL;
    }
    if (memoria_fisica) {
        free(memoria_fisica);
        memoria_fisica = NULL;
    }
}

// ----------------------------------------------------------------------
// --- Lógica Principal de Gerenciamento (processar_acesso) ---
// ----------------------------------------------------------------------

void processar_acesso(char *algoritmo, int pid, int endereco_virtual) {
    total_acessos++;
    int pagina, deslocamento;

    traduzir_endereco(endereco_virtual, &pagina, &deslocamento);

    if (pagina >= processos[pid].num_paginas) {
        fprintf(stderr, "Aviso: Acesso inválido (Endereço %d fora do limite do PID %d). Ignorando.\n",
                endereco_virtual, pid);
        total_acessos--;
        return;
    }

    EntradaPagina *entrada_tp = &processos[pid].tabela_paginas[pagina];

    
    if (entrada_tp->presente == 1) {
        int frame = entrada_tp->frame_fisico;
        entrada_tp->r_bit = 1;
        printf("Acesso: PID %d, Endereço %d (Página %d, Deslocamento %d) -> HIT: Página %d (PID %d) já está no Frame %d\n",
               pid, endereco_virtual, pagina, deslocamento, pagina, pid, frame);
    } else {
        total_page_faults++;
        int frame_alocado = encontrar_frame_livre();

        if (frame_alocado != -1) {
            printf("Acesso: PID %d, Endereço %d (Página %d, Deslocamento %d) -> PAGE FAULT -> Página %d (PID %d) alocada no Frame livre %d\n",
                   pid, endereco_virtual, pagina, deslocamento, pagina, pid, frame_alocado);
        } else {
            int frame_vitima;
            if (strcmp(algoritmo, "fifo") == 0) {
                frame_vitima = selecionar_vitima_fifo();
            } else {
                frame_vitima = selecionar_vitima_clock();
            }

            int pid_antigo = memoria_fisica[frame_vitima].pid_dono;
            int pagina_antiga = memoria_fisica[frame_vitima].pagina_dono;

            printf("Acesso: PID %d, Endereço %d (Página %d, Deslocamento %d) -> PAGE FAULT -> Memória cheia. Página %d (PID %d) (Frame %d) será desalocada. -> Página %d (PID %d) alocada no Frame %d\n",
                   pid, endereco_virtual, pagina, deslocamento,
                   pagina_antiga, pid_antigo, frame_vitima,
                   pagina, pid, frame_vitima);

            processos[pid_antigo].tabela_paginas[pagina_antiga].presente = 0;
            processos[pid_antigo].tabela_paginas[pagina_antiga].frame_fisico = -1;

            frame_alocado = frame_vitima;
        }

        entrada_tp->presente = 1;
        entrada_tp->frame_fisico = frame_alocado;
        entrada_tp->tempo_carga = contador_carga++;
        entrada_tp->r_bit = 1;

        memoria_fisica[frame_alocado].ocupado = 1;
        memoria_fisica[frame_alocado].pid_dono = pid;
        memoria_fisica[frame_alocado].pagina_dono = pagina;
    }
}

// ----------------------------------------------------------------------
// --- Funções de Leitura e Inicialização ---
// ----------------------------------------------------------------------

int ler_configuracao(char *arquivo_config) {
    FILE *fc = fopen(arquivo_config, "r");
    if (!fc) {
        perror("Erro ao abrir arquivo de configuração");
        return 0;
    }

    if (fscanf(fc, "%d", &NUM_FRAMES) != 1 ||
        fscanf(fc, "%d", &TAMANHO_DA_PAGINA) != 1 ||
        fscanf(fc, "%d", &NUM_PROCESSOS) != 1) {
        fprintf(stderr, "Erro ao ler cabeçalho do arquivo de configuração.\n");
        fclose(fc);
        return 0;
    }

    memoria_fisica = (FrameFisico*) malloc(NUM_FRAMES * sizeof(FrameFisico));
    if (!memoria_fisica) {
        perror("Erro ao alocar memória física");
        fclose(fc);
        return 0;
    }
    for (int f = 0; f < NUM_FRAMES; f++) {
        memoria_fisica[f].ocupado = 0;
        memoria_fisica[f].pid_dono = -1;
        memoria_fisica[f].pagina_dono = -1;
    }

    processos = (Processo*) malloc(NUM_PROCESSOS * sizeof(Processo));
    if (!processos) {
        perror("Erro ao alocar processos");
        free(memoria_fisica);
        fclose(fc);
        return 0;
    }

    for (int i = 0; i < NUM_PROCESSOS; i++) {
        int pid_lido;
        int tam_virt_lido;
        if (fscanf(fc, "%d %d", &pid_lido, &tam_virt_lido) != 2) {
            fprintf(stderr, "Erro ao ler dados do Processo %d\n", i);
            fclose(fc);
            liberar_memoria();
            return 0;
        }

        processos[i].pid = pid_lido;
        processos[i].tamanho_virtual = tam_virt_lido;
        processos[i].num_paginas = (tam_virt_lido + TAMANHO_DA_PAGINA - 1) / TAMANHO_DA_PAGINA;

        int num_pag = processos[i].num_paginas;
        processos[i].tabela_paginas = (EntradaPagina*) malloc(num_pag * sizeof(EntradaPagina));
        if (!processos[i].tabela_paginas) {
            perror("Erro ao alocar Tabela de Páginas");
            fclose(fc);
            liberar_memoria();
            return 0;
        }

        for (int p = 0; p < num_pag; p++) {
            processos[i].tabela_paginas[p].presente = 0;
            processos[i].tabela_paginas[p].frame_fisico = -1;
            processos[i].tabela_paginas[p].r_bit = 0;
            processos[i].tabela_paginas[p].tempo_carga = -1;
        }
    }

    fclose(fc);
    return 1;
}

void processar_acessos(char *algoritmo, char *arquivo_acessos) {
    FILE *fa = fopen(arquivo_acessos, "r");
    if (!fa) {
        perror("Erro ao abrir arquivo de acessos");
        return;
    }

    int pid_acesso;
    int end_virtual_acesso;

    while (fscanf(fa, "%d %d", &pid_acesso, &end_virtual_acesso) == 2) {
        if (pid_acesso >= 0 && pid_acesso < NUM_PROCESSOS) {
            processar_acesso(algoritmo, pid_acesso, end_virtual_acesso);
        } else {
            fprintf(stderr, "Aviso: PID %d inválido no arquivo de acessos. Ignorando.\n", pid_acesso);
        }
    }

    fclose(fa);
}

// ----------------------------------------------------------------------
// --- Função Principal (Main) ---
// ----------------------------------------------------------------------

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <algoritmo> <arquivo_config> <arquivo_acessos>\n", argv[0]);
        return 1;
    }

    char *algoritmo = argv[1];
    char *arq_config = argv[2];
    char *arq_acessos = argv[3];

    if (strcmp(algoritmo, "fifo") != 0 && strcmp(algoritmo, "clock") != 0) {
        fprintf(stderr, "Algoritmo inválido. Use 'fifo' ou 'clock'.\n");
        return 1;
    }

    if (!ler_configuracao(arq_config)) {
        fprintf(stderr, "Falha na leitura ou inicialização da configuração. Encerrando.\n");
        liberar_memoria();
        return 1;
    }

    processar_acessos(algoritmo, arq_acessos);

    
    fprintf(stdout, "--- Simulação Finalizada (Algoritmo: %s)\n", algoritmo);
    fprintf(stdout, "Total de Acessos: %d\n", total_acessos);
    fprintf(stdout, "Total de Page Faults: %d\n", total_page_faults);

    fflush(stdout);
    liberar_memoria();

    return 0;
}
