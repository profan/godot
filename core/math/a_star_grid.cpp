/*************************************************************************/
/*  a_star_grid.cpp                                                      */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2019 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2019 Godot Engine contributors (cf. AUTHORS.md)    */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "a_star_grid.h"

#include "core/math/geometry.h"
#include "core/script_language.h"
#include "scene/scene_string_names.h"

int euclidean_distance_between(int x1, int y1, int x2, int y2) {
	return ((x2 - x1) * (x2 - x1)) + ((y2 - y1) * (y2 - y1));
}

bool AStarGrid::_solve(int from_idx, int to_idx) {
	return false;
}

void AStarGrid::_bind_methods() {

}

real_t AStarGrid::_estimate_cost(int from_id, int to_id) {

	if (get_script_instance() && get_script_instance()->has_method(SceneStringNames::get_singleton()->_estimate_cost))
		return get_script_instance()->call(SceneStringNames::get_singleton()->_estimate_cost, from_id, to_id);

	return index_to_position(from_id).distance_to(index_to_position(to_id));

}

real_t AStarGrid::_compute_cost(int from_id, int to_id) {
	
	if (get_script_instance() && get_script_instance()->has_method(SceneStringNames::get_singleton()->_compute_cost))
		return get_script_instance()->call(SceneStringNames::get_singleton()->_compute_cost, from_id, to_id);

	return index_to_position(from_id).distance_to(index_to_position(to_id));

}

int AStarGrid::position_to_index(Vector3 pos) const {
	return position_to_index(pos.x, pos.y, pos.z);
}

int AStarGrid::position_to_index(int x, int y, int z) const {
	int i = (y * width) + x;
	return i;
}

Vector3 AStarGrid::index_to_position(int idx) const {
	int x = idx % width;
	int y = idx / width;
	int z = 0; // FIXME
	return Vector3(x, y, z);
}

void AStarGrid::connect_points(Vector3 from, Vector3 to, int cost, bool bidirectional) {
	int from_idx = position_to_index(from.x, from.y, from.z);
	int to_idx = position_to_index(to.x, to.y, from.z);

}

void AStarGrid::disconnect_points(Vector3 from, Vector3 to, bool bidirectional) {
	int from_idx = position_to_index(from.x, from.y, from.z);
	int to_idx = position_to_index(to.x, to.y, from.z);
	
}

bool AStarGrid::are_points_connected(Vector3 from, Vector3 to, bool bidirectional) const {
	return false;
}

void AStarGrid::resize(int w, int h, int d) {
	grid.resize(w * h * d);
}

void AStarGrid::clear() {

}

PoolIntArray AStarGrid::get_id_path(int from_id, int to_id) {
	return PoolIntArray();
}

AStarGrid::AStarGrid(int w, int h, int d) : width(w), height(h), depth(d) {
	Error err = grid.resize(width * height * depth);
	if (err != Error::OK) {
		
	}
}

AStarGrid::AStarGrid() : width(0), height(0), depth(0) {

}

AStarGrid::~AStarGrid() {
	clear();
}

bool AStarGrid2D::_solve(int from_idx, int to_idx) {

	pass++;

	Node *writer = grid.write().ptr();
	Node *begin_point = &writer[from_idx];

	bool found_path = false;

	Vector<int> open_list;
	SortArray<int, SortPath> sorter;
	sorter.compare.nodes = writer;

	begin_point->g_score = 0;
	begin_point->f_score = _estimate_cost(from_idx, to_idx);

	open_list.push_back(from_idx);

	while (!open_list.empty()) {

		int p_idx = open_list[0];
		Node* p = &writer[p_idx];
		
		if (p_idx == to_idx) {
			found_path = true;
			break;
		}

		sorter.pop_heap(0, open_list.size(), open_list.ptrw()); // Remove the current point from the open list
		open_list.remove(open_list.size() - 1);

		for (int n = 0; n < 8; ++n) {

			// skip unconnected edge, not a neighbour
			if (p->neighbours[n] == -1) continue;

			Vector2 n_pos = index_to_position(p_idx) + neighbours[n];
			int n_idx = position_to_index(n_pos);

			// out of bounds
			if (n_idx == -1) continue;

			int tentative_g_score = p->g_score + _compute_cost(p_idx, n);

			if (tentative_g_score < grid[n_idx].g_score) {
				Node *n_ptr = &writer[n_idx];
				n_ptr->came_from = p_idx;
				n_ptr->g_score = tentative_g_score;
				n_ptr->f_score = n_ptr->g_score + _estimate_cost(p_idx, n_idx);
				if (n_ptr->pass != pass) {
					n_ptr->pass = pass;
					open_list.push_back(n_idx);
				}
			}

		}

	}

	return found_path;

}

int AStarGrid2D::offset_to_neighbour(int x, int y) {
	if (x == -1 && y == 1) {
		return 0;
	} else if (x == 0 && y == 1) {
		return 1;
	} else if (x == 1 && y == 1) {
		 return 2;
	} else if (x == 1 && y == 0) {
		return 3;
	} else if (x == 1 && y == -1) {
		return 4;
	} else if (x == 0 && y == -1) {
		return 5;
	} else if (x == -1 && y == -1) {
		return 6;
	} else if (x == -1 && y == 0) {
		return 7;
	} else {
		return -1;
	}
}

