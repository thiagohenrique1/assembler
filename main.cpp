#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <bitset>
#include <vector>
#include <optional>
#include <cmath>
#include <unordered_map>
#include <iomanip>
#include "Serial.h"
#include "unistd.h"

using std::string;
using std::vector;
using label_hash = std::unordered_map<string, uint16_t>;
using instr_text = vector<std::pair<string, string>>;

struct instr {
	string name;
	string opcode;
	int type;
};

//#define set_bootloader

#ifdef set_bootloader
	#define default_addr 0
#else
	#define default_addr 0x21
//#define default_addr 0x0
#endif

template <int bit_size>
string get_imm(std::stringstream &line_ss, label_hash &labels);
string get_reg_addr(std::stringstream& line_ss);
std::optional<instr> get_opcode(string& instr_name, vector<instr> instr_list);
std::vector<instr> get_instr_list();
std::tuple<instr_text, label_hash, int> preprocess(std::ifstream &file);

int main() {
	Serial serial("/dev/ttyUSB0");
	#ifdef set_bootloader
	std::ofstream instructions_bin("/home/thiago/Workspace/CPU/instructions.txt");
	#else
	std::ofstream instructions_bin("instructions.txt");
	#endif
	int addr = default_addr;

	auto instr_list = get_instr_list();
	std::ifstream file("test.asm");
	auto [instructions, labels, instr_count] = preprocess(file);

	serial.send(static_cast<uint16_t>(instr_count));

	file.close();

	for(auto& [line, original_line] : instructions) {
		std::stringstream ss;
		string instr_name;
		ss << line;

		ss >> instr_name;
		if(auto opt_i = get_opcode(instr_name, instr_list)) {
			instr i = opt_i.value();
			std::stringstream bin;
			bin << i.opcode;

			switch (i.type) {
				case 1: {
					string ra = get_reg_addr(ss);
					string rb = get_reg_addr(ss);
					string rc = get_reg_addr(ss);
					bin << "00" << rc << rb << ra;
					break;
				} case 2: {
					string ra = get_reg_addr(ss);
					string rb = "000";
					if(instr_name != "br")
						rb = get_reg_addr(ss);
					string imm3 = "00000";
					if(instr_name != "br" && instr_name != "cmp") {
						imm3 = get_imm<5>(ss, labels);
					}
					bin << imm3 << rb << ra;
					break;
				} case 3: {
					string ra = get_reg_addr(ss);
					string imm2 = get_imm<8>(ss, labels);
					bin << imm2 << ra;
					break;
				} case 4: {
					string imm1 = get_imm<11>(ss, labels);
					bin << imm1;
					break;
				} default: break;
			}
//			Print binary
			std::cout << original_line << ":" << std::endl;
			std::cout << bin.str();
			instructions_bin << bin.str() << std::endl;

//			Print hex
			uint16_t binary = uint16_t(std::bitset<16>(bin.str()).to_ulong());
			std::cout << " - Hex: 0x" << std::setw(4) <<std::setfill('0')
					  << std::uppercase << std::hex << binary;
			std::cout << " - Addr: " << addr++ << std::endl;

			serial.send(std::bitset<16>(bin.str()));
		} else {
			std::cout << "Invalid instruction" << std::endl;
			break;
		}
	}
//	serial.send(0x1234);
//	int rec = serial.receive();
//	std::cout << std::to_string(rec) << std::endl;
	return 0;
}

template <int bit_size>
string get_imm(std::stringstream &line_ss, label_hash &labels) {
	uint16_t imm_int;
	string imm;
	line_ss >> imm;

	if(labels.count(imm)) {
		imm_int = labels[imm];
	} else {
		imm_int = uint16_t(std::stoi(imm));
		double max = std::exp2(bit_size) - 1;
		if(std::abs(std::stoi(imm)) > max)
			std::cout << "abs(imm) is bigger than maxsize (" << max << ")" << std::endl;
	}

	return std::bitset<bit_size>(imm_int).to_string();
}

string get_reg_addr(std::stringstream& line_ss) {
	if(string reg; line_ss >> reg) {
		if(reg == "lr") return "111";
		if(reg == "sp") return "110";
		if(reg[0] == 'r')
			return std::bitset<3>((uint8_t) reg[1]).to_string();
	}
	std::cout << "Not a register" << std::endl;
	return {};
}

std::optional<instr> get_opcode(string& instr_name, vector<instr> instr_list) {
	auto it = std::find_if(instr_list.begin(), instr_list.end(),
						   [&instr_name](const instr& op) {
			return op.name == instr_name;
	});
	if (it != instr_list.end()) return *it;
	else return {};
}

std::vector<instr> get_instr_list() {
	std::vector<instr> instr_list;

//	Type 1
	uint8_t alu_op = 0;
	for (auto& s : {"add","sub","not","and", "or", "xor", "mult", "shiftr"}) {
		string opcode = "00";
		opcode += std::bitset<3>(alu_op++).to_string();
		instr_list.push_back({s,opcode,1});
	}

//	Type 2
	instr_list.push_back({"cmp","11000",2});
	instr_list.push_back({"load_r","11001",2});
	instr_list.push_back({"store_r","11010",2});
	instr_list.push_back({"br","11011",2});
	alu_op = 0;
	for (auto& s : {"addi","subi","noti","andi", "ori", "xori", "multi", "shiftri"}) {
		string opcode = "01";
		opcode += std::bitset<3>(alu_op++).to_string();
		instr_list.push_back({s,opcode,2});
	}

//	Type 3
	instr_list.push_back({"cmpi","11100",3});
	instr_list.push_back({"load","11101",3});
	instr_list.push_back({"store","11110",3});
	instr_list.push_back({"load_i","11111",3});

//	Type 4
	instr_list.push_back({"b","10000",4});
	instr_list.push_back({"beq","10001",4});
	instr_list.push_back({"bne","10010",4});
	instr_list.push_back({"bhe","10011",4});
	instr_list.push_back({"bst","10100",4});
	instr_list.push_back({"bleq","10101",4});
	instr_list.push_back({"blne","10110",4});
	instr_list.push_back({"bl","10111",4});

	return instr_list;
}

std::tuple<instr_text, label_hash, int> preprocess(std::ifstream &file) {
	instr_text instructions;
	label_hash labels;
	std::string line;
	uint16_t addr = default_addr;
	while(std::getline(file, line)) {
		if(size_t pos = line.find(':'); pos != string::npos) {
			labels[line.substr(0, pos)] = addr;
		} else if(!line.empty()){
			std::string original_line = line;
			std::replace(line.begin(), line.end(), ',', ' ');
			instructions.push_back({line, original_line});
			addr++;
		}
	}
	return {instructions, labels, addr - default_addr};
}
