#include <SFML/Graphics.hpp>
#include <exception>
#include <vector>
#include <random>
#include <tuple>
#include <chrono>
#include <thread>

static const float block_len = 25.f;

using uint = unsigned int;

enum class BlockType : uint8_t {
    Vacant,
    OccupiedSnake,
    OccupiedFruit,
};

class Block : public sf::Drawable {
    BlockType _type = BlockType::Vacant;
    sf::Color _colour = sf::Color::Green;

    sf::VertexArray _tex;
public:
    Block() : _tex(sf::LinesStrip, 5) {}

    explicit Block(sf::Vector2f starting_position) :
        _tex(sf::LinesStrip, 5) {
        _tex[0].position = starting_position;
        _tex[1].position = sf::Vector2f(starting_position.x + block_len, starting_position.y);
        _tex[2].position = starting_position + sf::Vector2f(block_len, block_len);
        _tex[3].position = sf::Vector2f(starting_position.x, starting_position.y + block_len);
        _tex[4].position = starting_position;

        _tex[0].color = _colour;
        _tex[1].color = _colour;
        _tex[2].color = _colour;
        _tex[3].color = _colour;
        _tex[4].color = _colour;
    }

    BlockType type() const noexcept {
        return _type;
    }

    bool is_snake_occupied() const noexcept {
        return _type == BlockType::OccupiedSnake;
    }

    bool is_fruit_occupied() const noexcept {
        return _type == BlockType::OccupiedFruit;
    }

    bool is_occupied() const noexcept {
        return is_snake_occupied() || is_fruit_occupied();
    }

    bool is_vacant() const noexcept {
        return _type == BlockType::Vacant;
    }

    sf::Color colour() const noexcept {
        return _colour;
    }

    void set_color(sf::Color colour) {
        for (size_t i = 0; i < _tex.getVertexCount(); i++) {
            _tex[i].color = colour;
        }
    }

    void set_type(BlockType type) {
        switch (type) {
        case BlockType::Vacant:
            _tex.setPrimitiveType(sf::LinesStrip);
            break;
        case BlockType::OccupiedSnake:
        case BlockType::OccupiedFruit:
            _tex.setPrimitiveType(sf::Quads);
            break;
        }

        _type = type;
    }

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override {
        target.draw(_tex, states);
    }
};

class Grid : public sf::Drawable {
    size_t _horizontal, _vertical;
    std::vector<Block> blocks;

public:
    Grid(size_t horizontal, size_t vertical, sf::Vector2f pos, sf::Vector2u resolution) :
        _horizontal(horizontal),
        _vertical(vertical),
        blocks(horizontal* vertical) {

        const auto max_blocks_horizontal = size_t(std::floor(float(resolution.x - pos.x) / block_len));
        const auto max_blocks_vertical = size_t(std::floor(float(resolution.y - pos.y) / block_len));

        const float first_x = pos.x;

        for (size_t x = 0, y = 0; y < std::min(max_blocks_vertical, vertical); y++) {
            for (; x < std::min(max_blocks_horizontal, horizontal); x++) {
                blocks[x + y * horizontal] = Block(pos);

                pos.x += block_len;
            }

            x = 0;
            pos.x = first_x;
            pos.y += block_len;
        }
    }

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override {
        for (auto const& block : blocks) {
            target.draw(block, states);
        }
    }

    size_t horizontal() const noexcept {
        return _horizontal;
    }

    size_t vertical() const noexcept {
        return _vertical;
    }

    size_t len() const noexcept {
        return blocks.size();
    }

    Block& operator[](sf::Vector2u pos) {
        return blocks[pos.x + pos.y * horizontal()];
    }

    Block const& operator[](sf::Vector2u pos) const {
        return blocks[pos.x + pos.y * horizontal()];
    }

    Block& operator[](size_t pos) noexcept {
        return blocks[pos];
    }

    Block const& operator[](size_t pos) const noexcept {
        return blocks[pos];
    }
};

sf::Vector2u operator+(sf::Vector2u lhs, sf::Vector2i rhs) {
    return sf::Vector2u(uint(int(lhs.x) + rhs.x), uint(int(lhs.y) + rhs.y));
}

enum class Direction : uint8_t {
    None,
    Left,
    Right,
    Up,
    Down,
};

sf::Vector2i to_pos(Direction direction) {
    int x = 0, y = 0;

    switch (direction) {
    case Direction::Left:
        x = -1;
        break;
    case Direction::Right:
        x = 1;
        break;
    case Direction::Up:
        y = -1;
        break;
    case Direction::Down:
        y = 1;
        break;
    default:
        break;
    }

    return sf::Vector2i(x, y);
}

struct MotorException : public std::exception {
    char const* what() const noexcept override {
        return "cannot turn the opposite direction";
    }
};

struct CollisionException : public std::exception {
    char const* what() const noexcept override {
        return "collided with the snake's own body";
    }
};

namespace randomiser
{
    static std::random_device source;
    static std::mt19937 generator(source());

    size_t gen(size_t min, size_t max) {
        auto dist = std::uniform_int_distribution<size_t>(min, max);
        return dist(generator);
    }
}

class Snake {
    Grid& grid;

    sf::Vector2u head_position;
    std::vector<sf::Vector2u> body_positions;

    Direction _direction;

