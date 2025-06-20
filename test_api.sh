#!/bin/bash

# HTTP C++ Server API Test Script
# This script tests all the API endpoints of the HTTP server

SERVER_URL="http://localhost:8080"
COLLECTION="users"

echo "=================================="
echo "HTTP C++ Server API Test Script"
echo "=================================="
echo "Server URL: $SERVER_URL"
echo "Test Collection: $COLLECTION"
echo ""

# Function to print test results
print_test() {
    echo "----------------------------------------"
    echo "Test: $1"
    echo "----------------------------------------"
}

# Function to check if server is running
check_server() {
    echo "Checking if server is running..."
    if curl -s "$SERVER_URL/api/data/test" > /dev/null 2>&1; then
        echo "✓ Server is running"
        return 0
    else
        echo "✗ Server is not running or not accessible"
        echo "Please make sure the server is started with: ./bin/http_server"
        exit 1
    fi
}

# Test 1: Check server status
print_test "Server Status Check"
check_server
echo ""

# Test 2: Create items (POST)
print_test "CREATE Operations (POST)"

echo "Creating user 1..."
USER1_RESPONSE=$(curl -s -X POST "$SERVER_URL/api/data/$COLLECTION" \
  -H "Content-Type: application/json" \
  -d '{"name":"John Doe","email":"john@example.com","age":"30"}')
echo "Response: $USER1_RESPONSE"

echo ""
echo "Creating user 2..."
USER2_RESPONSE=$(curl -s -X POST "$SERVER_URL/api/data/$COLLECTION" \
  -H "Content-Type: application/json" \
  -d '{"name":"Jane Smith","email":"jane@example.com","age":"25"}')
echo "Response: $USER2_RESPONSE"

echo ""
echo "Creating user 3..."
USER3_RESPONSE=$(curl -s -X POST "$SERVER_URL/api/data/$COLLECTION" \
  -H "Content-Type: application/json" \
  -d '{"name":"Bob Johnson","email":"bob@example.com","age":"35"}')
echo "Response: $USER3_RESPONSE"
echo ""

# Test 3: Read all items (GET)
print_test "READ All Items (GET)"
echo "Getting all users..."
ALL_USERS=$(curl -s "$SERVER_URL/api/data/$COLLECTION")
echo "Response: $ALL_USERS"
echo ""

# Test 4: Read specific item (GET)
print_test "READ Specific Item (GET)"
echo "Getting user with ID 1..."
USER_1=$(curl -s "$SERVER_URL/api/data/$COLLECTION/1")
echo "Response: $USER_1"

echo ""
echo "Getting user with ID 2..."
USER_2=$(curl -s "$SERVER_URL/api/data/$COLLECTION/2")
echo "Response: $USER_2"
echo ""

# Test 5: Update item (PUT)
print_test "UPDATE Operation (PUT)"
echo "Updating user 1..."
UPDATE_RESPONSE=$(curl -s -X PUT "$SERVER_URL/api/data/$COLLECTION/1" \
  -H "Content-Type: application/json" \
  -d '{"name":"John Updated","email":"john.updated@example.com","age":"31"}')
echo "Response: $UPDATE_RESPONSE"

echo ""
echo "Getting updated user 1..."
UPDATED_USER=$(curl -s "$SERVER_URL/api/data/$COLLECTION/1")
echo "Response: $UPDATED_USER"
echo ""

# Test 6: Test with different collection
print_test "Different Collection Test"
PRODUCTS_COLLECTION="products"

echo "Creating product..."
PRODUCT_RESPONSE=$(curl -s -X POST "$SERVER_URL/api/data/$PRODUCTS_COLLECTION" \
  -H "Content-Type: application/json" \
  -d '{"name":"Laptop","price":"999","category":"Electronics"}')
echo "Response: $PRODUCT_RESPONSE"

echo ""
echo "Getting all products..."
ALL_PRODUCTS=$(curl -s "$SERVER_URL/api/data/$PRODUCTS_COLLECTION")
echo "Response: $ALL_PRODUCTS"
echo ""

# Test 7: File operations
print_test "FILE Operations"

# Create a test file
TEST_FILE="test_file.txt"
echo "This is a test file for upload/download functionality." > "$TEST_FILE"
echo "It contains some sample text to verify the file operations work correctly." >> "$TEST_FILE"
echo "Current timestamp: $(date)" >> "$TEST_FILE"

