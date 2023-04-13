#include "message.h"
#include "logging.h"
#include <bits/stdc++.h>
#include <sys/wait.h>

using namespace std;

#define BOMB_DIR "./bomb"
#define MAX_ARG_SIZE 256
#define distance(a, b) (abs((int)a.x - (int)b.x) + abs((int)a.y - (int)b.y))
#define PIPE(fd) socketpair(AF_UNIX, SOCK_STREAM, PF_UNIX, fd)

struct game_entity
{
private:
    pid_t pid;
    int fd;

public:
    coordinate position;
    ~game_entity()
    {
        waitpid(pid, NULL, WNOHANG);
    }
    void create_process(char *args[])
    {
        int pipe[2];
        PIPE(pipe);
        pid = fork();
        if (pid == 0)
        {
            // Child writes and reads from pipe[1]
            dup2(pipe[1], STDIN_FILENO);
            dup2(pipe[1], STDOUT_FILENO);
            close(pipe[0]);
            if (execvp(args[0], args) == -1)
            {
                perror("execvp");
                exit(1);
            }
        }
        else if (pid == -1)
        {
            perror("fork");
            exit(1);
        }
        close(pipe[1]);
        fd = pipe[0];
    }
    im read_message()
    {
        im message;
        read_data(fd, &message);
        imp print_message;
        print_message.pid = pid;
        print_message.m = &message;
        print_output(&print_message, NULL, NULL, NULL);
        return message;
    }

    // write with printing
    void write_outgoing_message(om &message)
    {
        omp print_message;
        print_message.pid = pid;
        print_message.m = &message;
        write_message(message);
        print_output(NULL, &print_message, NULL, NULL);
    }

    // write without printing
    void write_message(om &message)
    {
        // write(fd, &message, sizeof(message));
        send_message(fd, &message);
    }

    int get_fd()
    {
        return fd;
    }

    pid_t get_pid()
    {
        return pid;
    }
};

struct bomber : public game_entity
{
    void win()
    {
        om message;
        message.type = BOMBER_WIN;
        write_outgoing_message(message);
    }
    void send_location()
    {
        om message;
        message.type = BOMBER_LOCATION;
        message.data.new_position = position;
        write_outgoing_message(message);
    }
};

struct bomb : public game_entity
{
    unsigned radius;
};

