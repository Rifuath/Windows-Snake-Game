#include <cstdio>
#include <cstdlib>
#include <windows.h>
#include <stdint.h> 

//why? since static can be very confusing and matters where you use it.
//the locals will be active like a toggle which remembers the past.
#define internal        static  // local to translation unit
#define local_persist   static  // local to a block
#define global_variable static  // global variable

//My types since i hate writing _t after it all the time.
typedef  uint8_t    uint8;
typedef  uint16_t   uint16;
typedef  uint32_t   uint32;
typedef  uint64_t   uint64;

typedef  int8_t     int8;
typedef  int16_t    int16;
typedef  int32_t    int32;
typedef  int64_t    int64;

typedef  int32_t    bool32;

struct win32_bitmap_buffer 
{
    int Width;
    int Height;
    int BytesPerPixel;
    int Pitch;
    BITMAPINFO Info;
    void *Memory;
};

struct win32_window_dimension
{
    int Width, Height;
};

typedef enum
{
    DIR_NONE        = 0,                    // 0000
    DIR_UP          = 1 << 0,               // 0001
    DIR_DOWN        = 1 << 1,               // 0010
    DIR_LEFT        = 1 << 2,               // 0100
    DIR_RIGHT       = 1 << 3,               // 1000
}DirectionPoll;


typedef enum
{
    NoStyle = 0,
    Outline,
    Fill
}Style;


// Global For now at least might change later
global_variable bool GlobalRunning;
global_variable win32_bitmap_buffer GlobalBackBuffer;

global_variable float GlobalFPS;

global_variable int GlobalWidth;
global_variable int GlobalHeight;
global_variable int GlobalClientWidth;
global_variable int GlobalClientHeight;

//Grid
global_variable int GlobalGridSize;
global_variable int GlobalGridDimension;
// starting corner of the grid
global_variable int GlobalGridX;
global_variable int GlobalGridY;

global_variable int GlobalCursorX;
global_variable int GlobalCursorY;

global_variable DirectionPoll GlobalDirection;


// NOTE: this returns actuall the client area of the window. not the full width. 
internal win32_window_dimension Win32GetWindowDimension(HWND Window)
{
    win32_window_dimension result;
    RECT ClientRect;
    GetClientRect( Window, &ClientRect);

    result.Height = ClientRect.bottom - ClientRect.top;
    result.Width = ClientRect.right - ClientRect.left;
    return result;
}

//==================================================
// ERROR HANDLER

int Win32ShowError( const char* Title, const char* Message)
{
    return(MessageBoxA(0, Message, Title, MB_ICONERROR ));
}

int Win32ShowWarning( const char* Title, const char* Message)
{
    return(MessageBoxA(0, Message, Title, MB_ICONWARNING | MB_OKCANCEL));
}
//==================================================

internal void Win32Constraint(int * src, int lowest, int highest)
{
    if(*src < lowest) 
    {
        *src = highest;
    }
    else if(*src > highest)
    {
        *src = lowest;
    }

}

//This Function sets a single pixel in the given x, y cordinates
internal void Win32SetPixel(win32_bitmap_buffer *Buffer, int x, int y, uint32 color)
{
    uint32 *Memory = (uint32*) Buffer->Memory;

    //Checking the cordinate given is in the buffer. if not ignore it now.
    if (x >= 0 && x < GlobalClientWidth && y >= 0 && y < GlobalClientHeight) {

        Memory[y * GlobalClientWidth + x] = color;
    }
}

