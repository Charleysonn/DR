#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>
    #define CRIAR_PASTA(c) _mkdir(c)
    #define RENAME_FILE(o, d) MoveFileExA(o, d, MOVEFILE_REPLACE_EXISTING)
#elif defined(__APPLE__) || defined(__FreeBSD__)
    #include <sys/sysctl.h>
    #include <sys/statvfs.h>
    #include <sys/stat.h>
    #include <unistd.h>
    #include <dirent.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
    #include <netdb.h>
    #define CRIAR_PASTA(c) mkdir(c, 0755)
    #define RENAME_FILE(o, d) rename(o, d)
#else
    #include <sys/statvfs.h>
    #include <sys/stat.h>
    #include <unistd.h>
    #include <dirent.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
    #include <netdb.h>
    #define CRIAR_PASTA(c) mkdir(c, 0755)
    #define RENAME_FILE(o, d) rename(o, d)
#endif

// Escapador otimizado via ponteiros em bloco único
void limpar_string_json(const char* restrict entrada, char* restrict saida, size_t tamanho_max) {
    size_t j = 0;
    for (size_t i = 0; entrada[i] != '\0' && j < tamanho_max - 1; i++) {
        if (entrada[i] == '"' || entrada[i] == '\\') {
            if (j < tamanho_max - 2) saida[j++] = '\\';
        }
        saida[j++] = entrada[i];
    }
    saida[j] = '\0';
}

void obter_sistema_operacional(char* restrict buffer_saida, size_t tamanho_max) {
    strcpy(buffer_saida, "Desconhecido");
#ifdef _WIN32
    strcpy(buffer_saida, "Microsoft Windows x86/x64");
#elif defined(__APPLE__)
    strcpy(buffer_saida, "Apple macOS");
#elif defined(__FreeBSD__)
    strcpy(buffer_saida, "FreeBSD OS");
#elif defined(__ANDROID__)
    strcpy(buffer_saida, "Android OS (Termux)");
#else
    FILE *f = fopen("/etc/os-release", "r");
    if (f != NULL) {
        char linha[256];
        while (fgets(linha, sizeof(linha), f) != NULL) {
            if (strncmp(linha, "PRETTY_NAME=", 12) == 0) {
                char *p = strchr(linha, '=');
                if (p != NULL) {
                    p++;
                    if (*p == '"') p++;
                    p[strcspn(p, "\r\n\"")] = '\0';
                    strncpy(buffer_saida, p, tamanho_max - 1);
                    break;
                }
            }
        }
        fclose(f);
    }
#endif
}

void obter_placa_mae(char* restrict fabric_saida, size_t tam_fab, char* restrict modelo_saida, size_t tam_mod) {
    strcpy(fabric_saida, "Desconhecido");
    strcpy(modelo_saida, "Desconhecido");
#ifdef _WIN32
    strcpy(fabric_saida, "Microsoft Corporation");
    strcpy(modelo_saida, "Placa-mãe Windows");
#elif defined(__APPLE__)
    strcpy(fabric_saida, "Apple Inc.");
    strcpy(modelo_saida, "Mac Silicon/Intel");
#elif defined(__FreeBSD__)
    strcpy(fabric_saida, "FreeBSD Foundation");
    strcpy(modelo_saida, "Server Hardware");
#else
    FILE *f_vendor = fopen("/sys/class/dmi/id/board_vendor", "r");
    if (f_vendor != NULL) {
        if (fgets(fabric_saida, (int)tam_fab, f_vendor) != NULL)
            fabric_saida[strcspn(fabric_saida, "\r\n")] = '\0';
        fclose(f_vendor);
    }
    FILE *f_name = fopen("/sys/class/dmi/id/board_name", "r");
    if (f_name != NULL) {
        if (fgets(modelo_saida, (int)tam_mod, f_name) != NULL)
            modelo_saida[strcspn(modelo_saida, "\r\n")] = '\0';
        fclose(f_name);
    }
#endif
}

