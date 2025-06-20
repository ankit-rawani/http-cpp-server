# HTTP C++ Server

A simple HTTP API server written in C++ that provides basic CRUD operations, file upload/download functionality, and standard HTTP methods support.

## Features

- **CRUD Operations**: Create, Read, Update, Delete operations for data collections
- **File Upload/Download**: Upload files via multipart form data and download them
- **HTTP Methods**: Support for GET, POST, PUT, DELETE
- **JSON Responses**: All API responses are in JSON format
- **Multi-threaded**: Handles multiple concurrent connections
- **Cross-Origin Support**: CORS headers included for web client compatibility

## Building the Project

### Prerequisites

- C++17 compatible compiler (GCC 7+ or Clang 5+)
- Make
- Linux/Unix system (uses POSIX sockets)

### Build Instructions

```bash
# Clone or navigate to the project directory
cd http-cpp-server

# Create necessary directories
make setup

# Build the project
make

# Run the server
make run
```

### Alternative Build Commands

```bash
# Debug build with symbols
make debug

# Clean build artifacts
make clean

# Install to system (requires sudo)
make install
```

## Usage

### Starting the Server

```bash
# Default port (8080)
./bin/http_server

# Custom port
./bin/http_server 3000
```

Once started, you can:
- Use the **built-in web client** by opening `http://localhost:8080` in your browser
- Make API calls directly to the endpoints listed below

### API Endpoints

#### CRUD Operations

All CRUD operations work with collections and items. Collections are created automatically when you first add an item.

**Create Item**
```http
POST /api/data/{collection}
Content-Type: application/json

{"name": "John Doe", "email": "john@example.com"}
```

**Get All Items**
```http
GET /api/data/{collection}
```

**Get Specific Item**
```http
GET /api/data/{collection}/{id}
```

**Update Item**
```http
PUT /api/data/{collection}/{id}
Content-Type: application/json

{"name": "Jane Doe", "email": "jane@example.com"}
```

**Delete Item**
```http
DELETE /api/data/{collection}/{id}
```

#### File Operations

**Upload File**
```http
POST /api/files/upload
Content-Type: multipart/form-data

# Include file in form data with any field name
```

**List Files**
```http
GET /api/files
```

**Download File**
```http
GET /api/files/download/{filename}
```

#### Web Interface

**API Test Client**
```http
GET /
```

## API Examples

### Using curl

#### CRUD Examples

```bash
# Create a user
curl -X POST http://localhost:8080/api/data/users \
  -H "Content-Type: application/json" \
  -d '{"name":"John Doe","email":"john@example.com","age":"30"}'

# Get all users
curl http://localhost:8080/api/data/users

# Get specific user (replace '1' with actual ID)
curl http://localhost:8080/api/data/users/1

# Update user
curl -X PUT http://localhost:8080/api/data/users/1 \
  -H "Content-Type: application/json" \
  -d '{"name":"John Smith","email":"johnsmith@example.com","age":"31"}'

# Delete user
curl -X DELETE http://localhost:8080/api/data/users/1
```

#### File Upload/Download Examples

```bash
# Upload a file
curl -X POST http://localhost:8080/api/files/upload \
  -F "file=@/path/to/your/file.txt"

# List uploaded files
curl http://localhost:8080/api/files

# Download a file
curl -O http://localhost:8080/api/files/download/file.txt
```

### Using the Built-in Web Client

The server includes a built-in web interface for easy testing:

1. Start the server: `./bin/http_server`
2. Open your browser to `http://localhost:8080`
3. Use the web interface to test all API endpoints with a GUI

The web client provides forms for all CRUD operations, file upload/download, and displays responses in a user-friendly format.

### Using JavaScript (Browser/Node.js)

#### CRUD Operations

```javascript
// Create item
fetch('http://localhost:8080/api/data/products', {
  method: 'POST',
  headers: {
    'Content-Type': 'application/json',
  },
  body: JSON.stringify({
    name: 'Laptop',
    price: '999',
    category: 'Electronics'
  })
})
.then(response => response.json())
.then(data => console.log(data));

// Get all items
fetch('http://localhost:8080/api/data/products')
  .then(response => response.json())
  .then(data => console.log(data));

// Update item
fetch('http://localhost:8080/api/data/products/1', {
  method: 'PUT',
  headers: {
    'Content-Type': 'application/json',
  },
  body: JSON.stringify({
    name: 'Gaming Laptop',
    price: '1299',
    category: 'Electronics'
  })
})
.then(response => response.json())
.then(data => console.log(data));
```

#### File Upload

```javascript
// File upload with HTML form
const formData = new FormData();
const fileInput = document.querySelector('input[type="file"]');
formData.append('file', fileInput.files[0]);

fetch('http://localhost:8080/api/files/upload', {
  method: 'POST',
  body: formData
})
.then(response => response.json())
.then(data => console.log(data));
```

## Response Formats

### Success Responses

**Create Item**
```json
{
  "id": "1",
  "status": "created"
}
```

**Get Item**
```json
{
  "id": "1",
  "name": "John Doe",
  "email": "john@example.com"
}
```

**Get All Items**
```json
[
  {
    "id": "1",
    "name": "John Doe",
    "email": "john@example.com"
  },
  {
    "id": "2",
    "name": "Jane Smith",
    "email": "jane@example.com"
  }
]
```

**File Upload**
```json
{
  "uploaded_files": [
    {
      "filename": "document.pdf",
      "status": "uploaded"
    }
  ]
}
```

**File List**
```json
{
  "files": [
    "document.pdf",
    "image.jpg",
    "data.txt"
  ]
}
```

### Error Responses

```json
{
  "error": "Item not found"
}
```

## Project Structure

```
http-cpp-server/
├── src/
│   ├── main.cpp           # Main application entry point
│   └── http_server.cpp    # HTTP server implementation
├── include/
│   └── http_server.h      # HTTP server header file
├── uploads/               # Directory for uploaded files
├── data/                  # Directory for data storage
├── obj/                   # Object files (created during build)
├── bin/                   # Binary executable (created during build)
├── Makefile              # Build configuration
└── README.md             # This file
```

## Features Details

### Data Storage
- In-memory storage using STL containers
- Thread-safe operations with mutex protection
- Automatic ID generation for new items
- Collection-based organization

### File Handling
- Multipart form data parsing for file uploads
- Binary file support
- Automatic MIME type detection
- Secure file download with proper headers

### HTTP Features
- Custom HTTP request/response parsing
- Support for query parameters
- Form data parsing (URL-encoded and multipart)
- Basic routing with parameter extraction
- CORS support for web applications

## Limitations

- **In-memory storage**: Data is not persisted between server restarts
- **Basic authentication**: No built-in authentication or authorization
- **Single-threaded per request**: Each request is handled in a separate thread
- **Limited HTTP features**: Basic implementation without advanced HTTP features
- **No HTTPS**: Only HTTP is supported

## Future Enhancements

- Persistent storage (SQLite, JSON files)
- Authentication and authorization
- Request logging
- Configuration file support
- HTTPS/TLS support
- Request validation
- Rate limiting
- WebSocket support

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## License

This project is provided as-is for educational and development purposes.