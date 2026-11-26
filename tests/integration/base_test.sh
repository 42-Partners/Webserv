#!/usr/bin/env bash

# ==============================================================================
# Suite de Testes End-to-End (E2E) - Webserv
# ==============================================================================

# Teste #1 (GET / na porta 8080): Valida se o servidor resolve o index.html padrão da rota raiz e responde 200 OK.
# Teste #2 (GET /index.html na porta 8080): Valida o acesso direto a um arquivo estático específico, esperando 200 OK.
# Teste #3 (GET /arquivo_que_nao_existe.html): Testa se o servidor retorna 404 Not Found e serve o corpo configurado na error_page customizada ("Custom 404").
# Teste #4 (DELETE / na porta 8080): Verifica se o bloqueio de métodos funciona na rota raiz (onde apenas GET e POST são permitidos), retornando 405 Method Not Allowed.
# Teste #5 (GET /upload): Verifica a restrição de métodos na rota de upload (onde apenas POST é permitido), exigindo 405 Method Not Allowed.
# Teste #6 (POST /upload multipart): Envia um arquivo real (test_upload.txt), valida se o status é 200 ou 201 e confirma se o arquivo foi fisicamente salvo em ./uploads/.
# Teste #7 (GET / na porta 8081): Valida o suporte a múltiplas portas simultâneas (listen 8081), esperando 200 OK.
# Teste #8 (POST de 3MB na porta 8081): Testa a diretiva client_max_body_size 2M, exigindo que o excesso de tamanho seja rejeitado com 413 Payload Too Large.
# Teste #9 (POST de 3MB na porta 8080): Garante que o limite maior (client_max_body_size 10M) da porta 8080 aceita o mesmo payload de 3MB sem derrubar a conexão (000) nem retornar 413.
# Teste #10 (GET / com Host: virtual.local): Testa o roteamento de Virtual Host na porta 8080, verificando se o servidor serve o conteúdo de ./www/virtual/index.html ("Virtual Host Index").
# Teste #11 (DELETE / com Host: virtual.local): Valida se o Virtual Host bloqueia métodos não permitidos com 405 Method Not Allowed.


BIN="./webserv"
SERVER_PID=""
PASSED=0
FAILED=0
FAILED_TESTS=()

# Cores para output limpo
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

# Função de limpeza garantida (mata o servidor e remove arquivos temporários de teste)
cleanup() {
    if [ -n "$SERVER_PID" ] && kill -0 "$SERVER_PID" 2>/dev/null; then
        kill "$SERVER_PID" 2>/dev/null
        wait "$SERVER_PID" 2>/dev/null
    fi
    rm -f /tmp/test_upload.txt /tmp/large_payload.bin /tmp/curl_body.out
}
trap cleanup EXIT

# ------------------------------------------------------------------------------
# 1. Preparação do Ambiente de Arquivos Esperado pelo Config
# ------------------------------------------------------------------------------
if [ ! -x "$BIN" ]; then
    echo -e "${RED}Erro: Binário não encontrado ou sem permissão de execução em $BIN${NC}"
    exit 1
fi

mkdir -p ./www/virtual ./uploads

# Cria arquivos base caso não existam para garantir determinismo nos testes
[ ! -f ./www/index.html ] && echo "<h1>Index Principal</h1>" > ./www/index.html
[ ! -f ./www/404.html ] && echo "<h1>Custom 404 Page</h1>" > ./www/404.html
[ ! -f ./www/virtual/index.html ] && echo "<h1>Virtual Host Index</h1>" > ./www/virtual/index.html
VHOST_INDEX_CONTENT=$(cat ./www/virtual/index.html)

# Arquivos temporários para payload de upload e limite de tamanho
echo "Conteudo de teste para upload multipart" > /tmp/test_upload.txt
dd if=/dev/zero of=/tmp/large_payload.bin bs=1M count=3 2>/dev/null # 3MB (> 2M da porta 8081)

# ------------------------------------------------------------------------------
# 2. Inicialização do Webserv (sem parâmetros)
# ------------------------------------------------------------------------------
$BIN > /dev/null 2>&1 &
SERVER_PID=$!

# Aguarda até 3 segundos para as portas 8080 e 8081 abrirem
for i in {1..30}; do
    if curl -s http://127.0.0.1:8080/ > /dev/null && curl -s http://127.0.0.1:8081/ > /dev/null; then
        break
    fi
    sleep 0.1
done