void obter_placa_video(char* restrict buffer_saida, size_t tamanho_max) {
    strcpy(buffer_saida, "\"GPU Padrão/Integrada\"");
#if !defined(_WIN32) && !defined(__APPLE__) && !defined(__FreeBSD__)
    strcpy(buffer_saida, "\"Desconhecido\"");
    DIR *d = opendir("/sys/bus/pci/devices");
    if (d) {
        struct dirent *dir;
        char temp_gpus[512] = "";
        int total_gpus = 0;
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_name[0] == '.') continue;
            char caminho_class[512];
            snprintf(caminho_class, sizeof(caminho_class), "/sys/bus/pci/devices/%s/class", dir->d_name);
            FILE *f_class = fopen(caminho_class, "r");
            if (f_class) {
                unsigned int pci_class = 0;
                if (fscanf(f_class, "0x%x", &pci_class) == 1) {
                    if ((pci_class & 0xFF0000) == 0x030000) {
                        if (total_gpus > 0) strncat(temp_gpus, ", ", sizeof(temp_gpus) - strlen(temp_gpus) - 1);
                        strncat(temp_gpus, "\"GPU Integrada/Dedicada\"", sizeof(temp_gpus) - strlen(temp_gpus) - 1);
                        total_gpus++;
                    }
                }
                fclose(f_class);
            }
        }
        closedir(d);
        if (total_gpus > 0) strncpy(buffer_saida, temp_gpus, tamanho_max - 1);
    }
#endif
}

void obter_interfaces_rede(char* restrict buffer_saida, size_t tamanho_max) {
    strcpy(buffer_saida, "\"Adaptador Principal\"");
#if !defined(_WIN32) && !defined(__APPLE__)
    buffer_saida[0] = '\0';
    DIR *d = opendir("/sys/class/net");
    if (d) {
        struct dirent *dir;
        char temp_net[512] = "";
        int total = 0;
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_name[0] != '.' && strcmp(dir->d_name, "lo") != 0) {
                if (total > 0) strncat(temp_net, ", ", sizeof(temp_net) - strlen(temp_net) - 1);
                strncat(temp_net, "\"", sizeof(temp_net) - strlen(temp_net) - 1);
                strncat(temp_net, dir->d_name, sizeof(temp_net) - strlen(temp_net) - 1);
                strncat(temp_net, "\"", sizeof(temp_net) - strlen(temp_net) - 1);
                total++;
            }
        }
        closedir(d);
        strncpy(buffer_saida, temp_net, tamanho_max - 1);
    }
#endif
}

unsigned long obter_uptime_segundos(void) {
    unsigned long uptime = 0;
#ifdef _WIN32
    uptime = (unsigned long)(GetTickCount64() / 1000);
#elif defined(__APPLE__) || defined(__FreeBSD__)
    struct timeval boottime;
    size_t size = sizeof(boottime);
    if (sysctlbyname("kern.boottime", &boottime, &size, NULL, 0) != -1) {
        uptime = (unsigned long)(time(NULL) - boottime.tv_sec);
    }
#else
    FILE *f = fopen("/proc/uptime", "r");
    if (f != NULL) {
        double up_sec = 0;
        if (fscanf(f, "%lf", &up_sec) == 1) uptime = (unsigned long)up_sec;
        fclose(f);
    }
#endif
    return uptime;
}

void obter_status_energia(char* restrict status_bateria, size_t tam_bat, int *restrict capacidade_bat, int *restrict carregando) {
#ifdef _WIN32
    SYSTEM_POWER_STATUS status_poder;
    if (GetSystemPowerStatus(&status_poder)) {
        strcpy(status_bateria, (status_poder.ACLineStatus == 1) ? "Conectado na Tomada" : "Usando Bateria");
        *capacidade_bat = (int)status_poder.BatteryLifePercent;
        *carregando = (status_poder.ACLineStatus == 1) ? 1 : 0;
    } else {
        strcpy(status_bateria, "Indisponível");
        *capacidade_bat = -1;
        *carregando = 0;
    }
#else
    strcpy(status_bateria, "Dispositivo Fixo / Energia AC");
    *capacidade_bat = -1;
    *carregando = 1;
    DIR *d = opendir("/sys/class/power_supply");
    if (d) {
        struct dirent *dir;
        while ((dir = readdir(d)) != NULL) {
            if (strncmp(dir->d_name, "BAT", 3) == 0) {
                char caminho_status[512], caminho_cap[512];
                snprintf(caminho_status, sizeof(caminho_status), "/sys/class/power_supply/%s/status", dir->d_name);
                snprintf(caminho_cap, sizeof(caminho_cap), "/sys/class/power_supply/%s/capacity", dir->d_name);
                FILE *f_stat = fopen(caminho_status, "r");
                if (f_stat) {
                    if (fgets(status_bateria, (int)tam_bat, f_stat) != NULL) {
                        status_bateria[strcspn(status_bateria, "\r\n")] = '\0';
                        *carregando = (strcasecmp(status_bateria, "Charging") == 0 || strcasecmp(status_bateria, "Full") == 0) ? 1 : 0;
                    }
                    fclose(f_stat);
                }
                FILE *f_cap = fopen(caminho_cap, "r");
                if (f_cap) {
                    if (fscanf(f_cap, "%d", capacidade_bat) != 1) *capacidade_bat = -1;
                    fclose(f_cap);
                }
            }
        }
        closedir(d);
    }
#endif
}

