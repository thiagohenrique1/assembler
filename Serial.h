#ifndef ASSEMBLER_SERIAL_H
#define ASSEMBLER_SERIAL_H

#include <string>
#include <bitset>

class Serial {
	private:
		int fd;

	public:
		explicit Serial(const std::string &port);
		void send(uint16_t data);
		void send(std::bitset<16> data);
		uint8_t receive();
};

#endif //ASSEMBLER_SERIAL_H
