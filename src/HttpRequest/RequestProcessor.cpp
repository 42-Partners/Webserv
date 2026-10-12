#include "RequestProcessor/RequestProcessor.hpp"

/*
404 Not Found -> nenhum arquivo ou rota corresponde
405 Method Not Allowed -> método reconhecido, mas não permitido naquele location. Aqui a resposta deve trazer o header Allow
403 Forbidden -> sem permissão de leitura, ou diretório sem index com autoindex desligado
*/

void process(Connection & connection, ServerConfig & config) { 
	(void)connection;
	(void)config;
}