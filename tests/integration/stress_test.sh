#!/usr/bin/env bash

# ==============================================================================
# Suite Completa de Testes E2E & Stress - Webserv (42)
# ==============================================================================

BIN="./build/webserv"
CONF_FILE="/tmp/webserv_full_e2e.conf"
CGI_DIR="./tests/cgi-bin"
SERVER_PID=""
PASSED=0
FAILED=0
FAILED_TESTS=()

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

cleanup() {
    if [ -n "$SERVER_PID" ] && kill -0 "$SERVER_PID" 2>/dev/null; then
        kill "$SERVER_PID" 2>/dev/null
        wait "$SERVER_PID" 2>/dev/null
    fi
    rm -f "$CONF_FILE" /tmp/test_upload.txt /tmp/large_payload.bin \
          /tmp/curl_body.out /tmp/curl_headers.out /tmp/cgi_large_body.txt \
          ./uploads/test_upload.txt ./uploads/to_delete.txt
}
trap cleanup EXIT

if [ ! -x "$BIN" ]; then
    echo -e "${RED}Erro: Binário não encontrado ou sem permissão em $BIN${NC}"
    exit 1
fi

# Garante que nenhuma instância anterior esteja ocupando as portas 8080/8081
pkill -f "build/webserv" 2>/dev/null || true
sleep 0.2

# ------------------------------------------------------------------------------
# 1. Preparação do Ambiente Determinístico
# ------------------------------------------------------------------------------
mkdir -p ./www/virtual ./www/dir_listing ./uploads "${CGI_DIR}"

[ ! -f ./www/index.html ] && echo "<h1>Index Principal</h1>" > ./www/index.html
[ ! -f ./www/404.html ] && echo "<h1>Custom 404 Page</h1>" > ./www/404.html
[ ! -f ./www/virtual/index.html ] && echo "<h1>Virtual Host Index</h1>" > ./www/virtual/index.html
echo "Arquivo 1" > ./www/dir_listing/item_alpha.txt
echo "Arquivo 2" > ./www/dir_listing/item_beta.txt

# Captura uma assinatura real de cada index.html para não falhar por diferença de texto
MAIN_INDEX_SIG=$(head -n 1 ./www/index.html | tr -d '\r\n')
VHOST_INDEX_SIG=$(head -n 1 ./www/virtual/index.html | tr -d '\r\n')

# Garante scripts CGI mínimos caso ainda não existam
if [ ! -f "${CGI_DIR}/cgienv.py" ]; then
    cat << 'PYEOF' > "${CGI_DIR}/cgienv.py"
#!/usr/bin/env python3
import os
print("Content-Type: text/html\r\n\r\n", end="")
for k, v in sorted(os.environ.items()):
    print(f"<li><strong>{k}</strong>: {v}</li>")
PYEOF
fi

if [ ! -f "${CGI_DIR}/cgieco.py" ]; then
    cat << 'PYEOF' > "${CGI_DIR}/cgieco.py"
#!/usr/bin/env python3
import sys
body = sys.stdin.read()
print("Content-Type: text/plain")
print(f"X-Echo-Length: {len(body)}\r\n")
print(f"Echo recebido com sucesso:\n{body}", end="")
PYEOF
fi

if [ ! -f "${CGI_DIR}/cgiinf.py" ]; then
    cat << 'PYEOF' > "${CGI_DIR}/cgiinf.py"
#!/usr/bin/env python3
while True:
    pass
PYEOF
fi
chmod +x "${CGI_DIR}/cgienv.py" "${CGI_DIR}/cgieco.py" "${CGI_DIR}/cgiinf.py"

# Payloads temporários
echo "Conteudo de teste para upload multipart" > /tmp/test_upload.txt
dd if=/dev/zero of=/tmp/large_payload.bin bs=1M count=3 2>/dev/null # 3MB
python3 -c 'print("BLOCO_DE_DADOS_CGI_" * 5000)' > /tmp/cgi_large_body.txt

