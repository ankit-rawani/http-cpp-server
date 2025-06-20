#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <memory>
#include <thread>
#include <mutex>
#include <sstream>

// HTTP Request structure
struct HttpRequest {
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;
    std::map<std::string, std::string> query_params;
    std::map<std::string, std::string> form_data;
    
    // File upload data
    struct FileData {
        std::string filename;
        std::string content_type;
        std::vector<char> data;
    };
    std::map<std::string, FileData> files;
};

// HTTP Response structure
struct HttpResponse {
    int status_code;
    std::string status_text;
    std::map<std::string, std::string> headers;
    std::string body;
    std::vector<char> binary_data;
    bool is_binary;
    
    HttpResponse() : status_code(200), status_text("OK"), is_binary(false) {}
};

// Route handler function type
using RouteHandler = std::function<void(const HttpRequest&, HttpResponse&)>;

// Simple data store for CRUD operations
class DataStore {
private:
    std::map<std::string, std::map<std::string, std::map<std::string, std::string>>> data;
    std::mutex data_mutex;
    int next_id;

public:
    DataStore() : next_id(1) {}
    
    std::string create(const std::string& collection, const std::map<std::string, std::string>& item);
    std::map<std::string, std::string> read(const std::string& collection, const std::string& id);
    std::vector<std::map<std::string, std::string>> read_all(const std::string& collection);
    bool update(const std::string& collection, const std::string& id, const std::map<std::string, std::string>& item);
    bool remove(const std::string& collection, const std::string& id);
};

// HTTP Server class
class HttpServer {
private:
    int port;
    int server_socket;
    bool running;
    std::vector<std::thread> worker_threads;
    std::map<std::string, std::map<std::string, RouteHandler>> routes;
    DataStore data_store;
    
    // Helper methods
    void start_listening();
    void handle_client(int client_socket);
    HttpRequest parse_request(const std::string& request_str);
    std::string build_response(const HttpResponse& response);
    void parse_multipart_form_data(HttpRequest& request, const std::string& boundary);
    void parse_url_encoded_form_data(HttpRequest& request);
    std::string url_decode(const std::string& str);
    std::string get_content_type(const std::string& filename);
    std::string extract_filename_from_path(const std::string& path);
    
    // Built-in route handlers
    void handle_crud_create(const HttpRequest& request, HttpResponse& response);
    void handle_crud_read(const HttpRequest& request, HttpResponse& response);
    void handle_crud_read_all(const HttpRequest& request, HttpResponse& response);
    void handle_crud_update(const HttpRequest& request, HttpResponse& response);
    void handle_crud_delete(const HttpRequest& request, HttpResponse& response);
    void handle_file_upload(const HttpRequest& request, HttpResponse& response);
    void handle_file_download(const HttpRequest& request, HttpResponse& response);
    void handle_file_list(const HttpRequest& request, HttpResponse& response);
    void handle_client_page(const HttpRequest& request, HttpResponse& response);

public:
    HttpServer(int port = 8080);
    ~HttpServer();
    
    // Route registration
    void add_route(const std::string& method, const std::string& path, RouteHandler handler);
    void setup_default_routes();
    
    // Server control
    void start();
    void stop();
    
    // Utility methods
    void send_json_response(HttpResponse& response, const std::string& json, int status = 200);
    void send_error_response(HttpResponse& response, int status, const std::string& message);
    void send_file_response(HttpResponse& response, const std::string& filepath);
};

#endif // HTTP_SERVER_H