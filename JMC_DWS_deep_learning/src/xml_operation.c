/*
 * xml_operation.c
 *
 *  Created on: Jan 5, 2018
 *      Author: tony
 */
#include "xml_operation.h"

void create_new_xml_file(const KeyValuePair* key_value_array)
{
    xmlDocPtr doc = NULL; /* document pointer */
    xmlNodePtr root_node = NULL, node = NULL, node1 = NULL; /* node pointers */
    unsigned short i = 0;
    char key_value[32] = "";

    // Creates a new document, a node and set it as a root node
    doc = xmlNewDoc(BAD_CAST "1.0");
    root_node = xmlNewNode(NULL, BAD_CAST "root");
    xmlDocSetRootElement(doc, root_node);

    for(i=0; i<CONFIG_PARAMS_COUNT; i++)
    {
    	sprintf(key_value, "%d", (key_value_array+i)->value);
        //creates a new node, which is "attached" as child node of root_node node.
        xmlNewChild(root_node, NULL, BAD_CAST (key_value_array+i)->key_name, BAD_CAST key_value);
    }

    //Dumping document to stdio or file
    xmlSaveFormatFileEnc(PARAM_CONFIG_XML_PATH, doc, "UTF-8", 1);

    /*free the document */
    xmlFreeDoc(doc);
    xmlCleanupParser();
    xmlMemoryDump();//debug memory for regression tests
}


int scan_xml_file(const char* path_name, KeyValuePair* key_value_array)
{
	xmlDocPtr doc;   //定义解析文档指针
	xmlNodePtr cur, root_node;  //定义结点指针(你需要它为了在各个结点间移动)
	xmlChar *key;
	unsigned short i = 0, counter = 0;
	char key_value[32] = "";

	if(!key_value_array)
	{
		return -1;  //if pointer is NULL
	}

	doc = xmlReadFile(path_name, MY_ENCODING, 256); //解析文件

	if (doc == NULL )
	{
	   fprintf(stderr,"Document not parsed successfully. /n");
	   return -1;
	}

	cur = xmlDocGetRootElement(doc); //确定文档根元素

	/*检查确认当前文档中包含内容*/
	if (cur == NULL)
	{
	   fprintf(stderr, "empty document/n");
	   xmlFreeDoc(doc);
	   return -1;  // if xml file is empty, return -1
	}

	/*if can't get root node in xml file, then return -1*/
	if (xmlStrcmp(cur->name, (const xmlChar *) "root"))
	{
		fprintf(stderr,"document of the wrong type, root node != root");
	    xmlFreeDoc(doc);
	    return -1;
	}

	root_node = cur;  //get root node pointer

	/*check all of nodes in xml are intact for configuration parameters*/
	for(i=0, counter=0; i<CONFIG_PARAMS_COUNT; i++)
	{
		cur = root_node->xmlChildrenNode;

		while(cur != NULL)
		{
			if ((!xmlStrcmp(cur->name, (const xmlChar *)(key_value_array+i)->key_name)))
			{
				counter++;
			}

			cur = cur->next;
		}
	}

	/*if all of nodes in xml are intact, then update nodes' value from xml into memory variables*/
	if(counter >= CONFIG_PARAMS_COUNT)
	{
		for(i=0; i<CONFIG_PARAMS_COUNT; i++)
		{
			cur = root_node->xmlChildrenNode;

			while(cur != NULL)
			{
				/*if children node is existent, update new value into xml file*/
				if ((!xmlStrcmp(cur->name, (const xmlChar *)(key_value_array+i)->key_name)))
				{
					key = xmlNodeGetContent(cur);
					(key_value_array+i)->value = atoi((char*)key);
				}

				cur = cur->next;
			}
		}
	}
	else  //if all of nodes in xml are not intact, then update nodes' default value into xml file
	{
		for(i=0; i<CONFIG_PARAMS_COUNT; i++)
		{
			cur = root_node->xmlChildrenNode;

			while(cur != NULL)
			{
				/*if children node is existent, then break*/
				if ((!xmlStrcmp(cur->name, (const xmlChar *)(key_value_array+i)->key_name)))
				{
					break;
				}

				cur = cur->next;
			}

			/*if children node is not existent, add new node and its value*/
			if(!cur)
			{
				sprintf(key_value, "%d", (key_value_array+i)->value);
				xmlNewChild(root_node, NULL, BAD_CAST (key_value_array+i)->key_name, BAD_CAST key_value);
				xmlSaveFormatFileEnc(path_name,  doc, "UTF-8", 1);
			}
		}
	}

	xmlFreeDoc(doc);
	return 0;
}



