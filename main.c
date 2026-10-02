/*
 *
 * Cria uma tarefa monitora que, a cada 5 segundos, imprime no monitor serial
 * todas as tarefas existentes no sistema: nome, estado, prioridade,
 * tamanho (pilha) e nucleo.
 *
 * Requisitos no menuconfig (sdkconfig):
 *   CONFIG_FREERTOS_USE_TRACE_FACILITY=y
 */

#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/idf_additions.h" 

#define PERIODO_RELATORIO_MS      5000
#define TAMANHO_PILHA_MONITORA    4096
#define PRIORIDADE_MONITORA       5
#define FOLGA_NA_LISTA_DE_TAREFAS 5   /* margem caso surjam tarefas durante a leitura */

/* Converte o estado numerico da tarefa em texto legivel. */
static const char *estado_para_texto(eTaskState estado_atual)
{
    switch (estado_atual) {
        case eRunning:   return "Executando";
        case eReady:     return "Pronta";
        case eBlocked:   return "Bloqueada";
        case eSuspended: return "Suspensa";
        case eDeleted:   return "Deletada";
        default:         return "Invalido";
    }
}

/* Ordena a lista pelo numero de criacao, para a saida ficar estavel. */
static int comparar_por_numero_da_tarefa(const void *primeira, const void *segunda)
{
    const TaskStatus_t *tarefa_a = (const TaskStatus_t *)primeira;
    const TaskStatus_t *tarefa_b = (const TaskStatus_t *)segunda;
    return (int)tarefa_a->xTaskNumber - (int)tarefa_b->xTaskNumber;
}

static void tarefa_monitora(void *parametros)
{
    (void)parametros;

    while (1) {
        UBaseType_t capacidade_da_lista =
            uxTaskGetNumberOfTasks() + FOLGA_NA_LISTA_DE_TAREFAS;

        TaskStatus_t *lista_de_tarefas = malloc(capacidade_da_lista * sizeof(TaskStatus_t));
        if (lista_de_tarefas == NULL) {
            printf("Falha ao alocar memoria para a lista de tarefas.\n");
            vTaskDelay(pdMS_TO_TICKS(PERIODO_RELATORIO_MS));
            continue;
        }

        UBaseType_t quantidade_de_tarefas =
            uxTaskGetSystemState(lista_de_tarefas, capacidade_da_lista, NULL);

        qsort(lista_de_tarefas, quantidade_de_tarefas, sizeof(TaskStatus_t),
              comparar_por_numero_da_tarefa);

        printf("\n=== Tarefas em execucao: %u ===\n", (unsigned)quantidade_de_tarefas);
        printf("%-16s %-12s %-10s %-22s %-8s\n",
               "Nome", "Estado", "Prioridade", "Pilha livre min.(B)", "Nucleo");

        for (UBaseType_t i = 0; i < quantidade_de_tarefas; i++) {
            const TaskStatus_t *tarefa = &lista_de_tarefas[i];

            /* Nucleo consultado pela handle: nao depende de opcao do menuconfig. */
            BaseType_t nucleo_da_tarefa = xTaskGetCoreID(tarefa->xHandle);

            char texto_do_nucleo[16];
            if (nucleo_da_tarefa == tskNO_AFFINITY) {
                snprintf(texto_do_nucleo, sizeof(texto_do_nucleo), "0 e 1");
            } else {
                snprintf(texto_do_nucleo, sizeof(texto_do_nucleo), "%d", (int)nucleo_da_tarefa);
            }

            printf("%-16s %-12s %-10u %-22u %-8s\n",
                   tarefa->pcTaskName,
                   estado_para_texto(tarefa->eCurrentState),
                   (unsigned)tarefa->uxCurrentPriority,
                   (unsigned)tarefa->usStackHighWaterMark,
                   texto_do_nucleo);
        }

        free(lista_de_tarefas);
        vTaskDelay(pdMS_TO_TICKS(PERIODO_RELATORIO_MS));
    }
}

void app_main(void)
{
    xTaskCreate(tarefa_monitora, "monitora", TAMANHO_PILHA_MONITORA,
                NULL, PRIORIDADE_MONITORA, NULL);
}
