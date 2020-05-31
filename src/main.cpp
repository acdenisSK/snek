#include <SFML/Graphics.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <random>
#include <thread>
#include <tuple>
#include <vector>

static constexpr float block_len = 25.0f;

enum class BlockType : std::uint8_t {
  Vacant,
  OccupiedSnake,
  OccupiedFruit,
};

inline constexpr bool is_occupied(BlockType type) {
  return type == BlockType::OccupiedFruit || type == BlockType::OccupiedSnake;
}

class Block : public sf::Drawable {
  BlockType m_type = BlockType::Vacant;
  sf::Color m_colour = sf::Color::Green;

  sf::VertexArray m_arr;

public:
  Block() noexcept
  : m_type(BlockType::Vacant), m_colour(sf::Color::Green), m_arr(sf::LinesStrip, 5) {
    set_colour(m_colour);
  }

  sf::Vector2f position() const noexcept { return m_arr[0].position; }
  BlockType type() const noexcept { return m_type; }
  sf::Color colour() const noexcept { return m_colour; }

  void set_position(sf::Vector2f pos) noexcept {
    m_arr[0].position = pos;
    m_arr[1].position = sf::Vector2f(pos.x + block_len, pos.y);
    m_arr[2].position = pos + sf::Vector2f(block_len, block_len);
    m_arr[3].position = sf::Vector2f(pos.x, pos.y + block_len);
    m_arr[4].position = pos;
  }

  void set_colour(sf::Color colour) noexcept {
    for (std::size_t i = 0; i < m_arr.getVertexCount(); i++) {
      m_arr[i].color = colour;
    }
  }

  void set_type(BlockType type) noexcept {
    switch (type) {
    case BlockType::Vacant:
      m_arr.setPrimitiveType(sf::LinesStrip);
      break;
    case BlockType::OccupiedSnake:
    case BlockType::OccupiedFruit:
      m_arr.setPrimitiveType(sf::Quads);
      break;
    }

    m_type = type;
  }

  void draw(sf::RenderTarget &target, sf::RenderStates states) const override {
    target.draw(m_arr, states);
  }
};

class Grid : public sf::Drawable {
  std::size_t _horizontal, _vertical;
  std::vector<Block> blocks;

public:
  Grid(std::size_t horizontal, std::size_t vertical, sf::Vector2f pos, sf::Vector2u resolution)
    : _horizontal(horizontal), _vertical(vertical), blocks(horizontal * vertical) {

    const std::size_t max_blocks_horizontal =
      std::floor(float(resolution.x - pos.x) / block_len);
    const auto max_blocks_vertical =
      std::size_t(std::floor(float(resolution.y - pos.y) / block_len));

    const float first_x = pos.x;

    for (std::size_t x = 0, y = 0; y < std::min(max_blocks_vertical, vertical); y++) {
      for (; x < std::min(max_blocks_horizontal, horizontal); x++) {
        auto& block = blocks[x + y * horizontal];
        block.set_position(pos);

        pos.x += block_len;
      }

      x = 0;
      pos.x = first_x;
      pos.y += block_len;
    }
  }

  void draw(sf::RenderTarget &target, sf::RenderStates states) const override {
    for (const auto &block : blocks) {
      target.draw(block, states);
    }
  }

  std::size_t horizontal() const noexcept { return _horizontal; }
  std::size_t vertical() const noexcept { return _vertical; }
  std::size_t len() const noexcept { return blocks.size(); }

  Block &operator[](sf::Vector2u pos) { return blocks[pos.x + pos.y * horizontal()]; }
  const Block &operator[](sf::Vector2u pos) const { return blocks[pos.x + pos.y * horizontal()]; }

  Block &operator[](std::size_t pos) noexcept { return blocks[pos]; }
  const Block &operator[](std::size_t pos) const noexcept { return blocks[pos]; }
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
  int x = 0;
  int y = 0;

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
  const char *what() const noexcept override { return "cannot turn the opposite direction"; }
};

struct CollisionException : public std::exception {
  const char *what() const noexcept override { return "collided with the snake's own body"; }
};

