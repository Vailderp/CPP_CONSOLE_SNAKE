#include <iostream>
#include <Windows.h>
#include <vector>
#define CLEAR_CONSOLE "\x1B[2J\x1B[H"
#define FPS 5

//Interface
#define WALL '#'
#define NONE ' '
#define APPLE 'A'
#define SNAKE_BODY 'O'
#define SNAKE_HEAD '@'

constexpr bool __fastcall IsItChars(const char this_char, const char it_char)
{
	if (this_char == it_char)
	{
		return true;
	}
	return false;
}

template<typename... __VA_CHARS__>
constexpr bool __fastcall  IsItChars(const char this_char, const char it_char, __VA_CHARS__... chars)
{
	if (this_char == it_char && IsItChars(this_char, chars...))
	{
		return true;
	}
	return false;
}

template<typename T>
struct Vector2
{
	Vector2(T x = T(), T y = T()) : x(x), y(y) {}
	T x;
	T y;
};

using world_size_t = unsigned int;
using Vector2ui = Vector2<world_size_t>;

class Drawable
{
	friend class Canvas;
protected:
	virtual ~Drawable() = default;
	virtual void draw(Canvas& canvas) = 0;
};

class Canvas
{
private:
	char** canvas_;
	const Vector2ui world_size_;
	_STD string str_canvas_;
public:
	Canvas(const Vector2ui world_size) : world_size_(world_size)
	{
		canvas_ = reinterpret_cast<char**>(malloc(sizeof(char*) * world_size.y));
		for (world_size_t y = 0; y < world_size.y; y++)
		{
			canvas_[y] = reinterpret_cast<char*>(malloc(sizeof(char) * world_size.x));
		}
	}

	Vector2ui toCanvasCoord(Vector2ui coord)
	{
		coord.x %= world_size_.x;
		coord.y %= world_size_.y;
		return coord;
	}

	void draw(Drawable& drawable)
	{
		drawable.draw(*this);
	}

	void clear()
	{
		for (unsigned int y = 0; y < world_size_.y; y++)
		{
			for (unsigned int x = 0; x < world_size_.x; x++)
			{
				canvas_[y][x] = ' ';
			}
		}
	}

	void display()
	{
		str_canvas_ = "";
		for (unsigned int y = 0; y < world_size_.y; y++)
		{
			for (unsigned int x = 0; x < world_size_.x; x++)
			{
				str_canvas_ += canvas_[y][x];
			}
			str_canvas_ += "\n";
		}
		str_canvas_ += "\n";
		_STD cout << str_canvas_;
	}

	void set(const world_size_t x, const world_size_t y, const char val)
	{
		canvas_[y][x] = val;
	}

	char& get(const world_size_t x, const world_size_t y)
	{
		return canvas_[y][x];
	}

	void set(const Vector2ui coord, const char val)
	{
		canvas_[coord.y][coord.x] = val;
	}

	char& get(const Vector2ui coord)
	{
		return canvas_[coord.y][coord.x];
	}

	const Vector2ui& getWorldSize()
	{
		return world_size_;
	}
};


inline int  __fastcall rand(const int min, const int max, unsigned int seed)
{
	seed = 8253729 * seed + 2396403;
	int rand = (8253729 * seed + 2396403) % (max + min + 1) - min;
	while (rand < min || rand > max)
	{
		rand = (8253729 * seed * 2396403) % (max + min) - min;
	}
	return rand;
}

inline int  __fastcall rand(const int min, const int max)
{
	return rand(min, max, static_cast<int>(8253729 * (std::clock() / static_cast<float>(CLOCKS_PER_SEC))));
}

class Wall : public Drawable
{
	void draw(Canvas& canvas) override
	{
		for (int y = 0; y < canvas.getWorldSize().y; y++)
		{
			canvas.set(0, y, WALL);
			canvas.set(canvas.getWorldSize().x - 1, y, WALL);
		}
		for (int x = 0; x < canvas.getWorldSize().x; x++)
		{
			canvas.set(x, 0, WALL);
			canvas.set(x, canvas.getWorldSize().y - 1, WALL);
		}
	}
};

