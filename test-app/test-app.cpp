/*
Spatial Bloom Filter C++ Library (libSBF-cpp)
Copyright (C) 2017  Luca Calderoni, Dario Maio,
University of Bologna
Copyright (C) 2017  Paolo Palmieri,
Cranfield University

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <sbflib.h>

#include <ctime>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <math.h>
#include <stdlib.h>


//This simple program allows to build a SBF over our test datasets and 
//to perform some tests upon it.
//Specifically, the filter is tested with its own elements and with a larger set
//of elements which don't belong to the filter at all.
int main() {

	std::ifstream myfile;
	std::string line, a, member;
	int len, line_count, area, area_check, n, narea, nver;
	int well_recognised, false_positives, exchanged_elements;
	char* element;
	sbf::SBF* myFilter;

	/* ****************************** SETTINGS ****************************** */

	//csv file delimiter
	std::string delimiter = ",";

	int perform_verification = 0;
	int print_mode = 0;

	//desired false positives probability (upper bound)
	double max_fpp = 0.001;

	//select the construction dataset to be used
	std::string construction_dataset;

	//select the verification dataset
	std::string verification_dataset;

	//specify the hash salt data file
	time_t timer;
	struct tm * timeinfo;
	char buffer[80];
	time(&timer);
	timeinfo = localtime(&timer);
	strftime(buffer, sizeof(buffer), "%d-%m-%Y-%I_%M_%S", timeinfo);
	std::string buf(buffer);
	std::string hash_salt("SBFHashSalt" + buf + ".txt");


	//the exponent that determines filter size (in cells)
	int bit_mapping;
	//the number of hash executions for each mapped element
	int hn;
	//hash function to be used
	int hf = 4;

	/* **************************** END SETTINGS **************************** */


	//prints licence information
	printf("Spatial Bloom Filters\nCopyright (C) 2017  Luca Calderoni, Dario Maio (University of Bologna), Paolo Palmieri (Cranfield University)\nThis program comes with ABSOLUTELY NO WARRANTY. This is free software, and you are welcome\nto redistribute it under certain conditions.\nSee the attached files 'COPYING' and 'COPYING.LESSER' for details.\n\n");



	/* ***************************** USER INPUT ***************************** */

	std::string input;
	//asks for construction dataset file (mandatory)
	std::cout << "Enter the name of the construction dataset (like area-elements-unif.csv)..." << std::endl;
	std::cin >> construction_dataset;
	std::cin.ignore();

	//asks for hash type (optional)
	while (true) {
		std::cout << "Enter the type of hash function to use:" << std::endl;
		std::cout << "1 (SHA1), 4 (MD4), 5(MD5) (press ENTER for default)..." << std::endl;
		getline(std::cin, input);

		if (input.empty()) break;
		else {
			std::istringstream istr(input);
			istr >> hf;
			break;
		}

		std::cout << "Invalid number, please try again" << std::endl;
	}

	//asks for hash salt data file (optional)
	std::cout << "Enter the name of the hash salt data file (like SBFHashSalt.txt)" << std::endl;
	std::cout << "(press ENTER for default)..." << std::endl;
	getline(std::cin, input);

	if (input.empty()) {
		//nothing to do
	}
	else {
		std::istringstream istr(input);
		istr >> hash_salt;
	}

	//asks for verification dataset file (optional)
	std::cout << "Enter the name of the verification dataset (like non-elements.csv)" << std::endl;
	std::cout << "(press ENTER to ignore)..." << std::endl;
	getline(std::cin, input);

	if (input.empty()) {
		//nothing to do
	}
	else {
		perform_verification = 1;
		std::istringstream istr(input);
		istr >> verification_dataset;
	}


	//asks for print mode (optional)
	while (true) {
		std::cout << "Enter the print mode to use:" << std::endl;
		std::cout << "1 (prints filter information to the standard output)" << std::endl;
		std::cout << "2 (prints filter information and cells values to the standard output)" << std::endl;
		std::cout << "3 (save filter statistics and meta data to disk)" << std::endl;
		std::cout << "4 (save both filter and related meta data to disk)" << std::endl;
		std::cout << "(press ENTER to ignore)..." << std::endl;
		getline(std::cin, input);

		if (input.empty()) break;
		else {
			std::istringstream istr(input);
			istr >> print_mode;
			if (print_mode != 1 && print_mode != 2 && print_mode != 3 && print_mode != 4) print_mode = 0;
			break;
		}

		std::cout << "Invalid number, please try again" << std::endl;
	}

	/* *************************** END USER INPUT *************************** */




	//determines the number of elements and the number of areas depending on the
	//chosen dataset
	myfile.open(construction_dataset.c_str());
	if (myfile.is_open()) {
		line_count = 0;
		while (getline(myfile, line)) {
			++line_count;
			a = line.substr(0, line.find(delimiter));
			narea = atoi(a.c_str());
		}
		n = line_count;
		myfile.close();
	}
	else {
		printf("Unable to open file %s", construction_dataset.c_str());
		exit(0);
	}

	//determines the optimal bit_mapping and hash number
	int cells = (int)ceil((double)((double)-n*log(max_fpp)) / pow(log(2), 2));
	bit_mapping = (int)ceil(log2(cells));
	hn = (int)ceil((double)(cells / n)*log(2));

	//constructs the filter and fills it with elements contained in the
	//input dataset
	try {
		//filter construction
		myFilter = new sbf::SBF(bit_mapping, hf, hn, narea, hash_salt);
	}
	catch (const std::invalid_argument& ia)
	{
		std::cerr << ia.what() << std::endl;
	}

	myfile.open(construction_dataset.c_str());
	if (myfile.is_open()) {
		//elements insertion
		for (int i = 0; i < n; ++i)
		{
			//reads one line
			getline(myfile, line);
			a = line.substr(0, line.find(delimiter));
			area = atoi(a.c_str());
			member = line.substr(line.find(delimiter) + 1);
			len = member.length();
			element = new char[len];
			memcpy(element, member.c_str(), len);
			myFilter->Insert(element, len, area);
		}
		myfile.close();
	}
	else {
		printf("Unable to open file %s", construction_dataset.c_str());
		exit(0);
	}

	//calculates filter's probabilistic properties
	myFilter->SetAreaFpp();

	//prints filter to the standard output or saves it to disk
	if (print_mode == 1) myFilter->PrintFilter(0);
	if (print_mode == 2) myFilter->PrintFilter(1);
	if (print_mode == 3) myFilter->SaveToDisk("stats" + buf + ".csv", 1);
	if (print_mode == 4) {
		myFilter->SaveToDisk("filter" + buf + ".csv", 0);
		myFilter->SaveToDisk("stats" + buf + ".csv", 1);
	}


	//operates a self check upon the filter (i.e. runs the Check method for each
	//of the already mapped elements)
	well_recognised = 0, exchanged_elements = 0;
	myfile.open(construction_dataset.c_str());

	if (myfile.is_open()) {
		printf("Self-check:\n");
		for (int i = 0; i < n; i++)
		{
			//reads one line
			getline(myfile, line);
			a = line.substr(0, line.find(delimiter));
			area = atoi(a.c_str());
			member = line.substr(line.find(delimiter) + 1);
			len = member.length();
			element = new char[len];
			memcpy(element, member.c_str(), len);
			area_check = myFilter->Check(element, len);

			if (area == area_check) well_recognised++;
			else {
				exchanged_elements++;
			}

		}
		printf("Well recognised: %d\n", well_recognised);
		printf("Elements assigned to a wrong set: %d\n", exchanged_elements);
		printf("Exchange rate: %.5f\n", (float)exchanged_elements / (float)n);
		myfile.close();
	}
	else {
		printf("Unable to open file %s", construction_dataset.c_str());
		exit(0);
	}

	if (perform_verification) {

		//determines the number of elements depending on the verification dataset
		myfile.open(verification_dataset.c_str());

		if (myfile.is_open()) {
			line_count = 0;
			while (getline(myfile, line)) {
				++line_count;
			}
			nver = line_count;
			myfile.close();
		}
		else {
			printf("Unable to open file %s", verification_dataset.c_str());
			exit(0);
		}

		//operates a verification using non members dataset
		well_recognised = 0, false_positives = 0;
		myfile.open(verification_dataset.c_str());

		if (myfile.is_open()) {
			printf("\nVerification (non-elements):\n");
			for (int i = 0; i < nver; i++)
			{
				//reads one line
				getline(myfile, line);
				len = line.length();
				element = new char[len];
				memcpy(element, line.c_str(), len);
				area = myFilter->Check(element, len);

				if (area == 0)well_recognised++;
				else false_positives++;
			}
			printf("Well recognised: %d\n", well_recognised);
			printf("False positives: %d\n", false_positives);
			printf("False positives rate: %.5f\n", (float)false_positives / (float)nver);
			myfile.close();
		}
		else {
			printf("Unable to open file %s", verification_dataset.c_str());
			exit(0);
		}
	}

	printf("Press any key to continue\n");
	getline(std::cin, input);
	return 0;
}