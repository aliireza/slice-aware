/* 
 * This program generates random number with uniform distribution
 *
 * Copyright (c) 2019, Alireza Farshin, KTH Royal Institute of Technology - All Rights Reserved
 */

#include <iostream>
#include <random>
#include <iomanip>

template< typename T >
std::string int_to_hex( T i )
{
  std::stringstream stream;
  stream //<< "0x" 
         << std::setfill ('0') << std::setw(sizeof(T)*2) 
         << std::hex << i;
  return stream.str();
}

int main(int argc, char **argv)
{
	if(argc!=3){
		std::cout<<"Wrong input! Enter the range and number for random number!"<<std::endl;
		exit(1);
	}
	unsigned long long range = strtol(argv[1], nullptr, 0);
	unsigned long long limit = strtol(argv[2], nullptr, 0);
	range--;
	limit--;
  	std::default_random_engine generator;
  	for (int i=0; i<limit; ++i) {
  		std::uniform_int_distribution<uint64_t> distribution(0,range);
		uint64_t number = distribution(generator);
    		std::cout << number << std::endl;
		//std::cout << int_to_hex<uint64_t>(1000) << std::endl;
  	}

  	return 0;
}