class Snake : public Drawable
{
	std::vector<Vector2ui> positions_hist_ = { Vector2ui(2, 2) };
	Vector2ui movement_ = { 1, 0 };

	void update()
	{
		if (GetKeyState('D') & 0x8000)
		{
			if (movement_.x != -1)
			{
				movement_ = Vector2ui(1, 0);
			}
		}
		else if (GetKeyState('A') & 0x8000)
		{
			if (movement_.x != 1)
			{
				movement_ = Vector2ui(-1, 0);
			}
		}
		else if (GetKeyState('W') & 0x8000)
		{
			if (movement_.y != 1)
			{
				movement_ = Vector2ui(0, -1);
			}
		}
		else if (GetKeyState('S') & 0x8000)
		{
			if (movement_.y != -1)
			{
				movement_ = Vector2ui(0, 1);
			}
		}
		for (size_t i = positions_hist_.size() - 1; i > 0; i--)
		{
			positions_hist_[i].x = positions_hist_[i - 1].x;
			positions_hist_[i].y = positions_hist_[i - 1].y;
		}
		positions_hist_[0].x += movement_.x;
		positions_hist_[0].y += movement_.y;
	}

	void draw(Canvas& canvas) override
	{
		update();
		canvas.set(canvas.toCanvasCoord(positions_hist_[0]), SNAKE_HEAD);
		for (int i = 1; i < positions_hist_.size(); i++)
		{
			canvas.set(canvas.toCanvasCoord(positions_hist_[i]), SNAKE_BODY);
		}
		if (IsItChars(canvas.get(canvas.toCanvasCoord(positions_hist_[0])), SNAKE_BODY))
		{
			std::cout << "You lose!" << std::endl;
			exit(-1);
		}
	}
public:
	const Vector2ui& getHeadPosition()
	{
		return positions_hist_[0];
	}
	void onAppleEat()
	{
		positions_hist_.push_back(
			Vector2ui(positions_hist_[positions_hist_.size() - 1].x - movement_.x,
				positions_hist_[positions_hist_.size() - 1].y - movement_.y));
	}

};

class Apple : public Drawable
{
	Snake& snake_;
	int last_time = 0;
	Vector2ui apple_positions_;
	void draw(Canvas& canvas) override
	{
		if ((std::clock() / CLOCKS_PER_SEC) % 5 == 0 && std::clock() / CLOCKS_PER_SEC != last_time)
		{
			last_time = std::clock() / CLOCKS_PER_SEC;
			apple_positions_.x = rand(0, canvas.getWorldSize().x - 1);
			apple_positions_.y = rand(0, canvas.getWorldSize().y - 1);
			while (IsItChars(canvas.get(apple_positions_.x, apple_positions_.y), SNAKE_BODY, SNAKE_HEAD, WALL))
			{
				apple_positions_.x = rand(0, canvas.getWorldSize().x - 1);
				apple_positions_.y = rand(0, canvas.getWorldSize().y - 1);
			}
		}
		if (canvas.toCanvasCoord(snake_.getHeadPosition()).x == apple_positions_.x && canvas.toCanvasCoord(snake_.getHeadPosition()).y == apple_positions_.y)
		{
			apple_positions_.x = 0;
			apple_positions_.y = 0;
			snake_.onAppleEat();
		}
		if (apple_positions_.x != 0 && apple_positions_.y != 0)
		{
			canvas.set(apple_positions_.x, apple_positions_.y, APPLE);
		}
	}
public:
	Apple(Snake& snake) : snake_(snake) {}
};

int ___main()
{

	//Initialize world
	Canvas canvas({ 50, 25 });
	Wall wall;
	Snake snake;
	Apple apple(snake);

	//Main cycle
	while (true)
	{
		canvas.clear();
		canvas.draw(wall);
		canvas.draw(apple);
		canvas.draw(snake);
		canvas.display();
		Sleep(1000.F / FPS);
	}

	return 0;
}
