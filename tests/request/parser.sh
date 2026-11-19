#!/usr/bin/env bash
# Testa o parser de request pelo lado de fora: manda bytes crus pelo socket
# (o curl não deixa mandar requests malformados) e confere o status devolvido.
#
# Uso: ./test_request_parser.sh   (HOST=... PORT=... opcionais)
#
# Precisa do servidor já serializando e enviando respostas.
# Rotas usadas (da test.conf): /nothing -> 404, /upload -> aceita POST.

HOST="${HOST:-localhost}"; PORT="${PORT:-8080}"
PASS=0; FAIL=0
ok()   { printf '\033[32m[OK]\033[0m   %s\n' "$1"; PASS=$((PASS+1)); }
bad()  { printf '\033[31m[FAIL]\033[0m %s\n' "$1"; FAIL=$((FAIL+1)); }
info() { printf '\033[33m[INFO]\033[0m %s\n' "$1"; }
check_in() { [[ " $2 " == *" $3 "* ]] && ok "$1" || bad "$1 (esperado um de [$2], obtido '${3:-nada}')"; }

# envia bytes crus e devolve a resposta inteira
raw() {
	exec 3<>"/dev/tcp/$HOST/$PORT" || return 1
	printf '%b' "$1" >&3
	timeout 2 cat <&3
	exec 3>&-
}
# envia em pedaços, com pausa entre eles (testa parsing incremental)
raw_pieces() {
	exec 3<>"/dev/tcp/$HOST/$PORT" || return 1
	local p
	for p in "$@"; do printf '%b' "$p" >&3; sleep 0.3; done
	timeout 2 cat <&3
	exec 3>&-
}
code()        { raw "$1" | head -n1 | awk '{print $2}'; }
code_pieces() { raw_pieces "$@" | head -n1 | awk '{print $2}'; }

(exec 3<>"/dev/tcp/$HOST/$PORT") 2>/dev/null || { echo "Servidor não responde em $HOST:$PORT"; exit 2; }
if [ -z "$(code 'GET /nothing HTTP/1.1\r\nHost: x\r\n\r\n')" ]; then
	echo "O servidor aceitou a conexão mas não respondeu nada (falta serializar/enviar a resposta?)"; exit 2
fi

H='Host: x\r\n'

echo "== Requests válidos =="
check_in "GET simples"                        "404" "$(code "GET /nothing HTTP/1.1\r\n${H}\r\n")"
check_in "nome de header case-insensitive"    "404" "$(code 'GET /nothing HTTP/1.1\r\nhOsT: x\r\n\r\n')"
check_in "espaços após ':' do header"         "404" "$(code 'GET /nothing HTTP/1.1\r\nHost:    x\r\n\r\n')"
check_in "query string no path"               "404" "$(code "GET /nothing?a=1&b=2 HTTP/1.1\r\n${H}\r\n")"
check_in "HTTP/1.0 sem Host"                  "404 400" "$(code 'GET /nothing HTTP/1.0\r\n\r\n')"
check_in "header extra desconhecido"          "404" "$(code "GET /nothing HTTP/1.1\r\n${H}X-Qualquer: coisa\r\n\r\n")"

echo
echo "== Request line inválida -> 400 =="
check_in "lixo total"                         "400" "$(code 'GARBAGE\r\n\r\n')"
check_in "sem versão"                         "400" "$(code 'GET /nothing\r\n\r\n')"
check_in "dois espaços entre os campos"       "400" "$(code "GET  /nothing HTTP/1.1\r\n${H}\r\n")"
check_in "path sem '/' inicial"               "400" "$(code "GET nothing HTTP/1.1\r\n${H}\r\n")"
check_in "versão com formato inválido"        "400" "$(code "GET /nothing HTTP/abc\r\n${H}\r\n")"
check_in "método minúsculo (get)"             "400 405 501" "$(code "get /nothing HTTP/1.1\r\n${H}\r\n")"
check_in "versão HTTP/2.0 -> 505"             "505" "$(code "GET /nothing HTTP/2.0\r\n${H}\r\n")"