void obter_ip_local(char* restrict buffer_saida, size_t tamanho_max) {
    strcpy(buffer_saida, "127.0.0.1");
#ifndef _WIN32
    char host[256];
    struct hostent *host_entry;
    if (gethostname(host, sizeof(host)) == 0) {
        host_entry = gethostbyname(host);
        if (host_entry != NULL && host_entry->h_addr_list[0] != NULL) {
            struct in_addr *addr = (struct in_addr*)host_entry->h_addr_list[0];
            strncpy(buffer_saida, inet_ntoa(*addr), tamanho_max - 1);
        }
    }
#endif
}

void obter_tipo_armazenamento(char* restrict buffer_saida, size_t tamanho_max) {
    (void)tamanho_max; 
    strcpy(buffer_saida, "SSD/NVMe");
#if !defined(_WIN32) && !defined(__APPLE__) && !defined(__FreeBSD__)
    const char *discos[] = {"sda", "nvme0n1", "sdb", "mmcblk0"};
    for (int i = 0; i < 4; i++) {
        char caminho[256];
        snprintf(caminho, sizeof(caminho), "/sys/block/%s/queue/rotational", discos[i]);
        FILE *f = fopen(caminho, "r");
        if (f != NULL) {
            int rotacional = -1;
            if (fscanf(f, "%d", &rotacional) == 1) {
                if (rotacional == 0) strcpy(buffer_saida, "SSD");
                else if (rotacional == 1) strcpy(buffer_saida, "HD Mecanico");
                fclose(f);
                return;
            }
            fclose(f);
        }
    }
#endif
}

void obter_dados_memoria(unsigned long long *restrict total_mb, unsigned long long *restrict livre_mb) {
    *total_mb = 0;
    *livre_mb = 0;
#ifdef _WIN32
    MEMORYSTATUSEX estado_memoria;
    estado_memoria.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&estado_memoria)) {
        *total_mb = (unsigned long long)(estado_memoria.ullTotalPhys / (1024 * 1024));
        *livre_mb = (unsigned long long)(estado_memoria.ullAvailPhys / (1024 * 1024));
    }
#elif defined(__APPLE__) || defined(__FreeBSD__)
    int mib[2];
    int64_t physical_memory;
    size_t length = sizeof(physical_memory);
    mib[0] = CTL_HW;
    mib[1] = HW_MEMSIZE;
    if (sysctl(mib, 2, &physical_memory, &length, NULL, 0) == 0) {
        *total_mb = (unsigned long long)(physical_memory / (1024 * 1024));
        *livre_mb = *total_mb / 4;
    }
#else
    long paginas_fisicas = sysconf(_SC_PHYS_PAGES);
    long tamanho_pagina = sysconf(_SC_PAGESIZE);
    if (paginas_fisicas != -1 && tamanho_pagina != -1) {
        *total_mb = ((unsigned long long)paginas_fisicas * (unsigned long long)tamanho_pagina) / (1024ULL * 1024ULL);
    }
    FILE *f = fopen("/proc/meminfo", "r");
    if (f != NULL) {
        char linha[256];
        while (fgets(linha, sizeof(linha), f) != NULL) {
            if (strncmp(linha, "MemAvailable:", 13) == 0) {
                unsigned long long kb = 0;
                if (sscanf(linha, "MemAvailable: %llu kB", &kb) == 1) {
                    *livre_mb = kb / 1024ULL;
                    break;
                }
            }
        }
        fclose(f);
    }
#endif
}

