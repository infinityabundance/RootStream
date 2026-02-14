/**
 * PHASE 19: Web Dashboard - Authentication Manager Implementation
 * PHASE 30: Security - Use Argon2 and remove hardcoded credentials
 */

#include "auth_manager.h"
#include "../security/user_auth.h"
#include "../security/crypto_primitives.h"
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
    char password_hash[USER_AUTH_HASH_LEN];  // Use Argon2 hash size
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
 * Validate password strength
 * Returns 0 on success, -1 if password is too weak
 */
static int validate_password_strength(const char *password) {
    if (!password) {
        return -1;
    }
    
    size_t len = strlen(password);
    
    // Minimum length check
    if (len < 8) {
        fprintf(stderr, "Password too short (minimum 8 characters)\n");
        return -1;
    }
    
    // Maximum length check
    if (len > 128) {
        fprintf(stderr, "Password too long (maximum 128 characters)\n");
        return -1;
    }
    
    // Check for at least one letter and one number
    bool has_letter = false;
    bool has_digit = false;
    
    for (size_t i = 0; i < len; i++) {
        if ((password[i] >= 'a' && password[i] <= 'z') || 
            (password[i] >= 'A' && password[i] <= 'Z')) {
            has_letter = true;
        }
        if (password[i] >= '0' && password[i] <= '9') {
            has_digit = true;
        }
    }
    
    if (!has_letter || !has_digit) {
        fprintf(stderr, "Password must contain at least one letter and one number\n");
        return -1;
    }
    
    return 0;
}

/**
 * Generate cryptographically secure token
 * Returns 0 on success, -1 on error
 */
static int generate_token(const char *username, user_role_t role, char *token, size_t token_size) {
    // Generate cryptographically random bytes
    uint8_t random_bytes[32];
    if (crypto_prim_random_bytes(random_bytes, sizeof(random_bytes)) != 0) {
        fprintf(stderr, "CRITICAL: Failed to generate random bytes for token\n");
        crypto_prim_secure_wipe(random_bytes, sizeof(random_bytes));
        return -1;
    }
    
    // Convert to hex string
    const char hex[] = "0123456789abcdef";
    size_t pos = 0;
    
    // Add prefix with username and role for debugging (optional)
    int written = snprintf(token + pos, token_size - pos, "%s_%d_", username, role);
    if (written < 0 || (size_t)written >= token_size - pos) {
        fprintf(stderr, "ERROR: Token buffer too small\n");
        crypto_prim_secure_wipe(random_bytes, sizeof(random_bytes));
        return -1;
    }
    pos += written;
    
    // Add random hex string
    for (size_t i = 0; i < sizeof(random_bytes) && pos < token_size - 2; i++) {
        token[pos++] = hex[(random_bytes[i] >> 4) & 0xF];
        token[pos++] = hex[random_bytes[i] & 0xF];
    }
    token[pos] = '\0';
    
    // Securely wipe random bytes from memory
    crypto_prim_secure_wipe(random_bytes, sizeof(random_bytes));
    return 0;
}

/**
 * Initialize authentication manager
 */
auth_manager_t *auth_manager_init(void) {
    auth_manager_t *auth = (auth_manager_t *)calloc(1, sizeof(auth_manager_t));
    if (!auth) {
        return NULL;
    }

    // Initialize crypto primitives and user_auth
    if (crypto_prim_init() != 0) {
        fprintf(stderr, "Failed to initialize crypto primitives\n");
        free(auth);
        return NULL;
    }
    
    if (user_auth_init() != 0) {
        fprintf(stderr, "Failed to initialize user authentication\n");
        free(auth);
        return NULL;
    }

    pthread_mutex_init(&auth->lock, NULL);
    auth->user_count = 0;
    auth->session_count = 0;

    // SECURITY: Do NOT create default admin user with hardcoded credentials
    // Initial admin must be created through environment variables or setup script
    // Check environment variable for initial admin setup
    const char *admin_user = getenv("ROOTSTREAM_ADMIN_USERNAME");
    const char *admin_pass = getenv("ROOTSTREAM_ADMIN_PASSWORD");
    
    if (admin_user && admin_pass && strlen(admin_user) > 0 && strlen(admin_pass) > 0) {
        if (auth_manager_add_user(auth, admin_user, admin_pass, ROLE_ADMIN) == 0) {
            printf("Initial admin user created from environment variables\n");
        } else {
            fprintf(stderr, "WARNING: Failed to create initial admin user\n");
        }
    } else {
        printf("WARNING: No initial admin user created. Set ROOTSTREAM_ADMIN_USERNAME "
               "and ROOTSTREAM_ADMIN_PASSWORD environment variables to create one.\n");
    }

    return auth;
}

