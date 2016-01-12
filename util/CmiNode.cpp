#include"CmiNode.h"

///////////////////////////////////////////////////////////////////////////////////
int32_t CmiNode::add_child(CmiNode* newNode)
{
	if (newNode == 0)
		return 0;

	childNodeCount++;

	if (firstChild)
	{
		lastChild->next = newNode;
		newNode->prev = lastChild;
		lastChild = newNode;
	}
	else
	{
		firstChild = lastChild = newNode;
		newNode->prev = 0;
	}

	newNode->next = 0;
	newNode->parentNode = this;
	newNode->level = level + 1;
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////
int32_t CmiNode::add_level_child(CmiNode* newNode)
{
	if (newNode == 0)
		return 0;

	CmiNode* tmpfirst = firstChild;
	CmiNode* tmplast = lastChild;
	CmiNode* tmp = tmpfirst;

	firstChild = lastChild = newNode;
	newNode->parentNode = this;
	newNode->level = level + 1;
	newNode->childNodeCount = 0;

	childNodeCount = 1;

	while (tmp != tmplast)
	{
		newNode->add_child(tmp);
		tmp = tmp->next;
	}

	newNode->add_child(tmp);

	return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////
int32_t CmiNode::insert_before(CmiNode* newNode, CmiNode* refNode)
{
	if (refNode == 0)
		return (!add_child(newNode) ? 0 : 1);

	CmiNode* prev = refNode->prev;

	if (prev)
		prev->next = newNode;
	else
		firstChild = newNode;

	childNodeCount++;

	newNode->prev = prev;
	newNode->next = refNode;
	refNode->prev = newNode;
	newNode->parentNode = this;
	newNode->level = level + 1;

	return 1;
}


///////////////////////////////////////////////////////////////////////////////////
int32_t CmiNode::split_child(CmiNode* cNode)
{
	if (cNode == 0)
		return 0;

	childNodeCount--;

	CmiNode* cprev = cNode->prev;
	CmiNode* cnext = cNode->next;

	if (cprev){ cprev->next = cnext; }
	else{ firstChild = cnext; }

	if (cnext){ cnext->prev = cprev; }
	else{ lastChild = cprev; }

	cNode->parentNode = 0;
	cNode->prev = 0;
	cNode->next = 0;

	return 1;
}

////////////////////////////////////////////////////////////////////////////////////
void CmiNode::clone_childs(CmiNode* parentNode)
{
	for (CmiNode* tmp = parentNode->firstChild; tmp; tmp = tmp->next)
	{
		add_child(tmp->clone(true));
	}
}

////////////////////////////////////////////////////////////////////////////////////	
void CmiNode::create_child_node_list()
{
	if (childNodeCount > 0)
	{
		if (childNodes)
			operator delete(childNodes);

		CmiNode* node = firstChild;
		childNodes = (CmiNode**)operator new(sizeof(CmiNode*)* childNodeCount);

		for (int32_t i = 0; node; node = node->next)
		{
			childNodes[i++] = node;
			node->set_index(i);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////
void CmiNode::delete_child(CmiNode* cNode)
{
	split_child(cNode);
	CmiNode::deleteNode(cNode);
}

////////////////////////////////////////////////////////////////////////////////////	
void CmiNode::compute_child_level()
{
	int32_t lv = level + 1;

	for (CmiNode* node = firstChild; node; node = node->next)
	{
		node->set_level(lv);
		node->compute_child_level();
	}
}

////////////////////////////////////////////////////////////////////////////////////
CmiNode& CmiNode::operator = (CmiNode& obj)
{
	if (this == &obj)
		return *this;

	nodeType = obj.get_nodeType();
	return *this;
}


////////////////////////////////////////////////////////////////////////////////////
void CmiNode::deleteNode(CmiNode* node)
{
	if (node == 0)
		return;

	CmiNode* tmp;

	for (CmiNode* cnode = node->get_firstChild(); cnode;)
	{
		tmp = cnode->get_nextNode();
		CmiNode::deleteNode(cnode);
		cnode = tmp;
	}

	delete node;
}

////////////////////////////////////////////////////////////////////////////////////
void CmiNode::search_sibling_parent(CmiNode* &node1, CmiNode* &node2)
{
	int32_t lv1 = node1->get_level();
	int32_t lv2 = node2->get_level();

	if (lv1 > lv2)
	{
		lv1 -= lv2;
		while (lv1--) { node1 = node1->get_parentNode(); }
		lv1 = lv2;
	}
	else if (lv2 > lv1)
	{
		lv2 -= lv1;
		while (lv2--) { node2 = node2->get_parentNode(); }
		lv2 = lv1;
	}

	if (lv1 == lv2)
	{
		while (node1->get_parentNode() != node2->get_parentNode())
		{
			node1 = node1->get_parentNode();
			node2 = node2->get_parentNode();
		}
	}
}
