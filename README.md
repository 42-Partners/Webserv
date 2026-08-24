# Webserv — Divisão do Projeto em Features Numeradas

## Índice

**Fase 0 — Fundamentos:** 1. Estrutura do repositório e Makefile · 2. Parsing dos argumentos de linha de comando · 3. Parser do arquivo de configuração · 4. Estruturas de dados centrais

**Fase 1 — Núcleo de rede:** 5. Sockets de escuta (multi-porta) · 6. Modo non-blocking · 7. Loop principal com poll único · 8. Aceitação de conexões · 9. Ciclo de vida da conexão · 10. Múltiplos servers no mesmo loop

**Fase 2 — Parsing HTTP:** 11. Request-line e headers · 12. Corpo via Content-Length · 13. Corpo via chunked · 14. Limite de tamanho do corpo · 15. Keep-Alive

**Fase 3 — Roteamento:** 16. Resolução de server · 17. Resolução de location · 18. Validação de método · 19. Redirecionamento

**Fase 4 — Conteúdo estático:** 20. Servir arquivos estáticos · 21. Listagem de diretório · 22. Índice padrão · 23. Páginas de erro

**Fase 5 — Métodos e upload:** 24. Finalização do GET · 25. POST e upload · 26. multipart/form-data · 27. DELETE

**Fase 6 — CGI:** 28. Infraestrutura (fork/execve/pipe) · 29. Variáveis de ambiente · 30. Ponte stdin/stdout · 31. Timeout e erros

**Fase 7 — Robustez:** 32. Auditoria de status codes · 33. Path traversal · 34. Navegadores e NGINX · 35. Stress test

**Fase 8 — QA:** 36. Suíte de testes própria · 37. Tester oficial · 38. Configs de demonstração

**Fase 9 — Documentação:** 39. README.md

**Fase 10 — Bônus:** 40. Cookies e sessões · 41. Múltiplos tipos de CGI

---

## Restrições gerais (valem para o projeto inteiro)

- **Linguagem:** C++98 estrito — nada de C++11 em diante.
- **Libft:** não autorizada neste projeto.
- **Funções externas permitidas (única lista disponível):** `execve`, `pipe`, `strerror`, `gai_strerror`, `errno`, `dup`, `dup2`, `fork`, `socketpair`, `htons`, `htonl`, `ntohs`, `ntohl`, `select`, `poll`, `epoll_create`/`epoll_ctl`/`epoll_wait`, `kqueue`/`kevent`, `socket`, `accept`, `listen`, `send`, `recv`, `chdir`, `bind`, `connect`, `getaddrinfo`, `freeaddrinfo`, `setsockopt`, `getsockname`, `getprotobyname`, `fcntl`, `close`, `read`, `write`, `waitpid`, `kill`, `signal`, `access`, `stat`, `open`, `opendir`, `readdir`, `closedir`.
- **Makefile:** precisa ter as regras `NAME`, `all`, `clean`, `fclean`, `re`.
- **Execução:** `./webserv [configuration file]`.
- **fork():** só pode ser usado para CGI — nunca para outra finalidade.
- **execve():** nunca pode chamar outro servidor web.
- **poll() (ou equivalente):** apenas **um único** poll/select/epoll/kqueue cobrindo leitura e escrita de todos os fds de rede (incluindo os sockets de escuta); proibido checar `errno` depois de `read`/`write` para decidir comportamento; proibido ler ou escrever num fd sem que o poll tenha indicado que ele está pronto — exceto arquivos regulares em disco, que ficam de fora dessa regra.

---

## Fase 0 — Fundamentos do projeto

