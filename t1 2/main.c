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

//funcc
void processar_pedido(pedido_t p){

      create_prato(pedido_t pedido);

     if (p.prato == "SPAGHETTI"){
       /*  Spaghetti: 
        Esquentar o molho [5min]
        Ferver água [3min]
        Cozinhar o Spaghetti (na água fervente) [5min]
        Dourar o bacon em uma frigideira [2min]
        Empratar o pedido [3min] [DE]*/

         create_spaghetti();

         create_molho();

         esquentar_molho(molho_t* molho);

         destroy_molho(molho_t* molho);
         
         ferver_agua(agua_t* agua);

         cozinhar_spaghetti(spaghetti_t* spaghetti, agua_t* agua);

         destroy_spaghetti(spaghetti_t* spaghetti);

         
         empratar_spaghetti(spaghetti_t* spaghetti, molho_t* molho, 
                               bacon_t* bacon, prato_t* prato);




     }else if (p.prato == "CARNE"){


         /*  
        Carne:
        Cortar a carne [5min] [DE]
        Temperar a carne [3min] [DE]
        Grelhar a carne em uma frigideira [3min] [DE]
        Empratar o pedido [1min] [DE] 
        */
         



         create_carne();

         cortar_carne(carne_t* carne);

         temperar_carne(carne_t* carne);

         grelhar_carne(carne_t* carne);

         destroy_carne(carne_t* carne);

         empratar_carne(carne_t* carne, prato_t* prato);
    

     }else if (p.prato == "SOPA"){


        /* Sopa:
         Cortar legumes [10min] [DE]
         Ferver a água [3min]
         Fazer o caldo (com a água fervente, precisa de boca de fogão) [2min]
         Cozinhar os legumes no caldo [8min]
         Empratar o pedido [1min] [DE] */



         create_legumes();

         cortar_legumes(legumes_t* legumes);

         ferver_agua(agua_t* agua);


        preparar_caldo(agua_t* agua_ferv);  //1min

        destroy_caldo(caldo_t* caldo);

         cozinhar_legumes(legumes_t* legumes, caldo_t* caldo);

         destroy_legumes(legumes_t* legumes);

         empratar_sopa(legumes_t* legumes, caldo_t* caldo, prato_t* prato);

     }
     
 


   destroy_prato(prato_t* p);
}

void cozinha_init(int cozinheiros, int bocas, int frigideiras, int garcons, int tam_balcao){


for (size_t i= 0 ; i< bocas ; i++){
    sem_init(&bocas,0,bocas);
}

for(size_t i = 0; i< frigideiras ; i++){
    sem_init(&frigideiras,0,frigideiras);
}

sem_init(&BalcaoCheio,0,0);
sem_init(&BalcaoVazio,0,tam_balcao);

pthread_mutex_init(&cozinheiro, NULL);
pthread_mutex_init(&garcon,NULL);

pthread_t threads_cozinheiros = malloc(sizeof(cozinheiros));
pthread_t threads_garcons = malloc(sizeof(garcons));

}


int main(int argc, char** argv) {
    int bocas_total = 0, bocas = 4, frigideiras = 2, fogoes = 2, 
        cozinheiros = 6, garcons = 1, balcao = 5, c = 0;
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

    bocas_total = bocas*fogoes;
    check_missing(cozinheiros, "cozinheiros");
    check_missing(bocas, "bocas");
    check_missing(fogoes, "fogoes");
    check_missing(frigideiras, "frigideiras");
    check_missing(garcons, "garcons");
    check_missing(balcao, "balcao");
    
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
            // criando as threads
            pthread_create(&threads_cozinheiros[next_id++],NULL,&processar_pedido,&p);

            //processar_pedido(p);
    }
    if (ret != EOF) {
        perror("Erro lendo pedidos de stdin:");
    }
     
     

    free(buf);

   
    cozinha_destroy();


    return 0;
}
