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

#include <cstdio>
#include <limits>
#include <cstring>
#include <sstream>
#include <yaml.h>
#include <cmath>
#include <algorithm>
#include <string>
#include <memory>

#include "scenario_constants.hpp"
#include "scene.hpp"
#include "utils.hpp"
#include "map.hpp"
#include "event.hpp"
#include "object.hpp"
#include "window.hpp"

using std::to_string;
using std::string;
using std::vector;
using std::istringstream;
using std::getline;
using std::find;
using std::prev;
using std::unique_ptr;
using std::string;

void parse_call(const string &call, string &id, string &method, string &args);

using events = vector<unique_ptr<event>>;
using objects = vector<unique_ptr<object>>;

class scenario {

  string                    m_file;
  int                       m_lines;
  int                       m_cols;
  unique_ptr<character_map> m_source      = nullptr;
  unique_ptr<character_map> m_render      = nullptr;
  render_f                  m_render_f;
  events                    m_events;
  objects                   m_objects;
  objects::iterator         m_player      = m_objects.end();
  vector<string>            m_identifiers = {RESERVED_DIALOG_ID, RESERVED_SCENARIO_ID};

  objects::const_iterator find_object(const string& id) const;
  events::const_iterator  find_event(const string& id) const;

  bool abroad(int x, int y) const
  { return x >= m_source->width() || y >= m_source->height() || x < 0 || y < 0; }

  bool abroadx(int x) const
  { return x >= m_source->width() || x < 0; }

  bool abroady(int y) const
  { return y >= m_source->height() || y < 0; }

  void add_id(const string &id);
  void render_los(const object& viewer);
  void render_set_visible(int x, int y);
  void source_set_detected(int x, int y);
  void turn();
  void load(const string &f);
  void parse_yaml();
  void parse_yaml(const char *section_type, const yaml_node_t *node, yaml_document_t *doc);
  void parse_command(const string& comm);
  bool parse_condition(const string& cond) const;

  int height() const { return m_source->height(); }
  int width()  const { return m_source->width(); }
  int x()      const { return m_source->x(); }
  int y()      const { return m_source->y(); }

public:

  scenario(scenario &&) = default;

  scenario(const scenario &)            = delete;
  scenario& operator=(const scenario &) = delete;

  scenario(const string &f, render_f r_f, int l, int c);

  void render();
  void set_view   (int x, int y);
  void move_player(int x, int y);
  void move_view  (int x, int y);

  bool parse_conditions(const event_conditions &conds, size_t size) const;
  void parse_instructions  (const event_instructions &instructions);
};

static unique_ptr<scenario> single_scenario;

scenario::scenario(const string &f, render_f r_f, int l, int c) :
  m_lines(l), m_cols(c), m_render_f(r_f)
{
  load(f);

  set_view((*m_player)->x() - m_cols/2,
           (*m_player)->y() - m_lines/2);
}

void scenario::load(const string& f)
{
  m_file = f;
  parse_yaml();
}

void scenario::move_player(int x, int y)
{
  int px = (*m_player)->x();
  int py = (*m_player)->y();

  /* Return view to player */
  set_view(px - m_cols / 2, py - m_lines / 2);

  int npx = px + x;
  int npy = py + y;

  if (abroad(npx, npy))
    return;

  if ((*m_player)->move(x, y, m_source->at(npx, npy).symbol))
    {
      set_view(npx - m_cols / 2, npy - m_lines / 2);
      turn();
    }
}

void scenario::set_view(int x, int y)
{
  if (!abroadx(x) && !abroadx(x + m_cols - 1))
    m_source->setx(x);

  if (!abroady(y) && !abroady(y + m_lines - 1))
    m_source->sety(y);
}

void scenario::move_view(int x, int y)
{
  set_view(m_source->x() + x, m_source->y() + y);
}

void scenario::render_set_visible(int x, int y)
{
  m_render->at(x, y).attribute &= ~(A_INVIS | A_DIM);
  m_render->at(x, y).attribute |= A_BOLD;
}

void scenario::source_set_detected(int x, int y)
{
  m_source->at(x, y).attribute &= ~A_INVIS;
  m_source->at(x, y).attribute |= A_DIM;
}