void AStarGrid2D::_bind_methods() {

	ClassDB::bind_method(D_METHOD("connect_points", "pos", "to_pos", "cost", "bidirectional"), &AStarGrid2D::connect_points, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("disconnect_points", "pos", "to_pos", "bidirectional"), &AStarGrid2D::disconnect_points, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("are_points_connected", "pos", "to_pos"), &AStarGrid2D::are_points_connected, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("resize", "w", "h"), &AStarGrid2D::resize);

	ClassDB::bind_method(D_METHOD("clear"), &AStarGrid2D::clear);
	ClassDB::bind_method(D_METHOD("get_closest_point", "to_position"), &AStarGrid2D::get_closest_point);
	ClassDB::bind_method(D_METHOD("get_grid_path", "from_pos", "to_pos"), &AStarGrid2D::get_grid_path);

	BIND_VMETHOD(MethodInfo(Variant::REAL, "_estimate_cost", PropertyInfo(Variant::INT, "from_id"), PropertyInfo(Variant::INT, "to_id")));
	BIND_VMETHOD(MethodInfo(Variant::REAL, "_compute_cost", PropertyInfo(Variant::INT, "from_id"), PropertyInfo(Variant::INT, "to_id")));

}

real_t AStarGrid2D::_estimate_cost(int from_id, int to_id) {

	if (get_script_instance() && get_script_instance()->has_method(SceneStringNames::get_singleton()->_estimate_cost))
		return get_script_instance()->call(SceneStringNames::get_singleton()->_estimate_cost, from_id, to_id);

	return index_to_position(from_id).distance_to(index_to_position(to_id));

}

real_t AStarGrid2D::_compute_cost(int from_id, int n) {
	
	if (get_script_instance() && get_script_instance()->has_method(SceneStringNames::get_singleton()->_compute_cost))
		return get_script_instance()->call(SceneStringNames::get_singleton()->_compute_cost, from_id, n);

	return grid[from_id].neighbours[n];

}

int AStarGrid2D::position_to_index(Vector2 pos) const {
	return position_to_index(pos.x, pos.y);
}

int AStarGrid2D::position_to_index(int x, int y) const {
	if (x < 0 || x >= width || y < 0 || y >= height) return -1;
	int i = (y * width) + x;
	return i;
}

Vector2 AStarGrid2D::index_to_position(int idx) const {
	int x = idx % width;
	int y = idx / width;
	return Vector2(x, y);
}

bool AStarGrid2D::connect_points(Vector2 from, Vector2 to, int cost, bool bidirectional) {

	int from_idx = position_to_index(from.x, from.y);
	int to_idx = position_to_index(to.x, to.y);

	Vector2 n_offset = from - to;
	int to_n = offset_to_neighbour(n_offset.x, n_offset.y);
	if (to_n == -1) {
		return false;
	} else {
		grid.write()[to_idx].neighbours[to_n] = cost;
	}

	if (bidirectional) {
		Vector2 other_n_offset = to - from;
		int from_n = offset_to_neighbour(other_n_offset.x, other_n_offset.y);
		if (from_n == -1) {
			return false;
		} else {
			grid.write()[from_idx].neighbours[from_n] = cost;
		}
	}

	return true;

}

void AStarGrid2D::disconnect_points(Vector2 from, Vector2 to, bool bidirectional) {

	int from_idx = position_to_index(from.x, from.y);
	int to_idx = position_to_index(to.x, to.y);
	Vector2 n_offset = to - from;
	int to_n = offset_to_neighbour(n_offset.x, n_offset.y);

	if (bidirectional) {
		Vector2 other_n_offset = from - to;
		int from_n = offset_to_neighbour(other_n_offset.x, other_n_offset.y);
		grid.write()[to_idx].neighbours[from_n] = -1;
	}

	grid.write()[from_idx].neighbours[to_n] = -1;

}

bool AStarGrid2D::are_points_connected(Vector2 from, Vector2 to) const {
	return false;
}

/* disconnect the point from all its neighbours, and all its neighbours from the point */
void AStarGrid2D::disconnect_point(Vector2 point) {
	for (int n = 0; n < 8; ++n) {

	}
}

void AStarGrid2D::resize(int w, int h) {

	grid.resize(w * h);
	width = w;
	height = h;

	for (int i = 0; i < grid.size(); ++i) {
		Node* writer = grid.write().ptr();
		writer[i].pass = -1;
		writer[i].f_score = __INT_MAX__;
		writer[i].g_score = __INT_MAX__;
		for (int n = 0; n < 8; ++n) {
			writer[i].neighbours[n] = -1;
		}
	}

}

void AStarGrid2D::clear() {

}

int AStarGrid2D::get_closest_point(const Vector2 &p_point) const {
	return -1;
}

PoolVector2Array AStarGrid2D::get_grid_path(Vector2 from_pos, Vector2 to_pos) {
	
	int from_id = position_to_index(from_pos);
	int to_id = position_to_index(to_pos);

	PoolVector2Array path = {};
	bool found_path = _solve(from_id, to_id);

	int cur_id = to_id;
	while (cur_id != from_id) {
		int came_from_id = grid[cur_id].came_from;
		path.push_back(index_to_position(cur_id));
		cur_id = came_from_id;
	}

	return path;

}

AStarGrid2D::AStarGrid2D() : width(0), height(0) {

}

AStarGrid2D::~AStarGrid2D() {
	clear();
}
