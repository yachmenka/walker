/* This file is part of Walker.
 * 
 * Walker is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Walker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Walker.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef SCENE_HPP
#define SCENE_HPP

#include "event.hpp"

using render_f = void (*)(const vector<text> &m, int x, int y);

/* Reset current sceanrio if it exists */
void scenario_create_from_config(const string &, render_f r_f, int l, int c);
void scenario_render();
void scenario_set_view_x(arg_t);
void scenario_set_view_y(arg_t);
void scenario_move_view_x(arg_t);
void scenario_move_view_y(arg_t);
void scenario_move_player_x(arg_t);
void scenario_move_player_y(arg_t);
bool scenario_parse_conditions(const event_conditions &conds, size_t size);
void scenario_parse_instructions(const event_instructions& instructions);

#endif // SCENE_HPP
