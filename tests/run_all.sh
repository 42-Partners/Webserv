#!/usr/bin/env bash
# Roda todos os scripts de teste de tests/request/ contra o servidor.
#
# Uso: ./tests/run_all.sh                  inicia o servidor com test.conf e roda tudo
#      ./tests/run_all.sh status redirect  roda so esses (nome sem o .sh)
#      ./tests/run_all.sh --no-server      usa um servidor que ja esta rodando
#
# Variaveis: HOST, PORT, SERVER_BIN, CONF. As demais (BIG_MB, ...) passam
# direto para os scripts. Codigo de saida: 0 = tudo ok, 1 = alguma falha,
# 2 = nao conseguiu preparar o ambiente (servidor, config, script).

set -u

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT" || exit 2

export HOST="${HOST:-localhost}"
export PORT="${PORT:-8080}"
SERVER_BIN="${SERVER_BIN:-./webserv}"
CONF="${CONF:-tests/request/test.conf}"
TEST_DIR="tests/request"
LOG="${TMPDIR:-/tmp}/webserv_tests_server.log"

GREEN=$'\033[32m'; RED=$'\033[31m'; YELLOW=$'\033[33m'; RESET=$'\033[0m'

START_SERVER=1
NAMES=()
for arg in "$@"; do
	case "$arg" in
		--no-server) START_SERVER=0 ;;
		-h|--help)   sed -n '2,9p' "$0"; exit 0 ;;
		*)           NAMES+=("${arg%.sh}") ;;
	esac
done

port_open() { ( exec 3<>"/dev/tcp/$HOST/$PORT" ) 2>/dev/null; }

SERVER_PID=""
cleanup() {
	if [ -n "$SERVER_PID" ] && kill -0 "$SERVER_PID" 2>/dev/null; then
		kill "$SERVER_PID" 2>/dev/null
		wait "$SERVER_PID" 2>/dev/null
	fi
}
trap cleanup EXIT
trap 'exit 130' INT TERM

# ---------- scripts a rodar ----------
SCRIPTS=()
if [ "${#NAMES[@]}" -gt 0 ]; then
	for n in "${NAMES[@]}"; do
		f="$TEST_DIR/$n.sh"
		[ -f "$f" ] || { echo "Script nao encontrado: $f"; exit 2; }
		SCRIPTS+=("$f")
	done
else
	for f in "$TEST_DIR"/*.sh; do
		[ -e "$f" ] && SCRIPTS+=("$f")
	done
fi
[ "${#SCRIPTS[@]}" -gt 0 ] || { echo "Nenhum script em $TEST_DIR/"; exit 2; }

# ---------- servidor ----------
if [ "$START_SERVER" -eq 1 ]; then
	[ -x "$SERVER_BIN" ] || { echo "Binario '$SERVER_BIN' nao encontrado (rode make)."; exit 2; }
	[ -f "$CONF" ]       || { echo "Config '$CONF' nao encontrada."; exit 2; }
	if port_open; then
		echo "Ja existe algo escutando em $HOST:$PORT."
		echo "Pare esse servidor, ou use --no-server para testa-lo."
		exit 2
	fi
	"$SERVER_BIN" "$CONF" >"$LOG" 2>&1 &
	SERVER_PID=$!
	for _ in $(seq 1 50); do
		port_open && break
		if ! kill -0 "$SERVER_PID" 2>/dev/null; then
			echo "O servidor terminou ao iniciar. Ultimas linhas do log ($LOG):"
			tail -n 20 "$LOG"
			SERVER_PID=""
			exit 2
		fi
		sleep 0.1
	done
	port_open || { echo "O servidor nao abriu $HOST:$PORT em 5s. Log ($LOG):"; tail -n 20 "$LOG"; exit 2; }
	echo "${YELLOW}[INFO]${RESET} servidor iniciado (pid $SERVER_PID, config $CONF, log $LOG)"
else
	port_open || { echo "Nenhum servidor escutando em $HOST:$PORT (--no-server)."; exit 2; }
fi

# ---------- execucao ----------
RESULTS=()
FAILED=0
for f in "${SCRIPTS[@]}"; do
	name="$(basename "$f" .sh)"
	echo
	echo "==================== $name ===================="
	start=$SECONDS
	bash "$f"
	code=$?
	elapsed=$((SECONDS - start))

	if [ "$code" -eq 0 ]; then
		RESULTS+=("${GREEN}[OK]${RESET}   $name (${elapsed}s)")
	else
		RESULTS+=("${RED}[FAIL]${RESET} $name (saida $code, ${elapsed}s)")
		FAILED=$((FAILED + 1))
	fi

	# se o servidor caiu, os proximos testes nao significam nada
	if [ -n "$SERVER_PID" ] && ! kill -0 "$SERVER_PID" 2>/dev/null; then
		wait "$SERVER_PID" 2>/dev/null
		scode=$?
		SERVER_PID=""
		RESULTS+=("${RED}[FAIL]${RESET} o servidor caiu durante '$name' (saida $scode; 139 = segfault)")
		FAILED=$((FAILED + 1))
		echo
		echo "Ultimas linhas do log do servidor ($LOG):"
		tail -n 20 "$LOG"
		break
	fi
done

# ---------- resumo ----------
echo
echo "==================== Resumo ===================="
for r in "${RESULTS[@]}"; do echo "$r"; done
echo
if [ "$FAILED" -eq 0 ]; then
	echo "${GREEN}Tudo passou${RESET} (${#SCRIPTS[@]} scripts)."
else
	echo "${RED}$FAILED falha(s)${RESET}.${SERVER_PID:+ Log do servidor: $LOG}"
fi
[ "$FAILED" -eq 0 ]