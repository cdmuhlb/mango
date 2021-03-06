#!/usr/bin/env python3

def verify_increment_and_last_line_matches_internal_line(filename):
    try:
        f = open(filename)
    except:
        print("Error! Unable to open specified file: ",filename)
        raise

    lines = f.readlines()
    f.close()

    min_N_lines = 7
    if len(lines) < min_N_lines:
        print('File '+filename+' is <'+str(min_N_lines)+' lines long, so it must not have stopped before completing. Treating this as a success.')
        return

    # Verify that the first element in each line is an integer that increments each line.
    for index in range(5, len(lines)-1):
        splitline = lines[index].split(',')
        first_element = splitline[0]
        try:
            first_int = int(first_element)
        except:
            print('Error converting first element of line ',index,' to an integer. Here is the line:')
            print(splitline)
            raise

        expected_int = index - 4
        if first_int != expected_int:
            print('Error! First element is not incrementing properly. First_int=',first_int,', expected ',expected_int)
            print('Here are the preceding lines and the line in question:')
            print(lines[index-1].strip())
            print(lines[index].strip())
            exit(1)

    splitline = lines[-1].split(',')
    min_N_entries = 4
    if len(splitline) < min_N_entries:
        print('The last line of file '+filename+' has <'+str(min_N_lines)+' comma-delimited entries, so something must have gone wrong.')
        print('Here is the last line:')
        print(splitline)
        exit(1)

    # Get the function_evaluation index of the optimum
    try:
        best_function_evaluation = int(splitline[0])
    except:
        print('Error converting the first comma-delimited entry of the last line of '+filename+' to an integer.')
        print('Here is the last line:')
        print(splitline)
        raise

    # Verify the relevant lines match
    line_that_should_match = best_function_evaluation + 4
    if lines[line_that_should_match] != lines[-1]:
        print('Error! Last line of '+filename+' does not match the corresponding line in the interior of the file.')
        print('Last line:')
        print(lines[-1])
        print('0-based line ',line_that_should_match)
        print(lines[line_that_should_match])
        exit(1)

    # If we make it this far, then success.
    #print('Verified that last line of '+filename+' matches the corresponding line in the interior of the file.')


# Allow this module to be run as a standalone script.
if __name__ == '__main__':
    import sys
    if len(sys.argv) != 2:
        print('Error! Must supply 1 argument: the filename to check.')
        exit(1)
    filename = sys.argv[1]
    verify_increment_and_last_line_matches_internal_line(filename)
    print('In file '+filename+', verified that function_evaluation increments properly, and the last line matches the corresponding line in the interior of the file.')