int init_xml_file(char* path_name, KeyValuePair* key_value_array)
{
	struct stat buf;

	/*if file or directory is error or not existent*/
	if(lstat(path_name, &buf) < 0)
	{
		create_new_xml_file(key_value_array);  // create a new xml file
		*path_name = PARAM_CONFIG_XML_PATH;
		return 1;
	}
	else  // if get file or directory information successfully
	{
		if(S_ISREG(buf.st_mode))  // if path is file
		{
			if(scan_xml_file(path_name, key_value_array) < 0)
			{
				create_new_xml_file(key_value_array);
				*path_name = PARAM_CONFIG_XML_PATH;
				return 1;
			}
			else
			{
				return 0;
			}
		}
		else  //if path is a directory
		{
			create_new_xml_file(key_value_array);
			*path_name = PARAM_CONFIG_XML_PATH;
			return 1;
		}
	}
}




int create_xml_file(int argc, char* argv)
{
    xmlDocPtr doc = NULL; /* document pointer */
    xmlNodePtr root_node = NULL, node = NULL, node1 = NULL; /* node pointers */

    // Creates a new document, a node and set it as a root node
    doc = xmlNewDoc(BAD_CAST "1.0");
    root_node = xmlNewNode(NULL, BAD_CAST "root");
    xmlDocSetRootElement(doc, root_node);

    //creates a new node, which is "attached" as child node of root_node node.
    xmlNewChild(root_node, NULL, BAD_CAST "node1",BAD_CAST "content of node1");

    // xmlNewProp() creates attributes, which is "attached" to an node.
    node=xmlNewChild(root_node, NULL, BAD_CAST "node3", BAD_CAST"node has attributes");
    xmlNewProp(node, BAD_CAST "attribute", BAD_CAST "yes");

    //Here goes another way to create nodes.
    node = xmlNewNode(NULL, BAD_CAST "node4");
    node1 = xmlNewText(BAD_CAST"other way to create content");
    xmlAddChild(node, node1);
    xmlAddChild(root_node, node);

    //Dumping document to stdio or file
    xmlSaveFormatFileEnc(argc > 1 ? argv : "-", doc, "UTF-8", 1);

    /*free the document */
    xmlFreeDoc(doc);
    xmlCleanupParser();
    xmlMemoryDump();//debug memory for regression tests

    return 0;
}


int parse_xml_file(const char* url)
{
	xmlDocPtr doc;   //定义解析文档指针
	xmlNodePtr cur;  //定义结点指针(你需要它为了在各个结点间移动)
	xmlChar *key;

	doc = xmlReadFile(url, MY_ENCODING, 256); //解析文件

	if (doc == NULL )
	{
	   fprintf(stderr,"Document not parsed successfully. /n");
	   return -1;
	}

	cur = xmlDocGetRootElement(doc); //确定文档根元素

	/*检查确认当前文档中包含内容*/
	if (cur == NULL)
	{
	   fprintf(stderr, "empty document/n");
	   xmlFreeDoc(doc);
	   return -1;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *) "root"))
	{
		fprintf(stderr,"document of the wrong type, root node != root");
	    xmlFreeDoc(doc);
	    return -1;
	}

	cur = cur->xmlChildrenNode;

	while(cur != NULL)
	{
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"node1")))
		{
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			printf("keyword: %s\n", key);
			xmlFree(key);
		}

		cur = cur->next;
	}

	xmlFreeDoc(doc);
	return 0;
}

