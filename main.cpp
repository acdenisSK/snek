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

	float width() const {
		return _width;
	}

	float height() const {
		return _height;
	}

	sf::Color colour() const {
		return _colour;
	}

	void draw(sf::RenderTarget& target, sf::RenderStates states) const override {
		target.draw(_tex, states);
	}
};

template<size_t H, size_t V>
class Grid : public sf::Drawable {
	std::vector<Block> blocks;
public:
	Grid(sf::Vector2f pos, sf::Vector2u resolution) : blocks(H*V) {
		Block starting_block(pos);

		float width = starting_block.width();
		float height = starting_block.height();

		blocks[0] = std::move(starting_block);
		
		auto max_blocks_horizontal = static_cast<size_t>(std::floor(static_cast<float>(resolution.x - pos.x) / width));
		auto max_blocks_vertical = static_cast<size_t>(std::floor(static_cast<float>(resolution.y - pos.y) / height));
		
		float first_x = pos.x;

		pos.x += width;

		size_t x = 1;
		
		for (size_t y = 0; y < std::min(max_blocks_vertical, V); y++) {
			for (; x < std::min(max_blocks_horizontal, H); x++) {
				blocks[x + y * H] = Block(pos);

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
};

int main()
{
	sf::RenderWindow window(sf::VideoMode(500, 400), "Snek");

	Grid<19, 15> grid(sf::Vector2f(12.f, 8.f), window.getSize());

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