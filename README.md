DR - Device Reporter v1.0

Coletor de hardware ultra-leve, rápido e portável. Construído do zero absoluto em C99 puro, sem dependências externas.

Gera inventário completo do sistema em JSON com timestamp, pronto para ser usado como módulo de um projeto maior ou como programa independente.

O que coleta
- Placa-mãe
- Tipo de armazenamento (HD, SSD, NVMe)
- Webcam
- Bluetooth
- Outros dispositivos do sistema

Portabilidade Total
- Sistemas: Windows 7/10/11, Linux, macOS, Android via Termux, Servidores
- Arquiteturas: x86, x64, ARM, ARM64
- Compiladores: GCC, Clang, MSVC, TCC

Manual de Instalação Completo

*1. Baixar:*
`git clone https://github.com/Charleysonn/DR.git` e `cd DR`

*2. Compilar e Executar por Sistema:*

_Windows (10/11):_
`gcc DR.c -o DR.exe` depois `DR.exe`

_Linux Desktop (Ubuntu, Debian, Fedora, Arch):_
`gcc DR.c -o DR` depois `./DR`

_macOS:_
`clang DR.c -o DR` depois `./DR`

_Servidores (Windows Server / Linux Server Ubuntu, CentOS, Debian):_
Mesmo comando do Desktop, sem interface gráfica. 
Linux Server: `gcc DR.c -o DR && ./DR`
Windows Server: `gcc DR.c -o DR.exe` e roda

_Android (Termux):_
`pkg install clang` depois `clang DR.c -o DR` e `./DR`

_Raspberry Pi / ARM:_
`gcc DR.c -o DR` e `./DR` (mesmo do Linux)

Vai gerar um .json com tudo.

Desenvolvido por Charleysonn - Serra, ES
