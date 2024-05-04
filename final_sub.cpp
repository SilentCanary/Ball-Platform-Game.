#include <iostream>
#include <Windows.h>
#include <time.h>
#include <conio.h>
#include <vector>
#include <mutex>
#include <fstream>
#include <thread>
#include <string>

using namespace std;

// --------------------------definitions-----------------------------------------------
#define width 80
#define height 30
#define LEFT 75
#define RIGHT 77
#define SPACE 32
#define SHOOT_LEFT 'a'
#define SHOOT_RIGHT 'd'
#define door_left 48
#define door_right 55
#define door_top 25
#define door_bottom 29
// --------------------------------------------------------------------------------------

int platform_dx = 1;

mutex mtx;  // mutex object for locking of shared resources like coordinates of platform and ball

class platform
{
public:
    int PLATFORM_X;
    int PLATFORM_Y;
    int length_platform;
    int platform_dx = 1;
    bool platform_moves = false;
    platform(int x, int y, int l)
    {
        PLATFORM_X = x;
        PLATFORM_Y = y;
        length_platform = l;
    }
};

class Ball
{
public:
    int ball_x;
    int ball_y;
    bool on_platform = false;
    int current_platform = 0;
    Ball(int x, int y)
    {
        ball_x = x;
        ball_y = y;
    }
};

class Bullet

{
public:
    int bullet_x;
    int bullet_y;
    int bullet_dx;
    bool is_shooted;
    Bullet(int x, int y)
    {
        bullet_x = x;
        bullet_y = y;
        bullet_dx = 1;
        is_shooted = false;
    }
};

class Enemy
{
public:
    int enemy_x;
    int enemy_y;
    bool enemy_active;
    int dx = 1;
    int current_platform = 0;
    Enemy(int x, int y)
    {
        enemy_x = x;
        enemy_y = y;
        enemy_active = true;
    }
};

string player_name;
Ball ball(0, 0);
vector<platform> platforms;
Bullet bullet(0, 0);
vector<Enemy> enemies;

//         ----------FUNCTIONS USED---------------------

void gotoxy(int x, int y); // to get coordinates of a point on console
void draw_boundaries();    // to draw boundaries
void draw_door();
void print(int ch, platform p);  // for platform printing
bool check_if_ball_is_on_platform();
void print_bullet(int ch);
void print_enemy(const Enemy &enemy, int ch);
void print_ball(int ch);
void move_bullet();
void move_enemies();
bool check_collision();
bool check_if_ball_is_in_door();
void ball_check_movement(); // main fxn ---> controls movement of ball
void move_platforms();
void skipLine(fstream &file);
void save_game();
bool player_exists(string name);
void load_game(string name);

// ------------------------------------------------------------------------

bool stop_ball_thread = false;
bool stop_bullet_thread = false;
bool stop_enemy_thread = false;
bool stop_platform_thread = false;
bool saved_or_not = false;

// -------------------------------------------------------------------------------

void gotoxy(int x, int y)
{
    fflush(stdout);
    COORD c;
    c.X = x;
    c.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), c);
}

void skipLine(fstream &file) // used to skip lines in file cuz pattern is player name 12 lines of coordinates nd then second player
{
    char c;
    while (file.get(c) && c != '\n')
        ;
}

void FRONT_PAGE()
{
    gotoxy(23, 15);
    cout << "ENTER THE NAME OF THE PLAYER :";
    cin >> player_name;
    char ch;
    if (player_exists(player_name))
    {
        system("cls");
        gotoxy(25, 15);
        cout << "WELCOME BACK " << player_name << " ! " << endl;
        gotoxy(25, 16);
        cout << "Do you wish to continue the saved game ?(y/n)" << endl;
        cin >> ch;
        switch (ch)
        {
        case 'y':
            load_game(player_name);
            saved_or_not = true;
            draw_boundaries();
            break;
        case 'n':
            system("cls");
            draw_boundaries();
            gotoxy(25, 15);
            cout << "PRESS ENTER TO CONTINUE THE GAME ";
            getch();
            draw_boundaries();
        default:
            break;
        }
    }
    else
    {
        system("cls");
        gotoxy(25, 15);
        cout << "WELCOME TO THE GAME " << player_name << " ! " << endl;
        cout << "PRESS ENTER TO CONTINUE THE GAME ";
        getch();
        draw_boundaries();
    }
}

