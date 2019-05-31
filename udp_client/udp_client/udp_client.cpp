// udp_client.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <string>
#include <iostream>
#include <fstream>


#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/array.hpp>

using boost::asio::ip::udp;

class csv {
public:
	csv() {
		csv_file_.open("sensors_data.csv", std::ios::out);
		csv_file_ << "timestamp;sensor id;sensor reading;\n";
		csv_file_.close();	
	}

	void csv_writer(std::string timestamp, std::string sensor_id, std::string sensor_reading) {
		csv_file_.open("sensors_data.csv", std::ios::out | std::ios::app);
		csv_file_ << timestamp+ ";" + sensor_id + ";"  + sensor_reading + ";\n";
		csv_file_.close();
	}
private:
	std::ofstream csv_file_;
};

class udp_client {
public:
	udp_client(boost::asio::io_context& io)
	:	socket_(io), 
		rx_timer_(io, boost::asio::chrono::milliseconds(10)) {
		
		socket_.open(udp::v4());
		boost::asio::socket_base::broadcast option(true);
		socket_.set_option(option);
		socket_.bind(udp::endpoint(boost::asio::ip::address_v4::address_v4::any(), 12345));

		rx_timer_.async_wait(boost::bind(&udp_client::upd_cliend_rx, this));
	}

	void upd_cliend_rx(void) {
		boost::array<char, 128> recv_buf;
		udp::endpoint sender_endpoint;
		std::cout << "Recv\r\n";
		size_t len = socket_.receive_from(
			boost::asio::buffer(recv_buf), sender_endpoint);
		for (int i = 0; i < 8; i++) {
			printf("%d ", recv_buf.data()[i]);
		}
		rx_timer_.async_wait(boost::bind(&udp_client::upd_cliend_rx, this));
	}

	~udp_client() {

	}
private:
	udp::socket socket_;
	boost::asio::steady_timer rx_timer_;
};

int main() {
	try {
		csv file;
		file.csv_writer("1", "2", "3");
		boost::asio::io_context io;
		udp_client client(io);
		io.run();
		std::cout << "END\r\n";

	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
	return 0;
}
