/*************************************************************************/
/*  a_star.cpp                                                           */
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

#include "a_star_thin.h"

#include "core/math/geometry.h"
#include "core/script_language.h"
#include "scene/scene_string_names.h"

int AStarThin::get_available_point_id() const {

	if (points.empty()) {
		return 1;
	}

	// return points.back()->key() + 1;
	return 0;
}

void AStarThin::add_point(int p_id, const Vector3 &p_pos, real_t p_weight_scale) {

	ERR_FAIL_COND(p_id < 0);
	ERR_FAIL_COND(p_weight_scale < 1);

	if (!points.has(p_id)) {
		Point pt;
		pt.id = p_id;
		pt.pos = p_pos;
		pt.weight_scale = p_weight_scale;
		pt.prev_point = -1;
		pt.open_pass = 0;
		pt.closed_pass = 0;
		pt.enabled = true;
		points[p_id] = pt;
	} else {
		points[p_id].pos = p_pos;
		points[p_id].weight_scale = p_weight_scale;
	}
}

Vector3 AStarThin::get_point_position(int p_id) const {

	ERR_FAIL_COND_V(!points.has(p_id), Vector3());

	return points[p_id].pos;
}

void AStarThin::set_point_position(int p_id, const Vector3 &p_pos) {

	ERR_FAIL_COND(!points.has(p_id));

	points[p_id].pos = p_pos;
}

real_t AStarThin::get_point_weight_scale(int p_id) const {

	ERR_FAIL_COND_V(!points.has(p_id), 0);

	return points[p_id].weight_scale;
}

void AStarThin::set_point_weight_scale(int p_id, real_t p_weight_scale) {

	ERR_FAIL_COND(!points.has(p_id));
	ERR_FAIL_COND(p_weight_scale < 1);

	points[p_id].weight_scale = p_weight_scale;
}

void AStarThin::remove_point(int p_id) {

	ERR_FAIL_COND(!points.has(p_id));

	Point &p = points[p_id];

	for (Set<int>::Element *E = p.neighbours.front(); E; E = E->next()) {

		Point &n = points[E->get()];
		// Segment s(p_id, E->get());
		// segments.erase(s);

		n.neighbours.erase(p_id);
		n.unlinked_neighbours.erase(p_id);
	}

	for (Set<int>::Element *E = p.unlinked_neighbours.front(); E; E = E->next()) {

		Point &n = points[E->get()];
		// Segment s(p_id, E->get());
		// segments.erase(s);

		n.neighbours.erase(p_id);
		n.unlinked_neighbours.erase(p_id);
	}

	points.erase(p_id);
}

void AStarThin::connect_points(int p_id, int p_with_id, bool bidirectional) {

	ERR_FAIL_COND(!points.has(p_id));
	ERR_FAIL_COND(!points.has(p_with_id));
	ERR_FAIL_COND(p_id == p_with_id);

	Point &a = points[p_id];
	Point &b = points[p_with_id];
	a.neighbours.insert(p_with_id);

	if (bidirectional)
		b.neighbours.insert(p_id);
	else
		b.unlinked_neighbours.insert(p_id);

	// Segment s(p_id, p_with_id);
	// segments.insert(s);
}
void AStarThin::disconnect_points(int p_id, int p_with_id) {

	// Segment s(p_id, p_with_id);
	// ERR_FAIL_COND(!segments.has(s));

	// segments.erase(s);

	Point &a = points[p_id];
	Point &b = points[p_with_id];
	a.neighbours.erase(p_with_id);
	a.unlinked_neighbours.erase(p_with_id);
	b.neighbours.erase(p_id);
	b.unlinked_neighbours.erase(p_id);
}

bool AStarThin::has_point(int p_id) const {

	return points.has(p_id);
}

Array AStarThin::get_points() {

	Array point_list;

	OAHashMap<int, Point>::Iterator it = points.iter();
	while (it.valid) {
		point_list.push_back(*it.key);
		it = points.next_iter(it);
	}

	return point_list;
}

