#include "logger.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

// --- Variabili Globali Statiche ---
// Queste variabili memorizzano la configurazione
static const char* g_nome_file = NULL;
static LogLevel g_min_level = LOG_DEBUG;

// Array di stringhe per i nomi dei livelli
static const char* nomi_livelli[] = {
    "DEBUG", "WARN", "ERROR"
};

// --- Funzioni Pubbliche ---

void log_config(const char* nome_file, LogLevel min_livello) {
    g_nome_file = nome_file;
    g_min_level = min_livello;
}

void logger_scrivi(LogLevel livello, const char* file, int riga, const char* formato, ...) {
    
    // 1. Filtra per livello
    if (livello < g_min_level) {
        return;
    }

    // 2. Controlla se il nome del file è stato impostato
    if (g_nome_file == NULL) {
        // Non configurato, non fare nulla
        return;
    }

    // 3. Apri il file in modalità "append" (aggiungi alla fine)
    FILE* f = fopen(g_nome_file, "a");
    if (f == NULL) {
        // Impossibile aprire il file, non possiamo fare nulla
        return;
    }

    // 4. Ottieni il timestamp
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char time_buf[20]; // "YYYY-MM-DD HH:MM:SS"
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);

    // 5. Formatta l'intero messaggio in un buffer
    // (Questa è la parte chiave per la sicurezza multi-processo)
    char buffer[1024]; // Buffer per la linea di log
    char msg_buffer[768]; // Buffer per il messaggio dell'utente

    // Formatta il messaggio dell'utente
    va_list args;
    va_start(args, formato);
    vsnprintf(msg_buffer, sizeof(msg_buffer), formato, args);
    va_end(args);

    // Formatta l'intera linea di log
    snprintf(buffer, sizeof(buffer), "[%s] [%-5s] [%s:%d] %s\n",
             time_buf,
             nomi_livelli[livello],
             file, // Nota: non usiamo basename() per semplicità
             riga,
             msg_buffer);

    // 6. Scrivi la linea formattata sul file in UNA SOLA OPERAZIONE
    fprintf(f, "%s", buffer);

    // 7. Chiudi il file
    fclose(f);
}