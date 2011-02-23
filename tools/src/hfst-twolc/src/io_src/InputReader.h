//! @file InputReader.h
//!
//! @author Miikka Silfverberg
//! 
//! @note This file is based on the tutorial "Better error handling using
//! Flex and Bison", by Christian Hagen. The
//! <a href=
//! "http://www.ibm.com/developerworks/opensource/library/l-flexbison.html"> 
//! tutorial</a>.
//!
//! @brief Class for Flex and Bison input reading.

//   This program is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, version 3 of the Licence.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef INPUT_READER_H
#define INPUT_READER_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <iostream>
#include <string>
#include <cstdlib>

//! @brief Throw when an @a InputStream is used without first setting its
//! @a std::istream.
class InputNotSet
{};

//! @brief class For reading input from an @a std::istream one character at a
//! time.
//!
//! The class is also used for displaying error messages and warnings that 
//! point to the position of the error on the input line.
class InputReader
{
private:
  std::istream * input_stream;
  size_t &counter;
  char buffer[500000];
  size_t buffer_size;
  size_t buffer_index;

public:
  //! @brief Initialize a reader with line counter @a counter.
  //!
  //! @param counter would usually be a reference to @a yylineno which is 
  //! maintained by Bison.
  InputReader(size_t &counter);

  //! @brief Set the input stream from which the twolc-grmmar is read.
  void set_input(std::istream &file);

  //! @brief Display the warning message @a warning.
  void warn(const std::string &warning);

  //! @brief Display the error message @a err and exit with signal 1.
  void error(const std::string &err);
  
  //! @brief Read next input char.
  //!
  //! @return The next input character or 0 if end of file was reached.
  char input(void);
};

#endif // INPUT_READER_H
