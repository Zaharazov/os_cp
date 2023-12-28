#include <string>
#include <unistd.h>

//отправить сообщение серверу в удобной форме - логин$команда$сообщение
void send_message_to_server(int fd, std::string curlogin, std::string command, std::string data)
{
    std::string text = curlogin + "$" + command + "$" + data;
    int k = text.size();
    write(fd, &k, sizeof(k));
    char message[k];
    for (int i = 0; i < k; ++i)
        message[i] = text[i];

    write(fd, message, k);
}

//отправить сообщение клиенту
void send_message_to_client(int fd, std::string message)
{
    std::string text = message;
    int k = text.size();
    write(fd, &k, sizeof(k));
    char message_c[k];
    for (int i = 0; i < k; ++i)
        message_c[i] = text[i];
    write(fd, message_c, k);
}

//получить сообщение в удобной для клиента форме
std::string recieve_message_client(int fd)
{
    int size;
    read(fd, &size, sizeof(size));

    char message[size];
    read(fd, message, size);

    std::string recv;
    for (int i = 0; i < size; ++i)
    {
        if (message[i] != '$')
            recv.push_back(message[i]);
        else
            recv = recv + ": ";
    }
    return recv;
}

//получить логин из сообщения для сервера
std::string extract_login(std::string message)
{
    std::string login;
    int i = 0;
    while (message[i] != '$')
    {
        login.push_back(message[i]);
        ++i;
    }
    return login;
}

//получить команду из сообщения для сервера
std::string extract_command(std::string message)
{
    std::string command;
    int i = 0;
    while (message[i] != '$')
        ++i;
    ++i;
    while (message[i] != '$')
    {
        command.push_back(message[i]);
        ++i;
    }
    return command;
}

//получить информацию из сообщения для сервера
std::string extract_data(std::string message)
{
    std::string data;
    int i = 0;
    while (message[i] != '$')
        ++i;
    ++i;
    while (message[i] != '$')
        ++i;
    ++i;
    while (i < message.size())
    {
        data.push_back(message[i]);
        ++i;
    }
    return data;
}

void recieve_message_server(int fd, std::string *rcvd_name, std::string *rcvd_command, std::string *rcvd_data)
{
    int size;
    read(fd, &size, sizeof(size));

    char messagec[size];
    read(fd, messagec, size);

    std::string recv;
    for (int i = 0; i < size; ++i)
        recv.push_back(messagec[i]);
    
    *rcvd_name = extract_login(recv);
    *rcvd_command = extract_command(recv);
    *rcvd_data = extract_data(recv);
}

void extract_game_data(std::string message, std::string *game_name)
{
    int i = 0;
    std::string recv1;
    while (message[i] != '$') 
    {
        recv1.push_back(message[i]);
        ++i;
    }

    *game_name = recv1;
}