/*
 * query value according to specified node
 * parameter:
 *          const char* file_name: xml file name
 *          const char* node_name: specified node's name
 *          char* value: return specified node's value
 *
 * function return: int
 *                -1: read or parse xml file error
 *                0: can't find specified node in xml file
 *                1: set specified node's value successfully
 */
int query_node_value(const char* file_name, const char* node_name, char* value)
{
	xmlDocPtr doc;   //定义解析文档指针
	xmlNodePtr cur;  //定义结点指针(你需要它为了在各个结点间移动)
	xmlChar *key;

	doc = xmlReadFile(file_name, MY_ENCODING, 256); //解析文件

	if (doc == NULL )
	{
	   fprintf(stderr,"Document not parsed successfully. \n");
	   return -1;
	}

	cur = xmlDocGetRootElement(doc); //确定文档根元素

	/*检查确认当前文档中包含内容*/
	if (cur == NULL)
	{
	   fprintf(stderr, "empty document \n");
	   goto exit_error;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *) "root"))
	{
		fprintf(stderr, "document of the wrong type, root node != root \n");
		goto exit_error;
	}

	cur = cur->xmlChildrenNode;

	while(cur != NULL)
	{
		if ((!xmlStrcmp(cur->name, (const xmlChar *)node_name)))
		{
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			printf("keyword: %s \n", key);
			strcpy(value, (char*)key);
			xmlFree(key);
			goto exit_success;
		}

		cur = cur->next;
	}

exit_no_node:
	xmlFreeDoc(doc);
	return 0;

exit_success:
	xmlFreeDoc(doc);
	return 1;

exit_error:
	xmlFreeDoc(doc);
	return -1;
}


/*
 * update specified node's value
 * parameters:
 *           const char* file_name: xml file name
 *           const char* node_name: specified node's name
 *           const char* value: setting value
 *
 * function return: int
 *                -1: read or parse xml file error
 *                0: can't find specified node in xml file
 *                1: set specified node's value successfully
 */
int update_node_value(const char* file_name, const char* node_name, const char* value)
{
	xmlDocPtr doc;   //定义解析文档指针
	xmlNodePtr cur;  //定义结点指针(你需要它为了在各个结点间移动)

	doc = xmlReadFile(file_name, MY_ENCODING, 256); //解析文件

	if (doc == NULL )
	{
	   fprintf(stderr,"Document not parsed successfully.\n");
	   return -1;
	}

	cur = xmlDocGetRootElement(doc); //确定文档根元素

	/*检查确认当前文档中包含内容*/
	if (cur == NULL)
	{
	   fprintf(stderr, "empty document\n");
	   xmlFreeDoc(doc);
	   return -1;
	}

	if (xmlStrcmp(cur->name, (const xmlChar *) "root"))
	{
		fprintf(stderr,"document of the wrong type, root node != root\n");
		goto exit_error;
	}

	cur = cur->xmlChildrenNode;

	while(cur != NULL)
	{
		if ((!xmlStrcmp(cur->name, (const xmlChar *)node_name)))
		{
			xmlNodeSetContent(cur, (const xmlChar *)value);
			xmlSaveFormatFileEnc(file_name, doc, "UTF-8", 1);
			goto exit_success;
		}

		cur = cur->next;
	}

exit_no_node:
    xmlFreeDoc(doc);
    return 0;

exit_success:
    xmlFreeDoc(doc);
    return 1;

exit_error:
	xmlFreeDoc(doc);
	return -1;
}



/*
 * add a new node into xml file
 * parameters:
 *           const char* xml_path: xml file path
 *           const char* node_name: name of node to be added
 *           const char* node_value: value of node to be added
 *
 * function return: int
 *                -1: parse xml error
 *                1: add node successfully
 */
