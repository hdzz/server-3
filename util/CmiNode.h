#ifndef _CMINODE_H_ 
#define _CMINODE_H_ 

#include"utils.h"

//////////////////////////////////////////////////////////////////
#define DEFINE_CLONE_FUNC(CLASS,FUNC) \
\
virtual CLASS* FUNC(bool is_clone_children = false)\
{\
	CLASS* clone_parent = new CLASS(*this); \
\
	if (is_clone_children)\
		clone_parent->clone_childs(this);\
\
	return clone_parent;\
}


//////////////////////////////////////////////////////////////////

class CmiNode
{
public:
	CmiNode()
		: nodeType(0)
		, childNodes(0)
		, childNodeCount(0)
		, level(0)
		, parentNode(0)
		, prev(0)
		, next(0)
		, firstChild(0)
		, lastChild(0)
	{}

	virtual ~CmiNode()
	{
		if (childNodes)
			operator delete(childNodes);
	}

	DEFINE_CLONE_FUNC(CmiNode, clone);
	CmiNode& operator = (CmiNode& obj);

	virtual int32_t add_child(CmiNode* newNode);
	virtual int32_t add_level_child(CmiNode* newNode);
	virtual int32_t insert_before(CmiNode* newNode, CmiNode* refNode);
	virtual int32_t split_child(CmiNode* cNode);
	virtual void create_child_node_list();

	void delete_child(CmiNode* cNode);
	void compute_child_level();
	void clone_childs(CmiNode* parentNode);



	void set_level(int32_t lv){ level = lv; }
	void set_index(int32_t _index){ index = _index; }
	void set_nodeType(int32_t _nodeType){ nodeType = _nodeType; }
	void set_childNodes(CmiNode** _childNodes){ childNodes = _childNodes; }
	void set_parentNode(CmiNode *newNode){ parentNode = newNode; }
	void set_prevNode(CmiNode *newNode){ prev = newNode; }
	void set_nextNode(CmiNode *newNode){ next = newNode; }
	void set_firstChild(CmiNode *newNode){ firstChild = newNode; }
	void set_lastChild(CmiNode *newNode){ lastChild = newNode; }

	int32_t get_level(){ return level; }
	int32_t get_index(){ return index; }
	int32_t get_nodeType(){ return nodeType; }

	CmiNode* get_parentNode(){ return parentNode; }
	CmiNode* get_prevNode(){ return prev; }
	CmiNode* get_nextNode(){ return next; }
	CmiNode* get_firstChild(){ return firstChild; }
	CmiNode* get_lastChild(){ return lastChild; }
	CmiNode** get_childNodes(){ return childNodes; }

	int32_t get_childNodeCount(){ return childNodeCount; }


public:
	static void deleteNode(CmiNode* node);
	static void search_sibling_parent(CmiNode*& node1, CmiNode*& node2);

protected:
	int32_t level;
	int32_t index;
	int32_t nodeType;

	int32_t childNodeCount;
	CmiNode **childNodes;

	CmiNode *prev, *next;
	CmiNode *parentNode;
	CmiNode *firstChild, *lastChild;

};
#endif