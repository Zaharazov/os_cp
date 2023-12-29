#include <iostream>
#include <vector>
#include <thread>
#include <string>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include "funcs.h"

#define CLIENT_ID(name) in(logins,name)
#define PLAYER_ID(name) in(curr_playrs_name, name)

int create_game_pipe(std::string game_name){
    int curr_playrs;
    if (mkfifo(("game_%" + game_name).c_str(), 0777) == -1)
    {
        if(errno != EEXIST){
            std::cout << "ERROR: game " << ("game_%" + game_name).c_str() << std::endl;
            exit(1);
        }
    }

    int game_input_fd = open(("game_%" + game_name).c_str(), O_RDWR);
    
    if (game_input_fd == -1)
    {
        std::cout << "ERROR: game input fofi wasn`t opened!" << std::endl;
        exit(1);
    }
    
    return game_input_fd;
}

int in(std::vector<std::string> logins, std::string str)
{
    for (int i = 0; i < logins.size(); ++i)
    {
        if (logins[i] == str)
            return i;
    }
    return -1;
}

int create_main_pipe() {
    if (mkfifo("main_input", 0777) == -1){
        if(errno != EEXIST){
            std::cout << "ERROR: mkfifo main_input!" << std::endl;
            exit(1);
        }
    }

    int fd_recv = open("main_input", O_RDWR);
    
    if (fd_recv == -1)
    {
        std::cout << "ERROR: main input fifo wasn`t opened!";
        exit(1);
    }

    return fd_recv;
}

int create_admin_pipe() {

    if (mkfifo("admin", 0777) == -1){
        if(errno != EEXIST){
            std::cout << "ERROR: mkfifo admin!" << std::endl;
            exit(1);
        }
    }

    int admin_fd = open("admin", O_RDWR);
    
    if (admin_fd == -1)
    {
        std::cout << "ERROR: admin input fifo wasn`t opened!" << std::endl;
        exit(1);
    }
    
    return admin_fd;
}

int create_client_pipe(std::string rcvd_name) {
    if (mkfifo(rcvd_name.c_str(), 0777) == -1)
    {
        if(errno != EEXIST){
            std::cout << "ERROR: mkfifo client!" << std::endl;
            exit(1);
        }
    }

    int fd = open(rcvd_name.c_str(), O_RDWR);
    
    if (fd == -1)
    {
        std::cout << "ERROR: client input fifo wasn`t opened!" << std::endl;
        exit(1);
    }

    return fd;
}

int hit_check(std::string game_word, std::string try_word, int *cows, int *bulls,std::vector<int>& bulls_cows_index) {

    std::cout << game_word << "!" << try_word << std::endl;
    if (try_word.size() != game_word.size()) return -1;
    
    if (try_word == game_word) return -2;

    *cows = 0;
    *bulls = 0;

    for(int i{0}; i < try_word.size(); ++i){
        if(try_word[i] != game_word[i]){

            if(game_word.find(try_word[i]) != std::string::npos){
                *cows = *cows + 1;
                bulls_cows_index[i] = 1;
            }
        }
        else{
            *bulls = *bulls + 1;
            bulls_cows_index[i] = 2;
        }
    }

    return 0;
}

void game_process(std::string game_name, std::string game_word)
{
    std::vector<std::string> curr_playrs_name;
    std::vector<int> curr_playrs_fd;
    
    auto iter_fd = curr_playrs_fd.cbegin();
    auto iter_log = curr_playrs_name.cbegin();
    
    int game_input_fd = create_game_pipe(game_name);
    int cows,bulls;
    
    std::string game_respond;
    
    int game_status;

    std::cout << "START GAME: " << game_name << std::endl;
    std::cout.flush();

    std::string rcvd_name, rcvd_command, rcvd_data;

    std::vector<int> bulls_cows_index;
    std::vector<int> cows_index;

    while (1)
    {
        recieve_message_server(game_input_fd, &rcvd_name, &rcvd_command, &rcvd_data);
        if (rcvd_command == "connect")
        {
            curr_playrs_name.push_back(rcvd_name);
            curr_playrs_fd.push_back(open(rcvd_name.c_str(), O_RDWR));

            std::cout << "CLIENT: " << rcvd_name << " JOIN GAME: " << game_name << std::endl;
            std::cout.flush();

            game_respond = "|=|=| GAME " + game_name + " STARTED ";

            send_message_to_client(curr_playrs_fd[PLAYER_ID(rcvd_name)], game_respond.c_str());
            
            game_respond = "|=|=| Print: |ans ...| or |leave| "; 
            
            send_message_to_client(curr_playrs_fd[PLAYER_ID(rcvd_name)], game_respond.c_str());
        }
        else if (rcvd_command == "ans")
        {
            bulls_cows_index = {0,0,0,0,0};
            game_status = hit_check(game_word, rcvd_data, &cows, &bulls, bulls_cows_index);

            if (game_status == -1)
            {
                game_respond = "Words size don't match!";
                send_message_to_client(curr_playrs_fd[PLAYER_ID(rcvd_name)], game_respond.c_str());
            }
            else if (game_status == -2)
            {
                game_respond = "You've Won!";
                send_message_to_client(curr_playrs_fd[PLAYER_ID(rcvd_name)], game_respond.c_str());

                for (int i=0; i < curr_playrs_name.size(); i++)
                {
                    game_respond = "Winner is " + rcvd_name + "\nAnswer is " + game_word;
                    send_message_to_client(curr_playrs_fd[i], game_respond.c_str());
                    do{
                        game_respond = "Print: |leave|";
                        send_message_to_client(curr_playrs_fd[i], game_respond.c_str());
                        recieve_message_server(game_input_fd, &rcvd_name, &rcvd_command, &rcvd_data);
                    }while(rcvd_command != "leave");
                }

                close(game_input_fd);

                std::cout << "FINISH GAME: " << game_name << std::endl;
                std::cout.flush();
                int mainFD = open("main_input", O_RDWR);
                game_respond = "finish";
                send_message_to_server(mainFD, game_name, game_respond, "");
                return;
            }
            else if (game_status == 0)
            {
                std::string cows_symbols = " symbols: ";
                for(int elem {0}; elem < bulls_cows_index.size(); ++elem){
                    if(bulls_cows_index[elem] == 1)
                        cows_symbols = cows_symbols + rcvd_data[elem] + " "; 
                }

                std::string bulls_symbols = " symbols: ";
                for(int elem {0}; elem < bulls_cows_index.size(); ++elem){
                    if(bulls_cows_index[elem] == 2)
                        bulls_symbols = bulls_symbols + rcvd_data[elem] + " "; 
                }

                game_respond = "Cows: " + std::to_string(cows) + cows_symbols + "\nBulls: " + std::to_string(bulls) + bulls_symbols;

                std::cout << cows_symbols << std::endl;

                send_message_to_client(curr_playrs_fd[PLAYER_ID(rcvd_name)], game_respond.c_str()); 
            }
        }
        else if (rcvd_command == "leave")
        {
            iter_fd = curr_playrs_fd.cbegin();
            curr_playrs_fd.erase(iter_fd + PLAYER_ID(rcvd_name));
            iter_log = curr_playrs_name.cbegin();
            curr_playrs_name.erase(iter_log + PLAYER_ID(rcvd_name));
            std::cout << "CLIENT: " << rcvd_name << " LEFT GAME: " << game_name << std::endl;
        }
    }
}