# Configuração alinhada exatamente às diretivas do Parser.cpp
cat << EOF > "$CONF_FILE"
server {
    listen 8080;
    host 127.0.0.1;
    server_name localhost;
    client_max_body_size 10M;
    error_page 404 /404.html;

    location / {
        root ./www;
        index index.html;
        allow_methods GET POST;
    }

    location /upload {
        root ./uploads;
        allow_methods POST DELETE;
        upload_path ./uploads;
    }

    location /dir_listing {
        root ./www/dir_listing;
        autoindex on;
        allow_methods GET;
    }

    location /old-page {
        redirect 301 http://127.0.0.1:8080/index.html;
    }

    location /cgi-bin {
        root ./tests;
        allow_methods GET POST;
    }
}

server {
    listen 8081;
    host 127.0.0.1;
    server_name secondary.local;
    client_max_body_size 2M;

    location / {
        root ./www;
        index index.html;
        allow_methods GET POST;
    }
}

server {
    listen 8080;
    host 127.0.0.1;
    server_name virtual.local;
    client_max_body_size 5M;

    location / {
        root ./www/virtual;
        index index.html;
        allow_methods GET;
    }
}
EOF

# ------------------------------------------------------------------------------
# 2. Inicialização do Webserv
# ------------------------------------------------------------------------------
$BIN "$CONF_FILE" > /tmp/webserv_e2e.log 2>&1 &
SERVER_PID=$!

READY=0
for i in {1..30}; do
    if ! kill -0 "$SERVER_PID" 2>/dev/null; then
        echo -e "${RED}Erro fatal: Webserv encerrou ao iniciar. Log:${NC}"
        cat /tmp/webserv_e2e.log
        exit 1
    fi
    if curl -s http://127.0.0.1:8080/ > /dev/null && curl -s http://127.0.0.1:8081/ > /dev/null; then
        READY=1
        break
    fi
    sleep 0.1
done

if [ "$READY" -ne 1 ]; then
    echo -e "${RED}Timeout aguardando Webserv abrir as portas 8080 e 8081.${NC}"
    cat /tmp/webserv_e2e.log
    exit 1
fi

# ------------------------------------------------------------------------------
# 3. Helpers de Validação
# ------------------------------------------------------------------------------
run_test() {
    local num="$1" desc="$2" exp_status="$3" exp_body="$4" exp_hdr="$5"
    shift 5

    rm -f /tmp/curl_headers.out /tmp/curl_body.out
    local status_code
    status_code=$(curl -s -D /tmp/curl_headers.out -o /tmp/curl_body.out -w "%{http_code}" "$@")
    local body=$([ -f /tmp/curl_body.out ] && cat /tmp/curl_body.out || echo "")

    local ok=1
    [ "$status_code" != "$exp_status" ] && ok=0
    [ -n "$exp_body" ] && [[ "$body" != *"$exp_body"* ]] && ok=0
    if [ -n "$exp_hdr" ]; then
        grep -qi "$exp_hdr" /tmp/curl_headers.out 2>/dev/null || ok=0
    fi

    if [ "$ok" -eq 1 ]; then
        echo -e "  ${GREEN}[PASS]${NC} #${num} - ${desc}"
        ((PASSED++))
    else
        echo -e "  ${RED}[FAIL]${NC} #${num} - ${desc} (esperado ${exp_status}, obtido ${status_code})"
        ((FAILED++))
        FAILED_TESTS+=("#${num} [${desc}] (esperado HTTP ${exp_status}, obtido ${status_code})")
    fi
}

echo -e "${CYAN}=== FASE 1: Rotas Estáticas, Erros e Métodos ===${NC}"
run_test 1 "GET / (Index padrão)" "200" "$MAIN_INDEX_SIG" "" "http://127.0.0.1:8080/"
run_test 2 "GET /index.html explícito" "200" "$MAIN_INDEX_SIG" "" "http://127.0.0.1:8080/index.html"
run_test 3 "GET inexistente -> Custom 404" "404" "Custom 404" "" "http://127.0.0.1:8080/nao_existe.html"
run_test 4 "DELETE bloqueado em / -> 405" "405" "" "" -X DELETE "http://127.0.0.1:8080/"
run_test 5 "GET bloqueado em /upload -> 405" "405" "" "" -X GET "http://127.0.0.1:8080/upload"