void scenario::render_los(const object &viewer)
{
  using std::hypot;

  int vision_range = viewer.vision_range();

  int px = viewer.x();
  int py = viewer.y();

  for (int y = py - vision_range; y < py + vision_range; ++y)
    for (int x = px - vision_range; x < px + vision_range; ++x)
      {
        if (round(hypot(x - px, y - py)) >= vision_range) continue;

        int px = viewer.x();
        int py = viewer.y();

        int delta_x(x - px);

        signed char const ix((delta_x > 0) - (delta_x < 0));
        delta_x = std::abs(delta_x) << 1;

        int delta_y(y - py);

        signed char const iy((delta_y > 0) - (delta_y < 0));
        delta_y = std::abs(delta_y) << 1;

        if (!abroad(x, y))
          {
            source_set_detected(px, py);
            render_set_visible(px, py);
            if (!viewer.visible(m_source->at(px, py).symbol))
              goto next_line;
          }

        if (delta_x >= delta_y)
          {
            int error(delta_y - (delta_x >> 1));

            while (px != x)
              {
                if ((error > 0) || (!error && (ix > 0)))
                  {
                    error -= delta_x;
                    py += iy;
                  }

                error += delta_y;
                px += ix;

                if (!abroad(px, py))
                  {
                    source_set_detected(px, py);
                    render_set_visible(px, py);
                    if (!viewer.visible(m_source->at(px, py).symbol))
                      goto next_line;
                  }
              }
          }
        else
          {
            int error(delta_x - (delta_y >> 1));

            while (py != y)
              {
                if ((error > 0) || (!error && (iy > 0)))
                  {
                    error -= delta_y;
                    px += ix;
                  }

                error += delta_x;
                py += iy;

                if (!abroad(px, py))
                  {
                    source_set_detected(px, py);
                    render_set_visible(px, py);
                    if (!viewer.visible(m_source->at(px, py).symbol))
                      goto next_line;
                  }
              }
          }
        next_line: ;
      }
}

void scenario::turn()
{
  /* At first rendered map of the new state,
     * and then later checked
     * events and pop-up windows */
  render();

  for (auto &event : m_events)
    {
      event->test();

      /* Increase event turns if it was happened */
      if (event->happened())
        event->inc();
    }
}

void scenario::render()
{ 
  m_render.reset(new character_map(*m_source));

  render_los(*m_player->get());

  for (auto& obj : m_objects)
    {
      auto &tile = m_render->at(obj->x(), obj->y());
      tile.symbol = obj->symbol().symbol;
      tile.attribute &= ~COLOR_PAIR( PAIR_NUMBER(tile.attribute) );
      tile.attribute |=  COLOR_PAIR( PAIR_NUMBER(obj->symbol().attribute) );
    }
  m_render_f(m_render->get_map(), x(), y());
}

objects::const_iterator scenario::find_object(const string& id) const
{
  for (auto iter = m_objects.begin(); iter != m_objects.end(); ++iter)
    {
      if ((*iter)->id() == id)
        return iter;
    }
  return m_objects.end();
}

events::const_iterator scenario::find_event(const string& id) const
{
  for (auto iter = m_events.begin(); iter != m_events.end(); ++iter)
    {
      if ((*iter)->id() == id)
        return iter;
    }
  return m_events.end();
}

std::tuple<string, string, string> parse_call(const string &call)
{

  string id, method, args;

  auto pointer  = call.find('.');
  auto cbracket = call.rfind(')');
  auto obracket = call.find('(');

  /* function(2, 4, '.') - hasn't object, but point exists */
  if (pointer > obracket or pointer == string::npos)
    {
      pointer = 0;
      id = "";
    }
  else
    id = call.substr(0, pointer++);

  method = call.substr(pointer, obracket - pointer);

  if (obracket == string::npos or cbracket == string::npos)
    args = "";
  else
    args = call.substr(obracket + 1, cbracket - (obracket + 1));

  return std::make_tuple(id, method, args);
}

