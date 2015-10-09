import libhfst
import os.path
from inspect import currentframe

def get_linenumber():
    cf = currentframe()
    return cf.f_back.f_lineno

for type in (libhfst.TROPICAL_OPENFST_TYPE, libhfst.FOMA_TYPE):

    print('\n--- Testing implementation type %s ---\n' % libhfst.fst_type_to_string(type))

    libhfst.set_default_fst_type(type)

    tr1 = None
    tr2 = None
    tr3 = None

    if not os.path.isfile('foobar.hfst'):
        raise RuntimeError('Missing file: foobar.hfst')

    istr = libhfst.HfstInputStream('foobar.hfst')
    numtr = 0
    try:
        tr1 = istr.read()
        numtr += 1
        tr2 = istr.read()
        numtr += 1
        tr3 = istr.read()
        numtr += 1
    except libhfst.EndOfStreamException:
        pass
    except:
        raise RuntimeError(get_linenumber())
    istr.close()

    if numtr != 2:
        raise RuntimeError(get_linenumber())

    tr1.convert(libhfst.get_default_fst_type())
    tr2.convert(libhfst.get_default_fst_type())

    ostr = libhfst.HfstOutputStream(filename='foobar2.hfst')
    ostr.write(tr1)
    ostr.write(tr2)
    ostr.flush()
    ostr.close()

    TR1 = None
    TR2 = None
    TR3 = None

    istr = libhfst.HfstInputStream('foobar2.hfst')
    numtr = 0
    try:
        TR1 = istr.read()
        numtr += 1
        TR2 = istr.read()
        numtr += 1
        TR3 = istr.read()
        numtr += 1
    except libhfst.EndOfStreamException:
        pass
    except:
        raise RuntimeError(get_linenumber())
    istr.close()

    if numtr != 2:
        raise RuntimeError(get_linenumber())
    
    if not (TR1.compare(tr1)):
        raise RuntimeError(get_linenumber())
    if not (TR2.compare(tr2)):
        raise RuntimeError(get_linenumber())

    # Copy constructor
    transducer = libhfst.HfstTransducer(TR1)
    if not (TR1.compare(transducer)):
        raise RuntimeError(get_linenumber())
    if not (transducer.compare(TR1)):
        raise RuntimeError(get_linenumber())

    # Read lexc
    tr = libhfst.compile_lexc_file('test.lexc')
    tr.insert_freely(tr1).minimize()
    tr.insert_freely(('A','B')).minimize()

    # Substitute
    tr = libhfst.regex('a a:b b;')
    tr.substitute('a', 'A', input=True, output=False)
    eq = libhfst.regex('A:a A:b b;')
    if not (tr.compare(eq)):
        raise RuntimeError(get_linenumber())

    tr = libhfst.regex('a a:b b;')
    tr.substitute('a', 'A', input=False, output=True)
    eq = libhfst.regex('a:A a:b b;')
    if not (tr.compare(eq)):
        raise RuntimeError(get_linenumber())

    tr = libhfst.regex('a a:b b;')
    tr.substitute('a','A')
    eq = libhfst.regex('A A:b b;')
    if not (tr.compare(eq)):
        raise RuntimeError(get_linenumber())

    tr = libhfst.regex('a a:b b;')
    tr.substitute(('a','b'),('A','B'))
    eq = libhfst.regex('a A:B b;')
    if not (tr.compare(eq)):
        raise RuntimeError(get_linenumber())

    tr = libhfst.regex('a a:b b;')
    tr.substitute(('a','b'),(('A','B'),('B','C'),('C','D')))
    eq = libhfst.regex('a [A:B|B:C|C:D] b;')
    if not (tr.compare(eq)):
        raise RuntimeError(get_linenumber())

    tr = libhfst.regex('a a:b b;')
    tr.substitute(('a','b'),(('A','B'),('B','C'),('C','D')))
    eq = libhfst.regex('a [A:B|B:C|C:D] b;')
    if not (tr.compare(eq)):
        raise RuntimeError(get_linenumber())

    tr = libhfst.regex('a a:b b;')
    tr.substitute({'a':'A', 'b':'B', 'c':'C'})
    eq = libhfst.regex('A A:B B;')
    if not (tr.compare(eq)):
        raise RuntimeError(get_linenumber())

    tr = libhfst.regex('a a:b b;')
    tr.substitute({('a','a'):('A','a'), ('a','b'):('a','B'), ('c','c'):('C','c')})
    eq = libhfst.regex('A:a a:B b;')
    if not (tr.compare(eq)):
        raise RuntimeError(get_linenumber())

    tr = libhfst.regex('a a:b b;')
    sub = libhfst.regex('[c:d]+;')
    tr.substitute(('a','b'),sub)
    eq = libhfst.regex('a [c:d]+ b;')
    if not (tr.compare(eq)):
        raise RuntimeError(get_linenumber())

    # push weights
    tr = libhfst.regex('[a::1 a:b::0.3 b::0]::0.7;')
    tr.push_weights(libhfst.TO_INITIAL_STATE)
    tr.push_weights(libhfst.TO_FINAL_STATE)

    # set final weights
    tr = libhfst.regex('(a a:b (b));')
    tr.set_final_weights(0.1)
    tr.set_final_weights(0.4, True)

    # reading and writing in text format
    f = open('testfile.att', 'w')
    f.write('0 1 foo bar 0.5\n')
    f.write('0 1 fo ba 0.2\n')
    f.write('0 1 f b 0\n')
    f.write('1 2 baz baz\n')
    f.write('2 0.1\n')
    f.write('--\n')
    f.write('\n')
    f.close()

    numtr = 0
    f = libhfst.hfst_open('testfile.att', 'r')
    while not f.is_eof():
        TR = libhfst.read_att(f)
        numtr += 1
    try:
        libhfst.read_att(f)
    except libhfst.EndOfStreamException:
        pass
    f.close()
    if numtr != 2:
        raise RuntimeError(get_linenumber())

    f = libhfst.hfst_open('foo_att_prolog', 'w')
    f.write('-- in ATT format --\n')
    TR.write_att(f)
    f.write('-- in prolog format --\n')
    TR.write_prolog(f, 'FOOBAR_TRANSDUCER')
    f.close()

    # Lookup and path extraction
    tr = libhfst.regex('foo:bar::0.5 | foo:baz')

    print('tr.lookup')
    print(tr.lookup('foo', max_number=5, output='text'))

    print('tr.extract_paths')
    print(tr.extract_paths(obey_flags='True', filter_flags='False', max_number=3, output='dict'))

    # print('libhfst.dictionary')
    # print(libhfst.dictionary({'FOO':(('BAR',0.5),'BAZ'), 'foo':('bar','baz')}))


