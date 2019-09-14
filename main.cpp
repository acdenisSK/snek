#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>

class Block : public sf::Drawable {
    float _width = 25.f;
    float _height = 25.f;
    sf::Color _colour = sf::Color::Green;

    sf::VertexArray _tex;
public:
    Block() : _tex(sf::LinesStrip, 5) {}

    Block(sf::Vector2f starting_position) :
        _tex(sf::LinesStrip, 5)
    {
        _tex[0].position = starting_position;
        _tex[1].position = sf::Vector2f(starting_position.x + _width, starting_position.y);
        _tex[2].position = starting_position + sf::Vector2f(_width, _height);
        _tex[3].position = sf::Vector2f(starting_position.x, starting_position.y + _height);
        _tex[4].position = starting_position;

        _tex[0].color = _colour;
        _tex[1].color = _colour;
        _tex[2].color = _colour;
        _tex[3].color = _colour;
        _tex[4].color = _colour;
    }

    float width() const noexcept {
        return _width;
    }

    float height() const noexcept {
        return _height;
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
        blocks(horizontal* vertical)
    {
        const auto starting_block = Block(pos);

        const float width = starting_block.width();
        const float height = starting_block.height();

        blocks[0] = std::move(starting_block);

        const auto max_blocks_horizontal = static_cast<size_t>(std::floor(static_cast<float>(resolution.x - pos.x) / width));
        const auto max_blocks_vertical = static_cast<size_t>(std::floor(static_cast<float>(resolution.y - pos.y) / height));

        const float first_x = pos.x;

        pos.x += width;

        size_t x = 1;

        for (size_t y = 0; y < std::min(max_blocks_vertical, vertical); y++) {
            for (; x < std::min(max_blocks_horizontal, horizontal); x++) {
                blocks[x + y * horizontal] = Block(pos);

                pos.x += width;
            }

            x = 0;
            pos.x = first_x;
            pos.y += height;
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

class Snake {
    Grid& grid;

    sf::Vector2u pos;

    void assert(sf::Vector2u p) {
        if (p.x >= grid.horizontal()) throw std::out_of_range("cannot move outside the grid horizontally");
        if (p.y >= grid.vertical()) throw std::out_of_range("cannot move outside the grid vertically");
    }
public:
    Snake(Grid& grid) : grid(grid), pos(0, 0) {
        grid[pos].setOccupied();
    }

    void move(sf::Vector2i relative) {
        grid[pos].setVacant();


        auto new_pos = sf::Vector2u(
            static_cast<unsigned int>(static_cast<int>(pos.x) + relative.x),
            static_cast<unsigned int>(static_cast<int>(pos.y) + relative.y)
        );

        try {
            assert(new_pos);
        } catch (...) {
            grid[pos].setOccupied();
            throw;
        }

        pos = new_pos;

        grid[pos].setOccupied();
    }

    void moveH(int x) {
        move(sf::Vector2i(x, 0));
    }

    void moveV(int y) {
        move(sf::Vector2i(0, y));
    }

    sf::Vector2u current_position() const noexcept {
        return pos;
    }
};

static const char* title = "Snek";

int main()
{
    auto window = sf::RenderWindow(sf::VideoMode(500, 400), title);

    auto grid = Grid(19, 15, sf::Vector2f(12.f, 8.f), window.getSize());
    auto snake = Snake(grid);

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
                } catch (std::out_of_range ex) {
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