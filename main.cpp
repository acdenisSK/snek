#include <SFML/Graphics.hpp>
#include <exception>
#include <vector>
#include <random>
#include <ctime>

static const float block_len = 25.f;

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

        const auto max_blocks_horizontal = static_cast<size_t>(std::floor(static_cast<float>(resolution.x - pos.x) / block_len));
        const auto max_blocks_vertical = static_cast<size_t>(std::floor(static_cast<float>(resolution.y - pos.y) / block_len));

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

enum class Direction : uint8_t {
    Left,
    Right,
    Up,
    Down,
};

struct MotorException : public std::exception {
    char const* what() const noexcept override {
        return "cannot turn the opposite direction";
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

    std::vector<sf::Vector2u> positions;

    Direction direction;

    void assert(sf::Vector2u p) const {
        if (p.x >= grid.horizontal()) throw std::out_of_range("cannot move outside the grid horizontally");
        if (p.y >= grid.vertical()) throw std::out_of_range("cannot move outside the grid vertically");
    }

    sf::Vector2u create_pos(sf::Vector2u pos, int x, int y) const {
        return sf::Vector2u(
            static_cast<unsigned int>(static_cast<int>(pos.x) + x),
            static_cast<unsigned int>(static_cast<int>(pos.y) + y)
        );
    }

    void update_pos(sf::Vector2u& pos, sf::Vector2u new_pos) {
        grid[pos].set_type(BlockType::Vacant);

        pos = new_pos;

        grid[pos].set_type(BlockType::OccupiedSnake);
    }

    void move() {
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
        }

        auto it = positions.begin(), end = positions.end();

        auto& head = *it;

        ++it;

        auto old_pos = head;
        auto new_pos = create_pos(head, x, y);

        assert(new_pos);

        if (auto& block = grid[new_pos]; block.is_fruit_occupied()) {
            block.set_type(BlockType::Vacant);
            block.set_color(sf::Color::Green);

            add_body();
        }

        update_pos(head, new_pos);

        for (; it < end; it++) {
            auto& pos = *it;

            auto before = pos;

            update_pos(pos, old_pos);

            old_pos = before;
        }
    }
public:
    Snake(Grid& grid) : grid(grid), positions(), direction(Direction::Right) {
        auto horizontal = randomiser::gen(0, grid.horizontal());
        auto vertical = randomiser::gen(0, grid.vertical());
        auto initial = sf::Vector2u(horizontal, vertical);

        grid[initial].set_type(BlockType::OccupiedSnake);

        positions.push_back(std::move(initial));
    }

    void moveH(bool left) {
        auto direct = left ? Direction::Left : Direction::Right;

        if ((direct == Direction::Left && direction == Direction::Right)
            || (direct == Direction::Right && direction == Direction::Left)) {
            throw MotorException();
        }

        direction = direct;

        move();
    }

    void moveV(bool up) {
        auto direct = up ? Direction::Up : Direction::Down;

        if ((direct == Direction::Up && direction == Direction::Down)
            || (direct == Direction::Down && direction == Direction::Up)) {
            throw MotorException();
        }

        direction = direct;

        move();
    }

    void add_body() {
        auto tail = current_position();

        switch (direction) {
        case Direction::Left:
            positions.emplace_back(tail.x + 1, tail.y);
            break;
        case Direction::Right:
            positions.emplace_back(tail.x - 1, tail.y);
            break;
        case Direction::Up:
            positions.emplace_back(tail.x, tail.y + 1);
            break;
        case Direction::Down:
            positions.emplace_back(tail.x, tail.y - 1);
            break;
        }

        grid[current_position()].set_type(BlockType::OccupiedSnake);
    }

    sf::Vector2u current_position() const noexcept {
        return positions.back();
    }
};



sf::Color gen_fruit_colour() {
    static const sf::Color fruit_colours[] = {
        sf::Color::Yellow,
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

int main() {
    auto window = sf::RenderWindow(sf::VideoMode(500, 400), title);

    auto grid = Grid(19, 15, sf::Vector2f(12.f, 8.f), window.getSize());
    auto snake = Snake(grid);

    spawn_fruit(grid);
    spawn_fruit(grid);

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
                        snake.moveH(true);
                        break;
                    case sf::Keyboard::Right:
                        snake.moveH(false);
                        break;
                    case sf::Keyboard::Up:
                        snake.moveV(true);
                        break;
                    case sf::Keyboard::Down:
                        snake.moveV(false);
                        break;
                    default:
                        break;
                    }
                } catch (std::exception const& ex) {
                    window.setTitle(sf::String(title) + " : " + ex.what());
                }

                break;
            default:
                break;
            }
        }

        window.clear(sf::Color::White);
        window.draw(grid);
        window.display();
    }

    return 0;
}