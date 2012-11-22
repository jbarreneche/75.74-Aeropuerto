#include <iostream>
#include <stdlib.h>
#include <unistd.h>

#include <map>
#include <string>

#include "api_despachante.h"
#include "api_torre_de_control.h"

#include "api_carga.h"
#include "contenedor.h"
#include "equipaje.h"
#include "log.h"
#include "cintas.h"
#include "process.h"
#include "constants.h"

#include "database.h"
#include "stmt.h"
#include "tupleiter.h"

void agregar_equipaje(Equipaje & equipaje,
                      std::map<std::string, Contenedor> & contenedores_por_escala, 
                      ApiCarga &api_carga,
                      int id_robot ) {

	Log::info("RobotCarga(%d) se tomo el equipaje %s con escala destino '%s' peso=%d\n", id_robot,
             equipaje.toString().c_str(), equipaje.getRfid().get_escala().c_str(), equipaje.peso());

	if (equipaje.peso() <= MAX_PESO_CONTENEDOR) {
		std::string escala = equipaje.getRfid().get_escala();

		if (contenedores_por_escala.find(escala) == contenedores_por_escala.end()) {
			Log::info("RobotCarga(%d) no tengo contenedor, pido contenedor para escala '%s'\n",
                   id_robot, escala.c_str());
			contenedores_por_escala.insert(std::pair<std::string, Contenedor>(escala, Contenedor()));
		}

		if (!contenedores_por_escala[escala].hay_lugar(equipaje)) {
			Log::info("RobotCarga(%d) contenedor lleno, pido contenedor para escala '%s'\n",
                   id_robot, escala.c_str());
			api_carga.agregar_contenedor_cargado(contenedores_por_escala[escala]);
			contenedores_por_escala[escala] = Contenedor();
		}
		contenedores_por_escala[escala].agregar_equipaje(equipaje);

	} else {
		Log::crit("RobotCarga(%d) El equipaje %s es mas grande que el propio contenedor!!!\n",
                id_robot, equipaje.toString().c_str());
	}
}

int main(int argc, char** argv) try {
	Equipaje equipaje;
	bool checkin_cerro;
	char path[300];
	char path_cola[300];
	int id_robot;
	int numero_de_vuelo;
	int equipajes_por_cargar, equipajes_cargados;

	if (argc < 2) {
		Log::crit("Insuficientes parametros para robot de carga, se esperaba (id_robot)\n");
		exit(1);
	}

	id_robot = atoi(argv[1]);

	strcpy(path, PATH_KEYS);
	strcat(path, PATH_CINTA_CONTENEDOR);
	strcpy(path_cola, PATH_KEYS);
	strcat(path_cola, PATH_COLA_ROBOTS_ZONA_TRACTORES);

	std::map<std::string, Contenedor> contenedores_por_escala;

	ApiCarga api_carga(1, path, id_robot, path_cola);
   ApiDespachante api_despachante(id_robot, PATH_KEYS);
   ApiTorreDeControl api_torre( std::string(PATH_KEYS).append(PATH_TORRE_DE_CONTROL).c_str() );

	Log::info("Iniciando robot carga(%d)\n", id_robot);

	Log::info("RobotCarga(%d), lanzando proceso control_carga_contenedores\n", id_robot);
	char *args_control_carga[] = { (char*) "control_carga_contenedores", (char*) argv[1], NULL };
	Process control_carga_contenedores("control_carga_contenedores", args_control_carga);

	for (;;) {
		checkin_cerro = false;
		equipajes_cargados = 0;
      equipajes_por_cargar = 0;

		while ( (!checkin_cerro) || (equipajes_cargados<equipajes_por_cargar) ) {
			sleep(rand() % SLEEP_ROBOT_CARGA);
			Log::info("RobotCarga(%s) Intentando tomar un nuevo equipaje de cinta(%s)\n", argv[1],argv[2]);
			equipaje = api_carga.sacar_equipaje();

         //TODO: por ahora equipaje con rfid 0 es dummy(sale con este valor cuando se despierta la cinta por cierre de checkin)
         if(equipaje.getRfid().rfid != 0) {
            numero_de_vuelo = equipaje.getRfid().numero_de_vuelo_destino;//por ahora el n° de vuelo lo saca del equipaje

            agregar_equipaje(equipaje, contenedores_por_escala, api_carga,id_robot);
            equipajes_cargados++;

            if(!checkin_cerro) {
               Log::info("RobotCarga(%d) pongo equipaje %s en contenedor de escala '%s'.\n",
                         id_robot, equipaje.toString().c_str(), equipaje.getRfid().get_escala().c_str() );
            } else {
               Log::info("RobotCarga(%d) pongo equipaje %s en contenedor de escala '%s'.ya carge %d/%d equipajes\n",
                         id_robot, equipaje.toString().c_str(), equipaje.getRfid().get_escala().c_str(),
                         equipajes_cargados, equipajes_por_cargar);
            }
         }

         if( (!checkin_cerro) && (checkin_cerro=api_carga.checkin_cerrado()) ) {
            equipajes_por_cargar = api_carga.obtener_cantidad_equipaje_total();
            Log::info("RobotCarga(%d) notificado checkin cerrado. Equipaje total %d cargados %d\n",
                      id_robot, equipajes_por_cargar, equipajes_cargados);
         }

		}

      //el robot de despacho ya no atiende al vuelo
      Log::info("RobotCarga(%d) Deshabilito el vuelo de la zona (%d) en el robot_despacho", id_robot, id_robot);
      api_despachante.desasignar_vuelo(id_robot); //id_robot = num_zona

		api_carga.esperar_avion_en_zona();

		// cargo el resto de los contenedores
		std::map<std::string, Contenedor>::iterator it;
		for (it = contenedores_por_escala.begin(); it != contenedores_por_escala.end(); it++)
			api_carga.agregar_contenedor_cargado((*it).second);

		Log::info("Carga finalizada, enviando contenedores a tractores");
		api_carga.enviar_contenedores_a_avion(numero_de_vuelo);
		contenedores_por_escala.clear();

		Log::info("RobotCarga(%s) fin de carga de equipajes del vuelo %d, libero la zona %d\n", argv[1], numero_de_vuelo, id_robot);
      api_torre.liberar_zona(id_robot); // id_robot = num_zona
	}

} catch(const std::exception &e) {
   Log::crit("%s", e.what());
} catch(...) {
   Log::crit("Critical error. Unknow exception at the end of the 'main' function.");
}

