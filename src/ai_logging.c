#include "ai_logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

/* Internal state */
typedef struct {
    bool enabled;
    FILE *output;
    bool owns_file;
    uint64_t log_count;
} ai_logging_state_t;

static ai_logging_state_t g_ai_logging = {
    .enabled = false,
    .output = NULL,
    .owns_file = false,
    .log_count = 0
};

void ai_logging_init(rootstream_ctx_t *ctx) {
    /* Check environment variable first */
    const char *copilot_mode = getenv("AI_COPILOT_MODE");
    if (copilot_mode && (strcmp(copilot_mode, "1") == 0 || 
                         strcmp(copilot_mode, "true") == 0 ||
                         strcmp(copilot_mode, "TRUE") == 0)) {
        g_ai_logging.enabled = true;
    }
    
    /* Default to stderr */
    if (g_ai_logging.enabled) {
        g_ai_logging.output = stderr;
        g_ai_logging.owns_file = false;
        g_ai_logging.log_count = 0;
        
        /* Print startup banner */
        fprintf(stderr, "\n");
        fprintf(stderr, "╔═══════════════════════════════════════════════════════════════════╗\n");
        fprintf(stderr, "║          AI CODING LOGGING MODE ENABLED                           ║\n");
        fprintf(stderr, "╠═══════════════════════════════════════════════════════════════════╣\n");
        fprintf(stderr, "║  Verbose structured logging active for AI-assisted development    ║\n");
        fprintf(stderr, "║  Output format: [AICODING][module][tag] message                   ║\n");
        fprintf(stderr, "║                                                                   ║\n");
        fprintf(stderr, "║  To disable: AI_COPILOT_MODE=0 or remove --ai-coding-logs flag   ║\n");
        fprintf(stderr, "╚═══════════════════════════════════════════════════════════════════╝\n");
        fprintf(stderr, "\n");
        fflush(stderr);
        
        ai_log("core", "init: AI logging module initialized (mode=stderr)");
    }
}

bool ai_logging_is_enabled(rootstream_ctx_t *ctx) {
    (void)ctx; /* Unused for now, reserved for future per-context state */
    return g_ai_logging.enabled;
}

void ai_logging_set_enabled(rootstream_ctx_t *ctx, bool enabled) {
    (void)ctx; /* Unused for now */
    
    if (enabled && !g_ai_logging.enabled) {
        /* Enabling */
        g_ai_logging.enabled = true;
        if (!g_ai_logging.output) {
            g_ai_logging.output = stderr;
            g_ai_logging.owns_file = false;
        }
        ai_log("core", "config: AI logging enabled programmatically");
    } else if (!enabled && g_ai_logging.enabled) {
        /* Disabling */
        ai_log("core", "config: AI logging disabled programmatically");
        g_ai_logging.enabled = false;
    }
}

int ai_logging_set_output(rootstream_ctx_t *ctx, const char *filepath) {
    (void)ctx; /* Unused for now */
    
    if (!filepath) {
        /* Switch back to stderr */
        if (g_ai_logging.owns_file && g_ai_logging.output) {
            fclose(g_ai_logging.output);
        }
        g_ai_logging.output = stderr;
        g_ai_logging.owns_file = false;
        ai_log("core", "config: output switched to stderr");
        return 0;
    }
    
    /* Open new file */
    FILE *f = fopen(filepath, "a");
    if (!f) {
        fprintf(stderr, "ERROR: Failed to open AI log file: %s\n", filepath);
        return -1;
    }
    
    /* Close old file if we own it */
    if (g_ai_logging.owns_file && g_ai_logging.output) {
        fclose(g_ai_logging.output);
    }
    
    g_ai_logging.output = f;
    g_ai_logging.owns_file = true;
    
    ai_log("core", "config: output redirected to file=%s", filepath);
    return 0;
}

void ai_log(const char *module, const char *fmt, ...) {
    if (!g_ai_logging.enabled || !g_ai_logging.output) {
        return;
    }
    
    /* Get current timestamp */
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    /* Print structured prefix */
    fprintf(g_ai_logging.output, "[AICODING][%s][%s] ", 
            timestamp, module);
    
    /* Print formatted message */
    va_list args;
    va_start(args, fmt);
    vfprintf(g_ai_logging.output, fmt, args);
    va_end(args);
    
    fprintf(g_ai_logging.output, "\n");
    fflush(g_ai_logging.output);
    
    g_ai_logging.log_count++;
}

void ai_logging_shutdown(rootstream_ctx_t *ctx) {
    (void)ctx; /* Unused for now */
    
    if (g_ai_logging.enabled) {
        ai_log("core", "shutdown: AI logging module terminating (total_logs=%lu)", 
               (unsigned long)g_ai_logging.log_count);
        
        /* Print summary */
        if (g_ai_logging.output) {
            fprintf(g_ai_logging.output, "\n");
            fprintf(g_ai_logging.output, "╔═══════════════════════════════════════════════════════════════════╗\n");
            fprintf(g_ai_logging.output, "║          AI CODING LOGGING SESSION SUMMARY                        ║\n");
            fprintf(g_ai_logging.output, "╠═══════════════════════════════════════════════════════════════════╣\n");
            fprintf(g_ai_logging.output, "║  Total log entries: %-46lu║\n", (unsigned long)g_ai_logging.log_count);
            fprintf(g_ai_logging.output, "║  Output destination: %-43s║\n", 
                    g_ai_logging.owns_file ? "file" : "stderr");
            fprintf(g_ai_logging.output, "╚═══════════════════════════════════════════════════════════════════╝\n");
            fprintf(g_ai_logging.output, "\n");
            fflush(g_ai_logging.output);
        }
        
        /* Close file if we own it */
        if (g_ai_logging.owns_file && g_ai_logging.output) {
            fclose(g_ai_logging.output);
        }
        
        /* Reset state */
        g_ai_logging.enabled = false;
        g_ai_logging.output = NULL;
        g_ai_logging.owns_file = false;
        g_ai_logging.log_count = 0;
    }
}