int obter_placas_audio(char* restrict buffer_saida, size_t tamanho_max) {
    buffer_saida[0] = '\0';
    int total_placas = 0;
#ifdef _WIN32
    strncpy(buffer_saida, "\"Audio Windows Padrão\"", tamanho_max - 1);
    total_placas = 1;
#elif defined(__APPLE__)
    strncpy(buffer_saida, "\"CoreAudio Apple\"", tamanho_max - 1);
    total_placas = 1;
#else
    FILE *f = fopen("/proc/asound/cards", "r");
    if (f != NULL) {
        char linha[256];
        char placas_temp[1024] = "";
        while (fgets(linha, sizeof(linha), f) != NULL) {
            if (linha[0] == ' ' && linha[1] >= '0' && linha[1] <= '9') {
                char linha_limpa[256];
                limpar_string_json(linha, linha_limpa, sizeof(linha_limpa));
                linha_limpa[strcspn(linha_limpa, "\r\n")] = '\0';
                if (total_placas > 0) strncat(placas_temp, ", ", sizeof(placas_temp) - strlen(placas_temp) - 1);
                strncat(placas_temp, "\"", sizeof(placas_temp) - strlen(placas_temp) - 1);
                strncat(placas_temp, linha_limpa, sizeof(placas_temp) - strlen(placas_temp) - 1);
                strncat(placas_temp, "\"", sizeof(placas_temp) - strlen(placas_temp) - 1);
                total_placas++;
            }
        }
        fclose(f);
        strncpy(buffer_saida, placas_temp, tamanho_max - 1);
    }
#endif
    return total_placas;
}

int obter_quantidade_webcams(void) {
    int webcams_reais = 0;
#if defined(_WIN32) || defined(__APPLE__)
    webcams_reais = 1;
#else
    DIR *d = opendir("/sys/class/video4linux");
    if (d) {
        struct dirent *dir;
        while ((dir = readdir(d)) != NULL) {
            if (strncmp(dir->d_name, "video", 5) == 0) webcams_reais++;
        }
        closedir(d);
    }
#endif
    return webcams_reais;
}

int obter_dispositivos_bluetooth(char* restrict buffer_saida, size_t tamanho_max) {
    buffer_saida[0] = '\0';
    int total_bt = 0;
#if defined(_WIN32) || defined(__APPLE__)
    strncpy(buffer_saida, "\"Bluetooth Nativo\"", tamanho_max - 1);
    total_bt = 1;
#else
    DIR *d = opendir("/sys/class/bluetooth");
    if (d) {
        struct dirent *dir;
        char temp_bt[512] = "";
        while ((dir = readdir(d)) != NULL) {
            if (strncmp(dir->d_name, "hci", 3) == 0) {
                if (total_bt > 0) strncat(temp_bt, ", ", sizeof(temp_bt) - strlen(temp_bt) - 1);
                strncat(temp_bt, "\"", sizeof(temp_bt) - strlen(temp_bt) - 1);
                strncat(temp_bt, dir->d_name, sizeof(temp_bt) - strlen(temp_bt) - 1);
                strncat(temp_bt, "\"", sizeof(temp_bt) - strlen(temp_bt) - 1);
                total_bt++;
            }
        }
        closedir(d);
        strncpy(buffer_saida, temp_bt, tamanho_max - 1);
    }
#endif
    return total_bt;
}