PoolVector<int> AStarThin::get_point_connections(int p_id) {

	ERR_FAIL_COND_V(!points.has(p_id), PoolVector<int>());

	PoolVector<int> point_list;

	Point &p = points[p_id];

	OAHashMap<int, OAHashMap<int, bool>*> 
	for (Set<int>::Element *E = p.neighbours.front(); E; E = E->next()) {
		point_list.push_back(E->get());
	}

	return point_list;
}

bool AStarThin::are_points_connected(int p_id, int p_with_id) const {

	// Segment s(p_id, p_with_id);
	// return segments.has(s);
	return true;
}

void AStarThin::clear() {
	// segments.clear();
	edges.clear();
	points.clear();
}

int AStarThin::get_closest_point(const Vector3 &p_point) const {

	int closest_id = -1;
	real_t closest_dist = 1e20;
	OAHashMap<int, Point>::Iterator it = points.iter();

	while (it.valid) {

		const Point *p = it.value;
		if (!p->enabled) continue; // disabled points should not be considered.

		real_t d = p_point.distance_squared_to(p->pos);
		if (closest_id < 0 || d < closest_dist) {
			closest_dist = d;
			closest_id = p->id;
		}

		it = points.next_iter(it);

	}

	return closest_id;
}

Vector3 AStarThin::get_closest_position_in_segment(const Vector3 &p_point) const {

	real_t closest_dist = 1e20;
	bool found = false;
	Vector3 closest_point;

	/*
	for (const Set<Segment>::Element *E = segments.front(); E; E = E->next()) {

		const Point &from_point = points[E->get().from];
		const Point &to_point = points[E->get().to];

		if (!(from_point.enabled && to_point.enabled)) {
			continue;
		}

		Vector3 segment[2] = {
			from_point.pos,
			to_point.pos,
		};

		Vector3 p = Geometry::get_closest_point_to_segment(p_point, segment);
		real_t d = p_point.distance_squared_to(p);
		if (!found || d < closest_dist) {

			closest_point = p;
			closest_dist = d;
			found = true;
		}
	}
	*/

	return closest_point;
}

bool AStarThin::_solve(Point &begin_point, Point &end_point) {

	pass++;

	if (!end_point.enabled) {
		return false;
	}

	bool found_route = false;

	Vector<int> open_list;
	SortArray<int, SortPoints> sorter;
	sorter.compare.points = &points;

	begin_point.g_score = 0;
	begin_point.f_score = _estimate_cost(begin_point.id, end_point.id);

	open_list.push_back(begin_point.id);

	while (!open_list.empty()) {

		Point &p = points[open_list[0]]; // The currently processed point

		if (p.id == end_point.id) {
			found_route = true;
			break;
		}

		sorter.pop_heap(0, open_list.size(), open_list.ptrw()); // Remove the current point from the open list
		open_list.remove(open_list.size() - 1);
		p.closed_pass = pass; // Mark the point as closed

		OAHashMap<int, bool>::Iterator it = edges[p.id]->iter();

		while (it.valid) {
			
			Point &e = points[*it.value]; // The neighbour point

			if (!e.enabled || e.closed_pass == pass) {
				continue;
			}

			real_t tentative_g_score = p.g_score + _compute_cost(p.id, e.id) * e.weight_scale;

			bool new_point = false;

			if (e.open_pass != pass) { // The point wasn't inside the open list
				e.open_pass = pass;
				open_list.push_back(e.id);
				new_point = true;
			} else if (tentative_g_score >= e.g_score) { // The new path is worse than the previous
				continue;
			}

			e.prev_point = p.id;
			e.g_score = tentative_g_score;
			e.f_score = e.g_score + _estimate_cost(e.id, end_point.id);

			if (new_point) { // The position of the new points is already known
				sorter.push_heap(0, open_list.size() - 1, 0, e.id, open_list.ptrw());
			} else {
				sorter.push_heap(0, open_list.find(e.id), 0, e.id, open_list.ptrw());
			}

			it = edges[p.id]->next_iter(it);
			
		}

	}

	return found_route;
}

