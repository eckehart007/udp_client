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
#include <boost/multiprecision/cpp_int.hpp> 

using namespace boost::multiprecision;
using namespace std;

#define _CRT_SECURE_NO_WARNINGS

using boost::asio::ip::udp;

enum SENSOR {
	SENSOR1 = 0,
	SENSOR2 = 1,
	SENSOR3 = 3,
	SENSOR5 = 4,
	SENSOR6 = 5,
	SENSOR7 = 6,
	SENSOR8 = 7,
	SENSOR9 = 8,
	SENSOR10 = 9,
	
	SENSORS_TOTAL = 10
};

#pragma warning(disable : 4996) //to use time(..)
std::string get_timestamp() {
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
		print_timer_(io, boost::posix_time::seconds(1)),
		file_() {
		
		//Init udp client
		socket_.open(udp::v4());
		boost::asio::socket_base::broadcast option(true);
		socket_.set_option(option);
		socket_.bind(udp::endpoint(boost::asio::ip::address_v4::address_v4::any(), 12345));

		//Start timers
		rx_timer_.async_wait(strand_.wrap(boost::bind(&udp_client::upd_client_rx, this)));
		print_timer_.async_wait(strand_.wrap(boost::bind(&udp_client::udp_print, this)));
	}

	void udp_print() {
		std::cout << "timestamp		sensor id		sensor reading\n";
		for (uint8_t sensor = SENSOR1; sensor < SENSORS_TOTAL; sensor++) {
			std::cout << get_timestamp() + "		" + std::to_string(sensor+1) + "			" + get_sensor_average_(sensor) + "\n";
		}
		std::cout << "\n";
		

		//Reinit timer
		print_timer_.expires_at(print_timer_.expires_at() + boost::posix_time::seconds(1));
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
			sensor_data_ = (uint64_t)(recv_buf.data()[2]) 
							|  ((uint64_t)(recv_buf.data()[3]) << 8)
							| ((uint64_t)(recv_buf.data()[4]) << 16)
							| ((uint64_t)(recv_buf.data()[5]) << 24)
							| ((uint64_t)(recv_buf.data()[6]) << 32)
							| ((uint64_t)(recv_buf.data()[7]) << 40)
							| ((uint64_t)(recv_buf.data()[8]) << 48)
							| ((uint64_t)(recv_buf.data()[9]) << 56);

			file_.csv_writer(get_timestamp(),
							std::to_string(sensor_number_), 
							std::to_string(sensor_data_));
			add_sensor_data_(sensor_number_, sensor_data_);

		}

		//Reinit timer
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
	uint64_t sensor_arr_[SENSORS_TOTAL][10] = {}; //first column data array for every sensor, second row is sensor samples
	uint8_t sensor_arr_index_[SENSORS_TOTAL] = {};
	csv file_;

	void add_sensor_data_(uint8_t sensor_id, uint64_t data) {
		sensor_arr_[sensor_id][sensor_arr_index_[sensor_id]] = data;
		sensor_arr_index_[sensor_id]++;
		if (sensor_arr_index_[sensor_id] > SENSOR10) {
			sensor_arr_index_[sensor_id] = SENSOR1;
		}
	}

	string get_sensor_average_(uint8_t sensor) {
		uint128_t sum_of_sample = 0;
		for (uint8_t sample_index=0; sample_index < 10; sample_index++) {
			sum_of_sample += sensor_arr_[sensor][sample_index];
		}
		return std::to_string((uint64_t)sum_of_sample);
	}
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
