//! @file lexc-utils.cc
//!
//! @brief Implementation of some string handling in HFST lexc.
//!
//! @author Tommi A. Pirinen


//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, version 3 of the License.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string>
#include <vector>

#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cstdio>

#include "lexc-utils.h"
#include "lexc-parser.h"

using std::string;

// flex stuffa
extern int hlexclineno;
static char* hlexcfilename = 0;
extern char* hlexctext;
extern YYLTYPE hlexclloc;

namespace hfst { namespace lexc {
// string mangling
static
string&
replace_all(string& haystack, const string& needle, const string& replacement)
{
        size_t last_needle = haystack.find(needle);
        size_t needle_len = needle.length();
        while(last_needle != string::npos)
        {
                haystack.replace(last_needle, needle_len, replacement);
                last_needle = haystack.find(needle);
        }
        return haystack;
}


string&
stripPercents(string& s)
{
	string& stripped = s;
	stripped = replace_all(stripped, "%%", "@PERCENT@");
	stripped = replace_all(stripped, "%", "");
	stripped = replace_all(stripped, "@PERCENT@", "%");
	return stripped;
}

string&
addPercents(string& s)
{
	string& added = s;
	added = replace_all(added, "%", "%%");
	added = replace_all(added, "<", "%<");
	added = replace_all(added, ">", "%>");
	return added;
}

string&
joinerEncode(string& s)
{
	string& lxs = s;
	lxs = lxs.insert(0, LEXC_JOINER_START);
	lxs = lxs.append(LEXC_JOINER_END);
	return lxs;
}

string&
joinerDecode(string& s)
{
	assert(s.length() >= 4);
	string& decoded = s;
	size_t jStart = strlen(LEXC_JOINER_START);
	size_t jEnd = strlen(LEXC_JOINER_END);
	decoded = decoded.substr(jStart, (s.length() - (jStart + jEnd)));
	return decoded;
}

string&
xreDefinitionEncode(string& s)
{
	string& lxs = s;
	lxs = lxs.insert(0, LEXC_DFN_START);
	lxs = lxs.append(LEXC_DFN_END);
	return lxs;
}

// FLEX stuff



static
size_t
count_newlines(const char *linestring)
{
	size_t linecount = 0;
	char *linep = (char*)linestring;
	while(*linep != '\0')
	{
		if(*linep == '\n')
		{
			linecount++;
		}
		linep++;
	}
	return linecount;
}

void
set_infile_name(const char* s)
{
	if (hlexcfilename != 0)
	{
		free(hlexcfilename);
	}
	hlexcfilename = strdup(s);
}

void
token_reset_positions()
{
	hlexclloc.first_line = hlexclloc.last_line = 1;
	hlexclloc.first_column = hlexclloc.last_column = 1;
	hlexclineno = 1;
	if (hlexcfilename != 0)
	{
		free(hlexcfilename);
	}
	hlexcfilename = 0;
}

void
token_update_positions(const char *token)
{
	size_t token_length = strlen(token);
	int newlines = count_newlines(token);
	hlexclloc.first_line = hlexclineno;
	hlexclloc.last_line = (hlexclineno + newlines);
	// FIXME: columns equal bytes not characters
	hlexclloc.first_column = hlexclloc.last_column + 1;
	if (0 == newlines)
	{
		hlexclloc.last_column = (hlexclloc.first_column + token_length);
	}
	else
	{
		char *token_last_line_start = rindex((char*) token, '\n');
		char *token_end = rindex((char*) token, '\0');
		hlexclloc.last_column = (token_end - token_last_line_start) - 1;
	}
}

char*
strdup_token_positions()
{
	// N.B. reason for this error format is automagic support by vim/emacs/jedit
	// must be “filename:lineno:colno-lineno:colno: stuff”
	// c.f. http://www.gnu.org/prep/standards/standards.html#Errors
	char* filenames_lines_cols = (char*)malloc(sizeof(char) * 
			(strlen(hlexcfilename) + 100));
	// linenumbers and columns
	if( (hlexclloc.first_line == hlexclloc.last_line) && 
		(hlexclloc.first_column == (hlexclloc.last_column - 1)) )
	{
		// TRANSLATORS: filename, line and column
		(void)sprintf(filenames_lines_cols, "%s:%d.%d", 
			hlexcfilename, hlexclloc.first_line, hlexclloc.first_column);
	}
	else if ( hlexclloc.first_line == hlexclloc.last_line )
	{
		// TRANSLATORS: filename, line, column to column
		(void)sprintf(filenames_lines_cols, "%s:%d.%d-%d",
			hlexcfilename,
			hlexclloc.first_line, hlexclloc.first_column,
			hlexclloc.last_column);
	}
	else
	{
		// TRANSLATORS: filename, line-column to line-column
		(void)sprintf(filenames_lines_cols, "%s:%d.%d-%d.%d",
			hlexcfilename,
			hlexclloc.first_line, hlexclloc.first_column,
			hlexclloc.last_line, hlexclloc.last_column);
	}
	return filenames_lines_cols;
}

char*
strdup_token_part()
{
	char *error_token = (char*)malloc(sizeof(char)*strlen(hlexctext)+100);
	char* maybelbr = strchr(hlexctext, '\n');
	if (maybelbr != NULL)
	{
		char* beforelbr = (char*)malloc(sizeof(char)*strlen(hlexctext)+1);
		(void)memcpy(beforelbr, hlexctext, maybelbr-hlexctext);
		beforelbr[maybelbr-hlexctext] = '\0';
		(void)sprintf(error_token, "[near: `%s\\n']", beforelbr);
		free(beforelbr);
	}
	else if (strlen(hlexctext) < 80)
	{
		(void)sprintf(error_token, "[near: `%s']", hlexctext);
	}
	else
	{
		(void)sprintf(error_token, "[near: `%30s...' (truncated)]",
					  hlexctext);
	}
	return error_token;
}

char*
strip_percents(const char* s, bool do_zeros)
{
	char* rv = (char*)malloc(sizeof(char)*strlen(s)*6+1);
	char* p = rv;
	const char* c = s;
	bool escaping = false;
	while (*c != '\0')
	{
		if (escaping)
		{
			if (*c != '0')
			{
				*p = *c;
			}
			else
			{
				*p = '@';
				p++;
				*p = 'Z';
				p++;
				*p = 'E';
				p++;
				*p = 'R';
				p++;
				*p = 'O';
				p++;
				*p = '@';
			}
			escaping = false;
			++p;
			++c;
		}
		else if (*c == '%')
		{
			escaping = true;
			++c;
		}
		else if (do_zeros && (*c == '0'))
		{
			*p = '@';
			p++;
			*p = '0';
			p++;
			*p = '@';
			p++;
			c++;
		}
		else
		{
			*p = *c;
			++p;
			++c;
		}
	}
	*p = '\0';
	if (escaping)
	{
		fprintf(stderr, "Stray escape char %% in %s\n", s);
		return NULL;
	}
	return rv;
}

char*
strstrip(const char* s)
{
	char* rv = (char*)malloc(sizeof(char)*strlen(s)+1);
	char* p = rv;
	while (isspace(*s))
	{
		++s;
	}
	while (*s != '\0')
	{
		*p = *s;
		++p;
		++s;
	}
	*p = '\0';
	--p;
	while (isspace(*p))
	{
		*p = '\0';
		--p;
	}
	return rv;
}

char*
strdup_nonconst_part(const char* token, const char* prefix,
						  const char* suffix, bool strip)
{
	size_t prefix_len = 0;
	size_t suffix_len = 0;
	size_t varpart_len = 0;
	size_t token_len = strlen(token);
	char* token_part = (char*)malloc(sizeof(char) * token_len + 1);
	if (prefix)
	{
		prefix_len = strlen(prefix);
	}
	if (suffix)
	{
		suffix_len = strlen(suffix);
	}
	varpart_len = strlen(token) - prefix_len - suffix_len;
	assert(varpart_len <= token_len);
	assert(strncmp(token, prefix, prefix_len) == 0);
	assert(strncmp(token + prefix_len + varpart_len, suffix, suffix_len) == 0);
	(void)memcpy(token_part, token + prefix_len, varpart_len);
	token_part[varpart_len] = '\0';
	if (strip)
	{
		char* tmp = strstrip(token_part);
		free(token_part);
		token_part = tmp;
	}
	return token_part;
}


} } 

// vim: set ft=cpp.doxygen:

