DR - Diagnóstico Rápido

Ferramenta em C puro (C99) que faz um diagnóstico completo do hardware e sistema em segundos. Roda em Windows, Linux, macOS, FreeBSD, Android (Termux) e Raspberry Pi sem dependências.

O que ele coleta
- Sistema Operacional, Uptime, IP Local
- Placa-mãe (fabricante / modelo)
- Processador e núcleos lógicos
- Memória RAM total e disponível
- Armazenamento (tipo SSD/HD, total e livre)
- Placa de vídeo, Interfaces de rede, Áudio, Webcam, Bluetooth
- Status da bateria

Saída
Cria automaticamente a pasta DR relatorio e gera um arquivo JSON com data e hora.

Como compilar
Linux / macOS:
gcc -Wall -Wextra -std=c99 DR.c -o dr
./dr

Windows:
gcc -Wall -Wextra -std=c99 DR.c -o DR.exe

Autor
Charleysonn - Versão 3.2-Hyper-Optimized
