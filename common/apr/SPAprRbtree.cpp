// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016 Roman Katuntsev <sbkarr@stappler.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
**/

#include "SPCore.h"
#include "SPAprRbtree.h"
#include "SPAprStringStream.h"

#if SPAPR
NS_APR_BEGIN

namespace rbtree {

NodeBase * NodeBase::increment(NodeBase *c) {
	if (c->right) {
		c = c->right;
		while (c->left) {
			c = c->left;
		}
	} else {
		if (c->parent) {
			if (c->parent->left == c) {
				c = c->parent;
			} else {
				while (c->parent && c->parent->right == c) {
					c = c->parent;
				}
				if (!c->parent) {
					return nullptr;
				} else {
					c = c->parent;
				}
			}
		} else {
			return nullptr;
		}
	}
	return c;
}

const NodeBase * NodeBase::increment(const NodeBase *c) {
	if (c->right) {
		 // move right one step, then left until end
		c = c->right;
		while (c->left) {
			c = c->left;
		}
	} else {
		if (c->parent) {
			if (c->parent->left == c) {
				c = c->parent;
			} else {
				while (c->parent && c->parent->right == c) {
					c = c->parent;
				}
				if (!c->parent) {
					return nullptr; // end of iteration
				} else {
					c = c->parent;
				}
			}
		} else {
			return nullptr;
		}
	}
	return c;
}

NodeBase * NodeBase::decrement(NodeBase *c) {
	if (c->left) {
		 // move left one step, then right until end
		c = c->left;
		while (c->right) {
			c = c->right;
		}
	} else {
		if (c->parent) {
			if (c->parent->right == c) {
				c = c->parent;
			} else {
				while (c->parent && c->parent->left == c) {
					c = c->parent;
				}
				if (!c->parent) {
					return nullptr; // end of iteration
				} else {
					c = c->parent;
				}
			}
		} else {
			return nullptr;
		}
	}
	return c;
}

const NodeBase * NodeBase::decrement(const NodeBase *c) {
	if (c->left) {
		 // move left one step, then right until end
		c = c->left;
		while (c->right) {
			c = c->right;
		}
	} else {
		if (c->parent && c->parent->color) {
			if (c->parent->right == c) {
				c = c->parent;
			} else {
				while (c->parent && c->parent->left == c) {
					c = c->parent;
				}
				if (!c->parent) {
					return nullptr; // end of iteration
				} else {
					c = c->parent;
				}
			}
		} else {
			return nullptr;
		}
	}
	return c;
}

NodeBase *NodeBase::replace(NodeBase *old, NodeBase *n) {
	n->left = old->left;
	n->right = old->right;
	n->color = old->color;
	n->parent = old->parent;

	// link new with parent
	if (old->parent) {
    	if (old->parent->left == old) {
    		old->parent->left = n;
    	} else {
    		old->parent->right = n;
    	}
	}

	// link new with childs
	if (old->left && old->left != n) {
		old->left->parent = n;
	} else if (old->left == n) {
		n->left = nullptr;
	}
	if (old->right && old->right != n) {
		old->right->parent = n;
	} else if (old->right == n) {
		n->right = nullptr;
	}

	return old;
}

void RotateLeft(NodeBase * head, NodeBase * n, NodeBase * p) {
	NodeBase * tmp = n->left;
	if (p == head->left) {
		head->left = n;
	} else {
		if (p->parent->right == p) {
			p->parent->right = n;
		} else {
			p->parent->left = n;
		}
	}
	n->parent = p->parent;
	p->parent = n;
	n->left = p;

	if (tmp) tmp->parent = p;
	p->right = tmp;
}

void RotateRight(NodeBase * head, NodeBase * n, NodeBase * p) {
	NodeBase * tmp = n->right;
	if (p == head->left) {
		head->left = n;
	} else {
		if (p->parent->right == p) {
			p->parent->right = n;
		} else {
			p->parent->left = n;
		}
	}
	n->parent = p->parent;
	p->parent = n;
	n->right = p;

	if (tmp) tmp->parent = p;
	p->left = tmp;
}

void NodeBase::insert(NodeBase *head, NodeBase * n) {
    /* check Red-Black properties */
    while (n != head->left && n->parent->color == NodeColor::Red) {
    	auto p = n->parent;
    	auto g = n->parent->parent;
        if (p == g->left) {
        	NodeBase * u = g->right;
            if (u && u->color == NodeColor::Red) {
                p->color = NodeColor::Black;
                u->color = NodeColor::Black;
                g->color = NodeColor::Red;
                n = g;
            } else {
                if (n == p->right) {
                	RotateLeft(head, n, p);
                    n = n->left;
                    p = n->parent;
                }
                p->color = NodeColor::Black;
                g->color = NodeColor::Red;
                RotateRight(head, p, g);
            }
        } else {
        	NodeBase * u = g->left;
            if (u && u->color == NodeColor::Red) {
                p->color = NodeColor::Black;
                u->color = NodeColor::Black;
                g->color = NodeColor::Red;
                n = g;
            } else {
                if (n == n->parent->left) {
                	RotateRight(head, n, p);
            		n = n->right;
                    p = n->parent;
                }
                p->color = NodeColor::Black;
                g->color = NodeColor::Red;
                RotateLeft(head, p, g);
            }
        }
    }
    head->left->color = NodeColor::Black; // root
}

void NodeBase::remove(NodeBase *head, NodeBase * n) {
	while (n != head->left && n->color == NodeColor::Black) {
		if (n == n->parent->left) {
			NodeBase *s = n->parent->right;
			if (s && s->color == NodeColor::Red) {
				n->parent->color = NodeColor::Red;
				s->color = NodeColor::Black;
				RotateLeft(head, s, n->parent);
				s = n->parent->right;
			}
			if (s) {
				if (s->color == NodeColor::Black
						&& (!s->left || s->left->color == NodeColor::Black)
						&& (!s->right || s->right->color == NodeColor::Black)) {
					s->color = NodeColor::Red;
					if (s->parent->color == NodeColor::Red) {
						s->parent->color = NodeColor::Black;
						break;
					} else {
						n = n->parent;
					}
				} else {
					if ((!s->right || s->right->color == NodeColor::Black) && (s->left && s->left->color == NodeColor::Red)) {
						s->color = NodeColor::Red;
						s->left->color = NodeColor::Black;
						RotateRight(head, s->left, s);
						s = n->parent->right;
					}
					s->color = n->parent->color;
					n->parent->color = NodeColor::Black;
					if (s->right) s->right->color = NodeColor::Black;
					RotateLeft(head, s, n->parent);
					break;
				}
			} else {
				break;
			}
		} else {
			NodeBase *s = n->parent->left;
			if (s && s->color == NodeColor::Red) {
				n->parent->color = NodeColor::Red;
				s->color = NodeColor::Black;
				RotateRight(head, s, n->parent);
				s = n->parent->left;
			}
			if (s) {
				if (s->color == NodeColor::Black
						&& (!s->left || s->left->color == NodeColor::Black)
						&& (!s->right || s->right->color == NodeColor::Black)) {
					s->color = NodeColor::Red;
					if (s->parent->color == NodeColor::Red) {
						s->parent->color = NodeColor::Black;
						break;
					} else {
						n = n->parent;
					}
				} else {
					if ((!s->left || s->left->color == NodeColor::Black) && (s->right && s->right->color == NodeColor::Red)) {
						s->color = NodeColor::Red;
						s->right->color = NodeColor::Black;
						RotateLeft(head, s->right, s);
						s = n->parent->left;
					}
					s->color = n->parent->color;
					n->parent->color = NodeColor::Black;
					if (s->left) s->left->color = NodeColor::Black;
					RotateRight(head, s, n->parent);
					break;
				}
			} else {
				break;
			}
		}
	}
	n->color = NodeColor::Black;
}

#if APR_RBTREE_DEBUG

bool TreeDebug::make_test(std::ostream &ostream, int size) {
	apr::rbtree::Tree<int, int> tree_test;
	bool valid = true;

	apr::ostringstream stream;
	stream << "{ ";
	for (int i = 0; i < size; i ++) {
		auto val = rand() % 100000;
		tree_test.emplace(val);
		stream << val << ", ";

		auto v = validate(tree_test);
		switch (v) {
		case Validation::Valid:
			break;
		case Validation::RootIsNotBlack:
			stream << "\nInsert: Invalid Tree: RootIsNotBlack\n";
			visit(tree_test, stream);
			valid = false;
			break;
		case Validation::RedChildIntoRedNode:
			stream << "\nInsert: Invalid Tree: RedChildIntoRedNode\n";
			visit(tree_test, stream);
			valid = false;
			break;
		case Validation::DifferentBlackNodeCount:
			stream << "\nInsert: Invalid Tree: DifferentBlackNodeCount\n";
			visit(tree_test, stream);
			valid = false;
			break;
		}
	}
	stream << "}\nSize: " << tree_test.size() << "\n";

	apr::array<int> arr;
	stream << "Sorted: ";
	for (auto it = tree_test.begin(); it != tree_test.end(); it ++) {
		stream << *it << " ";
		arr.emplace_back(*it);
	}
	stream << "\n";

	if (valid) {
		ostream << stream.str();
		stream.clear();
	}

	auto val = validate(tree_test);
	switch (val) {
	case Validation::Valid:
		stream << "Valid Tree\n";
		break;
	case Validation::RootIsNotBlack:
		stream << "Invalid Tree: RootIsNotBlack\n";
		valid = false;
		break;
	case Validation::RedChildIntoRedNode:
		stream << "Invalid Tree: RedChildIntoRedNode\n";
		valid = false;
		break;
	case Validation::DifferentBlackNodeCount:
		stream << "Invalid Tree: DifferentBlackNodeCount\n";
		valid = false;
		break;
	}
	visit(tree_test, stream);

	auto max = arr.size();
	for (size_t i = 0; i < max; i++) {
		auto c = rand() % (max - i);

		stream << "erase " << arr[c] << " at " << c << "\n";
		tree_test.erase_unique(arr[c]);
		arr.erase(arr.begin() + c);

		for (auto it = tree_test.begin(); it != tree_test.end(); it ++) {
			stream << *it << " ";
		}
		stream << "\n";
		auto val = validate(tree_test);
		switch (val) {
		case Validation::Valid:
			break;
		case Validation::RootIsNotBlack:
			stream << "Erase: Invalid Tree: RootIsNotBlack\n";
			visit(tree_test, stream);
			valid = false;
			break;
		case Validation::RedChildIntoRedNode:
			stream << "Erase: Invalid Tree: RedChildIntoRedNode\n";
			visit(tree_test, stream);
			valid = false;
			break;
		case Validation::DifferentBlackNodeCount:
			stream << "Erase: Invalid Tree: DifferentBlackNodeCount\n";
			visit(tree_test, stream);
			valid = false;
			break;
		}
	}

	if (!valid) {
		ostream << stream.str();
	}

	return valid;
}

bool TreeDebug::make_test(std::ostream &ostream, const apr::array<int> &data, const apr::array<int> &erase) {
	apr::rbtree::Tree<int, int> tree_test;
	apr::ostringstream stream;
	bool valid = true;

	for (auto &it : data) {
		tree_test.emplace(it);
	}

	stream << "Sorted: ";
	apr::array<int> arr;
	for (auto it = tree_test.begin(); it != tree_test.end(); it ++) {
		stream << *it << " ";
		arr.emplace_back(*it);
	}
	stream << "\n";

	auto val = validate(tree_test);
	switch (val) {
	case Validation::Valid:
		stream << "Valid Tree\n";
		break;
	case Validation::RootIsNotBlack:
		stream << "Invalid Tree: RootIsNotBlack\n";
		valid = false;
		break;
	case Validation::RedChildIntoRedNode:
		stream << "Invalid Tree: RedChildIntoRedNode\n";
		valid = false;
		break;
	case Validation::DifferentBlackNodeCount:
		stream << "Invalid Tree: DifferentBlackNodeCount\n";
		valid = false;
		break;
	}
	visit(tree_test, stream);


	for (auto &it : erase) {
		tree_test.erase_unique(it);

		val = validate(tree_test);
		switch (val) {
		case Validation::Valid:
			break;
		case Validation::RootIsNotBlack:
			stream << "Invalid Tree: RootIsNotBlack\n";
			visit(tree_test, stream);
			valid = false;
			break;
		case Validation::RedChildIntoRedNode:
			stream << "Invalid Tree: RedChildIntoRedNode\n";
			visit(tree_test, stream);
			valid = false;
			break;
		case Validation::DifferentBlackNodeCount:
			stream << "Invalid Tree: DifferentBlackNodeCount\n";
			visit(tree_test, stream);
			valid = false;
			break;
		}
	}

	if (!valid) {
		ostream << stream.str();
	}

	return valid;
}

bool TreeDebug::make_hint_test(std::ostream &stream, int size) {
	Tree<int, int> tree_test1;
	auto time_test1_s = std::chrono::system_clock::now();
	for(int i = 0; i < size; ++i) {
		tree_test1.emplace(i);
	}
	auto time_test1 = std::chrono::system_clock::now() - time_test1_s;


	auto time_test2_s = std::chrono::system_clock::now();
	Tree<int, int> tree_test2;
	auto it2 = tree_test2.begin();
	for(int i = 0; i < size; ++i) {
		tree_test2.emplace_hint(it2, i);
		it2 = tree_test2.end();
	}
	auto time_test2 = std::chrono::system_clock::now() - time_test2_s;


	auto time_test3_s = std::chrono::system_clock::now();
	Tree<int, int> tree_test3;
	auto it3 = tree_test3.begin();
	for(int i = size; i > 0; --i) {
		tree_test3.emplace_hint(it3, i);
		it3 = tree_test3.end();
	}
	auto time_test3 = std::chrono::system_clock::now() - time_test3_s;


	auto time_test4_s = std::chrono::system_clock::now();
	Tree<int, int> tree_test4;
	auto it4 = tree_test4.begin();
	for(int i = size; i > 0; --i) {
		tree_test4.emplace_hint(it4, i);
		it4 = tree_test4.begin();
	}
	auto time_test4 = std::chrono::system_clock::now() - time_test4_s;


	auto time_test5_s = std::chrono::system_clock::now();
	Tree<int, int> tree_test5;
	auto it5 = tree_test5.begin();
	for(int i = 0; i < size; ++i) {
		it5 = tree_test5.emplace_hint(it5, i);
	}
	auto time_test5 = std::chrono::system_clock::now() - time_test5_s;


	stream << std::fixed << std::setprecision(2)
	<< " time: " << time_test1.count() << " Emplace test: " << validate(tree_test1) << "\n"
	<< " time: " << time_test2.count() << " Emplace Hint test: " << validate(tree_test2) << "\n"
	<< " time: " << time_test3.count() << " Emplace Wrong Hint test: " << validate(tree_test3) << "\n"
	<< " time: " << time_test4.count() << " Emplace Corrected Hint test: " << validate(tree_test4) << "\n"
	<< " time: " << time_test5.count() << " Emplace Closest Hint test: " << validate(tree_test5) << "\n";

	return true;
}

#endif

}

NS_APR_END
#endif