namespace randomiser {
  static std::random_device source;
  static std::mt19937 generator(source());

  std::size_t gen(std::size_t min, std::size_t max) {
    auto dist = std::uniform_int_distribution<std::size_t>(min, max);
    return dist(generator);
  }
} // namespace randomiser

class Snake {
  Grid &grid;

  sf::Vector2u head_position;
  std::vector<sf::Vector2u> body_positions;

  Direction _direction;

  void assert(sf::Vector2u pos) const {
    if (pos.x >= grid.horizontal())
      throw std::out_of_range("cannot move outside the grid horizontally");
    if (pos.y >= grid.vertical())
      throw std::out_of_range("cannot move outside the grid vertically");

    if (grid[pos].type() == BlockType::OccupiedSnake)
      throw CollisionException();
  }

  void assert_direction(Direction direct) const {
    if ((direct == Direction::Left && _direction == Direction::Right) ||
        (direct == Direction::Right && _direction == Direction::Left)) {
      throw MotorException();
    }

    if ((direct == Direction::Up && _direction == Direction::Down) ||
        (direct == Direction::Down && _direction == Direction::Up)) {
      throw MotorException();
    }
  }

  void update_pos(sf::Vector2u &pos, sf::Vector2u new_pos) {
    grid[pos].set_type(BlockType::Vacant);

    pos = new_pos;

    grid[pos].set_type(BlockType::OccupiedSnake);
  }

public:
  Snake(Grid &grid) : grid(grid), head_position(), body_positions(), _direction(Direction::None) {
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

    bool was_occupied_by_fruit = grid[new_pos].type() == BlockType::OccupiedFruit;

    update_pos(head_position, new_pos);

    for (auto &pos : body_positions) {
      auto before = pos;

      update_pos(pos, old_pos);

      old_pos = before;
    }

    if (was_occupied_by_fruit) {
      grid[new_pos].set_colour(sf::Color::Green);

      add_body();
    }
  }

  Direction direction() const noexcept { return _direction; }

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

    body_positions.emplace_back(std::move(tail));
  }
};

static sf::Color fruit_colours[] = {
  sf::Color::Red, sf::Color::Blue, sf::Color(0xFF, 0xA5, 0x00), // Orange
};

static sf::Color gen_fruit_colour() {
  return fruit_colours[randomiser::gen(0, 3)];
}

static Block& get_block_randomly(Grid &grid) {
  std::size_t pos = 0;

  do {
    pos = randomiser::gen(0, grid.len() - 1);
  } while (is_occupied(grid[pos].type()));

  return grid[pos];
}

static void spawn_fruit(Grid &grid) {
  auto& block = get_block_randomly(grid);

  block.set_type(BlockType::OccupiedFruit);
  block.set_colour(gen_fruit_colour());
}

static char const *title = "Snek";

enum class GameStates : uint8_t {
  Start,
  InProgress,
  End,
};

int main() {
  sf::RenderWindow window(sf::VideoMode(500, 400), "Snek");

  Grid grid(19, 15, sf::Vector2f(12.0f, 8.0f), window.getSize());
  Snake snake(grid);

  auto state = GameStates::Start;

  sf::Clock clock;

  auto movement_seconds = 0.0f;
  auto spawn_seconds = 0.0f;

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
        } catch (MotorException const &ex) {
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
    case GameStates::InProgress: {
      float secs = clock.restart().asSeconds();

      movement_seconds += secs;
      spawn_seconds += secs;

      if (spawn_seconds >= 5.0f) {
        spawn_fruit(grid);

        spawn_seconds = 0.0f;
      }

      if (movement_seconds >= 0.25f) {
        try {
          snake.move();
        } catch (std::out_of_range const &ex) {
          window.setTitle(sf::String(title) + " : " + ex.what() + "- over!");
          state = GameStates::End;
        } catch (CollisionException const &ex) {
          window.setTitle(sf::String(title) + " : " + ex.what() + "- over!");
          state = GameStates::End;
        }

        movement_seconds = 0.0f;
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