/**
 * Add user with password strength validation and Argon2 hashing
 */
int auth_manager_add_user(auth_manager_t *auth,
                         const char *username,
                         const char *password,
                         user_role_t role) {
    if (!auth || !username || !password || auth->user_count >= MAX_USERS) {
        return -1;
    }
    
    // Validate username
    if (strlen(username) == 0 || strlen(username) >= sizeof(((user_entry_t*)0)->username)) {
        fprintf(stderr, "Invalid username length\n");
        return -1;
    }
    
    // Validate password strength
    if (validate_password_strength(password) != 0) {
        return -1;
    }

    pthread_mutex_lock(&auth->lock);

    // Check if user already exists
    for (int i = 0; i < auth->user_count; i++) {
        if (strcmp(auth->users[i].username, username) == 0) {
            pthread_mutex_unlock(&auth->lock);
            fprintf(stderr, "User already exists: %s\n", username);
            return -1;
        }
    }

    // Add new user
    user_entry_t *user = &auth->users[auth->user_count];
    strncpy(user->username, username, sizeof(user->username) - 1);
    user->username[sizeof(user->username) - 1] = '\0';
    
    // Hash password using Argon2 via user_auth
    if (user_auth_hash_password(password, user->password_hash) != 0) {
        pthread_mutex_unlock(&auth->lock);
        fprintf(stderr, "Failed to hash password\n");
        return -1;
    }
    
    user->role = role;
    user->is_active = true;

    auth->user_count++;

    pthread_mutex_unlock(&auth->lock);

    printf("Added user: %s (role: %d) with Argon2 hashed password\n", username, role);
    
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
 * Change password with validation and Argon2 hashing
 */
int auth_manager_change_password(auth_manager_t *auth,
                                const char *username,
                                const char *new_password) {
    if (!auth || !username || !new_password) {
        return -1;
    }
    
    // Validate password strength
    if (validate_password_strength(new_password) != 0) {
        return -1;
    }

    pthread_mutex_lock(&auth->lock);

    for (int i = 0; i < auth->user_count; i++) {
        if (strcmp(auth->users[i].username, username) == 0) {
            // Hash password using Argon2 via user_auth
            if (user_auth_hash_password(new_password, auth->users[i].password_hash) != 0) {
                pthread_mutex_unlock(&auth->lock);
                fprintf(stderr, "Failed to hash new password\n");
                return -1;
            }
            pthread_mutex_unlock(&auth->lock);
            printf("Password changed for user: %s\n", username);
            return 0;
        }
    }

    pthread_mutex_unlock(&auth->lock);
    fprintf(stderr, "User not found: %s\n", username);
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

    // Verify password using Argon2 via user_auth
    if (!user_auth_verify_password(password, user->password_hash)) {
        pthread_mutex_unlock(&auth->lock);
        fprintf(stderr, "Authentication failed for user: %s\n", username);
        return -1;
    }

    // Generate token
    if (generate_token(username, user->role, token_out, token_size) != 0) {
        pthread_mutex_unlock(&auth->lock);
        fprintf(stderr, "Failed to generate token for user: %s\n", username);
        return -1;
    }

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
