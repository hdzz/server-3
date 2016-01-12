#ifndef _CMIVECTOR_H_ 
#define _CMIVECTOR_H_ 

#include"CmiMemory.h"


template<typename T>
class CmiVector
{
	class Node
	{
	public:
		T data;
		Node* up;
		Node* next;
	};

public:
	CmiVector()
		:first(0)
		, last(0)
		, curt(0)
		, sz(0)
	{}

	CmiVector(int32_t dims)
		:first(0)
		, last(0)
		, curt(0)
		, sz(0)
	{
		for (int32_t i = 0; i < dims; i++)
			push_back(0);
		clear();
	}

	~CmiVector()
	{
		Node* tmp;
		for (Node* nd = first; nd;)
		{
			tmp = nd->next;
			delete nd;
			nd = tmp;
		}
	}

	////////
	bool isempty(){ return (sz ? false : true); }
	int32_t size(){ return sz; }

	T* begin(){ return sz ? (first ? &(first->data) : 0) : 0;}
	T* end(){ return sz ? (last ? &(last->data) : 0) : 0; }
	T* cur(){ return curt ? &(curt->data) : 0; }

	T* next()
	{
		curt = curt ? curt->next : first;
		return curt ? &(curt->data) : 0;
	}

	T* up()
	{
		curt = curt ? curt->up : last;
		return curt ? &(curt->data) : 0;
	}

	T* node(void* nd)
	{
		Node* d = (Node*)nd;
		return d ? &(d->data) : 0;
	}

	void reset()
	{
		curt = 0;
	}

	void clear()
	{
		curt = 0;
		last = 0; 
		sz = 0;
	}

	void* get_cur_node()
	{
		return (void*)curt;
	}

	void* get_first_node()
	{
		return (void*)first;
	}

	void* get_last_node()
	{
		return (void*)last;
	}

	void* get_next_node(void* node)
	{
		return (void*)(((Node*)node)->next);
	}
	

	void* get_up_node(void* node)
	{
		return (void*)(((Node*)node)->up);
	}

	void goto_node(void* node)
	{
		curt = (Node*)node;
	}

	T* push_back(const T* data)
	{
		return push_back(*data);
	}

	T* push_back(const T& data)
	{
		if (last && last->next)
		{
			last = last->next;
			last->data = data;
			sz++;
			return &(last->data);
		}
		else if (first && !last)
		{
			first->data = data;
			last = first;
			sz++;		
			return  &(last->data);
		}

		Node* node = new Node;
		node->data = data;
		node->up = node->next = 0;
		sz++;

		if (last)
		{
			node->up = last;
			last->next = node;
			last = node;
		}
		else
		{
			first = last = node;
		}

		return &(last->data);
	}

	T* push_front(const T* data)
	{
		return push_front(*data);
	}

	T* push_front(const T& data)
	{	
		Node* node = new Node;
		node->data = data;
		node->up = node->next = 0;
		sz++;

		if (first)
		{
			node->next = first;
			first->up = node;
			first = node;
		}
		else
		{
			first = last = node;
		}

		return &(first->data);
	}

	

	void delete_node(void* node, int32_t direction = 0)
	{
		if (!node)
			return;

		Node* nd = (Node*)node;
		Node* tmp_up = nd->up;
		Node* tmp_next = nd->next;

		if (tmp_up)
			tmp_up->next = tmp_next;
		else
			first = tmp_next;

		if (tmp_next)
			tmp_next->up = tmp_up;
		else
			last = tmp_up;

		sz--;

		if (nd == curt){
			curt = (direction == 0 ? tmp_up : tmp_next);
		}

		delete nd;
	}

	void del(int32_t direction = 0)
	{
		delete_node(curt, direction);
	}

	void cancleup()
	{
		if (last == 0)
			return;
		else if (last == first)
			last = 0;
		else 
			last = last->up;
		sz--;
		curt = 0;
	}


	T* insert_before(const T& data, void* refnode)
	{
		Node* ref = (Node*)refnode;

		if (ref == 0)
			return push_back(data);

		Node* node = new Node;
		node->data = data;
		node->up = node->next = 0;
		sz++;

		Node* upnode = ref->up;

		if (upnode){ upnode->next = node; }
		else{ first = node; }
	
		node->up = upnode;
		node->next = ref;

		if (refnode){ ref->up = node; }
		else{ last = node; }

		return &(node->data);
	}


	static void delete_vector(CmiVector<T>* vec);
	
	static CmiVector<T>* cat_vector(
		CmiVector<T> *org,
		CmiVector<T> *cat,
		bool is_delete_cat = true);

private:
	int32_t sz;
	Node* first;
	Node* last;
	Node* curt;
};


/////////////////////////////////////////////////////////////////////////
template<typename T>
void CmiVector<T>::delete_vector(CmiVector<T>* vec)
{
	if (vec == 0)
		return;

	T* value;
	vec->reset();

	while (value = vec->next()){
		delete *value;
	}

	delete vec;
}

/////////////////////////////////////////////////////////////////////////////
template<typename T>
CmiVector<T>* CmiVector<T>::cat_vector(
	CmiVector<T> *org, 
	CmiVector<T> *cat, 
	bool is_delete_cat)
{
	if (org){
		if (cat){
			T* m;
			cat->reset();
			while (m = cat->next())
				org->push_back(*m);

			if (is_delete_cat)
				delete cat;
		}
		return org;
	}
	return cat;
}


#endif