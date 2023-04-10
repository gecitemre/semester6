#include <iostream>
#include "message.h"
#include "logging.h"
#include <sys/select.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <list>

using namespace std;

#define PIPE(fd) socketpair(AF_UNIX, SOCK_STREAM, PF_UNIX, fd)
#define distance(a, b) (abs((int)a.x - (int)b.x) + abs((int)a.y - (int)b.y))

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

im read_message(int fd, pid_t pid)
{
    im message;
    read(fd, &message, sizeof(im));
    imp in;
    in.pid = pid;
    in.m = &message;
    print_output(&in, NULL, NULL, NULL);
    return message;
}

void write_outgoing_message(int fd, pid_t pid, om &message)
{
    omp out;
    out.pid = pid;
    out.m = &message;
    write(fd, &message, sizeof(om));
    print_output(NULL, &out, NULL, NULL);
}

typedef struct
{
    coordinate position;
    int pipe[2];
    pid_t pid;
} bomber;

typedef struct
{
    coordinate position;
    int pipe[2];
    unsigned radius;
    pid_t pid;
} bomb;

list<bomber> bombers;
list<bomb> bombs;
list<obsd> obstacles;

void send_bomber_location(bomber &bomber)
{
    om message;
    message.type = BOMBER_LOCATION;
    message.data.new_position = bomber.position;
    // write(bomber.pipe[1], &message, sizeof(om));
    write_outgoing_message(bomber.pipe[1], bomber.pid, message);
}