void draw_boundaries()
{
    system("cls");
    for (int i = 0; i <= width; i++)
    {
        gotoxy(i, 0);
        cout << "-";
        gotoxy(i, height);
        cout << "-";
    }
    for (int i = 1; i < height; i++)
    {
        gotoxy(0, i);
        cout << "|";
        gotoxy(width, i);
        cout << "|";
    }
}

void draw_door()
{
    for (int i = door_left; i <= door_right; i++)
    {
        gotoxy(i, door_top);
        cout << "*";
        gotoxy(i, door_bottom);
        cout << "*";
    }
    for (int i = door_top; i <= door_bottom; i++)
    {
        gotoxy(door_left, i);
        cout << "*";
        gotoxy(door_right, i);
        cout << "*";
    }
}

void print(int ch, platform p)
{

    fflush(stdout);
    mtx.lock();
    gotoxy(p.PLATFORM_X, p.PLATFORM_Y);
    switch (ch)
    {
    case 1:
        for (int i = 0; i < p.length_platform; i++)
        {
            cout << char(178);
        }
        gotoxy(0, 0);
        break;
    case 0:
        for (int i = 0; i < p.length_platform; i++)
        {
            cout << " ";
        }
        gotoxy(0, 0);
        break;
    }
    mtx.unlock();
}

void print_bullet(int ch)
{
    mtx.lock();
    gotoxy(bullet.bullet_x, bullet.bullet_y);
    switch (ch)
    {
    case 1:
        cout << "-";
        break;
    case 0:
        cout << " ";
        break;
    default:
        break;
    }
    gotoxy(0, 0);
    mtx.unlock();
}

