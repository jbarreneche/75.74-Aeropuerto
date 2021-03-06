/*
 * GroupHandler.h
 *
 *  Created on: 16/02/2013
 *      Author: gonzalo
 */

#ifndef GROUPSENDER_H_
#define GROUPSENDER_H_

#include "mensajes_de_red.h"
#include "messagequeue.h"
#include "grupo.h"
#include "group_multiplexer.h"
#include <string>

class GroupSender {
private:

	GroupMultiplexer * grupo_remoto;
	Grupo grupo;
	std::string broker_local;
	mensajes::mensajes_local_broker_group_t mensaje;

	int cantidad_de_bloques_por_token;
	size_t tamanio_memoria;

	void loop_token();

	void send_token(int tipo);

public:
	GroupSender(const std::string & directorio_de_trabajo, const std::string & nombre_grupo,
		const std::string & nombre_broker_remoto);
	virtual ~GroupSender();

	void run();
};

#endif /* GROUPHANDLER_H_ */
