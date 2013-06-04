// Based on Mans Hulden's foma.
help["regex <regex>"] = "read a regular expression";
shortcmd["regex"].insert("regex <regex>");

help["ambiguous upper"] = "returns the input words which have multiple paths in a transducer";
shortcmd["ambiguous"].insert("ambiguous upper");

help["apply up <string>"] = "apply <string> up to the top network on stack";
help["apply down <string>"] = "apply <string> down to the top network on stack";
help["apply med <string>"] = "find approximate matches to string in top network by minimum edit distance";
help["apply up"] = "enter apply up mode (Ctrl-D exits)";
help["apply down"] = "enter apply down mode (Ctrl-D exits)";
help["apply med"] = "enter apply med mode (Ctrl-D exits)";
shortcmd["apply"].insert("apply up <string>");
shortcmd["apply"].insert("apply down <string>");
shortcmd["apply"].insert("apply med <string>");

help["apropos <string>"] = "search help for <string>";
help["clear stack"] = "clears the stack";
help["compact sigma"] = "removes redundant symbols from FSM";
help["complete net"] = "completes the FSM";
help["compose net"] = "composes networks on stack";
help["concatenate"] = "concatenates networks on stack";
help["crossproduct net"] = "cross-product of top two FSMs on stack";
help["define <name> <r.e.>"] = "define a network";
help["define <fname>(<v1,..,vn>) <r.e.>"] = "define function";
help["determinize net"] = "determinizes top FSM on stack";
help["echo <string>"] = "echo a string";
help["eliminate flag <name>"] = "eliminate flag <name> diacritics from the top network";
help["eliminate flags"] = "eliminate all flag diacritics from the top network";
help["export cmatrix (filename)"] = "export the confusion matrix as an AT&T transducer";
help["extract ambiguous"] = "extracts the ambiguous paths of a transducer";
help["extract unambiguous"] = "extracts the unambiguous paths of a transducer";
help["help"] = "lists all commands";
help["help <name>"] = "prints help message of a command";
help["help license"] = "prints license";
help["help warranty"] = "prints warranty information";
help["ignore net"] = "applies ignore to top two FSMs on stack";
help["intersect net"] = "intersects FSMs on stack";
help["invert net"] = "inverts top FSM";
help["label net"] = "extracts all attested symbol pairs from FSM";
help["letter machine"] = "Converts top FSM to a letter machine";
help["load stack <filename>"] = "Loads networks and pushes them on the stack";
help["load defined <filename>"] = "Restores defined networks from file";
help["lower-side net"] = "takes lower projection of top FSM";
help["minimize net"] = "minimizes top FSM";
help["name net <string>"] = "names top FSM";
help["negate net"] = "complements top FSM";
help["one-plus net"] = "Kleene plus on top FSM";
help["pop stack"] = "remove top FSM from stack";
help["print cmatrix"] = "prints the confusion matrix associated with the top network in tabular format";
help["print defined"] = "prints defined symbols and functions";
help["print dot (>filename)"] = "prints top FSM in Graphviz dot format";
help["print lower-words"] = "prints words on the lower-side of top FSM";
help["print name"] = "prints the name of the top FSM";
help["print net"] = "prints all information about top FSM";
help["print random-lower"] = "prints random words from lower side";
help["print random-upper"] = "prints random words from upper side";
help["print random-words"] = "prints random words from top FSM";
help["print sigma"] = "prints the alphabet of the top FSM";
help["print size"] = "prints size information about top FSM";
help["print shortest-string"] = "prints the shortest string of the top FSM";
help["print shortest-string-size"] = "prints length of shortest string";
help["prune net"] = "makes top network coaccessible";
help["push (defined) <name>"] = "adds a defined FSM to top of stack";
help["quit"] = "exit foma";
help["read att <filename>"] = "read a file in AT&T FSM format and add to top of stack";
help["read cmatrix <filename>"] = "read a confusion matrix and associate it with the network on top of the stack";
help["read prolog <filename>"] = "reads prolog format file";
help["read lexc <filename>"] = "read and compile lexc format file";
help["read spaced-text <filename>"] = "compile space-separated words/word-pairs separated by newlines into a FST";
help["read text <filename>"] = "compile a list of words separated by newlines into an automaton";
help["reverse net"] = "reverses top FSM";
help["rotate stack"] = "rotates stack";
help["save defined <filename>"] = "save all defined networks to binary file";
help["save stack <filename>"] = "save stack to binary file";
help["set <variable> <ON|OFF>"] = "sets a global variable (see show variables)";
help["show variables"] = "prints all variable/value pairs";
help["shuffle net"] = "asynchronous product on top two FSMs on stack";
help["sigma net"] = "Extracts the alphabet and creates a FSM that accepts all single symbols in it";
help["source <file>"] = "read and compile script file";
help["sort net"] = "sorts arcs topologically on top FSM";
help["sort in"] = "sorts input arcs by sigma numbers on top FSM";
help["sort out"] = "sorts output arcs by sigma number on top FSM";
help["substitute defined X for Y"] = "substitutes defined network X at all arcs containing Y ";
help["substitute symbol X for Y"] = "substitutes all occurrences of Y in an arc with X";
help["system <cmd>"] = "execute a system command";
help["test unambiguous"] = "test if top FST is unambiguous";
help["test equivalent"] = "test if the top two FSMs are equivalent";
help["test functional"] = "test if the top FST is functional (single-valued)";
help["test identity"] = "test if top FST represents identity relations only";
help["test lower-universal"] = "test if lower side is Σ*";
help["test upper-universal"] = "test if upper side is Σ*";
help["test non-null"] = "test if top machine is not the empty language";
help["test null"] = "test if top machine is the empty language";
help["test sequential"] = "tests if top machine is sequential";
help["test star-free"] = "test if top FSM is star-free";
help["turn stack"] = "turns stack upside down";
help["twosided flag-diacritics"] = "changes flags to always be identity pairs";
help["undefine <name>"] = "remove <name> from defined networks";
help["union net"] = "union of top two FSMs";
help["upper-side net"] = "upper projection of top FSM";
help["view net"] = "display top network (if supported)";
help["zero-plus net"] = "Kleene star on top fsm";
help["variable compose-tristate"] = "use the tristate composition algorithm";
help["variable show-flags"] = "show flag diacritics in `apply'";
help["variable obey-flags"] = "obey flag diacritics in `apply'";
help["variable minimal"] = "minimize resulting FSMs";
help["variable print-pairs"] = "always print both sides when applying";
help["variable print-space"] = "print spaces between symbols";
help["variable print-sigma"] = "print the alphabet when printing network";
help["quit-on-fail"] = "Abort operations when encountering errors";
help["variable recursive-define"] = "Allow recursive definitions";
help["variable verbose"] = "Verbosity of interface";
help["variable hopcroft-min"] = "ON = Hopcroft minimization, OFF = Brzozowski minimization";
help["variable med-limit"] = "the limit on number of matches in apply med";
help["variable med-cutoff"] = "the cost limit for terminating a search in apply med";
help["variable att-epsilon"] = "the EPSILON symbol when reading/writing AT&T files";
help["write prolog (> filename)"] = "writes top network to prolog format file/stdout";
help["write att (> <filename>)"] = "writes top network to AT&T format file/stdout";
