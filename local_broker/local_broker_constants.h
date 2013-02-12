/*
 * local_broker_constants.h
 *
 *  Created on: 06/02/2013
 *      Author: gonzalo
 */

#ifndef LOCAL_BROKER_CONSTANTS_H_
#define LOCAL_BROKER_CONSTANTS_H_

#include "mensajes_de_red.h"

#define PATH_TABLA_CLIENTES_LOCALES "/tabla_clientes.lck"
#define PATH_MUTEX_CLIENTES_LOCALES "/mutex_tabla_clientes.lck"
#define PATH_TABLA_BROKERS_LOCALES "/tabla_brokers_locales.lck"
#define PATH_COLAS_CLIENTES "/colas_client_handler.lck"
#define PATH_COLA_TOKEN_MANAGER "/cola_token_manager.lck"
#define PREFIJO_RECURSO "/recurso_"

//#define MAX_NAME_SIZE 30
#define MAX_CLIENTS 3
#define TAMANIO_TABLA_CLIENTES (sizeof(int)+sizeof(size_t) + sizeof(char) * MAX_NAME_SIZE * MAX_CLIENTS)

#endif /* LOCAL_BROKER_CONSTANTS_H_ */
