#ifndef LOGGER_H
#define LOGGER_H

// 1. I livelli di log richiesti
typedef enum {
    LOG_DEBUG,
    LOG_WARN,
    LOG_ERROR
} LogLevel;

/**
 * @brief Configura il logger (imposta il nome del file e il livello).
 * @param nome_file Il percorso del file di log.
 * @param min_livello Il livello minimo da registrare.
 */
void log_config(const char* nome_file, LogLevel min_livello);

/**
 * @brief Funzione interna di scrittura. NON usare direttamente.
 */
void logger_scrivi(LogLevel livello, const char* file, int riga, const char* formato, ...);

/* --- MACRO PUBBLICHE --- */
// (Usare __VA_ARGS__ richiede C99 o successivo)

#define log_debug(...) logger_scrivi(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define log_warn(...)  logger_scrivi(LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...) logger_scrivi(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)


#endif // LOGGER_H