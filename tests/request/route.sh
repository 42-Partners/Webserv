#!/usr/bin/env bash
# Testa a função route(): a ORDEM das decisões.
#   1. request com erro      -> handleError + Connection: close
#   2. location não achada   -> 404
#   3. location com redirect -> redirect (ANTES de checar método)
#   4. método não permitido  -> 405 + Allow
#   5. método permitido      -> chega no handler (GET/POST/DELETE)
#
# Uso: ./test_route.sh   (HOST=... PORT=... opcionais)

HOST="${HOST:-localhost}"; PORT="${PORT:-8080}"; BASE="http://$HOST:$PORT"
PASS=0; FAIL=0
ok()   { printf '\033[32m[OK]\033[0m   %s\n' "$1"; PASS=$((PASS+1)); }
bad()  { printf '\033[31m[FAIL]\033[0m %s\n' "$1"; FAIL=$((FAIL+1)); }
info() { printf '\033[33m[INFO]\033[0m %s\n' "$1"; }
check()     { [ "$2" == "$3" ] && ok "$1" || bad "$1 (esperado '$2', obtido '$3')"; }
check_in()  { [[ " $2 " == *" $3 "* ]] && ok "$1" || bad "$1 (esperado um de [$2], obtido '$3')"; }
check_not() { [ "$2" != "$3" ] && ok "$1" || bad "$1 (não deveria ser '$2')"; }
status()  { curl -s -o /dev/null -w '%{http_code}' --max-time 5 "$@"; }
headers() { curl -s -o /dev/null -D - --max-time 5 "$@" | tr -d '\r'; }
raw() { # envia bytes crus pelo socket e devolve a resposta
	exec 3<>"/dev/tcp/$HOST/$PORT" || return 1
	printf '%b' "$1" >&3
	timeout 2 cat <&3
	exec 3>&-
}
raw_status() { raw "$1" | head -n1 | awk '{print $2}'; }

curl -s -o /dev/null --max-time 3 "$BASE/" || { echo "Servidor não responde em $BASE"; exit 2; }

echo "== 1. Request com erro -> handleError + Connection: close =="
check "request line inválida -> 400"      "400" "$(raw_status 'GARBAGE\r\n\r\n')"
check "HTTP/1.1 sem Host -> 400"          "400" "$(raw_status 'GET /site HTTP/1.1\r\n\r\n')"
check "HTTP/2.0 -> 505"                   "505" "$(raw_status 'GET /site HTTP/2.0\r\nHost: x\r\n\r\n')"
if raw 'GARBAGE\r\n\r\n' | tr -d '\r' | grep -qi '^connection: close'; then
	ok "resposta de erro traz Connection: close"
else
	bad "resposta de erro NÃO traz Connection: close"
fi

echo
echo "== 2. Location não encontrada =="
check "GET /nothing -> 404" "404" "$(status "$BASE/nothing")"

echo
echo "== 3. Redirect vem ANTES da checagem de método =="
check "GET    /old -> 301" "301" "$(status "$BASE/old")"
check "POST   /old -> 301 (mesmo sem POST permitido)"   "301" "$(status -X POST   "$BASE/old")"
check "DELETE /old -> 301 (mesmo sem DELETE permitido)" "301" "$(status -X DELETE "$BASE/old")"
check "POST   /temp -> 302" "302" "$(status -X POST "$BASE/temp")"

echo
echo "== 4. Método não permitido -> 405 + Allow =="
check "GET    /upload -> 405"  "405" "$(status "$BASE/upload")"
check "DELETE /upload -> 405"  "405" "$(status -X DELETE "$BASE/upload")"
check "POST   /site   -> 405"  "405" "$(status -X POST "$BASE/site")"
check "GET    /delete -> 405"  "405" "$(status "$BASE/delete")"
if headers "$BASE/upload" | grep -qi '^allow:.*POST'; then ok "405 traz header Allow com POST"
else bad "405 sem header Allow (ou sem POST nele)"; fi
check_in "método desconhecido (FOO) em /site -> 405 ou 501" "405 501" "$(status -X FOO "$BASE/site")"

echo
echo "== 5. Método permitido chega no handler (não é 405/500/501) =="
code="$(status "$BASE/upload/any/")";                          check_in "GET    /upload/any/        chegou no handleGet"    "200 403 404" "$code"
code="$(status -X POST --data 'a=1' "$BASE/upload")";          check_in "POST   /upload             chegou no handlePost"   "200 201 204 400 403 404 409" "$code"
code="$(status -X DELETE "$BASE/delete/arquivo-inexistente")"; check_in "DELETE /delete/inexistente  chegou no handleDelete" "404" "$code"

echo
echo "== Servidor continua vivo depois de tudo =="
check_not "GET /nothing ainda responde" "000" "$(status "$BASE/nothing")"

echo
echo "Resultado: $PASS ok, $FAIL falhas"
[ "$FAIL" -eq 0 ]