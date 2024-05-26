from synthnotehelper import *


@synth_test_notes
def gen(synth):
    n0 = Note(32, amplitude=.5)
    n1 = Note(16, amplitude=.1)
    s = [
            {"press": [n0, n1]},
            3/8,
            {"release": n0},
            4/8,
            {"release": n1},
        ]
    yield []
    synth.change(start=s)
    yield 1

print("test interrupting sequence")

@synth_test_notes
def gen1(synth):
    n0 = Note(32, amplitude=.5)
    n1 = Note(16, amplitude=.1)
    s = [
            {"press": [n0, n1]},
            3/8,
            {"release": n0},
            4/8,
            {"release": n1},
        ]
    yield []
    synth.change(start=s)
    yield 1/2
    synth.change(stop=s)
    yield 1/2
