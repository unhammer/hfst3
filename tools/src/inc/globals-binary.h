//       This program is free software: you can redistribute it and/or modify
//       it under the terms of the GNU General Public License as published by
//       the Free Software Foundation, version 3 of the License.
//
//       This program is distributed in the hope that it will be useful,
//       but WITHOUT ANY WARRANTY; without even the implied warranty of
//       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//       GNU General Public License for more details.
//
//       You should have received a copy of the GNU General Public License
//       along with this program.  If not, see <http://www.gnu.org/licenses/>.


  // input transducers
  char *firstfilename = 0;
  FILE *firstfile = 0;
  bool firstNamed = false;
  char *secondfilename = 0;
  FILE *secondfile = 0;
  bool secondNamed = false;
  bool is_input_stdin = true;
  // result transducer
  char *outfilename = 0;
  FILE *outfile = 0;
  bool is_output_stdout = true;
  
