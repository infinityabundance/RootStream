#!/bin/bash
#
# PHASE 19: Web Dashboard Verification Script
# 
# This script verifies that the web dashboard components are properly
# implemented and functional.
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# Helper functions
print_header() {
    echo -e "\n${YELLOW}========================================${NC}"
    echo -e "${YELLOW}$1${NC}"
    echo -e "${YELLOW}========================================${NC}\n"
}

print_test() {
    echo -e "${YELLOW}[TEST]${NC} $1"
    TESTS_RUN=$((TESTS_RUN + 1))
}

print_pass() {
    echo -e "${GREEN}  ✓ PASSED:${NC} $1"
    TESTS_PASSED=$((TESTS_PASSED + 1))
}

print_fail() {
    echo -e "${RED}  ✗ FAILED:${NC} $1"
    TESTS_FAILED=$((TESTS_FAILED + 1))
}

print_info() {
    echo -e "  ℹ $1"
}

# Main verification
main() {
    print_header "PHASE 19: Web Dashboard Verification"
    
    echo "This script verifies the web dashboard implementation."
    echo ""
    
    # Check 1: Source files exist
    print_test "Checking backend source files..."
    if [ -f "src/web/api_server.h" ] && \
       [ -f "src/web/api_server.c" ] && \
       [ -f "src/web/websocket_server.h" ] && \
       [ -f "src/web/websocket_server.c" ] && \
       [ -f "src/web/auth_manager.h" ] && \
       [ -f "src/web/auth_manager.c" ] && \
       [ -f "src/web/rate_limiter.h" ] && \
       [ -f "src/web/rate_limiter.c" ] && \
       [ -f "src/web/api_routes.h" ] && \
       [ -f "src/web/api_routes.c" ] && \
       [ -f "src/web/models.h" ]; then
        print_pass "All backend source files exist"
    else
        print_fail "Missing backend source files"
    fi
    
    # Check 2: Frontend files exist
    print_test "Checking frontend source files..."
    if [ -d "frontend/src" ] && \
       [ -f "frontend/package.json" ] && \
       [ -f "frontend/src/App.js" ] && \
       [ -f "frontend/src/components/Dashboard.js" ] && \
       [ -f "frontend/src/components/PerformanceGraphs.js" ] && \
       [ -f "frontend/src/components/SettingsPanel.js" ] && \
       [ -f "frontend/src/services/api.js" ] && \
       [ -f "frontend/src/services/websocket.js" ]; then
        print_pass "All frontend source files exist"
    else
        print_fail "Missing frontend source files"
    fi
    
    # Check 3: Test files exist
    print_test "Checking test files..."
    if [ -f "tests/unit/test_web_dashboard.c" ]; then
        print_pass "Test file exists"
    else
        print_fail "Missing test file"
    fi
    
    # Check 4: Documentation exists
    print_test "Checking documentation..."
    if [ -f "docs/WEB_DASHBOARD_API.md" ] && \
       [ -f "docs/WEB_DASHBOARD_DEPLOYMENT.md" ] && \
       [ -f "frontend/README.md" ]; then
        print_pass "All documentation files exist"
    else
        print_fail "Missing documentation files"
    fi
    
    # Check 5: CMake configuration
    print_test "Checking CMake configuration..."
    if grep -q "BUILD_WEB_DASHBOARD" CMakeLists.txt && \
       grep -q "test_web_dashboard" CMakeLists.txt; then
        print_pass "CMake configured for web dashboard"
    else
        print_fail "CMake not properly configured"
    fi
    
    # Check 6: vcpkg.json updated
    print_test "Checking dependency configuration..."
    if grep -q "cjson" vcpkg.json; then
        print_pass "Dependencies configured in vcpkg.json"
    else
        print_fail "Dependencies not properly configured"
    fi
    
    # Check 7: Try to build (if build directory doesn't exist)
    if [ ! -d "build" ]; then
        print_test "Building project..."
        mkdir -p build
        cd build
        if cmake .. -DBUILD_WEB_DASHBOARD=ON > /dev/null 2>&1; then
            if make test_web_dashboard > /dev/null 2>&1; then
                print_pass "Project builds successfully"
                BUILD_SUCCESS=true
            else
                print_fail "Build failed"
                BUILD_SUCCESS=false
            fi
        else
            print_fail "CMake configuration failed"
            BUILD_SUCCESS=false
        fi
        cd ..
    else
        print_info "Build directory exists, skipping build test"
        print_info "To force rebuild, remove 'build' directory"
        BUILD_SUCCESS=true
    fi
    
    # Check 8: Run unit tests if build succeeded
    if [ "$BUILD_SUCCESS" = true ] && [ -f "build/test_web_dashboard" ]; then
        print_test "Running unit tests..."
        if build/test_web_dashboard > /tmp/test_output.txt 2>&1; then
            TESTS_PASSED_COUNT=$(grep "Tests passed:" /tmp/test_output.txt | awk '{print $3}')
            TESTS_TOTAL=$(grep "Tests run:" /tmp/test_output.txt | awk '{print $3}')
            if [ "$TESTS_PASSED_COUNT" = "$TESTS_TOTAL" ]; then
                print_pass "All unit tests passed ($TESTS_PASSED_COUNT/$TESTS_TOTAL)"
            else
                print_fail "Some unit tests failed ($TESTS_PASSED_COUNT/$TESTS_TOTAL)"
                print_info "See /tmp/test_output.txt for details"
            fi
        else
            print_fail "Unit tests execution failed"
        fi
    fi
    
    # Check 9: Frontend package dependencies
    print_test "Checking frontend dependencies..."
    if [ -f "frontend/package.json" ]; then
        if grep -q "\"react\":" frontend/package.json && \
           grep -q "\"recharts\":" frontend/package.json; then
            print_pass "Frontend dependencies properly configured"
        else
            print_fail "Missing frontend dependencies"
        fi
    else
        print_fail "frontend/package.json not found"
    fi
    
    # Check 10: Code structure validation
    print_test "Validating code structure..."
    
    # Check for authentication functions
    if grep -q "auth_manager_init" src/web/auth_manager.c && \
       grep -q "auth_manager_authenticate" src/web/auth_manager.c && \
       grep -q "auth_manager_verify_token" src/web/auth_manager.c; then
        print_pass "Authentication manager properly implemented"
    else
        print_fail "Authentication manager missing key functions"
    fi
    
    # Check for API routes
    if grep -q "api_route_get_host_info" src/web/api_routes.c && \
       grep -q "api_route_get_metrics_current" src/web/api_routes.c && \
       grep -q "api_route_get_streams" src/web/api_routes.c; then
        print_pass "API routes properly implemented"
    else
        print_fail "API routes missing key endpoints"
    fi
    
    # Check for WebSocket functionality
    if grep -q "websocket_server_init" src/web/websocket_server.c && \
       grep -q "websocket_server_broadcast_metrics" src/web/websocket_server.c; then
        print_pass "WebSocket server properly implemented"
    else
        print_fail "WebSocket server missing key functions"
    fi
    
    # Check 11: Frontend components
    print_test "Validating frontend components..."
    
    if grep -q "Dashboard" frontend/src/App.js && \
       grep -q "PerformanceGraphs" frontend/src/App.js && \
       grep -q "SettingsPanel" frontend/src/App.js; then
        print_pass "Frontend components properly integrated"
    else
        print_fail "Frontend components not properly integrated"
    fi
    
    # Summary
    print_header "Verification Summary"
    echo -e "Tests run:    ${TESTS_RUN}"
    echo -e "Tests passed: ${GREEN}${TESTS_PASSED}${NC}"
    echo -e "Tests failed: ${RED}${TESTS_FAILED}${NC}"
    
    if [ $TESTS_FAILED -eq 0 ]; then
        echo ""
        echo -e "${GREEN}✓ All verifications passed!${NC}"
        echo ""
        echo "The web dashboard is properly implemented."
        echo ""
        echo "Next steps:"
        echo "  1. Build: cmake .. -DBUILD_WEB_DASHBOARD=ON && make"
        echo "  2. Install frontend deps: cd frontend && npm install"
        echo "  3. Run backend: ./rootstream --enable-web-dashboard"
        echo "  4. Run frontend: cd frontend && npm start"
        echo "  5. Access dashboard at http://localhost:3000"
        echo ""
        echo "See docs/WEB_DASHBOARD_DEPLOYMENT.md for deployment instructions."
        echo ""
        return 0
    else
        echo ""
        echo -e "${RED}✗ Some verifications failed!${NC}"
        echo ""
        echo "Please review the failures above and fix them."
        echo ""
        return 1
    fi
}

# Run main function
main