int main()
{
    unsigned map_width, map_height, obstacle_count, bomber_count;
    cin >> map_width >> map_height >> obstacle_count >> bomber_count;
    for (unsigned i = 0; i < obstacle_count; i++)
    {
        obstacles.push_back(obsd());
        obsd &obstacle = obstacles.back();
        cin >> obstacle.position.x >> obstacle.position.y >> obstacle.remaining_durability;
    }

    for (unsigned i = 0; i < bomber_count; i++)
    {
        bombers.push_back(bomber());
        bomber &new_bomber = bombers.back();
        unsigned total_argument_count;

        cin >> new_bomber.position.x >> new_bomber.position.y >> total_argument_count;
        char **args = new char *[total_argument_count + 1];
        for (unsigned j = 0; j < total_argument_count; j++)
        {
            args[j] = new char[256];
            cin >> args[j];
        }
        args[total_argument_count] = NULL;
        pipe(new_bomber.pipe);
        new_bomber.pid = fork();
        if (new_bomber.pid == 0)
        {
            // Child writes and reads from pipe[1]
            dup2(new_bomber.pipe[1], STDIN_FILENO);
            dup2(new_bomber.pipe[1], STDOUT_FILENO);
            close(new_bomber.pipe[0]);
            if (execvp(args[0], args) == -1)
            {
                perror("execvp");
                exit(1);
            }
        }
        else if (new_bomber.pid == -1)
        {
            perror("fork");
            exit(1);
        }
        
        // Parent writes and reads from pipe[0]
        close(new_bomber.pipe[1]);
    }

    while (true)
    {
        fd_set readfds;
        FD_ZERO(&readfds);
        int nfds = 0;
        for (bomber &b : bombers)
        {
            FD_SET(b.pipe[0], &readfds);
            nfds = max(nfds, b.pipe[0]);
        }
        for (bomb bomb : bombs)
        {
            FD_SET(bomb.pipe[0], &readfds);
            nfds = max(nfds, bomb.pipe[0]);
        }
        int temp[2];
        PIPE(temp);
        FD_SET(temp[0], &readfds);
        if (select(nfds + 1, &readfds, NULL, NULL, NULL) == -1)
        {
            perror("select");
            exit(1);
        }
        for (bomber &ready_bomber : bombers)
        {
            if (FD_ISSET(ready_bomber.pipe[0], &readfds))
            {
                im incoming = read_message(ready_bomber.pipe[0], ready_bomber.pid);
                switch (incoming.type)
                {
                case BOMBER_START:
                    send_bomber_location(ready_bomber);
                    break;

                case BOMBER_MOVE:
                    if (distance(ready_bomber.position, incoming.data.target_position) != 1)
                    {
                        goto case_bomber_move_exit;
                    }

                    for (obsd &obstacle : obstacles)
                    {
                        // Check if the bomber is trying to move into an obstacle
                        if (obstacle.position.x == incoming.data.target_position.x && obstacle.position.y == incoming.data.target_position.y)
                        {
                            goto case_bomber_move_exit;
                        }
                    }

                    for (bomber &other_bomber : bombers)
                    {
                        // Check if there is a bomber in the target position
                        if (other_bomber.position.x == incoming.data.target_position.x && other_bomber.position.y == incoming.data.target_position.y)
                        {
                            goto case_bomber_move_exit;
                        }
                    }

                    // Check if the bomber is trying to move out of the map
                    if (incoming.data.target_position.x < 0 || incoming.data.target_position.x >= map_width || incoming.data.target_position.y < 0 || incoming.data.target_position.y >= map_height)
                    {
                        goto case_bomber_move_exit;
                    }

                    ready_bomber.position = incoming.data.target_position;

                case_bomber_move_exit:
                    send_bomber_location(ready_bomber);
                    break;

                case BOMBER_PLANT:
                    om bomber_plant_outgoing;
                    bomber_plant_outgoing.type = BOMBER_PLANT_RESULT;
                    bomber_plant_outgoing.data.planted = true;

                    for (bomb &other_bomb : bombs)
                    {
                        if (other_bomb.position.x == other_bomb.position.x && other_bomb.position.y == other_bomb.position.y)
                        {
                            bomber_plant_outgoing.data.planted = false;
                            break;
                        }
                    }
                    if (bomber_plant_outgoing.data.planted)
                    {
                        bombs.push_back(bomb());
                        bomb &new_bomb = bombs.back();
                        new_bomb.position = new_bomb.position;
                        new_bomb.radius = incoming.data.bomb_info.radius;
                        PIPE(new_bomb.pipe);
                        pid_t pid = fork();
                        if (pid == 0)
                        {
                            dup2(new_bomb.pipe[1], STDIN_FILENO);
                            dup2(new_bomb.pipe[1], STDOUT_FILENO);
                            close(new_bomb.pipe[0]);
                            char *args[3] = {"bomb", NULL, NULL};
                            args[1] = new char[256];
                            sprintf(args[1], "%ld", incoming.data.bomb_info.interval);
                            execvp("bomb", args);
                        }
                        new_bomb.pid = pid;
                    }
                    write_outgoing_message(ready_bomber.pipe[0], ready_bomber.pid, bomber_plant_outgoing);
                    break;
                case BOMBER_SEE:
                    object_data outgoing_data[25];
                    unsigned object_count = 0;
                    for (obsd &obstacle : obstacles)
                    {
                        if (distance(ready_bomber.position, obstacle.position) <= 3)
                        {
                            outgoing_data[object_count].type = OBSTACLE;
                            outgoing_data[object_count].position = obstacle.position;
                            object_count++;
                        }
                    }
                    for (bomber &other_bomber : bombers)
                    {
                        if (&other_bomber != &ready_bomber && distance(ready_bomber.position, other_bomber.position) <= 3)
                        {
                            outgoing_data[object_count].type = BOMBER;
                            outgoing_data[object_count].position = other_bomber.position;
                            object_count++;
                        }
                    }
                    for (bomb &bomb : bombs)
                    {
                        if (distance(ready_bomber.position, bomb.position) <= 3)
                        {
                            outgoing_data[object_count].type = BOMB;
                            outgoing_data[object_count].position = bomb.position;
                            object_count++;
                        }
                    }

                    om outgoing_object_count_message;
                    outgoing_object_count_message.type = BOMBER_VISION;
                    outgoing_object_count_message.data.object_count = object_count;
                    omp out;
                    out.pid = ready_bomber.pid;
                    out.m = &outgoing_object_count_message;
                    write(ready_bomber.pipe[0], &outgoing_object_count_message, sizeof(omd));

                    omp outgoing_object_count_message_print;
                    outgoing_object_count_message_print.pid = ready_bomber.pid;
                    outgoing_object_count_message_print.m = &outgoing_object_count_message;
                    unsigned index = 0;
                    send_object_data(ready_bomber.pipe[0], object_count, outgoing_data);
                    print_output(NULL, &outgoing_object_count_message_print, NULL, outgoing_data);
                    break;
                }
            }
        }
        for (auto bomb_it = bombs.begin(); bomb_it != bombs.end(); bomb_it++)
        {
            bomb &ready_bomb = *bomb_it;
            if (FD_ISSET(ready_bomb.pipe[0], &readfds))
            {
                im incoming = read_message(ready_bomb.pipe[0], ready_bomb.pid);
                // type is always BOMB_EXPLODE

                for (auto obstacle_it = obstacles.begin(); obstacle_it != obstacles.end(); obstacle_it++)
                {
                    obsd &obstacle = *obstacle_it;
                    if (distance(obstacle.position, ready_bomb.position) <= ready_bomb.radius)
                    {
                        if (obstacle.remaining_durability != -1)
                        {
                            if (--obstacle.remaining_durability == 0)
                                obstacles.erase(obstacles.begin());
                        }
                    }
                    write(ready_bomb.pipe[0], &obstacle, sizeof(obsd));
                    print_output(NULL, NULL, &obstacle, NULL);
                }

                for (auto bomber_it = bombers.begin(); bomber_it != bombers.end(); bomber_it++)
                {
                    bomber &bomber_in_range = *bomber_it;
                    if (distance(bomber_in_range.position, ready_bomb.position) <= ready_bomb.radius)
                    {
                        om bomber_death_message;
                        bomber_death_message.type = BOMBER_DIE;
                        write_outgoing_message(bomber_in_range.pipe[0], bomber_in_range.pid, bomber_death_message);
                        bombers.erase(bomber_it);
                        if (bombers.size() == 1)
                        {
                            om winner_message;
                            winner_message.type = BOMBER_WIN;
                            write_outgoing_message(bombers.front().pipe[0], bombers.front().pid, winner_message);
                            return 0;
                        }
                    }
                }
                bombs.erase(bomb_it);
            }
        }
    }
}