### 1. Estrutura do repositório e Makefile
**Objetivo:** preparar a base para compilar o projeto de forma incremental e organizada.
**Implementação:**
- Estrutura sugerida: `srcs/`, `includes/`, `config/` (arquivos `.conf` de exemplo), `www/` (site estático de teste), `cgi-bin/` (scripts CGI de teste).
- Makefile com as regras `NAME`, `all`, `clean`, `fclean`, `re`, flags `-Wall -Wextra -Werror -std=c++98`, e compilação incremental (só recompila `.cpp` alterados).
**Base no subject:** "Makefile NAME, all, clean, fclean, re"; arquivos a entregar incluem Makefile, `*.{h,hpp}`, `*.cpp`, `*.tpp`, `*.ipp` e arquivos de configuração.
**Como validar:** `make`, `make re`, `make clean` e `make fclean` funcionam sem warnings nem erros.

### 2. Parsing dos argumentos de linha de comando
**Objetivo:** implementar corretamente `./webserv [configuration file]`.
**Implementação:**
- Se nenhum argumento for passado, usar um caminho de configuração padrão.
- Validar existência e legibilidade do arquivo (`access`/`stat`) antes de tentar abrir.
- Tratar erros (arquivo inexistente, sem permissão, mais de um argumento) com mensagem clara, sem crash.
**Base no subject:** "Arguments: [A configuration file]".
**Como validar:** rodar sem argumentos usa o default; rodar com caminho inválido imprime erro e sai com código de erro, sem crashar.

### 3. Parser do arquivo de configuração
**Objetivo:** ler e interpretar um arquivo de configuração inspirado no bloco `server` do NGINX.
**Implementação:**
- Definir a gramática mínima: blocos `server` (um ou mais), cada um com `listen`, `server_name` (opcional), `error_page`, `client_max_body_size`, e blocos `location` (um ou mais).
- Cada `location` deve suportar: métodos aceitos, root/diretório, redirecionamento, autoindex on/off, arquivo de índice, upload habilitado + diretório de armazenamento, extensão de CGI.
- Reportar erros de sintaxe de forma clara (linha/motivo).
- Exemplo ilustrativo (a sintaxe exata é livre, já que o subject não exige regex nem formato específico):
```
server {
    listen 8080;
    server_name example.com;
    client_max_body_size 10M;
    error_page 404 /errors/404.html;

    location / {
        root ./www;
        index index.html;
        allow_methods GET POST;
        autoindex off;
    }

    location /upload {
        root ./www/upload;
        allow_methods POST DELETE;
        upload_store ./www/upload;
    }

    location /cgi-bin {
        root ./www/cgi-bin;
        cgi_extension .py /usr/bin/python3;
    }
}
```
**Base no subject:** "You can take inspiration from the 'server' section of the NGINX configuration file"; lista completa de diretivas exigidas na seção IV.3.
**Como validar:** dado um `.conf` de exemplo, o parser popula corretamente as estruturas internas (testável com prints/testes unitários).

### 4. Estruturas de dados centrais
**Objetivo:** modelar as entidades principais do servidor em C++98.
**Implementação:**
- `ServerConfig` e `LocationConfig` (resultado do parser da feature 3).
- `HttpRequest` (método, URI, versão, headers, corpo) e `HttpResponse` (status, headers, corpo).
- `Connection`/`Client` — encapsula o fd do cliente, buffers de leitura/escrita e o estado atual (lendo headers, lendo corpo, processando, escrevendo resposta etc.).
**Como validar:** projeto compila limpo com `-std=c++98 -Wall -Wextra -Werror`.

---

## Fase 1 — Núcleo de rede (sockets e loop de eventos)

### 5. Criação e binding dos sockets de escuta (multi-porta)
**Objetivo:** suportar múltiplos pares interface:porta definidos na configuração.
**Implementação:** para cada `listen` da config, chamar `socket()`, `setsockopt()` (`SO_REUSEADDR`), `bind()` e `listen()`; manter um socket de escuta por combinação interface:porta.
**Base no subject:** "Define all the interface:port pairs on which your server will listen to"; "listen to multiple ports to deliver different content".
**Como validar:** `netstat`/`lsof` confirma que todas as portas configuradas estão em `LISTEN`.

