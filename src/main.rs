#![deny(rust_2018_idioms)]

use sfml::graphics::*;
use sfml::system::*;
use sfml::window::*;

use rand::{Rng, RngCore};

use std::error::Error;
use std::fmt;
use std::ops::{Index, IndexMut};

static FRUIT_COLORS: [Color; 3] = [
    Color::RED,
    Color::BLUE,
    Color {
        r: 0xFF,
        g: 0xA5,
        b: 0x00,
        a: 0x00,
    },
];

const BLOCK_LEN: f32 = 25.0;

#[derive(Debug, Clone, Copy, PartialEq)]
enum BlockType {
    Vacant,
    OccupiedSnake,
    OccupiedFruit,
}

#[derive(Debug, Clone)]
struct Block {
    pub kind: BlockType,
    pub colour: Color,
    vertices: VertexArray,
}

impl Block {
    fn new(pos: Vector2f) -> Self {
        let mut block = Self::default();

        block.vertices[0].position = pos;
        block.vertices[1].position = Vector2f::new(pos.x + BLOCK_LEN, pos.y);
        block.vertices[2].position = pos + BLOCK_LEN;
        block.vertices[3].position = Vector2f::new(pos.x, pos.y + BLOCK_LEN);
        block.vertices[4].position = pos;

        block.vertices[0].color = block.colour;
        block.vertices[2].color = block.colour;
        block.vertices[2].color = block.colour;
        block.vertices[3].color = block.colour;
        block.vertices[4].color = block.colour;

        block
    }

    fn set_colour(&mut self, colour: Color) {
        for i in 0..self.vertices.vertex_count() {
            self.vertices[i].color = colour;
        }
    }

    fn set_type(&mut self, kind: BlockType) {
        match kind {
            BlockType::Vacant => self.vertices.set_primitive_type(PrimitiveType::LineStrip),
            BlockType::OccupiedSnake | BlockType::OccupiedFruit => {
                self.vertices.set_primitive_type(PrimitiveType::Quads)
            }
        }
    }

    fn is_occupied(&self) -> bool {
        match self.kind {
            BlockType::OccupiedSnake | BlockType::OccupiedFruit => true,
            BlockType::Vacant => false,
        }
    }
}

impl Default for Block {
    fn default() -> Self {
        Block {
            kind: BlockType::Vacant,
            colour: Color::GREEN,
            vertices: VertexArray::new(PrimitiveType::LineStrip, 5),
        }
    }
}

impl Drawable for Block {
    fn draw<'a: 'shader, 'texture, 'shader, 'shader_texture>(
        &'a self,
        target: &mut dyn RenderTarget,
        states: RenderStates<'texture, 'shader, 'shader_texture>,
    ) {
        target.draw_vertex_array(&self.vertices, states);
    }
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
        let mut pos = 0;
        let mut rng = rand::thread_rng();

        while self.blocks[pos].is_occupied() {
            pos = rng.gen::<usize>() % self.blocks.len();
        }

        let block = &mut self.blocks[pos];

        block.set_type(BlockType::OccupiedFruit);
        block.set_colour(random_fruit_colour());

        #[inline]
        fn random_fruit_colour() -> Color {
            let index = rand::thread_rng().gen::<usize>() % FRUIT_COLORS.len();

            FRUIT_COLORS[index]
        }
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

#[inline]
fn to_coords(v: Vector2u, horizontal: usize) -> usize {
    let x = v.x as usize;
    let y = v.y as usize;

    x + y * horizontal
}

impl Index<Vector2u> for Grid {
    type Output = Block;

    #[inline]
    fn index(&self, v: Vector2u) -> &Self::Output {
        &self.blocks[to_coords(v, self.horizontal())]
    }
}

impl IndexMut<Vector2u> for Grid {
    #[inline]
    fn index_mut(&mut self, v: Vector2u) -> &mut Self::Output {
        let horizontal = self.horizontal();
        &mut self.blocks[to_coords(v, horizontal)]
    }
}

#[inline]
fn add_vectors(left: Vector2u, right: Vector2i) -> Vector2u {
    Vector2u::new(
        ((left.x as i32) + right.x) as u32,
        ((left.y as i32) + right.y) as u32,
    )
}

#[inline]
fn sub_vectors(left: Vector2u, right: Vector2i) -> Vector2u {
    add_vectors(left, -right)
}

#[derive(Debug, Clone, Copy, PartialEq)]
#[repr(i32)]
enum HorizontalDirection {
    Left = -1,
    Right = 1,
}