float AStarThin::_estimate_cost(int p_from_id, int p_to_id) {

	if (get_script_instance() && get_script_instance()->has_method(SceneStringNames::get_singleton()->_estimate_cost))
		return get_script_instance()->call(SceneStringNames::get_singleton()->_estimate_cost, p_from_id, p_to_id);

	return points[p_from_id].pos.distance_to(points[p_to_id].pos);
}

float AStarThin::_compute_cost(int p_from_id, int p_to_id) {

	if (get_script_instance() && get_script_instance()->has_method(SceneStringNames::get_singleton()->_compute_cost))
		return get_script_instance()->call(SceneStringNames::get_singleton()->_compute_cost, p_from_id, p_to_id);

	return points[p_from_id].pos.distance_to(points[p_to_id].pos);
}

PoolVector<Vector3> AStarThin::get_point_path(int p_from_id, int p_to_id) {

	ERR_FAIL_COND_V(!points.has(p_from_id), PoolVector<Vector3>());
	ERR_FAIL_COND_V(!points.has(p_to_id), PoolVector<Vector3>());

	Point &a = points[p_from_id];
	Point &b = points[p_to_id];

	if (a.id == b.id) {
		PoolVector<Vector3> ret;
		ret.push_back(a.pos);
		return ret;
	}

	Point &begin_point = a;
	Point &end_point = b;

	bool found_route = _solve(begin_point, end_point);
	if (!found_route) return PoolVector<Vector3>();

	Point &p = end_point;
	int pc = 1; // Begin point
	while (p.id != begin_point.id) {
		pc++;
		p = points[p.prev_point];
	}

	PoolVector<Vector3> path;
	path.resize(pc);

	{
		PoolVector<Vector3>::Write w = path.write();

		Point &p2 = end_point;
		int idx = pc - 1;
		while (p2.id != begin_point.id) {
			w[idx--] = p2.pos;
			p2 = points[p2.prev_point];
		}

		w[0] = p2.pos; // Assign first
	}

	return path;
}

PoolVector<int> AStarThin::get_id_path(int p_from_id, int p_to_id) {

	ERR_FAIL_COND_V(!points.has(p_from_id), PoolVector<int>());
	ERR_FAIL_COND_V(!points.has(p_to_id), PoolVector<int>());

	Point &a = points[p_from_id];
	Point &b = points[p_to_id];

	if (a.id == b.id) {
		PoolVector<int> ret;
		ret.push_back(a.id);
		return ret;
	}

	Point &begin_point = a;
	Point &end_point = b;

	bool found_route = _solve(begin_point, end_point);
	if (!found_route) return PoolVector<int>();

	Point &p = end_point;
	int pc = 1; // Begin point
	while (p.id != begin_point.id) {
		pc++;
		p = points[p.prev_point];
	}

	PoolVector<int> path;
	path.resize(pc);

	{
		PoolVector<int>::Write w = path.write();

		p = end_point;
		int idx = pc - 1;
		while (p.id != begin_point.id) {
			w[idx--] = p.id;
			p = points[p.prev_point];
		}

		w[0] = p.id; // Assign first
	}

	return path;
}

void AStarThin::set_point_disabled(int p_id, bool p_disabled) {

	ERR_FAIL_COND(!points.has(p_id));

	points[p_id].enabled = !p_disabled;
}

bool AStarThin::is_point_disabled(int p_id) const {

	ERR_FAIL_COND_V(!points.has(p_id), false);

	return !points[p_id].enabled;
}