# ------------------------------------------------------------------------------
# 3. Helper de Validação
# ------------------------------------------------------------------------------
# Uso: run_test <NUM> <EXPECTED_STATUS> <EXPECTED_BODY_SUBSTRING> <CURL_ARGS...>
run_test() {
    local test_num="$1"
    local expected_status="$2"
    local expected_body="$3"
    shift 3

    local status_code
    status_code=$(curl -s -o /tmp/curl_body.out -w "%{http_code}" "$@")
    local body
    body=$(cat /tmp/curl_body.out)

    local ok=1
    if [ "$status_code" != "$expected_status" ]; then
        ok=0
    fi
    if [ -n "$expected_body" ] && [[ "$body" != *"$expected_body"* ]]; then
        ok=0
    fi

    if [ "$ok" -eq 1 ]; then
        ((PASSED++))
    else
        ((FAILED++))
        FAILED_TESTS+=("#${test_num} (esperado HTTP ${expected_status}, obtido ${status_code})")
    fi
}

# ------------------------------------------------------------------------------
# 4. Execução dos Cenários de Teste
# ------------------------------------------------------------------------------

# Teste 1: GET / na porta 8080 (Servidor 1 - index padrão)
run_test 1 "200" "" "http://127.0.0.1:8080/"

# Teste 2: GET explicitamente em /index.html na porta 8080
run_test 2 "200" "" "http://127.0.0.1:8080/index.html"

# Teste 3: GET em rota inexistente -> 404 com error_page customizada
run_test 3 "404" "Custom 404" "http://127.0.0.1:8080/arquivo_que_nao_existe.html"

# Teste 4: Método não permitido (DELETE em / onde só GET e POST são permitidos) -> 405
run_test 4 "405" "" -X DELETE "http://127.0.0.1:8080/"

# Teste 5: Método não permitido na rota /upload (GET onde só POST é permitido) -> 405
run_test 5 "405" "" -X GET "http://127.0.0.1:8080/upload"

# Teste 6: Upload de arquivo via POST multipart/form-data em /upload -> 201 (ou 200)
# Verifica se o status é 200 ou 201 e se o arquivo caiu em ./uploads
rm -f ./uploads/test_upload.txt
UPLOAD_STATUS=$(curl -s -o /dev/null -w "%{http_code}" -X POST -F "file=@/tmp/test_upload.txt" "http://127.0.0.1:8080/upload")
if [[ "$UPLOAD_STATUS" == "200" || "$UPLOAD_STATUS" == "201" ]] && [ -f "./uploads/test_upload.txt" ]; then
    ((PASSED++))
else
    ((FAILED++))
    FAILED_TESTS+=("#6 (Upload falhou - HTTP ${UPLOAD_STATUS} ou arquivo ausente em ./uploads)")
fi

# Teste 7: Multi-porta -> GET na porta 8081 (Servidor 2)
run_test 7 "200" "" "http://127.0.0.1:8081/"

# Teste 8: Limite de corpo (client_max_body_size 2M na porta 8081) -> Payload de 3MB deve retornar 413
run_test 8 "413" "" -X POST --data-binary "@/tmp/large_payload.bin" "http://127.0.0.1:8081/"

# Teste 9: Mesmo payload de 3MB na porta 8080 (onde limite é 10M) -> Não deve retornar 413
STATUS_8080_3M=$(curl -s -o /dev/null -w "%{http_code}" -X POST --data-binary "@/tmp/large_payload.bin" "http://127.0.0.1:8080/")
if [ "$STATUS_8080_3M" != "413" ] && [ "$STATUS_8080_3M" != "000" ]; then
    ((PASSED++))
else
    ((FAILED++))
    FAILED_TESTS+=("#9 (Payload de 3MB rejeitado indevidamente na porta 8080 - HTTP ${STATUS_8080_3M})")
fi

# Teste 10: Virtual Host na porta 8080 (Host: virtual.local -> root ./www/virtual)
run_test 10 "200" "$VHOST_INDEX_CONTENT" -H "Host: virtual.local" "http://127.0.0.1:8080/"

# Teste 11: Método não configurado no Virtual Host (POST onde allow_methods não foi definido)
# Nota: Dependendo da regra padrão do seu servidor quando allow_methods é omitido, ajuste se necessário (405 se restrito)
run_test 11 "405" "" -X DELETE -H "Host: virtual.local" "http://127.0.0.1:8080/"

# ------------------------------------------------------------------------------
# 5. Relatório Resumido Final
# ------------------------------------------------------------------------------
TOTAL=$((PASSED + FAILED))
echo "========================================"
echo "RELATÓRIO DE TESTES E2E"
echo "========================================"
echo -e "Total executados : ${TOTAL}"
echo -e "Passaram         : ${GREEN}${PASSED}${NC}"
echo -e "Falharam         : ${RED}${FAILED}${NC}"

if [ "$FAILED" -gt 0 ]; then
    echo "----------------------------------------"
    echo "Testes que falharam:"
    for fail in "${FAILED_TESTS[@]}"; do
        echo -e "  ${RED}- Teste ${fail}${NC}"
    done
    echo "========================================"
    exit 1
fi

echo "========================================"
exit 0
