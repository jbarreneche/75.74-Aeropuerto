#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include "cintas.h"

#include "api_comunicacion_intercargo.h"
#include "api_comunicacion_aeropuerto.h"
#include "contenedor.h"
#include "constants.h"
#include "log.h"

#include <map>

void cargar_equipajes(int numero_vuelo_destino, const char * directorio_de_trabajo,
	std::multimap<int, Equipaje> & equipajes_a_cargar)
{
	char path_archivo_equipajes [300];
	FILE * equipajes_vuelo_destino;
	char descripcion_equipaje [SIZE_DESCRIPCION_EQUIPAJE];
	int peso_equipaje,rfid,vuelo_destino;
	char escala_destino [MAX_SIZE_ESCALA];
	int escalas [MAX_CANT_ESCALAS];
	char primera_linea [300];
	int numero_vuelo_entrante;

	sprintf(path_archivo_equipajes, "%s/equipaje_destino_%d.csv", directorio_de_trabajo, numero_vuelo_destino);

	equipajes_vuelo_destino = fopen(path_archivo_equipajes, "rt");

	fscanf(equipajes_vuelo_destino, "%[^\n]s\n", primera_linea);

	while (fscanf(equipajes_vuelo_destino, "%d:%d:%[^:]:%d:%d:%d:%d:%d:%d:%d:%[^\n]s\n", &numero_vuelo_entrante,
		&peso_equipaje, descripcion_equipaje, &rfid, &vuelo_destino, escalas, escalas + 1, escalas + 2, escalas + 3,
		escalas + 4, escala_destino) != EOF)
	{
		Rfid rf(rfid, numero_vuelo_destino);
		strncpy(rf.escala_destino, escala_destino, MAX_SIZE_ESCALA);
		rf.sospechoso = false;
		memcpy(rf.escalas, escalas, sizeof(escalas));

		Equipaje equipaje(rf, peso_equipaje);
		equipaje.set_descripcion(descripcion_equipaje);
		equipajes_a_cargar.insert(std::pair<int, Equipaje>(numero_vuelo_entrante, equipaje));
	}

	fclose(equipajes_vuelo_destino);
}

int main(int argc, char** argv)
try
{

	//chdir("processes") != 0;
	int id_aeropuerto;
	int numero_vuelo_destino;
	int numero_vuelo_entrante;
	std::multimap<int, Equipaje>::iterator it;
   Log::info("OK, lanzando controlador_equipaje_intercargo");
	if (argc < 3) {
		Log::crit(
			"Insuficientes parametros para controlador equipaje de intercargo, se esperaba (directorio_de_trabajo, config_file, num_vuelo_destino)\n");
		exit(1);
	}
	// std::cout << "robot intercargo, vuelo destino " << argv [3] << std::endl;

	numero_vuelo_destino = atoi(argv [3]);

	ApiComunicacionIntercargo api_intercargo(argv [1], argv [2]);
   ApiComunicacionAeropuerto api_comm_aeropuerto(argv[1], argv[2]);

	std::multimap<int, Equipaje> equipajes_a_cargar;

	cargar_equipajes(numero_vuelo_destino, argv [1], equipajes_a_cargar);

	Log::info("Esperando que se asigne la zona para vuelo %d", numero_vuelo_destino);

	id_aeropuerto = api_comm_aeropuerto.esperar_zona_asignada(numero_vuelo_destino);

	Log::info("Se asigna el vuelo %d a zona en aeropuerto %d", numero_vuelo_destino, id_aeropuerto);

	while (!equipajes_a_cargar.empty()) {
		Log::info("Esperando vuelos entrantes con equipaje destino a %d", numero_vuelo_destino);
		numero_vuelo_entrante = api_intercargo.esperar_vuelo_entrante(numero_vuelo_destino);
		Log::info("Comenzando a cargar equipaje de vuelo entrante %d", numero_vuelo_entrante);
		for (it = equipajes_a_cargar.equal_range(numero_vuelo_entrante).first;
			it != equipajes_a_cargar.equal_range(numero_vuelo_entrante).second ; ++it)
		{
			Log::info("Enviando equipaje %d:%s al robot_intercargo del aeropuerto %d", (*it).second.getRfid().rfid, (*it).second.get_descripcion(), id_aeropuerto);
			// api_trasbordo.poner_en_cinta_principal((*it).second);
         api_comm_aeropuerto.enviar_equipaje_intercargo((*it).second, id_aeropuerto);
		}
		equipajes_a_cargar.erase(numero_vuelo_entrante);
	}
	Log::info("Terminé con los equipajes con destino a %d", numero_vuelo_destino);
}

catch (const std::exception &e) {
	Log::crit("%s", e.what());
}
catch (...) {
	Log::crit("Critical error. Unknow exception at the end of the 'main' function.");
}
