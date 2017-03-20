# libSBF-cpp #
libSBF - implementation of the Spatial Bloom Filters in C++

## Synopsys ##
The Spatial Bloom Filters (SBF) are a compact, set-based data structure that extends the original Bloom filter concept. An SBF represents and arbitrary number of sets, and their respective elements (as opposed to Bloom filters, which represent the elements of a single set). SBFs are particularly suited to be used in privacy-preserving protocols, as set-membership queries (i.e. the process of verifying if an element is in a set) can be easily computed over an encrypted SBF, through (somewhat) homomorphic encryption.

## Usage ##
Spatial Bloom Filters have been first proposed for use in location-privacy application, but have found application in a number of domains, including network security and the Internet of Things.

The libSBF-cpp repository contains the C++ implementation of the SBF data structure. The SBF class is provided, as well as various methods for managing the filter:
- once the filter is constructed, the user can insert elements into it through the Insert method. The Check method, on the contrary, is used to verify weather an element belongs to one of the mapped sets.
- methods SetAreaFpp, GetFilterSparsity, GetFilterFpp, GetAreaEmersion and GetAreaFlotation allow to compute and return several probabilistic properties of the constructed filter.
- finally, two methods are provided to print out the filter: PrintFilter prints the filter and related statistics to the standart output whereas SaveToDisk writes the filter onto a CSV file.

For more details on the implementation, and how to use the library please refer to the <a href="http://sbf.csr.unibo.it/">homepage</a> of the project.

## Bibliography ##
The SBFs have been proposed in the following research papers:
- Luca Calderoni, Paolo Palmieri, Dario Maio: <i>Location privacy without mutual trust: The spatial Bloom filter.</i> Computer Communications, vol. 68, pp. 4-16, September 2015. ISSN 0140-3664
- Paolo Palmieri, Luca Calderoni, Dario Maio: <i>Spatial Bloom Filters: Enabling Privacy in Location-aware Applications</i>. In: Inscrypt 2014. Lecture Notes in Computer Science, vol. 8957, pp. 16–36, Springer, 2015.

## Authors ##
Luca Calderoni, Dario Maio, University of Bologna

Paolo Palmieri, Cranfield University

## Licence ##
The source code is released under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version. Please refer to the included files COPYING and COPYING.LESSER.

## Acknowledgements ##
This project uses, where posible, existing libraries. In particular:
- We use the OpenSSL implementation of hash functions, copyright of The OpenSSL Project. Please refer to https://www.openssl.org/source/license.txt for OpenSSL project licence details.
- The functions implementing base64 encoding and decoding (provided in base64.cpp and base64.h) was written by René Nyffenegger (rene.nyffenegger@adp-gmbh.ch). A full copyright statement is provided within each file.