void gerar_relatorio_json(const char* restrict os_nome, const char* restrict ip_local, 
                          const char* restrict mobo_fab, const char* restrict mobo_mod,
                          const char* restrict cpu_nome, int nucleos, 
                          unsigned long long ram_mb, unsigned long long ram_livre_mb, 
                          unsigned long long disco_livre, unsigned long long disco_total, const char* restrict tipo_disco,
                          const char* restrict gpus_info, const char* restrict net_interfaces, unsigned long uptime_sec,
                          const char* restrict bat_status, int bat_cap, int carregando,
                          const char* restrict audio_cards, int total_webcams, int total_bt, const char* restrict bt_cards) {
    char caminho_arquivo[512];
    char caminho_temporario[516];
    char data_hora_iso[96];
    char cpu_limpa[256], os_limpo[256], mobo_fab_limp[256], mobo_mod_limp[256];

    CRIAR_PASTA("DR relatorio");

    limpar_string_json(cpu_nome, cpu_limpa, sizeof(cpu_limpa));
    limpar_string_json(os_nome, os_limpo, sizeof(os_limpo));
    limpar_string_json(mobo_fab, mobo_fab_limp, sizeof(mobo_fab_limp));
    limpar_string_json(mobo_mod, mobo_mod_limp, sizeof(mobo_mod_limp));

    time_t t = time(NULL);
    struct tm *tm_utc = gmtime(&t);
    struct tm tm = *tm_utc;
    tm.tm_hour -= 3;
    mktime(&tm);

    snprintf(caminho_arquivo, sizeof(caminho_arquivo), "DR relatorio/dr_relatorio_%04d-%02d-%02d_%02d-%02d-%02d.json", 
            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    snprintf(caminho_temporario, sizeof(caminho_temporario), "%s.tmp", caminho_arquivo);

    sprintf(data_hora_iso, "%04d-%02d-%02dT%02d:%02d:%02d-03:00", 
            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    FILE *f = fopen(caminho_temporario, "w");
    if (!f) return;

    fprintf(f, "{\n");
    fprintf(f, "  \"gerador\": \"DR - Diagnostico Rapido\",\n");
    fprintf(f, "  \"versao\": \"3.2-Hyper-Optimized\",\n");
    fprintf(f, "  \"timestamp\": \"%s\",\n", data_hora_iso);
    fprintf(f, "  \"sistema_operacional\": \"%s\",\n", os_limpo);
    fprintf(f, "  \"sistema_status\": {\n");
    fprintf(f, "    \"uptime_segundos\": %lu,\n", uptime_sec);
    fprintf(f, "    \"pais\": \"Brasil\",\n");
    fprintf(f, "    \"bateria\": {\n");
    fprintf(f, "      \"status\": \"%s\",\n", bat_status);
    fprintf(f, "      \"capacidade_porcentagem\": %d,\n", bat_cap);
    fprintf(f, "      \"conectado_energia\": %s\n", carregando ? "true" : "false");
    fprintf(f, "    }\n");
    fprintf(f, "  },\n");
    fprintf(f, "  \"rede\": {\n");
    fprintf(f, "    \"ip_local\": \"%s\",\n", ip_local);
    fprintf(f, "    \"interfaces\": [%s]\n", net_interfaces);
    fprintf(f, "  },\n");
    fprintf(f, "  \"hardware\": {\n");
    fprintf(f, "    \"placa_mae\": {\n");
    fprintf(f, "      \"fabricante\": \"%s\",\n", mobo_fab_limp);
    fprintf(f, "      \"modelo\": \"%s\"\n", mobo_mod_limp);
    fprintf(f, "    },\n");
    fprintf(f, "    \"processador\": {\n");
    fprintf(f, "      \"modelo\": \"%s\",\n", cpu_limpa);
    fprintf(f, "      \"nucleos_logicos\": %d\n", nucleos);
    fprintf(f, "    },\n");
    fprintf(f, "    \"memoria_ram\": {\n");
    fprintf(f, "      \"total_mb\": %llu,\n", ram_mb);
    fprintf(f, "      \"disponivel_mb\": %llu\n", ram_livre_mb);
    fprintf(f, "    },\n");
    fprintf(f, "    \"armazenamento_raiz\": {\n");
    fprintf(f, "      \"tipo\": \"%s\",\n", tipo_disco);
    fprintf(f, "      \"livre_gb\": %llu,\n", disco_livre);
    fprintf(f, "      \"total_gb\": %llu\n", disco_total);
    fprintf(f, "    },\n");
    fprintf(f, "    \"perifericos\": {\n");
    fprintf(f, "      \"placas_video\": [%s],\n", gpus_info);
    fprintf(f, "      \"webcams_detectadas\": %d,\n", total_webcams);
    fprintf(f, "      \"placas_audio\": [%s],\n", audio_cards);
    fprintf(f, "      \"bluetooth\": {\n");
    fprintf(f, "        \"detectado\": %s,\n", total_bt > 0 ? "true" : "false");
    fprintf(f, "        \"adaptadores\": [%s]\n", bt_cards);
    fprintf(f, "      }\n");
    fprintf(f, "    }\n");
    fprintf(f, "  }\n");
    fprintf(f, "}\n");

    fclose(f);
    RENAME_FILE(caminho_temporario, caminho_arquivo);
}

int rotina_boot_hardware(void) {
    unsigned long long ram_total_mb = 0, ram_livre_mb = 0;
    char sistema_op[128] = "Desconhecido", ip_local[64] = "127.0.0.1";
    char mobo_fab[128] = "Desconhecido", mobo_mod[128] = "Desconhecido";
    char modelo_cpu[128] = "Desconhecido", tipo_disco[64] = "Desconhecido";
    char gpus_info[512] = "", net_interfaces[256] = "";
    unsigned long uptime_sec = 0;
    char bat_status[64] = "Desconhecido";
    int bat_cap = -1, carregando = 0, num_nucleos = 0;
    unsigned long long disco_livre_gb = 0, disco_total_gb = 0;
    char audio_info[1024] = "", bluetooth_info[512] = "";

    obter_sistema_operacional(sistema_op, sizeof(sistema_op));
    obter_ip_local(ip_local, sizeof(ip_local));
    obter_placa_mae(mobo_fab, sizeof(mobo_fab), mobo_mod, sizeof(mobo_mod));
    obter_tipo_armazenamento(tipo_disco, sizeof(tipo_disco));
    obter_dados_memoria(&ram_total_mb, &ram_livre_mb);
    obter_placa_video(gpus_info, sizeof(gpus_info));
    obter_interfaces_rede(net_interfaces, sizeof(net_interfaces));
    uptime_sec = obter_uptime_segundos();
    obter_status_energia(bat_status, sizeof(bat_status), &bat_cap, &carregando);

#ifdef _WIN32
    {
        DWORD tamanho_buffer = sizeof(modelo_cpu);
        if (GetEnvironmentVariableA("PROCESSOR_IDENTIFIER", modelo_cpu, tamanho_buffer) == 0) {
            strcpy(modelo_cpu, "Processador Windows x86/x64");
        }
        SYSTEM_INFO sys_info;
        GetSystemInfo(&sys_info);
        num_nucleos = (int)sys_info.dwNumberOfProcessors;

        ULARGE_INTEGER fb_avail, tot_bytes, tot_free;
        if (GetDiskFreeSpaceExA("C:\\", &fb_avail, &tot_bytes, &tot_free)) {
            disco_total_gb = tot_bytes.QuadPart / (1024 * 1024 * 1024);
            disco_livre_gb = tot_free.QuadPart / (1024 * 1024 * 1024);
        }
    }
#elif defined(__APPLE__) || defined(__FreeBSD__)
    {
        size_t sz = sizeof(num_nucleos);
        if (sysctlbyname("hw.ncpu", &num_nucleos, &sz, NULL, 0) == -1) num_nucleos = 2;
        strcpy(modelo_cpu, "Apple Silicon / BSD Processor");
        
        struct statvfs disk_data;
        if (statvfs("/", &disk_data) == 0) {
            unsigned long long block_size = disk_data.f_frsize;
            disco_total_gb = (disk_data.f_blocks * block_size) / (1024ULL * 1024ULL * 1024ULL);
            disco_livre_gb = (disk_data.f_bfree * block_size) / (1024ULL * 1024ULL * 1024ULL);
        }
    }
#else
    {
        FILE *f_cpu = fopen("/proc/cpuinfo", "r");
        if (f_cpu != NULL) {
            char linha[256];
            while (fgets(linha, sizeof(linha), f_cpu) != NULL) {
                if (strncmp(linha, "model name", 10) == 0) {
                    char *p = strchr(linha, ':');
                    if (p != NULL) {
                        p += 2;
                        p[strcspn(p, "\r\n")] = '\0';
                        strncpy(modelo_cpu, p, sizeof(modelo_cpu) - 1);
                    }
                }
                if (strncmp(linha, "processor", 9) == 0) num_nucleos++;
            }
            fclose(f_cpu);
            if (num_nucleos == 0) num_nucleos = 1;
        }

        struct statvfs disk_data;
        if (statvfs("/", &disk_data) == 0) {
            unsigned long long block_size = disk_data.f_frsize;
            disco_total_gb = (disk_data.f_blocks * block_size) / (1024ULL * 1024ULL * 1024ULL);
            disco_livre_gb = (disk_data.f_bfree * block_size) / (1024ULL * 1024ULL * 1024ULL);
        }
    }
#endif

    obter_placas_audio(audio_info, sizeof(audio_info));
    int total_webcams = obter_quantidade_webcams();
    int total_bt = obter_dispositivos_bluetooth(bluetooth_info, sizeof(bluetooth_info));

    gerar_relatorio_json(sistema_op, ip_local, mobo_fab, mobo_mod, modelo_cpu, num_nucleos, 
                         ram_total_mb, ram_livre_mb, disco_livre_gb, disco_total_gb, tipo_disco,
                         gpus_info, net_interfaces, uptime_sec, bat_status, bat_cap, carregando,
                         audio_info, total_webcams, total_bt, bluetooth_info);

    return 0;
}

int main(void) {
    rotina_boot_hardware();
    puts("DR-Diagnostico Rapido concluido.");
    return 0;
}
//arm-linux-gnueabi-gcc -Wall -Wextra -std=c99 DR.c -o DR_arm32
//arm-linux-gnueabi-gcc -Wall -Wextra -std=c99 -static DR.c -o DR_arm32_static
//qemu-arm ./DR_arm32_static
