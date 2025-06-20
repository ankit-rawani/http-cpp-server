#include "../include/http_server.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include <filesystem>
#include <regex>

// DataStore implementation
std::string DataStore::create(const std::string& collection, const std::map<std::string, std::string>& item) {
    std::lock_guard<std::mutex> lock(data_mutex);
    std::string id = std::to_string(next_id++);
    
    std::map<std::string, std::string> new_item = item;
    new_item["id"] = id;
    data[collection][id] = new_item;
    
    return id;
}

std::map<std::string, std::string> DataStore::read(const std::string& collection, const std::string& id) {
    std::lock_guard<std::mutex> lock(data_mutex);
    
    if (data.find(collection) != data.end() && data[collection].find(id) != data[collection].end()) {
        return data[collection][id];
    }
    
    return std::map<std::string, std::string>();
}

std::vector<std::map<std::string, std::string>> DataStore::read_all(const std::string& collection) {
    std::lock_guard<std::mutex> lock(data_mutex);
    std::vector<std::map<std::string, std::string>> result;
    
    if (data.find(collection) != data.end()) {
        for (const auto& pair : data[collection]) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

bool DataStore::update(const std::string& collection, const std::string& id, const std::map<std::string, std::string>& item) {
    std::lock_guard<std::mutex> lock(data_mutex);
    
    if (data.find(collection) != data.end() && data[collection].find(id) != data[collection].end()) {
        std::map<std::string, std::string> updated_item = item;
        updated_item["id"] = id;
        data[collection][id] = updated_item;
        return true;
    }
    
    return false;
}

bool DataStore::remove(const std::string& collection, const std::string& id) {
    std::lock_guard<std::mutex> lock(data_mutex);
    
    if (data.find(collection) != data.end() && data[collection].find(id) != data[collection].end()) {
        data[collection].erase(id);
        return true;
    }
    
    return false;
}

// HttpServer implementation
HttpServer::HttpServer(int port) : port(port), server_socket(-1), running(false) {}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::add_route(const std::string& method, const std::string& path, RouteHandler handler) {
    routes[method][path] = handler;
}

void HttpServer::setup_default_routes() {
    // CRUD routes
    add_route("POST", "/api/data/{collection}", [this](const HttpRequest& req, HttpResponse& res) {
        handle_crud_create(req, res);
    });
    
    add_route("GET", "/api/data/{collection}/{id}", [this](const HttpRequest& req, HttpResponse& res) {
        handle_crud_read(req, res);
    });
    
    add_route("GET", "/api/data/{collection}", [this](const HttpRequest& req, HttpResponse& res) {
        handle_crud_read_all(req, res);
    });
    
    add_route("PUT", "/api/data/{collection}/{id}", [this](const HttpRequest& req, HttpResponse& res) {
        handle_crud_update(req, res);
    });
    
    add_route("DELETE", "/api/data/{collection}/{id}", [this](const HttpRequest& req, HttpResponse& res) {
        handle_crud_delete(req, res);
    });
    
    // File upload/download routes
    add_route("POST", "/api/files/upload", [this](const HttpRequest& req, HttpResponse& res) {
        handle_file_upload(req, res);
    });
    
    add_route("GET", "/api/files/download/{filename}", [this](const HttpRequest& req, HttpResponse& res) {
        handle_file_download(req, res);
    });
    
    add_route("GET", "/api/files", [this](const HttpRequest& req, HttpResponse& res) {
        handle_file_list(req, res);
    });
    
    // Static file route for client
    add_route("GET", "/", [this](const HttpRequest& req, HttpResponse& res) {
        handle_client_page(req, res);
    });
}

void HttpServer::start() {
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }
    
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Failed to bind socket" << std::endl;
        close(server_socket);
        return;
    }
    
    if (listen(server_socket, 10) == -1) {
        std::cerr << "Failed to listen on socket" << std::endl;
        close(server_socket);
        return;
    }
    
    running = true;
    std::cout << "HTTP Server started on port " << port << std::endl;
    
    setup_default_routes();
    start_listening();
}

