#include "../include/http_server.h"
#include <iostream>
#include <signal.h>
#include <thread>
#include <chrono>

HttpServer* server = nullptr;

void signal_handler(int) {
    if (server) {
        std::cout << "\nShutting down server..." << std::endl;
        server->stop();
        delete server;
        server = nullptr;
    }
    exit(0);
}

int main(int argc, char* argv[]) {
    int port = 8080;
    
    // Parse command line arguments for port
    if (argc > 1) {
        try {
            port = std::stoi(argv[1]);
        } catch (const std::exception& e) {
            std::cerr << "Invalid port number. Using default port 8080." << std::endl;
        }
    }
    
    // Set up signal handlers for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    std::cout << "Starting HTTP API Server..." << std::endl;
    std::cout << "Port: " << port << std::endl;
    std::cout << "Available endpoints:" << std::endl;
    std::cout << "  CRUD Operations:" << std::endl;
    std::cout << "    POST   /api/data/{collection}     - Create new item" << std::endl;
    std::cout << "    GET    /api/data/{collection}     - Get all items" << std::endl;
    std::cout << "    GET    /api/data/{collection}/{id} - Get specific item" << std::endl;
    std::cout << "    PUT    /api/data/{collection}/{id} - Update item" << std::endl;
    std::cout << "    DELETE /api/data/{collection}/{id} - Delete item" << std::endl;
    std::cout << "  File Operations:" << std::endl;
    std::cout << "    POST   /api/files/upload         - Upload files" << std::endl;
    std::cout << "    GET    /api/files                - List uploaded files" << std::endl;
    std::cout << "    GET    /api/files/download/{filename} - Download file" << std::endl;
    std::cout << "  Web Interface:" << std::endl;
    std::cout << "    GET    /                         - API test client" << std::endl;
    std::cout << "\nPress Ctrl+C to stop the server" << std::endl;
    std::cout << "Open http://localhost:" << port << " in your browser to use the web client\n" << std::endl;
    
    server = new HttpServer(port);
    server->start();
    
    // Keep the main thread alive
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return 0;
}