echo -e "\n${CYAN}=== FASE 2: Uploads, DELETE e Transfer-Encoding Chunked ===${NC}"
rm -f ./uploads/test_upload.txt
UPLOAD_STATUS=$(curl -s -o /dev/null -w "%{http_code}" -X POST -F "file=@/tmp/test_upload.txt" "http://127.0.0.1:8080/upload")
if [[ "$UPLOAD_STATUS" == "200" || "$UPLOAD_STATUS" == "201" ]] && [ -f "./uploads/test_upload.txt" ]; then
    echo -e "  ${GREEN}[PASS]${NC} #6 - POST multipart/form-data em /upload"
    ((PASSED++))
else
    echo -e "  ${RED}[FAIL]${NC} #6 - POST multipart/form-data (HTTP ${UPLOAD_STATUS})"
    ((FAILED++))
    FAILED_TESTS+=("#6 [Upload Multipart] (HTTP ${UPLOAD_STATUS})")
fi

# Teste 7: DELETE de arquivo existente
echo "deletar" > ./uploads/to_delete.txt
DEL_STATUS=$(curl -s -o /dev/null -w "%{http_code}" -X DELETE "http://127.0.0.1:8080/upload/to_delete.txt")
if [[ "$DEL_STATUS" == "200" || "$DEL_STATUS" == "204" ]] && [ ! -f "./uploads/to_delete.txt" ]; then
    echo -e "  ${GREEN}[PASS]${NC} #7 - DELETE de recurso existente (${DEL_STATUS})"
    ((PASSED++))
else
    echo -e "  ${RED}[FAIL]${NC} #7 - DELETE de recurso existente (HTTP ${DEL_STATUS})"
    ((FAILED++))
    FAILED_TESTS+=("#7 [DELETE arquivo] (HTTP ${DEL_STATUS})")
fi

# Teste 8: DELETE de arquivo inexistente -> 404
run_test 8 "DELETE de recurso inexistente -> 404" "404" "" "" -X DELETE "http://127.0.0.1:8080/upload/fantasma.txt"

# Teste 9: Transfer-Encoding: chunked no POST
run_test 9 "POST com Transfer-Encoding: chunked" "200" "Payload em chunks!" \
    "X-Echo-Length: 18" -X POST -H "Transfer-Encoding: chunked" \
    --data "Payload em chunks!" "http://127.0.0.1:8080/cgi-bin/cgieco.py"

echo -e "\n${CYAN}=== FASE 3: Multi-Porta, Limites de Body (413) e Virtual Hosts ===${NC}"
run_test 10 "Multi-porta: GET na porta 8081" "200" "$MAIN_INDEX_SIG" "" "http://127.0.0.1:8081/"
run_test 11 "Payload 3MB > 2M na porta 8081 -> 413" "413" "" "" -X POST --data-binary "@/tmp/large_payload.bin" "http://127.0.0.1:8081/"

STATUS_8080_3M=$(curl -s -o /dev/null -w "%{http_code}" -X POST --data-binary "@/tmp/large_payload.bin" "http://127.0.0.1:8080/")
if [ "$STATUS_8080_3M" != "413" ] && [ "$STATUS_8080_3M" != "000" ]; then
    echo -e "  ${GREEN}[PASS]${NC} #12 - Payload 3MB <= 10M aceito na porta 8080 (HTTP ${STATUS_8080_3M})"
    ((PASSED++))
else
    echo -e "  ${RED}[FAIL]${NC} #12 - Payload 3MB rejeitado na porta 8080 (HTTP ${STATUS_8080_3M})"
    ((FAILED++))
    FAILED_TESTS+=("#12 [Payload 3MB na 8080] (HTTP ${STATUS_8080_3M})")
fi

