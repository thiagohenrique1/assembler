#include "Serial.h"

#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <iostream>
#include <bitset>

void Serial::send(std::bitset<16> data) {
	auto instr_bin = uint16_t(data.to_ulong());
	send(instr_bin);
}

void Serial::send(uint16_t data) {
	auto upper = uint8_t(data >> 8u);
	write(fd, &data, 1);
	write(fd, &upper, 1);
}

uint8_t Serial::receive() {
	uint8_t buff;
	read(fd, &buff, 1);
	return buff;
}

Serial::Serial(const std::string &port) {
	fd = open(port.c_str(), O_RDWR);
	if(fd == -1) std::cerr << port << " unavailable" << std::endl;

	termios options{};
	tcgetattr(fd, &options);

//	Set baud rate to 115200
	cfsetispeed(&options, B115200);
	cfsetospeed(&options, B115200);
//	Enable receiver and set local mode
	options.c_cflag |= (CLOCAL | CREAD);
//	8 bits, no parity
	options.c_cflag &= ~PARENB;
	options.c_cflag &= ~CSTOPB;
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;
//	Raw input
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	options.c_iflag &= ~(ICRNL | IGNCR | ICRNL | IGNBRK | BRKINT | PARMRK);
//	No software flow control
	options.c_iflag &= ~(IXON | IXOFF | IXANY);
//	Raw output
	options.c_oflag &= ~OPOST;

	tcsetattr(fd,TCSANOW,&options);
}
