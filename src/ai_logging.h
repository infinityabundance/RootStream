#ifndef AI_LOGGING_H
#define AI_LOGGING_H

#include <stdbool.h>
#include "../include/rootstream.h"

/*
 * ============================================================================
 * AI Coding Logging Mode Module
 * ============================================================================
 * 
 * Self-contained logging module for AI-assisted development that provides
 * structured, machine-readable output with zero performance overhead when
 * disabled.
 * 
 * Features:
 *   - Toggleable via CLI flag (--ai-coding-logs[=FILE])
 *   - Toggleable via environment variable (AI_COPILOT_MODE=1)
 *   - Toggleable via API (ai_logging_set_enabled)
 *   - Structured output: [AICODING][module][tag] message
 *   - Zero overhead when disabled (macro compiles out)
 *   - Optional file output
 *   - Startup banner with warning
 * 
 * Usage:
 *   // In main.c
 *   ai_logging_init(&ctx);
 *   
 *   // In any subsystem
 *   ai_log("capture", "init: attempting DRM/KMS backend");
 *   ai_log("encode", "init: selected backend=%s", backend_name);
 *   
 *   // Shutdown (prints summary)
 *   ai_logging_shutdown(&ctx);
 * 
 * Activation:
 *   ./rootstream --ai-coding-logs
 *   ./rootstream --ai-coding-logs=/path/to/logfile
 *   AI_COPILOT_MODE=1 ./rootstream --service
 * ============================================================================
 */

/* Forward declare context type */
typedef struct rootstream_ctx rootstream_ctx_t;

/*
 * Initialize AI logging module
 * - Checks AI_COPILOT_MODE environment variable
 * - Must be called before any ai_log() calls
 * - Prints startup banner if enabled
 * 
 * @param ctx RootStream context
 */
void ai_logging_init(rootstream_ctx_t *ctx);

/*
 * Check if AI logging is enabled
 * 
 * @param ctx RootStream context
 * @return true if logging is active, false otherwise
 */
bool ai_logging_is_enabled(rootstream_ctx_t *ctx);

/*
 * Programmatically enable/disable AI logging
 * 
 * @param ctx RootStream context
 * @param enabled true to enable, false to disable
 */
void ai_logging_set_enabled(rootstream_ctx_t *ctx, bool enabled);

/*
 * Set AI logging output file
 * 
 * @param ctx RootStream context
 * @param filepath Path to log file, or NULL for stderr
 * @return 0 on success, -1 on error
 */
int ai_logging_set_output(rootstream_ctx_t *ctx, const char *filepath);

/*
 * Core logging function with structured output
 * Format: [AICODING][module][tag] message
 * 
 * @param module Module name (e.g., "capture", "encode", "network")
 * @param fmt Printf-style format string
 * @param ... Variable arguments
 */
void ai_log(const char *module, const char *fmt, ...) 
    __attribute__((format(printf, 2, 3)));

/*
 * Shutdown AI logging module
 * - Prints summary if enabled
 * - Closes log file if opened
 * 
 * @param ctx RootStream context
 */
void ai_logging_shutdown(rootstream_ctx_t *ctx);

/*
 * Convenience macros for common modules
 * Usage: AI_LOG_CAPTURE("init: DRM device=%s", path);
 */
#define AI_LOG_CAPTURE(fmt, ...) ai_log("capture", fmt, ##__VA_ARGS__)
#define AI_LOG_ENCODE(fmt, ...)  ai_log("encode", fmt, ##__VA_ARGS__)
#define AI_LOG_NETWORK(fmt, ...) ai_log("network", fmt, ##__VA_ARGS__)
#define AI_LOG_INPUT(fmt, ...)   ai_log("input", fmt, ##__VA_ARGS__)
#define AI_LOG_AUDIO(fmt, ...)   ai_log("audio", fmt, ##__VA_ARGS__)
#define AI_LOG_CRYPTO(fmt, ...)  ai_log("crypto", fmt, ##__VA_ARGS__)
#define AI_LOG_DISCOVERY(fmt, ...) ai_log("discovery", fmt, ##__VA_ARGS__)
#define AI_LOG_GUI(fmt, ...)     ai_log("gui", fmt, ##__VA_ARGS__)
#define AI_LOG_CORE(fmt, ...)    ai_log("core", fmt, ##__VA_ARGS__)

#endif /* AI_LOGGING_H */