void HttpServer::stop() {
    running = false;
    
    if (server_socket != -1) {
        close(server_socket);
        server_socket = -1;
    }
    
    for (auto& thread : worker_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    worker_threads.clear();
}

void HttpServer::start_listening() {
    while (running) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_len);
        if (client_socket == -1) {
            if (running) {
                std::cerr << "Failed to accept client connection" << std::endl;
            }
            continue;
        }
        
        worker_threads.emplace_back([this, client_socket]() {
            handle_client(client_socket);
        });
    }
}

void HttpServer::handle_client(int client_socket) {
    std::string request_str;
    char buffer[8192];
    ssize_t bytes_received;
    
    // Read the initial request
    bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        close(client_socket);
        return;
    }
    
    buffer[bytes_received] = '\0';
    request_str = std::string(buffer);
    
    // Check if there's a Content-Length header to read more data
    size_t content_length_pos = request_str.find("Content-Length:");
    if (content_length_pos != std::string::npos) {
        size_t line_end = request_str.find("\r\n", content_length_pos);
        if (line_end != std::string::npos) {
            std::string length_str = request_str.substr(content_length_pos + 15, line_end - content_length_pos - 15);
            // Trim whitespace
            length_str.erase(0, length_str.find_first_not_of(" \t"));
            length_str.erase(length_str.find_last_not_of(" \t\r\n") + 1);
            
            size_t content_length;
            try {
                content_length = std::stoul(length_str);
            } catch (const std::exception&) {
                content_length = 0;
            }
            
            // Find where headers end
            size_t header_end = request_str.find("\r\n\r\n");
            if (header_end != std::string::npos) {
                size_t headers_size = header_end + 4;
                size_t body_received = request_str.length() - headers_size;
                
                // Read remaining body data if needed
                while (body_received < content_length) {
                    bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
                    if (bytes_received <= 0) break;
                    
                    buffer[bytes_received] = '\0';
                    request_str += std::string(buffer);
                    body_received += bytes_received;
                }
            }
        }
    }
    
    HttpRequest request = parse_request(request_str);
    HttpResponse response;
    
    // Find matching route
    bool route_found = false;
    for (const auto& method_routes : routes) {
        if (method_routes.first == request.method) {
            for (const auto& route : method_routes.second) {
                std::string pattern = route.first;
                
                // Simple pattern matching for {param} style routes
                std::regex param_regex(R"(\{[^}]+\})");
                std::string regex_pattern = std::regex_replace(pattern, param_regex, "([^/]+)");
                regex_pattern = "^" + regex_pattern + "$";
                
                std::regex route_regex(regex_pattern);
                std::smatch matches;
                
                if (std::regex_match(request.path, matches, route_regex)) {
                    route.second(request, response);
                    route_found = true;
                    break;
                }
            }
            if (route_found) break;
        }
    }
    
    if (!route_found) {
        send_error_response(response, 404, "Not Found");
    }
    
    std::string response_str = build_response(response);
    send(client_socket, response_str.c_str(), response_str.length(), 0);
    
    if (response.is_binary && !response.binary_data.empty()) {
        send(client_socket, response.binary_data.data(), response.binary_data.size(), 0);
    }
    
    close(client_socket);
}

