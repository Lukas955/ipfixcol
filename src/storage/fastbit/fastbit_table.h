/**
 * \file template_table.h
 * \author Petr Kramolis <kramolis@cesnet.cz>
 * \brief 
 *
 * Copyright (C) 2011 CESNET, z.s.p.o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the Company nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * ALTERNATIVELY, provided that this notice is retained in full, this
 * product may be distributed under the terms of the GNU General Public
 * License (GPL) version 2 or later, in which case the provisions
 * of the GPL apply INSTEAD OF those given above.
 *
 * This software is provided ``as is, and any express or implied
 * warranties, including, but not limited to, the implied warranties of
 * merchantability and fitness for a particular purpose are disclaimed.
 * In no event shall the company or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 *
 */

#ifndef _TEMPLATE_TABLE_H_
#define _TEMPLATE_TABLE_H_


#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fastbit/ibis.h>
#include <map>
#include <iostream>
#include <string>
#include "pugixml/pugixml.hpp"
#include "fastbit_element.h"

extern "C" {
	//#include <commlbr.h>
	#include "../../../headers/storage.h"
}



class element; //needed because of Circular dependency

/* For each uniq template is created instance of template_table object.
 * The object is used to parse data records belonging to the template
 * */
class template_table 
{
private:
	uint64_t _rows_count;
	uint16_t _template_id;
	int _record_size;
	ibis::tablex * _tablex;
	char _name[10];
	char _index;
public:
	/* vector of elements stored in data record (based on template)
	 * element polymorphs to necessary data type
 	 */
	std::vector<element *> elements;	
	std::vector<element *>::iterator el_it;	

	template_table(int template_id): _rows_count(0) 
	{
		_template_id = template_id;
		sprintf(_name,"%u",template_id);
		_tablex = ibis::tablex::create();
		_index=0;
	}
	int rows() {return _rows_count;}
	void rows(int rows_count) {_rows_count = rows_count;}
	std::string name(){return std::string(_name);}
	int parse_template(struct ipfix_template * tmp);	

	/**
	 * \brief parse data_set and store it's data in memory 
	 *
	 * if memory usage is about to exceed memory limit
	 * data are flushed to disk.
	 *
	 * @param data_set ipfixcol data set
	 * @param path path to direcotry where should be data flushed
	 */
	int store(ipfix_data_set * data_set, std::string path);

	/**
	 * \brief flush data to disk and clean memory
	 *
	 * @param path path to direcotry where should be data flushed
	 */
	void flush(std::string path){ 
		_tablex->write((path + _name).c_str(),_name, "Generated by ipfixcol fastbit plugin", &_index);
		_tablex->clearData();
		_rows_count = 0;
	}
	~template_table();
};

#endif
