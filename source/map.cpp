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

#include <map>
#include <fstream>
#include <yaml.h>

#include "scenario_constants.hpp"
#include "map.hpp"
#include "utils.hpp"
#include "perlin.hpp"

#define FILE_GENERATE "Generation.txt"

using std::map;
using std::ofstream;
using std::ifstream;
using std::to_string;

/* Generation (count).txt */
static string nextgen(const string& s, int count);

static char textures[] = { '~', '#', '\'', '`' };
static map<char, attr_t> map_attrs = {
  {'~',  PAIR(COLOR_BLUE, COLOR_BLACK)  | DEFAULT_TILE_ATTRIBUTE},
  {'#',  PAIR(COLOR_WHITE, COLOR_BLACK) | DEFAULT_TILE_ATTRIBUTE},
  {'\'', PAIR(COLOR_GREEN, COLOR_BLACK) | DEFAULT_TILE_ATTRIBUTE},
  {'`',  PAIR(COLOR_GREEN, COLOR_BLACK) | DEFAULT_TILE_ATTRIBUTE},
  {'.',  PAIR(COLOR_CYAN, COLOR_BLACK)  | DEFAULT_TILE_ATTRIBUTE},
  {'(',  PAIR(COLOR_RED, COLOR_BLACK)   | DEFAULT_TILE_ATTRIBUTE},
  {')',  PAIR(COLOR_RED, COLOR_BLACK)   | DEFAULT_TILE_ATTRIBUTE},
};

void character_map::push(const string &s)
{
  if (s.empty())
    throw game_error("Line " + to_string(m_height + 1)
                     + " is empty.");

  int slen = int(s.length());

  if (m_width == 0)
    m_width = slen;

  else if (m_width != slen)
    throw game_error("The lenght of line number " + to_string(m_height + 1) +
                     " (" + to_string(slen) +
                     ") does not match the specified length (" +
                     to_string(m_width) + ").");
  ++m_height;
  m_lines.emplace_back(s);
}

character_map::character_map(const string &id, const string &map, int w, int h) : base(id)
{
  m_x = 0;
  m_y = 0;
  m_height = 0;
  m_width = w;

  for (string::size_type pos = 0, newline = 0; h--; pos = (newline + 1) )
    {
      newline = map.find('\n', pos);
      string &&temp = map.substr(pos, newline - pos);
      this->push(temp);
    }

  this->decorate();
}

character_map& character_map::create_from_yaml(const string &id, const yaml_node_t *node, yaml_document_t *doc)
{
  string text;
  int w = 0;
  int h = 0;

  if (!node)
    throw game_error("Empty map structure.");
  else if (node->type != YAML_MAPPING_NODE)
    throw game_error("Invalid map stucture.");

  for (auto b = node->data.mapping.pairs.start; b < node->data.mapping.pairs.top; ++b)
    {
      auto node_key = yaml_document_get_node(doc, b->key);
      auto node_value = yaml_document_get_node(doc, b->value);

      if (node_key->type != YAML_SCALAR_NODE or node_value->type != YAML_SCALAR_NODE)
        throw game_error("Invalid map structure.");

      const char *key = reinterpret_cast<const char *>(node_key->data.scalar.value);
      const char *value = reinterpret_cast<const char *>(node_value->data.scalar.value);

      if (!strcmp(key, YAML_MAP_WIDTH))
        w = atoi(value);

      else if (!strcmp(key, YAML_MAP_HEIGHT))
        h = atoi(value);

      else if (!strcmp(key, YAML_MAP_TEXT))
        text = value;
      else
        throw game_error( string("Found unknown field \"") + key + "\" in the map structure.");
    }

  return *new character_map(id, text, w, h);
}

void character_map::decorate()
{
  for (auto& line : m_lines) for (size_t i = 0; i < line.lenght; ++i)
    {
      auto attr = map_attrs.find(line.cstr[i].symbol);
      if (attr != map_attrs.end())
        line.cstr[i].attribute = attr->second;
      else
        line.cstr[i].attribute = DEFAULT_TILE_ATTRIBUTE;
    }
}

void character_map::generate(const string& f, int w, int h)
{
  srand(unsigned(time(nullptr)));
  perlin_set_seed(rand());

  ofstream fil;

  fil.exceptions(std::ios_base::failbit);
  try {
    fil.open(f);

    for (int y = 0; y < h; ++y)
      {
        for (int x = 0; x < w; ++x)
          {
            auto seed = unsigned(perlin2d(x, y, 0.05f, 10) * 10);
            fil << textures[seed % SIZE(textures)];
          }
        fil << '\n';
      }
  } catch (std::ios::failure &) {
    if (!fil.is_open()) throw game_error("Can't create file \"" + f + "\".");
    else throw game_error("Something went wrong when writing to " + f);
  }
}

string character_map::generate(int w, int h)
{
  string folder = CONFIG + DIR_GENERATIONS;

  ifstream fil;
  int count = 0;
  string filename = FILE_GENERATE;

  do {
      fil.close();
      if (count > 0)
        filename = nextgen(FILE_GENERATE, count);
      fil.open(folder + filename);
      ++count;
    } while (fil.is_open());

  fil.close();
  string f = folder + filename;

  generate(f, w, h);
  return f;
}

string nextgen(const string& str, int count)
{
  string s = str;
  s.insert(s.rfind('.'), " (" + to_string(count) + ")");
  return s;
}
