# Windows Snake Game

This is a simple Snake game implemented in C++ for Windows using a custom software renderer. The game uses the Windows API for window management and input handling, while the rendering is done using a custom bitmap buffer.

## Features

- Custom software renderer
- Grid-based gameplay
- Snake grows when eating fruit
- Collision detection (self-collision and boundaries)
- Increasing difficulty (speed increases as the snake grows)

## Requirements

- Windows operating system
- Microsoft Visual C++ compiler (cl.exe)
- Windows SDK (for user32.lib and gdi32.lib)

## Building and Running

1. Clone the repository or download the source code.
2. Open a command prompt with the Visual C++ build tools environment set up.
3. Navigate to the project's root directory.
4. Run the build script:
    build.bat
    This will compile the game and place the executable in the `build` directory.
5. Run the executable from the `build` directory.

### Build Script Details

The `build.bat` script does the following:

- Creates a `build` directory if it doesn't exist
- Compiles the source code using the Microsoft C++ compiler (cl)
- Links against user32.lib and gdi32.lib for Windows API support
- Generates debug symbols (-Zi flag)
- Uses full path in diagnostics (-FC flag)

There's also a commented-out release build configuration for optimized builds.

## Controls

- Use H, J, K, L keys to control the snake:
- H: Move left
- J: Move down
- K: Move up
- L: Move right
- ESC: Pause/Exit game

## Implementation Details

- The game uses a custom software renderer for drawing graphics.
- The snake and fruit are represented as squares on a grid.
- The game window is centered on the screen.
- The game loop handles input, updates game state, and renders the frame.
- Collision detection is implemented for self-collision and boundaries.

## Project Structure
Snake/
│
├── src/
│   └── snake.cpp
│
├── build/
│   └── (compiled executable and debug files)
│
└── build.bat

## Future Improvements

- Add a score display
- Implement a game over screen
- Add sound effects
- Optimize rendering for better performance
- Add different game modes or power-ups

## License

MIT License

## Acknowledgements

This project was created as a learning exercise in game development and software rendering using the Windows API.
