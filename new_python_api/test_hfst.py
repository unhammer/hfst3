import libhfst

istr = libhfst.input_stream('foobar.hfst')
try:
    tr1 = libhfst.HfstTransducer(istr)
    tr2 = libhfst.HfstTransducer(istr)
    tr3 = libhfst.HfstTransducer(istr)
except libhfst.EndOfStreamException:
    pass
except:
    print("FAIL")
istr.close()

istr = libhfst.input_stream('foobar.hfst')
try:
    tr1 = istr.read()
    tr2 = istr.read()
    tr3 = istr.read()
except libhfst.EndOfStreamException:
    pass
except:
    print("FAIL")
istr.close()


tr = libhfst.compile_lexc_file('test.lexc')
#print(tr)

tr.insert_freely(tr1).minimize()
#print(tr)
tr.insert_freely(('A','B')).minimize()
#print(tr)


# Possible to implement?
# HfstTransducer & substitute(bool (*func)(const StringPair &sp, hfst::StringPairSet &sps));

# HfstTransducer & substitute(const std::string &old_symbol, const std::string &new_symbol, bool input_side=true, bool output_side=true);
tr = libhfst.regex('a a:b b;')
tr.substitute('a', 'A', input=True, output=False)
eq = libhfst.regex('A:a A:b b;')
print(tr.compare(eq))

tr = libhfst.regex('a a:b b;')
tr.substitute('a', 'A', input=False, output=True)
eq = libhfst.regex('a:A a:b b;')
print(tr.compare(eq))

tr = libhfst.regex('a a:b b;')
tr.substitute('a','A')
eq = libhfst.regex('A A:b b;')
print(tr.compare(eq))

# HfstTransducer & substitute(const StringPair &old_symbol_pair, const StringPair &new_symbol_pair);
tr = libhfst.regex('a a:b b;')
tr.substitute(('a','b'),('A','B'))
eq = libhfst.regex('a A:B b;')
print(tr.compare(eq))

# HfstTransducer & substitute(const StringPair &old_symbol_pair, const hfst::StringPairSet &new_symbol_pair_set);
tr = libhfst.regex('a a:b b;')
tr.substitute(('a','b'),(('A','B'),('B','C'),('C','D'))) # use tuple as an equivalent for std::set
eq = libhfst.regex('a [A:B|B:C|C:D] b;')
print(tr.compare(eq))

tr = libhfst.regex('a a:b b;')
tr.substitute(('a','b'),(('A','B'),('B','C'),('C','D'))) # use list as an equivalent for std::set
eq = libhfst.regex('a [A:B|B:C|C:D] b;')
print(tr.compare(eq))

# HfstTransducer & substitute(const hfst::HfstSymbolSubstitutions &substitutions);
# HfstTransducer & substitute_symbols(const hfst::HfstSymbolSubstitutions &substitutions); // alias for the previous function
tr = libhfst.regex('a a:b b;')
tr.substitute({'a':'A', 'b':'B', 'c':'C'})
eq = libhfst.regex('A A:B B;')
print(tr.compare(eq))

# HfstTransducer & substitute(const hfst::HfstSymbolPairSubstitutions &substitutions);
# HfstTransducer & substitute_symbol_pairs(const hfst::HfstSymbolPairSubstitutions &substitutions); // alias for the previous function
tr = libhfst.regex('a a:b b;')
tr.substitute({('a','a'):('A','a'), ('a','b'):('a','B'), ('c','c'):('C','c')})
eq = libhfst.regex('A:a a:B b;')
print(tr.compare(eq))

# HfstTransducer & substitute(const StringPair &symbol_pair, HfstTransducer &transducer, bool harmonize=true);
tr = libhfst.regex('a a:b b;')
sub = libhfst.regex('[c:d]+;')
tr.substitute(('a','b'),sub)
eq = libhfst.regex('a [c:d]+ b;')
print(tr.compare(eq))

# push weights
tr = libhfst.regex('[a::1 a:b::0.3 b::0]::0.7;')
tr.push_weights(libhfst.TO_INITIAL_STATE)
print(tr)
tr.push_weights(libhfst.TO_FINAL_STATE)
print(tr)

# set final weights
tr = libhfst.regex('(a a:b (b));')
tr.set_final_weights(0.1)
print(tr)
tr.set_final_weights(0.4, True)
print(tr)

# reading and writing in text format
f = open('testfile.att', 'w')
f.write('0 1 foo bar 0.5\n')
f.write('0 1 fo ba 0.2\n')
f.write('0 1 f b 0\n')
f.write('1 2 baz baz\n')
f.write('2 0.1\n')
f.close()

f = libhfst.hfst_open('testfile.att', 'r')
while not f.is_eof():
    TR = libhfst.read_att(f)
    print(TR)
try:
    libhfst.read_att(f)
except libhfst.EndOfStreamException:
    print('Exception caught.')
f.close()

f = libhfst.hfst_stdout()
f.write('-- in ATT format --\n')
TR.write_att(f)
f.write('-- in prolog format --\n')
TR.write_prolog(f, 'FOOBAR_TRANSDUCER')
f.close() # has no effect as f is standard output

# basic transducer
print('Testing basic transducer.')

fsm = libhfst.HfstBasicTransducer()
fsm.add_state(0)
fsm.add_state(1)
fsm.set_final_weight(1, 0.3)
tr = libhfst.HfstBasicTransition(1, 'foo', 'bar', 0.5)
fsm.add_transition(0, tr)
fsm.add_transition(0, 0, 'baz', 'baz')
fsm.add_transition(0, 0, 'baz', 'BAZ', 0.1)
fsm.write_in_att_format(libhfst.hfst_stdout())
print(fsm)

index=0
for state in fsm.states_and_transitions():
    for transition in state:
        print ('%u\t%u\t%s\t%s\t%.2f' % (index, transition.get_target_state(), transition.get_input_symbol(), transition.get_output_symbol(), transition.get_weight()))
#       print (transition)
    if fsm.is_final_state(index):
        print('%s\t%.2f' % (index, fsm.get_final_weight(index)))
    index = index + 1

tr = libhfst.regex('foo:bar::0.5 | foo:baz')


print(tr.lookup('foo', fd='yes', loops=5, output='text'))

print(tr.extract_paths(obey_flags='True', filter_flags='False', max_number=3, foo='bar', output='dict'))

libhfst.dictionary({'FOO':(('BAR',0.5),'BAZ'), 'foo':('bar','baz')})

libhfst.foobar('RGW)T#R"#¤T)"¤TR?B"?¤')

fsm = libhfst.HfstBasicTransducer()
for state in [0,1,2]:
    fsm.add_state(state)
fsm.add_transition(0,1,'foo','bar',1)
fsm.add_transition(0,1,'foo','BAR',2)
fsm.add_transition(1,2,'baz','baz',0)
fsm.set_final_weight(2,0.5)

for state in fsm.states():
    for arc in fsm.transitions(state):
        print('%i ' % (state), end='')
        print(arc)
    if fsm.is_final_state(state):
        print('%i %f' % (state, fsm.get_final_weight(state)) )

for state, arcs in enumerate(fsm):
    for arc in arcs:
        print('%i ' % (state), end='')
        print(arc)
    if fsm.is_final_state(state):
        print('%i %f' % (state, fsm.get_final_weight(state)) )

tr = libhfst.HfstBasicTransducer(libhfst.regex('foo'))
tr.substitute({'foo':'bar'})
tr.substitute({('foo','foo'):('bar','bar')})
