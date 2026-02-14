/*
 * test_phase30_security.c - Unit tests for Phase 30 security fixes
 * Tests password validation, Argon2 hashing, and removal of hardcoded credentials
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "../../src/web/auth_manager.h"
#include "../../src/security/user_auth.h"
#include "../../src/security/crypto_primitives.h"

/* Test counter */
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    printf("  Testing %s... ", name); \
    fflush(stdout);

#define PASS() \
    printf("PASS\n"); \
    tests_passed++;

#define FAIL(msg) \
    printf("FAIL: %s\n", msg); \
    tests_failed++;

/* Test password strength validation */
void test_password_validation(void) {
    printf("\n=== Password Strength Validation Tests ===\n");
    
    /* Test initialization */
    TEST("auth_manager_init without hardcoded credentials");
    // Clear environment variables to ensure no default user is created
    unsetenv("ROOTSTREAM_ADMIN_USERNAME");
    unsetenv("ROOTSTREAM_ADMIN_PASSWORD");
    
    auth_manager_t *auth = auth_manager_init();
    if (auth != NULL) {
        PASS();
    } else {
        FAIL("Auth manager initialization failed");
        return;
    }
    
    /* Test weak password rejection - too short */
    TEST("reject password < 8 characters");
    if (auth_manager_add_user(auth, "testuser1", "short", ROLE_VIEWER) != 0) {
        PASS();
    } else {
        FAIL("Weak password accepted");
    }
    
    /* Test weak password rejection - no number */
    TEST("reject password without number");
    if (auth_manager_add_user(auth, "testuser2", "noNumbers", ROLE_VIEWER) != 0) {
        PASS();
    } else {
        FAIL("Password without number accepted");
    }
    
    /* Test weak password rejection - no letter */
    TEST("reject password without letter");
    if (auth_manager_add_user(auth, "testuser3", "12345678", ROLE_VIEWER) != 0) {
        PASS();
    } else {
        FAIL("Password without letter accepted");
    }
    
    /* Test strong password acceptance */
    TEST("accept strong password");
    if (auth_manager_add_user(auth, "testuser4", "StrongPass123", ROLE_VIEWER) == 0) {
        PASS();
    } else {
        FAIL("Strong password rejected");
    }
    
    /* Test Argon2 password verification */
    TEST("authenticate with correct password");
    char token[512];
    if (auth_manager_authenticate(auth, "testuser4", "StrongPass123", token, sizeof(token)) == 0) {
        PASS();
    } else {
        FAIL("Authentication with correct password failed");
    }
    
    /* Test wrong password rejection */
    TEST("reject wrong password");
    if (auth_manager_authenticate(auth, "testuser4", "WrongPass123", token, sizeof(token)) != 0) {
        PASS();
    } else {
        FAIL("Authentication with wrong password succeeded");
    }
    
    auth_manager_cleanup(auth);
}

/* Test token generation is cryptographically secure */
void test_token_generation(void) {
    printf("\n=== Secure Token Generation Tests ===\n");
    
    auth_manager_t *auth = auth_manager_init();
    if (!auth) {
        FAIL("Auth manager initialization failed");
        return;
    }
    
    /* Create a test user */
    if (auth_manager_add_user(auth, "tokentest", "SecurePass123", ROLE_ADMIN) != 0) {
        FAIL("Failed to create test user");
        auth_manager_cleanup(auth);
        return;
    }
    
    /* Generate multiple tokens and verify they're different */
    TEST("generate unique tokens");
    char token1[512], token2[512], token3[512];
    
    int auth1 = auth_manager_authenticate(auth, "tokentest", "SecurePass123", token1, sizeof(token1));
    int auth2 = auth_manager_authenticate(auth, "tokentest", "SecurePass123", token2, sizeof(token2));
    int auth3 = auth_manager_authenticate(auth, "tokentest", "SecurePass123", token3, sizeof(token3));
    
    if (auth1 != 0) {
        FAIL("First token generation failed");
    } else if (auth2 != 0) {
        FAIL("Second token generation failed");
    } else if (auth3 != 0) {
        FAIL("Third token generation failed");
    } else if (strcmp(token1, token2) == 0) {
        FAIL("Token 1 and 2 are identical");
    } else if (strcmp(token2, token3) == 0) {
        FAIL("Token 2 and 3 are identical");
    } else if (strcmp(token1, token3) == 0) {
        FAIL("Token 1 and 3 are identical");
    } else if (strlen(token1) < 64) {
        FAIL("Token 1 is too short (expected 64+ hex chars from random bytes)");
    } else if (strlen(token2) < 64) {
        FAIL("Token 2 is too short (expected 64+ hex chars from random bytes)");
    } else if (strlen(token3) < 64) {
        FAIL("Token 3 is too short (expected 64+ hex chars from random bytes)");
    } else {
        PASS();
    }
    
    /* Verify token is not "demo_token_12345" */
    TEST("no hardcoded demo token");
    if (strstr(token1, "demo_token_12345") == NULL) {
        PASS();
    } else {
        FAIL("Hardcoded demo token still present");
    }
    
    auth_manager_cleanup(auth);
}