void print_enemy(const Enemy &enemy, int ch)
{
    mtx.lock();
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    gotoxy(enemy.enemy_x, enemy.enemy_y);
    SetConsoleTextAttribute(h, FOREGROUND_RED | FOREGROUND_INTENSITY);
    switch (ch)
    {
    case 1:
        cout << "O";
        break;
    case 0:
        cout << " ";
        break;
    default:
        break;
    }
    SetConsoleTextAttribute(h, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    gotoxy(0, 0);
    mtx.unlock();
}

void print_ball(int ch)
{
    mtx.lock();
    gotoxy(ball.ball_x, ball.ball_y);
    switch (ch)
    {
    case 1:
        cout << "o";
        break;
    case 0:
        cout << " ";
        break;
    default:
        break;
    }
    gotoxy(0, 0);
    mtx.unlock();
}

void move_bullet()
{
    while (true && !stop_bullet_thread)
    {
        if (bullet.is_shooted)
        {
            print_bullet(0);
            bullet.bullet_x += bullet.bullet_dx;
            if (bullet.bullet_x == width - 1 || bullet.bullet_x == 1)
            {
                bullet.is_shooted = false;
                continue;
            }
            for (int i = 0; i < enemies.size(); i++)
            {
                bool hit_enemy = (((bullet.bullet_x == enemies[i].enemy_x) || (bullet.bullet_x + 1 == enemies[i].enemy_x)) && (bullet.bullet_y == enemies[i].enemy_y));

                if (bullet.is_shooted && hit_enemy && enemies[i].enemy_active)
                {
                    bullet.is_shooted = false;
                    enemies[i].enemy_active = false;
                }
            }
            print_bullet(1);
            this_thread::sleep_for(chrono::milliseconds(50));
        }
    }
}

void move_enemies()
{
    while (true && !stop_enemy_thread)
    {
        for (int i = 0; i < enemies.size(); i++)
        {
            print_enemy(enemies[i], 0);
            if (enemies[i].enemy_active)
            {
                mtx.lock();
                enemies[i].enemy_x += platforms[i].platform_dx;

                if (enemies[i].enemy_x >= (platforms[i].PLATFORM_X + platforms[i].length_platform))
                {
                    enemies[i].dx = -1;
                }
                else if (enemies[i].enemy_x <= platforms[i].PLATFORM_X)
                {
                    enemies[i].dx = 1;
                }
                enemies[i].enemy_x += enemies[i].dx;
                print_enemy(enemies[i], 1);
                mtx.unlock();
                print_enemy(enemies[i], 1);
            }
        }
        this_thread::sleep_for(chrono::milliseconds(120));
    }
}

bool check_collision()
{
    for (const auto &enemy : enemies)
    {
        if (ball.ball_x == enemy.enemy_x && ball.ball_y == enemy.enemy_y && enemy.enemy_active)
        {
            return true;
        }
    }
    return false;
}

bool check_if_ball_is_on_platform()
{
    int count = 0;
    for (const auto &itr : platforms)
    {

        if (ball.ball_x >= itr.PLATFORM_X && ball.ball_x <= itr.PLATFORM_X + itr.length_platform && ball.ball_y == itr.PLATFORM_Y - 1)
        {
            ball.on_platform = true;
            ball.current_platform = count;
            return true;
            break;
        }
        else
        {
            ball.on_platform = false;
        }
        count++;
    }
    return false;
}

bool check_if_ball_is_in_door()
{
    if (ball.ball_x >= door_left && ball.ball_x < door_right && ball.ball_y >= door_top && ball.ball_y <= door_bottom)
    {
        return true;
    }
    return false;
}

void ball_check_movement()
{
    mtx.lock();
    if (!saved_or_not)
    {
        ball.ball_x = platforms[0].PLATFORM_X + (platforms[0].length_platform / 2);
        ball.ball_y = platforms[0].PLATFORM_Y - 1;
    }
    print_ball(1);
    mtx.unlock();
    int dx = 0;
    while (true && !stop_ball_thread)
    {
        mtx.lock();
        print_ball(0);
        if (platforms[ball.current_platform].platform_moves == true && check_if_ball_is_on_platform() == true)
        {
            ball.ball_x += platforms[ball.current_platform].platform_dx;
        }
        print_ball(1);
        mtx.unlock();

        if (kbhit() && check_if_ball_is_on_platform())
        {
            char ch = getch();
            switch (ch)
            {
            case LEFT:
                dx = -1;
                break;
            case RIGHT:
                dx = 1;
                break;
            case SHOOT_LEFT:
                if (!bullet.is_shooted)
                {
                    bullet.bullet_x = ball.ball_x + 1;
                    bullet.bullet_y = ball.ball_y;
                    bullet.bullet_dx = -1;
                    bullet.is_shooted = true;
                }
                break;
            case SHOOT_RIGHT:
                if (!bullet.is_shooted)
                {
                    bullet.bullet_x = ball.ball_x + 1;
                    bullet.bullet_y = ball.ball_y;
                    bullet.bullet_dx = 1;
                    bullet.is_shooted = true;
                }
                break;
            case SPACE:
                save_game();

            default:
                break;
            }
            mtx.lock();
            print_ball(0);
            if (ball.ball_x == width - 1)
            {
                dx *= -1;
            }
            else if (ball.ball_x == 1)
            {
                dx *= 1;
            }
            ball.ball_x += dx;
            print_ball(1);
            mtx.unlock();
        }
        mtx.lock();
        check_if_ball_is_on_platform();
        if (check_if_ball_is_in_door())
        {
            system("cls");
            gotoxy(width / 2, height / 2);
            cout << "Congratulations! You have won the game!";
            exit(1);
        }

        if (check_collision())
        {
            system("cls");
            gotoxy(width / 2, (height / 2)-10);
            cout << "GAME OVER!! You were hit by an enemy!";
            exit(1);
        }
        mtx.unlock();

        // this part is for downward motion of ball

        if (!ball.on_platform)
        {
            mtx.lock();
            print_ball(0);
            ball.ball_y++;
            check_if_ball_is_on_platform();
            print_ball(1);
            if (ball.ball_y == height - 1)
            {
                system("cls");
                gotoxy(width / 2, height / 2);
                cout << "GAME OVER!!";
                exit(1);
            }
            mtx.unlock();
        }
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

void move_platforms()
{
    while (true && !stop_platform_thread)
    {
        for (auto &p : platforms)
        {
            int count = 0;
            mtx.lock();
            print(0, p);
            if (p.PLATFORM_X >= (width - p.length_platform - 1) || p.PLATFORM_X == 2)
            {

                p.platform_dx *= -1;
            }
            p.PLATFORM_X += p.platform_dx;
            p.platform_moves = true;
            print(1, p);

            mtx.unlock();
        }
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

void save_game()
{
    fstream saveFile;
    saveFile.open("MyGame.txt", ios::app);
    if (!saveFile.is_open())
    {
        cout << "CANNOT OPEN THE FILE!" << endl;
    }
    saveFile << player_name << endl;
    saveFile << ball.ball_x << " " << ball.ball_y << " " << ball.current_platform << " " << ball.on_platform << endl;
    saveFile << bullet.bullet_x << " " << bullet.bullet_y << " " << bullet.bullet_dx << " " << bullet.is_shooted << endl;

    for (const auto &enemy : enemies)
    {
        saveFile << enemy.enemy_x << " " << enemy.enemy_y << " " << enemy.enemy_active << " " << enemy.dx << endl;
    }

    for (const auto &p : platforms)
    {
        saveFile << p.PLATFORM_X << " " << p.PLATFORM_Y << " " << p.length_platform << " " << p.platform_dx << " " << p.platform_moves << endl;
    }
    stop_ball_thread = true; // because we want everything to stop and all are threads are continuously running
    stop_bullet_thread = true;
    stop_enemy_thread = true;
    stop_platform_thread = true;
    system("cls");
    system("cls");
    gotoxy(width / 2, height / 2);
    cout << "SAVED SUCCESSFULLY" << endl;
    saveFile.close();
}

void load_game(string name)
{
    gotoxy(120, 17);
    cout << player_name;
    Sleep(200);
    fstream saveFile;
    saveFile.open("MyGame.txt", ios::in);
    if (!saveFile.is_open())
    {
        cout << "UNABLE TO OPEN FILE !!" << endl;
        return;
    }

    string playerName;
    bool playerFound = false;
    while (saveFile >> playerName)
    {
        if (playerName == name)
        {
            playerFound = true;
            saveFile >> ball.ball_x >> ball.ball_y >> ball.current_platform >> ball.on_platform;
            saveFile >> bullet.bullet_x >> bullet.bullet_y >> bullet.bullet_dx >> bullet.is_shooted;
            for (auto &enemy : enemies)
            {
                saveFile >> enemy.enemy_x >> enemy.enemy_y >> enemy.enemy_active >> enemy.dx;
            }
            for (auto &p : platforms)
            {
                saveFile >> p.PLATFORM_X >> p.PLATFORM_Y >> p.length_platform >> p.platform_dx >> p.platform_moves;
            }
            break;
        }
        else
        {
            // Skip reading data for other players
            for (int i = 0; i < 13; i++)
            {
                skipLine(saveFile);
            }
        }
    }

    saveFile.close();

    if (!playerFound)
    {
        cout << "Player data not found!" << endl;
    }
}

bool player_exists(string name)
{
    fstream saveFile;
    saveFile.open("MyGame.txt", ios::in);
    if (!saveFile.is_open())
    {
        cout << "UNABLE TO OPEN FILE !!" << endl;
    }
    string playerName;
    while (saveFile >> playerName)
    {
        if (playerName == name)
        {
            saveFile.close();
            return true;
        }
        for (int i = 0; i < 13; i++)
        {
            skipLine(saveFile);
        }
    }
    saveFile.close();
    return false;
}

int main()
{
    system("cls");
    srand(time(NULL));
    draw_boundaries();

    // pushing platforms into vector at different coordinates.
    platforms.push_back(platform(5, 5, 15));
    platforms.push_back(platform(width / 2, height / 2, 13));
    platforms.push_back(platform(20, 10, 10));
    platforms.push_back(platform(10, 24, 13));
    platforms.push_back(platform(5, 16, 10));

    // pushing enemies to the vector such that they are genrated on second half og platform.
    for (int i = 0; i < platforms.size(); ++i)
    {
        int midpoint = platforms[i].PLATFORM_X + platforms[i].length_platform / 2;
        enemies.push_back(Enemy(midpoint + 2 + rand() % ((platforms[i].length_platform / 2) - 1), platforms[i].PLATFORM_Y - 1));
    }

    FRONT_PAGE();
    draw_door();

    gotoxy(100, 15);
    cout << "ENTER SPACE KEY TO SAVE AND EXIT." << endl;
    gotoxy(100, 17);
    thread platform_m(move_platforms);
    thread ball_m(ball_check_movement);
    thread bullet_m(move_bullet);
    thread enemy_m(move_enemies);
    platform_m.join();
    ball_m.join();
    enemy_m.join();
    bullet_m.join();
}