bool scenario::parse_condition(const string& cond) const
{
  auto [id, method, args] = parse_call(cond);

  if (id.empty())
    {
      if (method == "false")
          return false;

      else if (method == "true")
        return true;
    }
  else
    {
      if (id == RESERVED_SCENARIO_ID)
        {

          return false;
        }

      auto object = find_object(id);
      if (object != m_objects.end())
        {
          if (method == "in")
            {
              istringstream in(args);
              int x;
              int y;
              in >> x >> y;
              if (x == (*object)->x() and
                  y == (*object)->y())
                return true;
            }

          return false;
        }

      auto event = find_event(id);
      if (event != m_events.end())
        {
          if (method == "happened") {
              if (args.empty())
                return event->get()->happened();
              else
                return event->get()->happened(stoi(args));
            }

          return false;
        }
    }
  return false;
}

void scenario::parse_command(const string& comm)
{

  auto [id, method, args] = parse_call(comm);

  if (id.empty())
    {

    }
  else
    {
      if (id == RESERVED_SCENARIO_ID)
        {
          if (method == "exit")
            window_set(BUILD_MAIN);

          return;
        }
      else if (id == RESERVED_DIALOG_ID)
      {
          if (method == "close")
              window_pop();

          return;
      }

      auto object = find_object(id);
      if (object != m_objects.end())
        {

          return;
        }

      auto event = find_event(id);
      if (event != m_events.end())
        {
          if (method == "run")
            {
              (*event)->run();
            }
          return;
        }
    }
}

bool scenario::parse_conditions(const event_conditions &conds, size_t size) const
{
  bool result = 0;
  bool and_sequence = 1;

  for (size_t i = 0; i < size; ++i)
    {
      bool condition;

      /* If conds contain other conds, this function will be called recursively */
      if (conds[i].next)
        {
          condition = parse_conditions(conds[i].next, conds[i].size);
        }
      else if (conds[i].cond == "or")
        {
          result += and_sequence;
          and_sequence = 1;
          continue;
        }
      else
        {
          condition = (conds[i].cond.back() == '!')?
                !parse_condition(conds[i].cond) :
                parse_condition(conds[i].cond);
        }
      and_sequence *= condition;
    }
  result += and_sequence;
  return result;
}

void scenario::parse_instructions(const event_instructions &instructions)
{
  for (auto &f : instructions) parse_command(f);
}

void scenario::parse_yaml() {

  FILE *file = nullptr;

  yaml_parser_t parser;
  yaml_document_t document;
  yaml_node_t *node;

  std::ostringstream error;

  file = fopen(m_file.c_str(), "r");

  if(!yaml_parser_initialize(&parser))
    throw game_error("Failed to initialize parser!\n");
  else if(!file)
    throw game_error("Failed to open file!\n");

  yaml_parser_set_input_file(&parser, file);

  if (!yaml_parser_load(&parser, &document))
    {
      error << "YAML: ";
      switch (parser.error)
        {
        case YAML_MEMORY_ERROR:
          error << "Memory error: Not enough memory for parsing.\n";
          break;
        case YAML_READER_ERROR:
          if (parser.problem_value != -1) {
              error << "Reader error: "
                    << parser.problem        << ": #"
                    << parser.problem_value  << " at "
                    << parser.problem_offset <<".\n";
            }
          else {
              error << "Reader error: "
                    << parser.problem << " at "
                    << parser.problem_offset << ".\n";
            }
          break;
        case YAML_SCANNER_ERROR:
          error << "Scanner error: ";
          break;
        case YAML_PARSER_ERROR:
          error << "Parser error: ";
          break;
        case YAML_COMPOSER_ERROR:
          error << "Composer error: ";
          break;
        default:
          error << "Internal error.\n";
          break;
        }

      if (parser.error == YAML_SCANNER_ERROR ||
          parser.error == YAML_PARSER_ERROR  ||
          parser.error == YAML_COMPOSER_ERROR )
        {
          if (parser.context)
            error << parser.context << " at line "
                  << parser.context_mark.line+1
                  << ", column " << parser.context_mark.column+1 << "\n"
                  << parser.problem << " at line "
                  << parser.problem_mark.line+1 << ", column "
                  << parser.problem_mark.column+1 << ".\n";

          else
            error << parser.problem <<" at line "
                  << parser.problem_mark.line+1
                  << " , column " << parser.problem_mark.column+1
                  << ".\n";
        }

      yaml_document_delete(&document);
      yaml_parser_delete(&parser);
      fclose(file);

      throw game_error(error.str());
    }
  yaml_parser_delete(&parser);
  fclose(file);

  /* Read YAML config */
  node = yaml_document_get_root_node(&document);

  try {
    if (not (node and node->type == YAML_MAPPING_NODE))
      throw game_error(DEFAULT_PARSE_ERROR);

    for (auto pair = node->data.mapping.pairs.start;
         pair < node->data.mapping.pairs.top; ++pair)
      {
        auto section = yaml_document_get_node(&document, pair->key);
        if (section->type != YAML_SCALAR_NODE) throw game_error(DEFAULT_PARSE_ERROR);
        const char *key = reinterpret_cast<const char *>(section->data.scalar.value);
        parse_yaml(key, yaml_document_get_node(&document, pair->value), &document);
      }

    if (m_player == m_objects.end())
      throw game_error("Player structure doesn't exists.");

    yaml_document_delete(&document);
    return;

  } catch (const game_error &error) {
    yaml_document_delete(&document);
    throw error;
  }
}

