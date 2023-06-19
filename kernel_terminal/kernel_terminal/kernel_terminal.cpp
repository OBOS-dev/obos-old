#include <iostream>
#include <string>
#include <vector>
#include <thread>

#include <cstdarg>
#include <cstring>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <sys/ioctl.h>
#include <unistd.h>

#include <termios.h>

#define INVALID_SOCKET ((SOCKET)(-1))

//int ssend(int fd, const char* format, ...)
//{
//	va_list list;
//	std::vector<char> toSend;
//	va_start(list, format);
//	int size = vsnprintf(nullptr, 0, format, list);
//	va_end(list);
//	va_start(list, format);
//	toSend.resize(size + 1, 0);
//	vsnprintf(toSend.data(), size + 1, format, list);
//	va_end(list);
//	return send(fd, toSend.data(), size, 0);
//}

// Credit: https://stackoverflow.com/a/7469410
void initTermios(int echo);
// Credit: https://stackoverflow.com/a/7469410
char getch(void);
char getche(void);

std::string GetIPFromURL(const char* url)
{
	int r = 0;
	hostent* myHostent = gethostbyname(url);
	if (!myHostent)
		return "";
	std::string ip;
	char* temp = myHostent->h_addr_list[0];

	auto* addr = reinterpret_cast<in_addr*>(temp);

	const char* ipTemp = inet_ntoa(*addr);
	for (int i = 0; i < strlen(ipTemp); i++)
		ip.push_back(ipTemp[i]);
	return ip;
}

int fd = 0;

int main(int argc, char** argv)
{
#ifndef _DEBUG
	if (argc != 2)
	{
		std::cout << "Usage: kssh ip:port\n";
		return -1;
	}
	std::string argument = argv[1];
	std::string ip = argument.substr(0, argument.find(':'));
	short port = std::stoi(argument.substr(argument.find(':') + 1));
#else
	std::string ip = "127.0.0.1";
	short port = 1534;
#endif
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1)
	{
		int err = errno;
		fprintf(stderr, "socket: %s", strerror(err));
		return err;
	}
	ip = GetIPFromURL(ip.c_str());
	sockaddr_in addr;
	inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
	addr.sin_port = htons(port);
	addr.sin_family = AF_INET;
	int ret = connect(fd, (sockaddr*)&addr, sizeof(sockaddr_in)), err = 0;
	if (ret == -1)
	{
		err = errno;
		fprintf(stderr, "connect: %s", strerror(err));
		return err;
	}
	std::thread{ [&]() {
		std::string read{};
		read.resize(2048);
		while (1)
		{
			for (int i = 0; i < read.length(); i++) read[i] = 0;
			if (::read(fd, &read[0], read.length()) == -1)
			{
				err = errno;
				fprintf(stderr, "read: %s", strerror(err));
				exit(err);
			}
			std::cout << read;
		}
	} }.detach();
	char c;
	for (c = 0; true; c = getche())
		if(c != '\n')
			if(write(fd, &c, 1) == -1)
			{
				err = errno;
				fprintf(stderr, "write: %s", strerror(err));
				exit(err);
			}
			else {}
		else
			if (write(fd, "\r\n", 2) == -1)
			{
				err = errno;
				fprintf(stderr, "write: %s", strerror(err));
				exit(err);
			}
	shutdown(fd, SHUT_RDWR);
	close(fd);
	return 0;
}

static termios old, current;

void initTermios(int echo)
{
	tcgetattr(0, &old); /* grab old terminal i/o settings */
	current = old; /* make new settings same as old settings */
	current.c_lflag &= ~ICANON; /* disable buffered i/o */
	if (echo)
		current.c_lflag |= ECHO; /* set echo mode */
	else
		current.c_lflag &= ~ECHO; /* set no echo mode */
	tcsetattr(0, TCSANOW, &current); /* use these new terminal i/o settings now */
}

// Credit: https://stackoverflow.com/a/7469410
char getch_(int echo)
{
	char ch;
	initTermios(echo);
	ch = getchar();
	tcsetattr(0, TCSANOW, &old);
	return ch;
}
char getch(void)
{
	return getch_(false);
}
char getche(void)
{
	return getch_(true);
}