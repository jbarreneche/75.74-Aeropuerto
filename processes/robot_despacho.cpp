#include <iostream>
#include <stdlib.h>
#include <unistd.h>

#include "api_cinta.h"
#include "api_zona.h"
#include "api_admin.h"
#include "api_scanner.h"
#include "log.h"

#include "contenedor.h"
#include "equipaje.h"

#include <map>
#include <string>
#include "cintas.h"

/*
 * Argumentos para el robot de despacho.
 * ./robot_despacho <id_robot> <cinta_in> <cinta_zona1> <zona1> ... <cinta_zonan> <zonan>
 * En donde la cinta i lleva equipajes hacia la zona i.
 **/

int main( int argc, char** argv ) {
   
   if ( (argc < 5) || ((argc-4)%2!=0) ) {
      Log::crit("Insuficientes parametros para robot de despacho, se esperaba (id, path_cinta_central id_cinta_central path_cinta_contenedor id_cinta_contenedor...)\n");
      exit(1);
   }

   ApiScanner scanner(0);
   ApiAdmin admin;

   CintaCentral cinta_central( argv[2], atoi(argv[3]) );
   CintaContenedor cinta_contenedor( argv[4], atoi(argv[5]) );

   Log::info("Iniciando robot despacho(%s), cinta_central:%s cinta_contenedor:%s\n", argv[1], argv[3], argv[5] );
   
   for(;;) {
      sleep(rand() % 10);

      Log::info("Robot de despacho(%s) Sacando un nuevo equipaje de cinta(%s)\n", argv[1], argv[2]);
      Equipaje equipaje = cinta_central.obtener_valija_para( atoi(argv[1]) );

      int num_vuelo = admin.get_numero_vuelo(equipaje);
      int num_zona = admin.get_numero_zona(num_vuelo);
      Log::info("Robot despacho(%s), toma el equipaje %s con destino al vuelo %d (zona %d)", 
                argv[1], 
                equipaje.toString().c_str(), 
                num_vuelo,
                num_zona );

         cinta_contenedor.poner_equipaje(equipaje);
         Log::info("Robot de despacho(%s)  pongo equipaje %s en la cinta de contenedor %s \n", 
                   argv[1], equipaje.toString().c_str(), argv[5]); 
      
   } 
 
}
