/**
 * PHASE 19: Web Dashboard - Authentication Manager Implementation
 */

#include "auth_manager.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>

#define MAX_USERS 100
#define MAX_SESSIONS 1000
#define TOKEN_EXPIRY_SECONDS 86400  // 24 hours

typedef struct {
    char username[256];
    char password_hash[256];
    user_role_t role;
    bool is_active;
} user_entry_t;

typedef struct {
    char token[512];
    char username[256];
    user_role_t role;
    uint64_t expires_at;
} session_entry_t;

struct auth_manager {
    user_entry_t users[MAX_USERS];
    int user_count;
    session_entry_t sessions[MAX_SESSIONS];
    int session_count;
    pthread_mutex_t lock;
};

/**
 * Simple hash function (for demonstration)
 * In production, use bcrypt or similar
 */
static void simple_hash(const char *input, char *output, size_t output_size) {
    unsigned long hash = 5381;
    int c;
    
    while ((c = *input++)) {
        hash = ((hash << 5) + hash) + c;
    }
    
    snprintf(output, output_size, "%lx", hash);
}

/**
 * Generate token
 */
static void generate_token(const char *username, user_role_t role, char *token, size_t token_size) {
    time_t now = time(NULL);
    snprintf(token, token_size, "%s_%d_%ld_%ld", username, role, now, (long)rand());
}

/**
 * Initialize authentication manager
 */
auth_manager_t *auth_manager_init(void) {
    auth_manager_t *auth = (auth_manager_t *)calloc(1, sizeof(auth_manager_t));
    if (!auth) {
        return NULL;
    }

    pthread_mutex_init(&auth->lock, NULL);
    auth->user_count = 0;
    auth->session_count = 0;

    // Add default admin user
    auth_manager_add_user(auth, "admin", "admin", ROLE_ADMIN);

    return auth;
}

/**
 * Add user
 */
int auth_manager_add_user(auth_manager_t *auth,
                         const char *username,
                         const char *password,
                         user_role_t role) {
    if (!auth || !username || !password || auth->user_count >= MAX_USERS) {
        return -1;
    }

    pthread_mutex_lock(&auth->lock);

    // Check if user already exists
    for (int i = 0; i < auth->user_count; i++) {
        if (strcmp(auth->users[i].username, username) == 0) {
            pthread_mutex_unlock(&auth->lock);
            return -1;
        }
    }

    // Add new user
    user_entry_t *user = &auth->users[auth->user_count];
    strncpy(user->username, username, sizeof(user->username) - 1);
    simple_hash(password, user->password_hash, sizeof(user->password_hash));
    user->role = role;
    user->is_active = true;

    auth->user_count++;

    pthread_mutex_unlock(&auth->lock);

    printf("Added user: %s (role: %d)\n", username, role);
    
    return 0;
}

/**
 * Remove user
 */
int auth_manager_remove_user(auth_manager_t *auth, const char *username) {
    if (!auth || !username) {
        return -1;
    }

    pthread_mutex_lock(&auth->lock);

    for (int i = 0; i < auth->user_count; i++) {
        if (strcmp(auth->users[i].username, username) == 0) {
            // Mark as inactive
            auth->users[i].is_active = false;
            pthread_mutex_unlock(&auth->lock);
            return 0;
        }
    }

    pthread_mutex_unlock(&auth->lock);
    return -1;
}

/**
 * Change password
 */
int auth_manager_change_password(auth_manager_t *auth,
                                const char *username,
                                const char *new_password) {
    if (!auth || !username || !new_password) {
        return -1;
    }

    pthread_mutex_lock(&auth->lock);

    for (int i = 0; i < auth->user_count; i++) {
        if (strcmp(auth->users[i].username, username) == 0) {
            simple_hash(new_password, auth->users[i].password_hash,
                       sizeof(auth->users[i].password_hash));
            pthread_mutex_unlock(&auth->lock);
            return 0;
        }
    }

    pthread_mutex_unlock(&auth->lock);
    return -1;
}

/**
 * Authenticate user and generate token
 */
