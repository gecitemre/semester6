from mdb import *
from grading import *

prelude =  """
device PIC18F4620
set warningmessagebreakoptions.W0223_ADC_UNDERFLOW ignore
set warningmessagebreakoptions.W0222_ADC_OVERFLOW ignore
set oscillator.frequency 1
set oscillator.frequencyunit Mega
hwtool sim
"""

breakpoints = [
    # "ms500_passed",
    # "ms1000_passed",
]

FULL_STEP_PERIOD = 500e3
FULL_STEP_MARGIN = 50e3
HALF_STEP_PERIOD = 250e3
HALF_STEP_MARGIN = 50e3

rubric = [
    ["Initialization period", 5.0],
    ["Metronome operation without input", 15.0],
    ["Pause on RB0 click", 5.0],
    ["Resume on RB0 click", 5.0],
    ["Continue while RB0 is held", 10.0],
    ["Pause when RB0 is released", 5.0],
    ["Increase bar length on RB4 click", 5.0],
    ["Reset bar length on RB2 click after increasing", 2.5],
    ["Continue normally while RB4 is held", 5.0],
    ["Decrease bar length on RB3 click", 5.0],
    ["Reset bar length on RB2 click after decreasing", 2.5],
    ["Continue normally while RB3 is held", 5.0],
    ["1x speed while RB1 is held", 5.0],
    ["2x speed after RB1 is released", 10.0],
    ["2x speed while RB1 is held", 10.0],
    ["1x speed after RB1 is released", 5.0],
]

def initialization_check(initial):
    incorrect = bin(initial.data ^ 7).count('1')
    print("Initialization PORTA bits: " + bin8(initial.data))
    led_correct = 8 - incorrect
    led_grade = led_correct / 8.0
    print(f"Initialization LED correct bits: {led_correct}/8")
    timing_grade = initial.timing_grade(1e6, FULL_STEP_MARGIN)
    print("Initialization took " + str(initial.time / 1e6) + "s")
    print("Initialization timing is", "within" if timing_grade > 0 else "beyond", "margin")
    report.set_grade("Initialization period", (led_grade + timing_grade) * 0.5)

def report_grade(name: str, correct, max_correct):
    global report
    print("Correct/Max:", f"{correct} / {max_correct}")
    report.set_grade(name, correct / max_correct)

