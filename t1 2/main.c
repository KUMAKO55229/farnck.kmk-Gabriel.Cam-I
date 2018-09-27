#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include "pedido.h"
#include "cozinha.h"
#include "tarefas.h"


#include <unistd.h>

#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
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

void processar_pedido(pedido_t *p){

    prato_t *prato = create_prato(*p);
    sem_wait(&sem_cozinheiros);

    if(p->prato == PEDIDO_SPAGHETTI) {


       /*  Spaghetti:
        Esquentar o molho [5min]
        Ferver água [3min]
        Cozinhar o Spaghetti (na água fervente) [5min]
        Dourar o bacon em uma frigideira [2min]
        Empratar o pedido [3min] [DE]*/

        // receita correta?
        pthread_t molho_thread, agua_thread, dourar_thread;

        spaghetti_t spaghetti = create_spaghetti();
        molho_t *molho = create_molho();
        bacon_t *bacon = create_bacon();
        agua_t *agua = create_agua();

        sem_wait(&bocas);
        pthread_create(&molho_thread,NULL,&esquentar_molho,(void *)molho);


        sem_wait(&bocas);
        sem_wait(&frigideiras);
        pthread_create(&dourar_thread,NULL,&dourar_bacon,(void *)bacon);

        sem_wait(&bocas);
        pthread_create(&agua_thread,NULL,&ferver_agua,(void *)agua);

        pthread_join(&agua_thread,NULL);
        cozinhar_spaghetti(spaghetti, agua);

        pthread_join(&molho_thread,NULL);
        pthread_join(&dourar_bacon,NULL);


        sem_wait(&BalcaoVazio);
        empratar_spaghetti( spaghetti,  molho,
                             bacon,  prato);
        sem_post(&BalcaoCheio);

        sem_post(&bocas);
        sem_post(&frigideiras);
        sem_post(&bocas);
        sem_post(&bocas);


        destroy_molho(molho);
        destroy_spaghetti(spaghetti);
        destroy_bacon(bacon);
        destroy_agua(agua);

    }else if (p->prato == PEDIDO_CARNE){
         /*
        Carne:
        Cortar a carne [5min] [DE]
        Temperar a carne [3min] [DE]
        Grelhar a carne em uma frigideira [3min] [DE]
        Empratar o pedido [1min] [DE]
        */
        carne_t carne = create_carne();

        cortar_carne(carne);

        temperar_carne(carne);

        sem_wait(&bocas);
        sem_wait(&frigideira);
        grelhar_carne(carne);

        sem_wait(&BalcaoVazio);
        empratar_carne(carne, prato);
        sem_post(&frigideira);
        sem_post(&bocas);
        sem_post(&BalcaoCheio);


        destroy_carne(carne);
    }else if (p->prato == PEDIDO_SOPA){
        /* Sopa:
         Cortar legumes [10min] [DE]
         Ferver a água [3min]
         Fazer o caldo (com a água fervente, precisa de boca de fogão) [2min]
         Cozinhar os legumes no caldo [8min]
         Empratar o pedido [1min] [DE] */
         legumes_t legumes = create_legumes();
         agua_t agua = create_agua();

         cortar_legumes(legumes);

         sem_wait(&bocas);
         ferver_agua(agua);
         caldo_t caldo = preparar_caldo(agua);

         cozinhar_legumes(legumes, caldo);

         sem_wait(&BalcaoVazio);
         empratar_sopa(legumes,caldo, prato);
         sem_post(&bocas);
         sem_post(&BalcaoVazio);

         destroy_caldo(caldo);

         destroy_legumes( legumes);
     }
     destroy_pedido(p);

     sem_post(sem_cozinheiros);

     notificar_prato_no_balcao(*prato);

     funcao_garcon(prato);

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

    sem_init(&bocas,0,bocas);

    sem_init(&frigideiras,0,frigideiras);

    sem_init(&BalcaoCheio,0,0);
    sem_init(&BalcaoVazio,0,tam_balcao);

    sem_init(&sem_cozinheiros,0,cozinheiros);
    sem_init(&sem_garcons,0,garcons);

    pthread_t *threads_cozinheiros = malloc(cozinheiros * sizeof(cozinheiros));
    pthread_t *threads_garcons = malloc(garcons * sizeof(garcons));

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

    char* buf = (char*)malloc(4096);
    int next_id = 1;
    int ret = 0;
    while((ret = scanf("%4095s", buf)) > 0) {
        pedido_t p = {next_id++, pedido_prato_from_name(buf)};
        if (!p.prato)
            fprintf(stderr, "Pedido inválido descartado: \"%s\"\n", buf);
        else
            // criando as threads cozinheiros
            pthread_create(&threads_cozinheiros[next_id++],NULL,&processar_pedido,&p);
            // criando as threads garcons
            //pthread_create(&threads_garcons[next_id++],NULL,&funcao_garcon,NULL);


            //processar_pedido(p);
    }
    if (ret != EOF) {
        perror("Erro lendo pedidos de stdin:");
    }



    free(buf);


    cozinha_destroy();


    return 0;
}
