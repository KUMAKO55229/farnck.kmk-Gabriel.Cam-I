#ifndef __COZINHA_H__
#define __COZINHA_H__

#include "pedido.h"
#include "tarefas.h"

extern void cozinha_init(int cozinheiros, int bocas, int frigideiras, int garcons, int tam_balcao);
extern void *administrar_cozinheiros();
extern void funcao_garcon(prato_t *prato);
extern void *tratar_agua(void *agua);
extern void *tratar_bacon(void *bacon);
extern void *tratar_molho(void *molho);


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