def metronome_check(name: str, history, expected, target, margin):
    print("(Half point for every LED match, full point for every LED match with correct duration.)")
    prefix_index, longest_sequence = get_longest_match(history, expected)
    if prefix_index == -1:
        print("Metronome check failed: No recorded PORTA changes.")
        return report_grade(name, 0.0, len(expected)), longest_sequence
    # 0.5  for each correct LED pattern with incorrect timing
    # 0.75 if timing error is within 2*margin
    # 1.0  if timing error is within margin
    grades = [a.timing_grade(target, margin) * 0.5 + 0.5 for a in longest_sequence]
    expected_padded = [""] * prefix_index + [hex(e)[-1] for e in expected]
    print("Expected: ", " ".join(["{:>4}".format(e) for e in expected_padded]))
    print("Output:   ", " ".join(["{:>4}".format(hex(e.data)[-1]) for e in history]))
    print("Time (ms):", " ".join(["{:>4}".format(str(e.time // 1000)) for e in history]))
    print("Points:   ", " ".join(["{:>4}".format(grade) for grade in [""] * prefix_index + grades]))
    report_grade(name, sum(grades), len(expected))
    return longest_sequence

def timing_check(name: str, history, length, target, margin):
    print("(Full point for every LED pattern change with correct duration.)")
    if not history:
        print("Timing check failed: No recorded PORTA changes.")
        return report_grade(name, 0.0, length)
    prefix_index = max(0, len(history) - length)
    sequence = history[prefix_index:prefix_index+length]
    grades = [a.timing_grade(target, margin) for a in sequence]
    print("Output:   ", " ".join(["{:>4}".format(hex(e.data)[-1]) for e in history]))
    print("Time (ms):", " ".join(["{:>4}".format(str(e.time // 1000)) for e in history]))
    print("Points:   ", " ".join(["{:>4}".format(grade) for grade in [""] * prefix_index + grades]))
    return report_grade(name, sum(grades), length)

#
# TESTS
#

def no_input_test(m, bp2addr, addr2bp):
    """
    Tests whether the device works as intended when no input is given.
    """
    global report
    history = record_porta(m, 20, 24 * 1000 * 1000)
    print("INITIALIZATION")
    initialization_check(history[0])
    print("METRONOME CHECK")
    expected = metronome_pattern(4) * 2
    metronome_check("Metronome operation without input", history, expected, FULL_STEP_PERIOD, FULL_STEP_MARGIN)


def pause_test(m, bp2addr, addr2bp):
    """
    Checks whether the start/pause button works.
    """
    def grade_porta():
        nonlocal m
        def led_grade(porta):
            if porta & 4:
                if porta & 4 != porta:
                    return "RA2 was lit, but other LEDs were also lit", 0.5
                return "Only RA2 was lit as expected", 1.0
            return "RA2 was not lit", 0.0
        # Need to read PORTA from LATA since it's output
        leds = m.get("LATA")
        print("PORTA:", bin(leds))
        reason, grade = led_grade(leds)
        print(reason)
        return grade

    def check_if_paused(name: str):
        global report
        print("RA2 should be lit while paused:")
        grade1 = grade_porta()
        print()
        print("Waiting 1 million instructions...")
        m.stepi(1_000_000)
        print("LED 2 should be lit after pausing and waiting:")
        grade2 = grade_porta()
        grade = (grade1 + grade2) * 0.5
        report.set_grade(name, grade)
        print()
        return grade

    print("Waiting 1.5 million instructions...")
    m.stepi(1_500_000)
    print()
    click_button(m, "rb0")
    pause_grade = check_if_paused("Pause on RB0 click")

    if pause_grade == 0:
        print("Aborting resume test because pause test failed.")
        return

    # Try to continue
    click_button(m, "rb0")
    # Record
    history = record_porta(m, 24, 24 * 1000 * 1000)
    print("METRONOME CHECK AFTER PAUSE & RESUME")
    expected = metronome_pattern(4) * 2
    metronome_check("Resume on RB0 click", history, expected, FULL_STEP_PERIOD, FULL_STEP_MARGIN)
    print()

    print("Metronome should continue as usual while RB0 is held.")
    hold_button(m, "rb0")
    history = record_porta(m, 24, 24 * 1000 * 1000)
    print("METRONOME CHECK WHILE RB0 IS HELD")
    expected = metronome_pattern(4) * 2
    metronome_check("Continue while RB0 is held", history, expected, FULL_STEP_PERIOD, FULL_STEP_MARGIN)
    print()

    release_button(m, "rb0")
    m.stepi(50_000)
    print("Metronome should pause after RB0 is released.")
    check_if_paused("Pause when RB0 is released")


def increase_bar_test(m, bp2addr, addr2bp):
    """
    Tests whether the bar length can be increased using RB4 and reset using RB2.
    """
    m.stim("rb4.scl")
    history = record_porta(m, 24, 24 * 1000 * 1000)
    print("INCREASE BAR METRONOME CHECK")
    expected = metronome_pattern(6) + [3, 0, 1, 0]
    longest_sequence = metronome_check("Increase bar length on RB4 click", history, expected, FULL_STEP_PERIOD, FULL_STEP_MARGIN)
    print()

    if len(longest_sequence) < len(expected):
        print("Reset test is aborted: Failed to increase the bar length.")
        return

    m.stepi(50_000)
    click_button(m, "rb2")
    history = record_porta(m, 24, 24 * 1000 * 1000)
    print("RESET BAR METRONOME CHECK")
    expected = metronome_pattern(4) * 2
    metronome_check("Reset bar length on RB2 click after increasing", history, expected, FULL_STEP_PERIOD, FULL_STEP_MARGIN)
    print()

    print("Metronome should continue as usual while RB4 is held.")
    hold_button(m, "rb4")
    history = record_porta(m, 24, 24 * 1000 * 1000)
    print("METRONOME CHECK WHILE RB4 IS HELD")
    expected = metronome_pattern(4) * 2
    metronome_check("Continue normally while RB4 is held", history, expected, FULL_STEP_PERIOD, FULL_STEP_MARGIN)
    print()
    release_button(m, "rb4")


def decrease_bar_test(m, bp2addr, addr2bp):
    """
    Tests whether the bar length can be decreased using RB3 and reset using RB2.
    """
    m.stim("rb3.scl")
    history = record_porta(m, 24, 24 * 1000 * 1000)
    print("DECREASE BAR METRONOME CHECK")
    expected = metronome_pattern(3) * 2 + [3, 0, 1, 0]
    longest_sequence = metronome_check("Decrease bar length on RB3 click", history, expected, FULL_STEP_PERIOD, FULL_STEP_MARGIN)
    print()

    if len(longest_sequence) < len(expected):
        print("Reset test is aborted: Failed to increase the bar length.")
        return

    # Reset
    m.stepi(50_000)
    click_button(m, "rb2")
    history = record_porta(m, 24, 24 * 1000 * 1000)
    print("RESET BAR METRONOME CHECK")
    expected = metronome_pattern(4) * 2
    metronome_check("Reset bar length on RB2 click after decreasing", history, expected, FULL_STEP_PERIOD, FULL_STEP_MARGIN)
    print()

    print("Metronome should continue as usual while RB3 is held.")
    hold_button(m, "rb3")
    history = record_porta(m, 24, 24 * 1000 * 1000)
    print("METRONOME CHECK WHILE RB3 IS HELD")
    expected = metronome_pattern(4) * 2
    metronome_check("Continue normally while RB3 is held", history, expected, FULL_STEP_PERIOD, FULL_STEP_MARGIN)
    print()
    release_button(m, "rb3")


def change_speed_test(m, bp2addr, addr2bp):
    """
    Checks whether the speed can be changed using RB1.
    """
    print("Waiting 1.5 million instructions...")
    m.stepi(1_500_000)
    print()

    def check_speed(name: str, speedstr, period, margin):
        history = record_porta(m, 18, 24 * 1000 * 1000)
        print(speedstr, "SPEED TIMING CHECK:", name)
        timing_check(name, history, 16, period, margin)
        print()

    hold_button(m, "rb1")
    check_speed("1x speed while RB1 is held", "1X", FULL_STEP_PERIOD, FULL_STEP_MARGIN)

    release_button(m, "rb1")
    check_speed("2x speed after RB1 is released", "2X", HALF_STEP_PERIOD, HALF_STEP_MARGIN)

    hold_button(m, "rb1")
    check_speed("2x speed while RB1 is held", "2X", HALF_STEP_PERIOD, HALF_STEP_MARGIN)

    release_button(m, "rb1")
    check_speed("1x speed after RB1 is released", "1X", FULL_STEP_PERIOD, FULL_STEP_MARGIN)


if __name__ == "__main__":
    report = Report(rubric)

    tester = MdbTester(prelude, breakpoints)
    tester.run([
        no_input_test,
        pause_test,
        increase_bar_test,
        decrease_bar_test,
        change_speed_test,
    ])

    print()
    print(report.report_str())
    # report.save_grades("grades.pkl")
