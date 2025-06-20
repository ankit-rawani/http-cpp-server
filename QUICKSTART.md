# HTTP C++ Server - Quick Start Guide

## Overview

A lightweight HTTP API server written in C++ that provides:
- **CRUD Operations** - Create, Read, Update, Delete data in collections
- **File Upload/Download** - Handle file operations with multipart form data
- **RESTful API** - JSON responses with proper HTTP status codes
- **Multi-threaded** - Concurrent request handling

## Quick Start (5 minutes)

### 1. Build the Server
```bash
cd http-cpp-server
make setup    # Create necessary directories
make          # Build the server
```

### 2. Run the Server
```bash
./bin/http_server        # Default port 8080
# or
./bin/http_server 3000   # Custom port
```

### 3. Test with curl
```bash
# Create a user
curl -X POST http://localhost:8080/api/data/users \
  -H "Content-Type: application/json" \
  -d '{"name":"John Doe","email":"john@example.com"}'

# Get all users
curl http://localhost:8080/api/data/users

# Upload a file
echo "Hello World" > test.txt
curl -X POST http://localhost:8080/api/files/upload -F "file=@test.txt"

# List files
curl http://localhost:8080/api/files
```

### 4. Use the Built-in Web Client
Open `http://localhost:8080` in your browser for a GUI interface to test all API endpoints.

## API Endpoints Summary

| Method | Endpoint | Description |
|--------|----------|-------------|
| POST | `/api/data/{collection}` | Create new item |
| GET | `/api/data/{collection}` | Get all items in collection |
| GET | `/api/data/{collection}/{id}` | Get specific item |
| PUT | `/api/data/{collection}/{id}` | Update item |
| DELETE | `/api/data/{collection}/{id}` | Delete item |
| POST | `/api/files/upload` | Upload file(s) |
| GET | `/api/files` | List uploaded files |
| GET | `/api/files/download/{filename}` | Download file |
| GET | `/` | Built-in web client interface |

## Complete Testing

Run the comprehensive test script:
```bash
./test_api.sh
```

This will test all endpoints and demonstrate the full functionality.

## Project Structure

```
http-cpp-server/
├── bin/http_server      # Compiled executable
├── src/                 # Source code
├── include/             # Header files
├── uploads/             # File upload directory
├── client.html          # Web-based API client (served at /)
├── test_api.sh          # Comprehensive test script
├── README.md            # Detailed documentation
└── Makefile             # Build configuration
```

## Key Features Demonstrated

1. **Collections**: Automatic creation, no schema required
2. **JSON API**: All responses in JSON format with proper HTTP codes
3. **File Handling**: Binary file upload/download with multipart parsing
4. **Thread Safety**: Concurrent request handling with mutex protection
5. **CORS Support**: Cross-origin requests enabled for web clients

## Next Steps

- Open `http://localhost:8080` in your browser to use the web interface
- Read `README.md` for detailed API documentation
- Check `config.md` for advanced configuration options
- Modify source code for custom business logic
- Add authentication, validation, or persistence as needed

## Development Notes

- Data is stored in-memory (resets on server restart)
- No authentication by default
- Files stored in `uploads/` directory
- Thread-per-request model for simplicity
- C++17 required for compilation

## Common Use Cases

- **Rapid Prototyping**: Quick REST API for testing
- **Learning Tool**: Understanding HTTP server implementation
- **Microservice**: Lightweight service component
- **File Server**: Simple file upload/download service
- **Data API**: JSON-based data operations

The server is ready to use immediately after building - no additional configuration required!