/* Test no default admin credentials */
void test_no_default_credentials(void) {
    printf("\n=== No Default Credentials Tests ===\n");
    
    /* Clear environment variables */
    unsetenv("ROOTSTREAM_ADMIN_USERNAME");
    unsetenv("ROOTSTREAM_ADMIN_PASSWORD");
    
    TEST("no default admin:admin user created");
    auth_manager_t *auth = auth_manager_init();
    if (!auth) {
        FAIL("Auth manager initialization failed");
        return;
    }
    
    char token[512];
    if (auth_manager_authenticate(auth, "admin", "admin", token, sizeof(token)) != 0) {
        PASS();
    } else {
        FAIL("Default admin:admin credentials still exist");
    }
    
    auth_manager_cleanup(auth);
}

/* Test environment variable admin creation */
void test_env_admin_creation(void) {
    printf("\n=== Environment Variable Admin Creation Tests ===\n");
    
    TEST("create admin from environment variables");
    setenv("ROOTSTREAM_ADMIN_USERNAME", "envadmin", 1);
    setenv("ROOTSTREAM_ADMIN_PASSWORD", "EnvSecure123", 1);
    
    auth_manager_t *auth = auth_manager_init();
    if (!auth) {
        FAIL("Auth manager initialization failed");
        return;
    }
    
    char token[512];
    if (auth_manager_authenticate(auth, "envadmin", "EnvSecure123", token, sizeof(token)) == 0) {
        PASS();
    } else {
        FAIL("Environment-based admin creation failed");
    }
    
    /* Verify wrong password is rejected */
    TEST("reject wrong password for env admin");
    if (auth_manager_authenticate(auth, "envadmin", "wrongpass", token, sizeof(token)) != 0) {
        PASS();
    } else {
        FAIL("Wrong password accepted");
    }
    
    unsetenv("ROOTSTREAM_ADMIN_USERNAME");
    unsetenv("ROOTSTREAM_ADMIN_PASSWORD");
    auth_manager_cleanup(auth);
}

/* Test token verification */
void test_token_verification(void) {
    printf("\n=== Token Verification Tests ===\n");
    
    auth_manager_t *auth = auth_manager_init();
    if (!auth) {
        FAIL("Auth manager initialization failed");
        return;
    }
    
    auth_manager_add_user(auth, "verifytest", "VerifyPass123", ROLE_OPERATOR);
    
    TEST("verify valid token");
    char token[512];
    if (auth_manager_authenticate(auth, "verifytest", "VerifyPass123", token, sizeof(token)) != 0) {
        FAIL("Authentication failed");
    } else {
        char username[256];
        user_role_t role;
        if (auth_manager_verify_token(auth, token, username, sizeof(username), &role) == 0 &&
            strcmp(username, "verifytest") == 0 &&
            role == ROLE_OPERATOR) {
            PASS();
        } else {
            FAIL("Token verification failed or returned wrong data");
        }
    }
    
    TEST("reject invalid token");
    char username[256];
    user_role_t role;
    if (auth_manager_verify_token(auth, "invalid_token_xyz", username, sizeof(username), &role) != 0) {
        PASS();
    } else {
        FAIL("Invalid token was accepted");
    }
    
    TEST("invalidate session token");
    if (auth_manager_invalidate_session(auth, token) == 0 &&
        auth_manager_verify_token(auth, token, username, sizeof(username), &role) != 0) {
        PASS();
    } else {
        FAIL("Token invalidation failed");
    }
    
    auth_manager_cleanup(auth);
}

int main(void) {
    printf("RootStream Phase 30 Security Tests\n");
    printf("===================================\n");
    
    test_password_validation();
    test_token_generation();
    test_no_default_credentials();
    test_env_admin_creation();
    test_token_verification();
    
    printf("\n===================================\n");
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    printf("===================================\n");
    
    return tests_failed > 0 ? 1 : 0;
}
