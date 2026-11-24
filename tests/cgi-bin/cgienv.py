#!/usr/bin/env python3
import os

print("Content-Type: text/html\r\n\r\n")
print("<html><body>")
print("<h1>Ambiente CGI Recebido:</h1>")
print("<ul>")
for key, value in os.environ.items():
    print(f"<li><strong>{key}</strong>: {value}</li>")
print("</ul>")
print("</body></html>")
