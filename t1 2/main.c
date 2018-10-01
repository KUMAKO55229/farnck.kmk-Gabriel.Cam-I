#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>

#include "pedido.h"
#include "cozinha.h"
#include "tarefas.h"

pedido_t *pedido_atual;
prato_t **buffer;
int indice_garcons, indice_pratos, tamanho_buffer,lendoGarcon=0,lendoCozinheiro=0;

pthread_mutex_t mutex_cozinheiros, mutex_garcons, mutex_pedidos;

sem_t  sem_bocas;
sem_t  sem_frigideiras;
sem_t  BalcaoCheio;
sem_t  BalcaoVazio;
sem_t  sem_cozinheiros;
sem_t  sem_garcons;
sem_t  pedido_disponivel;
sem_t  espacoVazio;
sem_t  cozinheiro_disponivel;

static struct option cmd_opts[] = {
    {"cozinheiros", required_argument, 0, 'c'},
    {"bocas",       required_argument, 0, 'b'},
    {"fogoes",      required_argument, 0, 'f'},
    {"frigideiras", required_argument, 0, 'r'},
    {"garcons",     required_argument, 0, 'g'},
    {"balcao",      required_argument, 0, 'a'},
    {"help",        no_argument,       0, 'h'},
    {0, 0, 0, 0}
};




int parse_gt_zero(const char* buf, const char* name, int* res) {
    errno = 0;
    *res = strtol(buf, NULL, 10);
    if (errno) {
        fprintf(stderr, "Impossível parsear argumento da opção %s, \"%s\" como "
                "um inteiro: %s\n", name, buf, strerror(errno));
        return 0;
    }
    if (*res <= 0) {
        fprintf(stderr, "Esperava um valor maior que zero para %s, leu: %d\n",
                name, *res);
        return 0;
    }
    return 1;
}

void check_missing(int value, const char* name) {
    if (value > 0) return;
    fprintf(stderr, "%s faltando!\n", name);
    abort();
}

//As três funções a seguir são responsáveis pela concorrência das atividades
void *tratar_molho(void *molho){
    printf("Esquentando molho...\n");
    esquentar_molho((molho_t *)molho);
    return NULL;
}

void *tratar_bacon(void *bacon){
    printf("Dourando bacon...\n");
    dourar_bacon((bacon_t *)bacon);
    return NULL;
}

void *tratar_agua(void *agua){
    printf("Fervendo agua...\n");
    ferver_agua((agua_t *)agua);
    return NULL;
}

