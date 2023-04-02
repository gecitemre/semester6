#include <stdio.h>
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

typedef struct {
    coordinate position;
    int[2] pipe;
} bomber;

typedef struct {
    coordinate position;
    int pipe[2];
    int radius;
} bomb;

list<bomber> bombers;
list<bomb> bombs;
list<obsd> obstacles;

void send_bomber_location(bomber& bomber) {
    om message;
    message.type = BOMBER_LOCATION;
    message.data.new_position = bomber.position;
    write(bomber.pipe[1], &message, sizeof(om));
}

int main() {
    unsigned map_width, map_height, obstacle_count, bomber_count;
    cin >> map_width >> map_height >> obstacle_count >> bomber_count;
    for (unsigned i = 0; i < obstacle_count; i++) {
        obstacles.push_back(obsd());
        cin >> obstacles.back().position.x >> obstacles.back().position.y >> obstacles.back().remaining_durability;
    }

    for (unsigned i = 0; i < bomber_count; i++) {
        bombers.push_back(bomber());
        unsigned total_argument_count;
        cin >> bombers.back().data.position.x >> bombers.back().data.position.y >> total_argument_count;
        char args[total_argument_count][100];
        for (unsigned j = 0; j < total_argument_count; j++) {
            cin >> args[j];
        }
        pipe(bombers.back().pipe);
        pid_t pid = fork();
        if (pid == 0) {
            dup2(pipes[i][0], STDIN_FILENO);
            dup2(pipes[i][1], STDOUT_FILENO);
            execvp(args[0], args);
        }
    }
    while (true) {
        fd_set readfds;
        FD_ZERO(&readfds);
        unsigned nfds = 0;
        for (unsigned pipe[2]: pipes) {
            FD_SET(pipe[0], &readfds);
            nfds = max(nfds, pipe[0]);
        }
        select(nfds + 1, &readfds, NULL, NULL, NULL);
        for (bomber& bomber: bombers) {
            if (FD_ISSET(bomber.pipe[0], &readfds)) {
                im incoming;
                read(bomber.pipe[0], &incoming, sizeof(im));
                switch (incoming.type) {
                    case BOMBER_START:
                        send_bomber_location(bomber);
                        break;

                    case BOMBER_MOVE:
                        if (distance(bomber.position, incoming.data.target_position) != 1) {
                            goto case_bomber_move_exit;
                        }
                        
                        for (obsd& obstacle: obstacles) {
                            if (obstacle.position.x == incoming.data.target_position.x && obstacle.position.y == incoming.data.target_position.y) {
                                goto case_bomber_move_exit;
                            }
                        }

                        for (bomber& bomber: bombers) {
                            if (bomber.position.x == incoming.data.target_position.x && bomber.position.y == incoming.data.target_position.y) {
                                goto case_bomber_move_exit;
                            }
                        }

                        if (incoming.data.target_position.x < 0 || incoming.data.target_position.x >= map_width || incoming.data.target_position.y < 0 || incoming.data.target_position.y >= map_height) {
                            goto case_bomber_move_exit;
                        }

                        bomber.position = incoming.data.target_position;

                        case_bomber_move_exit:
                        send_bomber_location(bomber);
                        break;

                    case BOMBER_PLANT:
                        om bomber_plant_outgoing;
                        bomber_plant_outgoing.type = BOMBER_PLANT_RESULT;
                        bomber_plant_outgoing.data.planted = true;

                        for (bomb& bomb: bombs) {
                            if (bomb.position.x == bomber.position.x && bomb.position.y == bomber.position.y) {
                                bomber_plant_outgoing.data.planted = false;
                                break;
                            }
                        }
                        if (bomber_plant_outgoing.data.planted) {
                            bomb bomb;
                            bomb.position = bomber.position;
                            bomb.radius = incoming.data.radius;
                            pipe(bomb.pipe);
                            pid_t pid = fork();
                            if (pid == 0) {
                                dup2(bomb.pipe[0], STDIN_FILENO);
                                dup2(bomb.pipe[1], STDOUT_FILENO);
                                execvp("bomb", {"bomb", bomb.radius});
                            }
                        }
                        write(bomber.pipe[1], &bomber_plant_outgoing, sizeof(omd));
                        break;
                    case BOMBER_SEE:
                        unsigned object_count = 0;
                        list<od> outgoing_data;
                        for (unsigned j = 0; j < obstacle_count; j++) {
                            if (distance(bomber.position, obstacles[j].position) <= 3) {
                                object_count++;
                                outgoing_data.push_back(od());
                                outgoing_data.back().type = OBSTACLE;
                                outgoing_data.back().position = obstacles[j].position;
                            }
                        }
                        for (bomber& bomber: bombers) {
                            if (distance(bomber.position, bomber.position) <= 3) {
                                object_count++;
                                outgoing_data.push_back(od());
                                outgoing_data.back().type = BOMBER;
                                outgoing_data.back().position = bomber.position;
                            }
                        }
                        for (bomb& bomb: bombs) {
                            if (distance(bomber.position, bomb.position) <= 3) {
                                object_count++;
                                outgoing_data.push_back(od());
                                outgoing_data.back().type = BOMB;
                                outgoing_data.back().position = bomb.position;
                            }
                        }

                        write(bomber.pipe[1], &object_count, sizeof(unsigned));
                        for (od& outgoing_data: outgoing_data) {
                            write(bomber.pipe[1], &outgoing_data, sizeof(od));
                        }
                        break;
                }
            }
        }
        for (bomber& bomb: bombs) {
            if (FD_ISSET(bomb.pipe[0], &readfds)) {
                im incoming;
                // read to prevent blocking
                read(bomb.pipe[0], &incoming, sizeof(im));
                // type is always BOMB_EXPLODE
                for (obsd& obstacle: obstacles) {
                    if (distance(obstacle.position, bomb.position) <= bomb.radius) {
                        obstacle.remaining_durability--;
                        if (obstacle.remaining_durability == 0) {
                            obstacle.position.x = -1;
                            obstacle.position.y = -1;
                        }
                    }
                }
            }
        }
    }
}