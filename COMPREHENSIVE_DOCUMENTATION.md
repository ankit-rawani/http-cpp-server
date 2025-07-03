# HTTP C++ Server - Comprehensive Code Documentation

## Table of Contents
1. [Project Overview](#project-overview)
2. [Architecture](#architecture)
3. [Core Components](#core-components)
4. [Data Structures](#data-structures)
5. [Class Documentation](#class-documentation)
6. [Function Documentation](#function-documentation)
7. [API Endpoints](#api-endpoints)
8. [Build System](#build-system)
9. [Testing](#testing)
10. [Web Client](#web-client)
11. [Usage Examples](#usage-examples)
12. [Technical Details](#technical-details)

## Project Overview

This is a lightweight, multithreaded HTTP server implementation written in C++17. The server provides a complete REST API framework with built-in CRUD operations, file upload/download capabilities, and a web-based client interface. It's designed to be simple, fast, and easily extensible.

### Key Features
- **RESTful API**: Full CRUD operations for any collection
- **File Management**: Upload, download, and list files
- **Multithreading**: Concurrent request handling using worker threads
- **In-Memory Database**: Simple key-value data storage
- **Web Client**: Built-in HTML/JavaScript client for testing
- **Content Type Detection**: Automatic MIME type detection
- **Form Data Support**: Both JSON and form-encoded data parsing
- **Pattern Matching**: Dynamic URL routing with parameter extraction

## Architecture

The server follows a modular architecture with clear separation of concerns:

```
┌─────────────────────────────────────────────────────────────┐
│                        HTTP Server                          │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐  │
│  │   HTTP Parser   │  │   Route Handler │  │   Data Store    │  │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘  │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐  │
│  │ Response Builder│  │  File Manager   │  │  Thread Pool    │  │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘  │
├─────────────────────────────────────────────────────────────┤
│                    Socket Layer (TCP)                       │
└─────────────────────────────────────────────────────────────┘
```

## Core Components

### 1. **HTTP Request/Response Pipeline**
- **Request Parsing**: Parses HTTP headers, body, query parameters
- **Content Negotiation**: Handles different content types (JSON, form data, multipart)
- **Response Generation**: Builds HTTP responses with proper headers

### 2. **Data Storage Layer**
- **In-Memory Store**: Thread-safe key-value storage
- **Collection-based**: Organizes data in collections (like tables)
- **Auto-incrementing IDs**: Automatic ID generation for new items

### 3. **Threading Model**
- **Worker Threads**: Each client connection spawns a new thread
- **Thread Safety**: Mutex protection for shared resources
- **Graceful Shutdown**: Proper cleanup and thread joining

### 4. **File System Integration**
- **Upload Directory**: Configurable upload directory (`uploads/`)
- **Content Type Detection**: Based on file extensions
- **Binary/Text Handling**: Supports both binary and text file types

## Data Structures

### HttpRequest Structure
```cpp
struct HttpRequest {
    std::string method;                                    // HTTP method (GET, POST, etc.)
    std::string path;                                      // Request path
    std::string version;                                   // HTTP version
    std::map<std::string, std::string> headers;            // HTTP headers
    std::string body;                                      // Request body
    std::map<std::string, std::string> query_params;       // Query parameters
    std::map<std::string, std::string> form_data;          // Form data
    std::map<std::string, FileData> files;                 // Uploaded files
};
```

**Purpose**: Encapsulates all information from an HTTP request
**Usage**: Passed to route handlers for processing
**Key Features**:
- Automatic query parameter parsing from URL
- Form data extraction from POST requests
- File upload handling for multipart forms
- Header case-insensitive access

### HttpResponse Structure
```cpp
struct HttpResponse {
    int status_code;                                       // HTTP status code
    std::string status_text;                               // Status text
    std::map<std::string, std::string> headers;            // Response headers
    std::string body;                                      // Response body (text)
    std::vector<char> binary_data;                         // Binary response data
    bool is_binary;                                        // Binary/text flag
};
```

**Purpose**: Represents HTTP response data
**Usage**: Populated by route handlers and sent to clients
**Key Features**:
- Supports both text and binary responses
- Automatic content-length calculation
- CORS headers support

### FileData Structure
```cpp
struct FileData {
    std::string filename;                                  // Original filename
    std::string content_type;                              // MIME type
    std::vector<char> data;                                // File content
};
```

**Purpose**: Represents uploaded file information
**Usage**: Stored in HttpRequest for file upload handling
**Key Features**:
- Preserves original filename
- Stores MIME type for proper content handling
- Binary data storage

## Class Documentation

### HttpServer Class

The main server class that handles all HTTP operations.

#### Constructor
```cpp
HttpServer(int port = 8080)
```
**Purpose**: Initializes the server with specified port
**Parameters**:
- `port`: Port number to bind to (default: 8080)
**Behavior**: Sets up internal state but doesn't start listening

#### Core Methods

##### `void start()`
**Purpose**: Starts the HTTP server
**Behavior**:
1. Creates TCP socket
2. Binds to specified port
3. Starts listening for connections
4. Sets up default routes
5. Begins accepting client connections

**Implementation Details**:
- Uses `SO_REUSEADDR` to avoid "Address already in use" errors
- Configures socket for IPv4 (`AF_INET`)
- Sets listen backlog to 10 connections
- Spawns worker threads for each client

##### `void stop()`
**Purpose**: Gracefully shuts down the server
**Behavior**:
1. Sets running flag to false
2. Closes server socket
3. Joins all worker threads
4. Cleans up resources

##### `void add_route(const std::string& method, const std::string& path, RouteHandler handler)`
**Purpose**: Registers a new route handler
**Parameters**:
- `method`: HTTP method (GET, POST, PUT, DELETE)
- `path`: URL path pattern (supports {param} syntax)
- `handler`: Function to handle requests

**Example**:
```cpp
server.add_route("GET", "/users/{id}", [](const HttpRequest& req, HttpResponse& res) {
    // Handle user retrieval
});
```

#### Private Methods

##### `void start_listening()`
**Purpose**: Main server loop that accepts connections
**Behavior**:
- Runs in a loop while server is active
- Accepts incoming connections
- Spawns worker threads for each client
- Handles connection errors gracefully

**Thread Safety**: This method runs in the main thread

##### `void handle_client(int client_socket)`
**Purpose**: Processes a single client connection
**Parameters**:
- `client_socket`: Socket descriptor for the client
**Behavior**:
1. Reads HTTP request data
2. Handles chunked/content-length requests
3. Parses request into HttpRequest structure
4. Finds matching route handler
5. Executes handler and builds response
6. Sends response to client
7. Closes connection

**Thread Safety**: Each call runs in its own worker thread

##### `HttpRequest parse_request(const std::string& request_str)`
**Purpose**: Parses raw HTTP request into structured data
**Parameters**:
- `request_str`: Raw HTTP request string
**Returns**: Parsed HttpRequest object

**Parsing Process**:
1. **Request Line**: Extracts method, path, and HTTP version
2. **Headers**: Parses key-value pairs with proper trimming
3. **Query Parameters**: Extracts from URL query string
4. **Body**: Reads request body based on Content-Length
5. **Form Data**: Parses based on Content-Type:
   - `application/x-www-form-urlencoded`: URL-encoded form data
   - `multipart/form-data`: File uploads and form fields

##### `std::string build_response(const HttpResponse& response)`
**Purpose**: Converts HttpResponse to HTTP response string
**Parameters**:
- `response`: HttpResponse object to convert
**Returns**: Formatted HTTP response string

**Format**:
```
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: 42
Access-Control-Allow-Origin: *

{"message": "Hello World"}
```

##### `void parse_multipart_form_data(HttpRequest& request, const std::string& boundary)`
**Purpose**: Parses multipart form data for file uploads
**Parameters**:
- `request`: HttpRequest to populate
- `boundary`: Multipart boundary string

**Process**:
1. Splits body by boundary markers
2. Parses each part's headers
3. Extracts `Content-Disposition` for field names
4. Distinguishes between form fields and file uploads
5. Stores files in the `files` map
6. Stores form fields in the `form_data` map

##### `void parse_url_encoded_form_data(HttpRequest& request)`
**Purpose**: Parses URL-encoded form data
**Parameters**:
- `request`: HttpRequest to populate

**Process**:
1. Splits body by '&' delimiter
2. Splits each pair by '=' delimiter
3. URL-decodes keys and values
4. Stores in `form_data` map

##### `std::string url_decode(const std::string& str)`
**Purpose**: Decodes URL-encoded strings
**Parameters**:
- `str`: URL-encoded string
**Returns**: Decoded string

**Decoding Rules**:
- `%XX`: Hex-encoded characters
- `+`: Spaces
- Other characters: Pass-through

### DataStore Class

Thread-safe in-memory data storage system.

#### Constructor
```cpp
DataStore()
```
**Purpose**: Initializes empty data store
**Behavior**: Sets `next_id` to 1 for auto-incrementing IDs

#### Methods

##### `std::string create(const std::string& collection, const std::map<std::string, std::string>& item)`
**Purpose**: Creates a new item in a collection
**Parameters**:
- `collection`: Collection name (like a table name)
- `item`: Key-value data to store
**Returns**: Generated item ID

**Behavior**:
1. Thread-safe lock acquisition
2. Generates unique ID
3. Adds ID to item data
4. Stores in collection
5. Returns generated ID

**Thread Safety**: Uses mutex lock for atomic operations

##### `std::map<std::string, std::string> read(const std::string& collection, const std::string& id)`
**Purpose**: Retrieves a specific item by ID
**Parameters**:
- `collection`: Collection name
- `id`: Item ID
**Returns**: Item data or empty map if not found

##### `std::vector<std::map<std::string, std::string>> read_all(const std::string& collection)`
**Purpose**: Retrieves all items from a collection
**Parameters**:
- `collection`: Collection name
**Returns**: Vector of all items in collection

##### `bool update(const std::string& collection, const std::string& id, const std::map<std::string, std::string>& item)`
**Purpose**: Updates an existing item
**Parameters**:
- `collection`: Collection name
- `id`: Item ID
- `item`: Updated data
**Returns**: True if successful, false if item not found

**Behavior**:
1. Verifies item exists
2. Preserves original ID
3. Replaces item data
4. Returns success status

##### `bool remove(const std::string& collection, const std::string& id)`
**Purpose**: Deletes an item from collection
**Parameters**:
- `collection`: Collection name
- `id`: Item ID
**Returns**: True if successful, false if item not found

#### Data Structure
```cpp
private:
    std::map<std::string, std::map<std::string, std::map<std::string, std::string>>> data;
    std::mutex data_mutex;
    int next_id;
```

**Storage Format**:
- `data[collection][id][field] = value`
- Three-level nested map structure
- Thread-safe with mutex protection

## Function Documentation

### Route Handler Functions

#### CRUD Operations

##### `void handle_crud_create(const HttpRequest& request, HttpResponse& response)`
**Purpose**: Handles POST requests to create new items
**Path Pattern**: `/api/data/{collection}`
**Process**:
1. Extracts collection name from URL
2. Parses JSON or form data from request body
3. Creates item in data store
4. Returns JSON response with generated ID

**Request Body Formats**:
- JSON: `{"name": "John", "email": "john@example.com"}`
- Form: `name=John&email=john@example.com`

**Response Format**:
```json
{
  "id": "1",
  "status": "created"
}
```

##### `void handle_crud_read(const HttpRequest& request, HttpResponse& response)`
**Purpose**: Handles GET requests for specific items
**Path Pattern**: `/api/data/{collection}/{id}`
**Process**:
1. Extracts collection and ID from URL
2. Retrieves item from data store
3. Returns item data as JSON

**Response Format**:
```json
{
  "id": "1",
  "name": "John",
  "email": "john@example.com"
}
```

##### `void handle_crud_read_all(const HttpRequest& request, HttpResponse& response)`
**Purpose**: Handles GET requests for all items in collection
**Path Pattern**: `/api/data/{collection}`
**Process**:
1. Extracts collection name from URL
2. Retrieves all items from data store
3. Returns array of items as JSON

**Response Format**:
```json
[
  {"id": "1", "name": "John", "email": "john@example.com"},
  {"id": "2", "name": "Jane", "email": "jane@example.com"}
]
```

##### `void handle_crud_update(const HttpRequest& request, HttpResponse& response)`
**Purpose**: Handles PUT requests to update items
**Path Pattern**: `/api/data/{collection}/{id}`
**Process**:
1. Extracts collection and ID from URL
2. Parses updated data from request body
3. Updates item in data store
4. Returns success/failure status

**Response Format**:
```json
{
  "id": "1",
  "status": "updated"
}
```

##### `void handle_crud_delete(const HttpRequest& request, HttpResponse& response)`
**Purpose**: Handles DELETE requests to remove items
**Path Pattern**: `/api/data/{collection}/{id}`
**Process**:
1. Extracts collection and ID from URL
2. Removes item from data store
3. Returns success/failure status

**Response Format**:
```json
{
  "id": "1",
  "status": "deleted"
}
```

#### File Operations

##### `void handle_file_upload(const HttpRequest& request, HttpResponse& response)`
**Purpose**: Handles file upload requests
**Path Pattern**: `/api/files/upload`
**Process**:
1. Validates multipart form data
2. Extracts file data from request
3. Saves files to upload directory
4. Returns upload status

**Request Format**: Multipart form data with file fields

**Response Format**:
```json
{
  "uploaded_files": [
    {"filename": "document.pdf", "status": "uploaded"}
  ]
}
```

##### `void handle_file_download(const HttpRequest& request, HttpResponse& response)`
**Purpose**: Handles file download requests
**Path Pattern**: `/api/files/download/{filename}`
**Process**:
1. Extracts filename from URL
2. Reads file from upload directory
3. Sets appropriate content type
4. Returns file data with download headers

**Response**: Binary file data with `Content-Disposition: attachment`

##### `void handle_file_list(const HttpRequest& request, HttpResponse& response)`
**Purpose**: Lists all uploaded files
**Path Pattern**: `/api/files`
**Process**:
1. Scans upload directory
2. Collects file names
3. Returns list as JSON

**Response Format**:
```json
{
  "files": ["document.pdf", "image.jpg", "data.csv"]
}
```

##### `void handle_client_page(const HttpRequest& request, HttpResponse& response)`
**Purpose**: Serves the web client interface
**Path Pattern**: `/`
**Process**:
1. Reads `client.html` file
2. Returns HTML content
3. Sets proper content type

### Utility Functions

##### `std::string get_content_type(const std::string& filename)`
**Purpose**: Determines MIME type from file extension
**Parameters**:
- `filename`: File name with extension
**Returns**: MIME type string

**Supported Types**:
- `.html/.htm`: `text/html`
- `.css`: `text/css`
- `.js`: `application/javascript`
- `.json`: `application/json`
- `.png`: `image/png`
- `.jpg/.jpeg`: `image/jpeg`
- `.gif`: `image/gif`
- `.pdf`: `application/pdf`
- `.txt`: `text/plain`
- Default: `application/octet-stream`

##### `std::string extract_filename_from_path(const std::string& path)`
**Purpose**: Extracts filename from download URL path
**Parameters**:
- `path`: URL path like `/api/files/download/document.pdf`
**Returns**: Filename or empty string if not found

**Implementation**: Uses regex to match the pattern

##### `void send_json_response(HttpResponse& response, const std::string& json, int status = 200)`
**Purpose**: Helper to send JSON responses
**Parameters**:
- `response`: Response object to populate
- `json`: JSON string to send
- `status`: HTTP status code (default: 200)

**Headers Set**:
- `Content-Type: application/json`
- `Access-Control-Allow-Origin: *`

##### `void send_error_response(HttpResponse& response, int status, const std::string& message)`
**Purpose**: Helper to send error responses
**Parameters**:
- `response`: Response object to populate
- `status`: HTTP status code
- `message`: Error message

**Response Format**:
```json
{
  "error": "Error message here"
}
```

##### `void send_file_response(HttpResponse& response, const std::string& filepath)`
**Purpose**: Helper to send file responses
**Parameters**:
- `response`: Response object to populate
- `filepath`: Path to file to send

**Behavior**:
- For HTML files: Serves as text content
- For other files: Serves as binary download
- Sets appropriate content type
- Adds download headers for binary files

## API Endpoints

### CRUD Operations

| Method | Path | Description | Request Body | Response |
|--------|------|-------------|--------------|----------|
| `POST` | `/api/data/{collection}` | Create new item | JSON/Form data | `{"id": "1", "status": "created"}` |
| `GET` | `/api/data/{collection}` | Get all items | None | Array of items |
| `GET` | `/api/data/{collection}/{id}` | Get specific item | None | Item data |
| `PUT` | `/api/data/{collection}/{id}` | Update item | JSON/Form data | `{"id": "1", "status": "updated"}` |
| `DELETE` | `/api/data/{collection}/{id}` | Delete item | None | `{"id": "1", "status": "deleted"}` |

### File Operations

| Method | Path | Description | Request Body | Response |
|--------|------|-------------|--------------|----------|
| `POST` | `/api/files/upload` | Upload files | Multipart form data | Upload status |
| `GET` | `/api/files` | List files | None | File list |
| `GET` | `/api/files/download/{filename}` | Download file | None | File data |

### Web Interface

| Method | Path | Description | Response |
|--------|------|-------------|----------|
| `GET` | `/` | Web client interface | HTML page |

## Build System

### Makefile Structure

The project uses a GNU Make-based build system with the following targets:

#### Variables
```makefile
CXX = g++                                    # C++ compiler
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -pthread  # Compiler flags
SRCDIR = src                                 # Source directory
INCDIR = include                             # Include directory
OBJDIR = obj                                 # Object files directory
BINDIR = bin                                 # Binary directory
```

#### Targets

##### `all` (default)
**Purpose**: Builds the HTTP server executable
**Dependencies**: Object files and directories
**Output**: `bin/http_server`

##### `clean`
**Purpose**: Removes all build artifacts
**Files Removed**: `obj/` and `bin/` directories

##### `run`
**Purpose**: Builds and runs the server
**Dependencies**: `all` target
**Command**: `./bin/http_server`

##### `debug`
**Purpose**: Builds with debug symbols
**Flags Added**: `-g -DDEBUG`

##### `setup`
**Purpose**: Creates runtime directories
**Directories Created**: `uploads/`, `data/`

##### `install`
**Purpose**: Installs binary to system
**Location**: `/usr/local/bin/`
**Requirements**: sudo privileges

##### `uninstall`
**Purpose**: Removes installed binary
**Command**: `sudo rm -f /usr/local/bin/http_server`

### Build Process

1. **Compilation**: Each `.cpp` file is compiled to `.o` object file
2. **Linking**: Object files are linked to create the final executable
3. **Dependencies**: Automatic dependency tracking for headers

### Compiler Flags

- `-std=c++17`: C++17 standard compliance
- `-Wall -Wextra`: Comprehensive warnings
- `-O2`: Optimization level 2
- `-pthread`: POSIX threads support

## Testing

### Test Script (`test_api.sh`)

Comprehensive bash script that tests all server functionality.

#### Test Categories

1. **Server Status**: Verifies server is running
2. **CRUD Operations**: Tests create, read, update, delete
3. **File Operations**: Tests upload, download, list
4. **Error Handling**: Tests error responses
5. **Form Data**: Tests different content types
6. **Multiple Collections**: Tests data isolation

#### Test Process

1. **Setup**: Creates test data and files
2. **Execution**: Runs curl commands against server
3. **Verification**: Checks responses and compares files
4. **Cleanup**: Removes temporary files

#### Usage
```bash
chmod +x test_api.sh
./test_api.sh
```

#### Example Output
```
==================================
HTTP C++ Server API Test Script
==================================
Server URL: http://localhost:8080
Test Collection: users

✓ Server is running
✓ CRUD Operations tested
✓ File Upload/Download tested
✓ Error handling tested
✓ Form data handling tested
✓ Multiple collections tested
```

## Web Client (`client.html`)

### Overview

Full-featured web interface for testing server functionality.

### Features

#### Server Configuration
- Configurable server URL
- Server status checking
- Connection validation

#### CRUD Interface
- Collection management
- Item creation with JSON input
- Read operations (all items / specific item)
- Update operations
- Delete operations with confirmation

#### File Management
- File upload with drag-and-drop
- File listing with download links
- File download functionality
- Upload progress indication

#### Test Data
- Sample user data generation
- Sample product data generation
- Quick test data population

### JavaScript Architecture

#### Core Functions

##### `makeRequest(url, options)`
**Purpose**: Centralized HTTP request handling
**Features**:
- Automatic content type detection
- Error handling
- Network error detection
- Response parsing

##### `showResponse(elementId, data, isSuccess)`
**Purpose**: Displays API responses
**Features**:
- Color-coded success/error display
- JSON pretty-printing
- Response hiding/showing

#### API Integration

Each server endpoint has corresponding JavaScript functions:
- `createItem()`: POST to create items
- `getAllItems()`: GET to retrieve all items
- `getItem()`: GET to retrieve specific item
- `updateItem()`: PUT to update items
- `deleteItem()`: DELETE to remove items
- `uploadFile()`: POST to upload files
- `listFiles()`: GET to list files
- `downloadFile()`: GET to download files

### CSS Styling

- Responsive design
- Clean, modern interface
- Color-coded responses
- Accessible form controls
- Mobile-friendly layout

## Usage Examples

### Basic Server Setup

```cpp
#include "http_server.h"

int main() {
    HttpServer server(8080);
    
    // Add custom route
    server.add_route("GET", "/hello", [](const HttpRequest& req, HttpResponse& res) {
        res.body = "Hello, World!";
        res.headers["Content-Type"] = "text/plain";
    });
    
    server.start();
    return 0;
}
```

### Custom Route Handler

```cpp
server.add_route("GET", "/users/{id}/profile", [](const HttpRequest& req, HttpResponse& res) {
    // Extract path parameter
    std::regex pattern(R"(/users/([^/]+)/profile)");
    std::smatch matches;
    
    if (std::regex_match(req.path, matches, pattern)) {
        std::string userId = matches[1].str();
        
        // Process user profile request
        std::string json = "{\"user_id\":\"" + userId + "\",\"profile\":\"data\"}";
        server.send_json_response(res, json);
    }
});
```

### File Upload Handler

```cpp
server.add_route("POST", "/upload", [](const HttpRequest& req, HttpResponse& res) {
    if (req.files.empty()) {
        server.send_error_response(res, 400, "No files uploaded");
        return;
    }
    
    for (const auto& file : req.files) {
        std::string filename = file.second.filename;
        std::vector<char> data = file.second.data;
        
        // Save file
        std::ofstream outFile(filename, std::ios::binary);
        outFile.write(data.data(), data.size());
        outFile.close();
    }
    
    server.send_json_response(res, "{\"status\":\"success\"}");
});
```

### cURL Examples

```bash
# Create item
curl -X POST http://localhost:8080/api/data/users \
  -H "Content-Type: application/json" \
  -d '{"name":"John Doe","email":"john@example.com"}'

# Get all items
curl http://localhost:8080/api/data/users

# Get specific item
curl http://localhost:8080/api/data/users/1

# Update item
curl -X PUT http://localhost:8080/api/data/users/1 \
  -H "Content-Type: application/json" \
  -d '{"name":"John Updated","email":"john.updated@example.com"}'

# Delete item
curl -X DELETE http://localhost:8080/api/data/users/1

# Upload file
curl -X POST http://localhost:8080/api/files/upload \
  -F "file=@document.pdf"

# List files
curl http://localhost:8080/api/files

# Download file
curl http://localhost:8080/api/files/download/document.pdf -o downloaded.pdf
```

## Technical Details

### Threading Model

The server uses a **thread-per-request** model:

1. **Main Thread**: Accepts incoming connections
2. **Worker Threads**: Handle individual client requests
3. **Thread Safety**: Mutex protection for shared data

#### Thread Lifecycle
```
Accept Connection → Spawn Worker Thread → Handle Request → Send Response → Close Connection → Thread Ends
```

### Memory Management

- **RAII**: Resource management through destructors
- **Smart Pointers**: Automatic memory cleanup
- **Stack Allocation**: Most objects allocated on stack
- **No Memory Leaks**: Proper cleanup in destructors

### Error Handling

#### Network Errors
- Socket creation failures
- Bind/listen failures
- Connection acceptance errors
- Read/write timeouts

#### Application Errors
- Invalid JSON parsing
- File not found
- Permission denied
- Malformed requests

#### Error Response Format
```json
{
  "error": "Detailed error message",
  "code": "ERROR_CODE",
  "details": "Additional context"
}
```

### Performance Characteristics

#### Scalability
- **Concurrent Connections**: Limited by system thread limits
- **Memory Usage**: Minimal per-request overhead
- **CPU Usage**: Efficient parsing and routing

#### Benchmarks (Approximate)
- **Request/Response**: ~1-2ms overhead
- **File Upload**: ~10MB/s on local filesystem
- **JSON Processing**: ~1000 requests/second

### Security Considerations

#### Current Implementation
- **CORS Headers**: Allows cross-origin requests
- **File Access**: Limited to upload directory
- **Input Validation**: Basic parameter checking

#### Recommendations for Production
- **Authentication**: Add user authentication
- **Authorization**: Implement access control
- **Input Sanitization**: Validate all user input
- **Rate Limiting**: Prevent abuse
- **HTTPS**: SSL/TLS encryption
- **File Validation**: Check file types and sizes

### Configuration Options

#### Compile-time Configuration
- **Port**: Default 8080, configurable via constructor
- **Upload Directory**: Hardcoded as "uploads/"
- **Buffer Sizes**: 8KB read buffer

#### Runtime Configuration
- **Command Line**: Port number via argv[1]
- **Environment Variables**: None currently supported
- **Config Files**: None currently supported

### Dependencies

#### System Requirements
- **OS**: Linux/Unix-like system
- **Compiler**: G++ with C++17 support
- **Libraries**: Standard C++ library, POSIX threads

#### C++ Standard Library Usage
- `<iostream>`: Input/output operations
- `<string>`: String manipulation
- `<map>`: Key-value storage
- `<vector>`: Dynamic arrays
- `<thread>`: Threading support
- `<mutex>`: Thread synchronization
- `<filesystem>`: File system operations
- `<regex>`: Regular expressions
- `<fstream>`: File I/O
- `<sstream>`: String streams

#### System Libraries
- **Socket API**: BSD socket functions
- **POSIX Threads**: pthread library
- **File System**: POSIX file operations

### Future Enhancements

#### Planned Features
1. **Database Integration**: SQLite or MySQL support
2. **Authentication**: JWT token-based auth
3. **WebSocket Support**: Real-time communication
4. **Static File Serving**: Efficient static content
5. **Request Logging**: Comprehensive logging
6. **Configuration Files**: JSON/YAML config support
7. **Plugin System**: Extensible architecture
8. **Caching**: Response caching mechanisms

#### Performance Improvements
1. **Connection Pooling**: Reuse connections
2. **Asynchronous I/O**: Non-blocking operations
3. **HTTP/2 Support**: Modern HTTP protocol
4. **Compression**: Gzip response compression
5. **Load Balancing**: Multi-instance support

---

This documentation provides a comprehensive understanding of the HTTP C++ Server codebase. Each component is designed to be modular, maintainable, and extensible. The server demonstrates solid software engineering principles including proper error handling, thread safety, and clean architecture.

For additional information or contributions, please refer to the project repository and follow the established coding standards.