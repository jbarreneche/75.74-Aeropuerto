#ifndef CONEXIONES_AEROPUERTO_H_
#define CONEXIONES_AEROPUERTO_H_

#include "cintas.h"
#include "api_carga.h"
#include "api_admincontenedores.h"

const int cantidad_cintas_checkin = 1;
const int cantidad_cintas_scanner = 1;
const int cantidad_cintas_centrales = 1;
const int cantidad_cintas_contenedor = 2;
const int cantidad_puestos_checkin = 1;

class TorreDeControl {
public:
	TorreDeControl(char *path_lock_torre_de_control) :
			control(std::vector<short unsigned int>(CANT_MUTEX_CENTRAL, 1),
					path_lock_torre_de_control, MTX_CENTRAL), checkin(path_lock_torre_de_control,
					Q_CHECKINS_HABILITADOS, 0644, true), trasbordo(path_lock_torre_de_control,
					Q_TRASBORDO_LISTO, 0644, true) {
	}
	;

	virtual ~TorreDeControl() {
	}
	;

private:
	SemaphoreSet control;
	MessageQueue checkin;
	MessageQueue trasbordo;
};

class PuestoCheckin {   
public:
   PuestoCheckin(char* path_puesto_checkin, int id_puesto_checkin) :
      sem_checkin_realizado(std::vector<unsigned short>(1,1), path_puesto_checkin, id_puesto_checkin ){
   }
   virtual ~PuestoCheckin() {
   }
private:
   SemaphoreSet sem_checkin_realizado;
};

/*
 * Clase para crear fácilmente todo lo que se necesite en el aeropuerto
 */
class ConexionesAeropuerto {
public:
	ConexionesAeropuerto(const char *path_to_locks) {
		char path_lock[256];

		Log::info("Creando ipcs para Puesto de checkin...%s%s", path_to_locks,
				PATH_PUESTO_CHECKIN);
		snprintf(path_lock, 256, "%s%s", path_to_locks, PATH_PUESTO_CHECKIN);
		for (int i = 0; i < cantidad_puestos_checkin; i++) {
			puesto_checkin[i] = new PuestoCheckin(path_lock, i);
		}
      
		Log::info("Creando ipcs para Torre de control...%s%s", path_to_locks,
				PATH_TORRE_DE_CONTROL);
		snprintf(path_lock, 256, "%s%s", path_to_locks, PATH_TORRE_DE_CONTROL);
		torre_de_control = new TorreDeControl(path_lock);

		Log::info("Creando ipcs admin de contenedores...%s%s", path_to_locks,
				PATH_ADMIN_CONTENEDORES);
		snprintf(path_lock, 256, "%s%s", path_to_locks, PATH_ADMIN_CONTENEDORES);
		admin_contenedores = new ApiAdminContenedores(path_lock, CANT_CONTENEDORES_INICIAL);

		Log::info("Creando cintas...");

		snprintf(path_lock, 256, "%s%s", path_to_locks, PATH_CINTA_CHECKIN);
		for (int i = 0; i < cantidad_cintas_checkin; i++) {
			cintas_checkin[i] = new CintaCheckin(path_lock, i, true);
		}

		snprintf(path_lock, 256, "%s%s", path_to_locks, PATH_CINTA_SCANNER);
		for (int i = 0; i < cantidad_cintas_scanner; i++) {
			cintas_scanner[i] = new CintaScanner(path_lock, i, true);
		}

		snprintf(path_lock, 256, "%s%s", path_to_locks, PATH_CINTA_CENTRAL);
		for (int i = 0; i < cantidad_cintas_centrales; i++) {
			//cintas_central[i] = new CintaCentral(true,path_lock,i);
			cintas_central[i] = new CintaCentral(path_lock,
					CINTA_PRINCIPAL_CANTIDAD_MAXIMA_PRODUCTORES,
					CINTA_PRINCIPAL_CANTIDAD_DESPACHANTES, CINTA_PRINCIPAL_TAMANIO_CINTA_CENTRAL);
		}

		snprintf(path_lock, 256, "%s%s", path_to_locks, PATH_CINTA_CONTENEDOR);
		for (int i = 0; i < cantidad_cintas_contenedor; i++) {
			cintas_contenedor[i] = new CintaContenedor(path_lock, i, true);
		}
	}
	;

	virtual ~ConexionesAeropuerto() {

		for (int i = 0; i < cantidad_puestos_checkin; i++) {
			delete puesto_checkin[i];
		}
		for (int i = 0; i < cantidad_cintas_checkin; i++) {
			delete cintas_checkin[i];
		}
		for (int i = 0; i < cantidad_cintas_scanner; i++) {
			delete cintas_scanner[i];
		}
		for (int i = 0; i < cantidad_cintas_centrales; i++) {
			delete cintas_central[i];
		}
		for (int i = 0; i < cantidad_cintas_contenedor; i++) {
			delete cintas_contenedor[i];
		}

		delete torre_de_control;
		delete admin_contenedores;
	}

private:
   PuestoCheckin * puesto_checkin[1];
	TorreDeControl * torre_de_control;
	CintaCheckin * cintas_checkin[1];
	CintaScanner * cintas_scanner[1];
	CintaCentral * cintas_central[1];
	CintaContenedor * cintas_contenedor[2];
	ApiAdminContenedores * admin_contenedores;
};

#endif
