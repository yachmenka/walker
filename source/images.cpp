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

#include "images.hpp"
#include "utils.hpp"
#include <fstream>

static text cow =
    "^__^                \n"
    "(oo)\\_______       \n"
    "(__)\\       )\\/\\ \n"
    "    ||----w |       \n"
    "    ||     ||       \n";

static text centaur =
    "           ,~,       \n"
    "          (((}       \n"
    "          -''-.      \n"
    "         (\\  /\\)   \n"
    "   ~______\\) | `\\  \n"
    "~~~(         |  ')   \n"
    "   | )____(  |       \n"
    "   /|/     ` /|      \n"
    "   \\ \\      / |    \n"
    "    |\\|\\   /| |\\  \n";

static text horseback_fight =
    "        y             \n"
    "       /`'            \n"
    "       \\ O ,         \n"
    "        |,/(\\   /    \n"
    "  -`___-\\  |` ./O    \n"
    "  ''-(  /`--) `\\/\\  \n"
    "     7/`       /|     \n"
    "     \\       /  \\   \n";

static text mountains =
text(
    "             _,_           ")+text("+", A_BLINK)+"                   __\n"
    "             ','                  /\\          `. `.               \n"
    "       .                        .'  \\    +      \"  |             \n"
    "                               /     \\         /  .'         .    \n"
    "                    .'\\      .'       \\       `\"`               \n"
    "   +             .-'   `.   /          `.                          \n"
    "         .     .'        \\.'             \\                       \n"
    "            .-'           \\               \\   .-`\"`-.      . +  \n"
    "        .'.'               \\               \\.'       `-.         \n"
    "      /                    `.           .-'\\           `-._       \n"
    "     .'                       \\       .-'                   `-.   \n"
    "                                                               `-. \n"
    ".-------------------'''''''''''''''              _.--      .'      \n"
    "                             ___..         _.--''        .'        \n"
    "                       --''''             '            .'          \n"
    "                                                                   \n";

static text pikeman =
    "   ,   A           {}                 \n"
    "  / \\, | ,        .--.               \n"
    " |    =|= >      /.--.\\              \n"
    "  \\ /` | `       |====|              \n"
    "   `   |         |`::`|               \n"
    "       |     .-;`\\..../`;_.-^-._     \n"
    "      /\\\\/  /  |...::..|`   :   `|  \n"
    "      |:'\\ |   /'''::''|   .:.   |   \n"
    "       \\ /\\;-,/\\   ::  |..:::::..| \n"
    "       |\\ <` >  >._::_.| ':::::' |   \n"
    "       | `\"\"`  /   ^^  |   ':'   |  \n"
    "       |       |       \\    :    /   \n"
    "       |       |        \\   :   /    \n"
    "       |       |___/\\___|`-.:.-`     \n"
    "       |        \\_ || _/    `        \n"
    "       |        <_ >< _>              \n"
    "       |        |  ||  |              \n"
    "       |        |  ||  |              \n"
    "       |       _\\.:||:./_            \n"
    "       |      /____/\\____\\          \n";

static text scroll_and_ink_pen =
    "(\\                           \n"
    " \'\\                         \n"
    "  \'\\     __________         \n"
    "  / '|   ()_________)         \n"
    "  \\ '/    \\ ~~~~~~~~ \\     \n"
    "    \\       \\ ~~~~~~   \\   \n"
    "    ==).      \\__________\\  \n"
    "   (__)       ()__________)   \n";

static text open_book =
    "      ______ ______       \n"
    "    _/      Y      \\_    \n"
    "   // ~~ ~~ | ~~ ~  \\\\  \n"
    "  // ~ ~ ~~ | ~~~ ~~ \\\\ \n"
    " //________.|.________\\\\\n"
    "`----------`-'----------' \n";

const text *images[] =
{
  &cow,
  &centaur,
  &horseback_fight,
  &mountains,
  &pikeman,
  &scroll_and_ink_pen,
  &open_book,
};

using std::fstream;

text images_find(const char *image)
{
  string path = CONFIG + DIR_SCENARIOS + image;

  fstream f(path);

  if (!f.good())
    throw game_error(string("We can't open image file \"") + image + "\".");

  string im;
  size_t max_line = 0;
  while (f.good())
    {
      string temp;
      getline(f, temp);
      im += temp;
      im += '\n';
      if (temp.size() > max_line) max_line = temp.size();
    }

  return im;
}
