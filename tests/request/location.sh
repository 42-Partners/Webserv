#!/usr/bin/env bash
# Testa resolveLocation(): qual location casa com cada path.
# Truque: o header Allow / o status 405 revela QUAL location foi escolhida,
# porque cada uma da test.conf permite métodos diferentes.
#
# Uso: ./test_location.sh   (HOST=... PORT=... opcionais)

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
allow()   { headers "$@" | awk -F': ' 'tolower($1)=="allow"{print $2}' | tr ',' ' ' | xargs -n1 | sort | xargs; }

curl -s -o /dev/null --max-time 3 "$BASE/" || { echo "Servidor não responde em $BASE"; exit 2; }

echo "== Sem location correspondente =="
check "GET /           -> 404 (não há location /)" "404" "$(status "$BASE/")"
check "GET /nothing    -> 404"                     "404" "$(status "$BASE/nothing")"
check "GET /UPLOAD     -> 404 (case-sensitive)"    "404" "$(status "$BASE/UPLOAD")"

echo
echo "== Match exato e por prefixo =="
check "GET /upload            -> 405"        "405"  "$(status "$BASE/upload")"
check "Allow de /upload       = POST"        "POST" "$(allow "$BASE/upload")"
check "GET /upload/           -> 405"        "405"  "$(status "$BASE/upload/")"
check "GET /upload/file.txt   -> 405 (prefixo de /upload)" "405" "$(status "$BASE/upload/file.txt")"
check "Allow de /upload/file.txt = POST"     "POST" "$(allow "$BASE/upload/file.txt")"

echo
echo "== Prefixo mais longo vence =="
check_not "GET /upload/any          != 405 (location /upload/any)"  "405" "$(status "$BASE/upload/any")"
check_not "GET /upload/any/f.txt    != 405"                         "405" "$(status "$BASE/upload/any/f.txt")"
check_not "GET /upload/any/a/b/c    != 405"                         "405" "$(status "$BASE/upload/any/a/b/c")"
check "Allow de /upload/any = DELETE GET POST" "DELETE GET POST" "$(allow "$BASE/upload/any")"

echo
echo "== Query string não participa do match =="
check "GET /upload?x=1          -> 405 (casou /upload)"  "405" "$(status "$BASE/upload?x=1")"
check "GET /nothing?x=/upload   -> 404"                  "404" "$(status "$BASE/nothing?x=/upload")"

echo
echo "== Path traversal (--path-as-is) =="
code="$(status --path-as-is "$BASE/site/../../etc/passwd")"
check_in "/site/../../etc/passwd não vaza arquivo" "400 403 404" "$code"
code="$(status --path-as-is "$BASE/upload/any/../../../etc/passwd")"
check_in "/upload/any/../../../etc/passwd não vaza arquivo" "400 403 404" "$code"

echo
echo "== Location com redirect =="
check "GET /old -> 301" "301" "$(status "$BASE/old")"

echo
echo "== Casos de comportamento livre (só informativo) =="
info "GET /upload/anyway     -> $(status "$BASE/upload/anyway")  (match por string pega /upload/any; por segmento pega /upload)"
info "GET //upload           -> $(status "$BASE//upload")"
info "GET /%75pload (u cod.) -> $(status "$BASE/%75pload")  (405 se você decodifica antes do match)"
info "GET /upload/any/../    -> $(status --path-as-is "$BASE/upload/any/../")"

echo
echo "Resultado: $PASS ok, $FAIL falhas"
[ "$FAIL" -eq 0 ]