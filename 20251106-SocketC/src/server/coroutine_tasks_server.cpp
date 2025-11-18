#include  "../unp.h"
#include  <time.h>
#include "../thread/unpthread.h"

#include <asio.hpp>
#include <iostream>
#include <string>
#include <asio/ip/address.hpp>

using boost::asio::ip::tcp;
namespace asio = boost::asio;
// This is an example of TCP/TP transport layer protocol concurrent server for text read/write.
// Coroutine + tasks handle asynchronous using Boost Asio
// By using the C++ Boost Asio, we could simplify out syntax, and think more about the application behavior other than the pointer data type
// So our server will setup the host ip configuration -> If any error occurs at setup phase, we shutdown the system
// Then accept many the clients to in for loop -> If any error occurs at accept, we continue to accept other clients
//
void doit(tcp::socket client_sock);
void str_echo(tcp::socket sockfd);

int
main(int argc, char **argv)
{
    openlog("ServerCoroutine", LOG_PID | LOG_PERROR, LOG_USER); // development only
    boost::asio::io_context io_ctx;
    std::string bind_ip = "0.0.0.0";
    unsigned short port = 8080;

    tcp::acceptor acceptor(io_ctx);
    try {
        acceptor = tcp::acceptor(io_ctx, { asio::ip::make_address_v4(bind_ip), port});
    }
    catch (std::exception& e) {
        syslog(LOG_ERR, "We got an Error while setting up the server: ", e.what());
        return 1;
    }
        for ( ; ; ) {
            try {
            // this is equivalent to the fd, we could take the client connection info from these
            // without using sockaddr_in, inet_ntop, or any C-level functions.
            // since a tcp::socket holds a FILE DESCRIPTOR (FD) -> FD is a unique resource managed by the OS. So copy it to pass to the handler is not good.
            tcp::socket client_sock(io_ctx);
            acceptor.accept(client_sock);

            auto remote = client_sock.remote_endpoint();
            std::string client_ip = remote.address().to_string();
            unsigned short client_port = remote.port();
            syslog(LOG_INFO, "Accepted connection from %s:%d", client_ip.c_str(), client_port);

            // In C++ standard, by using the advantage of class, it could hide the complexity of TID, pointer type,...
            // If we have a local variable in a stack scope, we want to grasp that memory and put it inside other stack scope -> std::move
            std::thread t(doit, std::move(client_sock));
            std::thread::id id = t.get_id();
            t.detach();
            }
            catch (std::exception& e) {
               syslog(LOG_ERR, "We got an Error while running the server to handle client A: ", e.what());
               continue;
            }
        }
}

void
doit(tcp::socket client_sock)
{
    str_echo(std::move(client_sock));	/* same function as before */
    client_sock.close();
}

//void str_echo(tcp::socket sockfd) {
//    while (1) {

//        ssize_t n = recv(sockfd, buf, sizeof(buf), 0); //Recv

//        if (n > 0){
//            // We does care about the error of send so we let the wrap function to handle that
//            Send(sockfd, buf, n, 0);
//        }
//        else if (n == 0){
//            break;  // client closed connection
//        }
//        else if (n < 0 && errno == EINTR) {
//            continue; // interrupted, retry
//        }
//        else{
//            err_sys("read error");
//            break;
//        }
//    }
//}

// So here is the bad thing, since we stick with the Boost Asio socket, then we can not use the recv system call anymore, we can not deal with the file descriptor anymore, this is a completely new layer of software
// RULE1: Throwing functions DO NOT take error_code
// RULE2: Non-throwing functions ALWAYS take error_code
// RULE3: ALL async functions NEVER throw if you use error_code style
// C++
// std::fstream f("file");
// If it throws an exception → destructor closes file
// If scope exits → destructor closes file
//
// C
// int fd = open(...);
// if (!f) {
//      perror("fopen");
// }
//
//
void str_echo(tcp::socket sock)   // <-- BY VALUE (MOVE-ONLY)
{
        std::array<char, 4096> buf;

        for (;;) {
            boost::system::error_code ec;

            // blocking read (similar to recv())
            std::size_t n = sock.read_some(boost::asio::buffer(buf), ec);

            if (!ec && n > 0) {
                // blocking write (similar to send())
                boost::asio::write(sock, boost::asio::buffer(buf.data(), n));
            }
            else if (ec == boost::asio::error::eof) {
                // client closed connection
                syslog(LOG_INFO, "Client closed connection -> Stop this thread");
                break;
            }
            else if (ec == boost::asio::error::interrupted) {
                // EINTR equivalent
                syslog(LOG_WARNING, "User interrupt -> Ignore");
                continue;
            }
            else {
                // any real read error
                syslog(LOG_ERR, "Error: %s -> Stop this thread", ec.message().c_str());
                break;
            }
        }
    // sock closes automatically when function returns (RAII)
}
