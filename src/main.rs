#![deny(rust_2018_idioms)]

use sfml::graphics::*;
use sfml::system::*;
use sfml::window::*;

use rand::Rng;

use std::error::Error;
use std::fmt;
use std::ops::{Index, IndexMut};

mod block;

use block::{Block, BlockType, BLOCK_LEN};

#[inline]
fn random_fruit_colour() -> Color {
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

#[derive(Debug, Clone)]
struct Grid {
    x: usize,
    y: usize,
    blocks: Vec<Block>,
}

impl Grid {
    fn new(x: usize, y: usize, mut pos: Vector2f) -> Self {
        let mut blocks = Vec::with_capacity(x * y);

        let first_x = pos.x;

        for _ in 0..y {
            for _ in 0..x {
                blocks.push(Block::new(pos));

                pos.x += BLOCK_LEN;
            }

            pos.x = first_x;
            pos.y += BLOCK_LEN;
        }

        Self { x, y, blocks }
    }

    #[inline]
    fn horizontal(&self) -> usize {
        self.x
    }

    #[inline]
    fn vertical(&self) -> usize {
        self.y
    }

    #[inline]
    fn len(&self) -> usize {
        self.blocks.len()
    }

    fn spawn_fruit(&mut self) {
        let mut rng = rand::thread_rng();
        let mut pos = rng.gen::<usize>() % self.len();

        while self.blocks[pos].is_occupied() {
            pos = rng.gen::<usize>() % self.len();
        }

        let block = &mut self.blocks[pos];

        block.set_type(BlockType::OccupiedFruit);
        block.set_colour(random_fruit_colour());
    }
}

impl Drawable for Grid {
    #[inline]
    fn draw<'a: 'shader, 'texture, 'shader, 'shader_texture>(
        &'a self,
        target: &mut dyn RenderTarget,
        _: RenderStates<'texture, 'shader, 'shader_texture>,
    ) {
        for block in &self.blocks {
            target.draw(block);
        }
    }
}

impl Index<usize> for Grid {
    type Output = Block;

    #[inline]
    fn index(&self, v: usize) -> &Self::Output {
        &self.blocks[v]
    }
}

impl IndexMut<usize> for Grid {
    #[inline]
    fn index_mut(&mut self, v: usize) -> &mut Self::Output {
        &mut self.blocks[v]
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
enum Direction {
    Left,
    Right,
    Up,
    Down,
}

impl Direction {
    #[inline]
    fn opposite(&self) -> Self {
        match self {
            Direction::Left => Direction::Right,
            Direction::Right => Direction::Left,
            Direction::Up => Direction::Down,
            Direction::Down => Direction::Up,
        }
    }

    #[inline]
    fn to_coords(&self) -> (isize, isize) {
        match self {
            Direction::Left => (-1, 0),
            Direction::Right => (1, 0),
            Direction::Up => (0, -1),
            Direction::Down => (0, 1),
        }
    }
}

#[derive(Debug, Clone)]
enum SnekError {
    Motor(Direction),
    Collision,
    OutOfBounds,
}

impl fmt::Display for SnekError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            SnekError::Motor(_d) => f.write_str("cannot turn the opposite direction"),
            SnekError::Collision => f.write_str("collided with the snake's own body"),
            SnekError::OutOfBounds => f.write_str("cannot go outside the eating-ground"),
        }
    }
}

impl Error for SnekError {}

#[derive(Debug, Clone)]
struct Snake {
    head: usize,
    body: Vec<usize>,
    direction: Option<Direction>,
}

impl Snake {
    fn new(grid: &mut Grid) -> Self {
        let mut rng = rand::thread_rng();
        let pos = rng.gen::<usize>() % grid.len();

        grid[pos].set_type(BlockType::OccupiedSnake);

        Self {
            head: pos,
            body: Vec::new(),
            direction: None,
        }
    }

    fn assert_direction(&self, new_direction: Direction) -> Result<(), SnekError> {
        let direction = match self.direction {
            Some(d) => d,
            None => return Ok(()),
        };

        if new_direction == direction.opposite() || new_direction.opposite() == direction {
            Err(SnekError::Motor(new_direction))
        } else {
            Ok(())
        }
    }