### 6. Modo non-blocking em todos os file descriptors
**Objetivo:** garantir que nenhuma operação de I/O de rede bloqueie o processo.
**Implementação:** `fcntl(fd, F_SETFL, O_NONBLOCK)` em todos os sockets de escuta e de cliente assim que são criados/aceitos.
**Atenção (macOS):** só é permitido usar `fcntl()` com as flags `F_SETFL`, `O_NONBLOCK` e `FD_CLOEXEC` — nenhuma outra flag.
**Base no subject:** "Your server must remain non-blocking at all times."
**Como validar:** nenhuma chamada bloqueia mesmo sob carga (checável com `strace`/`dtruss` ou teste de stress).

### 7. Loop principal de eventos com poll()/equivalente único
**Objetivo:** implementar o coração do servidor — um único loop de `poll`/`select`/`epoll`/`kqueue` monitorando leitura **e** escrita de todos os fds simultaneamente.
**Implementação:**
- Manter uma lista dinâmica de fds monitorados (sockets de escuta sempre para leitura; sockets de cliente para leitura e/ou escrita conforme o estado da conexão).
- Nunca chamar `read`/`recv`/`write`/`send` sem que o poll tenha indicado que o fd está pronto.
- Nunca checar `errno` após `read`/`write` para decidir o comportamento seguinte.
- `select()` com as macros `FD_SET`/`FD_ISR` etc. é permitido, assim como qualquer helper associado ao mecanismo escolhido.
- Dica prática: `poll()`/`select()` são portáveis entre Linux e macOS sem código condicional; `epoll` (Linux) e `kqueue` (BSD/macOS) exigem compilação condicional por SO, mas escalam melhor.
**Base no subject:** "It must be non-blocking and use only 1 poll() ... for all the I/O operations ... poll() must monitor both reading and writing simultaneously ... You must never do a read or a write operation without going through poll() ... Checking the value of errno ... is strictly forbidden."
**Como validar:** revisão de código confirma um único poll() por iteração cobrindo todos os fds; nenhum `read`/`write` "cego" no código.

### 8. Aceitação de novas conexões
**Objetivo:** aceitar clientes quando um socket de escuta sinalizar leitura pronta.
**Implementação:** `accept()` não bloqueante; aplicar non-blocking (feature 6) ao novo fd; adicioná-lo à lista monitorada pelo poll; inicializar o estado da conexão (buffers, parser).
**Base no subject:** requisito implícito de servir múltiplos clientes de forma não bloqueante.
**Como validar:** múltiplos clientes conectam simultaneamente (várias abas de navegador ou múltiplos `curl` em paralelo).

### 9. Ciclo de vida da conexão (leitura, escrita, fechamento, timeout)
**Objetivo:** tratar corretamente desconexões e conexões ociosas.
**Implementação:**
- Detectar `recv()` retornando 0 (cliente fechou) e remover o fd do poll + fechar o socket.
- Tratar erros de socket sem derrubar o servidor.
- Implementar um timeout de conexão ociosa para não deixar clientes "pendurados" para sempre.
**Base no subject:** "properly handle client disconnections when necessary"; "A request to your server should never hang indefinitely."
**Como validar:** fechar a aba do navegador no meio de um request não trava nem crasha o servidor; conexões inativas são limpas após o timeout.

### 10. Suporte a múltiplos server blocks no mesmo loop
**Objetivo:** garantir que todos os "servers" virtuais definidos na config sejam atendidos por um único loop de eventos, sem duplicar poll() por servidor.
**Implementação:** mapear cada socket de escuta ao(s) `ServerConfig` correspondente(s); ao aceitar uma conexão, associar o cliente ao contexto correto para resolução de rota depois (feature 16).
**Base no subject:** múltiplos `listen` na mesma configuração, todos atendidos pelo mesmo processo/loop.
**Como validar:** dois `server` com portas diferentes respondem corretamente na mesma execução do binário.

---

## Fase 2 — Parsing de requisições HTTP