//Funcao principal das threads_cozinheiros. Cada if é um tipo de receita (com exceção
//do primeiro, que serve para "matar" a thread)
void *administrar_cozinheiros(){
    while(1){
        sem_post(&cozinheiro_disponivel);
        sem_wait(&pedido_disponivel);

        if(pedido_atual == NULL){
            pthread_mutex_unlock(&mutex_pedidos);
            sem_wait(&sem_cozinheiros);
            sem_post(&sem_cozinheiros);
            return NULL;

        }
        else if(pedido_atual->prato == PEDIDO_SPAGHETTI) {
            prato_t *prato = create_prato(*pedido_atual);
            pthread_mutex_unlock(&mutex_pedidos);
            printf("Cozinheiro preparando spaghetti\n");
            sem_wait(&sem_cozinheiros);
            pthread_t molho_thread, agua_thread, dourar_thread;

            spaghetti_t *spaghetti = create_spaghetti();
            molho_t *molho = create_molho();
            bacon_t *bacon = create_bacon();
            agua_t *agua = create_agua();

            sem_wait(&sem_bocas);
            pthread_create(&molho_thread,NULL,tratar_molho,(void *)molho);


            sem_wait(&sem_bocas);
            sem_wait(&sem_frigideiras);
            pthread_create(&dourar_thread,NULL,tratar_bacon,(void *)bacon);

            sem_wait(&sem_bocas);
            pthread_create(&agua_thread,NULL,tratar_agua,(void *)agua);

            pthread_join(agua_thread,NULL);
            cozinhar_spaghetti(spaghetti, agua);
            destroy_agua(agua);

            pthread_join(molho_thread,NULL);
            pthread_join(dourar_thread,NULL);


            sem_wait(&BalcaoVazio);
            pthread_mutex_lock(&mutex_cozinheiros);
            indice_pratos = (indice_pratos + 1) % tamanho_buffer;
            empratar_spaghetti( spaghetti,  molho,
                                 bacon, prato);
            notificar_prato_no_balcao(prato);
            buffer[indice_pratos] = prato;
            pthread_mutex_unlock(&mutex_cozinheiros);
            sem_post(&BalcaoCheio);

            sem_post(&sem_bocas);
            sem_post(&sem_frigideiras);
            sem_post(&sem_bocas);
            sem_post(&sem_bocas);
        }

        else if (pedido_atual->prato == PEDIDO_CARNE){

           prato_t *prato = create_prato(*pedido_atual);
           pthread_mutex_unlock(&mutex_pedidos);
           printf("Cozinheiro preparando carne\n");
           sem_wait(&sem_cozinheiros);

           carne_t *carne = create_carne();
           cortar_carne(carne);

           temperar_carne(carne);

           sem_wait(&sem_bocas);
           sem_wait(&sem_frigideiras);
           grelhar_carne(carne);

           sem_wait(&BalcaoVazio);
           pthread_mutex_lock(&mutex_cozinheiros);
           indice_pratos = (indice_pratos + 1) % tamanho_buffer;
           empratar_carne(carne, (prato_t *)prato);
           printf("Empratou carne!\n");
           notificar_prato_no_balcao(prato);
           buffer[indice_pratos] = prato;
           pthread_mutex_unlock(&mutex_cozinheiros);

           sem_post(&sem_frigideiras);
           sem_post(&sem_bocas);
           sem_post(&BalcaoCheio);
        }

        else if (pedido_atual->prato == PEDIDO_SOPA){
             prato_t *prato = create_prato(*pedido_atual);
             pthread_mutex_unlock(&mutex_pedidos);
             printf("Cozinheiro preparando sopa\n");
             sem_wait(&sem_cozinheiros);
             legumes_t *legumes = create_legumes();
             agua_t *agua = create_agua();

             cortar_legumes(legumes);

             sem_wait(&sem_bocas);
             ferver_agua(agua);
             caldo_t *caldo = preparar_caldo(agua);
             cozinhar_legumes(legumes, caldo);

             sem_wait(&BalcaoVazio);
             pthread_mutex_lock(&mutex_cozinheiros);
             indice_pratos = (indice_pratos + 1) % tamanho_buffer;
             empratar_sopa(legumes,caldo, (prato_t *)prato);
             notificar_prato_no_balcao(prato);
             buffer[indice_pratos] = prato;
             pthread_mutex_unlock(&mutex_cozinheiros);

             sem_post(&BalcaoCheio);
             sem_post(&sem_bocas);
        }
        sem_post(&sem_cozinheiros);
    }
}

//Funcao principal das threads_garcons
void *funcao_garcon(){
    while(1){
        sem_wait(&sem_garcons);
        sem_wait(&BalcaoCheio);
        pthread_mutex_lock(&mutex_garcons);
        indice_garcons = (indice_garcons + 1) % tamanho_buffer;
        prato_t *prato = malloc(sizeof(prato_t));
        prato = buffer[indice_garcons];

        if(prato == NULL){
            pthread_mutex_unlock(&mutex_garcons);
            sem_post(&BalcaoVazio);
            sem_post(&sem_garcons);
            return NULL;
        }


        entregar_pedido(prato);
        pthread_mutex_unlock(&mutex_garcons);
        sem_post(&BalcaoVazio);
        sem_post(&sem_garcons);
    }
}



// inicialização da cozinha
void cozinha_init(int cozinheiros, int bocas, int frigideiras, int garcons, int tam_balcao){

    //Semaforo para as bocas dos fogao
    sem_init(&sem_bocas,0,bocas);

    //Semaforo para as frigideiras
    sem_init(&sem_frigideiras,0,frigideiras);

    //Semaforos que controlam a troca de pratos no balcao, permitindo aos cozinheiros
    //empratarem os pratos e os garcons a servi-los
    sem_init(&BalcaoCheio,0,0);
    sem_init(&BalcaoVazio,0,tam_balcao);

    //Semaforos que controlam o numero de cozinheiros e garcons que podem existir
    //ao mesmo tempo
    sem_init(&sem_cozinheiros,0,cozinheiros);
    sem_init(&sem_garcons,0,garcons);

    //Semaforos que controlam a troca de pedidos entre a funcao main e as threads_garcons,
    //garantindo que os pedidos sejam lidos corretamente e apenas uma vez
    sem_init(&pedido_disponivel,0,0);
    sem_init(&cozinheiro_disponivel,0,cozinheiros);

    //Mutexes utilizados durante a execucao do programa
    pthread_mutex_init(&mutex_cozinheiros,NULL);
    pthread_mutex_init(&mutex_pedidos,NULL);
    pthread_mutex_init(&mutex_garcons,NULL);

    //Inicializacao das variaveis responsaveis pelo armazenamento (em um buffer)
    //dos pratos produzidos para a entrega dos garcons
    buffer = malloc(sizeof(prato_t) * tam_balcao);
    pedido_atual = malloc(sizeof(pedido_t));
    indice_pratos = 0;
    indice_garcons = 0;
    tamanho_buffer = tam_balcao;

}