void scenario::parse_yaml(const char *section_type, const yaml_node_t *node, yaml_document_t *doc)
{
  if (node->type != YAML_MAPPING_NODE) throw game_error(DEFAULT_PARSE_ERROR);

  for (auto pair = node->data.mapping.pairs.start; pair < node->data.mapping.pairs.top; ++pair)
    {
      auto node_key = yaml_document_get_node(doc, pair->key);
      auto node_value = yaml_document_get_node(doc, pair->value);

      if (node_key->type != YAML_SCALAR_NODE)
        throw game_error(DEFAULT_PARSE_ERROR);

      const char *key = reinterpret_cast<const char *>(node_key->data.scalar.value);

      if (!strcmp(section_type, YAML_SECTION_OBJECTS))
        {
          m_objects.emplace_back(&object::create_from_yaml(key, node_value, doc));
          if (!strcmp(key, DEFAULT_PLAYER_ID))
            m_player = prev(m_objects.end());
        }
      else if (!strcmp(section_type, YAML_SECTION_MAPS))
        m_source.reset(&character_map::create_from_yaml(key, node_value, doc));

      else if (!strcmp(section_type, YAML_SECTION_EVENTS))
        m_events.emplace_back(&event::create_from_yaml(key, node_value, doc));
      else
        throw game_error( string("Found unknown structure \"") + section_type + "\".");

      add_id(key);
    }
}

void scenario::add_id(const string &id)
{
  if (find(m_identifiers.begin(), m_identifiers.end(), id) != m_identifiers.end())
    throw game_error("Found identical identifiers \"" + id + "\".");
  else
    m_identifiers.emplace_back(id);
}

void scenario_create_from_config(const string &f, render_f r_f, int l, int c)
{ single_scenario.reset(new scenario(f, r_f, l, c)); }

void scenario_render()
{ if (single_scenario.get()) single_scenario->render(); }

void scenario_set_view_x(arg_t arg)
{ if (single_scenario.get()) single_scenario->set_view(int(arg), 0); }

void scenario_set_view_y(arg_t arg)
{ if (single_scenario.get()) single_scenario->set_view(0, int(arg)); }

void scenario_move_view_x(arg_t arg)
{ if (single_scenario.get()) single_scenario->move_view(int(arg), 0); }

void scenario_move_view_y(arg_t arg)
{ if (single_scenario.get()) single_scenario->move_view(0, int(arg)); }

void scenario_move_player_x(arg_t arg)
{ if (single_scenario.get()) single_scenario->move_player(int(arg), 0); }

void scenario_move_player_y(arg_t arg)
{ if (single_scenario.get()) single_scenario->move_player(0, int(arg)); }

bool scenario_parse_conditions(const event_conditions &conds, size_t size)
{ return single_scenario.get()? single_scenario->parse_conditions(conds, size) : false; }

void scenario_parse_instructions(const event_instructions& instructions)
{ if (single_scenario.get()) single_scenario->parse_instructions(instructions); }
