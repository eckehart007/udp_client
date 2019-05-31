// udp_client.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <string>
#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>  

#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/array.hpp>

#define _CRT_SECURE_NO_WARNINGS

using boost::asio::ip::udp;

#pragma warning(disable : 4996)
std::string get_timestamp()
{
	using namespace std; // For time_t, time and ctime;
	return std::to_string(time(0));
}

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
	:	strand_(io),
		socket_(io),
		rx_timer_(io, boost::posix_time::milliseconds(100)),
		print_timer_(io, boost::posix_time::seconds(10)),
		file_() {
		
		socket_.open(udp::v4());
		boost::asio::socket_base::broadcast option(true);
		socket_.set_option(option);
		socket_.bind(udp::endpoint(boost::asio::ip::address_v4::address_v4::any(), 12345));

		rx_timer_.async_wait(strand_.wrap(boost::bind(&udp_client::upd_client_rx, this)));
		print_timer_.async_wait(strand_.wrap(boost::bind(&udp_client::udp_print, this)));
	}

	void udp_print() {
		std::cout << "Udp_print\r\n";

		print_timer_.expires_at(print_timer_.expires_at() + boost::posix_time::seconds(10));
		print_timer_.async_wait(strand_.wrap(boost::bind(&udp_client::udp_print, this)));
	}

	void upd_client_rx(void) {
		boost::array<char, 128> recv_buf;
		udp::endpoint sender_endpoint;
		size_t len = socket_.receive_from(
			boost::asio::buffer(recv_buf), sender_endpoint);
		if (len > 0) {
			sensor_number_ = recv_buf.data()[0];
			value_type_ = recv_buf.data()[1];
			sensor_data_ = (uint64_t)recv_buf.data()[2] 
							|  (recv_buf.data()[3] << 8) 
							| (recv_buf.data()[4] << 16) 
							| (recv_buf.data()[5] << 24) 
							| (recv_buf.data()[6] << 32)
							| (recv_buf.data()[7] << 40)
							| (recv_buf.data()[8] << 48)
							| (recv_buf.data()[9] << 56);

			file_.csv_writer(get_timestamp(),
							std::to_string(sensor_number_), 
							std::to_string(sensor_data_));
		}
		rx_timer_.expires_at(rx_timer_.expires_at() + boost::posix_time::milliseconds(100));
		rx_timer_.async_wait(strand_.wrap(boost::bind(&udp_client::upd_client_rx, this)));
	}

	~udp_client() {

	}
private:
	udp::socket socket_;
	boost::asio::io_context::strand strand_;
	boost::asio::deadline_timer rx_timer_;
	boost::asio::deadline_timer print_timer_;
	uint8_t sensor_number_;
	uint8_t value_type_;
	uint64_t sensor_data_;

	csv file_;
};

int main() {
	try {
		boost::asio::io_context io;
		udp_client client(io);
		boost::thread t(boost::bind(&boost::asio::io_context::run, &io));
		io.run();
		t.join();
	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
	return 0;
}
