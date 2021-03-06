#ifndef EQUIPAJE_H_
#define EQUIPAJE_H_

#include "rfid.h"
#include <string>
#include <sstream>
/*
 * Es una Valija.
 * Tiene un rfid que la identifica
 **/

#define SIZE_DESCRIPCION_EQUIPAJE 30

class Equipaje {
public:

	Equipaje() :
      rfid(0,0), _peso(0) {
	}

	Equipaje(Rfid id, int peso = 0) :
			rfid(id), _peso(peso) {
		descripcion[0]='\0';
		this->rfid.sospechoso = false;
	}

	Rfid getRfid() const {
		return rfid;
	}

	Rfid& getRfid() {
		return rfid;
	}

	operator int() const {
		return rfid.rfid;
	}

	std::string toString() const {
		std::ostringstream ostream;
		ostream << "( Rfid:" << rfid.rfid << ":vuelo " << rfid.numero_de_vuelo_destino << ":" << this->descripcion << ")";
		return ostream.str();
	}

	int peso() const {
		return _peso;
	}

	bool es_sospechoso() {
		return rfid.sospechoso;
	}

	void set_sospechoso(bool sospechoso) {
		rfid.sospechoso = sospechoso;
	}

	void set_descripcion(const char * desc) {
		strncpy(this->descripcion, desc, 30);
	}

	char * get_descripcion() {
		return this->descripcion;
	}

private:
	Rfid rfid;
	int _peso;
	char descripcion[SIZE_DESCRIPCION_EQUIPAJE];
};

#endif /* EQUIPAJE_H_ */
