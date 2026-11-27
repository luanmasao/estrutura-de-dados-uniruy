# Estrutura de Dados Uniruy Wyden 

**Filtragem de ruído em sensores com janela deslizante**

Trabalho avaliativo da disciplina **Estrutura de Dados**, do curso de Ciência da Computação do Centro Universitário UniRuy Wyden.

- **Aluno:** Luan Masao Hirahata
- **Docente:** Prof. Heleno Cardoso

O trabalho responde à pergunta: *"Dado um problema real, qual Estrutura de Dados é mais adequada para solucioná-lo e por quê?"*

## Como compilar e executar

O programa usa apenas a biblioteca padrão do C (C99).

**Linux / macOS**

```bash
gcc -Wall -Wextra monitor_sensor_embarcado.c -o monitor_sensor
./monitor_sensor
```

**Windows (GCC do MSYS2, no PowerShell)**

```powershell
$env:Path = "C:\msys64\ucrt64\bin;" + $env:Path
gcc -Wall -Wextra monitor_sensor_embarcado.c -o monitor_sensor.exe
.\monitor_sensor.exe
```

A primeira linha coloca a pasta do GCC no PATH. Sem ela, o GCC não encontra as próprias DLLs e a compilação falha sem mostrar nenhuma mensagem.

Também é possível compilar em IDEs de C, como Dev-C++, Code::Blocks ou VS Code. O código exige o padrão **C99 ou mais recente**. Em IDEs com compilador antigo (por exemplo, o Dev-C++ 5.11, que traz o GCC 4.9), adicione `-std=c99` às opções do compilador. Sem isso, aparece o erro *"'for' loop initial declarations are only allowed in C99 mode"*.