### 11. Parser incremental de request-line e headers
**Objetivo:** parsear requisições que chegam fragmentadas em múltiplas chamadas de `recv()` (já que a leitura é non-blocking).
**Implementação:**
- Máquina de estados: `request-line → headers → body`, que continua de onde parou a cada novo pedaço de dado.
- Parsear método, URI e versão HTTP; parsear headers (`Host`, `Content-Length`, `Transfer-Encoding` etc.) para um mapa.
- Responder `400 Bad Request` em requisições malformadas e `505` se a versão HTTP não for suportada.
**Base no subject:** "The HTTP 1.0 is suggested as a reference point, but not enforced."; "Your HTTP response status codes must be accurate."
**Como validar:** enviar uma requisição via `telnet`/`nc` em pedaços pequenos (ex.: 1 byte por vez) e confirmar que o parser monta a requisição corretamente.

### 12. Leitura do corpo via Content-Length
**Objetivo:** ler corretamente corpos com tamanho conhecido.
**Implementação:** usar o header `Content-Length` para saber quantos bytes ler; parar exatamente nesse limite; tratar `Content-Length` ausente/inconsistente com o método.
**Como validar:** POST com corpo de tamanho conhecido é lido byte a byte corretamente (comparar tamanho recebido x header).

### 13. Leitura do corpo via Transfer-Encoding: chunked
**Objetivo:** suportar requisições chunked — necessário inclusive para alimentar o CGI corretamente.
**Implementação:** parser de chunks (tamanho em hexadecimal + CRLF + dados + CRLF, terminando num chunk de tamanho 0); "dechunkar" o corpo antes de repassar ao CGI, já que o CGI espera EOF, não chunks.
**Base no subject:** "for chunked requests, your server needs to un-chunk them, the CGI will expect EOF as the end of the body."
**Como validar:** `curl -H "Transfer-Encoding: chunked"` (ou script próprio) enviando corpo em chunks é processado corretamente.

### 14. Limite de tamanho do corpo (client_max_body_size)
**Objetivo:** rejeitar corpos maiores que o configurado.
**Implementação:** ler o limite por server/location da config; ao exceder, responder `413 Payload Too Large` e drenar/fechar a conexão sem travar o parser.
**Base no subject:** "Set the maximum allowed size for client request bodies."
**Como validar:** enviar corpo maior que o limite configurado retorna 413 e o servidor continua respondendo a outras requisições.

### 15. Keep-Alive e gerenciamento de conexão persistente
**Objetivo:** decidir se a conexão deve permanecer aberta após a resposta.
**Implementação:** respeitar o header `Connection` (e o padrão de cada versão HTTP — 1.1 mantém viva por padrão, 1.0 fecha por padrão); reutilizar a mesma conexão para requisições sequenciais quando aplicável; resetar o estado do parser após cada requisição completa.
**Como validar:** o navegador consegue carregar uma página com múltiplos recursos (CSS, imagens) reaproveitando a mesma conexão TCP.

---

## Fase 3 — Roteamento

### 16. Resolução de server (porta / Host)
**Objetivo:** decidir qual bloco `server` atende a requisição recebida.
**Implementação:** usar a porta da conexão como critério principal; opcionalmente, usar o header `Host` para desempatar entre vários `server` na mesma porta; se não houver match, usar o primeiro `server` daquela porta como default.
**Base no subject:** "the virtual host feature is considered out of scope. But you are allowed to implement it if you want."
**Como validar:** dois `server` na mesma porta com `server_name` diferentes respondem conforme o `Host` (se implementado) — no mínimo, o comportamento default funciona.

### 17. Resolução de location (matching de rota)
**Objetivo:** encontrar a `location` que melhor corresponde à URI solicitada.
**Implementação:** escolher a `location` cujo prefixo mais específico (mais longo) combina com a URI (sem regex); mapear a URI para o root correspondente — ex.: se `/kapouet` aponta para `/tmp/www`, a URI `/kapouet/pouic/toto/pouet` deve resolver para `/tmp/www/pouic/toto/pouet`.
**Base no subject:** "no regex required here"; exemplo do `/kapouet` citado literalmente no subject.
**Como validar:** requisições para diferentes paths resolvem para os diretórios corretos conforme os exemplos da config.

