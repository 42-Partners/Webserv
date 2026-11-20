#!/usr/bin/env python3
import sys

body = sys.stdin.read()

print("Content-Type: text/plain")
print(f"X-Echo-Length: {len(body)}\r\n")
print(f"Echo recebido com sucesso:\n{body}")
