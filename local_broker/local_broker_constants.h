/*
 * local_broker_constants.h
 *
 *  Created on: 06/02/2013
 *      Author: gonzalo
 */

#ifndef LOCAL_BROKER_CONSTANTS_H_
#define LOCAL_BROKER_CONSTANTS_H_

#include "mensajes_de_red.h"
#include "api_constants.h"

#define PATH_TABLA_CLIENTES_LOCALES "/tabla_clientes.lck"
#define PATH_MUTEX_CLIENTES_LOCALES "/mutex_tabla_clientes.lck"
#define PATH_TABLA_BROKERS_LOCALES "/tabla_brokers_locales.lck"
#define PATH_COLAS_CLIENTES "/colas_client_handler.lck"
#define PATH_COLA_TOKEN_MANAGER "/cola_token_manager.lck"
#define PATH_COLAS_BROKERS "/colas_broker.lck"
#define PATH_COLA_LEADER "/cola_leader.lck"
#define POSTFIJO_INIT ".init"
#define PATH_TABLA_GRUPOS "/tabla_grupos.lck"

//#define MAX_NAME_SIZE 30
#define MAX_CLIENTS 50
#define TAMANIO_TABLA_CLIENTES (sizeof(int)+sizeof(size_t) + sizeof(char) * MAX_NOMBRE_RECURSO * MAX_CLIENTS + 4 * sizeof(int))
#define MAX_GRUPOS 128
#define TAMANIO_TABLA_GRUPOS_CREADOS (sizeof(int) + (sizeof(char) * MAX_NOMBRE_RECURSO + sizeof(size_t *)) * MAX_GRUPOS)
#define TAMANIO_TABLA_COLAS_ASIGNADAS (sizeof(int)+sizeof(char) * MAX_NOMBRE_RECURSO * MAX_GRUPOS)

#define TIMEOUT_LIDER 10

#endif /* LOCAL_BROKER_CONSTANTS_H_ */
