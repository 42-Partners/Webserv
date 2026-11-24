#!/usr/bin/env bash
# test_finalize.sh - testa os headers adicionados por process()/finalizeResponse()
#
# Uso:
#   ./test_finalize.sh                                    # server 1 (8080, localhost)
#   PORT=8081 VHOST=test.local ./test_finalize.sh         # server 2 (405 testavel)
#   PORT=8080 VHOST=virtual.local ./test_finalize.sh      # server 3 (virtual host)

HOST=${HOST:-127.0.0.1}
PORT=${PORT:-8080}
VHOST=${VHOST:-localhost}                       # valor do header Host
EXISTING_PATH=${EXISTING_PATH:-/}               # path que devolve 200 com body
MISSING_PATH=${MISSING_PATH:-/nao-existe-xyz}   # path que devolve 404
NO_POST_PATH=${NO_POST_PATH:-}                  # location que NAO permite POST (405); vazio = pula
WAIT=${WAIT:-1}                                 # segundos de espera por resposta (keep-alive)

export LC_ALL=C
PASS=0
FAIL=0

# Envia request crua e imprime a resposta.
raw() {
	exec 3<>/dev/tcp/$HOST/$PORT || { echo "sem conexao"; return 1; }
	printf '%b' "$1" >&3
	timeout "$WAIT" cat <&3
	exec 3>&-
}

# check "nome" "resposta" "regex"  (grep -E, case-insensitive)
check() {
	if printf '%s\n' "$2" | tr -d '\r' | grep -qiE "$3"; then
		echo "[ OK ] $1"; PASS=$((PASS+1))
	else
		echo "[FAIL] $1   (esperado: $3)"; FAIL=$((FAIL+1))
	fi
}

# check_not "nome" "resposta" "regex"
check_not() {
	if printf '%s\n' "$2" | tr -d '\r' | grep -qiE "$3"; then
		echo "[FAIL] $1   (nao deveria ter: $3)"; FAIL=$((FAIL+1))
	else
		echo "[ OK ] $1"; PASS=$((PASS+1))
	fi
}

req() {   # req METHOD PATH [extra headers]
	printf '%s %s HTTP/1.1\\r\\nHost: %s\\r\\n%b\\r\\n' "$1" "$2" "$VHOST" "$3"
}

echo "Alvo: $HOST:$PORT  (Host: $VHOST)"
echo

# ---------------------------------------------------------------- 200
echo "== Headers padrao (200) =="
# sentinela 'x' evita que $(...) apague newlines finais do body
R=$(raw "$(req GET "$EXISTING_PATH" '')"; echo x)
R=${R%x}

check "status 200"                  "$R" '^HTTP/1\.1 200'
check "Server: 42webserver"         "$R" '^Server: 42webserver$'
check "Date em formato IMF-fixdate" "$R" '^Date: (Mon|Tue|Wed|Thu|Fri|Sat|Sun), [0-9]{2} (Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec) [0-9]{4} [0-9]{2}:[0-9]{2}:[0-9]{2} GMT$'
check "Content-Type presente"       "$R" '^Content-Type: .+'
check "Content-Type nao e octet-stream (pagina HTML)" "$R" '^Content-Type: text/html'
check "Content-Length presente"     "$R" '^Content-Length: [0-9]+$'
check_not "sem Connection: close (keep-alive padrao)" "$R" '^Connection: close'

# ---------------------------------------------------------------- Date
echo
echo "== Date proximo da hora atual =="
DATE_HDR=$(printf '%s\n' "$R" | tr -d '\r' | grep -i '^Date:' | head -1 | cut -d' ' -f2-)
SRV_TS=$(date -u -d "$DATE_HDR" +%s 2>/dev/null)
NOW_TS=$(date -u +%s)
if [ -n "$SRV_TS" ] && [ $(( NOW_TS - SRV_TS )) -le 5 ] && [ $(( SRV_TS - NOW_TS )) -le 5 ]; then
	echo "[ OK ] Date dentro de +-5s do relogio local"; PASS=$((PASS+1))
else
	echo "[FAIL] Date fora do esperado ($DATE_HDR)"; FAIL=$((FAIL+1))
fi

# ---------------------------------------------------------------- Content-Length
echo
echo "== Content-Length bate com o body =="
BODY=${R#*$'\r\n\r\n'}
BODY_LEN=${#BODY}
CL=$(printf '%s\n' "$R" | tr -d '\r' | grep -i '^Content-Length:' | head -1 | awk '{print $2}')
if [ "$BODY_LEN" = "$CL" ]; then
	echo "[ OK ] Content-Length ($CL) == tamanho do body"; PASS=$((PASS+1))
else
	echo "[FAIL] Content-Length=$CL, body=$BODY_LEN"; FAIL=$((FAIL+1))
fi

# ---------------------------------------------------------------- Connection
echo
echo "== Connection: close pedido pelo cliente =="
R=$(raw "$(req GET "$EXISTING_PATH" 'Connection: close\r\n')")
check "responde Connection: close" "$R" '^Connection: close$'

R=$(raw "$(req GET "$EXISTING_PATH" 'Connection: keep-alive, Close\r\n')")
check "token close entre varios (case-insensitive)" "$R" '^Connection: close$'

# ---------------------------------------------------------------- 404
echo
echo "== Respostas de erro tambem passam pelo finalize =="
R=$(raw "$(req GET "$MISSING_PATH" '')")
check "404"                       "$R" '^HTTP/1\.1 404'
check "404 tem Server"            "$R" '^Server: 42webserver$'
check "404 tem Date"              "$R" '^Date: '
check "404 tem Content-Type"      "$R" '^Content-Type: .+'
check "404 e text/html"           "$R" '^Content-Type: text/html'
check "404 tem Content-Length"    "$R" '^Content-Length: [0-9]+$'
check_not "404 mantem keep-alive" "$R" '^Connection: close'

# ---------------------------------------------------------------- 405
if [ -n "$NO_POST_PATH" ]; then
	R=$(raw "$(req POST "$NO_POST_PATH" 'Content-Length: 0\r\n')")
	if printf '%s\n' "$R" | head -1 | grep -q ' 405'; then
		check "405 tem Allow"     "$R" '^Allow: .+'
		check "405 tem Server"    "$R" '^Server: 42webserver$'
	else
		echo "[FAIL] POST em $NO_POST_PATH nao devolveu 405: $(printf '%s\n' "$R" | head -1 | tr -d '\r')"
		FAIL=$((FAIL+1))
	fi
else
	echo "[SKIP] 405 (defina NO_POST_PATH, ex.: PORT=8081 NO_POST_PATH=/)"
fi

# ---------------------------------------------------------------- parse errors
echo
echo "== Erro de parse fecha a conexao =="
R=$(raw 'ISSO NAO E HTTP\r\n\r\n')
check "400 em request malformada" "$R" '^HTTP/1\.1 400'
check "400 com Connection: close" "$R" '^Connection: close$'
check "400 tem Server"            "$R" '^Server: 42webserver$'

R=$(raw "$(req FOO / '')")
check "metodo desconhecido -> 501"  "$R" '^HTTP/1\.1 501'
check "501 com Connection: close"   "$R" '^Connection: close$'

# ---------------------------------------------------------------- resultado
echo
echo "== Resultado: $PASS ok, $FAIL falhas =="
[ "$FAIL" -eq 0 ]