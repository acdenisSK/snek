#![deny(rust_2018_idioms)]

use sfml::graphics::*;
use sfml::system::*;
use sfml::window::*;

use rand::Rng;

use std::ops::{Index, IndexMut};

mod block;
mod snake;
mod utils;

use block::{Block, BlockType, BLOCK_LEN};
use snake::{Snake, SnekError};
use utils::{Direction, random_fruit_colour};

#[derive(Debug, Clone)]
pub struct Grid {
    x: usize,
    y: usize,
    blocks: Vec<Block>,
}

impl Grid {
    pub fn new(x: usize, y: usize, mut pos: Vector2f) -> Self {
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
    pub fn horizontal(&self) -> usize {
        self.x
    }

    #[inline]
    pub fn vertical(&self) -> usize {
        self.y
    }

    #[inline]
    pub fn len(&self) -> usize {
        self.blocks.len()
    }

    pub fn spawn_fruit(&mut self) {
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
