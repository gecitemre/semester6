#include <stdio.h>
#include <message.h>
#include <logging.h>

// Algorithm 1 Controller Loop
// Read the input information about the game from standard input
// Create pipes for bombers (see IPC section)
// Fork the bomber processes (see fork function)
// Redirect the bomber standard input and output to the pipe (see dup2) and close the unused
// end.
// Execute bomber executable with its arguments (see exec family of functions)
// while there are more than one bomber remaining: do
// Select or poll the bomb pipes to see there is any input (see IPC for details)
// Read and act according to the message (see Messages)
// Remove any obstacles if their durability is zero and mark killed bombers
// Reap exploded bombs (see wait family of functions)
// Select or poll the bomber pipes to see there is any input (see IPC for details)
// Read and act according to the message (see Messages) unless the bomber is marked as killed
// Inform marked bombers
// Reap informed killed bombers (see wait family of functions)
// Sleep for 1 millisecond (to prevent CPU hogging)
// end while
// Wait for remaining bombs to explode and reap them

int main() {
    int map_width, map_height, obstacle_count, bomber_count;
    scanf("%d %d %d %d", &map_width, &map_height, &obstacle_count, &bomber_count);
    obsd obstacles[obstacle_count];
    for (int i = 0; i < obstacle_count; i++) {
        scanf("%d %d %d", &obstacles[i].position.x, &obstacles[i].position.y, &obstacles[i].remaining_durability);
    }
//     <bomber1_x> <bomber1_y> <bomber1_total_argument_count>
// <bomber1_executable_path> <bomber1_arg1> <bomber1_arg2> ... <bomber1_argM>
    
    od bombers[bomber_count];
    for (int i = 0; i < bomber_count; i++) {
        bombers[i].type = BOMBER;
        int total_argument_count;
        scanf("%d %d %d", &bombers[i].position.x, &bombers[i].position.y, &total_argument_count);
        char *args[total_argument_count];
        for (int j = 0; j < total_argument_count; j++) {
            args[j] = malloc(100);
            scanf("%s", args[j]);
        }
        
    }
    while (1) {
    }
}