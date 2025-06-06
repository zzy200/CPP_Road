#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <fstream>
#include <sstream>
#include "threadpool.cpp"

struct HttpRequest{
    std::string method;
    std::string path;
    std::string version;
};
HttpRequest ParseRequest(const std::string& request);
void handle_request(int client_fd);
void handle_get(int client_fd, std::string path);
void handle_post(int client_fd, std::string path, const char* request);
void handle_head(int client_fd, std::string path);
void send_error_response(int client_fd, int code);
bool is_cgi_request(std::string path);
void execute_cgi(int client_fd, const std::string& path, const std::string& post_data);

int main() {
    //create web socket, AF_INET means IPV4, SOCK_STREAM means TCP connection
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Socket creation failed !" << std::endl;
        return 1;
    }

    //create address used to bind socket
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);     //h(host, small-endian or big-endian) to n(network, all big-endian) s(short)

    //bind the socket to the address, if success bind return 0, else return -1
    if(bind(server_fd, (struct sockaddr*)& address, sizeof(address)) < 0) {
        std::cerr << "Bind failed !" << std::endl;
        close(server_fd);
        return -1;
    }

    //listen, if success bind return 0, else return -1, set the socket to passive listen mode, 
    if(listen(server_fd, 20) < 0) {
        std::cerr << "Listen failed !" << std::endl;
        close(server_fd);
        return -1;
    }

    std::cout << "Server listening on port:8080" << std::endl;
    

    //instantiate a threadpool
    Threadpool threadpool(1);

    //start infinite loop to handle connection requests
    while(true) {
        int client_socket_fd;   
        struct sockaddr_in client_addr;     //store client address
        socklen_t client_addr_len = sizeof(client_addr);
        client_socket_fd = accept(server_fd, (struct sockaddr*)& client_addr, &client_addr_len);
        if(client_socket_fd < 0) {
            std::cerr << "Accept failed !" << std::endl;
            continue;
        }

        //cout client ip address
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        std::cout << "New connections from: " << client_ip << std::endl;

        //simple response
        //const char* response = "Hello from my web server!\n";
        //send(client_socket_fd, response, strlen(response), 0);

        //enqueue task to threadpool
        threadpool.enqueue([client_socket_fd] {
            handle_request(client_socket_fd);
            close(client_socket_fd); //close current connection
        });
        
    }

    close(server_fd);
    return 0;
}


void handle_request(int client_fd) {
    //receive request data bytes from client
    char buffer[4096];
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);

    if(bytes_read <= 0) {
        return;
    }
    buffer[bytes_read] = '\0';

    HttpRequest request = ParseRequest(buffer);

    // 验证HTTP版本
    if(request.version.find("HTTP/1.0") == std::string::npos && 
       request.version.find("HTTP/1.1") == std::string::npos) {
        send_error_response(client_fd, 505);
        return;
    }

        // 方法分发
    if(request.method == "GET") {
        handle_get(client_fd, request.path);
    }
    else if(request.method == "HEAD") {
        handle_head(client_fd, request.path);
    }
    else if(request.method == "POST") {
        handle_post(client_fd, request.path, buffer);
    }
    else {
        send_error_response(client_fd, 501);
    }


}


HttpRequest ParseRequest(const std::string& request){
    HttpRequest req;
    std::istringstream iss(request);
    iss >> req.method >> req.path >> req.version;
    return req;
}

void handle_get(int client_fd, std::string path){
    if(path == "/") {
        path = "/index.html";
    }

    std::string file_path = "." + path;
    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if(!file) {
        send_error_response(client_fd, 404);
        //std::cout << "can't open file: " << file_path << std::endl;
        return;
    }
    int file_size = file.tellg();
    file.seekg(0);
    std::string content(file_size, '\0');
    file.read(&content[0], file_size);

    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/html\r\n";
    response += "Content-Length: " + std::to_string(file_size) + "\r\n\r\n";

    response += content;
    send(client_fd, response.c_str(), response.size(), 0);
    std::cout << "file: " << file_path << " successfully sent" << std::endl;
}

void handle_head(int client_fd, std::string path){
    if(path == "/") {
        path = "/index.html";
    }

    std::string file_path = "." + path;
    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if(!file) {
        send_error_response(client_fd, 404);
        //std::cout << "can't open file: " << file_path << std::endl;
        return;
    }
    int file_size = file.tellg();
    file.seekg(0);
    std::string content(file_size, '\0');
    file.read(&content[0], file_size);

    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/html\r\n";
    response += "Content-Length: " + std::to_string(file_size) + "\r\n\r\n";

    send(client_fd, response.c_str(), response.size(), 0);
    std::cout << "file head: " << file_path << " successfully sent" << std::endl;
}