print('\n--- Testing HfstBasicTransducer ---\n')

# Create basic transducer, write it to file, read it, and test equivalence
fsm = libhfst.HfstBasicTransducer()
fsm.add_state(0)
fsm.add_state(1)
fsm.set_final_weight(1, 0.3)
tr = libhfst.HfstBasicTransition(1, 'foo', 'bar', 0.5)
fsm.add_transition(0, tr)
fsm.add_transition(0, 0, 'baz', 'baz')
fsm.add_transition(0, 0, 'baz', 'BAZ', 0.1)

f = libhfst.hfst_open('foo_basic', 'w')
fsm.write_att(f)
f.close()

f = libhfst.hfst_open('foo_basic', 'r')
fsm2 = libhfst.HfstBasicTransducer.read_att(f, libhfst.EPSILON)
f.close()

FSM = libhfst.HfstTransducer(fsm, libhfst.FOMA_TYPE)
FSM2 = libhfst.HfstTransducer(fsm2, libhfst.FOMA_TYPE)
    
if not (FSM.compare(FSM2)):
    raise RuntimeError(get_linenumber())

for type in (libhfst.TROPICAL_OPENFST_TYPE, libhfst.FOMA_TYPE):
    Fsm = libhfst.HfstBasicTransducer(FSM.convert(type))
    Fsm2 = libhfst.HfstBasicTransducer(FSM2.convert(type))


# Print basic transducer
fsm = libhfst.HfstBasicTransducer()
for state in [0,1,2]:
    fsm.add_state(state)
fsm.add_transition(0,1,'foo','bar',1)
fsm.add_transition(0,1,'foo','BAR',2)
fsm.add_transition(1,2,'baz','baz',0)
fsm.set_final_weight(2,0.5)

# Different ways to print the transducer
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

index=0
for state in fsm.states_and_transitions():
    for transition in state:
        print('%u\t%u\t%s\t%s\t%.2f\n' % (index, transition.get_target_state(), transition.get_input_symbol(), transition.get_output_symbol(), transition.get_weight()))
    if fsm.is_final_state(index):
        print('%s\t%.2f\n' % (index, fsm.get_final_weight(index)))
    index = index + 1

print(fsm)

tr = libhfst.HfstBasicTransducer(libhfst.regex('foo'))
tr.substitute({'foo':'bar'})
tr.substitute({('foo','foo'):('bar','bar')})