int main()
{
    list<bomber> bombers;
    list<bomb> bombs;
    list<obsd> obstacles;
    unsigned map_width, map_height, obstacle_count, bomber_count;
    cin >> map_width >> map_height >> obstacle_count >> bomber_count;
    bomber *bombers_grid[map_width][map_height];
    obsd *obstacles_grid[map_width][map_height];
    bomb *bombs_grid[map_width][map_height];
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
        char *args[total_argument_count + 1];
        for (unsigned j = 0; j < total_argument_count; j++)
        {
            args[j] = new char[MAX_ARG_SIZE];
            cin >> args[j];
        }
        args[total_argument_count] = NULL;
        new_bomber.create_process(args);
        for (unsigned j = 0; j < total_argument_count; j++)
        {
            delete[] args[j];
        }
    }

    while (true)
    {
        fd_set readfds;
        FD_ZERO(&readfds);
        int nfds = 0;
        for (bomber &b : bombers)
        {
            FD_SET(b.get_fd(), &readfds);
            nfds = max(nfds, b.get_fd());
        }
        for (bomb bomb : bombs)
        {
            FD_SET(bomb.get_fd(), &readfds);
            nfds = max(nfds, bomb.get_fd());
        }
        if (select(nfds + 1, &readfds, NULL, NULL, NULL) == -1)
        {
            perror("select");
            exit(1);
        }
        for (bomber &ready_bomber : bombers)
        {
            if (FD_ISSET(ready_bomber.get_fd(), &readfds))
            {
                im incoming = ready_bomber.read_message();
                switch (incoming.type)
                {
                case BOMBER_START:
                    ready_bomber.send_location();
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
                    if (incoming.data.target_position.x >= map_width || incoming.data.target_position.y >= map_height)
                    {
                        goto case_bomber_move_exit;
                    }

                    ready_bomber.position = incoming.data.target_position;

                case_bomber_move_exit:
                    ready_bomber.send_location();
                    break;

                case BOMBER_PLANT:
                    om bomber_plant_outgoing;
                    bomber_plant_outgoing.type = BOMBER_PLANT_RESULT;
                    bomber_plant_outgoing.data.planted = true;

                    for (bomb &other_bomb : bombs)
                    {
                        if (other_bomb.position.x == ready_bomber.position.x && other_bomb.position.y == ready_bomber.position.y)
                        {
                            bomber_plant_outgoing.data.planted = false;
                            break;
                        }
                    }
                    if (bomber_plant_outgoing.data.planted)
                    {
                        bombs.push_back(bomb());
                        bomb &new_bomb = bombs.back();
                        new_bomb.position = ready_bomber.position;
                        new_bomb.radius = incoming.data.bomb_info.radius;
                        char *args[3] = {BOMB_DIR, new char[MAX_ARG_SIZE], NULL};
                        sprintf(args[1], "%ld", incoming.data.bomb_info.interval);
                        args[2] = NULL;
                        new_bomb.create_process(args);
                        delete[] args[1];
                    }
                    ready_bomber.write_outgoing_message(bomber_plant_outgoing);
                    break;
                case BOMBER_SEE:
                    memset(obstacles_grid, 0, sizeof(obstacles_grid));
                    for (obsd &obstacle : obstacles)
                    {
                        obstacles_grid[obstacle.position.x][obstacle.position.y] = &obstacle;
                    }
                    memset(bombers_grid, 0, sizeof(bombers_grid));
                    for (bomber &bomber : bombers)
                    {
                        bombers_grid[bomber.position.x][bomber.position.y] = &bomber;
                    }
                    memset(bombs_grid, 0, sizeof(bombs_grid));
                    for (bomb &bomb : bombs)
                    {
                        bombs_grid[bomb.position.x][bomb.position.y] = &bomb;
                    }

                    object_data outgoing_data[25];
                    unsigned object_count = 0;

#define test_vision(offset_x, offset_y)                                            \
    int x = ready_bomber.position.x + offset_x;                                    \
    int y = ready_bomber.position.y + offset_y;                                    \
    if (x >= 0 && x < map_width && y >= 0 && y < map_height)                       \
    {                                                                              \
        if (!(offset_x == 0 && offset_y == 0) && bombers_grid[x][y] != NULL)       \
        {                                                                          \
            outgoing_data[object_count].type = BOMBER;                             \
            outgoing_data[object_count].position = bombers_grid[x][y]->position;   \
            object_count++;                                                        \
        }                                                                          \
        else if (bombs_grid[x][y] != NULL)                                         \
        {                                                                          \
            outgoing_data[object_count].type = BOMB;                               \
            outgoing_data[object_count].position = bombs_grid[x][y]->position;     \
            object_count++;                                                        \
        }                                                                          \
        else if (obstacles_grid[x][y] != NULL)                                     \
        {                                                                          \
            outgoing_data[object_count].type = OBSTACLE;                           \
            outgoing_data[object_count].position = obstacles_grid[x][y]->position; \
            object_count++;                                                        \
            break;                                                                 \
        }                                                                          \
    }

                    test_vision(0, 0)

                        for (int offset_x = -1; offset_x >= -3; offset_x--)
                    {
                        test_vision(offset_x, 0)
                    }

                    for (int offset_x = 1; offset_x <= 3; offset_x++)
                    {
                        test_vision(offset_x, 0)
                    }

                    for (int offset_y = -1; offset_y >= -3; offset_y--)
                    {
                        test_vision(0, offset_y)
                    }

                    for (int offset_y = 1; offset_y <= 3; offset_y++)
                    {
                        test_vision(0, offset_y)
                    }

                    om outgoing_object_count_message;
                    outgoing_object_count_message.type = BOMBER_VISION;
                    outgoing_object_count_message.data.object_count = object_count;

                    ready_bomber.write_message(outgoing_object_count_message);

                    omp outgoing_object_count_message_print;
                    outgoing_object_count_message_print.pid = ready_bomber.get_pid();
                    outgoing_object_count_message_print.m = &outgoing_object_count_message;
                    send_object_data(ready_bomber.get_fd(), object_count, outgoing_data);
                    print_output(NULL, &outgoing_object_count_message_print, NULL, outgoing_data);
                    break;
                }
            }
        }
        if (bombers.size() == 1)
        {
            bombers.front().win();
            return 0;
        }
        for (auto bomb_it = bombs.begin(); bomb_it != bombs.end(); bomb_it++)
        {
            if (FD_ISSET(bomb_it->get_fd(), &readfds))
            {
                bomb &ready_bomb = *bomb_it;
                ready_bomb.read_message();
                // type is always BOMB_EXPLODE

                for (auto obstacle_it = obstacles.begin(); obstacle_it != obstacles.end(); obstacle_it++)
                {
                    obsd &obstacle = *obstacle_it;
                    if (distance(obstacle.position, ready_bomb.position) <= ready_bomb.radius &&
                        obstacle.position.x == ready_bomb.position.x &&
                        obstacle.position.y == ready_bomb.position.y)
                    {
                        if (obstacle.remaining_durability != -1)
                        {
                            if (--obstacle.remaining_durability == 0)
                                obstacles.erase(obstacles.begin());
                        }
                    }
                    write(ready_bomb.get_fd(), &obstacle, sizeof(obsd));
                    print_output(NULL, NULL, &obstacle, NULL);
                }

                for (auto bomber_it = bombers.begin(); bomber_it != bombers.end(); bomber_it++)
                {
                    if (distance(bomber_it->position, ready_bomb.position) <= ready_bomb.radius)
                    {
                        bomber &bomber_in_range = *bomber_it;
                        om bomber_death_message;
                        bomber_death_message.type = BOMBER_DIE;
                        bomber_in_range.write_outgoing_message(bomber_death_message);
                        bombers.erase(bomber_it);
                        if (bombers.size() == 1)
                        {
                            bombers.front().win();
                            return 0;
                        }
                    }
                }
                bombs.erase(bomb_it);
            }
        }
    }
}