run_test 13 "Virtual Host (Host: virtual.local)" "200" "$VHOST_INDEX_SIG" "" -H "Host: virtual.local" "http://127.0.0.1:8080/"
run_test 14 "Virtual Host com porta no header (Host: virtual.local:8080)" "200" "$VHOST_INDEX_SIG" "" -H "Host: virtual.local:8080" "http://127.0.0.1:8080/"
run_test 15 "Método bloqueado no Virtual Host (POST -> 405)" "405" "" "" -X POST --data "x" -H "Host: virtual.local" "http://127.0.0.1:8080/"

echo -e "\n${CYAN}=== FASE 4: Autoindex, Redirecionamentos e Protocolo HTTP/1.1 ===${NC}"
run_test 16 "Autoindex ON lista arquivos do diretório" "200" "item_alpha.txt" "" "http://127.0.0.1:8080/dir_listing/"
run_test 17 "Redirect HTTP 301 com header Location" "301" "" "Location: http://127.0.0.1:8080/index.html" "http://127.0.0.1:8080/old-page"

# Teste 18: Path Traversal não deve escapar do root (/etc/passwd)
TRAVERSAL_STATUS=$(curl --path-as-is -s -o /dev/null -w "%{http_code}" "http://127.0.0.1:8080/../../../../etc/passwd")
if [[ "$TRAVERSAL_STATUS" == "400" || "$TRAVERSAL_STATUS" == "403" || "$TRAVERSAL_STATUS" == "404" ]]; then
    echo -e "  ${GREEN}[PASS]${NC} #18 - Proteção Path Traversal (HTTP ${TRAVERSAL_STATUS})"
    ((PASSED++))
else
    echo -e "  ${RED}[FAIL]${NC} #18 - Vulnerável a Path Traversal (HTTP ${TRAVERSAL_STATUS})"
    ((FAILED++))
    FAILED_TESTS+=("#18 [Path Traversal] (HTTP ${TRAVERSAL_STATUS})")
fi

# Teste 19: Request HTTP/1.1 sem header Host -> 400 Bad Request (RFC 7230)
NO_HOST_STATUS=$(printf "GET / HTTP/1.1\r\nConnection: close\r\n\r\n" | nc -w 2 127.0.0.1 8080 | head -n 1 | awk '{print $2}')
if [ "$NO_HOST_STATUS" == "400" ]; then
    echo -e "  ${GREEN}[PASS]${NC} #19 - HTTP/1.1 sem header Host retorna 400"
    ((PASSED++))
else
    echo -e "  ${RED}[FAIL]${NC} #19 - HTTP/1.1 sem header Host (esperado 400, obtido '${NO_HOST_STATUS}')"
    ((FAILED++))
    FAILED_TESTS+=("#19 [HTTP/1.1 sem Host] (obtido '${NO_HOST_STATUS}')")
fi

# Teste 20: HTTP Keep-Alive (2 requisições sequenciais na mesma conexão TCP)
KA_COUNT=$(curl -s -o /dev/null -w "%{http_code}\n" "http://127.0.0.1:8080/" "http://127.0.0.1:8080/index.html" | grep -c "200")
if [ "$KA_COUNT" -eq 2 ]; then
    echo -e "  ${GREEN}[PASS]${NC} #20 - Conexão persistente (Keep-Alive) multi-request"
    ((PASSED++))
else
    echo -e "  ${RED}[FAIL]${NC} #20 - Falha no Keep-Alive (${KA_COUNT}/2 OK)"
    ((FAILED++))
    FAILED_TESTS+=("#20 [Keep-Alive] (${KA_COUNT}/2 OK)")
fi

echo -e "\n${CYAN}=== FASE 5: CGI & Timeout ===${NC}"
run_test 21 "CGI GET + Query String" "200" "aluno=42" "Content-Type: text/html" "http://127.0.0.1:8080/cgi-bin/cgienv.py?aluno=42"
LARGE_LEN=$(wc -c < /tmp/cgi_large_body.txt | tr -d ' ')
run_test 22 "CGI POST grande (multi-ciclo pipe)" "200" "BLOCO_DE_DADOS_CGI_" "X-Echo-Length: ${LARGE_LEN}" -X POST --data-binary "@/tmp/cgi_large_body.txt" "http://127.0.0.1:8080/cgi-bin/cgieco.py"