### 18. Validação de método permitido por rota
**Objetivo:** checar se o método da requisição está na lista de métodos aceitos daquela location.
**Implementação:** se o método não estiver na lista, responder `405 Method Not Allowed`, idealmente com header `Allow` listando os métodos aceitos.
**Base no subject:** "List of accepted HTTP methods for the route."
**Como validar:** `DELETE` numa location que só aceita `GET` retorna 405.

### 19. Redirecionamento HTTP configurável
**Objetivo:** suportar redirecionamento definido na location.
**Implementação:** responder com o status configurado (ex.: 301/302) e header `Location` apontando para a URL de destino definida na config.
**Base no subject:** "HTTP redirection."
**Como validar:** acessar uma rota configurada como redirect leva o navegador para a URL de destino esperada.

---

## Fase 4 — Conteúdo estático

### 20. Servir arquivos estáticos (mecânica do GET)
**Objetivo:** entregar o conteúdo de arquivos do sistema de arquivos.
**Implementação:** abrir o arquivo resolvido pela location; montar `Content-Type` via extensão (mapa extensão → MIME), `Content-Length` e status `200`; tratar arquivo inexistente (404) e sem permissão (403). Leitura de arquivos regulares em disco não precisa passar pelo poll (é a exceção explícita da regra da feature 7).
**Base no subject:** "You must be able to serve a fully static website."; "You are not required to use poll() ... for regular disk files."
**Como validar:** um site estático completo (HTML, CSS, JS, imagens) carrega corretamente no navegador.

### 21. Listagem de diretório (autoindex)
**Objetivo:** gerar uma página HTML simples listando os arquivos de um diretório quando habilitado.
**Implementação:** checar a flag `autoindex on/off` por location; gerar HTML com links para cada entrada usando `opendir`/`readdir`/`closedir`.
**Base no subject:** "Enabling or disabling directory listing."
**Como validar:** acessar um diretório sem índice com `autoindex on` mostra a listagem; com `off`, cai para índice (feature 22) ou erro apropriado.

### 22. Arquivo de índice padrão para diretórios
**Objetivo:** servir um arquivo default (ex.: `index.html`) quando a URI aponta para um diretório.
**Implementação:** checar (`access`/`stat`) se o arquivo configurado como `index` existe dentro do diretório; se existir, servi-lo; senão, cair para autoindex (se habilitado) ou erro.
**Base no subject:** "Default file to serve when the requested resource is a directory."
**Como validar:** acessar `/` com `index.html` presente serve o arquivo automaticamente, sem precisar especificar o nome na URL.

### 23. Páginas de erro (default e customizadas)
**Objetivo:** responder com páginas de erro apropriadas para cada status.
**Implementação:** ter páginas de erro default embutidas no binário/projeto (funcionam mesmo sem configuração); permitir sobrescrever via `error_page <código> <caminho>` por server.
**Base no subject:** "Your server must have default error pages if none are provided."
**Como validar:** erros 404/403/500 etc. mostram página coerente (customizada se configurada, default caso contrário).

---

## Fase 5 — Métodos HTTP e upload

### 24. Finalização do método GET (headers de resposta)
**Objetivo:** consolidar a resposta do GET com os headers corretos.
**Implementação:** garantir `Content-Type`, `Content-Length`, `Date` e `Server` corretos e consistentes em toda resposta.
**Como validar:** `curl -i` mostra headers e corpo corretos para diferentes tipos de arquivo (html, css, js, png etc.).

