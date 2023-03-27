from dataclasses import dataclass

def bin8(value: int):
    """
    Convert 8-bit register value to binary representation.
    """
    return ("0"*8 + bin(value)[2:])[-8:]

def metronome_pattern(bar_length):
    return [3, 0] + [1, 0] * (bar_length - 1)

def timing_grade(time, target, margin):
    error = abs(target-time)
    if error <= margin:
        return 1.0
    else:
        return 0

@dataclass
class RegRecording:
    data: int
    time: int
    tainted: bool = False

    def timing_grade(self, target, margin):
        return timing_grade(self.time, target, margin)

def bitmask_history(history, bitmask: int):
    first = history[0]
    newhist = [RegRecording(first.data & bitmask, first.time, first.data & bitmask != first.data)]
    for n in history[1:]:
        if n.data & bitmask == newhist[-1].data:
            newhist[-1].time += n.time
            if n.data & bitmask != n.data:
                newhist[-1].tainted = True
        else:
            newhist.append(RegRecording(n.data & bitmask, n.time, n.data & bitmask != n.data))
    return newhist

def prefix_match(sequence, target):
    out = []
    for i, expected in enumerate(target):
        if i >= len(sequence):
            break
        if sequence[i].data != expected:
            break
        out.append(sequence[i])
    return out

def get_longest_match(sequence, target):
    prefixes = [prefix_match(sequence[i:], target) for i in range(len(sequence))]
    if not prefixes:
        return -1, []
    # Reversed because we want to prefer the latest longest match.
    # For example, if we don't use the latest match in 2X speed test, we might incorrectly
    # detect a timing error.
    maxi = max(reversed(range(len(prefixes))), key=lambda i: len(prefixes[i]))
    return maxi, prefixes[maxi]

class ReportSection:
    def __init__(self, name: float, max_grade: float):
        self.name = name
        self.max_grade = max_grade
        self.grade = 0.0

    def grade_str(self) -> str:
        if self.max_grade > 0:
            return "%.1f / %.1f" % (self.grade, self.max_grade)
        else:
            return "%.1f" % self.grade

class Report:
    def __init__(self):
        self.grade_log = []

    def add_section(self, name, grade):
        new_section = ReportSection(name, grade)
        self.grade_log.append(new_section)
        return new_section

    def report_str(self):
        if len(self.grade_log) == 0:
            self.add_section("Failed to execute tests", 0.0)
        grade_items = [g.grade_str() for g in self.grade_log]
        f = "{0:>" + str(max([len(s) for s in grade_items])) + "}  {1}"
        grade_report = "\n".join([f.format(g, r.name) for g, r in zip(grade_items, self.grade_log)])

        total_grade = sum([g.grade for g in self.grade_log])
        max_grade = sum([g.max_grade for g in self.grade_log])
        return f"GRADE\n{grade_report}\nFINAL GRADE: {total_grade:.1f}/{max_grade:.1f}"

def record_porta(m, max_history: int, max_cycles: int, max_flicker: int = 1000):
    flickers = []
    # Watch writes to this register
    m.watch("PORTA W")
    m.watch("LATA W")
    history = []
    # Since PORTA is output, we need to read from LATCH
    value = m.get("LATA")
    current = RegRecording(value, 0)
    cycles = 0
    while len(history) < max_history:
        m.run()
        value = m.get("LATA")
        time = m.stopwatch()
        current.time += time
        cycles += time
        if value != current.data:
            print("PORTA changed to", str(value) + " (" + bin8(value) + ") after", str(time/1e6) + "s", end = " ")
            if current.time > max_flicker:  # Don't record if too short
                history.append(current)
            elif len(history) > 1:  # Ignore flickers at the start
                flickers.append(current)
                print("(FLICKER)", end="")
            print()
            current = RegRecording(value, 0)
        if cycles > max_cycles:
            if current.time > max_flicker:  # Don't record if too short
                history.append(current)
            break
    m.clear_breakpoints()
    return history

def click_button(m, pin):
    """
    Press the button, wait 10ms, release the button
    """
    print("Pressing & releasing "+ pin.upper() + "...")
    m.exec("write pin " + pin + " high")
    m.stepi(50_000)
    m.exec("write pin " + pin + " low")
    m.stepi(50_000)

def hold_button(m, pin):
    """
    Press the button
    """
    print("Holding "+ pin.upper() + "...")
    m.exec("write pin " + pin + " high")

def release_button(m, pin):
    """
    Release the button
    """
    print("Releasing "+ pin.upper() + "...")
    m.exec("write pin " + pin + " low")
