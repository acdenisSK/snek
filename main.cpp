#include <SFML/Graphics.hpp>
#include <array>

class Block : public sf::Drawable {
	float _width = 50.f;
	float _height = 50.f;
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

	void draw(sf::RenderTarget& target, sf::RenderStates states) const override {
		target.draw(_tex, states);
	}

	float width() const {
		return _width;
	}

	float height() const {
		return _height;
	}

	sf::Color colour() const {
		return _colour;
	}

	sf::Vector2f range() const {
		return sf::Vector2f(_tex[1].position.x, _tex[3].position.y);
	}
};

template<size_t N>
class Grid : public sf::Drawable {
	std::array<Block, N> blocks;
public:
	Grid(sf::Vector2f pos) : blocks() {
		Block starting_block(pos);

		float width = starting_block.width();
		float height = starting_block.height();

		blocks[0] = std::move(starting_block);
		
		for (size_t i = 1; i < N; i++) {
			pos.x += width;

			blocks[i] = Block(pos);
		}
	}

	void draw(sf::RenderTarget& target, sf::RenderStates states) const override {
		for (auto const& block : blocks) {
			target.draw(block, states);
		}
	}
};

int main()
{
	sf::RenderWindow window(sf::VideoMode(500, 400), "Snek");
	
	Grid<5> grid(sf::Vector2f(5.f, 5.f));

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
		}

		window.clear(sf::Color::White);
		window.draw(grid);
		window.display();
	}

	return 0;
}