HttpRequest HttpServer::parse_request(const std::string& request_str) {
    HttpRequest request;
    std::istringstream iss(request_str);
    std::string line;
    
    // Parse request line
    if (std::getline(iss, line)) {
        std::istringstream request_line(line);
        request_line >> request.method >> request.path >> request.version;
        
        // Parse query parameters
        size_t query_pos = request.path.find('?');
        if (query_pos != std::string::npos) {
            std::string query_string = request.path.substr(query_pos + 1);
            request.path = request.path.substr(0, query_pos);
            
            std::istringstream query_stream(query_string);
            std::string param;
            while (std::getline(query_stream, param, '&')) {
                size_t eq_pos = param.find('=');
                if (eq_pos != std::string::npos) {
                    std::string key = url_decode(param.substr(0, eq_pos));
                    std::string value = url_decode(param.substr(eq_pos + 1));
                    request.query_params[key] = value;
                }
            }
        }
    }
    
    // Parse headers
    while (std::getline(iss, line) && line != "\r" && !line.empty()) {
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = line.substr(0, colon_pos);
            std::string value = line.substr(colon_pos + 1);
            
            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t\r\n") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t\r\n") + 1);
            
            request.headers[key] = value;
        }
    }
    
    // Parse body
    std::string body;
    std::string body_line;
    while (std::getline(iss, body_line)) {
        body += body_line + "\n";
    }
    if (!body.empty()) {
        body.pop_back(); // Remove last newline
    }
    request.body = body;
    
    // Parse form data based on content type
    auto content_type_it = request.headers.find("Content-Type");
    if (content_type_it != request.headers.end()) {
        std::string content_type = content_type_it->second;
        
        if (content_type.find("multipart/form-data") != std::string::npos) {
            size_t boundary_pos = content_type.find("boundary=");
            if (boundary_pos != std::string::npos) {
                std::string boundary = content_type.substr(boundary_pos + 9);
                // Remove any trailing whitespace, semicolons, or quotes
                size_t end_pos = boundary.find_first_of("; \t\r\n\"");
                if (end_pos != std::string::npos) {
                    boundary = boundary.substr(0, end_pos);
                }
                // Remove leading quotes if present
                if (!boundary.empty() && boundary[0] == '"') {
                    boundary = boundary.substr(1);
                }
                if (!boundary.empty() && boundary.back() == '"') {
                    boundary = boundary.substr(0, boundary.length() - 1);
                }
                parse_multipart_form_data(request, boundary);
            }
        } else if (content_type.find("application/x-www-form-urlencoded") != std::string::npos) {
            parse_url_encoded_form_data(request);
        }
    }
    
    return request;
}

std::string HttpServer::build_response(const HttpResponse& response) {
    std::ostringstream oss;
    
    oss << "HTTP/1.1 " << response.status_code << " " << response.status_text << "\r\n";
    
    for (const auto& header : response.headers) {
        oss << header.first << ": " << header.second << "\r\n";
    }
    
    if (response.is_binary) {
        oss << "Content-Length: " << response.binary_data.size() << "\r\n";
    } else {
        oss << "Content-Length: " << response.body.length() << "\r\n";
    }
    
    oss << "\r\n";
    
    if (!response.is_binary) {
        oss << response.body;
    }
    
    return oss.str();
}

void HttpServer::parse_multipart_form_data(HttpRequest& request, const std::string& boundary) {
    std::string delimiter = "--" + boundary;
    std::string body = request.body;
    
    size_t pos = 0;
    while ((pos = body.find(delimiter, pos)) != std::string::npos) {
        pos += delimiter.length();
        
        if (pos >= body.length() || body.substr(pos, 2) == "--") {
            break;
        }
        
        size_t header_end = body.find("\r\n\r\n", pos);
        if (header_end == std::string::npos) {
            break;
        }
        
        std::string headers = body.substr(pos, header_end - pos);
        pos = header_end + 4;
        
        size_t content_end = body.find(delimiter, pos);
        if (content_end == std::string::npos) {
            break;
        }
        
        std::string content = body.substr(pos, content_end - pos - 2); // Remove \r\n
        
        // Parse headers
        std::istringstream header_stream(headers);
        std::string header_line;
        std::string name;
        std::string filename;
        std::string content_type;
        
        while (std::getline(header_stream, header_line)) {
            if (header_line.find("Content-Disposition:") != std::string::npos) {
                size_t name_pos = header_line.find("name=\"");
                if (name_pos != std::string::npos) {
                    name_pos += 6;
                    size_t name_end = header_line.find("\"", name_pos);
                    if (name_end != std::string::npos) {
                        name = header_line.substr(name_pos, name_end - name_pos);
                    }
                }
                
                size_t filename_pos = header_line.find("filename=\"");
                if (filename_pos != std::string::npos) {
                    filename_pos += 10;
                    size_t filename_end = header_line.find("\"", filename_pos);
                    if (filename_end != std::string::npos) {
                        filename = header_line.substr(filename_pos, filename_end - filename_pos);
                    }
                }
            } else if (header_line.find("Content-Type:") != std::string::npos) {
                content_type = header_line.substr(13);
                content_type.erase(0, content_type.find_first_not_of(" \t"));
                content_type.erase(content_type.find_last_not_of(" \t\r\n") + 1);
            }
        }
        
        if (!filename.empty()) {
            // File upload
            HttpRequest::FileData file_data;
            file_data.filename = filename;
            file_data.content_type = content_type;
            file_data.data = std::vector<char>(content.begin(), content.end());
            request.files[name] = file_data;
        } else {
            // Regular form field
            request.form_data[name] = content;
        }
        
        pos = content_end;
    }
}

