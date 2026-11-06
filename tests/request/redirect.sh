#!/usr/bin/env bash
# Testes de redirect
#
# Uso:
#   ./test_redirect.sh                          # testa servidor já rodando em localhost:8080
#   HOST=localhost PORT=9090 ./test_redirect.sh
#   SERVER_BIN=./webserv ./test_redirect.sh     # também testa configs inválidas

HOST="${HOST:-localhost}"
PORT="${PORT:-8080}"
BASE="http://$HOST:$PORT"

PASS=0
FAIL=0

green() { printf '\033[32m%s\033[0m\n' "$*"; }
red()   { printf '\033[31m%s\033[0m\n' "$*"; }

check() { # nome esperado obtido
	if [ "$2" == "$3" ]; then
		green "[OK]   $1"
		PASS=$((PASS + 1))
	else
		red   "[FAIL] $1 (esperado: '$2', obtido: '$3')"
		FAIL=$((FAIL + 1))
	fi
}

status()   { curl -s -o /dev/null -w '%{http_code}' "$@"; }
location() { curl -s -o /dev/null -D - "$@" | tr -d '\r' | awk -F': ' 'tolower($1)=="location"{print $2}'; }
header()   { # nome-do-header url [args do curl]
	local h="$1"; shift
	curl -s -o /dev/null -D - "$@" | tr -d '\r' | awk -F': ' -v h="$h" 'tolower($1)==tolower(h){print $2}'
}

if ! curl -s -o /dev/null --max-time 3 "$BASE/"; then
	red "Servidor não responde em $BASE. Suba o servidor antes de rodar os testes."
	exit 2
fi

echo "== Status e Location =="
check "301 em /old"              "301"                  "$(status "$BASE/old")"
check "Location em /old"         "/new"                 "$(location "$BASE/old")"
check "302 padrão em /temp"      "302"                  "$(status "$BASE/temp")"
check "Location em /temp"        "/new"                 "$(location "$BASE/temp")"
check "307 em /ext"              "307"                  "$(status "$BASE/ext")"
check "Location absoluto /ext"   "https://example.com"  "$(location "$BASE/ext")"
check "308 em /perm"             "308"                  "$(status "$BASE/perm")"

echo
echo "== Status line (reason phrase) =="
line="$(curl -s -o /dev/null -D - "$BASE/old" | head -n1 | tr -d '\r')"
check "status line 301"          "HTTP/1.1 301 Moved Permanently" "$line"

echo
echo "== Content-Length =="
cl="$(header content-length "$BASE/old")"
if [ -n "$cl" ]; then
	green "[OK]   Content-Length presente ($cl)"; PASS=$((PASS + 1))
else
	red   "[FAIL] Content-Length ausente (ou Transfer-Encoding?)"; FAIL=$((FAIL + 1))
fi

echo
echo "== Métodos (redirect vale para todos) =="
check "POST   /old"   "301" "$(status -X POST   "$BASE/old")"
check "DELETE /old"   "301" "$(status -X DELETE "$BASE/old")"

echo
echo "== Seguindo o redirect (-L) =="
final="$(curl -s -L -o /dev/null -w '%{url_effective}' "$BASE/old")"
check "/old termina em /new"  "$BASE/new" "$final"

echo
echo "== Keep-alive (2 requisições na mesma conexão) =="
curl -s -o /dev/null -o /dev/null --max-time 5 "$BASE/old" "$BASE/old"
rc=$?
check "duas requisições sem travar (rc=0)" "0" "$rc"

# ---------------------------------------------------------------------------
# Configs inválidas: o servidor deve se recusar a subir (exit != 0 rápido).
# Só roda se SERVER_BIN estiver definido.
# ---------------------------------------------------------------------------
if [ -n "$SERVER_BIN" ]; then
	echo
	echo "== Configs inválidas (SERVER_BIN=$SERVER_BIN) =="

	tmpdir="$(mktemp -d)"
	trap 'rm -rf "$tmpdir"' EXIT

	bad_config() { # nome diretiva-dentro-da-location
		local name="$1" directive="$2" cfg="$tmpdir/bad.conf"
		cat > "$cfg" <<EOF
server {
    listen 18080;
    location /a { $directive }
}
EOF
		timeout 2 "$SERVER_BIN" "$cfg" > /dev/null 2>&1
		local rc=$?
		# 124 = o timeout matou o servidor, ou seja, ele subiu (errado)
		if [ "$rc" -ne 0 ] && [ "$rc" -ne 124 ]; then
			green "[OK]   rejeitou: $name"
			PASS=$((PASS + 1))
		else
			red   "[FAIL] aceitou config inválida: $name (rc=$rc)"
			FAIL=$((FAIL + 1))
		fi
	}

	bad_config "return 200 /foo"        "return 200 /foo;"
	bad_config "return 999 /foo"        "return 999 /foo;"
	bad_config "return 301; (sem URL)"  "return 301;"
	bad_config "loop /a -> /a"          "return 301 /a;"
	bad_config "return vazio"           "return;"
fi

echo
echo "Resultado: $PASS ok, $FAIL falhas"
[ "$FAIL" -eq 0 ]