/*************************************************************************/
/*  a_star_grid_fixed.cpp                                                */
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

#include "a_star_grid_fixed.h"

#include "core/math/geometry.h"
#include "core/script_language.h"
#include "scene/scene_string_names.h"

/* decoding morton codes
 *  graciously borrowed from: https://fgiesen.wordpress.com/2009/12/13/decoding-morton-codes/
 * with regards to fgiesen
 */

uint32_t EncodeMorton2(uint32_t x, uint32_t y) {
  return (Part1By1(y) << 1) + Part1By1(x);
}

uint32_t EncodeMorton3(uint32_t x, uint32_t y, uint32_t z) {
  return (Part1By2(z) << 2) + (Part1By2(y) << 1) + Part1By2(x);
}

// "Insert" a 0 bit after each of the 16 low bits of x
uint32_t Part1By1(uint32_t x) {
  x &= 0x0000ffff;                  // x = ---- ---- ---- ---- fedc ba98 7654 3210
  x = (x ^ (x <<  8)) & 0x00ff00ff; // x = ---- ---- fedc ba98 ---- ---- 7654 3210
  x = (x ^ (x <<  4)) & 0x0f0f0f0f; // x = ---- fedc ---- ba98 ---- 7654 ---- 3210
  x = (x ^ (x <<  2)) & 0x33333333; // x = --fe --dc --ba --98 --76 --54 --32 --10
  x = (x ^ (x <<  1)) & 0x55555555; // x = -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0
  return x;
}

// "Insert" two 0 bits after each of the 10 low bits of x
uint32_t Part1By2(uint32_t x) {
  x &= 0x000003ff;                  // x = ---- ---- ---- ---- ---- --98 7654 3210
  x = (x ^ (x << 16)) & 0xff0000ff; // x = ---- --98 ---- ---- ---- ---- 7654 3210
  x = (x ^ (x <<  8)) & 0x0300f00f; // x = ---- --98 ---- ---- 7654 ---- ---- 3210
  x = (x ^ (x <<  4)) & 0x030c30c3; // x = ---- --98 ---- 76-- --54 ---- 32-- --10
  x = (x ^ (x <<  2)) & 0x09249249; // x = ---- 9--8 --7- -6-- 5--4 --3- -2-- 1--0
  return x;
}

// Inverse of Part1By1 - "delete" all odd-indexed bits
uint32_t Compact1By1(uint32_t x) {
  x &= 0x55555555;                  // x = -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0
  x = (x ^ (x >>  1)) & 0x33333333; // x = --fe --dc --ba --98 --76 --54 --32 --10
  x = (x ^ (x >>  2)) & 0x0f0f0f0f; // x = ---- fedc ---- ba98 ---- 7654 ---- 3210
  x = (x ^ (x >>  4)) & 0x00ff00ff; // x = ---- ---- fedc ba98 ---- ---- 7654 3210
  x = (x ^ (x >>  8)) & 0x0000ffff; // x = ---- ---- ---- ---- fedc ba98 7654 3210
  return x;
}

// Inverse of Part1By2 - "delete" all bits not at positions divisible by 3
uint32_t Compact1By2(uint32_t x) {
  x &= 0x09249249;                  // x = ---- 9--8 --7- -6-- 5--4 --3- -2-- 1--0
  x = (x ^ (x >>  2)) & 0x030c30c3; // x = ---- --98 ---- 76-- --54 ---- 32-- --10
  x = (x ^ (x >>  4)) & 0x0300f00f; // x = ---- --98 ---- ---- 7654 ---- ---- 3210
  x = (x ^ (x >>  8)) & 0xff0000ff; // x = ---- --98 ---- ---- ---- ---- 7654 3210
  x = (x ^ (x >> 16)) & 0x000003ff; // x = ---- ---- ---- ---- ---- --98 7654 3210
  return x;
}

uint32_t DecodeMorton2X(uint32_t code) {
  return Compact1By1(code >> 0);
}

uint32_t DecodeMorton2Y(uint32_t code) {
  return Compact1By1(code >> 1);
}