void HttpServer::parse_url_encoded_form_data(HttpRequest& request) {
    std::istringstream iss(request.body);
    std::string param;
    
    while (std::getline(iss, param, '&')) {
        size_t eq_pos = param.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = url_decode(param.substr(0, eq_pos));
            std::string value = url_decode(param.substr(eq_pos + 1));
            request.form_data[key] = value;
        }
    }
}

std::string HttpServer::url_decode(const std::string& str) {
    std::string decoded;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            int hex_value;
            std::istringstream hex_stream(str.substr(i + 1, 2));
            hex_stream >> std::hex >> hex_value;
            decoded += static_cast<char>(hex_value);
            i += 2;
        } else if (str[i] == '+') {
            decoded += ' ';
        } else {
            decoded += str[i];
        }
    }
    return decoded;
}

std::string HttpServer::get_content_type(const std::string& filename) {
    std::string ext = filename.substr(filename.find_last_of('.') + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == "html" || ext == "htm") return "text/html";
    if (ext == "css") return "text/css";
    if (ext == "js") return "application/javascript";
    if (ext == "json") return "application/json";
    if (ext == "png") return "image/png";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "gif") return "image/gif";
    if (ext == "pdf") return "application/pdf";
    if (ext == "txt") return "text/plain";
    
    return "application/octet-stream";
}

std::string HttpServer::extract_filename_from_path(const std::string& path) {
    std::regex filename_regex(R"(/api/files/download/([^/]+))");
    std::smatch matches;
    
    if (std::regex_match(path, matches, filename_regex)) {
        return matches[1].str();
    }
    
    return "";
}

// CRUD handlers
void HttpServer::handle_crud_create(const HttpRequest& request, HttpResponse& response) {
    std::regex collection_regex(R"(/api/data/([^/]+))");
    std::smatch matches;
    
    if (std::regex_match(request.path, matches, collection_regex)) {
        std::string collection = matches[1].str();
        
        // Parse JSON body (simple implementation)
        std::map<std::string, std::string> item;
        
        // For simplicity, we'll use form data or a simple key=value format
        if (!request.form_data.empty()) {
            item = request.form_data;
        } else {
            // Simple JSON-like parsing (very basic)
            std::string body = request.body;
            if (body.find('{') != std::string::npos && body.find('}') != std::string::npos) {
                // Remove braces and split by commas
                body = body.substr(1, body.length() - 2);
                std::istringstream iss(body);
                std::string pair;
                
                while (std::getline(iss, pair, ',')) {
                    size_t colon_pos = pair.find(':');
                    if (colon_pos != std::string::npos) {
                        std::string key = pair.substr(0, colon_pos);
                        std::string value = pair.substr(colon_pos + 1);
                        
                        // Remove quotes and whitespace
                        key.erase(std::remove_if(key.begin(), key.end(), [](char c) {
                            return c == '"' || c == ' ' || c == '\t';
                        }), key.end());
                        
                        value.erase(std::remove_if(value.begin(), value.end(), [](char c) {
                            return c == '"' || c == ' ' || c == '\t';
                        }), value.end());
                        
                        item[key] = value;
                    }
                }
            }
        }
        
        std::string id = data_store.create(collection, item);
        std::string json_response = "{\"id\":\"" + id + "\",\"status\":\"created\"}";
        send_json_response(response, json_response, 201);
    } else {
        send_error_response(response, 400, "Invalid collection path");
    }
}