### 25. Método POST e recepção/armazenamento de upload
**Objetivo:** processar o corpo de requisições POST e, quando aplicável, salvar como upload.
**Implementação:** usar a leitura de corpo já implementada (features 12/13); se a location permite upload, salvar o conteúdo no diretório configurado (`upload_store`) com um nome derivado (ex.: do `Content-Disposition` em multipart, ou nome gerado); responder `201 Created` (ou `200`) com informação do recurso criado.
**Base no subject:** "Clients must be able to upload files."; "Uploading files from the clients to the server is authorized, and storage location is provided."
**Como validar:** enviar um arquivo via `curl -F` ou formulário HTML e confirmar que ele aparece no diretório de upload configurado.

### 26. Parsing de multipart/form-data
**Objetivo:** suportar o formato mais comum de upload feito por navegador.
**Implementação:** parsear o `boundary` do header `Content-Type`; separar as partes; extrair nome do arquivo e conteúdo binário de cada parte.
**Como validar:** um formulário HTML com `<input type="file">` consegue enviar um arquivo com sucesso e o conteúdo salvo bate byte a byte com o original.

### 27. Método DELETE
**Objetivo:** remover um recurso do servidor.
**Implementação:** resolver o path do arquivo alvo; validar que ele permanece dentro do root permitido (ver feature 33); remover o arquivo; responder `204 No Content` (ou `200`) em sucesso, `404` se não existir, `403` se sem permissão.
**⚠️ Ponto de atenção:** a lista de "funções externas" do subject não inclui explicitamente `unlink`/`remove`, que normalmente são a forma de apagar um arquivo em C/C++. Vale confirmar com a equipe/staff qual chamada é aceita para esta operação, para não correr risco na correção por usar uma função fora da lista permitida.
**Base no subject:** "You need at least the GET, POST, and DELETE methods."
**Como validar:** `curl -X DELETE` remove o arquivo, e uma segunda tentativa na mesma URL retorna 404.

---

## Fase 6 — CGI

### 28. Infraestrutura de execução de CGI (fork + execve + pipe)
**Objetivo:** executar um programa CGI externo para a extensão configurada (ex.: `.php`, `.py`).
**Implementação:**
- Identificar se a location tem `cgi_extension` configurada e se o arquivo solicitado corresponde.
- Usar `pipe()` (ou `socketpair()` como alternativa full-duplex) para stdin/stdout do processo filho.
- `fork()`; no filho, redirecionar fds com `dup`/`dup2` e chamar `execve()` com o interpretador correto; no pai, fechar as pontas não usadas dos pipes e continuar monitorando os fds do pipe **dentro do mesmo poll()** usado para a rede — nunca criar um segundo mecanismo de espera.
- O CGI deve rodar no diretório correto do script (`chdir`) para que caminhos relativos dentro dele funcionem.
**Base no subject:** "You can't use fork for anything other than CGI"; "The CGI should be run in the correct directory for relative path file access."; "Your server should support at least one CGI (php-CGI, Python, and so forth)."
**Como validar:** acessar um script CGI simples ("Hello CGI") retorna o output esperado no navegador.

### 29. Variáveis de ambiente do CGI (meta-variáveis)
**Objetivo:** fornecer ao CGI as variáveis padrão do protocolo CGI/1.1.
**Implementação:** montar pelo menos `REQUEST_METHOD`, `QUERY_STRING`, `CONTENT_TYPE`, `CONTENT_LENGTH`, `SCRIPT_NAME`, `PATH_INFO`, `SERVER_PROTOCOL`, `SERVER_NAME`, `SERVER_PORT`, `GATEWAY_INTERFACE`; garantir que a requisição completa e os argumentos do cliente estejam disponíveis ao CGI.
**Dica prática:** se usar `php-cgi`, ele exige a variável `REDIRECT_STATUS` (ou a flag `cgi.force_redirect=0`) para aceitar rodar fora do Apache — um erro comum de quem implementa isso pela primeira vez.
**Base no subject:** "Have a careful look at the environment variables involved in the web server-CGI communication. The full request and arguments provided by the client must be available to the CGI."
**Como validar:** um script CGI de teste que imprime todas as variáveis de ambiente recebidas mostra os valores esperados.

