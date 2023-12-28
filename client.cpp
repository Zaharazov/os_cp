#include <iostream>
#include <thread>
#include <vector>
#include <fcntl.h>
#include "funcs.h"

//функция приёма сообщений (для потока)
void func(int fd_respond, std::string login)
{
    while (1)
    {
        std::string respond = recieve_message_client(fd_respond);
        
        std::cout << std::endl << respond << std::endl;
        std::cout.flush();
        
        if (respond == "SERVER CLOSED")
            exit(0);
        
        std::cout << login << "> ";
        std::cout.flush();
    }
}

void write_intro() {
    std::cout << "|=|=| The Game [Bulls & Cows] |=|=|" << std::endl;
    std::cout << std::endl;
    std::cout << "Please enter your login: ";
    std::cout.flush();
}

void write_menu(std::string login){
    std::cout << std::endl;
    std::cout << "|=|=|=|=|=| GAME MENU |=|=|=|=|=|" << std::endl;

    std::cout << "| Select an action:             |" << std::endl;
    std::cout << "| 1. create [name of the game]  |" << std::endl;
    std::cout << "| 2. connect [name of the game] |" << std::endl;
    std::cout << "| 3. find                       |" << std::endl;
    std::cout << "| 4. leave                      |" << std::endl;

    if (login != "admin") std::cout << "| 5. quit                       |" << std::endl;
    if (login == "admin") std::cout << "| 5. shutdown [!]               |" << std::endl;

    std::cout << "|=|=|=|=|=|=|=|=|=|=|=|=|=|=|=|=|" << std::endl;
    std::cout << std::endl;

    std::cout.flush();
}

int server_main_input_fd(){
    int fd = open("main_input", O_RDWR);
    
    if (fd == -1){
        std::cout << "ERROR: main fifo wasn`t opened!" << std::endl;
        exit(1);
    }

    return fd;
}

int main()
{
    int client_main_out_fd = server_main_input_fd(); 

    write_intro();

    std::string login;
    std::cin >> login;

    std::string password;

    if(login == "admin"){
        std::cout << "Please enter the password: ";
        std::cin >> password;
        std::cout << std::endl;

        if(password != "admin"){
            std::cout << "ERROR: wrong password!" << std::endl;
            exit(1);
        }
    }

    send_message_to_server(client_main_out_fd, login, "login", "");
    std::cout << "Connection..." << std::endl;
    sleep(1);

    int fd_respond = open(login.c_str(), O_RDWR);
    
    if (fd_respond == -1)
    {
        std::cout << "ERROR: respond fifo wasn`t opened!" << std::endl;
        exit(1);
    }

    std::cout << "Connected succesfully!" << std::endl;

    write_menu(login);

    std::thread thr_respond(func, fd_respond, login);

    std::string command, data;
    std::string game_word, game_name;
    
    int game_fd;

    while (1)
    {
        std::cout << login << "> ";
         std::cin >> command;
    
        if (command == "create")
        {
            std::cin >> game_name;
            data = game_name + "$";
            send_message_to_server(client_main_out_fd,login,command,data);
            std::cout << "|=|=| Print: |connect " << game_name << "| to open the room." << std::endl; 

        }
        else if (command == "find")
        {
            data = "";
            send_message_to_server(client_main_out_fd,login,command,data);
        }
        
        else if (command == "connect")
        {
            std::cin >> game_name;

            game_fd = open(("game_%" + game_name).c_str(), O_RDWR);
            
            if (game_fd == -1)
            {
                std::cout << "ERROR: game not found!" << std::endl;
                std::cout.flush();
            }
            else
            {
                data = "";
                send_message_to_server(game_fd,login,command,data);
                std::cout << login << "> ";
                std::cout.flush();
                while (1)
                {
                    std::cin >> command;

                    if (command == "ans")
                    {
                        std::cin >> data;
                        send_message_to_server(game_fd,login,command,data);
                    }
                    else if (command == "leave")
                    {
                        data = "";
                        send_message_to_server(game_fd,login,command,data);
                        write_menu(login);
                        break;
                    }
                    else
                    {
                        std::cout << login << "> ";
                        std::cout.flush();
                    }
                }
            }
        }
        else if (command == "quit" && login != "admin")
        {
            data = "";
            send_message_to_server(client_main_out_fd,login,command,data);
            thr_respond.detach();
            return 0;
        }
        else if (command == "shutdown" && login == "admin")
        {
            data = "";
            send_message_to_server(client_main_out_fd,login,command,data);
            thr_respond.detach();
            return 0;
        }
    }
    return 0;
}