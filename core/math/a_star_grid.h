/*************************************************************************/
/*  a_star_grid.h                                                        */
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

#ifndef ASTAR_GRID_H
#define ASTAR_GRID_H

#include "core/reference.h"
#include "core/self_list.h"

int euclidean_distance_between(int x1, int y1, int x2, int y2);

class AStarGrid : public Reference {

	GDCLASS(AStarGrid, Reference);

	struct Node {
		int f_score;
		int g_score;
		int neighbours[26];
	};

	int width;
	int height;
	int depth;
	PoolVector<Node> grid;

	bool _solve(int from_idx, int to_idx);

protected:

	static void _bind_methods();

	virtual real_t _estimate_cost(int from_id, int to_id);
	virtual real_t _compute_cost(int from_id, int to_id);

public:

	int position_to_index(Vector3) const;
	int position_to_index(int x, int y, int z) const;
	Vector3 index_to_position(int idx) const;

	void connect_points(Vector3 from, Vector3 to, int cost, bool bidirectional = true);
	void disconnect_points(Vector3 from, Vector3 to, bool bidirectional = true);
	bool are_points_connected(Vector3 from, Vector3 to, bool bidirectional = true) const;

	void resize(int w, int h, int d);
	void clear();

	int get_closest_point(const Vector3 &p_point) const;

	PoolIntArray get_id_path(int from_id, int to_id); 

	AStarGrid();
	AStarGrid(int width, int height, int depth);
	~AStarGrid();

};

class AStarGrid2D : public Reference {

	GDCLASS(AStarGrid2D, Reference);

	uint64_t pass;

	const Vector2i neighbours[8] = {
		{-1, 1},
		{0, 1},
		{1, 1},
		{1, 0},
		{1, -1},
		{0, -1},
		{-1, -1},
		{-1, 0}
	};

	struct Node {
		uint64_t open_pass;
		uint64_t closed_pass;
		real_t f_score;
		real_t g_score;
		int came_from;
		real_t neighbours[8];
	};

	int width;
	int height;
	PoolVector<Node> grid;

	bool _solve(int from_idx, int to_idx);

	struct SortPath {
		Node *nodes;
		_FORCE_INLINE_ bool operator()(int a_idx, int b_idx) const {
			Node *A = &nodes[a_idx];
			Node *B = &nodes[b_idx];
			if (A->f_score > B->f_score) {
				return true;
			} else if (A->f_score < B->f_score) {
				return false;
			} else {
				return A->g_score < B->g_score; // tiebreaker, prioritize points further away from start
			}
		}
	};

protected:

	static void _bind_methods();

	virtual real_t _estimate_cost(int from_id, int to_id);
	virtual real_t _compute_cost(int from_id, int n_id);

public:

	int offset_to_neighbour(int x, int y);
	int position_to_index(const Vector2 &pos) const;
	int position_to_index(int x, int y) const;
	Vector2 index_to_position(int idx) const;

	bool connect_points(const Vector2 &from, const Vector2 &to, real_t cost, bool bidirectional = true);
	void disconnect_points(const Vector2 &from, const Vector2 &to, bool bidirectional = true);
	bool are_points_connected(const Vector2 &from, const Vector2 &to) const;

	PoolIntArray get_neighbour_costs(const Vector2 &point);
	void connect_to_neighbours(const Vector2 &point, real_t cost, bool diagonals = true);
	void disconnect_from_neighbours(const Vector2 &point);

	void resize(int w, int h);
	void clear();

	Vector2 get_closest_point(const Vector2 &p_point) const;
	PoolVector2Array get_grid_path(const Vector2 &from, const Vector2 &to);

	AStarGrid2D();
	~AStarGrid2D();

};

#endif // ASTAR_GRID_H