### 30. Ponte de dados stdin/stdout com o CGI
**Objetivo:** fazer a ponte de dados bidirecional cliente ↔ servidor ↔ CGI.
**Implementação:** escrever o corpo da requisição (já "dechunkado", se era chunked — feature 13) no stdin do processo, de forma non-blocking, através do poll; ler o stdout do CGI da mesma forma; se o CGI não retornar `Content-Length`, usar EOF (fechamento do pipe) para saber que a resposta terminou; parsear os headers que o CGI devolve (ex.: `Status:`, `Content-Type:`) antes do corpo.
**Base no subject:** "The same applies to the output of the CGI. If no content_length is returned from the CGI, EOF will mark the end of the returned data."
**Como validar:** um CGI que lê o stdin e ecoa de volta funciona corretamente com corpos pequenos e grandes (múltiplos ciclos de leitura/escrita).

### 31. Timeout e tratamento de erros do CGI
**Objetivo:** evitar que um CGI travado ou com erro derrube o servidor ou cause hang.
**Implementação:** usar `waitpid()` de forma não bloqueante (`WNOHANG`) dentro do loop para colher o processo filho; usar `kill()` para encerrar um CGI que ultrapassar um tempo limite; mapear o exit code/sinal do CGI para `500 Internal Server Error`/`502 Bad Gateway` conforme apropriado.
**Base no subject:** "A request to your server should never hang indefinitely."
**Como validar:** um CGI que entra em loop infinito não trava o servidor nem os outros clientes.

---

## Fase 7 — Robustez e conformidade HTTP

### 32. Auditoria de status codes HTTP
**Objetivo:** revisar sistematicamente que cada cenário retorna o código HTTP correto.
**Implementação:** montar uma tabela cenário → status esperado (200, 201, 204, 301/302, 400, 403, 404, 405, 413, 500, 501, 505 etc.) e validar um a um com `curl`/`telnet`.
**Base no subject:** "Your HTTP response status codes must be accurate."
**Como validar:** tabela de testes com resultado esperado x obtido, 100% de acerto.

### 33. Proteção contra path traversal
**Objetivo:** impedir que URLs como `/../../etc/passwd` escapem do diretório root configurado.
**Implementação:** normalizar o path resolvido e verificar que ele permanece dentro do root da location antes de qualquer `open`/`stat`/remoção.
**Como validar:** tentativas de path traversal retornam 403/404 e nunca expõem arquivos fora do root.

### 34. Compatibilidade com navegadores e comparação com NGINX
**Objetivo:** validar que navegadores reais (não só `curl`) navegam o site normalmente.
**Implementação:** testar carregamento de página com múltiplos recursos, formulários, uploads e erros no Chrome/Firefox/Safari; comparar headers e comportamento lado a lado com uma instância local do NGINX nos casos de dúvida.
**Base no subject:** "Your server must be compatible with standard web browsers of your choice."; "NGINX may be used to compare headers and answer behaviours."
**Como validar:** navegação fluida, sem erros de console relacionados a headers malformados.

### 35. Testes de resiliência / stress test
**Objetivo:** garantir que o servidor não trava, não vaza memória/fds e não fica indisponível sob carga.
**Implementação:** usar `siege`, `ab` (Apache Bench) ou scripts próprios (Python/Go) para gerar muitas conexões simultâneas, conexões lentas e requisições malformadas; monitorar memória e fds abertos (`valgrind`, `lsof`) durante o teste.
**Base no subject:** "Stress test your server to ensure it remains available at all times."; "Resilience is key. Your server must remain operational at all times."
**Como validar:** o servidor sobrevive a um período prolongado de carga sem crashar, sem vazar recursos e sem ficar indisponível.

---

## Fase 8 — QA e material de avaliação

