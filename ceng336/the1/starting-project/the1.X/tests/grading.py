import pickle
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

class Report:
    def __init__(self, rubric: list):
        self.grades = {}
        self.rubric = rubric

    def set_grade(self, name: str, grade: float):
        self.grades[name] = grade
        matches = [r for r in self.rubric if r[0] == name]
        if len(matches) != 1:
            raise ValueError("Rubric error: " + str(matches))
        r = matches[0]
        scaled = round(grade * r[1], 2)
        print(f'Grade for "{r[0]}": {scaled:.2f} / {r[1]:.1f}')

    def save_grades(self, filename):
        with open(filename, 'wb') as f:
            pickle.dump(self.grades, f)

    def load_grades(self, filename):
        with open(filename, 'rb') as f:
            self.grades = pickle.load(f)

    def report_str(self):
        def process_rubric_item(r):
            grade = self.grades[r[0]] if r[0] in self.grades else 0.0
            grade = round(grade * r[1], 2)
            return [r[0], grade, r[1]]
        grade_items = [process_rubric_item(g) for g in self.rubric]
        f = "{0:>5} / {1:>4}  {2}"
        grade_report = "\n".join([f.format(g[1], g[2], g[0]) for g in grade_items])

        total_grade = sum([g[1] for g in grade_items])
        max_grade = sum([g[1] for g in self.rubric])
        return f"GRADE\n{grade_report}\nTOTAL GRADE: {total_grade:.1f}/{max_grade:.1f}"

def record_porta(m, max_history: int, max_cycles: int, flicker_duration: int = 1000, max_changes: int = 500):
    # Watch writes to this register
    m.watch("PORTA W")
    m.watch("LATA W")
    history = []
    # Since PORTA is output, we need to read from LATCH
    value = m.get("LATA")
    current = RegRecording(value, 0)
    cycles = 0
    changes = 0
    while len(history) < max_history:
        if not m.run_timeout():
            print("Timed out while waiting for a PORTA change!")
            m.clear_breakpoints()
            current.time += m.stopwatch()
            history.append(current)
            return history
        value = m.get("LATA")
        time = m.stopwatch()
        current.time += time
        cycles += time
        if value != current.data:
            print("PORTA changed from", str(current.data), "to", str(value) + " (" + bin8(value) + ") after", str(time/1e6) + "s", end = "")
            if current.time > flicker_duration:  # Don't record if too short
                history.append(current)
                print()
            else:
                print(" (FLICKER, ignored)")
            current = RegRecording(value, 0)
        if cycles > max_cycles:
            print(f"Aborting PORTA recording: Exceeded the maximum duration of {max_cycles} cycles")
            if current.time > flicker_duration or not history:
                history.append(current)
            break
        changes += 1
        if changes > max_changes:
            print(f"Aborting PORTA recording: Exceeded the maximum number of changes ({max_changes})")
            if current.time > flicker_duration or not history:
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
    m.stepi(10_000)
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
