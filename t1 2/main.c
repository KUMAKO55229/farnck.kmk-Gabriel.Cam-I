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

pthread_t threads_cozinheiros;

pedido_t pedido_atual;

sem_t  sem_bocas;
sem_t  sem_frigideiras;
sem_t  BalcaoCheio;
sem_t  BalcaoVazio;
sem_t  sem_cozinheiros;
sem_t  sem_garcons;
sem_t  pedido_disponivel;
sem_t  espacoVazio;

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

void *administrar_cozinheiros(){
    sem_wait(&pedido_disponivel);
    prato_t *prato = create_prato(pedido_atual);

    if(pedido_atual.prato == PEDIDO_SPAGHETTI) {
        printf("Fazendo spaghetti...\n");
        sem_post(&espacoVazio);
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
        printf("Cozinhando spaghetti...\n");
        cozinhar_spaghetti(spaghetti, agua);

        pthread_join(molho_thread,NULL);
        pthread_join(dourar_thread,NULL);


        sem_wait(&BalcaoVazio);
        printf("Empratando spaghetti...\n");
        empratar_spaghetti( spaghetti,  molho,
                             bacon, (prato_t *)prato);
        sem_post(&BalcaoCheio);
        funcao_garcon((prato_t *)prato);
        printf("Spaghetti servido!\n");

        sem_post(&sem_bocas);
        sem_post(&sem_frigideiras);
        sem_post(&sem_bocas);
        sem_post(&sem_bocas);
    }

    else if (pedido_atual.prato == PEDIDO_CARNE){
        /*
       Carne:
       Cortar a carne [5min] [DE]
       Temperar a carne [3min] [DE]
       Grelhar a carne em uma frigideira [3min] [DE]
       Empratar o pedido [1min] [DE]
       */
       sem_post(&espacoVazio);
       sem_wait(&sem_cozinheiros);
       carne_t *carne = create_carne();
       printf("Cortando carne...\n");
       cortar_carne(carne);

       printf("Temperando carne...\n");
       temperar_carne(carne);

       sem_wait(&sem_bocas);
       sem_wait(&sem_frigideiras);
       printf("Grelhando carne...\n");
       grelhar_carne(carne);

       sem_wait(&BalcaoVazio);
       printf("Empratando carne...\n");
       empratar_carne(carne, (prato_t *)prato);

       sem_post(&sem_frigideiras);
       sem_post(&sem_bocas);
       sem_post(&BalcaoCheio);

       funcao_garcon((prato_t *)prato);
       printf("Carne servida!\n");
    }

    else if (pedido_atual.prato == PEDIDO_SOPA){
        /* Sopa:
         Cortar legumes [10min] [DE]
         Ferver a água [3min]
         Fazer o caldo (com a água fervente, precisa de boca de fogão) [2min]
         Cozinhar os legumes no caldo [8min]
         Empratar o pedido [1min] [DE] */
         sem_post(&espacoVazio);
         sem_wait(&sem_cozinheiros);
         legumes_t *legumes = create_legumes();
         agua_t *agua = create_agua();

         cortar_legumes(legumes);
         printf("Cortando legumes...\n");

         sem_wait(&sem_bocas);
         printf("Fervendo agua...\n");
         ferver_agua(agua);
         printf("Preparando caldo...\n");
         caldo_t *caldo = preparar_caldo(agua);

         cozinhar_legumes(legumes, caldo);
         printf("Cozinhando legumes...\n");

         sem_wait(&BalcaoVazio);
         printf("Empratando sopa...\n");
         empratar_sopa(legumes,caldo, (prato_t *)prato);
         sem_post(&sem_bocas);
         sem_post(&BalcaoCheio);
         funcao_garcon((prato_t *)prato);
         printf("Sopa servida!\n");

    }

    sem_post(&sem_cozinheiros);

    notificar_prato_no_balcao(prato);
    return NULL;
}


void funcao_garcon(prato_t *prato){

    sem_wait(&sem_garcons);
    sem_wait(&BalcaoCheio);
    entregar_pedido(prato);
    sem_post(&BalcaoVazio);
    sem_post(&sem_garcons);
}



// inicialização da cozinha
void cozinha_init(int cozinheiros, int bocas, int frigideiras, int garcons, int tam_balcao){

    sem_init(&sem_bocas,0,bocas);

    sem_init(&sem_frigideiras,0,frigideiras);

    sem_init(&BalcaoCheio,0,0);
    sem_init(&BalcaoVazio,0,tam_balcao);

    sem_init(&sem_cozinheiros,0,cozinheiros);
    sem_init(&sem_garcons,0,garcons);

    sem_init(&pedido_disponivel,0,0);
    sem_init(&espacoVazio,0,cozinheiros);

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

    for(int i=0;i<cozinheiros;i++)
        pthread_create(&threads_cozinheiros,NULL,administrar_cozinheiros,NULL);

    char* buf = (char*)malloc(4096);
    int next_id = 1;
    int ret = 0;
    ret = scanf("%4095s", buf);
    printf("%d\n",ret);
    while((ret = scanf("%4095s", buf)) > 0) {
        printf("Esquentando molho...\n");
        pedido_t p = {next_id++, pedido_prato_from_name(buf)};
        if (!p.prato)
            fprintf(stderr, "Pedido inválido descartado: \"%s\"\n", buf);
        else{
            sem_wait(&espacoVazio);
            pedido_atual = p;
            sem_post(&pedido_disponivel);
        }
    }
    if (ret != EOF) {
        perror("Erro lendo pedidos de stdin:");
    }

    for(int i=0;i<cozinheiros;i++)
        pthread_join(threads_cozinheiros,NULL);

    free(buf);

    sem_destroy(&BalcaoCheio);
    sem_destroy(&BalcaoVazio);
    sem_destroy(&sem_garcons);
    sem_destroy(&sem_cozinheiros);
    sem_destroy(&sem_bocas);
    sem_destroy(&sem_frigideiras);
    sem_destroy(&pedido_disponivel);
    sem_destroy(&espacoVazio);

    return 0;
}