echo
echo "== Headers inválidos =="
check_in "HTTP/1.1 sem Host -> 400"           "400" "$(code 'GET /nothing HTTP/1.1\r\n\r\n')"
check_in "header sem ':' -> 400"              "400" "$(code "GET /nothing HTTP/1.1\r\n${H}BadHeader\r\n\r\n")"
check_in "espaço antes do ':' -> 400"         "400" "$(code 'GET /nothing HTTP/1.1\r\nHost : x\r\n\r\n')"
check_in "Host duplicado -> 400"              "400" "$(code 'GET /nothing HTTP/1.1\r\nHost: a\r\nHost: b\r\n\r\n')"

echo
echo "== Limites de tamanho =="
longuri="$(head -c 10000 /dev/zero | tr '\0' 'a')"
check_in "URI com 10000 chars -> 414"         "414 400" "$(code "GET /${longuri} HTTP/1.1\r\n${H}\r\n")"
longhdr="$(head -c 70000 /dev/zero | tr '\0' 'a')"
check_in "header com 70000 chars"             "400 431 413" "$(code "GET /nothing HTTP/1.1\r\n${H}X-Big: ${longhdr}\r\n\r\n")"
check_in "Content-Length enorme -> 413"       "413 400" "$(code "POST /upload HTTP/1.1\r\n${H}Content-Length: 99999999999\r\n\r\n")"

echo
echo "== Content-Length inválido =="
check_in "não numérico"                       "400" "$(code "POST /upload HTTP/1.1\r\n${H}Content-Length: abc\r\n\r\nhello")"
check_in "negativo"                           "400" "$(code "POST /upload HTTP/1.1\r\n${H}Content-Length: -5\r\n\r\nhello")"
check_in "duplicado com valores diferentes"   "400" "$(code "POST /upload HTTP/1.1\r\n${H}Content-Length: 5\r\nContent-Length: 6\r\n\r\nhello")"

echo
echo "== Body =="
check_in "POST com body completo"             "200 201 204" "$(code "POST /upload HTTP/1.1\r\n${H}Content-Length: 5\r\n\r\nhello")"
check_in "chunked (501 se não suportar)"      "200 201 204 501" "$(code "POST /upload HTTP/1.1\r\n${H}Transfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n0\r\n\r\n")"

echo
echo "== Parsing incremental (request chegando em pedaços) =="
check_in "request line quebrada em 3 pedaços" "404" "$(code_pieces 'GE' 'T /nothing HT' 'TP/1.1\r\nHost: x\r\n\r\n')"
check_in "headers e fim separados"            "404" "$(code_pieces 'GET /nothing HTTP/1.1\r\n' 'Host: x\r\n' '\r\n')"
check_in "\\r e \\n em pedaços diferentes"    "404" "$(code_pieces 'GET /nothing HTTP/1.1\r' '\nHost: x\r' '\n\r' '\n')"
check_in "body em pedaços"                    "200 201 204" "$(code_pieces "POST /upload HTTP/1.1\r\n${H}Content-Length: 5\r\n\r\n" 'he' 'llo')"

echo
echo "== Pipelining (2 requests na mesma conexão) =="
n="$(raw "GET /nothing HTTP/1.1\r\n${H}\r\nGET /nothing HTTP/1.1\r\n${H}\r\n" | tr -d '\r' | grep -c '^HTTP/1.1 404')"
[ "$n" == "2" ] && ok "duas respostas 404 recebidas" || bad "esperadas 2 respostas, recebidas $n (o parser descarta o que sobra do buffer?)"

echo
echo "== Informativo (decisão sua) =="
info "só \\n, sem \\r (bare LF)            -> $(code 'GET /nothing HTTP/1.1\nHost: x\n\n')  (404 se tolerante, 400 se estrito)"
info "Content-Length + Transfer-Encoding  -> $(code "POST /upload HTTP/1.1\r\n${H}Content-Length: 5\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n0\r\n\r\n")  (400 é o mais seguro)"
info "header com continuação (obs-fold)   -> $(code "GET /nothing HTTP/1.1\r\n${H}X-A: um\r\n  dois\r\n\r\n")  (400 ou 404)"

echo
echo "Resultado: $PASS ok, $FAIL falhas"
[ "$FAIL" -eq 0 ]