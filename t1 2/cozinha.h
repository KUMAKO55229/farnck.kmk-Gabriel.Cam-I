#ifndef __COZINHA_H__
#define __COZINHA_H__

#include "pedido.h"


#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>

extern void cozinha_init(int cozinheiros, int bocas, int frigideiras, int garcons, int tam_balcao);
extern void cozinha_destroy();
extern void processar_pedido(pedido_t *p);
extern void funcao_garcon(pedido_t *p);



extern  sem_t  bocas;
extern  sem_t  frigideiras;
extern  sem_t  BalcaoCheio;
extern  sem_t  BalcaoVazio;
extern  sem_t  sem_cozinheiros;
extern  sem_t  sem_garcons;


/*
typedef struct{
	int n_cozinheiros;
	int n_bocas;
	int n_frigideiras;
	int n_garcons ;
	int tamanho_balcao;


	sem_t bocas;

	sem_t frigideiras;

	sem_t tamanho_balcao;

	pthread_mutex_t cozinheiro; // a ser utilizado em tarefas com DE
	pthread_mutex_t garcon;// para controlar a entrega de pedido


} cozinha_t ;


*/

#endif /*__COZINHA_H__*/
