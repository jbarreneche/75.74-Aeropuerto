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

#if DEBUG_ROBOT_CARGA_EXTRAE_CINTA_CONTENEDOR==0

void agregar_equipaje(Equipaje & equipaje,
	std::map<std::string, Contenedor> & contenedores_por_escala,
	ApiCarga &api_carga ) {

	Log::info("se tomo el equipaje %s con escala destino '%s' peso=%d\n",
		equipaje.toString().c_str(), equipaje.getRfid().get_escala().c_str(), equipaje.peso());

	if (equipaje.peso() <= MAX_PESO_CONTENEDOR) {
		std::string escala = equipaje.getRfid().get_escala();

		if (contenedores_por_escala.find(escala) == contenedores_por_escala.end()) {
			Log::info("no tengo contenedor, pido contenedor para escala '%s'\n",
				escala.c_str());
			contenedores_por_escala.insert(std::pair<std::string, Contenedor>(escala, Contenedor()));
		}

		if (!contenedores_por_escala[escala].hay_lugar(equipaje)) {
			Log::info("contenedor lleno, pido contenedor para escala '%s'\n",
				escala.c_str());
			api_carga.agregar_contenedor_cargado(contenedores_por_escala[escala]);
			contenedores_por_escala[escala] = Contenedor();
		}
		contenedores_por_escala[escala].agregar_equipaje(equipaje);

	} else {
		Log::crit("El equipaje %s es mas grande que el propio contenedor!!!\n",
			equipaje.toString().c_str());
	}
}

int main(int argc, char** argv) try {
	Equipaje equipaje;
	bool checkin_cerro;
	int id_robot;
	int numero_de_vuelo;
	int equipajes_por_cargar, equipajes_cargados;

	chdir("processes");

	if (argc < 4) {
		Log::crit("Insuficientes parametros para robot de carga, se esperaba (directorio_de_trabajo, config_file, id_robot)\n");
		exit(1);
	}

	id_robot = atoi(argv[3]);

	std::map<std::string, Contenedor> contenedores_por_escala;

	ApiCarga api_carga(std::string("robot_carga").append(argv[3]).c_str(),argv[1], argv[2], id_robot,true,true);
	ApiDespachante api_despachante(argv[1], argv[2],"robot_carga", id_robot,false);
	ApiTorreDeControl api_torre( argv[1], argv[2] );

	Log::info("Iniciando robot carga(%d)\n", id_robot);

	Log::info("lanzando proceso control_carga_contenedores\n");
	char *args_control_carga[] = {(char*) "control_carga_contenedores", (char*) argv[1], (char*)argv[2], (char*) argv[3], NULL};
	Process control_carga_contenedores("control_carga_contenedores", args_control_carga);

	for (;;) {
		checkin_cerro = false;
		equipajes_cargados = 0;
		equipajes_por_cargar = 0;

		while ( (!checkin_cerro) || (equipajes_cargados<equipajes_por_cargar) ) {
			sleep(rand() % SLEEP_ROBOT_CARGA);
			Log::info("Tomando equipaje de Cinta Contenedor\n");
			equipaje = api_carga.sacar_equipaje();
			Log::info("Equipaje (%s) extraido de Cinta Contenedor\n", equipaje.toString().c_str());

			//TODO: por ahora equipaje con rfid 0 es dummy(sale con este valor cuando se despierta la cinta por cierre de checkin)
			if(equipaje.getRfid().rfid != 0) {
				numero_de_vuelo = equipaje.getRfid().numero_de_vuelo_destino; //por ahora el n° de vuelo lo saca del equipaje

				agregar_equipaje(equipaje, contenedores_por_escala, api_carga);
				equipajes_cargados++;

				if(!checkin_cerro) {
					Log::info("pongo equipaje %s en contenedor de escala '%s'.\n",
						equipaje.toString().c_str(), equipaje.getRfid().get_escala().c_str() );
				} else {
					Log::info("pongo equipaje %s en contenedor de escala '%s'.ya carge %d/%d equipajes\n",
						equipaje.toString().c_str(), equipaje.getRfid().get_escala().c_str(),
						equipajes_cargados, equipajes_por_cargar);
				}
			}

			if( (!checkin_cerro) && (checkin_cerro=api_carga.checkin_cerrado()) ) {
				equipajes_por_cargar = api_carga.obtener_cantidad_equipaje_total();
				Log::info("notificado checkin cerrado. Equipaje total %d cargados %d\n",
					equipajes_por_cargar, equipajes_cargados);
			}

		}

		//el robot de despacho ya no atiende al vuelo
		Log::info("Deshabilito el vuelo de la zona (%d) en el robot_despacho", id_robot, id_robot);
		api_despachante.desasignar_vuelo(id_robot);//id_robot = num_zona

		// cargo el resto de los contenedores
		std::map<std::string, Contenedor>::iterator it;
		for (it = contenedores_por_escala.begin(); it != contenedores_por_escala.end(); it++)
		api_carga.agregar_contenedor_cargado((*it).second);

		Log::info("Carga finalizada, enviando contenedores a tractores");
		api_carga.enviar_contenedores_a_avion(numero_de_vuelo);
		contenedores_por_escala.clear();

		Log::info("fin de carga de equipajes del vuelo %d, libero la zona %d\n", numero_de_vuelo, id_robot);
		api_torre.liberar_zona(id_robot);// id_robot = num_zona

		api_carga.comenzar_nueva_carga();
	}

} catch(const std::exception &e) {
	Log::crit("%s", e.what());
} catch(...) {
	Log::crit("Critical error. Unknow exception at the end of the 'main' function.");
}

#elif DEBUG_ROBOT_CARGA_EXTRAE_CINTA_CONTENEDOR==1

int main(int argc, char** argv)
try
{
	Equipaje equipaje;
	int numero_cinta_contenedor;
	int id_robot;
	printf("Debuggeando robot_carga\n");

	chdir("processes");

	if (argc < 4) {
		Log::crit(
			"Insuficientes parametros para robot de carga, se esperaba (directorio_de_trabajo, config_file, id_robot, numero_cinta_contenedor)\n");
		exit(1);
	}

	id_robot = atoi(argv [3]);
	numero_cinta_contenedor = atoi(argv [4]);

	ApiCarga api_carga(std::string("robot_carga").append(argv [3]).c_str(), argv [1], argv [2], id_robot,
		true, true);

	for (int i = 0 ; i < 3 ; i++) {

		printf("Intentando tomar un nuevo equipaje de cinta(%s)\n", argv [3]);
		equipaje = api_carga.sacar_equipaje();
		printf("Equipaje recibido(%s)\n", equipaje.toString().c_str());
	}

}
catch (const std::exception &e) {
	Log::crit("%s", e.what());
}
catch (...) {
	Log::crit("Critical error. Unknow exception at the end of the 'main' function.");
}

#endif