    void assert(sf::Vector2u pos) const {
        if (pos.x >= grid.horizontal()) throw std::out_of_range("cannot move outside the grid horizontally");
        if (pos.y >= grid.vertical()) throw std::out_of_range("cannot move outside the grid vertically");

        if (grid[pos].is_snake_occupied()) throw CollisionException();
    }

    void assert_direction(Direction direct) const {
        if ((direct == Direction::Left && _direction == Direction::Right)
            || (direct == Direction::Right && _direction == Direction::Left)) {
            throw MotorException();
        }

        if ((direct == Direction::Up && _direction == Direction::Down)
            || (direct == Direction::Down && _direction == Direction::Up)) {
            throw MotorException();
        }
    }

    void update_pos(sf::Vector2u& pos, sf::Vector2u new_pos) {
        grid[pos].set_type(BlockType::Vacant);

        pos = new_pos;

        grid[pos].set_type(BlockType::OccupiedSnake);
    }

public:
    Snake(Grid& grid) : grid(grid), head_position(), body_positions(), _direction(Direction::None) {
        auto horizontal = randomiser::gen(0, grid.horizontal());
        auto vertical = randomiser::gen(0, grid.vertical());
        auto initial = sf::Vector2u(horizontal, vertical);

        grid[initial].set_type(BlockType::OccupiedSnake);

        head_position = std::move(initial);
    }

    void move() {
        auto pos = to_pos(_direction);

        auto old_pos = head_position;
        auto new_pos = head_position + pos;

        assert(new_pos);

        bool was_occupied_by_fruit = grid[new_pos].is_fruit_occupied();

        update_pos(head_position, new_pos);

        for (auto& pos : body_positions) {
            auto before = pos;

            update_pos(pos, old_pos);

            old_pos = before;
        }

        if (was_occupied_by_fruit) {
            grid[new_pos].set_color(sf::Color::Green);

            add_body();
        }
    }

    Direction direction() const noexcept {
        return _direction;
    }

    void set_direction(Direction direct) {
        assert_direction(direct);

        _direction = direct;
    }

    void add_body() {
        auto tail = body_positions.empty() ? head_position : body_positions.back();

        switch (_direction) {
        case Direction::Left:
            tail.x += 1;
            break;
        case Direction::Right:
            tail.x -= 1;
            break;
        case Direction::Up:
            tail.y += 1;
            break;
        case Direction::Down:
            tail.y -= 1;
            break;
        default:
            break;
        }

        grid[tail].set_type(BlockType::OccupiedSnake);

        body_positions.push_back(std::move(tail));
    }
};

sf::Color gen_fruit_colour() {
    static const sf::Color fruit_colours[] = {
        sf::Color::Red,
        sf::Color::Blue,
        sf::Color::Color(0xFF, 0xA5, 0x00), // Orange
    };

    return fruit_colours[randomiser::gen(0, 3)];
}

void spawn_fruit(Grid& grid) {
    size_t pos;

    do {
        pos = randomiser::gen(0, grid.len() - 1);
    } while (grid[pos].is_occupied());

    auto& block = grid[pos];

    block.set_type(BlockType::OccupiedFruit);
    block.set_color(gen_fruit_colour());
}

static char const* title = "Snek";

enum class GameStates : uint8_t {
    Start,
    InProgress,
    End,
};

int main() {
    auto window = sf::RenderWindow(sf::VideoMode(500, 400), title);

    auto grid = Grid(19, 15, sf::Vector2f(12.f, 8.f), window.getSize());
    auto snake = Snake(grid);

    auto state = GameStates::Start;

    auto clock = sf::Clock();

    auto movement_seconds = 0.f;
    auto spawn_seconds = 0.f;

    while (window.isOpen()) {
        auto event = sf::Event();
        while (window.pollEvent(event)) {
            switch (event.type) {
            case sf::Event::Closed:
                window.close();
                break;
            case sf::Event::KeyPressed:
                try {
                    switch (event.key.code) {
                    case sf::Keyboard::Left:
                        snake.set_direction(Direction::Left);
                        break;
                    case sf::Keyboard::Right:
                        snake.set_direction(Direction::Right);
                        break;
                    case sf::Keyboard::Up:
                        snake.set_direction(Direction::Up);
                        break;
                    case sf::Keyboard::Down:
                        snake.set_direction(Direction::Down);
                        break;
                    default:
                        break;
                    }
                } catch (MotorException const& ex) {
                    window.setTitle(sf::String(title) + " : " + ex.what());
                }

                break;
            default:
                break;
            }
        }

        switch (state) {
        case GameStates::Start:
            if (snake.direction() != Direction::None) {
                state = GameStates::InProgress;
            }

            break;
        case GameStates::InProgress:
        {
            float secs = clock.restart().asSeconds();

            movement_seconds += secs;
            spawn_seconds += secs;

            if (spawn_seconds >= 5.f) {
                spawn_fruit(grid);

                spawn_seconds = 0.f;
            }

            if (movement_seconds >= 0.25f) {
                try {
                    snake.move();
                } catch (std::out_of_range const& ex) {
                    window.setTitle(sf::String(title) + " : " + ex.what() + "- over!");
                    state = GameStates::End;
                } catch (CollisionException const& ex) {
                    window.setTitle(sf::String(title) + " : " + ex.what() + "- over!");
                    state = GameStates::End;
                }

                movement_seconds = 0.f;
            }

            break;
        }
        case GameStates::End:
            break;
        }

        window.clear(sf::Color::White);
        window.draw(grid);
        window.display();
    }

    return 0;
}