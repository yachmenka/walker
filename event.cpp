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

#include <yaml.h>

#include "scene.hpp"
#include "event.hpp"

static void parse_commands_from_yaml  (const yaml_node_t *node, yaml_document_t *doc, Commands &cmds);
static void parse_conditions_from_yaml(const yaml_node_t *node, yaml_document_t *doc, Conditions &conds);
static void parse_buttons_from_yaml   (const yaml_node_t *node, yaml_document_t *doc, Buttons &butts);
static void parse_string_from_yaml    (const yaml_node_t *node, string &msg);
static void parse_position_from_yaml  (const yaml_node_t *node, W::Position &p);

/* Dynamic alloc */
Event* Event::create_from_yaml(const string &id, const yaml_node_t *node, yaml_document_t *doc, Scenario &scene)
{
    auto event = new Event(id, scene);

    event->m_title = "Event";
    event->m_position = W::small;

    if (!node)
        throw game_error("Empty map structure.");
    else if (node->type != YAML_MAPPING_NODE)
        throw game_error("Invalid map stucture.");

    for (auto b = node->data.mapping.pairs.start; b < node->data.mapping.pairs.top; ++b)
    {
        auto node_key = yaml_document_get_node(doc, b->key);
        auto node_value = yaml_document_get_node(doc, b->value);

        if (node_key->type != YAML_SCALAR_NODE)
            throw game_error("Invalid map structure.");

        const char *key = reinterpret_cast<const char *>(node_key->data.scalar.value);

        if (!strcmp("if", key)) {
            parse_conditions_from_yaml(node_value, doc, event->m_conditions);

        } else if (!strcmp("title", key)) {
            parse_string_from_yaml(node_value, event->m_title);

        } else if (!strcmp("size", key)) {
            parse_position_from_yaml(node_value, event->m_position);

        } else if (!strcmp("message", key)) {
            parse_string_from_yaml(node_value, event->m_message);

        } else if (!strcmp("buttons", key)) {
            parse_buttons_from_yaml(node_value, doc, event->m_buttons);

        } else if (!strcmp("do", key)) {
            parse_commands_from_yaml(node_value, doc, event->m_commands);
        } else
            throw game_error( string("Invalid field in event structure: \"") + key + "\"");
    }

    return event;
}

void Event::test()
{
    if (m_scenario.parse_conditions(m_conditions)) {
        if (m_message.empty()) {
            m_scenario.parse_commands(m_commands);
        } else {
            vector<W::Menu> menu;

            for (auto &b : m_buttons) {
                b.set_event(this);
                menu.emplace_back( W::Menu(b.get_label(), ActionAV(push_button, Arg(&b))));
            }

            auto wptr = W::push( W::Builder(m_position, menu, hooks.at(Hooks::event_dialog), m_message, m_title) );

            /* While wptr is exist, menu must not be destroyed */
            while (W::has(wptr))
                W::hook();
        }
    }
}

void Event::push_button(Arg button_ptr)
{
    auto button = reinterpret_cast<Button *>(button_ptr);

    /* Execute commands assigned to the button */
    button->get_event()->m_scenario.parse_commands(button->get_commands());
}

void parse_commands_from_yaml(const yaml_node_t *node, yaml_document_t *doc, Commands &cmds)
{
    if (node->type != YAML_SEQUENCE_NODE)
        throw game_error("Incorrectly set \"do\" sequence in the event structure.");

    for (auto seq = node->data.sequence.items.start; seq < node->data.sequence.items.top; ++seq) {
        auto seq_value = yaml_document_get_node(doc, *seq);
        if (seq_value->type != YAML_SCALAR_NODE)
            throw game_error("Incorrectly set \"do\" sequence in the event structure.");

        const char *value = reinterpret_cast<const char *>(seq_value->data.scalar.value);
        cmds.emplace_back( value );
    }
}

void parse_conditions_from_yaml(const yaml_node_t *node, yaml_document_t *doc, Conditions &conds)
{
    if (node->type != YAML_SEQUENCE_NODE)
        throw game_error("Incorrectly set \"if\" sequence in the event structure.");

    for (auto seq = node->data.sequence.items.start; seq < node->data.sequence.items.top; ++seq) {
        auto seq_value = yaml_document_get_node(doc, *seq);
        if (seq_value->type != YAML_SCALAR_NODE)
            throw game_error("Incorrectly set \"if\" sequence in the event structure.");

        const char *value = reinterpret_cast<const char *>(seq_value->data.scalar.value);
        conds.emplace_back( value );
    }
}

void parse_buttons_from_yaml(const yaml_node_t *node, yaml_document_t *doc, Buttons &butts)
{
    if (node->type != YAML_SEQUENCE_NODE)
        throw game_error("Incorrectly set \"press\" struct in the event structure.");

    for (auto b = node->data.sequence.items.start; b < node->data.sequence.items.top; ++b)
    {
        auto seq_value = yaml_document_get_node(doc, *b);

        if (seq_value->type != YAML_MAPPING_NODE)
            throw game_error("Incorrectly set \"press\" struct in the event structure.");

        butts.push_back(Button());
        auto &butt = butts.back();

        for (auto b = seq_value->data.mapping.pairs.start; b < seq_value->data.mapping.pairs.top; ++b)
        {
            auto node_key = yaml_document_get_node(doc, b->key);
            auto node_value = yaml_document_get_node(doc, b->value);

            if (node_key->type != YAML_SCALAR_NODE)
                throw game_error("Incorrectly set \"press\" struct in the event structure.");

             const char *key = reinterpret_cast<const char *>(node_key->data.scalar.value);

            if (!strcmp("label", key)) {
                parse_string_from_yaml(node_value, butt.get_label());
            } else if (!strcmp("do", key)) {
                parse_commands_from_yaml(node_value, doc, butt.get_commands());
            } else
                throw game_error( string("Invalid field in event structure: \"") + key + "\"");
        }
    }
}

static void parse_string_from_yaml(const yaml_node_t *node, string &msg)
{
    if (node->type != YAML_SCALAR_NODE)
        throw game_error("Incorrectly field in the event structure.");

    const char *value = reinterpret_cast<const char *>(node->data.scalar.value);
    msg = value;
}

static void parse_position_from_yaml(const yaml_node_t *node, W::Position &p)
{
    if (node->type != YAML_SCALAR_NODE)
        throw game_error("Incorrectly \"position\" field in the event structure.");

   const char *value = reinterpret_cast<const char *>(node->data.scalar.value);

   if (!strcmp(value, "small")) {
       p = W::small;
   } else if (!strcmp(value, "average")) {
       p = W::aver;
   } else if (!strcmp(value, "full")) {
       p = W::full;
   } else
       throw game_error(string("Invalid position value \"") + value + "\" in the event structure.");
}