### 36. Suíte de testes automatizados própria
**Objetivo:** ter testes repetíveis para cada feature (métodos, erros, CGI, upload, redirect etc.).
**Implementação:** escrever scripts em Python (`requests`/`socket`) ou Go cobrindo os cenários da feature 32, incluindo testes de baixo nível via socket puro para validar o comportamento non-blocking/dados parciais.
**Base no subject:** "Do not test with only one program. Write your tests in a more suitable language, such as Python or Golang."
**Como validar:** a suíte roda com um comando e reporta pass/fail por cenário.

### 37. Uso do tester oficial fornecido pela escola
**Objetivo:** rodar a ferramenta de teste opcional mencionada no subject para achar bugs adicionais.
**Implementação:** integrar a saída do tester ao processo de QA do projeto (não é obrigatório, mas ajuda a encontrar bugs).
**Base no subject:** "We have provided a small tester. Using it is not mandatory if everything works fine with your browser and tests, but it can help you find and fix bugs."
**Como validar:** o tester roda sem apontar falhas críticas.

### 38. Configs e conteúdo de demonstração para a correção
**Objetivo:** preparar arquivos de configuração e conteúdo cobrindo todas as features para a avaliação.
**Implementação:** ao menos um config com: múltiplos servers/portas; locations com diferentes métodos permitidos; location com redirect; location com autoindex on e off; location com upload habilitado; location com CGI (pelo menos 1 extensão); error pages customizadas; `client_max_body_size` definido.
**Base no subject:** "You must provide configuration files and default files to test and demonstrate that every feature works during the evaluation."
**Como validar:** cada exemplo de config tem um roteiro claro de "como testar" (documentado, por exemplo, no README ou num doc de correção).

---

## Fase 9 — Documentação

### 39. README.md conforme o Capítulo V do subject
**Objetivo:** escrever a documentação obrigatória do repositório.
**Implementação, deve conter:**
- Primeira linha, em itálico, exatamente: *This project has been created as part of the 42 curriculum by \<login1\>[, \<login2\>[, \<login3\>[...]]].*
- Seção **"Description"** — objetivo e visão geral do projeto.
- Seção **"Instructions"** — compilação, instalação e execução.
- Seção **"Resources"** — referências clássicas (RFCs de HTTP, documentação de `poll`/`epoll`/`kqueue`, artigos, tutoriais) **e** uma descrição de como IA foi usada, especificando para quais tarefas e em quais partes do projeto.
- Seções adicionais conforme a necessidade do projeto (ex.: exemplos de uso, lista de features, decisões técnicas).
- Todo o README deve estar em **inglês**.
**Base no subject:** Capítulo V (Readme Requirements) do documento.
**Como validar:** revisar o README linha por linha contra o checklist acima.

---

## Fase 10 — Bônus (só valem se o mandatory estiver 100% completo e funcional)

### 40. Cookies e gerenciamento de sessão
**Objetivo:** implementar suporte básico a cookies e sessões simples.
**Implementação:** gerar um identificador de sessão; armazená-lo em memória (mapa `session_id → dados`); enviar via header `Set-Cookie`; ler via header `Cookie` nas requisições seguintes; fornecer exemplos simples de uso (ex.: contador de visitas por sessão).
**Base no subject:** "Support cookies and session management (provide simple examples)."

### 41. Suporte a múltiplos tipos de CGI
**Objetivo:** generalizar as features 28–30 para suportar mais de um interpretador/extensão simultaneamente (ex.: `.php` e `.py` na mesma configuração).
**Base no subject:** "Handle multiple CGI types" — este é o último trecho presente no documento enviado, que parece cortado no meio da seção de Bônus.

---

## Nota final

O texto fornecido termina logo após "Handle multiple CGI types", no meio do Capítulo VI. Se o enunciado completo tiver mais itens de bônus (é comum esse tipo de projeto incluir também suporte a mais de um CGI ao mesmo tempo, ou outras features extras), me manda o restante do documento que eu complemento a Fase 10.