echo "Created test file: $TEST_FILE"
echo ""

echo "Uploading test file..."
UPLOAD_RESPONSE=$(curl -s -X POST "$SERVER_URL/api/files/upload" \
  -F "file=@$TEST_FILE")
echo "Response: $UPLOAD_RESPONSE"

echo ""
echo "Listing uploaded files..."
FILE_LIST=$(curl -s "$SERVER_URL/api/files")
echo "Response: $FILE_LIST"

echo ""
echo "Downloading uploaded file..."
curl -s "$SERVER_URL/api/files/download/$TEST_FILE" -o "downloaded_$TEST_FILE"
if [ -f "downloaded_$TEST_FILE" ]; then
    echo "✓ File downloaded successfully"
    echo "Downloaded file content:"
    cat "downloaded_$TEST_FILE"
    echo ""

    # Compare files
    if cmp -s "$TEST_FILE" "downloaded_$TEST_FILE"; then
        echo "✓ Downloaded file matches original"
    else
        echo "✗ Downloaded file differs from original"
    fi
else
    echo "✗ File download failed"
fi

# Clean up test files
rm -f "$TEST_FILE" "downloaded_$TEST_FILE"
echo ""

# Test 8: Delete operations (DELETE)
print_test "DELETE Operations"

echo "Deleting user 2..."
DELETE_RESPONSE=$(curl -s -X DELETE "$SERVER_URL/api/data/$COLLECTION/2")
echo "Response: $DELETE_RESPONSE"

echo ""
echo "Trying to get deleted user 2..."
DELETED_USER=$(curl -s "$SERVER_URL/api/data/$COLLECTION/2")
echo "Response: $DELETED_USER"

echo ""
echo "Getting all users after deletion..."
USERS_AFTER_DELETE=$(curl -s "$SERVER_URL/api/data/$COLLECTION")
echo "Response: $USERS_AFTER_DELETE"
echo ""

# Test 9: Error handling
print_test "ERROR Handling Tests"

echo "Trying to get non-existent user (ID 999)..."
NON_EXISTENT=$(curl -s "$SERVER_URL/api/data/$COLLECTION/999")
echo "Response: $NON_EXISTENT"

echo ""
echo "Trying to update non-existent user (ID 999)..."
UPDATE_NON_EXISTENT=$(curl -s -X PUT "$SERVER_URL/api/data/$COLLECTION/999" \
  -H "Content-Type: application/json" \
  -d '{"name":"Ghost User"}')
echo "Response: $UPDATE_NON_EXISTENT"

echo ""
echo "Trying to delete non-existent user (ID 999)..."
DELETE_NON_EXISTENT=$(curl -s -X DELETE "$SERVER_URL/api/data/$COLLECTION/999")
echo "Response: $DELETE_NON_EXISTENT"

echo ""
echo "Trying to download non-existent file..."
NON_EXISTENT_FILE=$(curl -s "$SERVER_URL/api/files/download/nonexistent.txt")
echo "Response: $NON_EXISTENT_FILE"
echo ""

# Test 10: Form data test
print_test "FORM Data Test"

echo "Creating user with form data..."
FORM_RESPONSE=$(curl -s -X POST "$SERVER_URL/api/data/$COLLECTION" \
  -H "Content-Type: application/x-www-form-urlencoded" \
  -d "name=Form User&email=form@example.com&age=28")
echo "Response: $FORM_RESPONSE"
echo ""

# Summary
echo "=================================="
echo "Test Summary"
echo "=================================="
echo "✓ CRUD Operations tested"
echo "✓ File Upload/Download tested"
echo "✓ Error handling tested"
echo "✓ Form data handling tested"
echo "✓ Multiple collections tested"
echo ""
echo "All tests completed!"
echo "Check the responses above to verify functionality."
echo ""
echo "Additional testing options:"
echo "1. Built-in Web Client: Open http://localhost:8080 in your browser"
echo "2. Individual curl commands:"
echo "   curl -X POST $SERVER_URL/api/data/users -H 'Content-Type: application/json' -d '{\"name\":\"Test User\"}'"
echo "   curl $SERVER_URL/api/data/users"
echo "   curl $SERVER_URL/api/files"
