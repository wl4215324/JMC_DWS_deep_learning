/*
 * xml_operation.h
 *
 *  Created on: Jan 5, 2018
 *      Author: tony
 */

#ifndef XML_OPERATION_H_
#define XML_OPERATION_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>


#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/encoding.h>


#if 0
#include "libxml/parser.h"
#include "libxml/tree.h"
#include "libxml/encoding.h"
#endif

#include "serial_pack_parse.h"

#define MY_ENCODING "ISO-8859-1"

#define PARAM_CONFIG_XML_PATH  "/home/user/param_config.xml"

extern int init_xml_file(char* path_name, KeyValuePair* key_value_array);

extern int create_xml_file(int argc, char* argv);

extern int parse_xml_file(const char* url);

extern int update_node_value(const char* file_name, const char* node_name, const char* value);

extern int add_node_into_xml(const char* xml_path, const char* node_name, const char* node_value);

extern int del_node_from_xml(const char* xml_path, const char* node_name);

#endif /* XML_OPERATION_H_ */