void handle_post(int client_fd, std::string path, const char* request) {
    std::cout << "is post request" << std::endl;
    //check if cgi_request
    if (is_cgi_request(path)) {
        std::cout << "is cgi request" << std::endl;
        //extract cgi request
        const char* body_start = strstr(request, "\r\n\r\n");
        if (!body_start) {
            send_error_response(client_fd, 400);
            return;
        }
        body_start += 4; // 跳过空行
        
        std::string post_data(body_start);
        execute_cgi(client_fd, path, post_data);
    } else {
        // 非 CGI POST 处理
        std::string response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: text/plain\r\n\r\n";
        response += "Received POST data";
        send(client_fd, response.c_str(), response.size(), 0);
    }
}
void send_error_response(int client_fd, int code) {
    std::unordered_map<int, std::string> status_text = {
        {400, "Bad Request"},
        {403, "Forbidden"},
        {404, "Not Found"},
        {500, "Internal Server Error"},
        {501, "Not Implemented"},
        {505, "HTTP Version Not Supported"}
    };
    
    std::string body = "<html><body>" + std::to_string(code) + 
                      " " + status_text[code] + "</body></html>";
    
    std::string response = "HTTP/1.1 " + std::to_string(code) + " " + status_text[code] + "\r\n";
    response += "Content-Type: text/html\r\n";
    response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    response += "Connection: close\r\n\r\n";
    response += body;
    
    send(client_fd, response.c_str(), response.size(), 0);
}

bool is_cgi_request(std::string path) {
    return path.find("/cgi-bin/") == 0;
}

void execute_cgi(int client_fd, const std::string& path, const std::string& post_data) {
    // 创建两个管道：一个用于输入，一个用于输出
    int input_pipe[2], output_pipe[2];
    if (pipe(input_pipe) || pipe(output_pipe)) {
        send_error_response(client_fd, 500);
        return;
    }

    std::cout << "POST data: " << post_data << std::endl;
    std::cout << "Content-Length: " << post_data.size() << std::endl;
        
    pid_t pid = fork();
    if (pid < 0) {
        close(input_pipe[0]); close(input_pipe[1]);
        close(output_pipe[0]); close(output_pipe[1]);
        send_error_response(client_fd, 500);
        return;
    }

    if (pid == 0) { // 子进程 (CGI)
        // 关闭不需要的管道端
        close(input_pipe[1]); // 关闭输入管道的写入端
        close(output_pipe[0]); // 关闭输出管道的读取端
        
        // 重定向标准输入输出
        dup2(input_pipe[0], STDIN_FILENO);   // 输入管道 -> 标准输入
        dup2(output_pipe[1], STDOUT_FILENO); // 输出管道 -> 标准输出
        dup2(output_pipe[1], STDERR_FILENO); // 错误输出也重定向到输出管道
        
        // 关闭多余的文件描述符
        close(input_pipe[0]);
        close(output_pipe[1]);
        
        // 设置环境变量
        setenv("REQUEST_METHOD", "POST", 1);
        setenv("CONTENT_LENGTH", std::to_string(post_data.size()).c_str(), 1);
        setenv("CONTENT_TYPE", "application/x-www-form-urlencoded", 1);
        setenv("SCRIPT_NAME", path.c_str(), 1);

        // 执行 CGI 脚本
        std::string full_path = "." + path;
        execl(full_path.c_str(), full_path.c_str(), nullptr);
        
        // 如果执行失败
        std::cerr << "execl failed: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    } 
    else { // 父进程 (服务器)
        // 关闭不需要的管道端
        close(input_pipe[0]); // 关闭输入管道的读取端
        close(output_pipe[1]); // 关闭输出管道的写入端
        
        // 1. 写入 POST 数据到 CGI 输入
        if (!post_data.empty()) {
            write(input_pipe[1], post_data.c_str(), post_data.size());
        }
        close(input_pipe[1]); // 关闭输入管道
        
        // 2. 读取 CGI 输出
        std::string cgi_output;
        char buffer[4096];
        ssize_t bytes_read;
        
        // 设置5秒超时
        struct timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(output_pipe[0], &read_fds);
        
        while (true) {
            int ready = select(output_pipe[0] + 1, &read_fds, nullptr, nullptr, &tv);
            if (ready <= 0) break; // 超时或错误
            
            bytes_read = read(output_pipe[0], buffer, sizeof(buffer));
            if (bytes_read <= 0) break;
            
            cgi_output.append(buffer, bytes_read);
        }
        
        // 3. 处理子进程退出
        int status;
        waitpid(pid, &status, 0);
        
        if (cgi_output.empty()) {
            // 从错误日志中获取信息
            std::ifstream log("cgi_error.log");
            if (log) {
                std::ostringstream ss;
                ss << log.rdbuf();
                cgi_output = "HTTP/1.1 500 Internal Server Error\r\n"
                             "Content-Type: text/plain\r\n\r\n" + ss.str();
            } else {
                send_error_response(client_fd, 500);
                return;
            }
        }
        
        // 4. 发送响应给客户端
        send(client_fd, cgi_output.c_str(), cgi_output.size(), 0);
        
        // 5. 清理
        close(output_pipe[0]);
    }
}