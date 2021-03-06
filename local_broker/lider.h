/*
 * Lider.h
 *
 *  Created on: 17/02/2013
 *      Author: gonzalo
 */

#ifndef LIDER_H_
#define LIDER_H_

#include "grupo.h"
#include "messagequeue.h"
#include "group_interface.h"
#include "mensajes_de_red.h"
#include "local_broker_constants.h"
#include "eventhandler.h"
#include "signalhandler.h"

class Lider : public EventHandler{
private:
	MessageQueue cola_token_manager;
	MessageQueue cola_lider;
	GroupInterface grupo_remoto;
	Grupo grupo;
	sig_atomic_t alarma;
	sig_atomic_t esperando_mensaje;
	sig_atomic_t error_en_conexion;
	std::string broker_local;
	int cant_nodos;

	mensajes::mensajes_local_broker_lider_t mensaje;
	mensajes::mensajes_local_broker_group_t mensaje_envio;

	void do_loop();
	void send_msg();
	void wait_msg();

public:
	Lider(const std::string directorio_de_trabajo, const std::string & nombre_grupo, char id_grupo,
		const std::string & nombre_broker);
	virtual ~Lider();

	virtual void handleSignal(int signum);
	void run();
};

#endif /* LIDER_H_ */
