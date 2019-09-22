use rand::Rng;
use sfml::graphics::Color;

#[inline]
pub fn random_fruit_colour() -> Color {
    static FRUIT_COLORS: [Color; 3] = [
        Color::RED,
        Color::BLUE,
        // Orange.
        Color {
            r: 0xFF,
            g: 0xA5,
            b: 0x00,
            a: 0x00,
        },
    ];

    let index = rand::thread_rng().gen::<usize>() % FRUIT_COLORS.len();

    FRUIT_COLORS[index]
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum Direction {
    Left,
    Right,
    Up,
    Down,
}

impl Direction {
    #[inline]
    pub fn opposite(&self) -> Self {
        match self {
            Direction::Left => Direction::Right,
            Direction::Right => Direction::Left,
            Direction::Up => Direction::Down,
            Direction::Down => Direction::Up,
        }
    }

    #[inline]
    pub fn to_coords(&self) -> (isize, isize) {
        match self {
            Direction::Left => (-1, 0),
            Direction::Right => (1, 0),
            Direction::Up => (0, -1),
            Direction::Down => (0, 1),
        }
    }
}