int main(int argc, char** argv) {
    int bocas_total = 0, bocas = 4, frigideiras = 2, fogoes = 2,
        cozinheiros = 6, garcons = 1, balcao = 5, c = 0;

    /*Loop para fazer o parsing dos argumentos corretamente*/
    while (c >= 0) {
        int long_idx;
        c = getopt_long(argc, argv, "cbfrga", cmd_opts, &long_idx);
        if (c == 0) c = cmd_opts[long_idx].val;

        switch (c) {
        case 'c':
            if (!parse_gt_zero(optarg, argv[optind-1], &cozinheiros)) abort();
            break;
        case 'b':
            if (!parse_gt_zero(optarg, argv[optind-1], &bocas      )) abort();
            break;
        case 'f':
            if (!parse_gt_zero(optarg, argv[optind-1], &fogoes     )) abort();
            break;
        case 'r':
            if (!parse_gt_zero(optarg, argv[optind-1], &frigideiras)) abort();
            break;
        case 'g':
            if (!parse_gt_zero(optarg, argv[optind-1], &garcons    )) abort();
            break;
        case 'a':
            if (!parse_gt_zero(optarg, argv[optind-1], &balcao     )) abort();
            break;
        case -1:
            break;
        default:
            abort();
        }
    }

    //Checa se cada um dos valores é maior do que zero; Se for menor, aborta a operação
    //e imprime o erro no arquivo.
    bocas_total = bocas*fogoes;
    check_missing(cozinheiros, "cozinheiros");
    check_missing(bocas, "bocas");
    check_missing(fogoes, "fogoes");
    check_missing(frigideiras, "frigideiras");
    check_missing(garcons, "garcons");
    check_missing(balcao, "balcao");

    //Inicializa a cozinha com as variáveis que foram parseadas.
    cozinha_init(cozinheiros, bocas_total, frigideiras,
                 garcons, balcao);

    pthread_t threads_cozinheiros[cozinheiros], threads_garcons[garcons];

    for(int i=0;i<cozinheiros;i++){
        pthread_create(&threads_cozinheiros[i],NULL,administrar_cozinheiros,NULL);
    }
    for(int i=0;i<garcons;i++){
        pthread_create(&threads_garcons[i],NULL,funcao_garcon,NULL);
    }

    char* buf = (char*)malloc(4096);
    int next_id = 1;
    int ret = 0, aux=0;
    while((ret = scanf("%4095s", buf)) > 0) {
        pedido_t p = {next_id++, pedido_prato_from_name(buf)};
        if (!p.prato)
            fprintf(stderr, "Pedido inválido descartado: \"%s\"\n", buf);
        else{
            sem_wait(&cozinheiro_disponivel);
            pthread_mutex_lock(&mutex_pedidos);
            aux+=1;
            *pedido_atual = p;
            sem_post(&pedido_disponivel);
        }
    }
    //Matando threads cozinheiros...
    for(int i=0;i<cozinheiros;i++){
        sem_wait(&cozinheiro_disponivel);
        pthread_mutex_lock(&mutex_pedidos);
        pedido_atual = NULL;
        sem_post(&pedido_disponivel);
    }

    for(int i=0;i<cozinheiros;i++)
        pthread_join(threads_cozinheiros[i],NULL);

    //Matando threads garcons...
    for(int i=0;i<garcons;i++){
        sem_wait(&BalcaoVazio);
        indice_pratos = (indice_pratos + 1) % tamanho_buffer;
        buffer[indice_pratos] = NULL;
        sem_post(&BalcaoCheio);
    }

    for(int i=0;i<garcons;i++)
        pthread_join(threads_garcons[i],NULL);

    if (ret != EOF) {
        perror("Erro lendo pedidos de stdin:");
    }

    //"Destruindo" a cozinha e as demais estruturas
    free(buf);
    free(buffer);
    free(pedido_atual);
    pthread_mutex_destroy(&mutex_cozinheiros);
    pthread_mutex_destroy(&mutex_pedidos);
    pthread_mutex_destroy(&mutex_garcons);
    sem_destroy(&cozinheiro_disponivel);
    sem_destroy(&BalcaoCheio);
    sem_destroy(&BalcaoVazio);
    sem_destroy(&sem_garcons);
    sem_destroy(&sem_cozinheiros);
    sem_destroy(&sem_bocas);
    sem_destroy(&sem_frigideiras);
    sem_destroy(&pedido_disponivel);


    return 0;
}