impl HorizontalDirection {
    #[inline]
    fn opposite(&self) -> Self {
        match self {
            HorizontalDirection::Left => HorizontalDirection::Right,
            HorizontalDirection::Right => HorizontalDirection::Left,
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
#[repr(i32)]
enum VerticalDirection {
    Up = -1,
    Down = 1,
}

impl VerticalDirection {
    #[inline]
    fn opposite(&self) -> Self {
        match self {
            VerticalDirection::Up => VerticalDirection::Down,
            VerticalDirection::Down => VerticalDirection::Up,
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
enum Direction {
    Horizontal(HorizontalDirection),
    Vertical(VerticalDirection),
}

impl Direction {
    #[inline]
    fn to_pos(&self) -> Vector2i {
        let (x, y) = match self {
            Direction::Horizontal(d) => (*d as i32, 0),
            Direction::Vertical(d) => (0, *d as i32),
        };

        Vector2i::new(x, y)
    }

    #[inline]
    fn opposite(&self) -> Self {
        match self {
            Direction::Horizontal(d) => Direction::Horizontal(d.opposite()),
            Direction::Vertical(d) => Direction::Vertical(d.opposite()),
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

struct Snake<'a> {
    grid: &'a mut Grid,
    head: Vector2u,
    body: Vec<Vector2u>,
    direction: Option<Direction>,
}

impl<'a> Snake<'a> {
    fn new(grid: &'a mut Grid) -> Self {
        let mut rng = rand::thread_rng();
        let x = rng.gen::<usize>() % grid.horizontal();
        let y = rng.gen::<usize>() % grid.vertical();

        let pos = Vector2u::new(x as u32, y as u32);

        grid[pos].set_type(BlockType::OccupiedSnake);

        Self {
            grid,
            head: pos,
            body: Vec::new(),
            direction: None,
        }
    }

    #[inline]
    fn direction_pos(&self) -> Vector2i {
        self.direction
            .as_ref()
            .map_or(Vector2i::new(0, 0), |d| d.to_pos())
    }

    fn assert_position(&self, pos: Vector2u) -> Result<(), SnekError> {
        if pos.x as usize >= self.grid.horizontal() || pos.y as usize >= self.grid.vertical() {
            Err(SnekError::OutOfBounds)
        } else if self.grid[pos].kind == BlockType::OccupiedSnake {
            Err(SnekError::Collision)
        } else {
            Ok(())
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

    fn update_pos(grid: &mut Grid, old: &mut Vector2u, new: Vector2u) {
        grid[*old].set_type(BlockType::Vacant);

        *old = new;

        grid[*old].set_type(BlockType::OccupiedSnake);
    }

    fn r#move(&mut self) -> Result<(), SnekError> {
        let pos = self.direction_pos();

        let mut old_pos = self.head;
        let new_pos = add_vectors(old_pos, pos);

        self.assert_position(new_pos)?;

        let fruit_occupied = self.grid[new_pos].kind == BlockType::OccupiedFruit;

        Self::update_pos(self.grid, &mut self.head, new_pos);

        for pos in &mut self.body {
            let before = *pos;

            Self::update_pos(self.grid, pos, old_pos);

            old_pos = before;
        }

        if fruit_occupied {
            self.grid[new_pos].set_colour(Color::GREEN);

            self.add_body();
        }

        Ok(())
    }

    fn add_body(&mut self) {
        let mut tail = if self.body.is_empty() {
            self.head
        } else {
            *self.body.last().unwrap()
        };

        tail = sub_vectors(tail, self.direction_pos());
        self.grid[tail].set_type(BlockType::OccupiedSnake);

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
    End(SnekError),
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

    loop {
        // events
        while let Some(ev) = window.poll_event() {
            match ev {
                Event::Closed => {
                    window.close();
                    return;
                }
                Event::KeyPressed { code, .. } => {
                    let direction = match code {
                        Key::Left => Direction::Horizontal(HorizontalDirection::Left),
                        Key::Right => Direction::Horizontal(HorizontalDirection::Right),
                        Key::Up => Direction::Vertical(VerticalDirection::Up),
                        Key::Down => Direction::Vertical(VerticalDirection::Down),
                        _ => continue,
                    };

                    if let Err(err) = snake.set_direction(direction) {
                        window.set_title(&format!("{}: {}", TITLE, err));
                    }
                }
                _ => {}
            }
        }

        match state {
            State::Start => {
                if snake.has_direction() {
                    state = State::InProgress;
                }
            }
            State::InProgress => {
                let seconds = clock.restart().as_seconds();

                movement_seconds += seconds;
                spawn_seconds += seconds;

                if spawn_seconds >= 5.0 {
                    grid.spawn_fruit();

                    spawn_seconds = 0.0;
                }

                if movement_seconds >= 0.25 {
                    if let Err(err) = snake.r#move() {
                        state = State::End(err);
                    }

                    movement_seconds = 0.0;
                }

                window.clear(&Color::WHITE);
                window.draw(&grid);
                window.display();
            }
            State::End(err) => {
                window.set_title(&format!("{}: {} - over!", TITLE, err));
            }
        }
    }
}