bool AStarGridFixed2D::_solve(int from_idx, int to_idx) {

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

		const int p_idx = open_list[0];
		Node* p = &writer[p_idx];
		
		if (p_idx == to_idx) {
			found_path = true;
			break;
		}

		// remove the current point from the open list
		sorter.pop_heap(0, open_list.size(), open_list.ptrw());
		open_list.remove(open_list.size() - 1);
		p->closed_pass = pass;

		for (int n = 0; n < 8; ++n) {

			// skip unconnected edge, not a neighbour
			if (p->neighbours[n] == -1) continue;

			const int n_x = DecodeMorton2X(p_idx) + neighbours[n].x;
			const int n_y = DecodeMorton2Y(p_idx) + neighbours[n].y;
			const int n_idx = position_to_index(n_x, n_y);

			// out of bounds or already handled
			if (n_idx == -1 || grid[n_idx].closed_pass == pass || !grid[n_idx].enabled) continue;

			const real_t tentative_g_score = p->g_score + _compute_cost(p_idx, n);
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

void AStarGridFixed2D::_bind_methods() {

	ClassDB::bind_method(D_METHOD("offset_to_neighbour", "x", "y"), &AStarGridFixed2D::offset_to_neighbour);
	ClassDB::bind_method(D_METHOD("index_to_position", "idx"), &AStarGridFixed2D::index_to_position);

	ClassDB::bind_method(D_METHOD("connect_points", "from", "to", "cost", "bidirectional"), &AStarGridFixed2D::connect_points, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("disconnect_points", "from", "to", "bidirectional"), &AStarGridFixed2D::disconnect_points, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("are_points_connected", "from", "to"), &AStarGridFixed2D::are_points_connected);

	ClassDB::bind_method(D_METHOD("connect_to_neighbours", "point", "cost", "diagonals"), &AStarGridFixed2D::connect_to_neighbours, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("disconnect_from_neighbours", "point"), &AStarGridFixed2D::disconnect_from_neighbours);

	ClassDB::bind_method(D_METHOD("set_point_enabled", "point", "enabled"), &AStarGridFixed2D::set_point_enabled);
	ClassDB::bind_method(D_METHOD("is_point_enabled", "point"), &AStarGridFixed2D::is_point_enabled);

	ClassDB::bind_method(D_METHOD("resize", "w", "h"), &AStarGridFixed2D::resize);
	ClassDB::bind_method(D_METHOD("clear"), &AStarGridFixed2D::clear);

	ClassDB::bind_method(D_METHOD("get_closest_point", "to_position"), &AStarGridFixed2D::get_closest_point);
	ClassDB::bind_method(D_METHOD("get_grid_path", "from", "to"), &AStarGridFixed2D::get_grid_path);

	BIND_VMETHOD(MethodInfo(Variant::REAL, "_estimate_cost", PropertyInfo(Variant::INT, "from_id"), PropertyInfo(Variant::INT, "to_id")));
	BIND_VMETHOD(MethodInfo(Variant::REAL, "_compute_cost", PropertyInfo(Variant::INT, "from_id"), PropertyInfo(Variant::INT, "n_id")));

}

real_t AStarGridFixed2D::_estimate_cost(int from_id, int to_id) {

	ScriptInstance *instance = get_script_instance();
	if (instance && instance->has_method(SceneStringNames::get_singleton()->_estimate_cost))
		return instance->call(SceneStringNames::get_singleton()->_estimate_cost, from_id, to_id);

	return index_to_position(from_id).distance_to(index_to_position(to_id));

}

real_t AStarGridFixed2D::_compute_cost(int from_id, int n_id) {
	
	ScriptInstance *instance = get_script_instance();
	if (instance && instance->has_method(SceneStringNames::get_singleton()->_compute_cost))
		return instance->call(SceneStringNames::get_singleton()->_compute_cost, from_id, n_id);

	return grid[from_id].neighbours[n_id];

}

int AStarGridFixed2D::offset_to_neighbour(int x, int y) const {

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

int AStarGridFixed2D::position_to_index(const Vector2 &pos) const {
	return position_to_index(pos.x, pos.y);
}

int AStarGridFixed2D::position_to_index(int x, int y) const {

	if (x < 0 || x >= width || y < 0 || y >= height)
		return -1;

	return EncodeMorton2(x, y);

}

Vector2 AStarGridFixed2D::index_to_position(int idx) const {

	ERR_EXPLAIN("index passed must be positive, was: " + itos(idx));
	ERR_FAIL_COND_V(idx < 0, Vector2(0, 0));

	const int x = DecodeMorton2X(idx);
	const int y = DecodeMorton2Y(idx);

	return Vector2(x, y);

}

bool AStarGridFixed2D::connect_points(const Vector2 &from, const Vector2 &to, real_t cost, bool bidirectional) {

	ERR_EXPLAIN("expected value within bounds of grid (" + itos(width) + "x" + itos(height) + ") for from, was out of bounds at (" + String(from) + ")");
	ERR_FAIL_COND_V(from.x < 0 || from.x >= width || from.y < 0 || from.y >= height, false);

	ERR_EXPLAIN("expected value within bounds of grid (" + itos(width) + "x" + itos(height) + ") for to, was out of bounds at (" + String(to) + ")");
	ERR_FAIL_COND_V(to.x < 0 || to.x >= width || to.y < 0 || to.y >= height, false);

	ERR_EXPLAIN("edge cost must be non-negative");
	ERR_FAIL_COND_V(cost < 0, false);

	const int from_idx = position_to_index(from.x, from.y);
	const int to_idx = position_to_index(to.x, to.y);

	Vector2 n_offset = from - to;
	const int to_n = offset_to_neighbour(n_offset.x, n_offset.y);
	if (to_n == -1) {
		return false;
	} else {
		grid.write()[to_idx].neighbours[to_n] = cost;
	}

	if (bidirectional) {
		const Vector2 other_n_offset = to - from;
		const int from_n = offset_to_neighbour(other_n_offset.x, other_n_offset.y);
		if (from_n == -1) {
			return false;
		} else {
			grid.write()[from_idx].neighbours[from_n] = cost;
		}
	}

	return true;

}

void AStarGridFixed2D::disconnect_points(const Vector2 &from, const Vector2 &to, bool bidirectional) {

	ERR_EXPLAIN("expected value within bounds of grid (" + itos(width) + "x" + itos(height) + ") for from, was out of bounds at (" + String(from) + ")");
	ERR_FAIL_COND(from.x < 0 || from.x >= width || from.y < 0 || from.y >= height);

	ERR_EXPLAIN("expected value within bounds of grid (" + itos(width) + "x" + itos(height) + ") for to, was out of bounds at (" + String(to) + ")");
	ERR_FAIL_COND(to.x < 0 || to.x >= width || to.y < 0 || to.y >= height);

	const int from_idx = position_to_index(from.x, from.y);
	const int to_idx = position_to_index(to.x, to.y);

	const Vector2 n_offset = to - from;
	const int to_n = offset_to_neighbour(n_offset.x, n_offset.y);
	if (to_n == -1) {
		return;
	} else {
		grid.write()[from_idx].neighbours[to_n] = -1;
	}

	if (bidirectional) {
		const Vector2 other_n_offset = from - to;
		const int from_n = offset_to_neighbour(other_n_offset.x, other_n_offset.y);
		if (from_n == -1) {
			return;
		} else {
			grid.write()[to_idx].neighbours[from_n] = -1;
		}
	}

}

bool AStarGridFixed2D::are_points_connected(const Vector2 &from, const Vector2 &to) const {

	ERR_EXPLAIN("expected value within bounds of grid  (" + itos(width) + "x" + itos(height) + ") for from, was out of bounds at (" + String(from) + ")");
	ERR_FAIL_COND_V(from.x < 0 || from.x >= width || from.y < 0 || from.y >= height, false);

	ERR_EXPLAIN("expected value within bounds of grid  (" + itos(width) + "x" + itos(height) + ") for to, was out of bounds at (" + String(to) + ")");
	ERR_FAIL_COND_V(to.x < 0 || to.x >= width || to.y < 0 || to.y >= height, false);

	const Vector2 delta = to - from;
	const int n_id = offset_to_neighbour(delta.x, delta.y);
	if (n_id != -1) {
		const int from_id = position_to_index(from);
		return grid[from_id].neighbours[n_id] != -1;
	}

	return false;

}

real_t AStarGridFixed2D::get_neighbour_cost(const Vector2 &point, int n_id) const {

	ERR_EXPLAIN("expected neighbour index between 0 and 8, was out of bounds at: " + itos(n_id));
	ERR_FAIL_COND_V(n_id < 0 || n_id > 8, -1);

	const int p_id = position_to_index(point);
	return grid[p_id].neighbours[n_id];
	
}

void AStarGridFixed2D::connect_to_neighbours(const Vector2 &point, real_t cost, bool diagonals) {

	// ERR_EXPLAIN([=](){ exit(-1); return "Hello, World!"; }());
	ERR_EXPLAIN("expected value within bounds of grid (" + itos(width) + "x" + itos(height) + ") for point, was out of bounds at (" + String(point) + ")");
	ERR_FAIL_COND(point.x < 0 || point.x >= width || point.y < 0 || point.y >= height);

	ERR_EXPLAIN("edge cost must be non-negative, was: " + rtos(cost));
	ERR_FAIL_COND(cost < 0);

	for (int n = 0; n < 8; ++n) {
		const Vector2 n_pos = point + neighbours[n];
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
void AStarGridFixed2D::disconnect_from_neighbours(const Vector2 &point) {

	ERR_EXPLAIN("expected value within bounds of grid (" + itos(width) + "x" + itos(height) + ") for point, was out of bounds at (" + String(point) + ")");
	ERR_FAIL_COND(point.x < 0 || point.x >= width || point.y < 0 || point.y >= height);

	for (int n = 0; n < 8; ++n) {
		const Vector2 n_pos = point + neighbours[n];
		if (n_pos.x < 0 || n_pos.x >= width || n_pos.y < 0 || n_pos.y >= height) {
			continue;
		} else {
			disconnect_points(point, n_pos);
		}
	}

}

void AStarGridFixed2D::set_point_enabled(const Vector2 &point, bool state) {

	ERR_EXPLAIN("expected value within bounds of grid (" + itos(width) + "x" + itos(height) + ") for point, was out of bounds at (" + String(point) + ")");
	ERR_FAIL_COND(point.x < 0 || point.x >= width || point.y < 0 || point.y >= height);

	grid.write()[position_to_index(point)].enabled = true;

}

bool AStarGridFixed2D::is_point_enabled(const Vector2 &point) const {

	ERR_EXPLAIN("expected value within bounds of grid (" + itos(width) + "x" + itos(height) + ") for point, was out of bounds at (" + String(point) + ")");
	ERR_FAIL_COND_V(point.x < 0 || point.x >= width || point.y < 0 || point.y >= height, false);

	return grid[position_to_index(point)].enabled;

}

void AStarGridFixed2D::resize(int w, int h) {

	ERR_EXPLAIN("grid dimensions must be less than 32768x32768, got: (" + itos(w) + "x" + itos(h) + ")");
	ERR_FAIL_COND(w > INT16_MAX && h > INT16_MAX);

	ERR_EXPLAIN("grid size dimensions must be positive, got: (" + itos(w) + "x" + itos(h) + ")");
	ERR_FAIL_COND(w < 0 || h < 0);

	int next_pot_w = next_power_of_2(w);
	int next_pot_h = next_power_of_2(h);
	int max_power = MAX(next_pot_w, next_pot_h);
	next_pot_w = next_pot_h = max_power;

	grid.resize(next_pot_w * next_pot_h);
	width = next_pot_w;
	height = next_pot_h;

	clear();

}

void AStarGridFixed2D::clear() {

	Node *writer = grid.write().ptr();

	for (int i = 0; i < grid.size(); ++i) {
		writer[i].came_from = -1;
		writer[i].open_pass = 0;
		writer[i].closed_pass = 0;
		writer[i].f_score = __INT_MAX__;
		writer[i].g_score = __INT_MAX__;
		writer[i].enabled = true;
		for (int n = 0; n < 8; ++n) {
			writer[i].neighbours[n] = -1;
		}
	}

}

Vector2 AStarGridFixed2D::get_closest_point(const Vector2 &point) const {

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

PoolVector2Array AStarGridFixed2D::get_grid_path(const Vector2 &from, const Vector2 &to) {

	ERR_EXPLAIN("expected value within bounds of grid (" + itos(width) + "x" + itos(height) + ") for from, was out of bounds at (" + String(from) + ")");
	ERR_FAIL_COND_V(from.x < 0 || from.x >= width || from.y < 0 || from.y >= height, PoolVector2Array());

	ERR_EXPLAIN("expected value within bounds of grid (" + itos(width) + "x" + itos(height) + ") for to, was out of bounds at (" + String(to) + ")");
	ERR_FAIL_COND_V(to.x < 0 || to.x >= width || to.y < 0 || to.y >= height, PoolVector2Array());
	
	const int from_id = position_to_index(from);
	const int to_id = position_to_index(to);

	PoolVector2Array path = {};
	const bool found_path = _solve(from_id, to_id);
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
	path.invert();

	return path;

}

AStarGridFixed2D::AStarGridFixed2D() : pass(1), width(0), height(0) {

}

AStarGridFixed2D::~AStarGridFixed2D() {
	pass = 1;
	clear();
}
