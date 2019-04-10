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

#define SBF_DLL

#include "sbf.h"

#include <fstream>
#include <iostream>

#include <openssl/md4.h>
#include <openssl/md5.h>
#include <openssl/rand.h>
#include <openssl/sha.h>


namespace sbf{

/* **************************** PRIVATE METHODS **************************** */


// Sets the hash digest length depending on the selected hash function
void SBF::SetHashDigestLength()
{
    switch(this->HASH_family){
        case 1:
            this->HASH_digest_length = SHA_DIGEST_LENGTH;
            break;
        case 4:
            this->HASH_digest_length = MD4_DIGEST_LENGTH;
            break;
        case 5:
            this->HASH_digest_length = MD5_DIGEST_LENGTH;
            break;
        default:
            this->HASH_digest_length = MD4_DIGEST_LENGTH;
            break;
    }
}


// Computes the hash digest, calling the selected hash function
// char *d            is the input of the hash value
// size_t n           is the input length
// unsigned char *md  is where the output should be written
void SBF::Hash(char *d, size_t n, unsigned char *md) const
{
    switch(this->HASH_family){
        case 1:
            SHA1((unsigned char*)d, n, (unsigned char*)md);
            break;
        case 4:
            MD4((unsigned char*)d, n, (unsigned char*)md);
            break;
        case 5:
            MD5((unsigned char*)d, n, (unsigned char*)md);
            break;
        default:
            MD4((unsigned char*)d, n, (unsigned char*)md);
            break;
    }
}


// Stores a hash salt byte array for each hash (the number of hashes is
// HASH_number). Each input element will be combined with the salt via XOR, by
// the Insert and Check methods. The length of salts is MAX_INPUT_SIZE bytes.
// Hashes are stored encoded in base64.
// TODO: remove printf and manage with an exception the case of a failed salt
void SBF::CreateHashSalt(std::string path)
{
    BYTE buffer[SBF::MAX_INPUT_SIZE];
    int rc;
    std::ofstream myfile;

    myfile.open (path.c_str());

    for(int i = 0; i < this->HASH_number; i++)
    {
        rc = RAND_bytes(buffer, sizeof(buffer));
        if(rc != 1) {
            //printf("Failed to generate hash salt.\n");
            abort();
        }

        // Fills hash salt matrix
        memcpy(this->HASH_salt[i], buffer, SBF::MAX_INPUT_SIZE);
        // Writes hash salt to disk to the path given in input
        std::string encoded = base64_encode(reinterpret_cast<const unsigned char*>(this->HASH_salt[i]), SBF::MAX_INPUT_SIZE);
        myfile << encoded << std::endl;

    }

    myfile.close();
}


// Loads from the path in input a hash salt byte array, one line per hash.
// Hashes are stored encoded in base64, and need to be decoded.
// TODO: manage exceptions for the getline, in particular when the number
//       of lines does not match with
void SBF::LoadHashSalt(std::string path)
{
    std::ifstream myfile;
    std::string line;

    myfile.open(path.c_str());

    for(int i = 0; i < this->HASH_number; i++)
    {
        // Reads one base64 hash salt from file (one per line)
        getline(myfile, line);

        //decode and fill hash salt matrix
        memcpy(this->HASH_salt[i], base64_decode(line).c_str(), SBF::MAX_INPUT_SIZE);
    }

    myfile.close();
}


// Sets the cell to the specified input (the area label). This method is called
// by Insert with the cell index, and the area label. It manages the two
// different possible cell sizes (one or two bytes) automatically set during
// filter construction.
// TODO: remove printf and manage with an exception area values out of bounds
void SBF::SetCell(unsigned int index, int area)
{
    int cell_value;

    switch (this->cell_size){
        // 1-byte cell size
        case 1:
            if((area>255)|(area<0)){
                //printf("Area label must be in [1,255]\n");
                break;
            }
            // Collisions handling
            cell_value = this->GetCell(index);
            if(cell_value == 0){
                //set cell value
                this->filter[index] = (BYTE)area;
                this->AREA_cells[area]++;
            }
            else if(cell_value < area){
                // Sets cell value
                this->filter[index] = (BYTE)area;
                this->collisions++;
                this->AREA_cells[area]++;
                this->AREA_cells[cell_value]--;
            }
            else if(cell_value == area){
                this->collisions++;
                this->AREA_self_collisions[area]++;
            }
            // This condition should never be reached as long as elements are
            // processed in ascending order of area label
            else if(cell_value > area){
                this->collisions++;
            }
            break;
        // 2-bytes cell size. Writing values over the two bytes is managed
        // manually, by copying byte per byte
        case 2:
            if((area>65535)|(area<0)){
                //printf("Area label must be in [1,65535]\n");
                break;
            }
            // Collisions handling
            cell_value = this->GetCell(index);
            if(cell_value == 0){
                //set cell value
                //copy area label (one byte at a time) in two adjacent bytes
                this->filter[2*index] = (BYTE)(area>>8);
                this->filter[(2*index)+1] = (BYTE)area;
                this->AREA_cells[area]++;
            }
            else if(cell_value < area){
                // Sets cell value
                // Copies area the label (one byte at a time) in two adjacent bytes
                this->filter[2*index] = (BYTE)(area>>8);
                this->filter[(2*index)+1] = (BYTE)area;
                this->collisions++;
                this->AREA_cells[area]++;
                this->AREA_cells[cell_value]--;
            }
            else if(cell_value == area){
                this->collisions++;
                this->AREA_self_collisions[area]++;
            }
            // This condition should never be reached as long as elements are
            // processed in ascending order of area label. Self-collisions may
            // contain several miscalculations if elements are not passed
            // following the ascending order of area labels.
            else if(cell_value > area){
                this->collisions++;
            }
            break;
        default:
            break;
    }

}


// Returns the area label stored at the specified index
int SBF::GetCell(unsigned int index) const
{
    int area;
    switch (this->cell_size){
        // 1-byte cell size
        case 1:
            return (int)this->filter[index];
            break;
        // 2-bytes cell size:
        // here we need to copy values byte per byte
        case 2:
            // Copies area label (one byte at a time) in two adjacent bytes
            area = (int)((this->filter[2*index] << 8) | this->filter[(2*index)+1]);
            return area;
            break;
        default:
            // This condition should never be reached
            return -1;
            break;
    }
}


/* ***************************** PUBLIC METHODS ***************************** */


// Prints the filter and related statistics to the standart output
// mode: 0    prints SBF stats only
// mode: 1    prints SBF information and the full SBF content
void SBF::PrintFilter(const int mode) const
{
    int potential_elements;

    printf("Spatial Bloom Filter details:\n\n");

    printf("HASH details:\n");
    printf("Hash family: %d\n",this->HASH_family);
    printf("Number of hash runs: %d\n\n",this->HASH_number);

    printf("Filter details:\n");
    printf("Number of cells: %d\n",this->cells);
    printf("Size in Bytes: %d\n",this->size);
    printf("Filter sparsity: %.5f\n",this->GetFilterSparsity());
	printf("Filter a-priori fpp: %.5f\n", this->GetFilterAPrioriFpp());
    printf("Filter fpp: %.5f\n",this->GetFilterFpp());
	printf("Filter a-priori safeness probability: %.5f\n", this->safeness);
    printf("Number of mapped elements: %d\n",this->members);
    printf("Number of hash collisions: %d\n",this->collisions);

    if(mode==1){
        printf("\nFilter cells content:");
        for(int i = 0; i < this->size; i+=this->cell_size)
        {
            // For readability purposes, we print a line break after 32 cells
            if(i%(32*this->cell_size)==0)printf("\n");
            switch(this->cell_size){
                case 1:
                    std::cout << (unsigned int)(filter[i]) << "|";
                    break;
                case 2:
                    unsigned long label;
                    label = (filter[i]<<8) | (filter[i+1]);
                    std::cout << (unsigned int)label  << "|";
                    break;
                default:
                    break;
            }
        }
        printf("\n\n");
    }
    else printf("\n");

    printf("Area-related parameters:\n");
    for(int j = 1; j < this->AREA_number+1; j++){
        potential_elements = (this->AREA_members[j]*this->HASH_number)-this->AREA_self_collisions[j];
        printf("Area %d: %d members, %d expected cells, %d cells out of %d potential (%d self-collisions)",j,this->AREA_members[j],this->AREA_expected_cells[j],this->AREA_cells[j],potential_elements,this->AREA_self_collisions[j]);
        printf("\n");
    }

    printf("\nEmersion, Fpp, Isep:\n");
    for(int j = 1; j < this->AREA_number+1; j++){
        printf("Area %d: expected emersion %.5f, emersion %.5f, a-priori fpp %.5f, fpp %.5f, a-priori isep %.5f, expected ise %.5f, isep %.5f, a-priori safep %.5f",j,this->GetExpectedAreaEmersion(j),this->GetAreaEmersion(j),this->AREA_a_priori_fpp[j],this->AREA_fpp[j],this->AREA_a_priori_isep[j], this->AREA_a_priori_isep[j]*this->AREA_members[j], this->AREA_isep[j], this->AREA_a_priori_safep[j]);
        printf("\n");
    }
    printf("\n");
}


// Prints the filter and related statistics onto a CSV file (path)
// mode: 1    writes SBF metadata (CSV: key;value)
// mode: 0    writes SBF cells (CSV: value)
void SBF::SaveToDisk(const std::string path, int mode)
{
    std::ofstream myfile;

    myfile.open (path.c_str());

	myfile.setf(std::ios_base::fixed, std::ios_base::floatfield);
	myfile.precision(5);

    if(mode){

        myfile << "hash_family" << ";" << this->HASH_family << std::endl;
        myfile << "hash_number" << ";" << this->HASH_number << std::endl;
        myfile << "area_number" << ";" << this->AREA_number << std::endl;
        myfile << "bit_mapping" << ";" << this->bit_mapping << std::endl;
        myfile << "cells_number" << ";" << this->cells << std::endl;
        myfile << "cell_size" << ";" << this->cell_size << std::endl;
        myfile << "byte_size" << ";" << this->size << std::endl;
        myfile << "members" << ";" << this->members << std::endl;
        myfile << "collisions" << ";" << this->collisions << std::endl;
        myfile << "sparsity" << ";" << this->GetFilterSparsity() << std::endl;
		myfile << "a-priori fpp" << ";" << this->GetFilterAPrioriFpp() << std::endl;
        myfile << "fpp" << ";" << this->GetFilterFpp() << std::endl;
		myfile << "a-priori safeness probability" << ";" << this->safeness << std::endl;
        // area-related parameters:
        // area,members,expected cells,self-collisions,cells,expected emersion,emersion,a-priori fpp,fpp,a-priori isep,expected ise,isep,a-priori safep
		myfile << "area" << ";" << "members" << ";" << "expected cells" << ";" << "self-collisions" << ";" << "cells" << ";" << "expected emersion" << ";" << "emersion" << ";" << "a-priori fpp" << ";" << "fpp" << ";" << "a-priori isep" << ";" << "expected ise" << ";" << "isep" << ";" << "a-priori safep" << std::endl;
        for(int j = 1; j < this->AREA_number+1; j++){
			myfile << j << ";" << this->AREA_members[j] << ";" << this->AREA_expected_cells[j] << ";" << this->AREA_self_collisions[j] << ";" << this->AREA_cells[j] << ";" << this->GetExpectedAreaEmersion(j) << ";" << this->GetAreaEmersion(j) << ";" << this->AREA_a_priori_fpp[j] << ";" << this->AREA_fpp[j] << ";" << this->AREA_a_priori_isep[j] << ";" << (float)this->AREA_members[j] * this->AREA_a_priori_isep[j] << ";" << (float)this->AREA_isep[j] << ";" << this->AREA_a_priori_safep[j] << std::endl;
        }

    }
    else{
        for(int i = 0; i < this->size; i+=this->cell_size)
        {
            switch(this->cell_size){
                case 1:
                    myfile << (unsigned int)(filter[i]) << std::endl;
                    break;
                case 2:
                    unsigned long label;
                    label = (filter[i]<<8) | (filter[i+1]);
                    myfile << (unsigned int)label  << std::endl;
                    break;
                default:
                    break;
            }
        }
    }

    myfile.close();
}


// Maps a single element (passed as a char array) to the SBF. For each hash
// function, internal method SetCell is called, passing elements coupled with
// the area labels. The elements MUST be passed following the ascending-order
// of area labels. If this is not the case, the self-collision calculation (done
// by SetCell) will likely be wrong.
// char *string     element to be mapped
// int size         length of the element
// int area         the area label
void SBF::Insert(const char *string, const int size, const int area)
{
    char* buffer = new char[size];

    // We allow a maximum SBF mapping of 32 bit (resulting in 2^32 cells).
    // Thus, the hash digest is limited to the first four bytes.
    unsigned char digest32[SBF::MAX_BYTE_MAPPING];

    unsigned char* digest = new unsigned char[this->HASH_digest_length];

    // Computes the hash digest of the input 'HASH_number' times; each
    // iteration combines the input char array with a different hash salt
    for(int k=0; k<this->HASH_number; k++){

        for(int j=0; j<size; j++){
            buffer[j] = (char)(string[j]^this->HASH_salt[k][j]);
        }

        this->Hash(buffer, size, (unsigned char*)digest);

        // Truncates the digest after the first 32 bits (see above)
        for(int i = 0; i < SBF::MAX_BYTE_MAPPING; i++){
             digest32[i] = digest[i];
        }

        // Copies the truncated digest (one byte at a time) in an integer
        // variable (endian independent)
		unsigned int digest_index;
		if (this->BIG_end) {
			digest_index = (digest32[0] << 24) | (digest32[1] << 16) | (digest32[2] << 8) | digest32[3];
		}
		else
		{
			digest_index = (digest32[3] << 24) | (digest32[2] << 16) | (digest32[1] << 8) | digest32[0];
		}
        

        // Shifts bits in order to preserve only the first 'bit_mapping'
        // least significant bits
        digest_index >>= (SBF::MAX_BIT_MAPPING - this->bit_mapping);

        this->SetCell(digest_index, area);

    }

    this->members++;
    this->AREA_members[area]++;

	delete[] buffer;
	delete[] digest;
}

// Verifies weather the input element belongs to one of the mapped sets.
// Returns the area label (i.e. the identifier of the set) if the element
// belongs to a set, 0 otherwise.
// char *string     the element to be verified
// int size         length of the element
int SBF::Check(const char *string, const int size) const
{
    char* buffer = new char[size];
    int area = 0;
    int current_area = 0;

    // We allow a maximum SBF mapping of 32 bit (resulting in 2^32 cells).
    // Thus, the hash digest is limited to the first four bytes.
    unsigned char digest32[SBF::MAX_BYTE_MAPPING];

    unsigned char* digest = new unsigned char[this->HASH_digest_length];

    // Computes the hash digest of the input 'HASH_number' times; each
    // iteration combines the input char array with a different hash salt
    for(int k=0; k<this->HASH_number; k++){

        for(int j=0; j<size; j++){
            buffer[j] = (char)(string[j]^this->HASH_salt[k][j]);
        }

        this->Hash(buffer, size, (unsigned char*)digest);

        // Truncates the digest to the first 32 bits
        for(int i = 0; i < SBF::MAX_BYTE_MAPPING; i++){
             digest32[i] = digest[i];
        }

		// Copies the truncated digest (one byte at a time) in an integer
		// variable (endian independent)
		unsigned int digest_index;
		if (this->BIG_end) {
			digest_index = (digest32[0] << 24) | (digest32[1] << 16) | (digest32[2] << 8) | digest32[3];
		}
		else
		{
			digest_index = (digest32[3] << 24) | (digest32[2] << 16) | (digest32[1] << 8) | digest32[0];
		}

        // Shifts bits in order to preserve only the first 'bit_mapping' least
        // significant bits
        digest_index >>= (SBF::MAX_BIT_MAPPING - this->bit_mapping);

        current_area = this->GetCell(digest_index);

        // If one hash points to an empty cell, the element does not belong
        // to any set.
        if(current_area==0) return 0;
        // Otherwise, stores the lower area label, among those which were returned
        else if(area == 0) area = current_area;
        else if(current_area < area) area = current_area;
    }

	delete[] buffer;
	delete[] digest;
    return area;
}


// Computes a-priori area-specific inter-set error probability (a_priori_isep)
// Computes a-priori area-specific safeness probability (a_priori_safep) and
// the overall safeness probability for the entire filter
void SBF::SetAPrioriAreaIsep()
{
	double p1, p2, p3;
	int nfill;

	p3 = 1;

	for (int i = this->AREA_number; i>0; i--) {
		nfill = 0;

		for (int j = i+1; j <= this->AREA_number; j++) {
			nfill += this->AREA_members[j];
		}

		p1 = (double)(1 - 1 / (double)this->cells);
		p1 = (double)(1 - (double)pow(p1, this->HASH_number*nfill));
		p1 = (double)pow(p1, this->HASH_number);

		p2 = (double)(1 - p1);
		p2 = (double)pow(p2, this->AREA_members[i]);

		p3 *= p2;

		this->AREA_a_priori_isep[i] = (float)p1;
		this->AREA_a_priori_safep[i] = (float)p2;
		
	}

	this->safeness = (float)p3;

}


// Computes a-posteriori area-specific inter-set error probability (isep)
void SBF::SetAreaIsep()
{
	double p;

	for (int i = this->AREA_number; i>0; i--) {

		p = (double)(1 - (double)this->GetAreaEmersion(i));
		p = (double)pow(p, this->HASH_number);

		this->AREA_isep[i] = (float)p;

	}
}


// Computes the expected number of cells for each area (expected_cells)
void SBF::SetExpectedAreaCells()
{
	double p1, p2;
	int nfill;


	for (int i = this->AREA_number; i>0; i--) {
		nfill = 0;

		for (int j = i + 1; j <= this->AREA_number; j++) {
			nfill += this->AREA_members[j];
		}

		p1 = (double)(1 - 1 / (double)this->cells);
		p2 = (double)pow(p1, this->HASH_number*nfill);
		p1 = (double)(1 - (double)pow(p1, this->HASH_number*this->AREA_members[i]));

		p1 = (double)(this->cells*p1*p2);

		this->AREA_expected_cells[i] = (int)round(p1);

	}
}


// Computes a-priori area-specific false positives probability (a_priori_fpp)
void SBF::SetAPrioriAreaFpp()
{
	double p;
	int c;

	for (int i = this->AREA_number; i>0; i--) {
		c = 0;

		for (int j = i; j <= this->AREA_number; j++) {
			c += this->AREA_members[j];
		}

		p = (double)(1 - 1 / (double)this->cells);
		p = (double)(1 - (double)pow(p, this->HASH_number*c));
		p = (double)pow(p, this->HASH_number);

		this->AREA_a_priori_fpp[i] = (float)p;

		for (int j = i; j<this->AREA_number; j++) {
			this->AREA_a_priori_fpp[i] -= this->AREA_a_priori_fpp[j + 1];
		}
		if (AREA_a_priori_fpp[i]<0) AREA_a_priori_fpp[i] = 0;
	}
}


// Computes a-posteriori area-specific false positives probability (fpp)
void SBF::SetAreaFpp()
{
    double p;
    int c;

    for(int i = this->AREA_number; i>0; i--){
        c = 0;

        for(int j=i; j<=this->AREA_number; j++){
            c += this->AREA_cells[j];
        }

        p = (double)c/(double)this->cells;
        p = (double)pow(p,this->HASH_number);

		this->AREA_fpp[i] = (float)p;

        for(int j=i; j<this->AREA_number; j++){
            this->AREA_fpp[i] -= this->AREA_fpp[j+1];
        }
        if(AREA_fpp[i]<0) AREA_fpp[i]=0;
    }
}


// Returns the number of inserted elements for the input area
int SBF::GetAreaMembers(const int area) const
{
	return this->AREA_members[area];
}


// Returns the sparsity of the entire SBF
float SBF::GetFilterSparsity() const
{
    float ret;
    int sum = 0;
    for(int i = 1; i < this->AREA_number+1; i++){
        sum += this->AREA_cells[i];
    }
    ret = 1-((float)sum/(float)this->cells);

    return ret;
}


// Returns the a-priori false positive probability over the entire filter
// (i.e. not area-specific)
float SBF::GetFilterAPrioriFpp() const
{
	double p;
	
	p = (double)(1 - 1 / (double)this->cells);
	p = (double)(1 - (double)pow(p, this->HASH_number*this->members));
	p = (double)pow(p, this->HASH_number);

	return (float)p;
}


// Returns the a-posteriori false positive probability over the entire filter
// (i.e. not area-specific)
float SBF::GetFilterFpp() const
{
	double p;
    int c = 0;
    // Counts non-zero cells
    for(int i = 1; i < this->AREA_number+1; i++){
        c += this->AREA_cells[i];
    }
    p = (double)c/(double)this->cells;

    p = (double)(pow(p,this->HASH_number));

    return (float)p;
}


// Returns the expected emersion value for the input area
float SBF::GetExpectedAreaEmersion(const int area) const
{
	double p;
	int nfill = 0;

	for (int j = area + 1; j <= this->AREA_number; j++) {
		nfill += this->AREA_members[j];
	}

	p = (double)(1 - 1 / (double)this->cells);
	p = (double)pow(p, this->HASH_number*nfill);
	
	return (float)p;
}


// Returns the emersion value for the input area
float SBF::GetAreaEmersion(const int area) const
{
    float ret, a, b;
    if((this->AREA_members[area]==0) || (this->HASH_number==0)) ret = -1;
    else{
        a = (float)this->AREA_cells[area];
        b = (float)((this->AREA_members[area]*this->HASH_number) - this->AREA_self_collisions[area]);
        ret = a/b;
    }
    return ret;
}




} //namespace sbf