void HttpServer::handle_crud_read(const HttpRequest& request, HttpResponse& response) {
    std::regex item_regex(R"(/api/data/([^/]+)/([^/]+))");
    std::smatch matches;
    
    if (std::regex_match(request.path, matches, item_regex)) {
        std::string collection = matches[1].str();
        std::string id = matches[2].str();
        
        auto item = data_store.read(collection, id);
        if (!item.empty()) {
            std::string json_response = "{";
            bool first = true;
            for (const auto& pair : item) {
                if (!first) json_response += ",";
                json_response += "\"" + pair.first + "\":\"" + pair.second + "\"";
                first = false;
            }
            json_response += "}";
            send_json_response(response, json_response);
        } else {
            send_error_response(response, 404, "Item not found");
        }
    } else {
        send_error_response(response, 400, "Invalid item path");
    }
}

void HttpServer::handle_crud_read_all(const HttpRequest& request, HttpResponse& response) {
    std::regex collection_regex(R"(/api/data/([^/]+))");
    std::smatch matches;
    
    if (std::regex_match(request.path, matches, collection_regex)) {
        std::string collection = matches[1].str();
        
        auto items = data_store.read_all(collection);
        std::string json_response = "[";
        
        for (size_t i = 0; i < items.size(); ++i) {
            if (i > 0) json_response += ",";
            json_response += "{";
            
            bool first = true;
            for (const auto& pair : items[i]) {
                if (!first) json_response += ",";
                json_response += "\"" + pair.first + "\":\"" + pair.second + "\"";
                first = false;
            }
            json_response += "}";
        }
        
        json_response += "]";
        send_json_response(response, json_response);
    } else {
        send_error_response(response, 400, "Invalid collection path");
    }
}

void HttpServer::handle_crud_update(const HttpRequest& request, HttpResponse& response) {
    std::regex item_regex(R"(/api/data/([^/]+)/([^/]+))");
    std::smatch matches;
    
    if (std::regex_match(request.path, matches, item_regex)) {
        std::string collection = matches[1].str();
        std::string id = matches[2].str();
        
        std::map<std::string, std::string> item;
        
        if (!request.form_data.empty()) {
            item = request.form_data;
        } else {
            // Simple JSON-like parsing (basic implementation)
            std::string body = request.body;
            if (body.find('{') != std::string::npos && body.find('}') != std::string::npos) {
                body = body.substr(1, body.length() - 2);
                std::istringstream iss(body);
                std::string pair;
                
                while (std::getline(iss, pair, ',')) {
                    size_t colon_pos = pair.find(':');
                    if (colon_pos != std::string::npos) {
                        std::string key = pair.substr(0, colon_pos);
                        std::string value = pair.substr(colon_pos + 1);
                        
                        key.erase(std::remove_if(key.begin(), key.end(), [](char c) {
                            return c == '"' || c == ' ' || c == '\t';
                        }), key.end());
                        
                        value.erase(std::remove_if(value.begin(), value.end(), [](char c) {
                            return c == '"' || c == ' ' || c == '\t';
                        }), value.end());
                        
                        item[key] = value;
                    }
                }
            }
        }
        
        if (data_store.update(collection, id, item)) {
            std::string json_response = "{\"id\":\"" + id + "\",\"status\":\"updated\"}";
            send_json_response(response, json_response);
        } else {
            send_error_response(response, 404, "Item not found");
        }
    } else {
        send_error_response(response, 400, "Invalid item path");
    }
}

void HttpServer::handle_crud_delete(const HttpRequest& request, HttpResponse& response) {
    std::regex item_regex(R"(/api/data/([^/]+)/([^/]+))");
    std::smatch matches;
    
    if (std::regex_match(request.path, matches, item_regex)) {
        std::string collection = matches[1].str();
        std::string id = matches[2].str();
        
        if (data_store.remove(collection, id)) {
            std::string json_response = "{\"id\":\"" + id + "\",\"status\":\"deleted\"}";
            send_json_response(response, json_response);
        } else {
            send_error_response(response, 404, "Item not found");
        }
    } else {
        send_error_response(response, 400, "Invalid item path");
    }
}

