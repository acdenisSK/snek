#include <SFML/Graphics.hpp>
#include <exception>
#include <vector>

static float block_width = 25.f;
static float block_height = 25.f;

class Block : public sf::Drawable {
    sf::Color _colour = sf::Color::Green;

    sf::VertexArray _tex;
public:
    Block() : _tex(sf::LinesStrip, 5) {}

    explicit Block(sf::Vector2f starting_position) :
        _tex(sf::LinesStrip, 5) {
        _tex[0].position = starting_position;
        _tex[1].position = sf::Vector2f(starting_position.x + block_width, starting_position.y);
        _tex[2].position = starting_position + sf::Vector2f(block_width, block_height);
        _tex[3].position = sf::Vector2f(starting_position.x, starting_position.y + block_height);
        _tex[4].position = starting_position;

        _tex[0].color = _colour;
        _tex[1].color = _colour;
        _tex[2].color = _colour;
        _tex[3].color = _colour;
        _tex[4].color = _colour;
    }

    sf::Color colour() const noexcept {
        return _colour;
    }

    void setOccupied() {
        _tex.setPrimitiveType(sf::Quads);
    }

    void setVacant() {
        _tex.setPrimitiveType(sf::LinesStrip);
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

        const auto max_blocks_horizontal = static_cast<size_t>(std::floor(static_cast<float>(resolution.x - pos.x) / block_width));
        const auto max_blocks_vertical = static_cast<size_t>(std::floor(static_cast<float>(resolution.y - pos.y) / block_height));

        const float first_x = pos.x;

        for (size_t x = 0, y = 0; y < std::min(max_blocks_vertical, vertical); y++) {
            for (; x < std::min(max_blocks_horizontal, horizontal); x++) {
                blocks[x + y * horizontal] = Block(pos);

                pos.x += block_width;
            }

            x = 0;
            pos.x = first_x;
            pos.y += block_height;
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

    Block& operator[](sf::Vector2u pos) {
        return blocks[pos.x + pos.y * horizontal()];
    }

    Block const& operator[](sf::Vector2u pos) const {
        return blocks[pos.x + pos.y * horizontal()];
    }
};

enum class Direction : uint8_t {
    Left,
    Right,
    Up,
    Down,
};

class MotorException : public std::exception {
    char const* message;
public:
    MotorException() = delete;
    explicit MotorException(char const* message) : message(message) {}

    char const* what() const noexcept override {
        return message;
    }
};

class Snake {
    Grid& grid;

    std::vector<sf::Vector2u> positions;

    Direction direction;

    void assert(sf::Vector2u p) {
        if (p.x >= grid.horizontal()) throw std::out_of_range("cannot move outside the grid horizontally");
        if (p.y >= grid.vertical()) throw std::out_of_range("cannot move outside the grid vertically");
    }

    sf::Vector2u create_pos(sf::Vector2u pos, int x, int y) {
        return sf::Vector2u(
            static_cast<unsigned int>(static_cast<int>(pos.x) + x),
            static_cast<unsigned int>(static_cast<int>(pos.y) + y)
        );
    }

    void update_pos(sf::Vector2u& pos, sf::Vector2u new_pos) {
        grid[pos].setVacant();

        pos = new_pos;

        grid[pos].setOccupied();
    }

    void move(int x, int y) {
        auto it = positions.begin(), end = positions.end();

        auto& head = *it;

        ++it;

        auto old_pos = head;
        auto new_pos = create_pos(head, x, y);

        assert(new_pos);

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
        positions.emplace_back(2, 0);

        grid[positions[0]].setOccupied();
    }

    void moveH(int x) {
        auto direct = (0 < x) ? Direction::Left : Direction::Right;

        if ((direct == Direction::Left && direction == Direction::Right)
            || (direct == Direction::Right && direction == Direction::Left)) {
            throw MotorException("cannot turn the opposite direction");
        }

        direction = direct;

        move(x, 0);
    }

    void moveV(int y) {
        auto direct = (0 < y) ? Direction::Up : Direction::Down;

        if ((direct == Direction::Up && direction == Direction::Down)
            || (direct == Direction::Down && direction == Direction::Up)) {
            throw MotorException("cannot turn the opposite direction");
        }

        direction = direct;

        move(0, y);
    }

    void add_body() {
        const auto& tail = positions.back();

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

        grid[positions.back()].setOccupied();
    }

    sf::Vector2u current_position() const noexcept {
        return positions.back();
    }
};

static char const* title = "Snek";

int main() {
    auto window = sf::RenderWindow(sf::VideoMode(500, 400), title);

    auto grid = Grid(19, 15, sf::Vector2f(12.f, 8.f), window.getSize());
    auto snake = Snake(grid);
    snake.add_body();
    snake.add_body();

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
                        snake.moveH(-1);
                        break;
                    case sf::Keyboard::Right:
                        snake.moveH(1);
                        break;
                    case sf::Keyboard::Up:
                        snake.moveV(-1);
                        break;
                    case sf::Keyboard::Down:
                        snake.moveV(1);
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