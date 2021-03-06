/*
 * MemoriaDistribuida.cpp
 *
 *  Created on: 12/02/2013
 *      Author: gonzalo
 */

#include "memoria_distribuida.h"
#include "globalconstants.h"
#include <cstdio>
#include "dir.h"
#include "process.h"
#include <csignal>
#include "daemon.h"
#include <iostream>

static char directorio [FILENAME_MAX];
static char nombre_app [MAX_NOMBRE_RECURSO];
static char brokers_file [FILENAME_MAX];
static char grupo [MAX_NOMBRE_RECURSO];
static char id_ipc [4];
static char tamanio_mem [10];
static char * args_local_broker_comm [] = {
	(char *)"local_broker_comm", directorio, nombre_app, brokers_file, grupo, id_ipc, tamanio_mem,NULL};

MemoriaDistribuida::MemoriaDistribuida(const std::string & directorio_de_trabajo, const std::string & nombre_app,
	const std::string & nombre_grupo, char id, size_t tamanio, bool create)
	: nombre_recurso(nombre_grupo), tamanio(tamanio)

{
	std::string a(nombre_app);
	a.append("a");
	// Creo el archivo lck
	create_if_not_exists(
		std::string(directorio_de_trabajo).append(PREFIJO_RECURSO).append(nombre_grupo).append(POSTFIJO_LCK).c_str());
	if (create) {
		memoria =
			new SharedMemory(
				std::string(directorio_de_trabajo).append(PREFIJO_RECURSO).append(nombre_grupo).append(POSTFIJO_LCK).c_str(),
				char(2 * id), tamanio, 0664, true, false);
		mutex =
			new SemaphoreSet(std::vector<short unsigned int>(2, 0),
				std::string(directorio_de_trabajo).append(PREFIJO_RECURSO).append(nombre_grupo).append(POSTFIJO_LCK).c_str(),
				char(2 * id + 1), 0664);
		control = ControlTokens::get_instance(directorio_de_trabajo, true);
	} else {
		memoria =
			new SharedMemory(
				std::string(directorio_de_trabajo).append(PREFIJO_RECURSO).append(nombre_grupo).append(POSTFIJO_LCK).c_str(),
				char(2 * id), 0, false, false);
		mutex =
			new SemaphoreSet(
				std::string(directorio_de_trabajo).append(PREFIJO_RECURSO).append(nombre_grupo).append(POSTFIJO_LCK).c_str(),
				char(2 * id + 1), 0);
		control = ControlTokens::get_instance(directorio_de_trabajo);
	}
	//strcat(path, nombre_app.c_str());
	lanzar_comunicacion(directorio_de_trabajo, nombre_app,
		std::string(directorio_de_trabajo).append(PATH_LOCAL_BROKER_LIST_FILE).c_str(), nombre_grupo, id, tamanio);

}

MemoriaDistribuida::MemoriaDistribuida(const std::string & directorio_de_trabajo, const std::string & nombre_grupo,
	char id)
	: nombre_recurso(nombre_grupo), tamanio(0)

{
	create_if_not_exists(
		std::string(directorio_de_trabajo).append(PREFIJO_RECURSO).append(nombre_grupo).append(POSTFIJO_LCK).c_str());

	memoria = new SharedMemory(
		std::string(directorio_de_trabajo).append(PREFIJO_RECURSO).append(nombre_grupo).append(POSTFIJO_LCK).c_str(),
		char(2 * id), 0, false, false);
	mutex = new SemaphoreSet(
		std::string(directorio_de_trabajo).append(PREFIJO_RECURSO).append(nombre_grupo).append(POSTFIJO_LCK).c_str(),
		char(2 * id + 1), 0);

	control = ControlTokens::get_instance(directorio_de_trabajo);
	p = NULL;
}

MemoriaDistribuida::~MemoriaDistribuida() {

	try {
		if (p) {
			//std::cout << "envio señal SIGUSR1" << std::endl;
			p->send_signal(SIGUSR1, false);
			//std::cout << "esperando hijo" << std::endl;
			p->wait();
			//std::cout << "hijo termino" << std::endl;
			delete p;
		}
	} catch (OSError &) {

	}
	if (memoria)
		delete memoria;
	if (mutex)
		delete mutex;
	ControlTokens::destroy_instance();

}

void MemoriaDistribuida::lock() {
	control->cargar_esperando_token(nombre_recurso.c_str());
	mutex->wait_on(0);
	control->limpiar_esperando_token();
}

void MemoriaDistribuida::unlock() {
	control->limpiar_esperando_token();
	mutex->signalize(1);
}

void MemoriaDistribuida::entregar_token() {
	mutex->signalize(0);
}

void MemoriaDistribuida::esperar_token() {
	mutex->wait_on(1);
}

size_t MemoriaDistribuida::get_mem_size() {
	return this->tamanio;
}

void * MemoriaDistribuida::memory_pointer() {
	return this->memoria->memory_pointer();
}

void MemoriaDistribuida::lanzar_comunicacion(const std::string & directorio_de_trabajo,
	const std::string & nombre_aplicacion, const std::string & local_brokers_file, const std::string & nombre_grupo,
	char id, size_t tamanio_memoria)
{
	char current_working_dir [FILENAME_MAX];
	char launch_dir [FILENAME_MAX];

	//ignore_signals();

	snprintf(directorio, FILENAME_MAX, "%s", directorio_de_trabajo.c_str());
	snprintf(nombre_app, MAX_NOMBRE_RECURSO, "%s", nombre_aplicacion.c_str());
	snprintf(brokers_file, FILENAME_MAX, "%s", local_brokers_file.c_str());
	snprintf(grupo, MAX_NOMBRE_RECURSO, "%s", nombre_grupo.c_str());
	snprintf(id_ipc, 4, "%d", id);

#ifdef __x86_64__
	snprintf(tamanio_mem, sizeof(tamanio_mem), "%lu", tamanio_memoria);
#else
	snprintf(tamanio_mem, sizeof(tamanio_mem), "%u", tamanio_memoria);
#endif

	if (!getcwd(current_working_dir, sizeof(current_working_dir)))
		throw GenericError("Unable to get current working dir");
	current_working_dir [sizeof(current_working_dir) - 1] = '\0';

	locate_dir(launch_dir, current_working_dir, (char *)"clientes");

	relativize_dir(directorio, directorio_de_trabajo.c_str(), launch_dir, current_working_dir);
	relativize_dir(brokers_file, local_brokers_file.c_str(), launch_dir, current_working_dir);

	if (chdir(launch_dir) != 0) {
		throw GenericError("Cannot change working dir to %s", launch_dir);
	}

	p = new Process("local_broker_comm", args_local_broker_comm);

	if (chdir(current_working_dir) != 0) {
		throw GenericError("Cannot change working dir to %s", current_working_dir);
	}

}
