#!/usr/bin/env python3
import sys

# Lê todo o stdin enviado pelo servidor (mesmo que seja grande)
body = sys.stdin.read()

print("Content-Type: text/plain")
print(f"X-Echo-Length: {len(body)}\r\n")
print(f"Echo recebido com sucesso:\n{body}")