// File handlers
void HttpServer::handle_file_upload(const HttpRequest& request, HttpResponse& response) {
    // Debug: Check if we have any form data or files
    if (request.files.empty() && request.form_data.empty()) {
        std::string debug_info = "No files or form data found. ";
        debug_info += "Content-Type: ";
        auto ct_it = request.headers.find("Content-Type");
        if (ct_it != request.headers.end()) {
            debug_info += ct_it->second;
        } else {
            debug_info += "missing";
        }
        debug_info += ", Body size: " + std::to_string(request.body.size());
        send_error_response(response, 400, debug_info);
        return;
    }
    
    if (request.files.empty()) {
        send_error_response(response, 400, "No files uploaded");
        return;
    }
    
    std::string json_response = "{\"uploaded_files\":[";
    bool first = true;
    
    for (const auto& file_pair : request.files) {
        const auto& file_data = file_pair.second;
        
        std::string filepath = "uploads/" + file_data.filename;
        std::ofstream file(filepath, std::ios::binary);
        
        if (file.is_open()) {
            file.write(file_data.data.data(), file_data.data.size());
            file.close();
            
            if (!first) json_response += ",";
            json_response += "{\"filename\":\"" + file_data.filename + "\",\"status\":\"uploaded\"}";
            first = false;
        }
    }
    
    json_response += "]}";
    send_json_response(response, json_response, 201);
}

void HttpServer::handle_file_download(const HttpRequest& request, HttpResponse& response) {
    std::string filename = extract_filename_from_path(request.path);
    if (filename.empty()) {
        send_error_response(response, 400, "Invalid filename");
        return;
    }
    
    std::string filepath = "uploads/" + filename;
    send_file_response(response, filepath);
}

void HttpServer::handle_file_list(const HttpRequest&, HttpResponse& response) {
    std::string json_response = "{\"files\":[";
    bool first = true;
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator("uploads")) {
            if (entry.is_regular_file()) {
                if (!first) json_response += ",";
                json_response += "\"" + entry.path().filename().string() + "\"";
                first = false;
            }
        }
    } catch (const std::exception& e) {
        // Directory might not exist, create it
        std::filesystem::create_directories("uploads");
    }
    
    json_response += "]}";
    send_json_response(response, json_response);
}

void HttpServer::handle_client_page(const HttpRequest&, HttpResponse& response) {
    send_file_response(response, "client.html");
}

// Utility methods
void HttpServer::send_json_response(HttpResponse& response, const std::string& json, int status) {
    response.status_code = status;
    response.status_text = (status == 200) ? "OK" : (status == 201) ? "Created" : "Error";
    response.headers["Content-Type"] = "application/json";
    response.headers["Access-Control-Allow-Origin"] = "*";
    response.body = json;
}

void HttpServer::send_error_response(HttpResponse& response, int status, const std::string& message) {
    response.status_code = status;
    response.status_text = message;
    response.headers["Content-Type"] = "application/json";
    response.headers["Access-Control-Allow-Origin"] = "*";
    response.body = "{\"error\":\"" + message + "\"}";
}

void HttpServer::send_file_response(HttpResponse& response, const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        send_error_response(response, 404, "File not found");
        return;
    }
    
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::string content_type = get_content_type(filepath);
    
    // For HTML files, serve as text content instead of binary download
    if (content_type == "text/html") {
        std::string html_content((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
        file.close();
        
        response.is_binary = false;
        response.status_code = 200;
        response.status_text = "OK";
        response.headers["Content-Type"] = content_type;
        response.headers["Access-Control-Allow-Origin"] = "*";
        response.body = html_content;
    } else {
        // Binary file download
        response.binary_data.resize(file_size);
        file.read(response.binary_data.data(), file_size);
        file.close();
        
        response.is_binary = true;
        response.status_code = 200;
        response.status_text = "OK";
        response.headers["Content-Type"] = content_type;
        response.headers["Content-Disposition"] = "attachment; filename=\"" + 
            std::filesystem::path(filepath).filename().string() + "\"";
        response.headers["Access-Control-Allow-Origin"] = "*";
    }
}