int add_node_into_xml(const char* xml_path, const char* node_name, const char* node_value)
{
	xmlDocPtr doc;   //定义解析文档指针
	xmlNodePtr cur = NULL;  //定义结点指针(你需要它为了在各个结点间移动)
	xmlNodePtr root_node = NULL;

	/*if node name or value is NULL, return error*/
	if((NULL == node_name) || (NULL == node_value))
	{
		return -1;
	}

	doc = xmlReadFile(xml_path, MY_ENCODING, 256); //parse xml file

	if (doc == NULL )
	{
		fprintf(stderr, "Document not parsed successfully. /n");
		return -1;
	}

	cur = xmlDocGetRootElement(doc); //确定文档根元素

	/*检查确认当前文档中包含内容*/
	if (cur == NULL)
	{
		fprintf(stderr, "empty document/n");
		xmlFreeDoc(doc);
		return -1;  // if xml file is empty, return -1
	}

	/*check whether root node's is root, if not return -1*/
	if (xmlStrcmp(cur->name, (const xmlChar *) "root"))
	{
		fprintf(stderr, "document of the wrong type, root node != root");
	    xmlFreeDoc(doc);
	    return -1;
	}

	root_node = cur;
	cur = cur->xmlChildrenNode;

	while(cur != NULL)
	{   /* if there is a specified node in xml, then update given value into xml */
		if ((!xmlStrcmp(cur->name, (const xmlChar *)node_name)))
		{
			xmlNodeSetContent(cur, (const xmlChar *)node_value);
			xmlSaveFormatFileEnc(xml_path, doc, "UTF-8", 1);
			goto exit_success;
		}

		cur = cur->next;
	}

	/* if there is no specified node in xml file, add the node into xml file */
	xmlNewChild(root_node, NULL, BAD_CAST (node_name), BAD_CAST (node_value));
	xmlSaveFormatFileEnc(xml_path, doc, "UTF-8", 1);

exit_success:
    xmlFreeDoc(doc);
	return 1;
}


/*
 * delete a specified node from xml file
 * parameters:
 *           const char* xml_path: xml file path
 *           const char* node_name: name of node to be deleted
 *
 * function return: int
 *                -1: parse xml error
 *                1: delete node successfully
 */
int del_node_from_xml(const char* xml_path, const char* node_name)
{
	xmlDocPtr doc;   //定义解析文档指针
	xmlNodePtr cur = NULL;  //定义结点指针(你需要它为了在各个结点间移动)
	xmlNodePtr tempNode = NULL;

	/*if node name is NULL, return error*/
	if(!node_name)
	{
		return -1;
	}

	doc = xmlReadFile(xml_path, MY_ENCODING, 256); //解析文件

	if (doc == NULL )
	{
	   fprintf(stderr,"Document not parsed successfully. /n");
	   return -1;
	}

	cur = xmlDocGetRootElement(doc); //确定文档根元素

	/*检查确认当前文档中包含内容*/
	if (cur == NULL)
	{
	   fprintf(stderr, "empty document/n");
	   xmlFreeDoc(doc);
	   return -1;  // if xml file is empty, return -1
	}

	/*check whether root node's is root, if not return -1*/
	if (xmlStrcmp(cur->name, (const xmlChar *) "root"))
	{
		fprintf(stderr, "document of the wrong type, root node != root");
	    xmlFreeDoc(doc);
	    return -1;
	}

	cur = cur->xmlChildrenNode;

	while(cur != NULL)
	{   /* if there is a specified node in xml, then delete it */
		if ((!xmlStrcmp(cur->name, (const xmlChar *)node_name)))
		{
			tempNode = cur->next;
	        xmlUnlinkNode(cur);
	        xmlFreeNode(cur);
	        cur = tempNode;
	        //continue;
			xmlSaveFormatFileEnc(xml_path, doc, "UTF-8", 1);
			goto exit_success;
		}

		cur = cur->next;
	}

exit_success:
    xmlFreeDoc(doc);
	return 1;
}