void AStarThin::_bind_methods() {

	ClassDB::bind_method(D_METHOD("get_available_point_id"), &AStarThin::get_available_point_id);
	ClassDB::bind_method(D_METHOD("add_point", "id", "position", "weight_scale"), &AStarThin::add_point, DEFVAL(1.0));
	ClassDB::bind_method(D_METHOD("get_point_position", "id"), &AStarThin::get_point_position);
	ClassDB::bind_method(D_METHOD("set_point_position", "id", "position"), &AStarThin::set_point_position);
	ClassDB::bind_method(D_METHOD("get_point_weight_scale", "id"), &AStarThin::get_point_weight_scale);
	ClassDB::bind_method(D_METHOD("set_point_weight_scale", "id", "weight_scale"), &AStarThin::set_point_weight_scale);
	ClassDB::bind_method(D_METHOD("remove_point", "id"), &AStarThin::remove_point);
	ClassDB::bind_method(D_METHOD("has_point", "id"), &AStarThin::has_point);
	ClassDB::bind_method(D_METHOD("get_point_connections", "id"), &AStarThin::get_point_connections);
	ClassDB::bind_method(D_METHOD("get_points"), &AStarThin::get_points);

	ClassDB::bind_method(D_METHOD("set_point_disabled", "id", "disabled"), &AStarThin::set_point_disabled, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("is_point_disabled", "id"), &AStarThin::is_point_disabled);

	ClassDB::bind_method(D_METHOD("connect_points", "id", "to_id", "bidirectional"), &AStarThin::connect_points, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("disconnect_points", "id", "to_id"), &AStarThin::disconnect_points);
	ClassDB::bind_method(D_METHOD("are_points_connected", "id", "to_id"), &AStarThin::are_points_connected);

	ClassDB::bind_method(D_METHOD("clear"), &AStarThin::clear);

	ClassDB::bind_method(D_METHOD("get_closest_point", "to_position"), &AStarThin::get_closest_point);
	ClassDB::bind_method(D_METHOD("get_closest_position_in_segment", "to_position"), &AStarThin::get_closest_position_in_segment);

	ClassDB::bind_method(D_METHOD("get_point_path", "from_id", "to_id"), &AStarThin::get_point_path);
	ClassDB::bind_method(D_METHOD("get_id_path", "from_id", "to_id"), &AStarThin::get_id_path);

	BIND_VMETHOD(MethodInfo(Variant::REAL, "_estimate_cost", PropertyInfo(Variant::INT, "from_id"), PropertyInfo(Variant::INT, "to_id")));
	BIND_VMETHOD(MethodInfo(Variant::REAL, "_compute_cost", PropertyInfo(Variant::INT, "from_id"), PropertyInfo(Variant::INT, "to_id")));
}

AStarThin::AStarThin() {

	pass = 1;
}

AStarThin::~AStarThin() {

	pass = 1;
	clear();
}

/////////////////////////////////////////////////////////////

int AStarThin2D::get_available_point_id() const {
	return astar.get_available_point_id();
}

void AStarThin2D::add_point(int p_id, const Vector2 &p_pos, real_t p_weight_scale) {
	astar.add_point(p_id, Vector3(p_pos.x, p_pos.y, 0), p_weight_scale);
}

Vector2 AStarThin2D::get_point_position(int p_id) const {
	Vector3 p = astar.get_point_position(p_id);
	return Vector2(p.x, p.y);
}

void AStarThin2D::set_point_position(int p_id, const Vector2 &p_pos) {
	astar.set_point_position(p_id, Vector3(p_pos.x, p_pos.y, 0));
}

real_t AStarThin2D::get_point_weight_scale(int p_id) const {
	return astar.get_point_weight_scale(p_id);
}

void AStarThin2D::set_point_weight_scale(int p_id, real_t p_weight_scale) {
	astar.set_point_weight_scale(p_id, p_weight_scale);
}

void AStarThin2D::remove_point(int p_id) {
	astar.remove_point(p_id);
}

bool AStarThin2D::has_point(int p_id) const {
	return astar.has_point(p_id);
}

PoolVector<int> AStarThin2D::get_point_connections(int p_id) {
	return astar.get_point_connections(p_id);
}

Array AStarThin2D::get_points() {
	return astar.get_points();
}

void AStarThin2D::set_point_disabled(int p_id, bool p_disabled) {
	astar.set_point_disabled(p_id, p_disabled);
}

bool AStarThin2D::is_point_disabled(int p_id) const {
	return astar.is_point_disabled(p_id);
}

void AStarThin2D::connect_points(int p_id, int p_with_id, bool p_bidirectional) {
	astar.connect_points(p_id, p_with_id, p_bidirectional);
}

