#!/usr/bin/env bash

BIN="./webserv"
CGI_DIR="./tests/cgi-bin"
CONF_1="/tmp/webserv_cgi_1.conf"
CONF_2="/tmp/webserv_cgi_2.conf"

SERVER_PID=""
PASSED=0
FAILED=0
FAILED_TESTS=()

GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

stop_server() {
    if [ -n "$SERVER_PID" ] && kill -0 "$SERVER_PID" 2>/dev/null; then
        kill "$SERVER_PID" 2>/dev/null
        wait "$SERVER_PID" 2>/dev/null
    fi
    SERVER_PID=""
}

cleanup() {
    stop_server
    rm -f "$CONF_1" "$CONF_2" /tmp/cgi_body.out /tmp/cgi_headers.out /tmp/cgi_large_body.txt
}
trap cleanup EXIT

if [ ! -x "$BIN" ]; then
    echo -e "${RED}Erro: Binário não encontrado em $BIN${NC}"
    exit 1
fi

mkdir -p ./www
[ ! -f ./www/index.html ] && echo "<h1>Index</h1>" > ./www/index.html

chmod +x "${CGI_DIR}/cgienv.py" "${CGI_DIR}/cgieco.py" "${CGI_DIR}/cgiinf.py" 2>/dev/null
python3 -c 'print("BLOCO_DE_DADOS_CGI_" * 5000)' > /tmp/cgi_large_body.txt

# Config 1: Porta 8085 (GET e POST permitidos no CGI)
cat << EOF > "$CONF_1"
server {
    listen 8085;
    host 127.0.0.1;
    server_name cgi.local;
    client_max_body_size 5M;

    location / {
        root ./www;
        index index.html;
        allow_methods GET;
    }

    location /cgi-bin {
        root ./tests;
        allow_methods GET POST;
    }
}
EOF

# Config 2: Porta 8086 (Apenas GET permitido no CGI)
cat << EOF > "$CONF_2"
server {
    listen 8086;
    host 127.0.0.1;
    server_name cgi_restricted.local;

    location / {
        root ./www;
        index index.html;
        allow_methods GET;
    }

    location /cgi-bin {
        root ./tests;
        allow_methods GET;
    }
}
EOF

start_server() {
    stop_server
    $BIN "$1" > /dev/null 2>&1 &
    SERVER_PID=$!
    for i in {1..30}; do
        curl -s "http://127.0.0.1:${2}/" > /dev/null 2>&1 && break
        sleep 0.1
    done
    sleep 0.1
}

run_test() {
    local num="$1" desc="$2" exp_status="$3" exp_body="$4" exp_hdr="$5"
    shift 5

    rm -f /tmp/cgi_headers.out /tmp/cgi_body.out
    local status
    status=$(curl -s -D /tmp/cgi_headers.out -o /tmp/cgi_body.out -w "%{http_code}" "$@")
    local body=$([ -f /tmp/cgi_body.out ] && cat /tmp/cgi_body.out || echo "")

    local hdr_ok=1
    if [ -n "$exp_hdr" ]; then
        grep -qi "$exp_hdr" /tmp/cgi_headers.out 2>/dev/null || hdr_ok=0
    fi

    if [ "$status" == "$exp_status" ] && \
       { [ -z "$exp_body" ] || [[ "$body" == *"$exp_body"* ]]; } && \
       [ "$hdr_ok" -eq 1 ]; then
        ((PASSED++))
    else
        ((FAILED++))
        FAILED_TESTS+=("#${num} [${desc}] (esperado ${exp_status}, obtido ${status})")
    fi
}

# --- Bateria 1: Porta 8085 ---
start_server "$CONF_1" 8085

run_test 1 "Meta-variaveis + Query String" "200" "aluno=42" "Content-Type: text/html" \
    "http://127.0.0.1:8085/cgi-bin/cgienv.py?aluno=42"

run_test 2 "Eco POST pequeno + Header CGI" "200" "Payload rapido" "X-Echo-Length: 14" \
    -X POST --data "Payload rapido" "http://127.0.0.1:8085/cgi-bin/cgieco.py"

LARGE_LEN=$(wc -c < /tmp/cgi_large_body.txt | tr -d ' ')
run_test 3 "Eco POST grande (multi-ciclo pipe)" "200" "BLOCO_DE_DADOS_CGI_" "X-Echo-Length: ${LARGE_LEN}" \
    -X POST --data-binary "@/tmp/cgi_large_body.txt" "http://127.0.0.1:8085/cgi-bin/cgieco.py"

TIMEOUT_STATUS=$(curl -s -m 10 -o /dev/null -w "%{http_code}" "http://127.0.0.1:8085/cgi-bin/cgiinf.py")
if [[ "$TIMEOUT_STATUS" == "504" || "$TIMEOUT_STATUS" == "500" ]]; then
    ((PASSED++))
else
    ((FAILED++))
    FAILED_TESTS+=("#4 [Timeout Loop Infinito] (esperado 504/500, obtido ${TIMEOUT_STATUS})")
fi

run_test 5 "Resiliencia apos timeout" "200" "GATEWAY_INTERFACE" "" \
    "http://127.0.0.1:8085/cgi-bin/cgienv.py"

run_test 6 "Script CGI inexistente" "404" "" "" \
    "http://127.0.0.1:8085/cgi-bin/inexistente.py"

# --- Bateria 2: Porta 8086 ---
start_server "$CONF_2" 8086

run_test 7 "CGI GET permitido" "200" "SERVER_PORT" "" \
    "http://127.0.0.1:8086/cgi-bin/cgienv.py"

run_test 8 "CGI POST bloqueado (405)" "405" "" "" \
    -X POST --data "teste" "http://127.0.0.1:8086/cgi-bin/cgieco.py"

# --- Relatório ---
echo "========================================"
echo "RELATÓRIO DE TESTES E2E - CGI"
echo "========================================"
echo -e "Total executados : $((PASSED + FAILED))"
echo -e "Passaram         : ${GREEN}${PASSED}${NC}"
echo -e "Falharam         : ${RED}${FAILED}${NC}"

if [ "$FAILED" -gt 0 ]; then
    echo "----------------------------------------"
    for fail in "${FAILED_TESTS[@]}"; do
        echo -e "  ${RED}- Teste ${fail}${NC}"
    done
    echo "========================================"
    exit 1
fi
echo "========================================"