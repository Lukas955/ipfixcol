/**
 * \file fastbit_table.cpp
 * \author Petr Kramolis <kramolis@cesnet.cz>
 * \brief methods of object wrapers for information elements.
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


#include "fastbit_table.h"

template_table::~template_table(){
	for (el_it = elements.begin(); el_it!=elements.end(); ++el_it) {
		delete (*el_it);
	}

	delete _tablex;
}

int template_table::store(ipfix_data_set * data_set, std::string path){
	uint8_t *data = data_set->records;
	int ri;
	if (data == NULL){
		return 0;
	}

	//count how many records does data_set contain
	int record_count = (ntohs(data_set->header.length)-(sizeof(struct ipfix_set_header)))/_record_size; 

        for(ri=0;ri<record_count;ri++){
		for (el_it = elements.begin(); el_it!=elements.end(); ++el_it) {
			//CHECK DATA SIZE?!?
			(*el_it)->fill(data);
			_tablex->append((*el_it)->name(), _rows_count, _rows_count+1, (*el_it)->value);
			data += (*el_it)->size();
		}
		_rows_count++;
		if(_rows_count >= RESERVED_SPACE){
			_tablex->write((path + _name).c_str(),_name, "Generated by ipfixcol fastbit plugin", &_index);
			_tablex->clearData();
			for (el_it = elements.begin(); el_it!=elements.end(); ++el_it) {
				(*el_it)->flush(path + _name);
			}
			std::cout << "WRITEN " << _name << " rows: " << _rows_count << std::endl;
			_rows_count = 0;
		}
	}
	return ri;
}

int template_table::parse_template(struct ipfix_template * tmp){
	int i;
	int en = 0; // enterprise number (0 = IANA elements)
	int en_offset = 0;
	template_ie *field;
	element *new_element;
	std::cout << "PARSE TEMPLATE" << std::endl;
	//Is there anything to parse?
	if(tmp == NULL){
		return 1;
	}
	//we dont want to parse option tables ect. so check it!
	if(tmp->template_type != TM_TEMPLATE){
		return 1; 
	}
	_template_id = tmp->template_id;
	_record_size = tmp->data_length;

	//Find elements
	for(i=0;i<tmp->field_count + en_offset;i++){
		field = &(tmp->fields[i]);
		
		//Is this an enterprise element?
		en = 0;
		if(field->ie.id & 0x8000){
			i++;
			en_offset++;
			en = tmp->fields[i].enterprise_number;
		}
		//TODO std::cout << "element size:" << field->ie.length << " id:" << (field->ie.id & 0x7FFF) << " en:" << en << std::endl;
		switch(get_type_from_xml(en, field->ie.id & 0x7FFF)){
			case UINT:
				new_element = new el_uint(field->ie.length, en, field->ie.id & 0x7FFF);
				_tablex->addColumn(new_element->name(), new_element->type());
				break;
			case IPv6:
				/* ipv6 is 128b so we have to split it to two 64b rows!
                                 * adding p0 p1 sufixes to row name
                                 */
				new_element = new el_ipv6(sizeof(uint64_t), en, field->ie.id & 0x7FFF, 1);
				_tablex->addColumn(new_element->name(), new_element->type());
				elements.push_back(new_element);

				new_element = new el_ipv6(sizeof(uint64_t), en, field->ie.id & 0x7FFF, 0);
				_tablex->addColumn(new_element->name(), new_element->type());
				break;
			case INT: 
				new_element = new el_sint(field->ie.length, en, field->ie.id & 0x7FFF);
				_tablex->addColumn(new_element->name(), new_element->type());
				break;
			case FLOAT:
				new_element = new el_float(field->ie.length, en, field->ie.id & 0x7FFF);
				_tablex->addColumn(new_element->name(), new_element->type());
				break;
			case BLOB:
			case TEXT:
			case UNKNOWN:
			default:
				//store unknown types as uint if possible
				//std::cout << "UNKNOWN! size:" << field->ie.length << std::endl;
				if(field->ie.length < 9){
					new_element = new el_uint(field->ie.length, en, field->ie.id & 0x7FFF);
					_tablex->addColumn(new_element->name(), new_element->type());
				} else if(field->ie.length == 65535){ //variable size element
					//std::cout << "UNKNOWN! - variable size (skip:" << field->ie.length << std::endl;
					new_element = new el_var_size(field->ie.length, en, field->ie.id & 0x7FFF);
					//_tablex->addColumn(new_element->name(), new_element->type());
				} else { //TODO blob ect
					//std::cout << "UNKNOWN! - blop:" << field->ie.length << std::endl;
					new_element = new element(field->ie.length, en, field->ie.id & 0x7FFF);
				}

		}
		if(!new_element){
			std::cerr << "Something is wrong with template elements!" << std::endl;
			return 1;
		}
		elements.push_back(new_element);
	}
	_tablex->reserveSpace(RESERVED_SPACE);
	return 0;
}