    fn update_pos(grid: &mut Grid, old: &mut usize, new: usize) {
        grid[*old].set_type(BlockType::Vacant);

        *old = new;

        grid[*old].set_type(BlockType::OccupiedSnake);
    }

    fn r#move(&mut self, grid: &mut Grid) -> Result<(), SnekError> {
        let mut old_pos = self.head;
        let x = old_pos % grid.horizontal();
        let y = old_pos / grid.horizontal();

        let (h, v) = self.direction.as_ref().map_or((0, 0), |d| d.to_coords());

        let x = (x as isize + h) as usize;
        let y = (y as isize + v) as usize;

        if x >= grid.horizontal() || y >= grid.vertical() {
            return Err(SnekError::OutOfBounds);
        }

        let new_pos = x + y * grid.horizontal();

        if grid[new_pos].kind == BlockType::OccupiedSnake {
            return Err(SnekError::Collision);
        }

        let fruit_occupied = grid[new_pos].kind == BlockType::OccupiedFruit;

        Self::update_pos(grid, &mut self.head, new_pos);

        for pos in &mut self.body {
            let before = *pos;

            Self::update_pos(grid, pos, old_pos);

            old_pos = before;
        }

        if fruit_occupied {
            grid[new_pos].set_colour(Color::GREEN);

            self.add_body(grid);
        }

        Ok(())
    }

    fn add_body(&mut self, grid: &mut Grid) {
        let mut tail = if self.body.is_empty() {
            self.head
        } else {
            *self.body.last().unwrap()
        };

        let x = tail % grid.horizontal();
        let y = tail / grid.horizontal();

        let (h, v) = self.direction.as_ref().map_or((0, 0), |d| d.to_coords());

        let x = (x as isize - h) as usize;
        let y = (y as isize - v) as usize;

        tail = x + y * grid.horizontal();

        grid[tail].set_type(BlockType::OccupiedSnake);

        self.body.push(tail);
    }

    #[inline]
    fn set_direction(&mut self, new: Direction) -> Result<(), SnekError> {
        self.assert_direction(new)?;

        self.direction = Some(new);

        Ok(())
    }

    #[inline]
    fn has_direction(&self) -> bool {
        self.direction.is_some()
    }
}

const TITLE: &str = "Snek";

#[derive(Debug, Clone)]
enum State {
    Start,
    InProgress,
    Error(SnekError),
    End,
}

fn main() {
    let mut window = RenderWindow::new(
        (500, 400),
        TITLE,
        Style::DEFAULT,
        &ContextSettings::default(),
    );

    let mut grid = Grid::new(19, 15, Vector2f::new(12.0, 8.0));
    let mut snake = Snake::new(&mut grid);

    let mut state = State::Start;
    let mut clock = Clock::start();

    let mut movement_seconds = 0.0;
    let mut spawn_seconds = 0.0;

    while window.is_open() {
        while let Some(ev) = window.poll_event() {
            match ev {
                Event::Closed => window.close(),
                Event::KeyPressed { code, .. } => {
                    let direction = match code {
                        Key::Left => Direction::Left,
                        Key::Right => Direction::Right,
                        Key::Up => Direction::Up,
                        Key::Down => Direction::Down,
                        _ => continue,
                    };

                    if let Err(err) = snake.set_direction(direction) {
                        window.set_title(&format!("{}: {}", TITLE, err));
                    }
                }
                _ => {}
            }
        }

        let seconds = clock.restart().as_seconds();

        movement_seconds += seconds;
        spawn_seconds += seconds;

        match state {
            State::Start => {
                if snake.has_direction() {
                    state = State::InProgress;
                }
            }
            State::InProgress => {
                if spawn_seconds >= 5.0 {
                    grid.spawn_fruit();

                    spawn_seconds = 0.0;
                }

                if movement_seconds >= 0.25 {
                    if let Err(err) = snake.r#move(&mut grid) {
                        state = State::Error(err);
                    }

                    movement_seconds = 0.0;
                }
            }
            State::Error(err) => {
                window.set_title(&format!("{}: {} - over!", TITLE, err));
                state = State::End;
                continue;
            }
            State::End => {}
        }

        window.clear(&Color::WHITE);
        window.draw(&grid);
        window.display();
    }
}