int main()
{
    srand(time(NULL));

    std::vector<std::string> logins;  // Логины пользователей

    std::vector<int> client_pipe_fd;  // Pipe для каждого клиента
    
    std::vector<std::thread> games_threads;  // Поток для игры
    
    std::vector<std::string> games_name;  // Имена игр
    
    std::string game_name_table;
    
    std::string game_word_size;

    std::string game_word;
    
    int fd_recv = create_main_pipe();
    int admin_fd = create_admin_pipe();

    std::string login;
    
    std::string rcvd_name;
    std::string rcvd_command;
    std::string rcvd_data; 

    auto iter_fd = client_pipe_fd.cbegin();
    auto iter_log = logins.cbegin();
    auto iter_game_thread = games_threads.cbegin();
    auto iter_game_name = games_name.cbegin();

    while (1)
    {       
        recieve_message_server(fd_recv, &rcvd_name, &rcvd_command, &rcvd_data);

        if (rcvd_command == "login" && rcvd_name != "admin")
        {
            std::cout <<"New client: " << rcvd_name << std::endl;
            std::cout.flush();

            // Создать pipe для клиента и добавить в вектор
            client_pipe_fd.push_back(create_client_pipe(rcvd_name));

            // Добавить имя пользователя в вектор логинов
            logins.push_back(rcvd_name);
        }
        else if (rcvd_command == "create")
        {
            // Получить имя игры и загаданное слово
            extract_game_data(rcvd_data, &game_name_table);

            std::string number = std::to_string(rand()%90000+10000);
            game_word = number;
            
            games_name.push_back(game_name_table);
            
            games_threads.push_back(std::thread(game_process, game_name_table, game_word));
        }
        else if (rcvd_command == "find")
        {
            std::vector<int> curr_playrs_fd_test;
            std::vector<std::string> curr_playrs_name;
            std::vector <std::string> game_respond_vector;
            std::string game_respond;

            curr_playrs_name.push_back(rcvd_name);
            curr_playrs_fd_test.push_back(open(rcvd_name.c_str(), O_RDWR));

            for (size_t i = 0; i < games_name.size(); ++i)
            {
                game_respond_vector.push_back(games_name[i]);
            }

            game_respond = "|=|=| Random lobby: " + game_respond_vector[rand() % game_respond_vector.size()];
            send_message_to_client(curr_playrs_fd_test[PLAYER_ID(rcvd_name)], game_respond.c_str());
        }
         else if (rcvd_command == "finish")
        {
            std::remove(("game_%" + rcvd_name).c_str());
            
            games_threads[in(games_name, rcvd_name)].detach();
            
            iter_game_thread = games_threads.cbegin();
            
            games_threads.erase(iter_game_thread + in(games_name, rcvd_name));
            
            iter_game_name = games_name.cbegin();
            
            games_name.erase(iter_game_name + in(games_name, rcvd_name));
            
        }
        else if (rcvd_command == "quit")
        {
            close(client_pipe_fd[CLIENT_ID(rcvd_name)]);
            std::remove(rcvd_name.c_str());

            iter_fd = client_pipe_fd.cbegin();
            iter_log = logins.cbegin();

            client_pipe_fd.erase(iter_fd + CLIENT_ID(rcvd_name));
            logins.erase(iter_log + CLIENT_ID(rcvd_name));
            std::cout << "CLIENT: " << rcvd_name << " LEFT\n";

        }
        else if (rcvd_command == "shutdown" && rcvd_name == "admin")
        {
            for(int i=0 ; i < logins.size(); i++)
            {
                send_message_to_client(client_pipe_fd[i],"SERVER CLOSED");
                close(client_pipe_fd[i]);
                std::remove(logins[i].c_str());
            }
            for(int i=0 ; i < games_threads.size(); i++)
            {
                std::remove(games_name[i].c_str());
                games_threads[i].detach();
            }
            
            close(admin_fd);
            std::remove("admin");
            std::remove("main_input");
            std::cout << "SERVER OFF\n";
            
            return 0;
        }
        else{
            std::cout << "Unknown command!" << std::endl;
        }
    }
}
