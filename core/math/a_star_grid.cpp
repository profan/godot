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

		// remove the current point from the open list
		sorter.pop_heap(0, open_list.size(), open_list.ptrw());
		open_list.remove(open_list.size() - 1);
		p->closed_pass = pass;

		for (int n = 0; n < 4; ++n) {

			// skip unconnected edge, not a neighbour
			if (p->neighbours[n] == -1) continue;

			int n_x = DecodeMorton2X(p_idx) + neighbours[n].x;
			int n_y = DecodeMorton2Y(p_idx) + neighbours[n].y;
			int n_idx = position_to_index(n_x, n_y);

			// out of bounds or already handled
			if (n_idx == -1 || grid[n_idx].closed_pass == pass) continue;

			real_t tentative_g_score = p->g_score + _compute_cost(p_idx, n);
			bool new_point = false;

			Node *n_ptr = &writer[n_idx];
			if (n_ptr->open_pass != pass) {
				n_ptr->open_pass = pass;
				open_list.push_back(n_idx);
				new_point = true;
			} else if (tentative_g_score >= grid[n_idx].g_score) {
				continue;
			}

			n_ptr->came_from = p_idx;
			n_ptr->g_score = tentative_g_score;
			n_ptr->f_score = n_ptr->g_score + _estimate_cost(n_idx, to_idx);

			if (new_point) {
				sorter.push_heap(0, open_list.size() - 1, 0, n_idx, open_list.ptrw());
			} else {
				sorter.push_heap(0, open_list.find(n_idx), 0, n_idx, open_list.ptrw());
			}

		}

	}

	return found_path;

}

AStarGrid2D::Node AStarGrid2D::resolve_to_chunk(int x, int y) {

	AStarGrid2D::Node found_node;
	bool lookup_result = grid.lookup(Vector2i(x, y), found_node);
	if (lookup_result) {
		return found_node;
	} else {
		Node new_node;
		new_node.came_from = -1;
		new_node.closed_pass = 0;
		new_node.open_pass = 0;
		new_node.f_score = __FLT_MAX__;
		new_node.g_score = __FLT_MAX__;
		AStarGridFixed2D *new_grid = memnew(AStarGridFixed2D);
		new_grid->resize(chunk_width, chunk_height);
		new_node.grid = new_grid;
		return new_node;
	}

}

void AStarGrid2D::_bind_methods() {

	ClassDB::bind_method(D_METHOD("offset_to_neighbour", "x", "y"), &AStarGrid2D::offset_to_neighbour);
	ClassDB::bind_method(D_METHOD("index_to_position", "idx"), &AStarGrid2D::index_to_position);

	ClassDB::bind_method(D_METHOD("connect_points", "from", "to", "cost", "bidirectional"), &AStarGrid2D::connect_points, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("disconnect_points", "from", "to", "bidirectional"), &AStarGrid2D::disconnect_points, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("are_points_connected", "from", "to"), &AStarGrid2D::are_points_connected);

	ClassDB::bind_method(D_METHOD("connect_to_neighbours", "point", "cost", "diagonals"), &AStarGrid2D::connect_to_neighbours, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("disconnect_from_neighbours", "point"), &AStarGrid2D::disconnect_from_neighbours);

	ClassDB::bind_method(D_METHOD("clear"), &AStarGrid2D::clear);

	ClassDB::bind_method(D_METHOD("get_closest_point", "to_position"), &AStarGrid2D::get_closest_point);
	ClassDB::bind_method(D_METHOD("get_grid_path", "from", "to"), &AStarGrid2D::get_grid_path);

	BIND_VMETHOD(MethodInfo(Variant::REAL, "_estimate_cost", PropertyInfo(Variant::INT, "from_id"), PropertyInfo(Variant::INT, "to_id")));
	BIND_VMETHOD(MethodInfo(Variant::REAL, "_compute_cost", PropertyInfo(Variant::INT, "from_id"), PropertyInfo(Variant::INT, "n_id")));

}

real_t AStarGrid2D::_estimate_cost(int from_id, int to_id) {

	if (get_script_instance() && get_script_instance()->has_method(SceneStringNames::get_singleton()->_estimate_cost))
		return get_script_instance()->call(SceneStringNames::get_singleton()->_estimate_cost, from_id, to_id);

	return index_to_position(from_id).distance_to(index_to_position(to_id));

}

real_t AStarGrid2D::_compute_cost(int from_id, int n_id) {
	
	if (get_script_instance() && get_script_instance()->has_method(SceneStringNames::get_singleton()->_compute_cost))
		return get_script_instance()->call(SceneStringNames::get_singleton()->_compute_cost, from_id, n_id);

	return grid[from_id].neighbours[n_id];

}

int AStarGrid2D::offset_to_neighbour(int x, int y) const {

	if (x == 0 && y == 1) {
		return 0;
	} else if (x == 1 && y == 0) {
		return 1;
	} else if (x == 0 && y == -1) {
		return 2;
	} else if (x == -1 && y == 0) {
		return 3;
	} else {
		return -1;
	}

}

int AStarGrid2D::position_to_index(const Vector2 &pos) const {
	return position_to_index(pos.x, pos.y);
}