TIMEOUT_STATUS=$(curl -s -m 10 -o /dev/null -w "%{http_code}" "http://127.0.0.1:8080/cgi-bin/cgiinf.py")
if [[ "$TIMEOUT_STATUS" == "504" || "$TIMEOUT_STATUS" == "500" ]]; then
    echo -e "  ${GREEN}[PASS]${NC} #23 - Timeout em loop infinito CGI (${TIMEOUT_STATUS})"
    ((PASSED++))
else
    echo -e "  ${RED}[FAIL]${NC} #23 - Timeout CGI falhou (obtido ${TIMEOUT_STATUS})"
    ((FAILED++))
    FAILED_TESTS+=("#23 [Timeout CGI] (obtido ${TIMEOUT_STATUS})")
fi

echo -e "\n${CYAN}=== FASE 6: Stress & Concorrência (Non-Blocking I/O) ===${NC}"

# Teste 24: 100 requisições estáticas concorrentes (20 workers paralelos)
CONC_OK=$(seq 1 100 | xargs -P 20 -I {} curl -s -o /dev/null -w "%{http_code}\n" "http://127.0.0.1:8080/" | grep -c "^200$")
if [ "$CONC_OK" -eq 100 ]; then
    echo -e "  ${GREEN}[PASS]${NC} #24 - Stress Estático: 100/100 requests concorrentes (20 paralelos)"
    ((PASSED++))
else
    echo -e "  ${RED}[FAIL]${NC} #24 - Stress Estático: apenas ${CONC_OK}/100 responderam 200"
    ((FAILED++))
    FAILED_TESTS+=("#24 [Stress Estático 100 reqs] (${CONC_OK}/100 OK)")
fi

# Teste 25: 15 requisições CGI POST concorrentes + resiliência do servidor vivo
CGI_CONC_OK=$(seq 1 15 | xargs -P 5 -I {} curl -s -o /dev/null -w "%{http_code}\n" -X POST --data "stress_payload_{}" "http://127.0.0.1:8080/cgi-bin/cgieco.py" | grep -c "^200$")
ALIVE_AFTER=$(curl -s -o /dev/null -w "%{http_code}" "http://127.0.0.1:8080/")
if [ "$CGI_CONC_OK" -eq 15 ] && [ "$ALIVE_AFTER" == "200" ] && kill -0 "$SERVER_PID" 2>/dev/null; then
    echo -e "  ${GREEN}[PASS]${NC} #25 - Stress CGI: 15/15 POSTs paralelos + servidor íntegro"
    ((PASSED++))
else
    echo -e "  ${RED}[FAIL]${NC} #25 - Stress CGI: ${CGI_CONC_OK}/15 OK (pós-stress HTTP ${ALIVE_AFTER})"
    ((FAILED++))
    FAILED_TESTS+=("#25 [Stress CGI 15 reqs] (${CGI_CONC_OK}/15 OK)")
fi

# ------------------------------------------------------------------------------
# 4. Relatório Final
# ------------------------------------------------------------------------------
TOTAL=$((PASSED + FAILED))
echo ""
echo "========================================"
echo "RELATÓRIO COMPLETO E2E & STRESS"
echo "========================================"
echo -e "Total executados : ${TOTAL}"
echo -e "Passaram         : ${GREEN}${PASSED}${NC}"
echo -e "Falharam         : ${RED}${FAILED}${NC}"

if [ "$FAILED" -gt 0 ]; then
    echo "----------------------------------------"
    echo "Falhas detectadas:"
    for fail in "${FAILED_TESTS[@]}"; do
        echo -e "  ${RED}- ${fail}${NC}"
    done
    echo "========================================"
    exit 1
fi

echo "========================================"
exit 0