int auth_manager_authenticate(auth_manager_t *auth,
                             const char *username,
                             const char *password,
                             char *token_out,
                             size_t token_size) {
    if (!auth || !username || !password || !token_out) {
        return -1;
    }

    pthread_mutex_lock(&auth->lock);

    // Find user
    user_entry_t *user = NULL;
    for (int i = 0; i < auth->user_count; i++) {
        if (strcmp(auth->users[i].username, username) == 0 &&
            auth->users[i].is_active) {
            user = &auth->users[i];
            break;
        }
    }

    if (!user) {
        pthread_mutex_unlock(&auth->lock);
        return -1;
    }

    // Verify password
    char password_hash[256];
    simple_hash(password, password_hash, sizeof(password_hash));
    if (strcmp(user->password_hash, password_hash) != 0) {
        pthread_mutex_unlock(&auth->lock);
        return -1;
    }

    // Generate token
    generate_token(username, user->role, token_out, token_size);

    // Create session
    if (auth->session_count < MAX_SESSIONS) {
        session_entry_t *session = &auth->sessions[auth->session_count];
        strncpy(session->token, token_out, sizeof(session->token) - 1);
        strncpy(session->username, username, sizeof(session->username) - 1);
        session->role = user->role;
        session->expires_at = (uint64_t)time(NULL) + TOKEN_EXPIRY_SECONDS;
        auth->session_count++;
    }

    pthread_mutex_unlock(&auth->lock);

    printf("User authenticated: %s\n", username);
    
    return 0;
}

/**
 * Verify token and get user info
 */
int auth_manager_verify_token(auth_manager_t *auth,
                              const char *token,
                              char *username_out,
                              size_t username_size,
                              user_role_t *role_out) {
    if (!auth || !token) {
        return -1;
    }

    pthread_mutex_lock(&auth->lock);

    uint64_t now = (uint64_t)time(NULL);

    for (int i = 0; i < auth->session_count; i++) {
        if (strcmp(auth->sessions[i].token, token) == 0) {
            // Check expiration
            if (auth->sessions[i].expires_at < now) {
                pthread_mutex_unlock(&auth->lock);
                return -1;  // Token expired
            }

            if (username_out) {
                strncpy(username_out, auth->sessions[i].username, username_size - 1);
            }
            if (role_out) {
                *role_out = auth->sessions[i].role;
            }

            pthread_mutex_unlock(&auth->lock);
            return 0;
        }
    }

    pthread_mutex_unlock(&auth->lock);
    return -1;
}

/**
 * Check if role can control streaming
 */
bool auth_manager_can_control_streaming(user_role_t role) {
    return (role == ROLE_ADMIN || role == ROLE_OPERATOR);
}

/**
 * Check if role can modify settings
 */
bool auth_manager_can_modify_settings(user_role_t role) {
    return (role == ROLE_ADMIN || role == ROLE_OPERATOR);
}

/**
 * Check if role can manage users
 */
bool auth_manager_can_manage_users(user_role_t role) {
    return (role == ROLE_ADMIN);
}

/**
 * Create session
 */
int auth_manager_create_session(auth_manager_t *auth, const char *username) {
    if (!auth || !username) {
        return -1;
    }

    // Session creation is handled in authenticate
    return 0;
}

/**
 * Invalidate session/token
 */
int auth_manager_invalidate_session(auth_manager_t *auth, const char *token) {
    if (!auth || !token) {
        return -1;
    }

    pthread_mutex_lock(&auth->lock);

    for (int i = 0; i < auth->session_count; i++) {
        if (strcmp(auth->sessions[i].token, token) == 0) {
            // Remove session by shifting
            for (int j = i; j < auth->session_count - 1; j++) {
                auth->sessions[j] = auth->sessions[j + 1];
            }
            auth->session_count--;
            pthread_mutex_unlock(&auth->lock);
            return 0;
        }
    }

    pthread_mutex_unlock(&auth->lock);
    return -1;
}

/**
 * Cleanup authentication manager
 */
void auth_manager_cleanup(auth_manager_t *auth) {
    if (!auth) {
        return;
    }

    pthread_mutex_destroy(&auth->lock);
    free(auth);
}