int AStarGrid2D::position_to_index(int x, int y) const {

	if (x < 0 || x >= width || y < 0 || y >= height)
		return -1;

	return EncodeMorton2(x, y);

}

Vector2 AStarGrid2D::index_to_position(int idx) const {

	ERR_FAIL_COND_V(idx < 0, Vector2(0, 0));

	int x = DecodeMorton2X(idx);
	int y = DecodeMorton2Y(idx);

	return Vector2(x, y);

}

bool AStarGrid2D::connect_points(const Vector2 &from, const Vector2 &to, real_t cost, bool bidirectional) {

	ERR_EXPLAIN("edge cost must be non-negative");
	ERR_FAIL_COND_V(cost < 0, false);

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

void AStarGrid2D::disconnect_points(const Vector2 &from, const Vector2 &to, bool bidirectional) {

	int from_idx = position_to_index(from.x, from.y);
	int to_idx = position_to_index(to.x, to.y);

	Vector2 n_offset = to - from;
	int to_n = offset_to_neighbour(n_offset.x, n_offset.y);
	if (to_n == -1) {
		return;
	} else {
		grid.write()[from_idx].neighbours[to_n] = -1;
	}

	if (bidirectional) {
		Vector2 other_n_offset = from - to;
		int from_n = offset_to_neighbour(other_n_offset.x, other_n_offset.y);
		if (from_n == -1) {
			return;
		} else {
			grid.write()[to_idx].neighbours[from_n] = -1;
		}
	}

}

bool AStarGrid2D::are_points_connected(const Vector2 &from, const Vector2 &to) const {

	Vector2 delta = to - from;
	int n_id = offset_to_neighbour(delta.x, delta.y);
	if (n_id != -1) {
		int from_id = position_to_index(from);
		return grid[from_id].neighbours[n_id] != -1;
	}

	return false;

}

real_t AStarGrid2D::get_neighbour_cost(const Vector2 &point, int n_id) const {

	// ERR_EXPLAIN("expected neighbour index between 0 and 8, was out of bounds at: " + itos(n_id));
	ERR_FAIL_COND_V(n_id < 0 || n_id > 8, -1);

	int p_id = position_to_index(point);
	return grid[p_id].neighbours[n_id];
	
}

void AStarGrid2D::connect_to_neighbours(const Vector2 &point, real_t cost, bool diagonals) {

	ERR_EXPLAIN("edge cost must be non-negative, was: " + rtos(cost));
	ERR_FAIL_COND(cost < 0);

	Node chunk = _resolve_to_chunk(point.x, point.y);

	for (int n = 0; n < 4; ++n) {
		Vector2 n_pos = point + neighbours[n];
		if (n_pos.x < 0 || n_pos.x >= width || n_pos.y < 0 || n_pos.y >= height) {
			continue;
		} else {
			if (diagonals || (neighbours[n].x == 0 || neighbours[n].y == 0)) {
				connect_points(point, n_pos, cost * point.distance_to(n_pos), true);
			}
		}
	}
	
}

/* disconnect the point from all its neighbours, and all its neighbours from the point */
void AStarGrid2D::disconnect_from_neighbours(const Vector2 &point) {

	Node chunk = _resolve_to_chunk(point.x, point.y);

	for (int n = 0; n < 4; ++n) {
		Vector2 n_pos = point + neighbours[n];
		if (n_pos.x < 0 || n_pos.x >= width || n_pos.y < 0 || n_pos.y >= height) {
			continue;
		} else {
			disconnect_points(point, n_pos);
		}
	}

}

void AStarGrid2D::clear() {

	Node *writer = grid.write().ptr();

	for (int i = 0; i < grid.size(); ++i) {
		writer[i].came_from = -1;
		writer[i].open_pass = 0;
		writer[i].closed_pass = 0;
		writer[i].f_score = __INT_MAX__;
		writer[i].g_score = __INT_MAX__;
		for (int n = 0; n < 8; ++n) {
			writer[i].neighbours[n] = -1;
		}
	}

}

Vector2 AStarGrid2D::get_closest_point(const Vector2 &point) const {

	/* intersect with Rect2 of grid */
	if (point.x < 0 || point.x >= width || point.y < 0 || point.y >= height) {

		Vector2 center = Vector2(width / 2, height / 2);

		Vector2 result;
		Rect2 grid_rect = Rect2(0, 0, width, height);
		grid_rect.intersects_segment(point, center, &result);

		return result.floor();

	} else {
		
		return point.floor();

	}

}

PoolVector2Array AStarGrid2D::get_grid_path(const Vector2 &from, const Vector2 &to) {
	
	int from_id = position_to_index(from);
	int to_id = position_to_index(to);

	PoolVector2Array path = {};
	bool found_path = _solve(from_id, to_id);
	if (!found_path) {
		return path;
	}

	int cur_id = to_id;
	while (cur_id != from_id) {
		int came_from_id = grid[cur_id].came_from;
		path.push_back(index_to_position(cur_id));
		cur_id = came_from_id;
	}
	
	path.push_back(index_to_position(from_id));

	return path;

}

AStarGrid2D::AStarGrid2D() : pass(1), width(0), height(0) {

}

AStarGrid2D::~AStarGrid2D() {
	pass = 1;
	clear();
}
