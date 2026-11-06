#!/usr/bin/env bash
# Testa status codes: o código certo, a reason phrase na status line
# e a consistência de Content-Length nas respostas de erro.
#
# Uso: ./test_status.sh   (HOST=... PORT=... opcionais)
#      BIG_MB=5 ./test_status.sh   # tamanho do body para o teste de 413 (limite da conf: 1M)

HOST="${HOST:-localhost}"; PORT="${PORT:-8080}"; BASE="http://$HOST:$PORT"
BIG_MB="${BIG_MB:-5}"
PASS=0; FAIL=0
ok()   { printf '\033[32m[OK]\033[0m   %s\n' "$1"; PASS=$((PASS+1)); }
bad()  { printf '\033[31m[FAIL]\033[0m %s\n' "$1"; FAIL=$((FAIL+1)); }
info() { printf '\033[33m[INFO]\033[0m %s\n' "$1"; }
check()    { [ "$2" == "$3" ] && ok "$1" || bad "$1 (esperado '$2', obtido '$3')"; }
check_in() { [[ " $2 " == *" $3 "* ]] && ok "$1" || bad "$1 (esperado um de [$2], obtido '$3')"; }
status()  { curl -s -o /dev/null -w '%{http_code}' --max-time 10 "$@"; }
sline()   { curl -s -o /dev/null -D - --max-time 5 "$@" | head -n1 | tr -d '\r'; }
raw() {
	exec 3<>"/dev/tcp/$HOST/$PORT" || return 1
	printf '%b' "$1" >&3
	timeout 2 cat <&3
	exec 3>&-
}
raw_status() { raw "$1" | head -n1 | awk '{print $2}'; }
raw_sline()  { raw "$1" | head -n1 | tr -d '\r'; }

curl -s -o /dev/null --max-time 3 "$BASE/" || { echo "Servidor não responde em $BASE"; exit 2; }

echo "== Status codes =="
check    "301 GET /old"                               "301" "$(status "$BASE/old")"
check    "302 GET /temp (redirect sem código)"        "302" "$(status "$BASE/temp")"
check    "307 GET /ext"                               "307" "$(status "$BASE/ext")"
check    "308 GET /perm"                              "308" "$(status "$BASE/perm")"
check    "400 request line inválida"                  "400" "$(raw_status 'GARBAGE\r\n\r\n')"
check    "404 GET /nothing"                           "404" "$(status "$BASE/nothing")"
check    "405 GET /upload"                            "405" "$(status "$BASE/upload")"
check    "505 HTTP/2.0"                               "505" "$(raw_status 'GET /site HTTP/2.0\r\nHost: x\r\n\r\n')"
check_in "2xx POST /upload (body pequeno)"            "200 201 204" "$(status -X POST --data 'a=1' "$BASE/upload")"
check_in "414 URI gigante (10000 chars)"              "414 400"     "$(status "$BASE/$(head -c 10000 /dev/zero | tr '\0' 'a')")"
check_in "501/405 método desconhecido"                "405 501"     "$(status -X FOO "$BASE/site")"

echo
echo "== 413: body maior que client_max_body_size (${BIG_MB}MB vs limite da conf) =="
code="$(head -c "${BIG_MB}M" /dev/zero | curl -s -o /dev/null -w '%{http_code}' --max-time 10 \
	-X POST -H 'Content-Type: application/octet-stream' --data-binary @- "$BASE/upload")"
check "413 body acima do limite" "413" "$code"

echo
echo "== Reason phrase na status line =="
check "301" "HTTP/1.1 301 Moved Permanently"   "$(sline "$BASE/old")"
check "302" "HTTP/1.1 302 Found"               "$(sline "$BASE/temp")"
check "307" "HTTP/1.1 307 Temporary Redirect"  "$(sline "$BASE/ext")"
check "308" "HTTP/1.1 308 Permanent Redirect"  "$(sline "$BASE/perm")"
check "400" "HTTP/1.1 400 Bad Request"         "$(raw_sline 'GARBAGE\r\n\r\n')"
check "404" "HTTP/1.1 404 Not Found"           "$(sline "$BASE/nothing")"
check "405" "HTTP/1.1 405 Method Not Allowed"  "$(sline "$BASE/upload")"
check "505" "HTTP/1.1 505 HTTP Version Not Supported" "$(raw_sline 'GET /site HTTP/2.0\r\nHost: x\r\n\r\n')"

echo
echo "== Content-Length bate com o body nas respostas de erro =="
for path in /nothing /upload /old; do
	declared="$(curl -s -o /dev/null -D - --max-time 5 "$BASE$path" | tr -d '\r' | awk -F': ' 'tolower($1)=="content-length"{print $2}')"
	real="$(curl -s -o /dev/null -w '%{size_download}' --max-time 5 "$BASE$path")"
	if [ -z "$declared" ]; then
		bad "GET $path sem Content-Length"
	else
		check "GET $path: Content-Length ($declared) == bytes recebidos" "$declared" "$real"
	fi
done

echo
echo "== Informativo =="
info "POST sem Content-Length nem chunked -> $(raw_status 'POST /upload HTTP/1.1\r\nHost: x\r\n\r\n')  (411/400 é o ideal; 2xx se você aceita body vazio)"

echo
echo "Resultado: $PASS ok, $FAIL falhas"
[ "$FAIL" -eq 0 ]