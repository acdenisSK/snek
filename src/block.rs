use sfml::graphics::{Color, Drawable, PrimitiveType, RenderStates, RenderTarget, VertexArray};
use sfml::system::Vector2f;

pub const BLOCK_LEN: f32 = 25.0;

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum BlockType {
    Vacant,
    OccupiedSnake,
    OccupiedFruit,
}

#[derive(Debug, Clone)]
pub struct Block {
    pub kind: BlockType,
    pub colour: Color,
    vertices: VertexArray,
}

impl Block {
    pub fn new(pos: Vector2f) -> Self {
        let mut block = Self::default();

        block.vertices[0].position = pos;
        block.vertices[1].position = Vector2f::new(pos.x + BLOCK_LEN, pos.y);
        block.vertices[2].position = pos + BLOCK_LEN;
        block.vertices[3].position = Vector2f::new(pos.x, pos.y + BLOCK_LEN);
        block.vertices[4].position = pos;

        block.vertices[0].color = block.colour;
        block.vertices[1].color = block.colour;
        block.vertices[2].color = block.colour;
        block.vertices[3].color = block.colour;
        block.vertices[4].color = block.colour;

        block
    }

    pub fn set_colour(&mut self, colour: Color) {
        for i in 0..self.vertices.vertex_count() {
            self.vertices[i].color = colour;
        }
    }

    pub fn set_type(&mut self, kind: BlockType) {
        match kind {
            BlockType::Vacant => self.vertices.set_primitive_type(PrimitiveType::LineStrip),
            BlockType::OccupiedSnake | BlockType::OccupiedFruit => {
                self.vertices.set_primitive_type(PrimitiveType::Quads)
            }
        }

        self.kind = kind;
    }

    #[inline]
    pub fn is_occupied(&self) -> bool {
        match self.kind {
            BlockType::OccupiedSnake | BlockType::OccupiedFruit => true,
            BlockType::Vacant => false,
        }
    }
}

impl Default for Block {
    #[inline]
    fn default() -> Self {
        Block {
            kind: BlockType::Vacant,
            colour: Color::GREEN,
            vertices: VertexArray::new(PrimitiveType::LineStrip, 5),
        }
    }
}

impl Drawable for Block {
    #[inline]
    fn draw<'a: 'shader, 'texture, 'shader, 'shader_texture>(
        &'a self,
        target: &mut dyn RenderTarget,
        states: RenderStates<'texture, 'shader, 'shader_texture>,
    ) {
        target.draw_vertex_array(&self.vertices, states);
    }
}
