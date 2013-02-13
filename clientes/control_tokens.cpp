/*
 * ControlRecursos.cpp
 *
 *  Created on: 12/02/2013
 *      Author: gonzalo
 */

#include "control_tokens.h"
#include "constants.h"
#include <cstring>

#define MEMORIA_DE_CONTROL MAX_NAME_SIZE

ControlTokens::ControlTokens(const std::string & directorio_de_trabajo)
	: memoria_control(std::string(directorio_de_trabajo).append(PATH_CONTROL_TOKENS).c_str(), char(0), 0, false, false),
		mutex(std::string(directorio_de_trabajo).append(PATH_CONTROL_TOKENS).c_str(), char(1), 0)
{
	token_esperando = (char *) memoria_control.memory_pointer();
}

ControlTokens::ControlTokens(const std::string & directorio_de_trabajo, bool create)
	:
		memoria_control(std::string(directorio_de_trabajo).append(PATH_CONTROL_TOKENS).c_str(), char(0),
			MEMORIA_DE_CONTROL, 0664, true, false),
		mutex(std::vector<short unsigned int>(1, 1),
			std::string(directorio_de_trabajo).append(PATH_CONTROL_TOKENS).c_str(), char(1), 0664)
{
	if(create){
		token_esperando = (char *) memoria_control.memory_pointer();
	}
}

ControlTokens::~ControlTokens() {
}

void ControlTokens::cargar_esperando_token(const char nombre [MAX_NAME_SIZE]){
	mutex.wait_on(0);
	strncpy(token_esperando,nombre,MAX_NAME_SIZE);
	mutex.signalize(0);
}

void ControlTokens::limpiar_esperando_token(){
	mutex.wait_on(0);
	*token_esperando = '\0';
	mutex.signalize(0);
}

bool ControlTokens::comparar_token(const char nombre [MAX_NAME_SIZE]){
	bool result;
	mutex.wait_on(0);
	result = (strncmp(token_esperando,nombre,MAX_NAME_SIZE) == 0);
	mutex.signalize(0);
	return result;
}