internal void Win32DrawLine(win32_bitmap_buffer *Buffer, int x0, int y0, int x1, int y1, uint32 color)
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2; // error value e_xy

    while (1) {
        Win32SetPixel(Buffer, x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

internal void Win32DrawCircle(win32_bitmap_buffer * Buffer, int centerX, int centerY, int radius, uint32 color)
{
    int x = radius;
    int y = 0;
    int decisionOver2 = 1 - x; // Decision criterion divided by 2

    while (y <= x) {
        // Draw the eight symmetric points
        Win32SetPixel(Buffer,centerX + x, centerY + y, color);
        Win32SetPixel(Buffer,centerX - x, centerY + y, color);
        Win32SetPixel(Buffer,centerX + x, centerY - y, color);
        Win32SetPixel(Buffer,centerX - x, centerY - y, color);
        Win32SetPixel(Buffer,centerX + y, centerY + x, color);
        Win32SetPixel(Buffer,centerX - y, centerY + x, color);
        Win32SetPixel(Buffer,centerX + y, centerY - x, color);
        Win32SetPixel(Buffer,centerX - y, centerY - x, color);

        y++;
        if (decisionOver2 <= 0) {
            decisionOver2 += 2 * y + 1; // Move to the next point vertically
        } else {
            x--;
            decisionOver2 += 2 * (y - x) + 1; // Move diagonally
        }
    }
}

internal void Win32DrawSquare(win32_bitmap_buffer * Buffer, int x, int y, int width, uint32 color, Style style)
{
    for(int i = y; i < y+width; i++)
    {
        for(int j = x; j < x+width; j++)
        {
            if(i == y || i == (y+width) - 1)
            {
                //outline
                Win32SetPixel(Buffer, j, i, color);
            }
            else if(j == x || j == (x+width)-1)
            {
                //outline
                Win32SetPixel(Buffer, j, i, color);
            }
            else if(style == Fill)
            {
                Win32SetPixel(Buffer, j, i, color);
            }
        }
    }

}
internal void Win32DrawGrid(win32_bitmap_buffer *Buffer, int X, int Y, int gridSize,  int gridDimension, uint32 color)
{

    //Win32DrawGrid(&GlobalBackBuffer,GlobalGridX, GlobalGridY, GlobalGridSize, GlobalGridDimension, 0xff333344);
    // Draw vertical lines
    for (int x = X; x <= X + gridDimension; x += gridSize)
    {
        Win32DrawLine(Buffer, x, Y, x, Y + gridDimension, color);
    }

    // Draw horizontal lines
    for (int y = Y; y <= Y + gridDimension; y += gridSize)
    {
        Win32DrawLine(Buffer, X, y, X + gridDimension, y, color);
    }
    /*
       for(int y = 0; y < GlobalClientHeight; y+=gridSize)
       {
       for(int x = 0; x < GlobalClientWidth; x+=gridSize)
       {
       Win32DrawSquare(Buffer , x, y, gridSize, color, Outline);

       }
       }
     */
}




internal void BitRendererclear(win32_bitmap_buffer *Buffer, uint32 color)
{
    uint8 *Row = (uint8 *)Buffer->Memory;

    for(int y = 0; y < Buffer->Height; y++)
    {
        uint32 *Pixel = (uint32*)Row;

        for(int x = 0; x < Buffer->Width; x++)
        {
            //*Pixel++ = ((Red << 16) | (Green << 8) | Blue);
            *Pixel++ = color;
        }
        Row += Buffer->Pitch;
    }

}


//basically render the buffer for the resize and then use gdi to render.
internal void Win32ResizeDIBSection(win32_bitmap_buffer *Buffer, int Width, int Height)
{

    //TODO(Rifey): Bulletproof this
    // Maybe free after it fails, don't free first 

    if(Buffer->Memory)
    {
        //free if exists, note '0' as a size, since the VirtualAlloc remembers the size
        // so specifying the size of actually not necessary after creation.
        // Note: VirtualFree frees the page we allocated using VirtualAlloc.( windows specific ).
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;
    Buffer->BytesPerPixel = 4;
    Buffer->Pitch = Buffer->Width * Buffer->BytesPerPixel;

    // global variable init 'BitmapInfo'
    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height; // why? windows!! topleft drawing is by -
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    // alloc bitmap memory
    // r g b = 24 bytes but need bits alligned so 32 bit 
    // each carries 8 bytes so 8 + 8 + 8 is the rgb but need 8 more for 32 bit.
    // which makes in total 4, 8 bytes

    int BitmapMemorySize = (Buffer->Width * Buffer->Height) * Buffer->BytesPerPixel;

    //VirtualAlloc is windows specific func for getting an actual page memory.
    // In general for c/cpp malloc is used but it also calls this function so 
    // It's much fast/memory_effiecient in thoery. I could be wrong!!.

    // i use here commit and read/write only. there is other stuff as well.
    // so to conclude i am asking for memory and assigning the address of that to 
    // BitmapMemory for creating our own buffer.
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);


    //TODO(rifey): Clear this to black probably. meaning clear the buffer to all black
    //BitRendererclear(Buffer, 0x14141f);


}

//Why not a pointer ClientRect. just for easing the compiler and since RECT is not big.
internal void Win32DisplayBufferInWindow(win32_bitmap_buffer *Buffer,
        HDC DeviceContext, int WindowWidth, int WindowHeight)
{
    //TODO: Aspect Ration Correction

    // For now stretch DIbits into the client area of the windows.
    StretchDIBits(DeviceContext,
            /*
               X, Y, Width, Height, 
               X, Y, Width, Height,  // source cordinates and dimension we use the same for now.
             */
            0, 0, WindowWidth, WindowHeight,  // destination
            0, 0, Buffer->Width, Buffer->Height, // source
            Buffer->Memory,
            &Buffer->Info,
            DIB_RGB_COLORS, SRCCOPY);
}

//================================================================================
// MainWindowCallback function 
//================================================================================

LRESULT CALLBACK Win32MainWindowCallback(HWND Window, 
        UINT Message, 
        WPARAM WParam, 
        LPARAM LParam)
{
    LRESULT result = 0;

    switch (Message) {
        case WM_CREATE:
            {
                ShowCursor(0);
            }break;
        case WM_SIZE:
            {
                //In here when the window is being resized, this case will be true
                //since i am dealing with drawing on the screen, it's important to update the 
                //screen when resized happens.
                // in this case we want to get the client area size of the window and pass the
                // width and height to the DIB section.
                // !!UPDATE: i have moved the Resize DIB section to the entry point. with a fixed Dimension.

                //win32_window_dimension  Dimension  = Win32GetWindowDimension(Window);
                //Win32ResizeDIBSection(&GlobalBackBuffer, Dimension.Width, Dimension.Height);
            } break;

        case WM_DESTROY:
            {
                //NOTE(Rifey): Handle with a error message and try to recreate window
                GlobalRunning = false;
            } break;

        case WM_MOUSEMOVE:
            {
                GlobalCursorX = (int)(short)LOWORD(LParam);
                GlobalCursorY = (int)(short)HIWORD(LParam);
            } break;
            // keyboard inputs

        case WM_SYSKEYUP:
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        case WM_KEYUP:
            {
                uint32 VKCode = WParam;
                bool WasDown = ((LParam & (1 << 30)) != 0 );
                bool IsDown = ((LParam & (1 << 31)) == 0 );

                if( WasDown != IsDown)
                {
                    if(VKCode == 'J')
                    {
                        if(IsDown)
                        {
                            if(GlobalDirection != DIR_UP)
                            {
                                GlobalDirection = DIR_DOWN;
                            }
                        }
                    }
                    else if(VKCode == 'K')
                    {
                        if(IsDown)
                        {
                            if(GlobalDirection != DIR_DOWN)
                            {
                                GlobalDirection = DIR_UP;
                            }
                        }


                    }
                    else if(VKCode == 'H')
                    {
                        if(IsDown)
                        {
                            if(GlobalDirection != DIR_RIGHT)
                            {
                                GlobalDirection = DIR_LEFT;
                            }
                        }


                    }
                    else if(VKCode == 'L')
                    {
                        if(IsDown)
                        {
                            if(GlobalDirection != DIR_LEFT)
                            {
                                GlobalDirection = DIR_RIGHT;
                            }

                        }


                    }
                    else if(VKCode == 'A')
                    {
                    }
                    else if(VKCode == 'E')
                    {
                    }
                    else if(VKCode == VK_UP)
                    {
                    }
                    else if(VKCode == VK_DOWN)
                    {
                    }
                    else if(VKCode == VK_LEFT)
                    {
                    }
                    else if(VKCode == VK_RIGHT)
                    {
                    }
                    else if(VKCode == VK_ESCAPE)
                    {
                        if(IsDown)
                        {
                            if(Win32ShowWarning("Exit", "Do you want exit?") == IDOK)
                            {
                                GlobalRunning = false;
                            }
                        }
                        if(WasDown)
                        {
                        }
                    }
                    else if(VKCode == VK_SPACE)
                    {
                    }

                    bool32 AltKeyWasDown = (LParam & (1 << 29));
                    if( (VKCode == VK_F4) && AltKeyWasDown)
                    {
                        if(Win32ShowWarning("Exit", "Do you want exit?") == IDOK)
                        {
                            GlobalRunning = false;
                        }
                    }
                }

            }break;

            // end of keyboard inputs
        case WM_CLOSE:
            {
                //NOTE(Rifey): Handle with a message to user
                GlobalRunning = false;
            } break;

        case WM_ACTIVATEAPP:
            {
            } break;

            /*
               case WM_PAINT:
               {
            // In here the painting happens on the client area of the window we create.
            // since i want to make my own rendering, i will feed the cordinates of the 
            // paint struct with width and height to update the window accordingly.
            // all these structs are windows specific.
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint( Window, &Paint );

            win32_window_dimension  Dimension  = Win32GetWindowDimension(Window);

            Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);
            EndPaint( Window, &Paint);

            } break;
             */

        default:
            {
                //            OutputDebugStringA("default\n");
                result = DefWindowProc(Window, Message, WParam, LParam);
            } break;
    }

    return(result);
}

//================================================================================

struct Fruit
{
    int X, Y;
    int Dimension;
    uint32 Color;

    Fruit() = delete;
    Fruit(int dimension, uint32 color) : Dimension(dimension), Color(color)
    {
        ReSpawn();
    }

    void ReSpawn()
    {
        X = (rand() % GlobalGridDimension) + GlobalGridX;
        X -= (X % Dimension);

        Y = (rand() % GlobalGridDimension) + GlobalGridY;
        Y -= (Y % Dimension);

    }
    void Render()
    {
        Win32DrawSquare(&GlobalBackBuffer, X, Y, Dimension, Color, Fill);

    }

};

//snake
struct SnakeNode
{
    int X, Y;
    uint32 color;
    DirectionPoll Dir;
    struct SnakeNode* next;

};
struct Snake
{
    //holds the global direction variable
    DirectionPoll *Direction;
    SnakeNode *SnakeHead;
    SnakeNode *SnakeTail;
    int X;
    int Y;
    int Dimension; 
    int Size;
    bool32 Alive;



    Snake() = delete;
    Snake(int x, int y, DirectionPoll *dir, int dimension) : X(x), Y(y), Direction(dir),Dimension(dimension)
    {
        //lets constraint the cordinates
        Win32Constraint(&X, GlobalGridX, (GlobalGridDimension - Dimension) + GlobalGridX );
        Win32Constraint(&Y, GlobalGridY, (GlobalGridDimension - Dimension) + GlobalGridY);


        Size = 2;
        Alive = 1;
        SnakeHead = (SnakeNode*) malloc(sizeof(SnakeNode));
        SnakeTail =  (SnakeNode*) malloc(sizeof(SnakeNode));

        SnakeHead->color = 0xffed8796,
            SnakeHead->X = X;
        SnakeHead->Y = Y;
        SnakeHead->Dir = *Direction;
        SnakeHead->next = SnakeTail;

        SnakeTail->color = 0xffeed49f,
            SnakeTail->X = X;
        SnakeTail->Y = Y - Dimension;
        SnakeTail->Dir = SnakeHead->Dir;
        SnakeTail->next = NULL;

    }

    bool32 Ate(Fruit *fruit)
    {
        if( fruit->X == X && fruit->Y == Y)
        { 
            if((Size % 20) == 0)
            {
                GlobalFPS++;
            }

            return 1;
        }
        return 0;
    }

    bool32 CheckSelfColision()
    {
        SnakeNode *body = SnakeHead->next;
        while(body != NULL)
        {
            if( (body->X == SnakeHead->X) && (body->Y == SnakeHead->Y) )
            {
                return 1;
            }
            body = body->next;
        }
        return 0;
    }

    void Add()
    {
        SnakeNode *temp = (SnakeNode*) malloc(sizeof(SnakeNode));

        //we are adding to the end of the list. so check the tail dir as well.
        if(SnakeTail->Dir == DIR_UP)
        {
            temp->X = SnakeTail->X;
            temp->Y = SnakeTail->Y + Dimension;
        }
        else if(SnakeTail->Dir == DIR_DOWN)
        {
            temp->X = SnakeTail->X;
            temp->Y = SnakeTail->Y - Dimension;

        }
        else if(SnakeTail->Dir == DIR_LEFT)
        {
            temp->X = SnakeTail->X + Dimension;
            temp->Y = SnakeTail->Y;

        }
        else if(SnakeTail->Dir == DIR_RIGHT)
        {
            temp->X = SnakeTail->X - Dimension;
            temp->Y = SnakeTail->Y;

        }
        //lets constraint the cordinates
        Win32Constraint(&temp->X, GlobalGridX, (GlobalGridDimension - Dimension) + GlobalGridX );
        Win32Constraint(&temp->Y, GlobalGridY, (GlobalGridDimension - Dimension) + GlobalGridY);



        temp->color = 0xffeed49f;
        temp->next = NULL;

        SnakeTail->next = temp;
        SnakeTail = temp;

        Size++;

    }


    void Reset()
    {
        // Generate random position for the snake within the game boundaries
        int x = (rand() % GlobalClientWidth) + GlobalGridX;
        x -= x % Dimension; // Align with the grid
        int y = (rand() % GlobalClientHeight) + GlobalGridY;
        y -= y % Dimension; // Align with the grid

        // Reset the snake size
        Size = 2;

        // Free the existing snake body (if any)
        SnakeTail = SnakeHead->next;
        SnakeNode *temp = SnakeTail->next;

        SnakeNode *temp2;
        while (temp != NULL)
        {
            temp2 = temp->next;
            free(temp);
            temp = temp2;
        }

        // Reinitialize head and tail
        SnakeHead->X = x;
        SnakeHead->Y = y;
        SnakeHead->Dir = DIR_UP;
        SnakeHead->next = SnakeTail;

        SnakeTail->X = x;
        SnakeTail->Y = y - Dimension; // Position tail just behind the head
        SnakeTail->Dir = SnakeHead->Dir;
        SnakeTail->next = NULL;

        // Set snake to alive
        Alive = 1;

        // Reset the direction to some default (e.g., DIR_RIGHT)
        *Direction = DIR_UP;

    }


    void Update()
    {
        switch (*Direction) {
            case DIR_UP:
                {
                    Y-=Dimension;
                } break;
            case DIR_RIGHT:
                {
                    X+=Dimension;
                } break;
            case DIR_DOWN:
                {
                    Y+=Dimension;
                } break;
            case DIR_LEFT:
                {
                    X-=Dimension;
                } break;
            default:
                {} break;
        }

        //lets constraint the cordinates
        Win32Constraint(&X, GlobalGridX, (GlobalGridDimension - Dimension) + GlobalGridX );
        Win32Constraint(&Y, GlobalGridY, (GlobalGridDimension - Dimension) + GlobalGridY);
        // update the X, Y according to the direction
        int prevX = X;
        int prevY = Y;
        DirectionPoll prevDir = *Direction;


        SnakeNode *temp = SnakeHead;
        while(temp != NULL)
        {
            int tempX = temp->X;
            int tempY = temp->Y;
            DirectionPoll tempDir = temp->Dir;

            temp->X = prevX;
            temp->Y = prevY;
            temp->Dir = prevDir;

            prevX = tempX;
            prevY = tempY;
            prevDir = tempDir;
            temp = temp->next;

        }



    }

    void Render()
    {
        SnakeNode *temp = SnakeHead;
        while(temp != NULL)
        {

            Win32DrawSquare(&GlobalBackBuffer, temp->X, temp->Y, Dimension, temp->color, Fill);
            temp = temp->next;

        }

    }

    ~Snake()
    {
        //Free The Nodes
        SnakeNode * temp;
        while(SnakeHead != NULL)
        {
            temp = SnakeHead->next;
            free(SnakeHead);
            SnakeHead = temp;
        }


    }


};

//================================================================================
void Win32UpdateFrame(HWND window, HDC DeviceContext) 
{
    // Render here

    // rending my own abstracted buffer rendering

    //Win32BitRendererRipgl(&GlobalBackBuffer, 0, GlobalSnakeY); 
    //Win32SetPixel(&GlobalBackBuffer, GlobalCursorX, GlobalCursorY, 0xffaaffee);
    //Win32DrawLine(&GlobalBackBuffer, 0, 100,  100, 0, 0xffff0000);
    BitRendererclear(&GlobalBackBuffer, 0xff14141f);
    //Win32DrawSquare(&GlobalBackBuffer, GlobalSnakeX, GlobalSnakeY, 20, 0xff89b4fa, Fill);
    //Win32DrawCircle(&GlobalBackBuffer, GlobalCursorX, GlobalCursorY, 2, 0xffaaffee);
    local_persist Fruit fruit(GlobalGridSize, 0xffff0000);
    local_persist Snake snake( (GlobalGridDimension / 2) % GlobalGridSize, (GlobalGridDimension / 2) % GlobalGridSize, &GlobalDirection, GlobalGridSize);


    snake.Update();

    if(snake.CheckSelfColision() == 1 )
    {
        snake.Reset();
    }

    if(snake.Ate(&fruit) == 1)
    {
        snake.Add();
        fruit.ReSpawn();
    }

    snake.Render();
    fruit.Render();


    Win32DrawGrid(&GlobalBackBuffer, GlobalGridX, GlobalGridY, GlobalGridSize, GlobalGridDimension, 0xff333344);


    win32_window_dimension Dimension = Win32GetWindowDimension(window);
    Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext,  Dimension.Width, Dimension.Height);



    // Calculate sleep time to achieve target FPS
    float sleeptime = 1000 / GlobalFPS;
    Sleep(sleeptime);


}
//================================================================================
//  Entry Point
//================================================================================


int CALLBACK WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
{


    GlobalFPS = 10;
    GlobalWidth  = 800;
    GlobalHeight = 800;

    GlobalGridSize = 20;
    GlobalGridDimension = 500;


  GlobalDirection = DIR_UP;

    WNDCLASSA WindowClass = {0};

    WindowClass.style = CS_HREDRAW|CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback; 
    WindowClass.hInstance = Instance;
    //WindowClass.hIcon = 0; 
    WindowClass.lpszClassName = "SnakeWindowClass"; 

    //register the window class and handle if failed
    if(!RegisterClass(&WindowClass))
    {
        Win32ShowError("Error", "Failed to Register WindowClass");
        return -1;
    }

    //lets try to find the middle of the entire screen 
    int WinX = (GetSystemMetrics(SM_CXSCREEN) / 2);
    int WinY = (GetSystemMetrics(SM_CYSCREEN) / 2);
    // and finally adjust the cordinates with according 
    // to the width and height of the window we create
    WinX -=  (GlobalWidth / 2) ;
    WinY -=  (GlobalHeight / 2) ;


    HWND Window = 
        CreateWindowExA(
                0,
                WindowClass.lpszClassName,
                "Snake",
                WS_CAPTION|WS_VISIBLE,
                WinX, //x
                WinY, //y
                GlobalWidth, // width
                GlobalHeight, // height
                0, 0,
                Instance, 0);

    //Handle the Window if any Error
    if(!Window)
    {
        Win32ShowError("Error", "Failed to Create Window");
        return -1;
    }

    // get the client dimension
    win32_window_dimension  Dimension  = Win32GetWindowDimension(Window);

    GlobalClientWidth = Dimension.Width;
    GlobalClientHeight = Dimension.Height;
    //resize the buffer according to the client width / height
    Win32ResizeDIBSection(&GlobalBackBuffer, GlobalClientWidth, GlobalClientHeight);

    // Calculate the starting X position for the grid to be centered
    GlobalGridX = (GlobalClientWidth - GlobalGridDimension) / 2;
    GlobalGridX -= GlobalGridX % GlobalGridSize;
    // Calculate the starting Y position for the grid to be centered
    GlobalGridY = (GlobalClientHeight - GlobalGridDimension) / 2;
    GlobalGridY -= GlobalGridY % GlobalGridSize;


    HDC DeviceContext = GetDC(Window);


    GlobalRunning = true;
    while(GlobalRunning)
    {

        MSG Message;
        while( PeekMessageA(&Message, 0, 0, 0, PM_REMOVE) )
        {
            if( Message.message == WM_QUIT)
            {
                GlobalRunning = false;
            }
            TranslateMessage(&Message);
            DispatchMessageA(&Message);
        }
        //rendering using frame rate
        Win32UpdateFrame(Window, DeviceContext);


    }

    ReleaseDC(Window, DeviceContext);
    return(0);
}


