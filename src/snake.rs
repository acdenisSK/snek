use super::Grid;
use crate::utils::Direction;
use crate::block::BlockType;
use rand::Rng;
use sfml::graphics::Color;

use std::error::Error;
use std::fmt;

#[derive(Debug, Clone)]
pub enum SnekError {
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
pub struct Snake {
    head: usize,
    body: Vec<usize>,
    direction: Option<Direction>,
}

impl Snake {
    #[inline]
    pub fn new(grid: &mut Grid) -> Self {
        let mut rng = rand::thread_rng();
        let pos = rng.gen::<usize>() % grid.len();

        grid[pos].set_type(BlockType::OccupiedSnake);

        Self {
            head: pos,
            body: Vec::new(),
            direction: None,
        }
    }

    #[inline]
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

    #[inline]
    fn update_pos(grid: &mut Grid, old: &mut usize, new: usize) {
        grid[*old].set_type(BlockType::Vacant);

        *old = new;

        grid[*old].set_type(BlockType::OccupiedSnake);
    }

    pub fn r#move(&mut self, grid: &mut Grid) -> Result<(), SnekError> {
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
    pub fn set_direction(&mut self, new: Direction) -> Result<(), SnekError> {
        self.assert_direction(new)?;

        self.direction = Some(new);

        Ok(())
    }

    #[inline]
    pub fn has_direction(&self) -> bool {
        self.direction.is_some()
    }
}