void AStarThin2D::disconnect_points(int p_id, int p_with_id) {
	astar.disconnect_points(p_id, p_with_id);
}

bool AStarThin2D::are_points_connected(int p_id, int p_with_id) const {
	return astar.are_points_connected(p_id, p_with_id);
}

void AStarThin2D::clear() {
	astar.clear();
}

int AStarThin2D::get_closest_point(const Vector2 &p_point) const {
	return astar.get_closest_point(Vector3(p_point.x, p_point.y, 0));
}

Vector2 AStarThin2D::get_closest_position_in_segment(const Vector2 &p_point) const {
	Vector3 p = astar.get_closest_position_in_segment(Vector3(p_point.x, p_point.y, 0));
	return Vector2(p.x, p.y);
}

PoolVector<Vector2> AStarThin2D::get_point_path(int p_from_id, int p_to_id) {

	PoolVector3Array pv = astar.get_point_path(p_from_id, p_to_id);
	int size = pv.size();
	PoolVector2Array path;
	path.resize(size);
	{
		PoolVector<Vector3>::Read r = pv.read();
		PoolVector<Vector2>::Write w = path.write();
		for (int i = 0; i < size; i++) {
			Vector3 p = r[i];
			w[i] = Vector2(p.x, p.y);
		}
	}
	return path;
}

PoolVector<int> AStarThin2D::get_id_path(int p_from_id, int p_to_id) {
	return astar.get_id_path(p_from_id, p_to_id);
}

void AStarThin2D::_bind_methods() {

	ClassDB::bind_method(D_METHOD("get_available_point_id"), &AStarThin2D::get_available_point_id);
	ClassDB::bind_method(D_METHOD("add_point", "id", "position", "weight_scale"), &AStarThin2D::add_point, DEFVAL(1.0));
	ClassDB::bind_method(D_METHOD("get_point_position", "id"), &AStarThin2D::get_point_position);
	ClassDB::bind_method(D_METHOD("set_point_position", "id", "position"), &AStarThin2D::set_point_position);
	ClassDB::bind_method(D_METHOD("get_point_weight_scale", "id"), &AStarThin2D::get_point_weight_scale);
	ClassDB::bind_method(D_METHOD("set_point_weight_scale", "id", "weight_scale"), &AStarThin2D::set_point_weight_scale);
	ClassDB::bind_method(D_METHOD("remove_point", "id"), &AStarThin2D::remove_point);
	ClassDB::bind_method(D_METHOD("has_point", "id"), &AStarThin2D::has_point);
	ClassDB::bind_method(D_METHOD("get_point_connections", "id"), &AStarThin2D::get_point_connections);
	ClassDB::bind_method(D_METHOD("get_points"), &AStarThin2D::get_points);

	ClassDB::bind_method(D_METHOD("set_point_disabled", "id", "disabled"), &AStarThin2D::set_point_disabled, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("is_point_disabled", "id"), &AStarThin2D::is_point_disabled);

	ClassDB::bind_method(D_METHOD("connect_points", "id", "to_id", "bidirectional"), &AStarThin2D::connect_points, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("disconnect_points", "id", "to_id"), &AStarThin2D::disconnect_points);
	ClassDB::bind_method(D_METHOD("are_points_connected", "id", "to_id"), &AStarThin2D::are_points_connected);

	ClassDB::bind_method(D_METHOD("clear"), &AStarThin2D::clear);

	ClassDB::bind_method(D_METHOD("get_closest_point", "to_position"), &AStarThin2D::get_closest_point);
	ClassDB::bind_method(D_METHOD("get_closest_position_in_segment", "to_position"), &AStarThin2D::get_closest_position_in_segment);

	ClassDB::bind_method(D_METHOD("get_point_path", "from_id", "to_id"), &AStarThin2D::get_point_path);
	ClassDB::bind_method(D_METHOD("get_id_path", "from_id", "to_id"), &AStarThin2D::get_id_path);
}

AStarThin2D::AStarThin2D() {
}

AStarThin2D::~AStarThin2D() {
}
