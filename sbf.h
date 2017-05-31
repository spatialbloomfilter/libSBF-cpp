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

#pragma once

#ifndef SBF_H
#define SBF_H 

// OS specific headers
#if defined(__MINGW32__) || defined(__MINGW64__)
#include <windef.h>
#include "win/libexport.h"
#elif defined(_MSC_VER)
#include <windows.h>
#include "win/libexport.h"
#define WIN32_LEAN_AND_MEAN
#elif __GNUC__
#include "linux/lindef.h"
#include "linux/libexport.h"
#endif

#include "end.h"

#include <fstream>
#include <iostream>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "base64.h"


namespace sbf {

	// The SBF class implementing the Spatial Bloom FIlters
	class DLL_PUBLIC SBF
	{

	private:
		BYTE *filter;
		BYTE ** HASH_salt;
		int bit_mapping;
		int cells;
		int cell_size;
		int size;
		int HASH_family;
		int HASH_number;
		int HASH_digest_length;
		int members;
		int collisions;
		int AREA_number;
		int *AREA_members;
		int *AREA_cells;
		int *AREA_self_collisions;
		float *AREA_a_priori_fpp;
		float *AREA_fpp;
		float *AREA_a_priori_isep;
		int BIG_end;

		// Private methods (commented in the sbf.cpp)
		void SetCell(unsigned int index, int area);
		int GetCell(unsigned int index);
		void CreateHashSalt(std::string path);
		void LoadHashSalt(std::string path);
		void SetHashDigestLength();
		void Hash(char *d, size_t n, unsigned char *md);


	public:
		// The maximum string (as a char array) length in bytes of each element
		// given as input to be mapped in the SBF
		const static int MAX_INPUT_SIZE = 128;
		// This value defines the maximum size (as in number of cells) of the SBF:
		// MAX_BIT_MAPPING = 32 states that the SBF will be composed at most by
		// 2^32 cells. The value is the number of bits used for SBF indexing.
		const static int MAX_BIT_MAPPING = 32;
		// Utility byte value of the above MAX_BIT_MAPPING
		const static int MAX_BYTE_MAPPING = MAX_BIT_MAPPING / 8;
		// The maximum number of allowed areas. This way, we limit the memory size 
		// (which is the memory size of each cell) to 2 bytes
		const static int MAX_AREA_NUMBER = 65535;
		// The maximum number of allowed digests
		const static int MAX_HASH_NUMBER = 1024;

		// SBF class constructor
		// Arguments:
		// bit_mapping    actual size of the filter (as in number of cells): for
		//                instance, bit_mapping = 10 states that the SBF will be
		//                composed by 2^10 cells. As such, the size can only be
		//                defined as a power of 2. This value is bounded by the
		//                MAX_BIT_MAPPING constant.
		// HASH_family    specifies the hash function to be used. Currently available:
		//                1: SHA1
		//                4: MD4
		//                5: MD5
		// HASH_number    number of digests to be produced (by running the hash function
		//                specified above using different salts) in the insertion and
		//                check phases.
		// AREA_number    number of areas over which to build the filter.
		// salt_path      path to the file where to read from/write to the hash salts
		//                (which are used to produce different digests).
		//                If the file exists, reads one salt per line.
		//                If the file doesn't exist, the salts are randomly generated
		//                during the filter creation phase
		SBF(int bit_mapping, int HASH_family, int HASH_number, int AREA_number, std::string salt_path)
		{

			// Argumnet validation
			if (bit_mapping <= 0 || bit_mapping > MAX_BIT_MAPPING) throw std::invalid_argument("Invalid bit mapping.");
			if (AREA_number <= 0 || AREA_number > MAX_AREA_NUMBER) throw std::invalid_argument("Invalid number of areas.");
			if (HASH_number <= 0 || HASH_number > MAX_HASH_NUMBER) throw std::invalid_argument("Invalid number of hash runs.");
			if (salt_path.length() == 0) throw std::invalid_argument("Invalid hash salt path.");

			// Checks whether the execution is being performed on a big endian or little endian machine
			this->BIG_end = is_big_endian();

			// Defines the number of bytes required for each cell depending on AREA_number
			// In order to reduce the memory footprint of the filter, we use 1 byte 
			// for a number of areas <= 255, 2 bytes for a up to MAX_AREA_NUMBER
			if (AREA_number <= 255) this->cell_size = 1;
			else if (AREA_number > 255) this->cell_size = 2;


			// Sets the type of hash function to be used
			this->HASH_family = HASH_family;
			this->SetHashDigestLength();
			// Sets the number of digests
			this->HASH_number = HASH_number;

			// Initializes the HASH_salt matrix
			this->HASH_salt = new BYTE*[HASH_number];
			for (int j = 0; j<HASH_number; j++) {
				this->HASH_salt[j] = new BYTE[SBF::MAX_INPUT_SIZE];
			}

			// Creates the hash salts or loads them from the specified file
			std::ifstream my_file(salt_path.c_str());
			if (my_file.good()) this->LoadHashSalt(salt_path);
			else this->CreateHashSalt(salt_path);

			// Defines the number of cells in the filter
			this->cells = (int)pow(2, bit_mapping);
			this->bit_mapping = bit_mapping;

			// Defines the total size in bytes of the filter
			this->size = this->cell_size*this->cells;

			// Memory allocation for the SBF array
			this->filter = new BYTE[this->size];

			// Initializes the cells to 0
			for (int i = 0; i < this->size; i++) {
				this->filter[i] = 0;
			}

			// Sets the number of mapped areas
			this->AREA_number = AREA_number;
			// Memory allocations for area related parameters
			this->AREA_members = new int[this->AREA_number + 1];
			this->AREA_cells = new int[this->AREA_number + 1];
			this->AREA_self_collisions = new int[this->AREA_number + 1];
			this->AREA_fpp = new float[this->AREA_number + 1];
			this->AREA_a_priori_fpp = new float[this->AREA_number + 1];
			this->AREA_a_priori_isep = new float[this->AREA_number + 1];

			// Parameter initializations
			this->members = 0;
			this->collisions = 0;
			for (int a = 0; a < this->AREA_number + 1; a++) {
				this->AREA_members[a] = 0;
				this->AREA_cells[a] = 0;
				this->AREA_self_collisions[a] = 0;
				this->AREA_fpp[a] = -1;
				this->AREA_a_priori_fpp[a] = -1;
				this->AREA_a_priori_isep[a] = -1;
			}
		}

		// SBF class destructor
		~SBF()
		{
			// Frees the allocated memory
			delete[] filter;
			delete[] AREA_members;
			delete[] AREA_cells;
			delete[] AREA_self_collisions;
			delete[] AREA_fpp;
			delete[] AREA_a_priori_fpp;
			delete[] AREA_a_priori_isep;
			for (int j = 0; j<this->HASH_number; j++) {
				delete[] HASH_salt[j];
			}
			delete[] HASH_salt;
		}


		// Public methods (commented in the sbf.cpp)
		void PrintFilter(int mode);
		void SaveToDisk(std::string path, int mode);
		void Insert(char *string, int size, int area);
		int Check(char *string, int size);
		int GetAreaMembers(int area);
		float GetFilterSparsity();
		float GetFilterFpp();
		float GetFilterAPrioriFpp();
		void SetAreaFpp();
		void SetAPrioriAreaFpp();
		void SetAPrioriAreaIsep();
		float GetAreaEmersion(int area);
	};

} //namespace sbf

#endif /* SBF_H */